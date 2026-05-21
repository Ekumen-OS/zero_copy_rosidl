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

#ifndef ROSIDL_RUNTIME_C__EXPERIMENTAL__STRING_H_
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__STRING_H_

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <uchar.h>

#include "rcutils/allocator.h"
#include "rosidl_runtime_c/experimental/detail/value_helpers.h"
#include "rosidl_runtime_c/experimental/initialization.h"
#include "rosidl_runtime_c/experimental/memory.h"
#include "rosidl_runtime_c/experimental/storage.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// @file
/// @brief Experimental C11 string wrapper macros.

#define ROSIDL_RUNTIME_C__EXPERIMENTAL__INLINE_STRING_CAPACITY 63U

/// @brief Initialization options for experimental strings.
///
/// Provides control over allocation and external storage for string initialization.
typedef struct rosidl_string_init_options_s
{
  /// Optional allocator (NULL to use default allocator).
  const rcutils_allocator_t * allocator;
  
  /// Optional external storage region (NULL for heap allocation).
  const rosidl_memory_region_t * external_storage;
  
  /// Reserved for future expansion (must be NULL).
  void * reserved[4];
} rosidl_string_init_options_t;

/// @brief Declare a bounded basic string wrapper and function signatures.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_BOUNDED_STRING_DECLARE(STRUCT_NAME, CHAR_TYPE) \
  typedef rosidl_string_init_options_t STRUCT_NAME ## __InitOptions; \
  typedef rosidl_memory_region_t STRUCT_NAME ## __ExternalStorage; \
  typedef struct STRUCT_NAME ## _s \
  { \
    CHAR_TYPE * value; \
    size_t size; \
    struct \
    { \
      rosidl_runtime_c__experimental__storage_kind_t kind; \
      union \
      { \
        rosidl_memory_region_t region; \
        struct { \
          CHAR_TYPE data[ROSIDL_RUNTIME_C__EXPERIMENTAL__INLINE_STRING_CAPACITY + 1U]; \
        } local; \
        CHAR_TYPE * data; \
      } storage; \
      size_t capacity; \
      size_t upper_bound; \
      rcutils_allocator_t allocator; \
    } _impl; \
  } STRUCT_NAME; \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _string, \
    size_t upper_bound); \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _string, \
    size_t upper_bound, \
    const STRUCT_NAME ## __InitOptions * options); \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _string); \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _string, \
    size_t requested_capacity); \
  bool STRUCT_NAME ## __assignn( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * value, \
    size_t size); \
  bool STRUCT_NAME ## __assign( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * value); \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _string, \
    size_t size); \
  bool STRUCT_NAME ## __appendn( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * suffix, \
    size_t suffix_size); \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs); \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output);

/// @brief Declare a basic string wrapper and function signatures.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_STRING_DECLARE(STRUCT_NAME, CHAR_TYPE) \
  typedef rosidl_string_init_options_t STRUCT_NAME ## __InitOptions; \
  typedef rosidl_memory_region_t STRUCT_NAME ## __ExternalStorage; \
  typedef struct STRUCT_NAME ## _s \
  { \
    CHAR_TYPE * value; \
    size_t size; \
    struct \
    { \
      rosidl_runtime_c__experimental__storage_kind_t kind; \
      union \
      { \
        rosidl_memory_region_t region; \
        struct { \
          CHAR_TYPE data[ROSIDL_RUNTIME_C__EXPERIMENTAL__INLINE_STRING_CAPACITY + 1U]; \
        } local; \
        CHAR_TYPE * data; \
      } storage; \
      size_t capacity; \
      rcutils_allocator_t allocator; \
    } _impl; \
  } STRUCT_NAME; \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _string); \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _string, \
    const STRUCT_NAME ## __InitOptions * options); \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _string); \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _string, \
    size_t requested_capacity); \
  bool STRUCT_NAME ## __assignn( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * value, \
    size_t size); \
  bool STRUCT_NAME ## __assign( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * value); \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _string, \
    size_t size); \
  bool STRUCT_NAME ## __appendn( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * suffix, \
    size_t suffix_size); \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs); \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output);

