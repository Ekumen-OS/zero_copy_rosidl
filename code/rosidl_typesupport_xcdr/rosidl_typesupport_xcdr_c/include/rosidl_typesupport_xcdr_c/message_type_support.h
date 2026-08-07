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

#ifndef ROSIDL_TYPESUPPORT_XCDR_C__MESSAGE_TYPE_SUPPORT_H_
#define ROSIDL_TYPESUPPORT_XCDR_C__MESSAGE_TYPE_SUPPORT_H_

#include <stdbool.h>
#include <stddef.h>

#include "rcutils/types/rcutils_ret.h"

#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/experimental/memory.h"

#include "rosidl_typesupport_xcdr_c/visibility_control.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// Callback for reporting constraint incompatibilities (C-compatible).
/**
 * \param[in] user_data   Opaque pointer from the caller.
 * \param[in] field_path  Dot-separated path to the field that fails (e.g. "payload.size").
 * \param[in] reason_code Reason code (0 = looser bound, 1 = type-specific mismatch).
 */
typedef void (*rosidl_typesupport_xcdr_c_constraint_report_callback_t)(
  void * user_data,
  const char * field_path,
  int reason_code);

/// XCDR message type support (outer, language-agnostic).
/**
 * Each language-specific XCDR typesupport (C, C++, Python) populates
 * an instance of this struct and stores it in the `data` field of a
 * `rosidl_message_type_support_t`.
 *
 * The `inner` pointer carries language-specific state and implementation
 * callbacks (`rosidl_message_xcdr_cpp_type_support_t` for C++,
 * `rosidl_message_xcdr_c_type_support_t` for C, etc.).  Every function
 * pointer in this table receives `inner` as its first argument so it
 * can dispatch to the language-specific logic.
 *
 * == Lifetime ==
 *
 * - Statically generated instances (one per message type) are never
 *   destroyed individually.
 * - Instances produced by `create_constrained` are caller-owned and
 *   must be freed via `destroy_constrained`.
 * - The `inner` pointer is owned by this outer struct.
 *   `destroy_constrained` calls `destroy_inner` to release it.
 * - `destroy_inner` may be NULL for statically generated instances
 *   that do not own their inner state.
 */
