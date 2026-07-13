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

#include <stdbool.h>
#include <string.h>

#include "rcutils/error_handling.h"

#include "rosidl_typesupport_xcdr_c/identifier.h"
#include "rosidl_typesupport_xcdr_c/message_type_support.h"

/// Check whether a typesupport identifier is any XCDR variant (C or C++).
/** Returns true if the identifier contains the substring "xcdr".
 *  This allows C++ typesupport handles to flow through the C trampolines
 *  (e.g. when rosidl_typesupport_xcdr_cpp wrappers delegate here). */
static inline bool
is_xcdr_identifier(const char * id)
{
  return  NULL != id && NULL != strstr(id, "xcdr");
}

bool
rosidl_typesupport_xcdr_c_is_valid_handle(
  const rosidl_message_type_support_t * type_support)
{
  if (NULL == type_support) {
    return false;
  }
  if (strcmp(type_support->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    return false;
  }
  if (NULL == type_support->data) {
    return false;
  }
  return true;
}

// ---- Size queries ----

rcutils_ret_t
rosidl_typesupport_xcdr_c_get_expected_size(
  const rosidl_message_type_support_t * type_support,
  size_t * size)
{
  if (NULL == type_support) {
    RCUTILS_SET_ERROR_MSG("type_support is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == size) {
    RCUTILS_SET_ERROR_MSG("size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr) {
    RCUTILS_SET_ERROR_MSG("XCDR type support data is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == xcdr->get_expected_size) {
    RCUTILS_SET_ERROR_MSG("get_expected_size callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return xcdr->get_expected_size(xcdr, size);
}

rcutils_ret_t
rosidl_typesupport_xcdr_c_get_message_size(
  const rosidl_message_type_support_t * type_support,
  const void * message,
  size_t * size)
{
  if (NULL == type_support) {
    RCUTILS_SET_ERROR_MSG("type_support is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == size) {
    RCUTILS_SET_ERROR_MSG("size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr) {
    RCUTILS_SET_ERROR_MSG("xcdr callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == xcdr->get_message_size) {
    RCUTILS_SET_ERROR_MSG("get_message_size callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return xcdr->get_message_size(xcdr, message, size);
}

// ---- Zero-copy construction and casting ----

rcutils_ret_t
rosidl_typesupport_xcdr_c_construct_message_at(
  const rosidl_message_type_support_t * type_support,
  rosidl_memory_region_t storage,
  void ** message)
{
  if (NULL == type_support) {
    RCUTILS_SET_ERROR_MSG("type_support is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr) {
    RCUTILS_SET_ERROR_MSG("xcdr callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == xcdr->construct_message_at) {
    RCUTILS_SET_ERROR_MSG("construct_message_at callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return xcdr->construct_message_at(xcdr, storage, message);
}

rcutils_ret_t
rosidl_typesupport_xcdr_c_cast_message_at(
  const rosidl_message_type_support_t * type_support,
  rosidl_memory_region_t storage,
  void ** message)
{
  if (NULL == type_support) {
    RCUTILS_SET_ERROR_MSG("type_support is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr) {
    RCUTILS_SET_ERROR_MSG("xcdr callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == xcdr->cast_message_at) {
    RCUTILS_SET_ERROR_MSG("cast_message_at callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return xcdr->cast_message_at(xcdr, storage, message);
}

// ---- Serialization ----

rcutils_ret_t
rosidl_typesupport_xcdr_c_serialize_message_into(
  const rosidl_message_type_support_t * type_support,
  const void * message,
  rosidl_memory_region_t storage)
{
  if (NULL == type_support) {
    RCUTILS_SET_ERROR_MSG("type_support is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr) {
    RCUTILS_SET_ERROR_MSG("xcdr callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == xcdr->serialize_message_into) {
    RCUTILS_SET_ERROR_MSG("serialize_message_into callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return xcdr->serialize_message_into(xcdr, message, storage);
}

rcutils_ret_t
rosidl_typesupport_xcdr_c_deserialize_message_from(
  const rosidl_message_type_support_t * type_support,
  rosidl_memory_region_t storage,
  void * message)
{
  if (NULL == type_support) {
    RCUTILS_SET_ERROR_MSG("type_support is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr) {
    RCUTILS_SET_ERROR_MSG("xcdr callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == xcdr->deserialize_message_from) {
    RCUTILS_SET_ERROR_MSG("deserialize_message_from callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return xcdr->deserialize_message_from(xcdr, storage, message);
}

// ---- Message lifecycle ----

void
rosidl_typesupport_xcdr_c_destroy_message(
  const rosidl_message_type_support_t * type_support,
  void * message)
{
  if (NULL == type_support || NULL == message) {
    return;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    return;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr || NULL == xcdr->destroy_message) {
    return;
  }
  xcdr->destroy_message(message);
}

rosidl_memory_region_t
rosidl_typesupport_xcdr_c_release_message(
  const rosidl_message_type_support_t * type_support,
  void * message)
{
  rosidl_memory_region_t empty = {{NULL, 0}, 0};
  if (NULL == type_support || NULL == message) {
    return empty;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    return empty;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr || NULL == xcdr->release_message) {
    return empty;
  }
  return xcdr->release_message(message);
}

rosidl_memory_region_t
rosidl_typesupport_xcdr_c_get_backing_storage(
  const rosidl_message_type_support_t * type_support,
  const void * message)
{
  rosidl_memory_region_t empty = {{NULL, 0}, 0};
  if (NULL == type_support || NULL == message) {
    return empty;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    return empty;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr || NULL == xcdr->get_backing_storage) {
    return empty;
  }
  return xcdr->get_backing_storage(message);
}

// ---- Constrained-handle lifecycle ----

rosidl_message_type_support_t *
rosidl_typesupport_xcdr_c_create_constrained_message_type_support(
  const rosidl_message_type_support_t * base_typesupport,
  const rosidl_message_type_constraints_t * constraints)
{
  if (NULL == base_typesupport) {
    RCUTILS_SET_ERROR_MSG("base_typesupport is nullptr");
    return NULL;
  }
  if (NULL == constraints) {
    RCUTILS_SET_ERROR_MSG("constraints is nullptr");
    return NULL;
  }
  if (!is_xcdr_identifier(base_typesupport->typesupport_identifier)) {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return NULL;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)base_typesupport->data;
  if (NULL == xcdr) {
    RCUTILS_SET_ERROR_MSG("xcdr callback table is nullptr");
    return NULL;
  }
  if (NULL == xcdr->create_constrained) {
    RCUTILS_SET_ERROR_MSG("create_constrained callback is nullptr");
    return NULL;
  }
  return xcdr->create_constrained(xcdr, constraints);
}

const rosidl_message_type_constraints_t *
rosidl_typesupport_xcdr_c_get_constraints(
  const rosidl_message_type_support_t * type_support)
{
  if (NULL == type_support) {
    RCUTILS_SET_ERROR_MSG("type_support is nullptr");
    return NULL;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return NULL;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr) {
    RCUTILS_SET_ERROR_MSG("xcdr callback table is nullptr");
    return NULL;
  }
  if (NULL == xcdr->get_constraints) {
    return NULL;
  }
  return xcdr->get_constraints(xcdr);
}

void
rosidl_typesupport_xcdr_c_destroy_constrained_message_type_support(
  rosidl_message_type_support_t * type_support)
{
  if (NULL == type_support) {
    return;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    return;
  }
  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr || NULL == xcdr->destroy_constrained) {
    return;
  }
  xcdr->destroy_constrained(type_support);
}

bool
rosidl_typesupport_xcdr_c_compare_constraints(
  const rosidl_message_type_support_t * type_support,
  const rosidl_message_type_constraints_t * candidate,
  const rosidl_message_type_constraints_t * baseline,
  rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
  void * user_data)
{
  // No baseline means any candidate is acceptable.
  if (NULL == baseline) {
    return true;
  }

  // Candidate must exist when baseline exists.
  if (NULL == candidate) {
    if (report_cb) {
      report_cb(user_data, "(null)", 0);
    }
    return false;
  }

  // Compare blanket string length limit.
  if (candidate->max_string_length > baseline->max_string_length &&
    0 != baseline->max_string_length)
  {
    if (report_cb) {
      report_cb(user_data, "max_string_length", 0);
    }
    return false;
  }

  // Compare blanket total size limit.
  if (candidate->max_total_size > baseline->max_total_size &&
    0 != baseline->max_total_size)
  {
    if (report_cb) {
      report_cb(user_data, "max_total_size", 0);
    }
    return false;
  }

  // Compare type-specific constraints.
  if (NULL != candidate->type_specific && NULL != baseline->type_specific) {
    if (NULL == type_support) {
      // Without a handle, fall back to pointer-identity check.
      if (candidate->type_specific != baseline->type_specific) {
        if (report_cb) {
          report_cb(user_data, "type_specific", 0);
        }
        return false;
      }
    } else {
      // Accept any XCDR-like identifier (contains "xcdr").
      if (NULL == type_support->typesupport_identifier ||
        NULL == strstr(type_support->typesupport_identifier, "xcdr"))
      {
        return false;
      }
      const rosidl_message_xcdr_type_support_t * xcdr =
        (const rosidl_message_xcdr_type_support_t *)type_support->data;
      if (NULL == xcdr || NULL == xcdr->compare_type_specific_constraints) {
        return false;
      }
      // The generated callback invokes Constraints::CheckCompatible which
      // itself calls the report callback for each failing field path.
      return xcdr->compare_type_specific_constraints(
        candidate->type_specific, baseline->type_specific);
    }
  }

  // If baseline has type_specific constraints but candidate doesn't,
  // the candidate is looser.
  if (NULL == candidate->type_specific && NULL != baseline->type_specific) {
    if (report_cb) {
      report_cb(user_data, "type_specific", 0);
    }
    return false;
  }

  return true;
}

// ---- Compaction ----

rosidl_memory_region_t
rosidl_typesupport_xcdr_c_compact_message_in_place(
  const rosidl_message_type_support_t * type_support,
  void * message)
{
  rosidl_memory_region_t null_region = {{NULL, 0}, 0};

  if (NULL == type_support) {
    RCUTILS_SET_ERROR_MSG("type_support is nullptr");
    return null_region;
  }
  if (NULL == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return null_region;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return null_region;
  }

  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr) {
    RCUTILS_SET_ERROR_MSG("xcdr callback table is nullptr");
    return null_region;
  }

  // If there is no compact callback, the type does not support compaction.
  if (NULL == xcdr->compact_message_in_place) {
    RCUTILS_SET_ERROR_MSG("compact_message_in_place callback not available");
    return null_region;
  }
  return xcdr->compact_message_in_place(xcdr, message);
}

// ---- Message instance validation ----

rcutils_ret_t
rosidl_typesupport_xcdr_c_validate_message(
  const rosidl_message_type_support_t * type_support,
  const rosidl_message_type_constraints_t * constraints,
  const void * message,
  rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
  void * user_data)
{
  if (NULL == type_support) {
    RCUTILS_SET_ERROR_MSG("type_support is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == message) {
    RCUTILS_SET_ERROR_MSG("message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (!is_xcdr_identifier(type_support->typesupport_identifier)) {
    RCUTILS_SET_ERROR_MSG("Not an XCDR typesupport");
    return RCUTILS_RET_ERROR;
  }

  if (NULL == constraints) {
    return RCUTILS_RET_OK;
  }

  const rosidl_message_xcdr_type_support_t * xcdr =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == xcdr) {
    RCUTILS_SET_ERROR_MSG("xcdr callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }

  if (!constraints->strict && NULL != xcdr->get_expected_size && NULL != xcdr->get_message_size) {
    size_t expected = 0;
    if (RCUTILS_RET_OK != xcdr->get_expected_size(xcdr, &expected)) {
      return RCUTILS_RET_ERROR;
    }
    size_t actual = 0;
    if (RCUTILS_RET_OK != xcdr->get_message_size(xcdr, message, &actual)) {
      return RCUTILS_RET_ERROR;
    }
    if (actual <= expected) {
      return RCUTILS_RET_OK;
    }
  }

  if (NULL == xcdr->validate_message) {
    RCUTILS_SET_ERROR_MSG("unsupported per field constraint validation");
    return RCUTILS_RET_ERROR;
  }
  return xcdr->validate_message(xcdr, constraints, message, report_cb, user_data);
}
