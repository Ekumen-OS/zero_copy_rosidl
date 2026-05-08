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

#include "rosidl_typesupport_xcdr_cpp/service_type_support.hpp"

#include <cstring>

#include "rcutils/error_handling.h"
#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"

namespace rosidl_typesupport_xcdr_cpp
{

const rosidl_service_type_support_t *
get_service_typesupport_handle(
  const rosidl_service_type_support_t * handle,
  const char * identifier)
{
  if (!handle) {
    RCUTILS_SET_ERROR_MSG("handle is nullptr");
    return nullptr;
  }

  if (!identifier) {
    RCUTILS_SET_ERROR_MSG("identifier is nullptr");
    return nullptr;
  }

  if (std::strcmp(identifier, handle->typesupport_identifier) == 0) {
    return handle;
  }

  if (handle->func) {
    return handle->func(handle, identifier);
  }

  RCUTILS_SET_ERROR_MSG("handle's func is nullptr");
  return nullptr;
}

const rosidl_service_type_support_t *
get_service_typesupport_handle_function(
  const rosidl_service_type_support_t * handle,
  const char * identifier)
{
  if (!handle) {
    RCUTILS_SET_ERROR_MSG("handle is nullptr");
    return nullptr;
  }

  if (!identifier) {
    RCUTILS_SET_ERROR_MSG("identifier is nullptr");
    return nullptr;
  }

  if (std::strcmp(identifier, handle->typesupport_identifier) == 0) {
    return handle;
  }

  RCUTILS_SET_ERROR_MSG("identifier does not match");
  return nullptr;
}

}  // namespace rosidl_typesupport_xcdr_cpp