typedef struct rosidl_message_xcdr_type_support_s
{
  /// Language-specific inner state (opaque to this layer).
  void * inner;

  // -------------------------------------------------------------------
  // Message operations
  // -------------------------------------------------------------------

  /// Compute the expected message size from the (possibly constrained) layout.
  /** \param[in]  type_support  XCDR type support for the message type.
   *  \param[out] size  Expected size in bytes.
   *  \return RCUTILS_RET_OK on success, or RCUTILS_RET_ERROR. */
  rcutils_ret_t (*get_expected_size)(
    const struct rosidl_message_xcdr_type_support_s * type_support,
    size_t * size);

  /// Compute the actual serialized size of a given message.
  /** \param[in]  type_support  XCDR type support for the message type.
   *  \param[in]  message Fully constructed message.
   *  \param[out] size    Serialized size in bytes.
   *  \return RCUTILS_RET_OK on success, or RCUTILS_RET_ERROR. */
  rcutils_ret_t (*get_message_size)(
    const struct rosidl_message_xcdr_type_support_s * type_support,
    const void * message,
    size_t * size);

  /// Construct a message in-place at a given storage region.
  /** The storage region must be at least as large as the expected size.
   *  The output `*message` points into the storage region.
   *  \param[in]  type_support  XCDR type support for the message type.
   *  \param[in]  storage Pre-allocated memory region.
   *  \param[out] message Pointer to the constructed message (borrows from storage).
   *  \return RCUTILS_RET_OK on success, or RCUTILS_RET_ERROR. */
  rcutils_ret_t (*construct_message_at)(
    const struct rosidl_message_xcdr_type_support_s * type_support,
    rosidl_memory_region_t storage,
    void ** message);

  /// Cast an XCDR buffer into a message (zero-copy deserialization).
  /** Parses the layout from the buffer.  The resulting message borrows from
   *  the buffer and must not outlive it.
   *  \param[in]  type_support  XCDR type support for the message type.
   *  \param[in]  storage Buffer containing a serialized XCDR message.
   *  \param[out] message Pointer to the deserialized message.
   *  \return RCUTILS_RET_OK on success, or RCUTILS_RET_ERROR. */
  rcutils_ret_t (*cast_message_at)(
    const struct rosidl_message_xcdr_type_support_s * type_support,
    rosidl_memory_region_t storage,
    void ** message);

  /// Serialize a message into a storage region.
  /** The storage region must have at least the capacity returned by
   *  `get_expected_size` or `get_message_size`.
   *  \param[in]  type_support  XCDR type support for the message type.
   *  \param[in]  message Message to serialize.
   *  \param[in]  storage Destination memory region.
   *  \return RCUTILS_RET_OK on success, or RCUTILS_RET_ERROR. */
  rcutils_ret_t (*serialize_message_into)(
    const struct rosidl_message_xcdr_type_support_s * type_support,
    const void * message,
    rosidl_memory_region_t storage);

  /// Deserialize a message from a storage region into an existing message.
  /** The message is allocated by the caller and must match the type.
   *  \param[in]  type_support  XCDR type support for the message type.
   *  \param[in]  storage Source buffer.
   *  \param[out] message Destination message (already allocated).
   *  \return RCUTILS_RET_OK on success, or RCUTILS_RET_ERROR. */
  rcutils_ret_t (*deserialize_message_from)(
    const struct rosidl_message_xcdr_type_support_s * type_support,
    rosidl_memory_region_t storage,
    void * message);

  /// Destroy a message created by `construct_message_at` or `cast_message_at`.
  /** \param[in] message Message to destroy. */
  void (*destroy_message)(
    void * message);

  /// Release a message and return its backing storage.
  /** Only valid for messages constructed via `construct_message_at` or
   *  `cast_message_at`.  The message pointer becomes invalid after this call.
   *  \param[in]  message Message to release.
   *  \return Storage region, or { {NULL, 0}, 0 } if the message has no
   *          external storage to release. */
  rosidl_memory_region_t (*release_message)(
    void * message);

  // -------------------------------------------------------------------
  // Message ownership / lifetime contract
  //
  // Messages produced by `construct_message_at` and `cast_message_at`
  // follow a strict handoff protocol.  Every language-specific typesupport
  // must implement it:
  //
  // 1. The caller receives a fully usable message whose serialized
  //    representation lives in the caller-provided storage region.
  //    For zero-copy typesupports the message *views* that region: it must
  //    not be accessed after the region is freed.
  // 2. Exactly one of `destroy_message` or `release_message` must be called
  //    on the message before the storage region is freed or reused.
  //    Calling both, or calling either twice, is a contract violation
  //    (double-free / use-after-free).
  // 3. `destroy_message` only tears down the message; the storage region is
  //    returned to the caller unchanged and may be reused.
  // 4. `release_message` tears down the message and *returns* the backing
  //    storage region to the caller in a single step.  It is the
  //    complement of `construct_message_at` / `cast_message_at` used by
  //    loan-return paths (e.g. rmw loans).  After it returns, the message
  //    pointer must not be used again.
  // 5. `get_backing_storage` is the non-consuming query counterpart of
  //    `release_message`: it reports the region a message would release,
  //    without invalidating the message.  Safe to call any number of times
  //    between construction and destroy/release.
  // 6. `destroy_message` is the only safe terminal call for messages that
  //    were never loaned (no release is required); `release_message` on a
  //    message with no external storage returns { {NULL, 0}, 0 } and still
  //    consumes the message.
  // -------------------------------------------------------------------

  /// Return the backing storage of a message without releasing it.
  /** Non-consuming counterpart of `release_message`.  Returns the same
   *  backing memory region that `release_message` would return, but does
   *  NOT destroy the message view.  The message remains valid after this call.
   *
   *  For messages with external storage, returns the external block region.
   *  For inline-only messages (no external storage), returns a region whose
   *  address is the message pointer itself and size is 0 (zero-length marker).
   *
   *  \param[in]  message Message to query.
   *  \return Storage region, or { {NULL, 0}, 0 } on failure. */
  rosidl_memory_region_t (*get_backing_storage)(
    const void * message);

  // -------------------------------------------------------------------
  // Constrained-handle lifecycle
  // -------------------------------------------------------------------

  /// Create a constrained typesupport handle from an existing outer callback.
  /** Allocates a new `rosidl_message_type_support_t` and its associated
   *  outer + inner callback instances.  The caller owns the returned handle
   *  and must eventually pass it to `destroy_constrained`.
   *  The caller must ensure the constraints outlive the returned handle
   *  (the handle clones the constraints into owned storage via
   *  `clone_constraints` for C++ inner typesupports).
   *  \param[in]  outer       Outer callback table to base the new handle on.
   *  \param[in]  constraints Full constraints struct (blanket + type-specific).
   *  \return A new typesupport handle, or NULL on failure.
   */
  rosidl_message_type_support_t * (*create_constrained)(
    const struct rosidl_message_xcdr_type_support_s * outer,
    const rosidl_message_type_constraints_t * constraints);

  /// Destroy a constrained typesupport handle previously returned by `create_constrained`.
  /** Releases the outer callback instance, its inner state, and the handle itself.
   *  Safe to call with NULL.
   * \param[in,out] ts The handle to destroy.
   */
  void (*destroy_constrained)(
    rosidl_message_type_support_t * ts);

  /// Compare language-specific constraint payloads.
  /** \param[in] lhs Left-hand constraint payload.
   *  \param[in] rhs Right-hand constraint payload.
   *  \return true if lhs is compatible with rhs (not looser),
   *          false if lhs is looser or on error. */
  bool (*compare_type_specific_constraints)(
    const void * lhs,
    const void * rhs);

  /// Validate a message instance against the type's constraints.
  /** Walks the message fields and compares each against its constraint
   *  bounds (from constraints->type_specific).  Reports violating fields
   *  through the optional report callback with dot-separated field paths.
   *  Encapsulates strict vs non-strict policy: when constraints->strict is
   *  false, a cheap payload-size check is attempted first; full per-field
   *  validation is only run on payload-size failure.  When constraints->strict
   *  is true, full per-field validation always runs.
   *  \param[in]  type_support  XCDR type support for the message type.
   *  \param[in]  constraints  Constraints (type_specific + strict policy).
   *  \param[in]  message      Message instance to validate.
   *  \param[in]  report_cb    Callback for each violating field (may be NULL).
   *  \param[in]  user_data    Opaque pointer forwarded to report_cb.
   *  \return RCUTILS_RET_OK if the message satisfies all constraints,
   *          RCUTILS_RET_ERROR if a constraint is violated. */
  rcutils_ret_t (*validate_message)(
    const struct rosidl_message_xcdr_type_support_s * type_support,
    const rosidl_message_type_constraints_t * constraints,
    const void * message,
    rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
    void * user_data);

  /// Return the owned constraints for this typesupport handle.
  /** For dynamically created constrained handles, returns a pointer to the
   *  cloned constraints whose lifetime is tied to the handle.  For statically
   *  created (unconstrained) handles, returns NULL.
   *  \param[in] type_support  XCDR type support to query.
   *  \return Pointer to the baseline constraints, or NULL if unconstrained.
   */
  const rosidl_message_type_constraints_t * (*get_constraints)(
    const struct rosidl_message_xcdr_type_support_s * type_support);

  /// Destroy the language-specific inner state (only set for dynamically allocated instances).
  /** May be NULL for statically generated instances.
   *  \param[in] type_support  XCDR type support (derives inner state from inner field). */
  void (*destroy_inner)(
    const struct rosidl_message_xcdr_type_support_s * type_support);

  // -------------------------------------------------------------------
  // Compaction
  // -------------------------------------------------------------------

  /// Consume a message view by compacting it in-place (layout-driven).
  /**
   * Consumes the message view: uses the cached constrained layout for
   * per-field maximum bounds, rewrites a compact (actual-size) XCDR
   * encoding into the message's own backing buffer using an offset writer
   * (data area after the CDR header), and destroys the message view.
   *
   * On success returns a region whose .location.address is the blob
   * pointer (same value that loan_sample returned to rmw, for Fast DDS
   * write/discard) and .size is the compacted payload byte count
   * (excluding the CDR representation header).
   *
   * On failure returns a null region ({ {nullptr, 0}, 0 }).
   *
   * \param[in]  type_support  XCDR type support with cached layout.
   * \param[in]  message       Message view to consume and compact.
   * \return Region with blob pointer + size on success, null on failure.
   */
  rosidl_memory_region_t (*compact_message_in_place)(
    const struct rosidl_message_xcdr_type_support_s * type_support,
    void * message);

  /// ROS message namespace (e.g. "sensor_msgs::msg" or "sensor_msgs::msg::experimental").
  /** Populated by the code generator. May be NULL for constrained or synthetic handles. */
  const char * message_namespace;

  /// ROS message name (e.g. "Image").
  /** Populated by the code generator. May be NULL for constrained or synthetic handles. */
  const char * message_name;
} rosidl_message_xcdr_type_support_t;