/// @brief Create a type alias for an unbounded string type with function forwarding.
/// Generates a typedef and static inline forwarding functions for all string operations.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__STRING_ALIAS(ALIAS_NAME, BASE_TYPE, CHAR_TYPE) \
  typedef BASE_TYPE ALIAS_NAME; \
  typedef BASE_TYPE ## __InitOptions ALIAS_NAME ## __InitOptions; \
  typedef BASE_TYPE ## __ExternalStorage ALIAS_NAME ## __ExternalStorage; \
  static inline bool ALIAS_NAME ## __init(ALIAS_NAME * _string) \
  { \
    return BASE_TYPE ## __init(_string); \
  } \
  static inline bool ALIAS_NAME ## __init_with_options( \
    ALIAS_NAME * _string, \
    const ALIAS_NAME ## __InitOptions * options) \
  { \
    return BASE_TYPE ## __init_with_options(_string, options); \
  } \
  static inline void ALIAS_NAME ## __fini(ALIAS_NAME * _string) \
  { \
    BASE_TYPE ## __fini(_string); \
  } \
  static inline bool ALIAS_NAME ## __reserve( \
    ALIAS_NAME * _string, \
    size_t requested_capacity) \
  { \
    return BASE_TYPE ## __reserve(_string, requested_capacity); \
  } \
  static inline bool ALIAS_NAME ## __assign( \
    ALIAS_NAME * _string, \
    const CHAR_TYPE * value) \
  { \
    return BASE_TYPE ## __assign(_string, value); \
  } \
  static inline bool ALIAS_NAME ## __resize( \
    ALIAS_NAME * _string, \
    size_t size) \
  { \
    return BASE_TYPE ## __resize(_string, size); \
  } \
  static inline bool ALIAS_NAME ## __are_equal( \
    const ALIAS_NAME * lhs, \
    const ALIAS_NAME * rhs) \
  { \
    return BASE_TYPE ## __are_equal(lhs, rhs); \
  } \
  static inline bool ALIAS_NAME ## __copy( \
    const ALIAS_NAME * input, \
    ALIAS_NAME * output) \
  { \
    return BASE_TYPE ## __copy(input, output); \
  }

/// @brief Create a type alias for a bounded string type with function forwarding.
/// Generates a typedef and static inline forwarding functions including upper_bound parameter.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_STRING_ALIAS(ALIAS_NAME, BASE_TYPE, CHAR_TYPE) \
  typedef BASE_TYPE ALIAS_NAME; \
  typedef BASE_TYPE ## __InitOptions ALIAS_NAME ## __InitOptions; \
  typedef BASE_TYPE ## __ExternalStorage ALIAS_NAME ## __ExternalStorage; \
  static inline bool ALIAS_NAME ## __init(ALIAS_NAME * _string, size_t upper_bound) \
  { \
    return BASE_TYPE ## __init(_string, upper_bound); \
  } \
  static inline bool ALIAS_NAME ## __init_with_options( \
    ALIAS_NAME * _string, \
    size_t upper_bound, \
    const ALIAS_NAME ## __InitOptions * options) \
  { \
    return BASE_TYPE ## __init_with_options(_string, upper_bound, options); \
  } \
  static inline void ALIAS_NAME ## __fini(ALIAS_NAME * _string) \
  { \
    BASE_TYPE ## __fini(_string); \
  } \
  static inline bool ALIAS_NAME ## __reserve( \
    ALIAS_NAME * _string, \
    size_t requested_capacity) \
  { \
    return BASE_TYPE ## __reserve(_string, requested_capacity); \
  } \
  static inline bool ALIAS_NAME ## __assign( \
    ALIAS_NAME * _string, \
    const CHAR_TYPE * value) \
  { \
    return BASE_TYPE ## __assign(_string, value); \
  } \
  static inline bool ALIAS_NAME ## __resize( \
    ALIAS_NAME * _string, \
    size_t size) \
  { \
    return BASE_TYPE ## __resize(_string, size); \
  } \
  static inline bool ALIAS_NAME ## __are_equal( \
    const ALIAS_NAME * lhs, \
    const ALIAS_NAME * rhs) \
  { \
    return BASE_TYPE ## __are_equal(lhs, rhs); \
  } \
  static inline bool ALIAS_NAME ## __copy( \
    const ALIAS_NAME * input, \
    ALIAS_NAME * output) \
  { \
    return BASE_TYPE ## __copy(input, output); \
  }

