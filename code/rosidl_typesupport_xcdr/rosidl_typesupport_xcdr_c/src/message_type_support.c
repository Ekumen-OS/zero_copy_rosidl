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
  if (strcmp(type_support->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR C typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == outer) {
    RCUTILS_SET_ERROR_MSG("outer callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == outer->get_expected_size) {
    RCUTILS_SET_ERROR_MSG("get_expected_size callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->get_expected_size(outer->inner, size);
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
  if (strcmp(type_support->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR C typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == outer) {
    RCUTILS_SET_ERROR_MSG("outer callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == outer->get_message_size) {
    RCUTILS_SET_ERROR_MSG("get_message_size callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->get_message_size(outer->inner, message, size);
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
  if (strcmp(type_support->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR C typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == outer) {
    RCUTILS_SET_ERROR_MSG("outer callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == outer->construct_message_at) {
    RCUTILS_SET_ERROR_MSG("construct_message_at callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->construct_message_at(outer->inner, storage, message);
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
  if (strcmp(type_support->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR C typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == outer) {
    RCUTILS_SET_ERROR_MSG("outer callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == outer->cast_message_at) {
    RCUTILS_SET_ERROR_MSG("cast_message_at callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->cast_message_at(outer->inner, storage, message);
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
  if (strcmp(type_support->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR C typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == outer) {
    RCUTILS_SET_ERROR_MSG("outer callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == outer->serialize_message_into) {
    RCUTILS_SET_ERROR_MSG("serialize_message_into callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->serialize_message_into(outer->inner, message, storage);
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
  if (strcmp(type_support->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR C typesupport");
    return RCUTILS_RET_ERROR;
  }
  const rosidl_message_xcdr_type_support_t * outer =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == outer) {
    RCUTILS_SET_ERROR_MSG("outer callback table is nullptr");
    return RCUTILS_RET_ERROR;
  }
  if (NULL == outer->deserialize_message_from) {
    RCUTILS_SET_ERROR_MSG("deserialize_message_from callback is nullptr");
    return RCUTILS_RET_ERROR;
  }
  return outer->deserialize_message_from(outer->inner, storage, message);
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
  if (strcmp(type_support->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    return;
  }
  const rosidl_message_xcdr_type_support_t * outer =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == outer || NULL == outer->destroy_message) {
    return;
  }
  outer->destroy_message(message);
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
  if (strcmp(type_support->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    return empty;
  }
  const rosidl_message_xcdr_type_support_t * outer =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == outer || NULL == outer->release_message) {
    return empty;
  }
  return outer->release_message(message);
}

// ---- Constrained-handle lifecycle ----

rosidl_message_type_support_t *
rosidl_typesupport_xcdr_c_create_constrained_message_type_support(
  const rosidl_message_type_support_t * base_typesupport,
  const void * constraints)
{
  if (NULL == base_typesupport) {
    RCUTILS_SET_ERROR_MSG("base_typesupport is nullptr");
    return NULL;
  }
  if (NULL == constraints) {
    RCUTILS_SET_ERROR_MSG("constraints is nullptr");
    return NULL;
  }
  if (strcmp(base_typesupport->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    RCUTILS_SET_ERROR_MSG("Not an XCDR C typesupport");
    return NULL;
  }
  const rosidl_message_xcdr_type_support_t * outer =
    (const rosidl_message_xcdr_type_support_t *)base_typesupport->data;
  if (NULL == outer) {
    RCUTILS_SET_ERROR_MSG("outer callback table is nullptr");
    return NULL;
  }
  if (NULL == outer->create_constrained) {
    RCUTILS_SET_ERROR_MSG("create_constrained callback is nullptr");
    return NULL;
  }
  return outer->create_constrained(outer, constraints);
}

void
rosidl_typesupport_xcdr_c_destroy_constrained_message_type_support(
  rosidl_message_type_support_t * type_support)
{
  if (NULL == type_support) {
    return;
  }
  if (strcmp(type_support->typesupport_identifier,
      rosidl_typesupport_xcdr_c__identifier) != 0)
  {
    return;
  }
  const rosidl_message_xcdr_type_support_t * outer =
    (const rosidl_message_xcdr_type_support_t *)type_support->data;
  if (NULL == outer || NULL == outer->destroy_constrained) {
    return;
  }
  outer->destroy_constrained(type_support);
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
      const rosidl_message_xcdr_type_support_t * outer =
        (const rosidl_message_xcdr_type_support_t *)type_support->data;
      if (NULL == outer || NULL == outer->compare_type_specific_constraints) {
        return false;
      }
      // The generated callback invokes Constraints::CheckCompatible which
      // itself calls the report callback for each failing field path.
      return outer->compare_type_specific_constraints(
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
