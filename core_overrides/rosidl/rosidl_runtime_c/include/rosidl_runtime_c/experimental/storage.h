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

#ifndef ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_H_
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_H_

#ifdef __cplusplus
extern "C"
{
#endif

/// @file
/// @brief Shared storage model declarations for experimental C11 containers.

/// @brief Storage kind used by tagged unions in experimental C11 containers.
typedef enum rosidl_runtime_c__experimental__storage_kind_e
{
  ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__LOCAL = 0,
  ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL = 1,
  ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED = 2,
} rosidl_runtime_c__experimental__storage_kind_t;

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_H_
