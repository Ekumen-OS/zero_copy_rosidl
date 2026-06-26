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

#ifndef ROSIDL_TYPESUPPORT_XCDR_CPP__MESSAGE_TYPE_SUPPORT_HPP_
#define ROSIDL_TYPESUPPORT_XCDR_CPP__MESSAGE_TYPE_SUPPORT_HPP_

#include <cstddef>
#include <memory>

#include "rcutils/types/rcutils_ret.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_cpp/experimental/memory.hpp"
#include "rosidl_typesupport_xcdr_cpp/visibility_control.h"

// Forward declare xcdr_buffers types to avoid including xcdr_buffers here
namespace xcdr_buffers
{
class XCdrStructLayout;
class XCdrWriter;
class XCdrReader;
class XCdrLayoutBuilder;
}  // namespace xcdr_buffers

namespace rosidl_typesupport_xcdr_cpp
{

/// Callbacks for experimental message typesupport (zero-copy capable).
struct message_type_support_callbacks_experimental_t
{
  /// Message namespace.
  const char * message_namespace;

  /// Message name.
  const char * message_name;

  /// Cached layout (nullptr if unconstrained).
  std::shared_ptr<xcdr_buffers::XCdrStructLayout> cached_layout;

  /// Build layout from constraints (for unconstrained typesupports).
  /// Returns nullptr on failure.
  std::shared_ptr<xcdr_buffers::XCdrStructLayout>(*build_constrained_layout)(
    const void * constraints);

  /// Indicates if this typesupport was dynamically allocated.
  bool is_dynamically_allocated;

  /// Computes expected message size given layout (in callbacks).
  /// Fails if cached_layout is nullptr.
  rcutils_ret_t (*get_expected_message_size)(
    const message_type_support_callbacks_experimental_t * callbacks,
    size_t * size);

  /// Computes actual message size.
  rcutils_ret_t (*get_message_size)(
    const void * message,
    size_t * size);

  /// Constructs message at storage using layout (zero-copy).
  /// Requires cached_layout to be non-null.
  rcutils_ret_t (*construct_message_at)(
    const message_type_support_callbacks_experimental_t * callbacks,
    rosidl_runtime_cpp::MemoryRegion<void> & storage,
    void ** message);

  /// Casts buffer at storage into message (zero-copy deserialization).
  /// Parses layout from buffer.
  rcutils_ret_t (*cast_message_at)(
    rosidl_runtime_cpp::MemoryRegion<void> storage,
    void ** message);

  /// Deserializes message from storage (traditional).
  /// Writes into existing message instead of allocating.
  rcutils_ret_t (*deserialize_message_from)(
    rosidl_runtime_cpp::MemoryRegion<void> storage,
    void * message);

  /// Serializes message into storage (traditional).
  rcutils_ret_t (*serialize_message_into)(
    const void * message,
    rosidl_runtime_cpp::MemoryRegion<void> storage);

  /// Destroys message created by construct_at or cast_at.
  /// Only needed for experimental messages with zero-copy support.
  /// Can be nullptr for non-experimental messages.
  void (*destroy_message)(void * message);

  // ========== Private callbacks for recursion (internal use only) ==========

  /// Private: Serialize message fields directly into existing XCdrWriter.
  /// Does not write XCDR header - used for inline nested message serialization.
  /// \param message Message pointer
  /// \param writer Reference to XCdrWriter to write fields into
  /// \return RCUTILS_RET_OK on success
  rcutils_ret_t (*serialize_into_writer)(
    const void * message,
    xcdr_buffers::XCdrWriter & writer);

  /// Private: Deserialize message fields directly from existing XCdrReader.
  /// Does not expect XCDR header - used for inline nested message deserialization.
  /// \param reader Reference to XCdrReader to read fields from
  /// \param message Message pointer to write into
  /// \return RCUTILS_RET_OK on success
  rcutils_ret_t (*deserialize_from_reader)(
    xcdr_buffers::XCdrReader & reader,
    void * message);

  /// Private: Build layout fields into existing LayoutBuilder.
  /// Used for recursive constrained layout construction with nested messages.
  /// \param builder Reference to XCdrLayoutBuilder to add fields to
  /// \param constraints Type-specific constraints (can be nullptr for fully bounded)
  /// \return RCUTILS_RET_OK on success
  rcutils_ret_t (*build_layout_fields)(
    xcdr_buffers::XCdrLayoutBuilder & builder,
    const void * constraints);

  // ========== Storage management ==========

  /// Release message and return its external storage.
  /// Only valid for messages constructed via construct_message_at or cast_message_at.
  /// Message pointer becomes invalid after this call.
  /// \param message Message pointer to release
  /// \return Storage region, or {nullptr, 0} if message has no external storage
  rosidl_runtime_cpp::MemoryRegion<void>(*release_message)(
    void * message);
};

/// Create constrained typesupport handle with cached layout.
///
/// \param base_typesupport Base typesupport handle (must be XCDR experimental).
/// \param constraints Message constraints (type-specific).
/// \return New constrained typesupport handle, or nullptr on failure.
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rosidl_message_type_support_t *
create_constrained_message_type_support(
  const rosidl_message_type_support_t * base_typesupport,
  const void * constraints);

/// Destroy constrained typesupport handle.
/// Safe to call with nullptr or singleton handles.
///
/// \param typesupport Typesupport handle to destroy.
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
void
destroy_constrained_message_type_support(
  rosidl_message_type_support_t * typesupport);

/// Generic trampoline for get_expected_message_size.
/// Verifies typesupport identifier and delegates to callback.
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
get_expected_message_size(
  const rosidl_message_type_support_t * typesupport,
  size_t * size);

/// Generic trampoline for get_message_size.
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
get_message_size(
  const rosidl_message_type_support_t * typesupport,
  const void * message,
  size_t * size);

/// Generic trampoline for construct_message_at.
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
construct_message_at(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> & storage,
  void ** message);

/// Generic trampoline for cast_message_at.
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
cast_message_at(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void ** message);

/// Generic trampoline for deserialize_message_from.
/// Deserializes into existing message (void*) instead of allocating.
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
deserialize_message_from(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void * message);

/// Generic trampoline for serialize_message_into.
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
serialize_message_into(
  const rosidl_message_type_support_t * typesupport,
  const void * message,
  rosidl_runtime_cpp::MemoryRegion<void> storage);

/// Destroy message created by construct_at or cast_at.
/// Safe to call with nullptr.
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
void
destroy_message(
  const rosidl_message_type_support_t * typesupport,
  void * message);

/// Release message and return its external storage.
/// Only valid for messages constructed via construct_message_at or cast_message_at.
/// Message pointer becomes invalid after this call.
/// Returns storage region, or {nullptr, 0} if message has no external storage.
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rosidl_runtime_cpp::MemoryRegion<void>
release_message(
  const rosidl_message_type_support_t * typesupport,
  void * message);

/// Template to get message type support handle for specific message type.
/// Specialized for each generated message type.
template<typename MessageT>
const rosidl_message_type_support_t *
get_message_type_support_handle();

}  // namespace rosidl_typesupport_xcdr_cpp

#endif  // ROSIDL_TYPESUPPORT_XCDR_CPP__MESSAGE_TYPE_SUPPORT_HPP_
