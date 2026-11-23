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
#include <type_traits>
#include <variant>

#include "rcutils/error_handling.h"
#include "rosidl_typesupport_xcdr_c/identifier.h"
#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"

#include "xcdr_buffers/common/types.hpp"
#include "xcdr_buffers/serialization/reader.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

namespace rosidl_typesupport_xcdr_cpp
{

// ============================================================================
// C-linkage callback functions for the xcdr rosidl_message_xcdr_type_support_t
//
// Each function matches the function pointer signature in the xcdr struct.
// They receive `const void * inner` which is always a
// rosidl_message_xcdr_cpp_type_support_t * in the C++ package.
// ============================================================================

namespace
{

rcutils_ret_t
region_layout_is_bounded(const xcdr_buffers::XCdrLayout & layout, bool & bounded);

rcutils_ret_t
region_struct_is_bounded(const xcdr_buffers::XCdrStructLayout & layout, bool & bounded)
{
  for (const auto & member : layout.members()) {
    auto ret = region_layout_is_bounded(member.layout(), bounded);
    if (ret != RCUTILS_RET_OK || !bounded) {
      return ret;
    }
  }
  return RCUTILS_RET_OK;
}

rcutils_ret_t
region_layout_is_bounded(const xcdr_buffers::XCdrLayout & layout, bool & bounded)
{
  return std::visit(
    [&bounded](const auto & entry) -> rcutils_ret_t {
      using T = std::decay_t<decltype(entry)>;
      constexpr bool is_array = std::is_same_v<T, xcdr_buffers::XCdrArrayLayout>;
      constexpr bool is_sequence = std::is_same_v<T, xcdr_buffers::XCdrSequenceLayout>;
      if constexpr (std::is_same_v<T, xcdr_buffers::XCdrStringLayout>) {
        bounded = entry.actual_length() != 0;
      } else if constexpr (std::is_same_v<T, xcdr_buffers::XCdrPrimitiveSequenceLayout>) {
        bounded = entry.actual_count() != 0;
      } else if constexpr (is_array || is_sequence) {
        size_t count;
        if constexpr (std::is_same_v<T, xcdr_buffers::XCdrArrayLayout>) {
          count = entry.count();
        } else {
          count = entry.actual_count();
        }
        if constexpr (std::is_same_v<T, xcdr_buffers::XCdrSequenceLayout>) {
          if (count == 0) {
            bounded = false;
            return RCUTILS_RET_OK;
          }
        }
        for (size_t i = 0; i < count; ++i) {
          auto element = entry.element_layout(i);
          if (!element) {
            RCUTILS_SET_ERROR_MSG("invalid region container layout");
            return RCUTILS_RET_ERROR;
          }
          auto ret = region_layout_is_bounded(element->get(), bounded);
          if (ret != RCUTILS_RET_OK || !bounded) {
            return ret;
          }
        }
      } else if constexpr (std::is_same_v<T, xcdr_buffers::XCdrStructLayout>) {
        return region_struct_is_bounded(entry, bounded);
      }
      return RCUTILS_RET_OK;
    }, layout);
}

extern "C" rcutils_ret_t
cpp_get_expected_size(
  const rosidl_message_xcdr_type_support_t * xcdr, size_t * size)
{
  if (nullptr == xcdr || nullptr == size) {
    RCUTILS_SET_ERROR_MSG("xcdr or size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
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
cpp_get_message_size(
  const rosidl_message_xcdr_type_support_t * xcdr,
  const void * message, size_t * size)
{
  if (nullptr == xcdr || nullptr == message || nullptr == size) {
    RCUTILS_SET_ERROR_MSG("xcdr, message, or size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);

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
  const rosidl_message_xcdr_type_support_t * xcdr,
  rosidl_memory_region_t storage,
  void ** message)
{
  if (nullptr == xcdr || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("inner or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
  if (nullptr == impl->construct_message) {
    RCUTILS_SET_ERROR_MSG("construct_message callback not available");
    return RCUTILS_RET_ERROR;
  }
  rosidl_runtime_cpp::MemoryRegion<void> cpp_storage(storage);
  return impl->construct_message(impl, cpp_storage, message);
}

extern "C" rcutils_ret_t
cpp_cast_message_at(
  const rosidl_message_xcdr_type_support_t * xcdr,
  rosidl_memory_region_t storage,
  void ** message)
{
  if (nullptr == xcdr || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("inner or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
  if (nullptr == impl->cast_message) {
    RCUTILS_SET_ERROR_MSG("cast_message callback not available");
    return RCUTILS_RET_ERROR;
  }
  rosidl_runtime_cpp::MemoryRegion<void> cpp_storage(storage);
  return impl->cast_message(impl, cpp_storage, message);
}

extern "C" rcutils_ret_t
cpp_serialize_message_into(
  const rosidl_message_xcdr_type_support_t * xcdr,
  const void * message,
  rosidl_memory_region_t storage)
{
  if (nullptr == xcdr || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("xcdr or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
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
  const rosidl_message_xcdr_type_support_t * xcdr,
  rosidl_memory_region_t storage,
  void * message)
{
  if (nullptr == xcdr || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("xcdr or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
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
  // Default no-op.  The codegen sets the xcdr struct's destroy_message
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

extern "C" rosidl_memory_region_t
cpp_get_backing_storage(const void * message)
{
  // Default no-op (returns empty region).  Overridden by codegen for
  // experimental messages that support external storage.
  (void)message;
  return rosidl_memory_region_t{{nullptr, 0}, 0};
}

extern "C" rosidl_message_type_support_t *
cpp_create_constrained(
  const rosidl_message_xcdr_type_support_t * xcdr,
  const rosidl_typesupport_xcdr_c__constraints_t * constraints)
{
  if (nullptr == xcdr || nullptr == constraints) {
    RCUTILS_SET_ERROR_MSG("xcdr or constraints is nullptr");
    return nullptr;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
  if (nullptr == impl) {
    RCUTILS_SET_ERROR_MSG("inner state is nullptr");
    return nullptr;
  }

  // Build constrained layout from type-specific constraints.
  if (nullptr == impl->build_constrained) {
    RCUTILS_SET_ERROR_MSG("build_constrained callback not available");
    return nullptr;
  }
  auto layout = impl->build_constrained(constraints->type_specific);
  if (!layout) {
    RCUTILS_SET_ERROR_MSG("Failed to build constrained layout");
    return nullptr;
  }

  // Clone constraints into owned storage.
  if (nullptr == impl->clone_constraints) {
    RCUTILS_SET_ERROR_MSG("clone_constraints callback not available");
    return nullptr;
  }
  auto owned = impl->clone_constraints(constraints);
  if (!owned) {
    RCUTILS_SET_ERROR_MSG("Failed to clone constraints");
    return nullptr;
  }

  // Create new inner struct with the constrained layout and owned constraints.
  auto * new_inner = new rosidl_message_xcdr_cpp_type_support_t(*impl);
  new_inner->cached_layout = layout;
  new_inner->owned_constraints = owned;
  new_inner->is_dynamically_allocated = true;

  // Create new xcdr struct (copy base handle's outer table, set inner).
  // Copying from xcdr preserves all per-message function pointers (including
  // destroy_message, release_message, get_backing_storage, etc.) that the
  // prototype doesn't carry.  Only inner is replaced.
  auto * new_xcdr = new rosidl_message_xcdr_type_support_t(*xcdr);
  new_xcdr->inner = new_inner;

  // Create new handle.
  auto * new_handle = new rosidl_message_type_support_t();
  new_handle->typesupport_identifier = rosidl_typesupport_xcdr_cpp__identifier;
  new_handle->data = new_xcdr;
  new_handle->func = nullptr;
  new_handle->get_type_hash_func = xcdr_default_get_type_hash;
  new_handle->get_type_description_func = xcdr_default_get_type_description;
  new_handle->get_type_description_sources_func = xcdr_default_get_type_description_sources;

  return new_handle;
}

extern "C" void
cpp_destroy_constrained(rosidl_message_type_support_t * typesupport)
{
  if (nullptr == typesupport) {
    return;
  }
  auto * xcdr = static_cast<const rosidl_message_xcdr_type_support_t *>(
    typesupport->data);
  if (nullptr == xcdr) {
    return;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
  if (nullptr != impl && impl->is_dynamically_allocated) {
    delete impl;
  }
  delete xcdr;
  delete typesupport;
}

extern "C" bool
cpp_compare_type_specific_constraints(const void * lhs, const void * rhs)
{
  (void)lhs;
  (void)rhs;
  return false;
}

extern "C" rcutils_ret_t
cpp_validate_message(
  const rosidl_message_xcdr_type_support_t * xcdr,
  const rosidl_typesupport_xcdr_c__constraints_t * constraints,
  const void * message,
  rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
  void * user_data)
{
  if (nullptr == xcdr || nullptr == message || nullptr == constraints) {
    RCUTILS_SET_ERROR_MSG("xcdr, constraints, or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
  if (nullptr == impl->validate_fields) {
    // No validation callback — this type does not support per-field validation.
    if (constraints->strict) {
      RCUTILS_SET_ERROR_MSG("message validation requested but validate_fields is null");
      return RCUTILS_RET_ERROR;
    }
    return RCUTILS_RET_OK;
  }
  // Extract type_specific from constraints for the per-field callback.
  return impl->validate_fields(
    constraints->type_specific, message, report_cb, user_data);
}

extern "C" rosidl_memory_region_t
cpp_compact_message_in_place(
  const rosidl_message_xcdr_type_support_t * xcdr,
  void * message)
{
  rosidl_memory_region_t null_region = {{nullptr, 0}, 0};

  if (nullptr == xcdr || nullptr == message) {
    RCUTILS_SET_ERROR_MSG("xcdr or message is nullptr");
    return null_region;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
  if (nullptr == impl->compact_fields) {
    RCUTILS_SET_ERROR_MSG("compact_fields callback not available");
    return null_region;
  }

  return impl->compact_fields(
    message,
    impl->cached_layout.get());
}

extern "C" void
cpp_destroy_inner(const rosidl_message_xcdr_type_support_t * xcdr)
{
  if (nullptr == xcdr || nullptr == xcdr->inner) {
    return;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
  if (impl->is_dynamically_allocated) {
    delete impl;
  }
}

extern "C" const rosidl_typesupport_xcdr_c__constraints_t *
cpp_get_constraints(const rosidl_message_xcdr_type_support_t * xcdr)
{
  if (nullptr == xcdr || nullptr == xcdr->inner) {
    return nullptr;
  }
  auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
  if (impl->owned_constraints) {
    return impl->owned_constraints.get();
  }
  return nullptr;
}

// ---- Default fallbacks for type hash / type description ----
// These are used by the generated dispatch handle when the message type
// does not provide its own functions (e.g., experimental XCDR-only types).

extern "C" const rosidl_type_hash_t *
xcdr_default_get_type_hash(const rosidl_message_type_support_t * /*type_support*/)
{
  static const rosidl_type_hash_t zero_hash = {ROSIDL_TYPE_HASH_VERSION_UNSET, {0}};
  return &zero_hash;
}

extern "C" const rosidl_runtime_c__type_description__TypeDescription *
xcdr_default_get_type_description(const rosidl_message_type_support_t * /*type_support*/)
{
  return nullptr;
}

extern "C" const rosidl_runtime_c__type_description__TypeSource__Sequence *
xcdr_default_get_type_description_sources(const rosidl_message_type_support_t * /*type_support*/)
{
  return nullptr;
}

}  // anonymous namespace

rcutils_ret_t
region_layout_is_bounded(const rosidl_message_type_support_t * type_support, bool * bounded)
{
  if (!type_support || !type_support->typesupport_identifier || !type_support->data || !bounded ||
    std::strcmp(type_support->typesupport_identifier, rosidl_typesupport_xcdr_cpp__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("expected XCDR C++ type support and a bounded output");
    return RCUTILS_RET_ERROR;
  }
  const auto * xcdr = static_cast<const rosidl_message_xcdr_type_support_t *>(type_support->data);
  if (!xcdr->inner || !xcdr->supports_region_loans) {
    RCUTILS_SET_ERROR_MSG("type support is not an XCDR region view");
    return RCUTILS_RET_ERROR;
  }
  const auto * impl = static_cast<const rosidl_message_xcdr_cpp_type_support_t *>(xcdr->inner);
  *bounded = impl->cached_layout != nullptr;
  if (!*bounded) {
    return RCUTILS_RET_OK;
  }
  // Generated constraint layouts use zero only for unbounded fields; bounded
  // fields substitute their IDL bound when no tighter limit is supplied.
  return region_struct_is_bounded(*impl->cached_layout, *bounded);
}

// ============================================================================
// Prototype xcdr table
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
    cpp_get_backing_storage,      // overridden by codegen for experimental
    cpp_create_constrained,
    cpp_destroy_constrained,
    cpp_compare_type_specific_constraints,
    cpp_validate_message,
    cpp_get_constraints,
    cpp_destroy_inner,
    cpp_compact_message_in_place,
    nullptr,                      // message_namespace (set by codegen callers)
    nullptr,                      // message_name (set by codegen callers)
    false,                        // supports_region_loans (set by codegen)
  };
  return &prototype;
}

// ============================================================================
// Constrained typesupport lifecycle
// ============================================================================

std::shared_ptr<rosidl_message_type_support_t>
create_constrained_message_type_support(
  const rosidl_message_type_support_t * base_typesupport,
  const rosidl_typesupport_xcdr_c__constraints_t * constraints)
{
  if (!base_typesupport) {
    RCUTILS_SET_ERROR_MSG("base_typesupport is nullptr");
    return nullptr;
  }
  if (!constraints) {
    RCUTILS_SET_ERROR_MSG("constraints is nullptr");
    return nullptr;
  }
  if (nullptr == base_typesupport->typesupport_identifier ||
    nullptr == std::strstr(base_typesupport->typesupport_identifier, "xcdr"))
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return nullptr;
  }

  auto * xcdr = static_cast<const rosidl_message_xcdr_type_support_t *>(
    base_typesupport->data);
  if (nullptr == xcdr) {
    RCUTILS_SET_ERROR_MSG("xcdr type support is nullptr");
    return nullptr;
  }

  // Delegate to the create_constrained callback on the xcdr struct.
  if (nullptr == xcdr->create_constrained) {
    RCUTILS_SET_ERROR_MSG("create_constrained callback not available");
    return nullptr;
  }
  auto * raw =
    rosidl_typesupport_xcdr_c_create_constrained_message_type_support(
    base_typesupport, constraints);
  if (nullptr == raw) {
    return nullptr;
  }
  return std::shared_ptr<rosidl_message_type_support_t>(
    raw,
    [](rosidl_message_type_support_t * p) {
      destroy_constrained_message_type_support(p);
    });
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

  auto * xcdr = static_cast<const rosidl_message_xcdr_type_support_t *>(
    typesupport->data);
  if (nullptr == xcdr || nullptr == xcdr->destroy_constrained) {
    return;
  }
  xcdr->destroy_constrained(typesupport);
}

rcutils_ret_t
get_expected_message_size(
  const rosidl_message_type_support_t * typesupport,
  size_t * size)
{
  return rosidl_typesupport_xcdr_c_get_expected_size(typesupport, size);
}

rcutils_ret_t
get_message_size(
  const rosidl_message_type_support_t * typesupport,
  const void * message,
  size_t * size)
{
  return rosidl_typesupport_xcdr_c_get_message_size(typesupport, message, size);
}

rcutils_ret_t
construct_message_at(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> & storage,
  void ** message)
{
  return rosidl_typesupport_xcdr_c_construct_message_at(
    typesupport, storage.c_region(), message);
}

rcutils_ret_t
cast_message_at(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void ** message)
{
  return rosidl_typesupport_xcdr_c_cast_message_at(
    typesupport, storage.c_region(), message);
}

rcutils_ret_t
serialize_message_into(
  const rosidl_message_type_support_t * typesupport,
  const void * message,
  rosidl_runtime_cpp::MemoryRegion<void> storage)
{
  return rosidl_typesupport_xcdr_c_serialize_message_into(
    typesupport, message, storage.c_region());
}

rcutils_ret_t
deserialize_message_from(
  const rosidl_message_type_support_t * typesupport,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void * message)
{
  return rosidl_typesupport_xcdr_c_deserialize_message_from(
    typesupport, storage.c_region(), message);
}

rosidl_memory_region_t
compact_message_in_place(
  const rosidl_message_type_support_t * typesupport,
  void * message)
{
  return rosidl_typesupport_xcdr_c_compact_message_in_place(
      typesupport, message);
}

void
destroy_message(
  const rosidl_message_type_support_t * typesupport,
  void * message)
{
  rosidl_typesupport_xcdr_c_destroy_message(typesupport, message);
}

rosidl_runtime_cpp::MemoryRegion<void>
release_message(
  const rosidl_message_type_support_t * typesupport,
  void * message)
{
  return rosidl_runtime_cpp::MemoryRegion<void>(
    rosidl_typesupport_xcdr_c_release_message(typesupport, message));
}

rosidl_runtime_cpp::MemoryRegion<void>
get_backing_storage(
  const rosidl_message_type_support_t * typesupport,
  const void * message)
{
  return rosidl_runtime_cpp::MemoryRegion<void>(
    rosidl_typesupport_xcdr_c_get_backing_storage(typesupport, message));
}

// ============================================================================
// Constraint comparison
// ============================================================================

rcutils_ret_t
validate_message(
  const rosidl_message_type_support_t * typesupport,
  const rosidl_typesupport_xcdr_c__constraints_t * constraints,
  const void * message,
  rosidl_runtime_cpp::ConstraintReportCallback report_cb)
{
  using CCallback = rosidl_typesupport_xcdr_c_constraint_report_callback_t;
  struct Adapter
  {
    static void call(void * ud, const char * path, int code)
    {
      if (ud) {
        auto & cb = *static_cast<rosidl_runtime_cpp::ConstraintReportCallback *>(ud);
        if (cb) {
          cb(std::string_view(path), code);
        }
      }
    }
  };
  return rosidl_typesupport_xcdr_c_validate_message(
    typesupport, constraints, message,
    report_cb ? static_cast<CCallback>(&Adapter::call) : nullptr,
    report_cb ? &report_cb : nullptr);
}

bool
compare_constraints(
  const rosidl_message_type_support_t * typesupport,
  const rosidl_typesupport_xcdr_c__constraints_t * candidate,
  const rosidl_typesupport_xcdr_c__constraints_t * baseline,
  rosidl_runtime_cpp::ConstraintReportCallback report_cb)
{
  using CCallback = rosidl_typesupport_xcdr_c_constraint_report_callback_t;
  struct Adapter
  {
    static void call(void * ud, const char * path, int code)
    {
      if (ud) {
        auto & cb = *static_cast<rosidl_runtime_cpp::ConstraintReportCallback *>(ud);
        if (cb) {
          cb(std::string_view(path), code);
        }
      }
    }
  };
  return rosidl_typesupport_xcdr_c_compare_constraints(
    typesupport, candidate, baseline,
    report_cb ? static_cast<CCallback>(&Adapter::call) : nullptr,
    report_cb ? &report_cb : nullptr);
}

}  // namespace rosidl_typesupport_xcdr_cpp
