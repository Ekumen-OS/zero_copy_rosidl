// Copyright 2026 Ekumen, Inc.
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

#ifndef ROSIDL_RUNTIME_C__EXPERIMENTAL__INITIALIZATION_H_
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__INITIALIZATION_H_

#include "rosidl_runtime_c/experimental/memory.h"

#include "rcutils/allocator.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// @file
/// @brief Initialization options for experimental C11 messages and containers.

/// @brief Message initialization modes for experimental messages.
typedef enum rosidl_runtime_c__experimental__message_initialization_e
{
  /// Skip initialization (user must initialize all fields before use).
  ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_SKIP = 0,
  /// Initialize all fields: defaults if available, otherwise zero/empty.
  ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL = 1,
  /// Initialize all fields to zero/empty (ignore defaults).
  ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ZERO = 2,
  /// Initialize only fields with defaults (leave others untouched).
  ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_DEFAULTS_ONLY = 3,
} rosidl_runtime_c__experimental__message_initialization_t;

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_RUNTIME_C__EXPERIMENTAL__INITIALIZATION_H_