/// @brief Define basic string functions.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_BOUNDED_STRING_DEFINE(STRUCT_NAME, CHAR_TYPE) \
  static bool STRUCT_NAME ## __init_with_allocator( \
    STRUCT_NAME * _string, \
    size_t upper_bound, \
    const rcutils_allocator_t * allocator) \
  { \
    if (_string == NULL) { \
      return false; \
    } \
    _string->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__LOCAL; \
    _string->_impl.upper_bound = upper_bound; \
    _string->_impl.capacity = ROSIDL_RUNTIME_C__EXPERIMENTAL__INLINE_STRING_CAPACITY; \
    if (_string->_impl.capacity > _string->_impl.upper_bound) { \
      _string->_impl.capacity = _string->_impl.upper_bound; \
    } \
    if (allocator != NULL) { \
      if (!rcutils_allocator_is_valid(allocator)) { \
        return false; \
      } \
      _string->_impl.allocator = *allocator; \
    } else { \
      _string->_impl.allocator = rcutils_get_default_allocator(); \
    } \
    _string->value = _string->_impl.storage.local.data; \
    _string->size = 0U; \
    _string->value[0] = (CHAR_TYPE)0; \
    return true; \
  } \
  static bool STRUCT_NAME ## __init_with_region( \
    STRUCT_NAME * _string, \
    size_t upper_bound, \
    rosidl_memory_region_t region) \
  { \
    if (_string == NULL || !rosidl_memory_region_is_valid(&region)) { \
      return false; \
    } \
    if (region.size < sizeof(CHAR_TYPE)) { \
      return false; \
    } \
    _string->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL; \
    _string->_impl.storage.region = region; \
    _string->_impl.upper_bound = upper_bound; \
    _string->_impl.capacity = (region.size / sizeof(CHAR_TYPE)) - 1U; \
    if (_string->_impl.capacity > _string->_impl.upper_bound) { \
      _string->_impl.capacity = _string->_impl.upper_bound; \
    } \
    _string->_impl.allocator = rcutils_get_zero_initialized_allocator(); \
    _string->value = (CHAR_TYPE *)region.location.address; \
    _string->size = 0U; \
    _string->value[0] = (CHAR_TYPE)0; \
    return true; \
  } \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _string, \
    size_t upper_bound, \
    const STRUCT_NAME ## __InitOptions * options) \
  { \
    if (options != NULL && options->external_storage != NULL) { \
      return STRUCT_NAME ## __init_with_region(_string, upper_bound, *options->external_storage); \
    } \
    const rcutils_allocator_t * allocator = (options != NULL) ? options->allocator : NULL; \
    return STRUCT_NAME ## __init_with_allocator(_string, upper_bound, allocator); \
  } \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _string, \
    size_t upper_bound) \
  { \
    return STRUCT_NAME ## __init_with_options(_string, upper_bound, NULL); \
  } \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _string) \
  { \
    if (_string == NULL) { \
      return; \
    } \
    if (_string->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED && \
      _string->_impl.storage.data != NULL) \
    { \
      _string->_impl.allocator.deallocate( \
        _string->_impl.storage.data, _string->_impl.allocator.state); \
    } \
    _string->value = NULL; \
  } \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _string, \
    size_t requested_capacity) \
  { \
    if (_string == NULL) { \
      return false; \
    } \
    if (requested_capacity > _string->_impl.upper_bound) { \
      return false; \
    } \
    if (requested_capacity <= _string->_impl.capacity) { \
      return true; \
    } \
    if (_string->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL) { \
      return false; \
    } \
    size_t new_capacity = rosidl_runtime_c__experimental__detail__next_capacity( \
      _string->_impl.capacity, requested_capacity); \
    size_t byte_count = 0U; \
    if (!rosidl_runtime_c__experimental__detail__compute_bytes( \
        new_capacity + 1U, sizeof(CHAR_TYPE), &byte_count)) \
    { \
      return false; \
    } \
    void * old_buffer = \
      _string->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED ? \
      _string->_impl.storage.data : NULL; \
    void * new_buffer = _string->_impl.allocator.reallocate( \
        old_buffer, byte_count, _string->_impl.allocator.state); \
    if (new_buffer == NULL) { \
      return false; \
    } \
    _string->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED; \
    _string->_impl.storage.data = new_buffer; \
    _string->_impl.capacity = new_capacity; \
    _string->value = (CHAR_TYPE *)new_buffer; \
    return true; \
  } \
  bool STRUCT_NAME ## __assignn( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * value, \
    size_t size) \
  { \
    if (_string == NULL || (size > 0U && value == NULL)) { \
      return false; \
    } \
    if (!STRUCT_NAME ## __reserve(_string, size)) { \
      return false; \
    } \
    if (size > 0U) { \
      (void)memcpy(_string->value, value, size * sizeof(CHAR_TYPE)); \
    } \
    _string->size = size; \
    _string->value[_string->size] = (CHAR_TYPE)0; \
    return true; \
  } \
  bool STRUCT_NAME ## __assign( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * value) \
  { \
    if (_string == NULL || value == NULL) { \
      return false; \
    } \
    size_t size = 0U; \
    while (value[size] != (CHAR_TYPE)0) { \
      ++size; \
    } \
    return STRUCT_NAME ## __assignn(_string, value, size); \
  } \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _string, \
    size_t size) \
  { \
    if (_string == NULL) { \
      return false; \
    } \
    size_t previous_size = _string->size; \
    if (!STRUCT_NAME ## __reserve(_string, size)) { \
      return false; \
    } \
    if (size > previous_size) { \
      (void)memset(&_string->value[previous_size], 0, (size - previous_size) * sizeof(CHAR_TYPE)); \
    } \
    _string->size = size; \
    _string->value[_string->size] = (CHAR_TYPE)0; \
    return true; \
  } \
  bool STRUCT_NAME ## __appendn( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * suffix, \
    size_t suffix_size) \
  { \
    if (_string == NULL || (suffix_size > 0U && suffix == NULL)) { \
      return false; \
    } \
    size_t new_size = _string->size + suffix_size; \
    if (new_size < _string->size) { \
      return false; \
    } \
    if (!STRUCT_NAME ## __reserve(_string, new_size)) { \
      return false; \
    } \
    if (suffix_size > 0U) { \
      (void)memcpy(&_string->value[_string->size], suffix, suffix_size * sizeof(CHAR_TYPE)); \
    } \
    _string->size = new_size; \
    _string->value[_string->size] = (CHAR_TYPE)0; \
    return true; \
  } \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs) \
  { \
    if (lhs == NULL || rhs == NULL) { \
      return false; \
    } \
    if (lhs->size != rhs->size) { \
      return false; \
    } \
    if (lhs->size == 0U) { \
      return true; \
    } \
    if (lhs->value == NULL || rhs->value == NULL) { \
      return false; \
    } \
    return memcmp(lhs->value, rhs->value, lhs->size * sizeof(CHAR_TYPE)) == 0; \
  } \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output) \
  { \
    if (input == NULL || output == NULL) { \
      return false; \
    } \
    if (input->size > 0U && input->value == NULL) { \
      return false; \
    } \
    return STRUCT_NAME ## __assignn(output, input->value, input->size); \
  }

