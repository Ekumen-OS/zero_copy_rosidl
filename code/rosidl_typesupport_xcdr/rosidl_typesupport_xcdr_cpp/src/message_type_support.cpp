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
#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"

namespace rosidl_typesupport_xcdr_cpp
{

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

  // Verify this is an XCDR typesupport
  if (std::strcmp(base_typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return nullptr;
  }

  // Cast to experimental callbacks
  auto base_callbacks = static_cast<const message_type_support_callbacks_experimental_t *>(
    base_typesupport->data);

  // Check if already constrained
  if (base_callbacks->cached_layout != nullptr) {
    RCUTILS_SET_ERROR_MSG("Typesupport is already constrained");
    return nullptr;
  }

  // Check if it can be constrained
  if (!base_callbacks->build_constrained_layout) {
    RCUTILS_SET_ERROR_MSG("Message has no variable-length fields (already fully bounded)");
    return nullptr;
  }

  // Build layout from constraints
  auto layout = base_callbacks->build_constrained_layout(constraints);
  if (!layout) {
    RCUTILS_SET_ERROR_MSG("Failed to build layout from constraints");
    return nullptr;
  }

  // Create new callback structure (shallow copy + new layout)
  auto new_callbacks = new message_type_support_callbacks_experimental_t(*base_callbacks);
  new_callbacks->cached_layout = layout;
  new_callbacks->is_dynamically_allocated = true;

  // Create new handle
  auto new_handle = new rosidl_message_type_support_t(*base_typesupport);
  new_handle->data = new_callbacks;

  return new_handle;
}

void
destroy_constrained_message_type_support(
  rosidl_message_type_support_t * typesupport)
{
  if (!typesupport) {
    return;
  }

  // Verify this is an XCDR typesupport
  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    return;
  }

  // Cast to experimental callbacks
  auto callbacks = static_cast<const message_type_support_callbacks_experimental_t *>(
    typesupport->data);

  // Only delete if dynamically allocated
  if (callbacks->is_dynamically_allocated) {
    delete callbacks;  // shared_ptr in callbacks will auto-free layout
    delete typesupport;
  }
}

rcutils_ret_t
get_expected_message_size(
  const rosidl_message_type_support_t * typesupport,
  size_t * size)
{
  if (!typesupport) {
    RCUTILS_SET_ERROR_MSG("typesupport is nullptr");
    return RCUTILS_RET_ERROR;
  }

  if (!size) {
    RCUTILS_SET_ERROR_MSG("size is nullptr");
    return RCUTILS_RET_ERROR;
  }

  // Verify this is an XCDR typesupport
  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }

  // Try experimental callbacks first
  auto exp_callbacks = static_cast<const message_type_support_callbacks_experimental_t *>(
    typesupport->data);

  if (exp_callbacks->get_expected_message_size) {
    return exp_callbacks->get_expected_message_size(exp_callbacks, size);
  }

  RCUTILS_SET_ERROR_MSG("get_expected_message_size not supported for non-experimental messages");
  return RCUTILS_RET_ERROR;
}

rcutils_ret_t
get_message_size(
  const rosidl_message_type_support_t * typesupport,
  const void * message,
  size_t * size)
{
  if (!typesupport) {
    RCUTILS_SET_ERROR_MSG("typesupport is nullptr");
    return RCUTILS_RET_ERROR;
  }

  if (!message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }

  if (!size) {
    RCUTILS_SET_ERROR_MSG("size is nullptr");
    return RCUTILS_RET_ERROR;
  }

  // Verify this is an XCDR typesupport
  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }

  // Try experimental callbacks first
  auto exp_callbacks = static_cast<const message_type_support_callbacks_experimental_t *>(
    typesupport->data);

  if (exp_callbacks->get_message_size) {
    return exp_callbacks->get_message_size(message, size);
  }

  RCUTILS_SET_ERROR_MSG("get_message_size not supported for non-experimental messages");
  return RCUTILS_RET_ERROR;
}

