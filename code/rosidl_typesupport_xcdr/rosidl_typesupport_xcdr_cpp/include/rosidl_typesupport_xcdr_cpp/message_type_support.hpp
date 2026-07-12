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

  /// Owned copy of the baseline constraints (nullptr for statically generated
  /// or unconstrained instances).  Populated at constrained handle creation
  /// time via the clone_constraints callback.  The custom deleter frees both
  /// the type_specific clone and the constraints struct itself.
  std::shared_ptr<rosidl_message_type_constraints_t> owned_constraints{nullptr};

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

  /// Clone constraints into handle-owned storage.
  /**
   * Returns a shared_ptr containing a deep copy of the full constraints struct
   * (blanket fields + type-specific).  The custom deleter frees the
   * type_specific clone before releasing the constraints struct itself.
   * Only set on generated inner structs for constrained experimental messages.
   */
  std::shared_ptr<rosidl_message_type_constraints_t> (*clone_constraints)(
    const rosidl_message_type_constraints_t * src){nullptr};

  /// Per-type message validation callback.
  /**
   * Walks the message fields and compares each against its constraint
   * bound (from type_specific).  Reports violating fields through the
   * report callback with dot-separated field paths.
   * \param[in]  type_specific  Per-message constraint values.
   * \param[in]  message        Message instance to validate.
   * \param[in]  report_cb      Callback for each violating field (may be NULL).
   * \param[in]  user_data      Opaque pointer forwarded to report_cb.
   * \return RCUTILS_RET_OK if the message satisfies all constraints,
   *         RCUTILS_RET_ERROR if a constraint is violated.
   */
  rcutils_ret_t (*validate_fields)(
    const void * type_specific,
    const void * message,
    rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
    void * user_data){nullptr};

  /// Per-type consume-and-compact callback (layout-driven).
  /**
   * Consumes the message view: uses the cached constrained layout to
   * determine per-field maximum bounds during traversal, re-writes compact
   * (actual-size) XCDR encoding into the message's own backing buffer using
   * an offset writer (data area after CDR header), and destroys the view.
   *
   * On success returns a region whose .location.address is the blob
   * pointer (same value that loan_sample returned) and .size is the
   * compacted payload byte count (excl. header).
   *
   * On failure returns a null region ({ {nullptr, 0}, 0 }).
   *
   * \param[in]  message         Message view to consume and compact.
   * \param[in]  cached_layout   Pre-built constrained layout from handle.
   * \return Region with blob + size on success, null on failure.
   */
  rosidl_memory_region_t (*compact_fields)(
    void * message,
    const xcdr_buffers::XCdrStructLayout * cached_layout){nullptr};

  /// Per-type recursive compact fields helper (internal recursion, layout-driven).
  /**
   * Non-consuming recursive workhorse shared across nested messages.
   * Uses a shared writer and write-mode flag; the flag tunnels through
   * the call stack so a mismatch deep in a nested message flips write
   * mode for the entire remaining traversal.
   * Derives per-field maximum bounds from the provided layout.
   *
   * \param[in]  message      Message to compact (untyped).
   * \param[in]  layout       Struct layout with member metadata.
   * \param[in]  writer       Shared writer.
   * \param[out] emit         Write-mode flag (set to true on first undersized field).
   * \return RCUTILS_RET_OK on success, RCUTILS_RET_ERROR on constraint violation.
   */
  rcutils_ret_t (*compact_fields_recursive)(
    const void * message,
    const xcdr_buffers::XCdrStructLayout & layout,
    xcdr_buffers::XCdrWriter & writer,
    bool & emit){nullptr};

  /// Return the backing storage of a message without releasing it.
  /** Non-consuming counterpart of release.  Returns the same backing memory
   *  region that release_message would return, but does NOT destroy the view.
   *  For messages with external storage, returns the external block region.
   *  For inline-only messages, returns the message pointer itself with size 0.
   *  \param[in]  message  Message to query.
   *  \return Storage region, or null region on failure. */
  rosidl_memory_region_t (*get_backing_storage)(const void * message){nullptr};
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

