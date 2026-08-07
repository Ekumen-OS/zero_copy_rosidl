// Copyright 2026 Ekumen Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rosidl_typesupport_xcdr_cpython/python_helpers.hpp"

#include <cstring>
#include <string>

namespace rosidl_typesupport_xcdr_cpython
{

// ============================================================================
// Error translation
// ============================================================================

rcutils_ret_t
translate_python_error(const char * context)
{
  // Must be called with the GIL held.
  if (PyErr_Occurred()) {
    PyObject * type = nullptr;
    PyObject * value = nullptr;
    PyObject * traceback = nullptr;
    PyErr_Fetch(&type, &value, &traceback);
    PyErr_NormalizeException(&type, &value, &traceback);
    std::string message;
    if (value != nullptr) {
      PyObject * str = PyObject_Str(value);
      if (str != nullptr) {
        const char * utf8 = PyUnicode_AsUTF8(str);
        if (utf8 != nullptr) {
          message = utf8;
        }
        Py_DECREF(str);
      }
    }
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(traceback);
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "%s: %s", context != nullptr ? context : "Python error",
      message.empty() ? "unknown error" : message.c_str());
  } else {
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "%s: unknown Python error", context != nullptr ? context : "Python error");
  }
  return RCUTILS_RET_ERROR;
}

rcutils_ret_t
translate_pybind_error(const char * context)
{
  try {
    throw;
  } catch (const py::error_already_set & e) {
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "%s: %s", context != nullptr ? context : "Python error", e.what());
  } catch (const std::exception & e) {
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "%s: %s", context != nullptr ? context : "C++ error", e.what());
  } catch (...) {
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "%s: unknown error", context != nullptr ? context : "error");
  }
  return RCUTILS_RET_ERROR;
}

// ============================================================================
// RawBuffer C API
// ============================================================================

bool
ensure_raw_buffer_capi()
{
  // Static local initialisation is thread-safe (C++11 magic statics).
  // RawBuffer_ImportCAPI uses PyCapsule_Import, which acquires the GIL
  // internally, so this is safe from any thread.
  static const bool imported = []() {
      py::gil_scoped_acquire acquire;
      return RawBuffer_ImportCAPI() == 0;
    }();
  if (!imported) {
    RCUTILS_SET_ERROR_MSG(
      "RawBuffer C API import failed; is rosidl_runtime_cpython installed?");
  }
  return imported;
}

py::object
raw_buffer_from_region(const rosidl_memory_region_t & region)
{
  py::gil_scoped_acquire acquire;
  if (!ensure_raw_buffer_capi()) {
    return py::object();
  }
  PyObject * buf = RawBuffer_FromRegion(&region);
  if (buf == nullptr) {
    translate_python_error("raw_buffer_from_region");
    return py::object();
  }
  // Steal the new reference into a py::object.
  return py::reinterpret_steal<py::object>(buf);
}

// ============================================================================
// Attribute helpers
// ============================================================================

py::object
py_get_attr(py::handle obj, const char * name, const char * context)
{
  // obj is borrowed; the returned py::object is a *borrowed* handle into it.
  // It is only valid while obj is alive.  Callers that need to keep it must
  // keep the owning object alive.
  try {
    return obj.attr(name);
  } catch (const py::error_already_set &) {
    translate_pybind_error(context != nullptr ? context : "py_get_attr");
    return py::object();
  }
}

rcutils_ret_t
py_set_attr(py::handle obj, const char * name, py::handle value, const char * context)
{
  try {
    obj.attr(name) = value;
    return RCUTILS_RET_OK;
  } catch (const py::error_already_set &) {
    return translate_pybind_error(context != nullptr ? context : "py_set_attr");
  }
}

// ============================================================================
// Container accessors
// ============================================================================

py::object
scalar_get_value(py::handle scalar, const char * context)
{
  try {
    return scalar.attr("value");
  } catch (const py::error_already_set &) {
    translate_pybind_error(context != nullptr ? context : "scalar_get_value");
    return py::object();
  }
}

rcutils_ret_t
scalar_set_value(py::handle scalar, py::handle value, const char * context)
{
  return py_set_attr(scalar, "value", value, context);
}