// ============================================================================
// Generic C trampolines
//
// Each trampoline:
//   1. Validates inputs.
//   2. Checks that the typesupport identifier matches the XCDR C identifier.
//   3. Casts `type_support->data` to `rosidl_message_xcdr_type_support_t *`.
//   4. Calls the corresponding function pointer on the outer struct.
//
// These are the universal interface that any middleware (C, C++, Python) can
// call regardless of which language generated the inner callbacks.
// ============================================================================

/// Check that the typesupport handle is XCDR and has a valid outer callback table.
/**
 * \param[in] type_support  The typesupport handle to validate.
 * \return true if `type_support` matches the XCDR C identifier and
 *         `->data` points to a non-null outer callback table, false otherwise.
 */
ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
bool
rosidl_typesupport_xcdr_c_is_valid_handle(
  const rosidl_message_type_support_t * type_support);

/// \name Size queries
/// \{

ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rcutils_ret_t
rosidl_typesupport_xcdr_c_get_expected_size(
  const rosidl_message_type_support_t * type_support,
  size_t * size);

ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rcutils_ret_t
rosidl_typesupport_xcdr_c_get_message_size(
  const rosidl_message_type_support_t * type_support,
  const void * message,
  size_t * size);

/// \}

/// \name Zero-copy construction and casting
/// \{

ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rcutils_ret_t
rosidl_typesupport_xcdr_c_construct_message_at(
  const rosidl_message_type_support_t * type_support,
  rosidl_memory_region_t storage,
  void ** message);

ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rcutils_ret_t
rosidl_typesupport_xcdr_c_cast_message_at(
  const rosidl_message_type_support_t * type_support,
  rosidl_memory_region_t storage,
  void ** message);

