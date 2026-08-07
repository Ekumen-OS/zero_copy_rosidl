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

py::object
raw_buffer_from_region(const rosidl_memory_region_t & region)
{
  // The wrapper lives in rosidl_runtime_cpython (the package that owns
  // RawBuffer); this typesupport just imports it.
  py::gil_scoped_acquire acquire;
  py::object module = py::module_::import("rosidl_runtime_cpython._raw_buffer_cpp");
  py::object from_region = module.attr("from_region");
  return from_region(
    py::cast(reinterpret_cast<uintptr_t>(region.location.address)),
    py::cast(region.size));
}

// ============================================================================
// String / ExternalStorage helpers
// ============================================================================

void
string_assign_bytes(py::handle str_obj, const void * data, size_t size)
{
  py::object assign = str_obj.attr("assign");
  // The container's assign() accepts bytes-like objects.  Build a bytes
  // object without copying: bytes(b) copies once into the container, which
  // is the intended deserialization path.
  py::object py_bytes = py::bytes(static_cast<const char *>(data), size);
  assign(py_bytes);
}

void
external_storage_set_block(
  py::handle ext_storage, const rosidl_memory_region_t & region)
{
  py::object block = raw_buffer_from_region(region);
  ext_storage.attr("block") = block;
}

py::object
message_initialization_skip()
{
  // Enum members are process-lifetime singletons, so a fresh lookup each
  // call returns the same object — no caching, no reference leak.
  py::gil_scoped_acquire acquire;
  return py::module_::import("rosidl_runtime_cpython.message_initialization")
         .attr("MessageInitialization").attr("SKIP");
}

py::object
message_from_external_storage(py::handle msg_class, py::handle ext_storage)
{
  return msg_class(
    py::arg("_storage") = ext_storage,
    py::arg("_init") = message_initialization_skip());
}

}  // namespace rosidl_typesupport_xcdr_cpython
