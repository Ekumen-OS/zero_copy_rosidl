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

#include "rosidl_typesupport_xcdr_cpython/message_type_support.hpp"

#include "rcutils/error_handling.h"
#include "rosidl_typesupport_xcdr_cpython/identifier.hpp"
#include "rosidl_typesupport_xcdr_cpython/python_helpers.hpp"

#include "xcdr_buffers/common/types.hpp"
#include "xcdr_buffers/serialization/reader.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

namespace rosidl_typesupport_xcdr_cpython
{

// ============================================================================
// C-linkage callback functions for the xcdr rosidl_message_xcdr_type_support_t
//
// Each function matches the function pointer signature in the xcdr struct.
// They receive `const void * inner` which is always a
// rosidl_message_xcdr_cpython_type_support_t * in this package.
//
// Message payloads are PyObject * (experimental Python message instances).
// Every callback that touches a message acquires the GIL via pybind11's
// py::gil_scoped_acquire, because middleware threads do not hold it.
// ============================================================================

namespace
{

extern "C" rcutils_ret_t
cpython_get_expected_size(
  const rosidl_message_xcdr_type_support_t * xcdr, size_t * size)
{
  if (nullptr == xcdr || nullptr == size) {
    RCUTILS_SET_ERROR_MSG("xcdr or size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (nullptr == impl->cached_layout) {
    // No cached layout means the expected size is not statically known
    // (unbounded type, or an unconstrained handle).  Report 0 instead of
    // failing: callers use 0 to mean "we don't know" (not plain, not
    // bounded) rather than an error.
    *size = 0;
    return RCUTILS_RET_OK;
  }
  *size = impl->cached_layout->total_size();
  return RCUTILS_RET_OK;
}

extern "C" rcutils_ret_t
cpython_get_message_size(
  const rosidl_message_xcdr_type_support_t * xcdr,
  const void * message, size_t * size)
{
  if (nullptr == xcdr || nullptr == message || nullptr == size) {
    RCUTILS_SET_ERROR_MSG("xcdr, message, or size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);

  // Preferred: use generated size computation callback.  It touches the
  // Python message (reads container sizes / external storage), so the GIL
  // must be held.
  if (nullptr != impl->compute_serialized_size) {
    py::gil_scoped_acquire acquire;
    return impl->compute_serialized_size(message, size);
  }

  // Fallback: serialize to temporary writer and measure (migration path).
  if (nullptr == impl->serialize_fields) {
    RCUTILS_SET_ERROR_MSG("serialize_fields callback not available");
    return RCUTILS_RET_ERROR;
  }
  py::gil_scoped_acquire acquire;
  xcdr_buffers::XCdrWriter writer;
  auto ret = impl->serialize_fields(const_cast<void *>(message), writer);
  if (ret != RCUTILS_RET_OK) {
    return RCUTILS_RET_ERROR;
  }
  auto buffer = writer.flush();
  *size = buffer.size();
  return RCUTILS_RET_OK;
}

extern "C" rcutils_ret_t
cpython_construct_message_at(
  const rosidl_message_xcdr_type_support_t * xcdr,
  rosidl_memory_region_t storage,
  void ** message)
{
  if (nullptr == xcdr || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("inner or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (nullptr == impl->construct_message) {
    RCUTILS_SET_ERROR_MSG("construct_message callback not available");
    return RCUTILS_RET_ERROR;
  }
  py::gil_scoped_acquire acquire;
  rosidl_runtime_cpp::MemoryRegion<void> cpp_storage(storage);
  return impl->construct_message(impl, cpp_storage, message);
}

extern "C" rcutils_ret_t
cpython_cast_message_at(
  const rosidl_message_xcdr_type_support_t * xcdr,
  rosidl_memory_region_t storage,
  void ** message)
{
  if (nullptr == xcdr || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("inner or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (nullptr == impl->cast_message) {
    RCUTILS_SET_ERROR_MSG("cast_message callback not available");
    return RCUTILS_RET_ERROR;
  }
  py::gil_scoped_acquire acquire;
  rosidl_runtime_cpp::MemoryRegion<void> cpp_storage(storage);
  return impl->cast_message(impl, cpp_storage, message);
}

extern "C" rcutils_ret_t
cpython_serialize_message_into(
  const rosidl_message_xcdr_type_support_t * xcdr,
  const void * message,
  rosidl_memory_region_t storage)
{
  if (nullptr == xcdr || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("xcdr or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (nullptr == impl->serialize_fields) {
    RCUTILS_SET_ERROR_MSG("serialize_fields callback not available");
    return RCUTILS_RET_ERROR;
  }
  py::gil_scoped_acquire acquire;
  rosidl_runtime_cpp::MemoryRegion<void> cpp_storage(storage);
  xcdr_buffers::XCdrWriter writer(
    tcb::span<uint8_t>(static_cast<uint8_t *>(cpp_storage.data()), cpp_storage.size()));
  auto ret = impl->serialize_fields(const_cast<void *>(message), writer);
  if (ret != RCUTILS_RET_OK) {
    return ret;
  }
  if (writer.has_error()) {
    RCUTILS_SET_ERROR_MSG("XCdrWriter reported buffer overflow");
    return RCUTILS_RET_ERROR;
  }
  return RCUTILS_RET_OK;
}

extern "C" rcutils_ret_t
cpython_deserialize_message_from(
  const rosidl_message_xcdr_type_support_t * xcdr,
  rosidl_memory_region_t storage,
  void * message)
{
  if (nullptr == xcdr || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("xcdr or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (nullptr == impl->deserialize_fields) {
    RCUTILS_SET_ERROR_MSG("deserialize_fields callback not available");
    return RCUTILS_RET_ERROR;
  }
  auto buffer_span = tcb::span<const uint8_t>(
    static_cast<const uint8_t *>(storage.location.address), storage.size);
  auto reader_result = xcdr_buffers::XCdrReader::wrap(buffer_span);
  if (!reader_result) {
    RCUTILS_SET_ERROR_MSG("Failed to create XCdrReader");
    return RCUTILS_RET_ERROR;
  }
  py::gil_scoped_acquire acquire;
  return impl->deserialize_fields(*reader_result, message);
}

extern "C" void
cpython_destroy_message(void * message)
{
  if (nullptr == message) {
    return;
  }
  // message is a PyObject *; DECREF requires the GIL.
  py::gil_scoped_acquire acquire;
  Py_DECREF(static_cast<PyObject *>(message));
}

extern "C" rosidl_memory_region_t
cpython_release_message(void * message)
{
  // Default no-op (returns empty region).  Overridden by codegen for
  // experimental messages that support release.
  (void)message;
  return rosidl_memory_region_t{{nullptr, 0}, 0};
}

extern "C" rosidl_memory_region_t
cpython_get_backing_storage(const void * message)
{
  // Default no-op (returns empty region).  Overridden by codegen for
  // experimental messages that support external storage.
  (void)message;
  return rosidl_memory_region_t{{nullptr, 0}, 0};
}

extern "C" rosidl_message_type_support_t *
cpython_create_constrained(
  const rosidl_message_xcdr_type_support_t * xcdr,
  const rosidl_message_type_constraints_t * constraints)
{
  if (nullptr == xcdr || nullptr == constraints) {
    RCUTILS_SET_ERROR_MSG("xcdr or constraints is nullptr");
    return nullptr;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (nullptr == impl) {
    RCUTILS_SET_ERROR_MSG("inner state is nullptr");
    return nullptr;
  }

  // Build constrained layout from type-specific constraints.  The
  // type_specific payload is a Python Msg.Constraints instance and the
  // generated build_constrained callback walks it via pybind11, so the GIL
  // must be held (middleware threads do not hold it).
  if (nullptr == impl->build_constrained) {
    RCUTILS_SET_ERROR_MSG("build_constrained callback not available");
    return nullptr;
  }
  std::shared_ptr<xcdr_buffers::XCdrStructLayout> layout;
  std::shared_ptr<rosidl_message_type_constraints_t> owned;
  {
    py::gil_scoped_acquire acquire;
    layout = impl->build_constrained(constraints->type_specific);
    if (!layout) {
      RCUTILS_SET_ERROR_MSG("Failed to build constrained layout");
      return nullptr;
    }

    // Clone constraints into owned storage.
    if (nullptr == impl->clone_constraints) {
      RCUTILS_SET_ERROR_MSG("clone_constraints callback not available");
      return nullptr;
    }
    owned = impl->clone_constraints(constraints);
    if (!owned) {
      RCUTILS_SET_ERROR_MSG("Failed to clone constraints");
      return nullptr;
    }
  }

  // Create new inner struct with the constrained layout and owned constraints.
  auto * new_inner = new rosidl_message_xcdr_cpython_type_support_t(*impl);
  new_inner->cached_layout = layout;
  new_inner->owned_constraints = owned;
  new_inner->is_dynamically_allocated = true;

  // Create new xcdr struct (copy base handle's outer table, set inner).
  auto * new_xcdr = new rosidl_message_xcdr_type_support_t(*xcdr);
  new_xcdr->inner = new_inner;

  // Create new handle.
  auto * new_handle = new rosidl_message_type_support_t();
  new_handle->typesupport_identifier = rosidl_typesupport_xcdr_cpython__identifier;
  new_handle->data = new_xcdr;
  new_handle->func = nullptr;
  new_handle->get_type_hash_func = xcdr_cpython_default_get_type_hash;
  new_handle->get_type_description_func = xcdr_cpython_default_get_type_description;
  new_handle->get_type_description_sources_func =
    xcdr_cpython_default_get_type_description_sources;

  return new_handle;
}

extern "C" void
cpython_destroy_constrained(rosidl_message_type_support_t * typesupport)
{
  if (nullptr == typesupport) {
    return;
  }
  auto * xcdr = static_cast<const rosidl_message_xcdr_type_support_t *>(
    typesupport->data);
  if (nullptr == xcdr) {
    return;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (nullptr != impl && impl->is_dynamically_allocated) {
    delete impl;
  }
  delete xcdr;
  delete typesupport;
}

extern "C" bool
cpython_compare_type_specific_constraints(const void * lhs, const void * rhs)
{
  (void)lhs;
  (void)rhs;
  return false;
}

extern "C" rcutils_ret_t
cpython_validate_message(
  const rosidl_message_xcdr_type_support_t * xcdr,
  const rosidl_message_type_constraints_t * constraints,
  const void * message,
  rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
  void * user_data)
{
  if (nullptr == xcdr || nullptr == message || nullptr == constraints) {
    RCUTILS_SET_ERROR_MSG("xcdr, constraints, or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (nullptr == impl->validate_fields) {
    // No validation callback — this type does not support per-field validation.
    if (constraints->strict) {
      RCUTILS_SET_ERROR_MSG("message validation requested but validate_fields is null");
      return RCUTILS_RET_ERROR;
    }
    return RCUTILS_RET_OK;
  }
  // Extract type_specific from constraints for the per-field callback.
  py::gil_scoped_acquire acquire;
  return impl->validate_fields(
    constraints->type_specific, message, report_cb, user_data);
}

extern "C" rosidl_memory_region_t
cpython_compact_message_in_place(
  const rosidl_message_xcdr_type_support_t * xcdr,
  void * message)
{
  rosidl_memory_region_t null_region = {{nullptr, 0}, 0};

  if (nullptr == xcdr || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("xcdr or message is nullptr");
    return null_region;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (nullptr == impl->compact_fields) {
    RCUTILS_SET_ERROR_MSG("compact_fields callback not available");
    return null_region;
  }

  py::gil_scoped_acquire acquire;
  return impl->compact_fields(
    message,
    impl->cached_layout.get());
}

extern "C" void
cpython_destroy_inner(const rosidl_message_xcdr_type_support_t * xcdr)
{
  if (nullptr == xcdr || nullptr == xcdr->inner) {
    return;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (impl->is_dynamically_allocated) {
    delete impl;
  }
}

extern "C" const rosidl_message_type_constraints_t *
cpython_get_constraints(const rosidl_message_xcdr_type_support_t * xcdr)
{
  if (nullptr == xcdr || nullptr == xcdr->inner) {
    return nullptr;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpython_type_support_t *>(xcdr->inner);
  if (impl->owned_constraints) {
    return impl->owned_constraints.get();
  }
  return nullptr;
}

// ---- Default fallbacks for type hash / type description ----

extern "C" const rosidl_type_hash_t *
xcdr_cpython_default_get_type_hash(const rosidl_message_type_support_t * /*type_support*/)
{
  static const rosidl_type_hash_t zero_hash = {ROSIDL_TYPE_HASH_VERSION_UNSET, {0}};
  return &zero_hash;
}

extern "C" const rosidl_runtime_c__type_description__TypeDescription *
xcdr_cpython_default_get_type_description(const rosidl_message_type_support_t * /*type_support*/)
{
  return nullptr;
}

extern "C" const rosidl_runtime_c__type_description__TypeSource__Sequence *
xcdr_cpython_default_get_type_description_sources(
  const rosidl_message_type_support_t * /*type_support*/)
{
  return nullptr;
}

}  // anonymous namespace

// ============================================================================
// Prototype xcdr table
// ============================================================================

const rosidl_message_xcdr_type_support_t *
get_xcdr_cpython_type_support_prototype()
{
  static const rosidl_message_xcdr_type_support_t prototype = {
    nullptr,                      // inner (overridden by callers)
    cpython_get_expected_size,
    cpython_get_message_size,
    cpython_construct_message_at,
    cpython_cast_message_at,
    cpython_serialize_message_into,
    cpython_deserialize_message_from,
    cpython_destroy_message,      // overridden by codegen for experimental
    cpython_release_message,      // overridden by codegen for experimental
    cpython_get_backing_storage,  // overridden by codegen for experimental
    cpython_create_constrained,
    cpython_destroy_constrained,
    cpython_compare_type_specific_constraints,
    cpython_validate_message,
    cpython_get_constraints,
    cpython_destroy_inner,
    cpython_compact_message_in_place,
    nullptr,                      // message_namespace (set by codegen callers)
    nullptr,                      // message_name (set by codegen callers)
  };
  return &prototype;
}

}  // namespace rosidl_typesupport_xcdr_cpython