/// \}

/// \name Serialization
/// \{

ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rcutils_ret_t
rosidl_typesupport_xcdr_c_serialize_message_into(
  const rosidl_message_type_support_t * type_support,
  const void * message,
  rosidl_memory_region_t storage);

ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rcutils_ret_t
rosidl_typesupport_xcdr_c_deserialize_message_from(
  const rosidl_message_type_support_t * type_support,
  rosidl_memory_region_t storage,
  void * message);

/// \}

/// \name Message lifecycle
/// \{

ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
void
rosidl_typesupport_xcdr_c_destroy_message(
  const rosidl_message_type_support_t * type_support,
  void * message);

ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rosidl_memory_region_t
rosidl_typesupport_xcdr_c_release_message(
  const rosidl_message_type_support_t * type_support,
  void * message);

ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rosidl_memory_region_t
rosidl_typesupport_xcdr_c_get_backing_storage(
  const rosidl_message_type_support_t * type_support,
  const void * message);

/// \}

/// \name Constrained-handle lifecycle
/// \{

/// Create a constrained typesupport handle that clones and owns the constraints.
/**
 * The returned handle owns a deep copy of `constraints` (blanket fields +
 * type-specific).  Its lifetime is independent of the original constraints
 * pointer.  Retrieve the owned baseline via `rosidl_typesupport_xcdr_c_get_constraints`.
 *
 * \param[in] base_typesupport  Base XCDR typesupport handle.
 * \param[in] constraints       Full constraints struct (blanket + type-specific).
 * \return A new typesupport handle, or NULL on error.
 */
ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rosidl_message_type_support_t *
rosidl_typesupport_xcdr_c_create_constrained_message_type_support(
  const rosidl_message_type_support_t * base_typesupport,
  const rosidl_message_type_constraints_t * constraints);

ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
void
rosidl_typesupport_xcdr_c_destroy_constrained_message_type_support(
  rosidl_message_type_support_t * type_support);

/// Return the owned baseline constraints for a constrained XCDR handle.
/**
 * Returns a pointer to the constraints stored inside the typesupport handle
 * (owned storage, lives for the handle's lifetime).  Returns NULL for
 * unconstrained (statically generated) handles.
 *
 * \param[in] type_support  XCDR typesupport handle.
 * \return Pointer to the baseline constraints, or NULL if not constrained.
 */
ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
const rosidl_message_type_constraints_t *
rosidl_typesupport_xcdr_c_get_constraints(
  const rosidl_message_type_support_t * type_support);