rcutils_ret_t
string_assign_bytes(
  py::handle str_obj, const void * data, size_t size, const char * context)
{
  try {
    py::object assign = str_obj.attr("assign");
    // The container's assign() accepts bytes-like objects.  Build a bytes
    // object without copying: bytes(b) copies once into the container, which
    // is the intended deserialization path.
    py::object py_bytes = py::bytes(static_cast<const char *>(data), size);
    assign(py_bytes);
    return RCUTILS_RET_OK;
  } catch (const py::error_already_set &) {
    return translate_pybind_error(context != nullptr ? context : "string_assign_bytes");
  }
}

py::array
container_numpy(py::handle container, const char * context)
{
  try {
    py::object numpy = container.attr("numpy");
    return py::reinterpret_borrow<py::array>(numpy());
  } catch (const py::error_already_set &) {
    translate_pybind_error(context != nullptr ? context : "container_numpy");
    return py::array();
  }
}

rcutils_ret_t
sequence_resize(py::handle seq, py::ssize_t new_size, const char * context)
{
  try {
    py::object resize = seq.attr("resize");
    resize(new_size);
    return RCUTILS_RET_OK;
  } catch (const py::error_already_set &) {
    return translate_pybind_error(context != nullptr ? context : "sequence_resize");
  }
}

// ============================================================================
// ExternalStorage construction
// ============================================================================

py::object
external_storage_new(py::handle msg_class, const char * context)
{
  try {
    py::object storage_class = msg_class.attr("ExternalStorage");
    return storage_class();
  } catch (const py::error_already_set &) {
    translate_pybind_error(context != nullptr ? context : "external_storage_new");
    return py::object();
  }
}

rcutils_ret_t
external_storage_set_block(
  py::handle ext_storage, const rosidl_memory_region_t & region, const char * context)
{
  py::object block = raw_buffer_from_region(region);
  if (block.is_none()) {
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "%s: failed to wrap storage block",
        context != nullptr ? context : "external_storage_set_block");
    return RCUTILS_RET_ERROR;
  }
  return py_set_attr(ext_storage, "block", block, context);
}

rcutils_ret_t
external_storage_set_member(
  py::handle ext_storage, const char * name, py::handle value, const char * context)
{
  try {
    py::object members = ext_storage.attr("members");
    members.attr(name) = value;
    return RCUTILS_RET_OK;
  } catch (const py::error_already_set &) {
    return translate_pybind_error(context != nullptr ? context : "external_storage_set_member");
  }
}

py::object
external_storage_members(py::handle ext_storage, const char * context)
{
  try {
    return ext_storage.attr("members");
  } catch (const py::error_already_set &) {
    translate_pybind_error(context != nullptr ? context : "external_storage_members");
    return py::object();
  }
}

PyObject *
message_initialization_skip()
{
  // Cache the MessageInitialization.SKIP enum value.  The reference is
  // intentionally leaked (heap-allocated raw pointer): it must outlive any
  // static destruction order relative to Python finalisation.
  static PyObject * const skip = []() -> PyObject * {
      py::gil_scoped_acquire acquire;
      py::object obj = py::module_::import("rosidl_runtime_cpython.message_initialization")
        .attr("MessageInitialization").attr("SKIP");
      return obj.release().ptr();
    }();
  return skip;
}

py::object
message_from_external_storage(
  py::handle msg_class, py::handle ext_storage, const char * context)
{
  try {
    return msg_class(
      py::arg("_storage") = ext_storage,
      py::arg("_init") = py::reinterpret_borrow<py::object>(message_initialization_skip()));
  } catch (const py::error_already_set &) {
    translate_pybind_error(context != nullptr ? context : "message_from_external_storage");
    return py::object();
  }
}

py::object
message_new(py::handle msg_class, const char * context)
{
  try {
    return msg_class();
  } catch (const py::error_already_set &) {
    translate_pybind_error(context != nullptr ? context : "message_new");
    return py::object();
  }
}

}  // namespace rosidl_typesupport_xcdr_cpython
