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

#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"

#include <cstring>

#include "rcutils/error_handling.h"
#include "rosidl_typesupport_xcdr_c/identifier.h"
#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"

#include "xcdr_buffers/common/types.hpp"
#include "xcdr_buffers/serialization/reader.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

namespace rosidl_typesupport_xcdr_cpp
{

// ============================================================================
// C-linkage callback functions for the outer rosidl_message_xcdr_type_support_t
//
// Each function matches the function pointer signature in the outer struct.
// They receive `const void * inner` which is always a
// rosidl_message_xcdr_cpp_type_support_t * in the C++ package.
// ============================================================================

namespace
{

extern "C" rcutils_ret_t
cpp_get_expected_size(const void * inner, size_t * size)
{
  if (nullptr == inner || nullptr == size) {
    RCUTILS_SET_ERROR_MSG("inner or size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(inner);
  if (nullptr == impl->cached_layout) {
    RCUTILS_SET_ERROR_MSG("cached_layout not available");
    return RCUTILS_RET_ERROR;
  }
  *size = impl->cached_layout->total_size();
  return RCUTILS_RET_OK;
}

extern "C" rcutils_ret_t
cpp_get_message_size(
  const void * inner, const void * message, size_t * size)
{
  if (nullptr == inner || nullptr == message || nullptr == size) {
    RCUTILS_SET_ERROR_MSG("inner, message, or size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(inner);

  // Preferred: use generated size computation callback.
  if (nullptr != impl->compute_serialized_size) {
    return impl->compute_serialized_size(message, size);
  }

  // Fallback: serialize to temporary writer and measure (migration path).
  if (nullptr == impl->serialize_fields) {
    RCUTILS_SET_ERROR_MSG("serialize_fields callback not available");
    return RCUTILS_RET_ERROR;
  }
  xcdr_buffers::XCdrWriter writer;
  auto ret = impl->serialize_fields(message, writer);
  if (ret != RCUTILS_RET_OK) {
    return RCUTILS_RET_ERROR;
  }
  auto buffer = writer.flush();
  *size = buffer.size();
  return RCUTILS_RET_OK;
}

extern "C" rcutils_ret_t
cpp_construct_message_at(
  const void * inner,
  rosidl_memory_region_t storage,
  void ** message)
{
  if (nullptr == inner || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("inner or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(inner);
  if (nullptr == impl->construct_message) {
    RCUTILS_SET_ERROR_MSG("construct_message callback not available");
    return RCUTILS_RET_ERROR;
  }
  rosidl_runtime_cpp::MemoryRegion<void> cpp_storage(storage);
  return impl->construct_message(impl, cpp_storage, message);
}

extern "C" rcutils_ret_t
cpp_cast_message_at(
  const void * inner,
  rosidl_memory_region_t storage,
  void ** message)
{
  if (nullptr == inner || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("inner or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(inner);
  if (nullptr == impl->cast_message) {
    RCUTILS_SET_ERROR_MSG("cast_message callback not available");
    return RCUTILS_RET_ERROR;
  }
  rosidl_runtime_cpp::MemoryRegion<void> cpp_storage(storage);
  return impl->cast_message(cpp_storage, message);
}

extern "C" rcutils_ret_t
cpp_serialize_message_into(
  const void * inner,
  const void * message,
  rosidl_memory_region_t storage)
{
  if (nullptr == inner || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("inner or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(inner);
  if (nullptr == impl->serialize_fields) {
    RCUTILS_SET_ERROR_MSG("serialize_fields callback not available");
    return RCUTILS_RET_ERROR;
  }
  rosidl_runtime_cpp::MemoryRegion<void> cpp_storage(storage);
  xcdr_buffers::XCdrWriter writer(
    tcb::span<uint8_t>(static_cast<uint8_t *>(cpp_storage.data()), cpp_storage.size()));
  auto ret = impl->serialize_fields(message, writer);
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
cpp_deserialize_message_from(
  const void * inner,
  rosidl_memory_region_t storage,
  void * message)
{
  if (nullptr == inner || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("inner or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(inner);
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
  return impl->deserialize_fields(*reader_result, message);
}

extern "C" void
cpp_destroy_message(void * message)
{
  // Default no-op.  The codegen sets the outer struct's destroy_message
  // slot to a per-type function when experimental messages need destruction.
  (void)message;
}

extern "C" rosidl_memory_region_t
cpp_release_message(void * message)
{
  // Default no-op (returns empty region).  Overridden by codegen for
  // experimental messages that support release.
  (void)message;
  return rosidl_memory_region_t{{nullptr, 0}, 0};
}

extern "C" rosidl_message_type_support_t *
cpp_create_constrained(
  const rosidl_message_xcdr_type_support_t * outer,
  const void * type_specific_constraints)
{
  if (nullptr == outer || nullptr == type_specific_constraints) {
    RCUTILS_SET_ERROR_MSG("outer or constraints is nullptr");
    return nullptr;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(outer->inner);
  if (nullptr == impl) {
    RCUTILS_SET_ERROR_MSG("inner state is nullptr");
    return nullptr;
  }
  if (nullptr == impl->build_constrained) {
    RCUTILS_SET_ERROR_MSG("build_constrained callback not available");
    return nullptr;
  }
  auto layout = impl->build_constrained(type_specific_constraints);
  if (!layout) {
    RCUTILS_SET_ERROR_MSG("Failed to build constrained layout");
    return nullptr;
  }

  // Create new inner struct with the constrained layout.
  auto * new_inner = new rosidl_message_xcdr_cpp_type_support_t(*impl);
  new_inner->cached_layout = layout;
  new_inner->is_dynamically_allocated = true;

  // Create new outer struct (copy prototype, set inner).
  auto * new_outer = new rosidl_message_xcdr_type_support_t();
  *new_outer = *get_xcdr_cpp_type_support_prototype();
  new_outer->inner = new_inner;

  // Create new handle.
  auto * new_handle = new rosidl_message_type_support_t();
  new_handle->typesupport_identifier = rosidl_typesupport_xcdr_cpp__identifier;
  new_handle->data = new_outer;
  new_handle->func = nullptr;
  new_handle->get_type_hash_func = nullptr;
  new_handle->get_type_description_func = nullptr;
  new_handle->get_type_description_sources_func = nullptr;

  return new_handle;
}

extern "C" void
cpp_destroy_constrained(rosidl_message_type_support_t * typesupport)
{
  if (nullptr == typesupport) {
    return;
  }
  auto * outer = static_cast<const rosidl_message_xcdr_type_support_t *>(
    typesupport->data);
  if (nullptr == outer) {
    return;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(outer->inner);
  if (nullptr != impl && impl->is_dynamically_allocated) {
    delete impl;
  }
  delete outer;
  delete typesupport;
}

extern "C" bool
cpp_compare_type_specific_constraints(const void * lhs, const void * rhs)
{
  (void)lhs;
  (void)rhs;
  return false;
}

extern "C" void
cpp_destroy_inner(void * inner)
{
  if (nullptr == inner) {
    return;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(inner);
  if (impl->is_dynamically_allocated) {
    delete impl;
  }
}

}  // anonymous namespace

// ============================================================================
// Prototype outer table
// ============================================================================

const rosidl_message_xcdr_type_support_t *
get_xcdr_cpp_type_support_prototype()
{
  static const rosidl_message_xcdr_type_support_t prototype = {
    nullptr,                      // inner (overridden by callers)
    cpp_get_expected_size,
    cpp_get_message_size,
    cpp_construct_message_at,
    cpp_cast_message_at,
    cpp_serialize_message_into,
    cpp_deserialize_message_from,
    cpp_destroy_message,          // overridden by codegen for experimental
    cpp_release_message,          // overridden by codegen for experimental
    cpp_create_constrained,
    cpp_destroy_constrained,
    cpp_compare_type_specific_constraints,
    cpp_destroy_inner,
  };
  return &prototype;
}

// ============================================================================
// Constrained typesupport lifecycle
// ============================================================================

rosidl_message_type_support_t *
create_constrained_message_type_support(
  const rosidl_message_type_support_t * base_typesupport,
  const void * constraints)
{
  if (!base_typesupport) {
    RCUTILS_SET_ERROR_MSG("base_typesupport is nullptr");
    return nullptr;
  }
  if (!constraints) {
    RCUTILS_SET_ERROR_MSG("constraints is nullptr");
    return nullptr;
  }
  if (std::strcmp(base_typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR C++ typesupport");
    return nullptr;
  }

  auto * outer = static_cast<const rosidl_message_xcdr_type_support_t *>(
    base_typesupport->data);
  if (nullptr == outer) {
    RCUTILS_SET_ERROR_MSG("outer type support is nullptr");
    return nullptr;
  }

  // Delegate to the create_constrained callback on the outer struct.
  if (nullptr == outer->create_constrained) {
    RCUTILS_SET_ERROR_MSG("create_constrained callback not available");
    return nullptr;
  }
  return outer->create_constrained(outer, constraints);
}

void
destroy_constrained_message_type_support(
  rosidl_message_type_support_t * typesupport)
{
  if (!typesupport) {
    return;
  }
  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    return;
  }

  auto * outer = static_cast<const rosidl_message_xcdr_type_support_t *>(
    typesupport->data);
  if (nullptr == outer || nullptr == outer->destroy_constrained) {
    return;
  }
  outer->destroy_constrained(typesupport);
}

// ============================================================================
// Generic trampolines
//
// Each trampoline checks the C++ identifier, casts ts->data to
// rosidl_message_xcdr_type_support_t *, and dispatches.
// ============================================================================

namespace
{

/// Validate and dispatch through the outer type support.
/// \return true on success, false on error (error message set).
bool
resolve_and_dispatch(
  const rosidl_message_type_support_t * typesupport,
  const rosidl_message_xcdr_type_support_t ** outer)
{
  if (nullptr == typesupport) {
    return false;
  }
  if (nullptr == outer) {
    return false;
  }
  *outer = nullptr;

  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR C++ typesupport");
    return false;
  }

  *outer = static_cast<const rosidl_message_xcdr_type_support_t *>(
    typesupport->data);
  if (nullptr == *outer) {
    RCUTILS_SET_ERROR_MSG("outer type support data is nullptr");
    return false;
  }
  return true;
}

}  // anonymous namespace

rcutils_ret_t
get_expected_message_size(
  const rosidl_message_type_support_t * typesupport,
  size_t * size)
{
  if (nullptr == size) {
    RCUTILS_SET_ERROR_MSG("size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer = nullptr;
  if (!resolve_and_dispatch(typesupport, &outer)) {
    return RCUTILS_RET_ERROR;
  }
  if (nullptr == outer->get_expected_size) {
    RCUTILS_SET_ERROR_MSG("get_expected_size callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->get_expected_size(outer->inner, size);
}

rcutils_ret_t
get_message_size(
  const rosidl_message_type_support_t * typesupport,
  const void * message,
  size_t * size)
{
  if (nullptr == message || nullptr == size) {
    RCUTILS_SET_ERROR_MSG("message or size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer = nullptr;
  if (!resolve_and_dispatch(typesupport, &outer)) {
    return RCUTILS_RET_ERROR;
  }
  if (nullptr == outer->get_message_size) {
    RCUTILS_SET_ERROR_MSG("get_message_size callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->get_message_size(outer->inner, message, size);
}

rcutils_ret_t
construct_message_at(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> & storage,
  void ** message)
{
  if (nullptr == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer = nullptr;
  if (!resolve_and_dispatch(typesupport, &outer)) {
    return RCUTILS_RET_ERROR;
  }
  if (nullptr == outer->construct_message_at) {
    RCUTILS_SET_ERROR_MSG("construct_message_at callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->construct_message_at(outer->inner, storage.c_region(), message);
}

rcutils_ret_t
cast_message_at(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void ** message)
{
  if (nullptr == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer = nullptr;
  if (!resolve_and_dispatch(typesupport, &outer)) {
    return RCUTILS_RET_ERROR;
  }
  if (nullptr == outer->cast_message_at) {
    RCUTILS_SET_ERROR_MSG("cast_message_at callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->cast_message_at(outer->inner, storage.c_region(), message);
}

rcutils_ret_t
serialize_message_into(
  const rosidl_message_type_support_t * typesupport,
  const void * message,
  rosidl_runtime_cpp::MemoryRegion<void> storage)
{
  if (nullptr == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer = nullptr;
  if (!resolve_and_dispatch(typesupport, &outer)) {
    return RCUTILS_RET_ERROR;
  }
  if (nullptr == outer->serialize_message_into) {
    RCUTILS_SET_ERROR_MSG("serialize_message_into callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->serialize_message_into(outer->inner, message, storage.c_region());
}

rcutils_ret_t
deserialize_message_from(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void * message)
{
  if (nullptr == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer = nullptr;
  if (!resolve_and_dispatch(typesupport, &outer)) {
    return RCUTILS_RET_ERROR;
  }
  if (nullptr == outer->deserialize_message_from) {
    RCUTILS_SET_ERROR_MSG("deserialize_message_from callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->deserialize_message_from(outer->inner, storage.c_region(), message);
}

void
destroy_message(
  const rosidl_message_type_support_t * typesupport,
  void * message)
{
  if (!typesupport || !message) {
    return;
  }
  const rosidl_message_xcdr_type_support_t * outer = nullptr;
  if (!resolve_and_dispatch(typesupport, &outer)) {
    return;
  }
  if (outer->destroy_message) {
    outer->destroy_message(message);
  }
}

rosidl_runtime_cpp::MemoryRegion<void>
release_message(
  const rosidl_message_type_support_t * typesupport,
  void * message)
{
  if (!typesupport || !message) {
    return rosidl_runtime_cpp::MemoryRegion<void>{nullptr, 0};
  }
  const rosidl_message_xcdr_type_support_t * outer = nullptr;
  if (!resolve_and_dispatch(typesupport, &outer)) {
    return rosidl_runtime_cpp::MemoryRegion<void>{nullptr, 0};
  }
  if (outer->release_message) {
    return rosidl_runtime_cpp::MemoryRegion<void>(outer->release_message(message));
  }
  return rosidl_runtime_cpp::MemoryRegion<void>{nullptr, 0};
}

// ============================================================================
// Constraint comparison
// ============================================================================



}  // namespace rosidl_typesupport_xcdr_cpp