/// @brief Define basic string functions.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_STRING_DEFINE(STRUCT_NAME, CHAR_TYPE) \
  static bool STRUCT_NAME ## __init_with_allocator( \
    STRUCT_NAME * _string, \
    const rcutils_allocator_t * allocator) \
  { \
    if (_string == NULL) { \
      return false; \
    } \
    if (allocator != NULL) { \
      if (!rcutils_allocator_is_valid(allocator)) { \
        return false; \
      } \
      _string->_impl.allocator = *allocator; \
    } else { \
      _string->_impl.allocator = rcutils_get_default_allocator(); \
    } \
    _string->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__LOCAL; \
    _string->_impl.capacity = ROSIDL_RUNTIME_C__EXPERIMENTAL__INLINE_STRING_CAPACITY; \
    _string->value = _string->_impl.storage.local.data; \
    _string->size = 0U; \
    _string->value[0] = (CHAR_TYPE)0; \
    return true; \
  } \
  static bool STRUCT_NAME ## __init_with_region( \
    STRUCT_NAME * _string, \
    rosidl_memory_region_t region) \
  { \
    if (_string == NULL || !rosidl_memory_region_is_valid(&region)) { \
      return false; \
    } \
    if (region.size < sizeof(CHAR_TYPE)) { \
      return false; \
    } \
    _string->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL; \
    _string->_impl.storage.region = region; \
    _string->_impl.capacity = (region.size / sizeof(CHAR_TYPE)) - 1U; \
    _string->_impl.allocator = rcutils_get_zero_initialized_allocator(); \
    _string->value = (CHAR_TYPE *)region.location.address; \
    _string->size = 0U; \
    _string->value[0] = (CHAR_TYPE)0; \
    return true; \
  } \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _string, \
    const STRUCT_NAME ## __InitOptions * options) \
  { \
    if (options != NULL && options->external_storage != NULL) { \
      return STRUCT_NAME ## __init_with_region(_string, *options->external_storage); \
    } \
    const rcutils_allocator_t * allocator = (options != NULL) ? options->allocator : NULL; \
    return STRUCT_NAME ## __init_with_allocator(_string, allocator); \
  } \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _string) \
  { \
    return STRUCT_NAME ## __init_with_options(_string, NULL); \
  } \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _string) \
  { \
    if (_string == NULL) { \
      return; \
    } \
    if (_string->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED && \
      _string->_impl.storage.region.location.address != NULL) \
    { \
      _string->_impl.allocator.deallocate( \
        _string->_impl.storage.region.location.address, _string->_impl.allocator.state); \
    } \
    _string->value = NULL; \
  } \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _string, \
    size_t requested_capacity) \
  { \
    if (_string == NULL) { \
      return false; \
    } \
    if (requested_capacity <= _string->_impl.capacity) { \
      return true; \
    } \
    if (_string->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL) { \
      return false; \
    } \
    size_t new_capacity = rosidl_runtime_c__experimental__detail__next_capacity( \
      _string->_impl.capacity, requested_capacity); \
    size_t byte_count = 0U; \
    if (!rosidl_runtime_c__experimental__detail__compute_bytes( \
        new_capacity + 1U, sizeof(CHAR_TYPE), &byte_count)) \
    { \
      return false; \
    } \
    void * old_buffer = \
      _string->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED ? \
      _string->_impl.storage.data : NULL; \
    void * new_buffer = _string->_impl.allocator.reallocate( \
        old_buffer, byte_count, _string->_impl.allocator.state); \
    if (new_buffer == NULL) { \
      return false; \
    } \
    _string->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED; \
    _string->_impl.storage.data = new_buffer; \
    _string->_impl.capacity = new_capacity; \
    _string->value = (CHAR_TYPE *)new_buffer; \
    return true; \
  } \
  bool STRUCT_NAME ## __assignn( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * value, \
    size_t size) \
  { \
    if (_string == NULL || (size > 0U && value == NULL)) { \
      return false; \
    } \
    if (!STRUCT_NAME ## __reserve(_string, size)) { \
      return false; \
    } \
    if (size > 0U) { \
      (void)memcpy(_string->value, value, size * sizeof(CHAR_TYPE)); \
    } \
    _string->size = size; \
    _string->value[_string->size] = (CHAR_TYPE)0; \
    return true; \
  } \
  bool STRUCT_NAME ## __assign( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * value) \
  { \
    if (_string == NULL || value == NULL) { \
      return false; \
    } \
    size_t size = 0U; \
    while (value[size] != (CHAR_TYPE)0) { \
      ++size; \
    } \
    return STRUCT_NAME ## __assignn(_string, value, size); \
  } \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _string, \
    size_t size) \
  { \
    if (_string == NULL) { \
      return false; \
    } \
    size_t previous_size = _string->size; \
    if (!STRUCT_NAME ## __reserve(_string, size)) { \
      return false; \
    } \
    if (size > previous_size) { \
      (void)memset(&_string->value[previous_size], 0, (size - previous_size) * sizeof(CHAR_TYPE)); \
    } \
    _string->size = size; \
    _string->value[_string->size] = (CHAR_TYPE)0; \
    return true; \
  } \
  bool STRUCT_NAME ## __appendn( \
    STRUCT_NAME * _string, \
    const CHAR_TYPE * suffix, \
    size_t suffix_size) \
  { \
    if (_string == NULL || (suffix_size > 0U && suffix == NULL)) { \
      return false; \
    } \
    size_t new_size = _string->size + suffix_size; \
    if (new_size < _string->size) { \
      return false; \
    } \
    if (!STRUCT_NAME ## __reserve(_string, new_size)) { \
      return false; \
    } \
    if (suffix_size > 0U) { \
      (void)memcpy(&_string->value[_string->size], suffix, suffix_size * sizeof(CHAR_TYPE)); \
    } \
    _string->size = new_size; \
    _string->value[_string->size] = (CHAR_TYPE)0; \
    return true; \
  } \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs) \
  { \
    if (lhs == NULL || rhs == NULL) { \
      return false; \
    } \
    if (lhs->size != rhs->size) { \
      return false; \
    } \
    if (lhs->size == 0U) { \
      return true; \
    } \
    if (lhs->value == NULL || rhs->value == NULL) { \
      return false; \
    } \
    return memcmp(lhs->value, rhs->value, lhs->size * sizeof(CHAR_TYPE)) == 0; \
  } \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output) \
  { \
    if (input == NULL || output == NULL) { \
      return false; \
    } \
    if (input->size > 0U && input->value == NULL) { \
      return false; \
    } \
    return STRUCT_NAME ## __assignn(output, input->value, input->size); \
  }

/// @brief Convenience macro declaring and defining a bounded basic string in one place.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_BOUNDED_STRING(STRUCT_NAME, CHAR_TYPE) \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_BOUNDED_STRING_DECLARE(STRUCT_NAME, CHAR_TYPE) \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_BOUNDED_STRING_DEFINE(STRUCT_NAME, CHAR_TYPE)

/// @brief Convenience macro declaring and defining a basic string in one place.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_STRING(STRUCT_NAME, CHAR_TYPE) \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_STRING_DECLARE(STRUCT_NAME, CHAR_TYPE); \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_STRING_DEFINE(STRUCT_NAME, CHAR_TYPE)

ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_STRING_DECLARE(
  rosidl_runtime_c__experimental__String, char);
ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_STRING_DECLARE(
  rosidl_runtime_c__experimental__WString, char16_t);

ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_BOUNDED_STRING_DECLARE(
  rosidl_runtime_c__experimental__BoundedString, char);
ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_BOUNDED_STRING_DECLARE(
  rosidl_runtime_c__experimental__BoundedWString, char16_t);

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_RUNTIME_C__EXPERIMENTAL__STRING_H_