/// \}

/// \name Message instance validation
/// \{

/// Validate a message instance against the type's constraints.
/**
 * Walks the message fields and compares each against its constraint
 * bounds (from constraints->type_specific).  Reports violating fields
 * through the optional callback.
 *
 * Encapsulates strict / non-strict validation policy:
 *   - When constraints->strict is false (default), a cheap payload-size
 *     check is attempted first.  If the payload fits within the expected
 *     bound, OK is returned without per-field walking.
 *   - When constraints->strict is true, full per-field validation always
 *     runs (for diagnostics and tighter enforcement).
 *   - When constraints->strict is false but the payload-size check fails,
 *     full validation is triggered automatically to report the culprit field path.
 *
 * The type_support must have an identifier containing "xcdr".
 *
 * \param[in]  type_support  XCDR typesupport handle.
 * \param[in]  constraints   Constraints (type_specific + strict policy).
 * \param[in]  message       Message instance to validate.
 * \param[in]  report_cb     Callback for each violating field (may be NULL).
 * \param[in]  user_data     Opaque pointer forwarded to the callback.
 * \return RCUTILS_RET_OK if the message satisfies all constraints,
 *         RCUTILS_RET_ERROR if a constraint is violated or on error.
 */
ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rcutils_ret_t
rosidl_typesupport_xcdr_c_validate_message(
  const rosidl_message_type_support_t * type_support,
  const rosidl_message_type_constraints_t * constraints,
  const void * message,
  rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
  void * user_data);

/// \}

/// \name Compaction
/// \{

/// Consume a message view by compacting it in-place (layout-driven).
/**
 * Consumes the message view: uses the cached constrained layout from
 * the handle for per-field maximum bounds, rewrites a compact (actual-size)
 * XCDR encoding into the message's own backing buffer using an offset
 * writer, and destroys the message view.
 *
 * On success returns a region whose .location.address is the blob
 * pointer (same value that loan_sample returned) and .size is the
 * compacted payload byte count (excl. CDR header).
 *
 * On failure returns a null region ({ {nullptr, 0}, 0 }).
 *
 * The type_support must have an identifier containing "xcdr".
 *
 * \param[in]  type_support  XCDR typesupport handle (constrained, with cached layout).
 * \param[in]  message       Message instance to consume and compact.
 * \return Region with blob pointer + size on success, null on failure.
 */
ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
rosidl_memory_region_t
rosidl_typesupport_xcdr_c_compact_message_in_place(
  const rosidl_message_type_support_t * type_support,
  void * message);

/// \}

/// \name Constraint comparison
/// \{

/// Compare constraints (blanket + type-specific) with incompatible-field reporting.
/**
 * This is the single canonical constraint comparison function for XCDR
 * typesupports.  It handles blanket limits (max_string_length,
 * max_total_size) generically, and dispatches type-specific comparison
 * through the outer struct's compare_type_specific_constraints callback
 * when a handle is available and both type_specific pointers are non-null.
 *
 * The type_support handle must have an identifier containing "xcdr" (e.g.
 * rosidl_typesupport_xcdr_c__identifier or rosidl_typesupport_xcdr_cpp__identifier).
 * This check is case-sensitive.
 *
 * Incompatibilities are reported through the optional report callback.
 *
 * \param[in] type_support  XCDR typesupport handle (may be NULL; without it
 *                          type-specific comparison degrades to pointer-identity).
 * \param[in] candidate     Candidate constraints to check.
 * \param[in] baseline      Reference constraints to check against.
 * \param[in] report_cb     Callback invoked for each incompatible field (may be NULL).
 * \param[in] user_data     Opaque pointer forwarded to the callback.
 * \return true if candidate is compatible with baseline, false otherwise.
 */
ROSIDL_TYPESUPPORT_XCDR_C_PUBLIC
bool
rosidl_typesupport_xcdr_c_compare_constraints(
  const rosidl_message_type_support_t * type_support,
  const rosidl_message_type_constraints_t * candidate,
  const rosidl_message_type_constraints_t * baseline,
  rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
  void * user_data);

/// \}

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_TYPESUPPORT_XCDR_C__MESSAGE_TYPE_SUPPORT_H_
