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

#ifndef ROSIDL_TYPESUPPORT_XCDR_CPYTHON__MESSAGE_TYPE_SUPPORT_HPP_
#define ROSIDL_TYPESUPPORT_XCDR_CPYTHON__MESSAGE_TYPE_SUPPORT_HPP_

#include <cstddef>
#include <memory>

#include "rcutils/types/rcutils_ret.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_cpp/experimental/memory.hpp"
#include "rosidl_typesupport_xcdr_c/message_type_support.h"
#include "rosidl_typesupport_xcdr_cpython/visibility_control.h"

// xcdr_buffers headers needed by the inner struct definition
#include "xcdr_buffers/layout/layout.hpp"
#include "xcdr_buffers/layout/layout_parser.hpp"

// Forward declare remaining xcdr_buffers types
namespace xcdr_buffers
{
class XCdrWriter;
class XCdrReader;
class XCdrLayoutBuilder;
class XCdrConstAccessor;
}  // namespace xcdr_buffers

namespace rosidl_typesupport_xcdr_cpython
{

// ============================================================================
// Message ownership / lifetime contract (Python)
//
// The CPython typesupport implements the language-agnostic contract from
// rosidl_typesupport_xcdr_c/message_type_support.h with Python reference
// semantics:
//
// 1. `construct_message` / `cast_message` return a **new reference** to a
//    Python message object (`PyObject *`) whose containers view the
//    caller-provided storage region through non-owning RawBuffers.  The
//    message must not outlive the storage region.
// 2. `destroy_message` DECREFs the Python object once.  The storage region
//    is not touched.
// 3. `release_message` reads the backing region out of the Python object's
//    ExternalStorage descriptors (no copies), DECREFs the Python object
//    once, and returns the region.  Exactly one of destroy_message /
//    release_message must be called per constructed/cast message; calling
//    both double-DECREFs (use-after-free).
// 4. `get_backing_storage` is non-consuming: it reads the region without
//    touching the message's reference count.
// 5. All callbacks that touch a Python object must hold the GIL.  The
//    generated release/get_backing_storage wrappers acquire it; the outer
//    table defaults (cpython_destroy_message etc.) do too.
// ============================================================================

// ============================================================================
// Language-specific inner state
// ============================================================================

/// CPython XCDR message type support inner state.
/**
 * Carries the layout cache and per-type callbacks needed by the
 * C-linkage functions installed on the outer rosidl_message_xcdr_type_support_t.
 *
 * Unlike the C++ typesupport (which casts `void * message` to a C++ message
 * struct), the CPython typesupport treats the message payload as a
 * `PyObject *` — an instance of the generated experimental Python message
 * class.  Every callback that touches the message must hold the GIL.
 *
 * Statically generated per message type by the code generator.
 * Dynamically allocated for constrained handles.
 */
struct rosidl_message_xcdr_cpython_type_support_t
{
  /// Cached layout (nullptr for unconstrained or fully-bounded messages
  /// that build their layout statically).
  std::shared_ptr<xcdr_buffers::XCdrStructLayout> cached_layout{nullptr};

  /// Owned copy of the baseline constraints (nullptr for statically generated
  /// or unconstrained instances).  Populated at constrained handle creation
  /// time via the clone_constraints callback.  The custom deleter frees both
  /// the type_specific clone and the constraints struct itself.
  std::shared_ptr<rosidl_message_type_constraints_t> owned_constraints{nullptr};

  /// Whether this inner struct was dynamically allocated.
  bool is_dynamically_allocated{false};

  /// Per-type serialization callback: serialize Python message fields.
  /**
   * \param[in]  message  Python message instance (PyObject *).
   * \param[out] writer   XCDR writer positioned after the header.
   * \return RCUTILS_RET_OK on success.
   */
  rcutils_ret_t (*serialize_fields)(void * message, xcdr_buffers::XCdrWriter & writer) = nullptr;

  /// Per-type deserialization callback: mutate a Python message in place.
  /**
   * \param[in]  reader   XCDR reader positioned after the header.
   * \param[out] message  Python message instance (PyObject *) to fill.
   * \return RCUTILS_RET_OK on success.
   */
  rcutils_ret_t (*deserialize_fields)(xcdr_buffers::XCdrReader & reader, void * message) = nullptr;

  /// Per-type layout building callback (build layout fields into a builder).
  /**
   * \param[in,out] builder         Layout builder to append fields to.
   * \param[in]     constraints_ptr Opaque pointer to the Python Constraints
   *                                instance, or nullptr for fully-bounded
   *                                messages (bounds come from the type).
   * \return RCUTILS_RET_OK on success.
   */
  rcutils_ret_t (*build_layout_fields)(
    xcdr_buffers::XCdrLayoutBuilder & builder, const void * constraints_ptr) = nullptr;

  /// Construct a Python message at storage (zero-copy sender side).
  /**
   * Applies the cached layout to the storage region, builds the Python
   * message's ExternalStorage descriptors (RawBuffers pointing into the
   * storage), and constructs a Python message instance via
   * `MsgClass(_storage=..., _init=SKIP)`.
   *
   * \param[in]  impl     This inner struct.
   * \param[in]  storage  Pre-allocated memory region.
   * \param[out] message  New reference to the constructed Python message.
   * \return RCUTILS_RET_OK on success.
   */
  rcutils_ret_t (*construct_message)(
    const rosidl_message_xcdr_cpython_type_support_t * impl,
    rosidl_runtime_cpp::MemoryRegion<void> & storage,
    void ** message) = nullptr;

