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

/// Translate the pending Python exception into an rcutils error message.
/**
 * Returns RCUTILS_RET_ERROR.  Clears the Python error state.
 * Safe to call with the GIL held.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
rcutils_ret_t translate_python_error(const char * context);

/// Same as translate_python_error but for use after a pybind11 exception.
/**
 * Catches any active py::error_already_set, extracts the message, sets an
 * rcutils error, and returns RCUTILS_RET_ERROR.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
rcutils_ret_t translate_pybind_error(const char * context);

// ============================================================================
// RawBuffer C API
// ============================================================================

/// Ensure the RawBuffer C API is imported from rosidl_runtime_cpython.
/**
 * Idempotent and thread-safe (GIL held internally).  Must complete before
 * raw_buffer_from_region() is used.  Generated construct/cast callbacks
 * invoke this lazily.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
bool ensure_raw_buffer_capi();

/// Wrap a rosidl_memory_region_t as a non-owning external RawBuffer.
/**
 * The region must outlive the returned object.  Returns a new reference
 * owned by the caller.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::object raw_buffer_from_region(const rosidl_memory_region_t & region);

// ============================================================================
// Attribute helpers
// ============================================================================

/// Read an attribute, returning a borrowed-reference py::object.
/**
 * On failure, sets an rcutils error and returns a disengaged object
 * (call py::object::is_none() to check).  The returned object is a
 * temporary handle: it must not outlive the owning object.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::object py_get_attr(py::handle obj, const char * name, const char * context);

/// Write an attribute.  Returns RCUTILS_RET_OK on success.
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
rcutils_ret_t py_set_attr(
  py::handle obj, const char * name, py::handle value, const char * context);

// ============================================================================
// Container accessors
// ============================================================================

/// Read a Scalar's value as a py::object (e.g. Python int / float / bool).
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::object scalar_get_value(py::handle scalar, const char * context);

/// Write a Scalar's value from a py::object.
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
rcutils_ret_t scalar_set_value(py::handle scalar, py::handle value, const char * context);

/// Assign raw bytes to a String / WString via its assign() method.
/**
 * \param[in] str_obj  Python String/WString container.
 * \param[in] data     Bytes to assign (UTF-8 for String, UTF-16-LE for
 *                     WString; the container's assign() accepts bytes).
 * \param[in] size     Byte count.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
rcutils_ret_t string_assign_bytes(
  py::handle str_obj, const void * data, size_t size, const char * context);

/// Return the live numpy view of a container (Scalar/String/Array/Sequence).
/**
 * Uses the container's .numpy() method.  The view is live: writes through
 * it are reflected in the container and vice versa.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::array container_numpy(py::handle container, const char * context);

/// Resize a primitive Sequence to *new_size* elements (via .resize()).
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
rcutils_ret_t sequence_resize(
  py::handle seq, py::ssize_t new_size, const char * context);

// ============================================================================
// ExternalStorage construction
// ============================================================================

/// Create a Python ExternalStorage instance for a message class.
/**
 * Calls `MsgClass.ExternalStorage()` and returns the new instance.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::object external_storage_new(py::handle msg_class, const char * context);

/// Set `ext_storage.block` to an external RawBuffer over *region*.
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
rcutils_ret_t external_storage_set_block(
  py::handle ext_storage, const rosidl_memory_region_t & region, const char * context);

/// Set a single-member descriptor: `ext_storage.members.<name> = value`.
/**
 * *value* must be a RawBuffer (or a list of RawBuffer/ExternalStorage for
 * complex members, prebuilt by the caller).  The caller passes the
 * already-constructed Python value.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
rcutils_ret_t external_storage_set_member(
  py::handle ext_storage, const char * name, py::handle value, const char * context);

/// Get the `ext_storage.members` dataclass (borrowed handle into ext_storage).
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::object external_storage_members(py::handle ext_storage, const char * context);

/// Construct a message instance: `MsgClass(_storage=ext_storage, _init=SKIP)`.
/**
 * Returns a new reference to the constructed Python message, or a disengaged
 * object on failure (rcutils error set).
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::object message_from_external_storage(
  py::handle msg_class, py::handle ext_storage, const char * context);

/// Construct a message instance with managed storage: `MsgClass()`.
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
py::object message_new(py::handle msg_class, const char * context);

/// Return the cached `MessageInitialization.SKIP` enum value.
/**
 * The reference is owned by the process (deliberately leaked) and is valid
 * for the lifetime of the interpreter.
 */
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
PyObject * message_initialization_skip();

}  // namespace rosidl_typesupport_xcdr_cpython

#endif  // ROSIDL_TYPESUPPORT_XCDR_CPYTHON__PYTHON_HELPERS_HPP_
