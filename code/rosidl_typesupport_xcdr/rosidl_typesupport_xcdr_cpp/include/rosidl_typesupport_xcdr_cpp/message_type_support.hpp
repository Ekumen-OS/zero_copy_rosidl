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
#include "rosidl_runtime_cpp/experimental/constraints.hpp"
#include "rosidl_runtime_cpp/experimental/memory.hpp"
#include "rosidl_typesupport_xcdr_c/message_type_support.h"
#include "rosidl_typesupport_xcdr_cpp/visibility_control.h"

// xcdr_buffers headers needed by the inner struct definition
#include "xcdr_buffers/layout/layout.hpp"

// Forward declare remaining xcdr_buffers types
namespace xcdr_buffers
{
class XCdrWriter;
class XCdrReader;
class XCdrLayoutBuilder;
}  // namespace xcdr_buffers

namespace rosidl_typesupport_xcdr_cpp
{

// ============================================================================
// Language-specific inner state (defined in message_type_support.cpp)
// ============================================================================

/// C++ XCDR message type support inner state.
/**
 * Carries the layout cache and per-type callbacks needed by the
 * C-linkage functions installed on the outer rosidl_message_xcdr_type_support_t.
 * Statically generated per message type by the code generator.
 * Dynamically allocated for constrained handles.
 */
struct rosidl_message_xcdr_cpp_type_support_t
{
  /// Cached layout (nullptr for unconstrained or non-experimental messages).
  std::shared_ptr<xcdr_buffers::XCdrStructLayout> cached_layout{nullptr};

  /// Whether this inner struct was dynamically allocated.
  bool is_dynamically_allocated{false};

  /// Per-type serialization callback.
  rcutils_ret_t (*serialize_fields)(const void *, xcdr_buffers::XCdrWriter &){nullptr};
  /// Per-type deserialization callback.
  rcutils_ret_t (*deserialize_fields)(xcdr_buffers::XCdrReader &, void *){nullptr};
  /// Per-type layout building callback.
  rcutils_ret_t (*build_layout_fields)(
    xcdr_buffers::XCdrLayoutBuilder &, const void *){nullptr};
  /// Construct message at storage using layout from this inner struct.
  rcutils_ret_t (*construct_message)(
    const rosidl_message_xcdr_cpp_type_support_t *,
    rosidl_runtime_cpp::MemoryRegion<void> &, void **){nullptr};
  /// Cast message at storage (parses layout from buffer).
  rcutils_ret_t (*cast_message)(
    rosidl_runtime_cpp::MemoryRegion<void>, void **){nullptr};
  /// Compute serialized size without performing serialization.
  /**
   * For experimental messages with external storage, this callback
   * can return the external storage block size directly.
   * For non-experimental messages, it computes the XCDR-encoded
   * size by summing field sizes with proper alignment.
   * Returns RCUTILS_RET_OK on success, RCUTILS_RET_ERROR on failure.
   */
  rcutils_ret_t (*compute_serialized_size)(
    const void * message, size_t * size){nullptr};

  /// Build constrained layout from constraints.
  std::shared_ptr<xcdr_buffers::XCdrStructLayout>(*build_constrained)(
    const void *){nullptr};
};

// ============================================================================
// Prototype outer table
// ============================================================================

/// Return a prototype outer rosidl_message_xcdr_type_support_t wired to the
/// C-linkage C++ callback functions defined in message_type_support.cpp.
/**
 * The returned table has `inner` set to nullptr.  Callers copy it and
 * override `inner` with their handle-specific inner struct pointer.
 */
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
const rosidl_message_xcdr_type_support_t *
get_xcdr_cpp_type_support_prototype();

// ============================================================================
// Constrained typesupport lifecycle
// ============================================================================

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rosidl_message_type_support_t *
create_constrained_message_type_support(
  const rosidl_message_type_support_t * base_typesupport,
  const void * constraints);

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
void
destroy_constrained_message_type_support(
  rosidl_message_type_support_t * typesupport);

// ============================================================================
// Generic trampolines (C++ convenience API)
//
// These are the middleware-facing entry points.  Each checks the C++
// identifier, casts ts->data to the outer rosidl_message_xcdr_type_support_t,
// and dispatches through its function pointers.
// ============================================================================

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
get_expected_message_size(
  const rosidl_message_type_support_t * typesupport,
  size_t * size);

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
get_message_size(
  const rosidl_message_type_support_t * typesupport,
  const void * message,
  size_t * size);

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
construct_message_at(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> & storage,
  void ** message);

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
cast_message_at(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void ** message);

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
deserialize_message_from(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void * message);

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
serialize_message_into(
  const rosidl_message_type_support_t * typesupport,
  const void * message,
  rosidl_runtime_cpp::MemoryRegion<void> storage);

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
void
destroy_message(
  const rosidl_message_type_support_t * typesupport,
  void * message);

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rosidl_runtime_cpp::MemoryRegion<void>
release_message(
  const rosidl_message_type_support_t * typesupport,
  void * message);

/// Template to get message type support handle for specific message type.
template<typename MessageT>
const rosidl_message_type_support_t *
get_message_type_support_handle();

// ============================================================================
// Constraint comparison
//
// Single canonical implementation in the C package.
// The C++ trampoline is a thin inline wrapper.
// ============================================================================

/// Compare constraints (blanket + type-specific) with incompatible-field reporting.
/**
 * Delegates to the canonical C function rosidl_typesupport_xcdr_c_compare_constraints
 * which handles blanket limits and dispatches type-specific comparison through
 * the outer struct callback.
 *
 * \param typesupport  XCDR typesupport handle (may be NULL).
 * \param candidate    Candidate constraints.
 * \param baseline     Reference constraints.
 * \param report_cb    Callback invoked for each incompatible field (may be NULL).
 * \param user_data    Opaque pointer forwarded to the callback.
 * \return true if candidate is compatible with baseline, false otherwise.
 */
inline bool
compare_constraints(
  const rosidl_message_type_support_t * typesupport,
  const rosidl_message_type_constraints_t * candidate,
  const rosidl_message_type_constraints_t * baseline,
  rosidl_runtime_cpp::ConstraintReportCallback report_cb = nullptr,
  void * user_data = nullptr)
{
  return rosidl_typesupport_xcdr_c_compare_constraints(
    typesupport, candidate, baseline,
    reinterpret_cast<rosidl_typesupport_xcdr_c_constraint_report_callback_t>(report_cb),
    user_data);
}

/// Validate constraints (convenience equivalent to compare_constraints).
inline bool
validate_constraints(
  const rosidl_message_type_support_t * typesupport,
  const rosidl_message_type_constraints_t * candidate,
  const rosidl_message_type_constraints_t * baseline,
  rosidl_runtime_cpp::ConstraintReportCallback report_cb = nullptr,
  void * user_data = nullptr)
{
  return compare_constraints(typesupport, candidate, baseline, report_cb, user_data);
}

}  // namespace rosidl_typesupport_xcdr_cpp

#endif  // ROSIDL_TYPESUPPORT_XCDR_CPP__MESSAGE_TYPE_SUPPORT_HPP_