  /// Cast a serialized XCDR buffer into a Python message (zero-copy receiver).
  /**
   * Parses the layout from the buffer, builds ExternalStorage descriptors
   * pointing into the buffer, and constructs a Python message instance whose
   * containers view the buffer memory directly.  The message must not outlive
   * the buffer.
   *
   * \param[in]  impl     This inner struct.
   * \param[in]  storage  Buffer containing a serialized XCDR message.
   * \param[out] message  New reference to the constructed Python message.
   * \return RCUTILS_RET_OK on success.
   */
  rcutils_ret_t (*cast_message)(
    const rosidl_message_xcdr_cpython_type_support_t * impl,
    rosidl_runtime_cpp::MemoryRegion<void> storage,
    void ** message) = nullptr;

  /// Per-type layout-field parser (cast path).
  /**
   * Walks the buffer with the given layout parser, emitting the parse calls
   * for this message's own members.  Nested struct members recurse into the
   * nested type's own parse_fields callback.
   *
   * \param[in,out] parser  Layout parser positioned after this struct's
   *                        context has been opened (begin_parse_struct).
   * \return RCUTILS_RET_OK on success.
   */
  rcutils_ret_t (*parse_fields)(xcdr_buffers::XCdrLayoutParser & parser) = nullptr;

  /// Compute serialized size without performing serialization.
  /**
   * For messages with external storage, returns the block size directly.
   * Otherwise computes the XCDR-encoded size by summing field sizes with
   * proper alignment.
   *
   * \param[in]  message  Python message instance.
   * \param[out] size     Serialized size in bytes.
   * \return RCUTILS_RET_OK on success.
   */
  rcutils_ret_t (*compute_serialized_size)(const void * message, size_t * size) = nullptr;

  /// Build constrained layout from constraints.
  std::shared_ptr<xcdr_buffers::XCdrStructLayout>(*build_constrained)(
    const void *) = nullptr;

  /// Clone constraints into handle-owned storage.
  std::shared_ptr<rosidl_message_type_constraints_t>(*clone_constraints)(
    const rosidl_message_type_constraints_t * src) = nullptr;

  /// Per-type message validation callback.
  /**
   * Walks the message fields and compares each against its constraint bound
   * (from type_specific).  Reports violating fields through the report
   * callback with dot-separated field paths.
   */
  rcutils_ret_t (*validate_fields)(
    const void * type_specific,
    const void * message,
    rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
    void * user_data) = nullptr;

  /// Per-type consume-and-compact callback (layout-driven).
  rosidl_memory_region_t (*compact_fields)(
    void * message,
    const xcdr_buffers::XCdrStructLayout * cached_layout) = nullptr;

  /// Per-type recursive compact fields helper (internal recursion, layout-driven).
  /**
   * Non-consuming recursive workhorse shared across nested messages.
   * Uses a shared writer and write-mode flag; the flag tunnels through
   * the call stack so a mismatch deep in a nested message flips write
   * mode for the entire remaining traversal.
   * Derives per-field maximum bounds from the provided layout.
   *
   * \param[in]  message      Message to compact (untyped; a PyObject *).
   * \param[in]  layout       Struct layout with member metadata.
   * \param[in]  writer       Shared writer.
   * \param[out] emit         Write-mode flag (set to true on first undersized field).
   * \return RCUTILS_RET_OK on success, RCUTILS_RET_ERROR on constraint violation.
   */
  rcutils_ret_t (*compact_fields_recursive)(
    const void * message,
    const xcdr_buffers::XCdrStructLayout & layout,
    xcdr_buffers::XCdrWriter & writer,
    bool & emit) = nullptr;

  /// Populate a Python ExternalStorage instance from an accessor.
  /**
   * Walks the accessor fields and sets the corresponding RawBuffer /
   * ExternalStorage descriptors on the Python ExternalStorage object
   * (`ext_storage.members.<field>`), with RawBuffers pointing into the
   * buffer.  Called by the construct and cast paths.
   *
   * \param[in]  accessor         XCDR const accessor for the buffer.
   * \param[in]  ext_storage_obj  Python ExternalStorage instance (owned by
   *                              caller) whose members get populated.
   * \return RCUTILS_RET_OK on success.
   */
  rcutils_ret_t (*populate_external_storage)(
    const xcdr_buffers::XCdrConstAccessor & accessor, void * ext_storage_obj) = nullptr;

  /// Return the backing storage of a message without releasing it.
  rosidl_memory_region_t (*get_backing_storage)(const void * message) = nullptr;
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
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
const rosidl_message_xcdr_type_support_t *
get_xcdr_cpython_type_support_prototype();

/// Template to get message type support handle for specific message type.
template<typename MessageT>
const rosidl_message_type_support_t *
get_message_type_support_handle();

}  // namespace rosidl_typesupport_xcdr_cpython

// ---- Default fallback functions for generated dispatch handles ----
// These return zero / null values when a message type does not provide
// its own type hash, type description, or type description sources.

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
const rosidl_type_hash_t *
xcdr_cpython_default_get_type_hash(const rosidl_message_type_support_t * type_support);

ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
const rosidl_runtime_c__type_description__TypeDescription *
xcdr_cpython_default_get_type_description(const rosidl_message_type_support_t * type_support);

ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC
const rosidl_runtime_c__type_description__TypeSource__Sequence *
xcdr_cpython_default_get_type_description_sources(
  const rosidl_message_type_support_t * type_support);

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_TYPESUPPORT_XCDR_CPYTHON__MESSAGE_TYPE_SUPPORT_HPP_
