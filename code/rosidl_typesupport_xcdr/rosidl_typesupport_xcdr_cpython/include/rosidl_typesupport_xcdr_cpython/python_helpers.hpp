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

#ifndef ROSIDL_TYPESUPPORT_XCDR_CPYTHON__PYTHON_HELPERS_HPP_
#define ROSIDL_TYPESUPPORT_XCDR_CPYTHON__PYTHON_HELPERS_HPP_

/// @file python_helpers.hpp
/// @brief pybind11 helpers shared by the runtime and generated code.
///
/// The generated per-message field walkers use pybind11 directly to
/// manipulate the Python experimental message representation
/// (rosidl_runtime_cpython containers).  This header centralises the
/// glue that is common to every generated callback:
///
///  - Python exception → rcutils error translation
///  - RawBuffer C API import and external-region wrapping
///  - typed access to Scalar / String / WString / Array / Sequence
///  - building Python ExternalStorage descriptors from an XCDR accessor
///
/// GIL discipline: every entry point that touches Python objects uses
/// pybind11's `py::gil_scoped_acquire` (RAII, reentrant) before any
/// Python API call, because middleware threads do not hold the GIL.
/// Generated callbacks follow the same rule.

#include <Python.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "rcutils/error_handling.h"
#include "rcutils/types/rcutils_ret.h"
#include "rosidl_runtime_c/experimental/memory.h"
#include "rosidl_runtime_cpython/raw_buffer.h"
#include "rosidl_typesupport_xcdr_cpython/visibility_control.h"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

namespace rosidl_typesupport_xcdr_cpython
{

// ============================================================================
// Error translation
// ============================================================================

/// True while the Python interpreter is alive (initialised and not finalising).
/**
 * Safe to call from C-linkage callbacks that may run after Py_Finalize started
 * (e.g. shared_ptr deleters on handles destroyed during shutdown): DECREFs
 * must be skipped then, because acquiring the GIL would crash.
 *
 * The public ``Py_IsFinalizing`` name exists on Python >= 3.13; before that
 * it is the private ``_Py_IsFinalizing``.
 */
inline bool interpreter_alive()
{
#if PY_VERSION_HEX >= 0x030D0000
  return Py_IsInitialized() != 0 && Py_IsFinalizing() == 0;
#else
  return Py_IsInitialized() != 0 && _Py_IsFinalizing() == 0;
#endif
}

/// Translate an active pybind11 exception into an rcutils error message.
/**
 * Extracts the message, sets an rcutils error, and returns RCUTILS_RET_ERROR.
 * The pending Python exception state is left intact: the caught
 * py::error_already_set restores it on destruction, so rclpy can still
 * propagate the original exception once control returns to Python.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
rcutils_ret_t translate_pybind_error(const char * context);

// ============================================================================
// RawBuffer C API
// ============================================================================

/// Wrap a rosidl_memory_region_t as a non-owning external RawBuffer.
/**
 * The region must outlive the returned object.  Returns a new reference
 * owned by the caller.  Delegates to the pybind11 wrapper that lives in
 * rosidl_runtime_cpython (which owns RawBuffer).  Raises on failure.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::object raw_buffer_from_region(const rosidl_memory_region_t & region);

// ============================================================================
// Payload / string helpers
// ============================================================================

/// Borrow a Python object from a type-erased payload pointer.
/**
 * Generated code receives message payloads and constraints as ``void *`` /
 * ``const void *`` (actually ``PyObject *``); this centralises the cast.
 */
inline py::object py_borrow(const void * ptr)
{
  return py::reinterpret_borrow<py::object>(
    static_cast<PyObject *>(const_cast<void *>(ptr)));
}

/// Assign raw bytes to a String / WString via its assign() method.
/**
 * \param[in] str_obj  Python String/WString container.
 * \param[in] data     Bytes to assign (UTF-8 for String, UTF-16-LE for
 *                     WString; the container's assign() accepts bytes).
 * \param[in] size     Byte count.
 *
 * Raises on failure; the enclosing extern "C" callback translates it.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
void string_assign_bytes(
  py::handle str_obj, const void * data, size_t size);

// ============================================================================
// ExternalStorage construction
// ============================================================================

/// Set `ext_storage.block` to an external RawBuffer over *region*.
/**
 * Raises on failure (see string_assign_bytes).
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
void external_storage_set_block(
  py::handle ext_storage, const rosidl_memory_region_t & region);

/// Construct a message instance: `MsgClass(_storage=ext_storage, _init=SKIP)`.
/**
 * Returns a new reference to the constructed Python message.  Raises on
 * failure (see string_assign_bytes).
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::object message_from_external_storage(
  py::handle msg_class, py::handle ext_storage);

/// Return the `MessageInitialization.SKIP` enum value.
/**
 * Enum members are process-lifetime singletons, so a fresh attribute lookup
 * each call returns the same object with no caching or reference leak.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::object message_initialization_skip();

}  // namespace rosidl_typesupport_xcdr_cpython

#endif  // ROSIDL_TYPESUPPORT_XCDR_CPYTHON__PYTHON_HELPERS_HPP_