/// Create a constrained typesupport handle, returning a shared_ptr with
/// automatic destruction.
/**
 * The returned shared_ptr owns the handle and all its associated state
 * (outer + inner callbacks, owned constraints).  When the last shared_ptr
 * goes out of scope, the handle is automatically destroyed via the
 * equivalent of destroy_constrained_message_type_support.
 *
 * The raw destroy function is still available for C interop and internal
 * use, but C++ callers should prefer the shared_ptr API.
 *
 * \param[in] base_typesupport  Base XCDR typesupport handle.
 * \param[in] constraints       Full constraints struct (blanket + type-specific).
 * \return A shared_ptr to the new constrained handle, or nullptr on error.
 */
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
std::shared_ptr<rosidl_message_type_support_t>
create_constrained_message_type_support(
  const rosidl_message_type_support_t * base_typesupport,
  const rosidl_message_type_constraints_t * constraints);

/// Destroy a constrained typesupport handle.
/**
 * Only needed for C interop or explicit lifecycle management.
 * C++ callers should use the shared_ptr returned by
 * create_constrained_message_type_support instead.
 */
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

/// Validate a message instance against the type's constraints.
/**
 * Dispatches through the C trampoline to the per-message generated
 * validation callback.  The trampoline encapsulates strict / non-strict
 * policy based on constraints->strict.
 *
 * Implementation in message_type_support.cpp.
 *
 * \param typesupport   XCDR typesupport handle.
 * \param constraints   Constraints (type_specific + strict policy).
 * \param message       Message instance to validate.
 * \param report_cb     Callback for each violating field (may be NULL).
 * \return RCUTILS_RET_OK if the message satisfies all constraints,
 *         RCUTILS_RET_ERROR if a constraint is violated or on error.
 */
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rcutils_ret_t
validate_message(
  const rosidl_message_type_support_t * typesupport,
  const rosidl_message_type_constraints_t * constraints,
  const void * message,
  rosidl_runtime_cpp::ConstraintReportCallback report_cb = nullptr);

  /// Consume a message view by compacting it in-place (layout-driven).
  /**
   * Dispatches through the C trampoline to the per-message generated
   * compact callback.  Uses the cached layout from the constrained handle
   * for per-field maximum bounds.  Consumes the message: traverses fields,
   * re-writes compact encoding into the backing buffer using an offset
   * writer, and destroys the view.  Returns a region with blob pointer +
   * compacted size on success, or null region on failure.
   *
   * \param typesupport  XCDR typesupport handle (constrained, with cached layout).
   * \param message      Message view to consume and compact.
   * \return Region with blob pointer + size on success, null on failure.
   */
  ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
  rosidl_memory_region_t
  compact_message_in_place(
    const rosidl_message_type_support_t * typesupport,
    void * message);

/// Return the backing storage of a message without releasing it.
/** Non-consuming counterpart of release_message.  Dispatches through the
 *  C trampoline to the per-message generated callback.
 *  \param[in]  type_support  XCDR typesupport handle.
 *  \param[in]  message       Message view to query.
 *  \return MemoryRegion wrapping the backing storage, or null on failure.
 */
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
rosidl_runtime_cpp::MemoryRegion<void>
get_backing_storage(
  const rosidl_message_type_support_t * type_support,
  const void * message);

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
 * Implementation in message_type_support.cpp.
 *
 * \param typesupport  XCDR typesupport handle (may be NULL).
 * \param candidate    Candidate constraints.
 * \param baseline     Reference constraints.
 * \param report_cb    Callback invoked for each incompatible field (may be NULL).
 * \return true if candidate is compatible with baseline, false otherwise.
 */
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
bool
compare_constraints(
  const rosidl_message_type_support_t * typesupport,
  const rosidl_message_type_constraints_t * candidate,
  const rosidl_message_type_constraints_t * baseline,
  rosidl_runtime_cpp::ConstraintReportCallback report_cb = nullptr);

}  // namespace rosidl_typesupport_xcdr_cpp
#endif  // ROSIDL_TYPESUPPORT_XCDR_CPP__MESSAGE_TYPE_SUPPORT_HPP_