rcutils_ret_t
construct_message_at(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> & storage,
  void ** message)
{
  if (!typesupport) {
    RCUTILS_SET_ERROR_MSG("typesupport is nullptr");
    return RCUTILS_RET_ERROR;
  }

  if (!message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }

  // Verify this is an XCDR typesupport
  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }

  // Try experimental callbacks first
  auto exp_callbacks = static_cast<const message_type_support_callbacks_experimental_t *>(
    typesupport->data);

  if (exp_callbacks->construct_message_at) {
    return exp_callbacks->construct_message_at(exp_callbacks, storage, message);
  }

  RCUTILS_SET_ERROR_MSG("construct_message_at not supported for non-experimental messages");
  return RCUTILS_RET_ERROR;
}

rcutils_ret_t
cast_message_at(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void ** message)
{
  if (!typesupport) {
    RCUTILS_SET_ERROR_MSG("typesupport is nullptr");
    return RCUTILS_RET_ERROR;
  }

  if (!message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }

  // Verify this is an XCDR typesupport
  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }

  // Try experimental callbacks first
  auto exp_callbacks = static_cast<const message_type_support_callbacks_experimental_t *>(
    typesupport->data);

  if (exp_callbacks->cast_message_at) {
    return exp_callbacks->cast_message_at(storage, message);
  }

  RCUTILS_SET_ERROR_MSG("cast_message_at not supported for non-experimental messages");
  return RCUTILS_RET_ERROR;
}

rcutils_ret_t
deserialize_message_from(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void * message)
{
  if (!typesupport) {
    RCUTILS_SET_ERROR_MSG("typesupport is nullptr");
    return RCUTILS_RET_ERROR;
  }

  if (!message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }

  // Verify this is an XCDR typesupport
  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }

  // All messages now use experimental callbacks struct
  auto callbacks = static_cast<const message_type_support_callbacks_experimental_t *>(
    typesupport->data);

  if (callbacks->deserialize_message_from) {
    return callbacks->deserialize_message_from(storage, message);
  }

  RCUTILS_SET_ERROR_MSG("deserialize_message_from not found");
  return RCUTILS_RET_ERROR;
}

rcutils_ret_t
serialize_message_into(
  const rosidl_message_type_support_t * typesupport,
  const void * message,
  rosidl_runtime_cpp::MemoryRegion<void> storage)
{
  if (!typesupport) {
    RCUTILS_SET_ERROR_MSG("typesupport is nullptr");
    return RCUTILS_RET_ERROR;
  }

  if (!message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }

  // Verify this is an XCDR typesupport
  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }

  // All messages now use experimental callbacks struct
  auto callbacks = static_cast<const message_type_support_callbacks_experimental_t *>(
    typesupport->data);

  if (callbacks->serialize_message_into) {
    return callbacks->serialize_message_into(message, storage);
  }

  RCUTILS_SET_ERROR_MSG("serialize_message_into not found");
  return RCUTILS_RET_ERROR;
}

void
destroy_message(
  const rosidl_message_type_support_t * typesupport,
  void * message)
{
  if (!typesupport || !message) {
    return;
  }

  // Verify this is an XCDR typesupport
  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    return;
  }

  auto callbacks = static_cast<const message_type_support_callbacks_experimental_t *>(
    typesupport->data);

  if (callbacks->destroy_message) {
    callbacks->destroy_message(message);
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

  // Verify this is an XCDR typesupport
  if (std::strcmp(typesupport->typesupport_identifier,
                  rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return rosidl_runtime_cpp::MemoryRegion<void>{nullptr, 0};
  }

  auto callbacks = static_cast<const message_type_support_callbacks_experimental_t *>(
    typesupport->data);

  if (callbacks->release_message) {
    return callbacks->release_message(message);
  }

  // No release_message callback - not an experimental message or no external storage
  return rosidl_runtime_cpp::MemoryRegion<void>{nullptr, 0};
}

}  // namespace rosidl_typesupport_xcdr_cpp
