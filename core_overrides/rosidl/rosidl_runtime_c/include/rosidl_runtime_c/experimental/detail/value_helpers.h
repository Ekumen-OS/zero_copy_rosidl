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

#ifndef ROSIDL_RUNTIME_C__EXPERIMENTAL__DETAIL__VALUE_HELPERS_H_
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__DETAIL__VALUE_HELPERS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rcutils/allocator.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// @file
/// @brief Internal helpers shared by experimental C11 value containers.

/// @brief Safely multiply `count` by `element_size` and report overflow.
static inline bool
rosidl_runtime_c__experimental__detail__compute_bytes(
  size_t count,
  size_t element_size,
  size_t * bytes)
{
  if (bytes == NULL) {
    return false;
  }
  if (element_size != 0U && count > (SIZE_MAX / element_size)) {
    return false;
  }
  *bytes = count * element_size;
  return true;
}

/// @brief Geometric growth policy for capacities.
static inline size_t
rosidl_runtime_c__experimental__detail__next_capacity(
  size_t current_capacity,
  size_t required_capacity)
{
  size_t next = current_capacity == 0U ? 1U : current_capacity;
  while (next < required_capacity) {
    if (next > (SIZE_MAX / 2U)) {
      return required_capacity;
    }
    next *= 2U;
  }
  return next;
}

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_RUNTIME_C__EXPERIMENTAL__DETAIL__VALUE_HELPERS_H_
