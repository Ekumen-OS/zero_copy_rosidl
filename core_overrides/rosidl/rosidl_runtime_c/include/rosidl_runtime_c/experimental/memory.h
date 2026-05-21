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

#ifndef ROSIDL_RUNTIME_C__EXPERIMENTAL__MEMORY_H_
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__MEMORY_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Memory descriptor.
 *
 * Describes a memory location and its attributes.
 */
typedef struct rosidl_memory_s
{
  void * address;    ///< Memory address.
  int attributes;    ///< Memory attributes.
} rosidl_memory_t;

/**
 * Check if a memory descriptor is valid.
 *
 * \param[in] memory Memory descriptor to validate.
 * \return true if the memory descriptor is valid, false otherwise.
 */
bool rosidl_memory_is_valid(const rosidl_memory_t * memory);

/**
 * Memory region descriptor.
 *
 * Describes a contiguous region of memory with a location and size.
 */
typedef struct rosidl_memory_region_s
{
  rosidl_memory_t location;    ///< Memory location.
  size_t size;                 ///< Size of the memory region in bytes.
} rosidl_memory_region_t;

/**
 * Check if a memory region descriptor is valid.
 *
 * \param[in] region Memory region descriptor to validate.
 * \return true if the memory region descriptor is valid, false otherwise.
 */
bool rosidl_memory_region_is_valid(const rosidl_memory_region_t * region);

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_RUNTIME_C__EXPERIMENTAL__MEMORY_H_
