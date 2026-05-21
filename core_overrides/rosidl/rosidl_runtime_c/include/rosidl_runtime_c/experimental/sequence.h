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

#ifndef ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_H_
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <uchar.h>

#include "rcutils/allocator.h"
#include "rosidl_runtime_c/experimental/detail/value_helpers.h"
#include "rosidl_runtime_c/experimental/memory.h"
#include "rosidl_runtime_c/experimental/storage.h"
#include "rosidl_runtime_c/experimental/string.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// @file
/// @brief Experimental C11 sequence wrapper macros.

/// @brief Initialization options for experimental sequences.
///
/// Provides control over allocation, capacity, and external storage for sequence initialization.
typedef struct rosidl_primitive_sequence_init_options_s
{
  /// Optional allocator (NULL to use default allocator).
  const rcutils_allocator_t * allocator;

  /// Optional external storage region for sequence backing buffer (NULL for heap allocation).
  const rosidl_memory_region_t * external_storage;

  /// Reserved for future expansion (must be NULL).
  void * reserved[4];
} rosidl_primitive_sequence_init_options_t;

/// @brief Declare a bounded dynamic typed sequence wrapper and function signatures.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE( \
    STRUCT_NAME, VALUE_TYPE) \
  typedef rosidl_primitive_sequence_init_options_t STRUCT_NAME ## __InitOptions; \
  typedef struct STRUCT_NAME ## _s \
  { \
    VALUE_TYPE * value; \
    size_t size; \
    size_t capacity; \
    struct \
    { \
      rosidl_runtime_c__experimental__storage_kind_t kind; \
      union \
      { \
        rosidl_memory_region_t region; \
        VALUE_TYPE * data; \
      } storage; \
      rcutils_allocator_t allocator; \
      size_t upper_bound; \
    } _impl; \
  } STRUCT_NAME; \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound); \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound, \
    const STRUCT_NAME ## __InitOptions * options); \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _sequence); \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity); \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size); \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    VALUE_TYPE value); \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs); \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output);

/// @brief Declare a dynamic typed sequence wrapper and function signatures.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(STRUCT_NAME, VALUE_TYPE) \
  typedef rosidl_primitive_sequence_init_options_t STRUCT_NAME ## __InitOptions; \
  typedef struct STRUCT_NAME ## _s \
  { \
    VALUE_TYPE * value; \
    size_t size; \
    size_t capacity; \
    struct \
    { \
      rosidl_runtime_c__experimental__storage_kind_t kind; \
      union \
      { \
        rosidl_memory_region_t region; \
        VALUE_TYPE * data; \
      } storage; \
      rcutils_allocator_t allocator; \
    } _impl; \
  } STRUCT_NAME; \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence); \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    const STRUCT_NAME ## __InitOptions * options); \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _sequence); \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity); \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size); \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    VALUE_TYPE value); \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs); \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output);

/// @brief Define dynamic typed sequence functions.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DEFINE(STRUCT_NAME, VALUE_TYPE) \
  static bool STRUCT_NAME ## __init_with_allocator( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound, \
    const rcutils_allocator_t * allocator) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (allocator != NULL) { \
      if (!rcutils_allocator_is_valid(allocator)) { \
        return false; \
      } \
      _sequence->_impl.allocator = *allocator; \
    } else { \
      _sequence->_impl.allocator = rcutils_get_default_allocator(); \
    } \
    _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED; \
    _sequence->_impl.storage.data = NULL; \
    _sequence->_impl.upper_bound = upper_bound; \
    _sequence->value = NULL; \
    _sequence->size = 0U; \
    _sequence->capacity = 0U; \
    return true; \
  } \
  static bool STRUCT_NAME ## __init_with_region( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound, \
    rosidl_memory_region_t region) \
  { \
    if (_sequence == NULL || !rosidl_memory_region_is_valid(&region)) { \
      return false; \
    } \
    _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL; \
    _sequence->_impl.storage.region = region; \
    _sequence->_impl.upper_bound = upper_bound; \
    _sequence->_impl.allocator = rcutils_get_zero_initialized_allocator(); \
    _sequence->value = (VALUE_TYPE *)region.location.address; \
    _sequence->size = 0U; \
    _sequence->capacity = region.size / sizeof(VALUE_TYPE); \
    if (_sequence->capacity > _sequence->_impl.upper_bound) { \
      _sequence->capacity = _sequence->_impl.upper_bound; \
    } \
    return true; \
  } \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound, \
    const STRUCT_NAME ## __InitOptions * options) \
  { \
    if (options != NULL && options->external_storage != NULL) { \
      return STRUCT_NAME ## __init_with_region(_sequence, upper_bound, *options->external_storage); \
    } \
    const rcutils_allocator_t * allocator = options != NULL ? options->allocator : NULL; \
    return STRUCT_NAME ## __init_with_allocator(_sequence, upper_bound, allocator); \
  } \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound) \
  { \
    return STRUCT_NAME ## __init_with_options(_sequence, upper_bound, NULL); \
  } \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _sequence) \
  { \
    if (_sequence == NULL) { \
      return; \
    } \
    if (_sequence->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED && \
      _sequence->_impl.storage.data != NULL) \
    { \
      _sequence->_impl.allocator.deallocate( \
        _sequence->_impl.storage.data, \
        _sequence->_impl.allocator.state); \
    } \
    _sequence->value = NULL; \
    _sequence->size = 0U; \
    _sequence->capacity = 0U; \
    _sequence->_impl.storage.data = NULL; \
  } \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (requested_capacity > _sequence->_impl.upper_bound) { \
      return false; \
    } \
    if (requested_capacity <= _sequence->capacity) { \
      return true; \
    } \
    if (_sequence->_impl.kind != ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED) { \
      return false; \
    } \
    size_t new_capacity = rosidl_runtime_c__experimental__detail__next_capacity( \
      _sequence->capacity, requested_capacity); \
    if (_sequence->_impl.upper_bound > 0U && new_capacity > _sequence->_impl.upper_bound) { \
      new_capacity = _sequence->_impl.upper_bound; \
    } \
    size_t new_bytes = 0U; \
    if (!rosidl_runtime_c__experimental__detail__compute_bytes(new_capacity, sizeof(VALUE_TYPE), \
      &new_bytes)) { \
      return false; \
    } \
    void * reallocated = _sequence->_impl.allocator.reallocate( \
      _sequence->_impl.storage.data, new_bytes, _sequence->_impl.allocator.state); \
    if (reallocated == NULL) { \
      return false; \
    } \
    _sequence->_impl.storage.data = (VALUE_TYPE *)reallocated; \
    _sequence->value = _sequence->_impl.storage.data; \
    _sequence->capacity = new_capacity; \
    return true; \
  } \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (new_size > _sequence->capacity && \
      !STRUCT_NAME ## __reserve(_sequence, new_size)) \
    { \
      return false; \
    } \
    if (new_size > _sequence->size) { \
      (void)memset(&_sequence->value[_sequence->size], 0, \
        (new_size - _sequence->size) * sizeof(VALUE_TYPE)); \
    } \
    _sequence->size = new_size; \
    return true; \
  } \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    VALUE_TYPE value) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (_sequence->size == _sequence->capacity && \
      !STRUCT_NAME ## __reserve(_sequence, _sequence->size + 1U)) \
    { \
      return false; \
    } \
    _sequence->value[_sequence->size] = value; \
    ++_sequence->size; \
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
    return memcmp(lhs->value, rhs->value, lhs->size * sizeof(VALUE_TYPE)) == 0; \
  } \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output) \
  { \
    if (input == NULL || output == NULL) { \
      return false; \
    } \
    if (input->value == NULL || output->value == NULL) { \
      return false; \
    } \
    if (input == output) { \
      return true; \
    } \
    if (!STRUCT_NAME ## __resize(output, input->size)) { \
      return false; \
    } \
    if (input->size > 0U) { \
      (void)memcpy(output->value, input->value, input->size * sizeof(VALUE_TYPE)); \
    } \
    return true; \
  }

/// @brief Define sequence functions declared with
/// ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DEFINE.
/// This macro defines unbounded sequences.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DEFINE(STRUCT_NAME, VALUE_TYPE) \
  static bool STRUCT_NAME ## __init_with_allocator( \
    STRUCT_NAME * _sequence, \
    const rcutils_allocator_t * allocator) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (allocator != NULL) { \
      if (!rcutils_allocator_is_valid(allocator)) { \
        return false; \
      } \
      _sequence->_impl.allocator = *allocator; \
    } else { \
      _sequence->_impl.allocator = rcutils_get_default_allocator(); \
    } \
    _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED; \
    _sequence->_impl.storage.data = NULL; \
    _sequence->value = NULL; \
    _sequence->size = 0U; \
    _sequence->capacity = 0U; \
    return true; \
  } \
  static bool STRUCT_NAME ## __init_with_region( \
    STRUCT_NAME * _sequence, \
    rosidl_memory_region_t region) \
  { \
    if (_sequence == NULL || !rosidl_memory_region_is_valid(&region)) { \
      return false; \
    } \
    _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL; \
    _sequence->_impl.storage.region = region; \
    _sequence->_impl.allocator = rcutils_get_zero_initialized_allocator(); \
    _sequence->value = (VALUE_TYPE *)region.location.address; \
    _sequence->size = 0U; \
    _sequence->capacity = region.size / sizeof(VALUE_TYPE); \
    return true; \
  } \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    const STRUCT_NAME ## __InitOptions * options) \
  { \
    if (options != NULL && options->external_storage != NULL) { \
      return STRUCT_NAME ## __init_with_region(_sequence, *options->external_storage); \
    } \
    const rcutils_allocator_t * allocator = options != NULL ? options->allocator : NULL; \
    return STRUCT_NAME ## __init_with_allocator(_sequence, allocator); \
  } \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence) \
  { \
    return STRUCT_NAME ## __init_with_options(_sequence, NULL); \
  } \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _sequence) \
  { \
    if (_sequence == NULL) { \
      return; \
    } \
    if (_sequence->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED && \
      _sequence->_impl.storage.data != NULL) \
    { \
      _sequence->_impl.allocator.deallocate( \
        _sequence->_impl.storage.data, \
        _sequence->_impl.allocator.state); \
    } \
    _sequence->_impl.storage.data = NULL; \
    _sequence->value = NULL; \
    _sequence->size = 0U; \
    _sequence->capacity = 0U; \
  } \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (requested_capacity <= _sequence->capacity) { \
      return true; \
    } \
    if (_sequence->_impl.kind != ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED) { \
      return false; \
    } \
    size_t new_capacity = rosidl_runtime_c__experimental__detail__next_capacity( \
      _sequence->capacity, requested_capacity); \
    size_t new_bytes = 0U; \
    if (!rosidl_runtime_c__experimental__detail__compute_bytes(new_capacity, sizeof(VALUE_TYPE), \
      &new_bytes)) { \
      return false; \
    } \
    void * reallocated = _sequence->_impl.allocator.reallocate( \
      _sequence->_impl.storage.data, new_bytes, _sequence->_impl.allocator.state); \
    if (reallocated == NULL) { \
      return false; \
    } \
    _sequence->_impl.storage.data = (VALUE_TYPE *)reallocated; \
    _sequence->value = _sequence->_impl.storage.data; \
    _sequence->capacity = new_capacity; \
    return true; \
  } \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (new_size > _sequence->capacity && \
      !STRUCT_NAME ## __reserve(_sequence, new_size)) \
    { \
      return false; \
    } \
    if (new_size > _sequence->size) { \
      (void)memset(&_sequence->value[_sequence->size], 0, \
        (new_size - _sequence->size) * sizeof(VALUE_TYPE)); \
    } \
    _sequence->size = new_size; \
    return true; \
  } \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    VALUE_TYPE value) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (!STRUCT_NAME ## __reserve(_sequence, _sequence->size + 1U)) { \
      return false; \
    } \
    _sequence->value[_sequence->size++] = value; \
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
    return memcmp(lhs->value, rhs->value, lhs->size * sizeof(VALUE_TYPE)) == 0; \
  } \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output) \
  { \
    if (input == NULL || output == NULL) { \
      return false; \
    } \
    if (input->value == NULL || output->value == NULL) { \
      return false; \
    } \
    if (input == output) { \
      return true; \
    } \
    if (!STRUCT_NAME ## __resize(output, input->size)) { \
      return false; \
    } \
    if (input->size > 0U) { \
      (void)memcpy(output->value, input->value, input->size * sizeof(VALUE_TYPE)); \
    } \
    return true; \
  }

#define ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE(STRUCT_NAME, VALUE_TYPE) \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(STRUCT_NAME, VALUE_TYPE) \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DEFINE(STRUCT_NAME, VALUE_TYPE)

/// @brief Create a type alias for an unbounded primitive sequence type with function forwarding.
/// Generates a typedef and static inline forwarding functions for all sequence operations.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_ALIAS(ALIAS_NAME, BASE_TYPE, VALUE_TYPE) \
  typedef BASE_TYPE ALIAS_NAME; \
  typedef BASE_TYPE ## __InitOptions ALIAS_NAME ## __InitOptions; \
  static inline bool ALIAS_NAME ## __init(ALIAS_NAME * _sequence) \
  { \
    return BASE_TYPE ## __init(_sequence); \
  } \
  static inline bool ALIAS_NAME ## __init_with_options( \
    ALIAS_NAME * _sequence, \
    const ALIAS_NAME ## __InitOptions * options) \
  { \
    return BASE_TYPE ## __init_with_options(_sequence, options); \
  } \
  static inline void ALIAS_NAME ## __fini(ALIAS_NAME * _sequence) \
  { \
    BASE_TYPE ## __fini(_sequence); \
  } \
  static inline bool ALIAS_NAME ## __reserve( \
    ALIAS_NAME * _sequence, \
    size_t requested_capacity) \
  { \
    return BASE_TYPE ## __reserve(_sequence, requested_capacity); \
  } \
  static inline bool ALIAS_NAME ## __resize( \
    ALIAS_NAME * _sequence, \
    size_t new_size) \
  { \
    return BASE_TYPE ## __resize(_sequence, new_size); \
  } \
  static inline bool ALIAS_NAME ## __push_back( \
    ALIAS_NAME * _sequence, \
    VALUE_TYPE value) \
  { \
    return BASE_TYPE ## __push_back(_sequence, value); \
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

#define ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE(STRUCT_NAME, VALUE_TYPE) \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(STRUCT_NAME, VALUE_TYPE) \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DEFINE(STRUCT_NAME, VALUE_TYPE)

/// @brief Create a type alias for a bounded primitive sequence type with function forwarding.
/// Generates a typedef and static inline forwarding functions including upper_bound parameter.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_ALIAS(ALIAS_NAME, BASE_TYPE, VALUE_TYPE) \
  typedef BASE_TYPE ALIAS_NAME; \
  typedef BASE_TYPE ## __InitOptions ALIAS_NAME ## __InitOptions; \
  static inline bool ALIAS_NAME ## __init(ALIAS_NAME * _sequence, size_t upper_bound) \
  { \
    return BASE_TYPE ## __init(_sequence, upper_bound); \
  } \
  static inline bool ALIAS_NAME ## __init_with_options( \
    ALIAS_NAME * _sequence, \
    size_t upper_bound, \
    const ALIAS_NAME ## __InitOptions * options) \
  { \
    return BASE_TYPE ## __init_with_options(_sequence, upper_bound, options); \
  } \
  static inline void ALIAS_NAME ## __fini(ALIAS_NAME * _sequence) \
  { \
    BASE_TYPE ## __fini(_sequence); \
  } \
  static inline bool ALIAS_NAME ## __reserve( \
    ALIAS_NAME * _sequence, \
    size_t requested_capacity) \
  { \
    return BASE_TYPE ## __reserve(_sequence, requested_capacity); \
  } \
  static inline bool ALIAS_NAME ## __resize( \
    ALIAS_NAME * _sequence, \
    size_t new_size) \
  { \
    return BASE_TYPE ## __resize(_sequence, new_size); \
  } \
  static inline bool ALIAS_NAME ## __push_back( \
    ALIAS_NAME * _sequence, \
    VALUE_TYPE value) \
  { \
    return BASE_TYPE ## __push_back(_sequence, value); \
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

/// @brief Declare a bounded sequence of object (non-primitive) elements.
///
/// Unlike the PRIMITIVE_BOUNDED_SEQUENCE, elements are heap-allocated individually
/// (no realloc), so ELEMENT_TYPE may itself contain pointers into managed memory.
/// push_back takes a pointer (not a value) to avoid requiring copyability by value.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DECLARE( \
    STRUCT_NAME, ELEMENT_TYPE) \
  typedef struct STRUCT_NAME ## __InitOptions_s { \
    const rcutils_allocator_t * allocator; \
    const ELEMENT_TYPE ## __ExternalStorage * external_element_storage; \
    const rosidl_memory_region_t * external_storage; \
    void * reserved[4]; \
  } STRUCT_NAME ## __InitOptions; \
  typedef struct STRUCT_NAME ## _s \
  { \
    ELEMENT_TYPE * value; \
    size_t size; \
    size_t capacity; \
    struct \
    { \
      rosidl_runtime_c__experimental__storage_kind_t kind; \
      union \
      { \
        rosidl_memory_region_t region; \
        ELEMENT_TYPE * data; \
      } storage; \
      rcutils_allocator_t allocator; \
      struct { \
        const ELEMENT_TYPE ## __ExternalStorage * storage; \
      } prototype; \
      size_t upper_bound; \
    } _impl; \
  } STRUCT_NAME; \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound); \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound, \
    const STRUCT_NAME ## __InitOptions * options); \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _sequence); \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity); \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size); \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    const ELEMENT_TYPE * value); \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs); \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output);

/// @brief Declare an unbounded sequence of object (non-primitive) elements.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_DECLARE(STRUCT_NAME, ELEMENT_TYPE) \
  typedef struct STRUCT_NAME ## __InitOptions_s { \
    const rcutils_allocator_t * allocator; \
    const ELEMENT_TYPE ## __ExternalStorage * external_element_storage; \
    size_t external_element_storage_size; \
    const rosidl_memory_region_t * external_storage; \
    void * reserved[4]; \
  } STRUCT_NAME ## __InitOptions; \
  typedef struct STRUCT_NAME ## _s \
  { \
    ELEMENT_TYPE * value; \
    size_t size; \
    size_t capacity; \
    struct \
    { \
      rosidl_runtime_c__experimental__storage_kind_t kind; \
      union \
      { \
        rosidl_memory_region_t region; \
        ELEMENT_TYPE * data; \
      } storage; \
      rcutils_allocator_t allocator; \
      struct { \
        const ELEMENT_TYPE ## __ExternalStorage * storage; \
        size_t max_instances; \
      } prototype; \
    } _impl; \
  } STRUCT_NAME; \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence); \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    const STRUCT_NAME ## __InitOptions * options); \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _sequence); \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity); \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size); \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    const ELEMENT_TYPE * value); \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs); \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output);

/// @brief Define a bounded sequence of object elements.
///
/// reserve allocates a new backing buffer and per-element-initialises every new
/// slot, then copies existing elements; it does NOT use realloc (which would
/// invalidate per-element internal pointers such as those in String / sub-message
/// fields).
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DEFINE(STRUCT_NAME, ELEMENT_TYPE) \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound, \
    const STRUCT_NAME ## __InitOptions * options) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (options != NULL && options->allocator != NULL) { \
      if (!rcutils_allocator_is_valid(options->allocator)) { \
        return false; \
      } \
      _sequence->_impl.allocator = *options->allocator; \
    } else { \
      _sequence->_impl.allocator = rcutils_get_default_allocator(); \
    } \
    _sequence->_impl.upper_bound = upper_bound; \
    if (options != NULL && options->external_storage != NULL) { \
      _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL; \
      _sequence->_impl.storage.region = *options->external_storage; \
      _sequence->value = (ELEMENT_TYPE *)_sequence->_impl.storage.region.location.address; \
      _sequence->capacity = _sequence->_impl.storage.region.size / sizeof(ELEMENT_TYPE); \
      _sequence->size = 0U; \
      if (_sequence->capacity > _sequence->_impl.upper_bound) { \
        _sequence->capacity = _sequence->_impl.upper_bound; \
      } \
    } else { \
      _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED; \
      _sequence->_impl.storage.data = NULL; \
      _sequence->value = NULL; \
      _sequence->capacity = 0U; \
      _sequence->size = 0U; \
    } \
    _sequence->_impl.prototype.storage = options != NULL ? options->external_element_storage : NULL; \
    return true; \
  } \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound) \
  { \
    return STRUCT_NAME ## __init_with_options(_sequence, upper_bound, NULL); \
  } \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _sequence) \
  { \
    if (_sequence == NULL) { \
      return; \
    } \
    if (_sequence->value != NULL) { \
      for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
        ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
      } \
    } \
    if (_sequence->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED && \
      _sequence->_impl.storage.data != NULL) \
    { \
      _sequence->_impl.allocator.deallocate( \
        _sequence->_impl.storage.data, _sequence->_impl.allocator.state); \
      _sequence->_impl.storage.data = NULL; \
    } \
    _sequence->value = NULL; \
    _sequence->size = 0U; \
    _sequence->capacity = 0U; \
  } \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (requested_capacity <= _sequence->capacity) { \
      return true; \
    } \
    if (requested_capacity > _sequence->_impl.upper_bound) { \
      return false; \
    } \
    if (_sequence->_impl.kind != ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED) { \
      return false; \
    } \
    size_t new_capacity = rosidl_runtime_c__experimental__detail__next_capacity( \
      _sequence->capacity, requested_capacity); \
    if (new_capacity > _sequence->_impl.upper_bound) { \
      new_capacity = _sequence->_impl.upper_bound; \
    } \
    size_t new_bytes = 0U; \
    if (!rosidl_runtime_c__experimental__detail__compute_bytes( \
        new_capacity, sizeof(ELEMENT_TYPE), &new_bytes)) \
    { \
      return false; \
    } \
    ELEMENT_TYPE * new_data = \
      (ELEMENT_TYPE *)_sequence->_impl.allocator.allocate( \
        new_bytes, _sequence->_impl.allocator.state); \
    if (new_data == NULL) { \
      return false; \
    } \
    for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
      ELEMENT_TYPE ## __InitOptions _element_options = {0}; \
      _element_options.allocator = &_sequence->_impl.allocator; \
      if (_sequence->_impl.prototype.storage != NULL) { \
        _element_options.external_storage = &_sequence->_impl.prototype.storage[_i]; \
      } \
      if (!ELEMENT_TYPE ## __init_with_options(&new_data[_i], &_element_options)) { \
        for (size_t _j = 0U; _j < _i; ++_j) { \
          ELEMENT_TYPE ## __fini(&new_data[_j]); \
        } \
        _sequence->_impl.allocator.deallocate( \
          new_data, _sequence->_impl.allocator.state); \
        return false; \
      } \
      if (!ELEMENT_TYPE ## __copy(&_sequence->value[_i], &new_data[_i])) { \
        for (size_t _j = 0U; _j <= _i; ++_j) { \
          ELEMENT_TYPE ## __fini(&new_data[_j]); \
        } \
        _sequence->_impl.allocator.deallocate( \
          new_data, _sequence->_impl.allocator.state); \
        return false; \
      } \
    } \
    for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
      ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
    } \
    _sequence->_impl.storage.data = new_data; \
    _sequence->value = new_data; \
    _sequence->capacity = new_capacity; \
    return true; \
  } \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (new_size == _sequence->size) { \
      return true; \
    } \
    if (new_size > _sequence->size) { \
      if (!STRUCT_NAME ## __reserve(_sequence, new_size)) { \
        return false; \
      } \
      for (size_t _i = _sequence->size; _i < new_size; ++_i) { \
        ELEMENT_TYPE ## __InitOptions _element_options = {0}; \
        _element_options.allocator = &_sequence->_impl.allocator; \
        if (_sequence->_impl.prototype.storage != NULL) { \
          _element_options.external_storage = &_sequence->_impl.prototype.storage[_i]; \
        } \
        if (!ELEMENT_TYPE ## __init_with_options(&_sequence->value[_i], &_element_options)) \
        { \
          for (size_t _j = _i - 1; _j > _sequence->size; --_j) { \
            ELEMENT_TYPE ## __fini(&_sequence->value[_j]); \
          } \
          return false; \
        } \
      } \
    } else { \
      for (size_t _i = _sequence->size - 1; _i >= new_size; --_i) { \
        ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
      } \
    } \
    _sequence->size = new_size; \
    return true; \
  } \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    const ELEMENT_TYPE * value) \
  { \
    if (_sequence == NULL || value == NULL) { \
      return false; \
    } \
    if (!STRUCT_NAME ## __resize(_sequence, _sequence->size + 1U)) { \
      return false; \
    } \
    return ELEMENT_TYPE ## __copy(value, &_sequence->value[_sequence->size - 1]); \
  } \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs) \
  { \
    if (lhs == NULL || rhs == NULL) { \
      return false; \
    } \
    if (lhs->value == NULL || rhs->value == NULL) { \
      return false; \
    } \
    if (lhs->size != rhs->size) { \
      return false; \
    } \
    if (lhs->size == 0U) { \
      return true; \
    } \
    for (size_t _i = 0U; _i < lhs->size; ++_i) { \
      if (!ELEMENT_TYPE ## __are_equal(&lhs->value[_i], &rhs->value[_i])) { \
        return false; \
      } \
    } \
    return true; \
  } \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output) \
  { \
    if (input == NULL || output == NULL) { \
      return false; \
    } \
    if (input->value == NULL || output->value == NULL) { \
      return false; \
    } \
    if (input == output) { \
      return true; \
    } \
    if (!STRUCT_NAME ## __resize(output, input->size)) { \
      return false; \
    } \
    for (size_t _i = 0U; _i < input->size; ++_i) { \
      if (!ELEMENT_TYPE ## __copy(&input->value[_i], &output->value[_i])) { \
        return false; \
      } \
    } \
    return true; \
  }

/// @brief Define an unbounded sequence of object elements.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_DEFINE(STRUCT_NAME, ELEMENT_TYPE) \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    const STRUCT_NAME ## __InitOptions * options) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (options != NULL && options->allocator != NULL) { \
      if (!rcutils_allocator_is_valid(options->allocator)) { \
        return false; \
      } \
      _sequence->_impl.allocator = *options->allocator; \
    } else { \
      _sequence->_impl.allocator = rcutils_get_default_allocator(); \
    } \
    if (options != NULL && options->external_storage != NULL) { \
      _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL; \
      _sequence->_impl.storage.region = *options->external_storage; \
      _sequence->value = (ELEMENT_TYPE *)_sequence->_impl.storage.region.location.address; \
      _sequence->capacity = _sequence->_impl.storage.region.size / sizeof(ELEMENT_TYPE); \
      _sequence->size = 0U; \
    } else { \
      _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED; \
      _sequence->_impl.storage.data = NULL; \
      _sequence->value = NULL; \
      _sequence->capacity = 0U; \
      _sequence->size = 0U; \
    } \
    if (options != NULL) { \
      _sequence->_impl.prototype.storage = options->external_element_storage; \
      _sequence->_impl.prototype.max_instances = options->external_element_storage_size; \
    } else { \
      _sequence->_impl.prototype.storage = NULL; \
      _sequence->_impl.prototype.max_instances = 0U; \
    } \
    return true; \
  } \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence) \
  { \
    return STRUCT_NAME ## __init_with_options(_sequence, NULL); \
  } \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _sequence) \
  { \
    if (_sequence == NULL) { \
      return; \
    } \
    if (_sequence->value != NULL) { \
      for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
        ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
      } \
    } \
    if (_sequence->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED && \
      _sequence->_impl.storage.data != NULL) \
    { \
      _sequence->_impl.allocator.deallocate( \
        _sequence->_impl.storage.data, _sequence->_impl.allocator.state); \
      _sequence->_impl.storage.data = NULL; \
    } \
    _sequence->value = NULL; \
    _sequence->size = 0U; \
    _sequence->capacity = 0U; \
  } \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (requested_capacity <= _sequence->capacity) { \
      return true; \
    } \
    if (_sequence->_impl.kind != ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED) { \
      return false; \
    } \
    if (_sequence->_impl.prototype.max_instances > 0U && \
      requested_capacity > _sequence->_impl.prototype.max_instances) \
    { \
      return false; \
    } \
    size_t new_capacity = rosidl_runtime_c__experimental__detail__next_capacity( \
      _sequence->capacity, requested_capacity); \
    if (_sequence->_impl.prototype.max_instances > 0U && \
      new_capacity > _sequence->_impl.prototype.max_instances) \
    { \
      new_capacity = _sequence->_impl.prototype.max_instances; \
    } \
    size_t new_bytes = 0U; \
    if (!rosidl_runtime_c__experimental__detail__compute_bytes( \
        new_capacity, sizeof(ELEMENT_TYPE), &new_bytes)) \
    { \
      return false; \
    } \
    ELEMENT_TYPE * new_data = (ELEMENT_TYPE *)_sequence->_impl.allocator.allocate( \
      new_bytes, _sequence->_impl.allocator.state); \
    if (new_data == NULL) { \
      return false; \
    } \
    for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
      ELEMENT_TYPE ## __InitOptions _element_options = {0}; \
      _element_options.allocator = &_sequence->_impl.allocator; \
      if (_sequence->_impl.prototype.storage != NULL) { \
        _element_options.external_storage = &_sequence->_impl.prototype.storage[_i]; \
      } \
      if (!ELEMENT_TYPE ## __init_with_options(&new_data[_i], &_element_options)) { \
        for (size_t _j = 0U; _j < _i; ++_j) { \
          ELEMENT_TYPE ## __fini(&new_data[_j]); \
        } \
        _sequence->_impl.allocator.deallocate( \
          new_data, _sequence->_impl.allocator.state); \
        return false; \
      } \
      if (!ELEMENT_TYPE ## __copy(&_sequence->value[_i], &new_data[_i])) { \
        for (size_t _j = 0U; _j <= _i; ++_j) { \
          ELEMENT_TYPE ## __fini(&new_data[_j]); \
        } \
        _sequence->_impl.allocator.deallocate( \
          new_data, _sequence->_impl.allocator.state); \
        return false; \
      } \
    } \
    for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
      ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
    } \
    _sequence->_impl.storage.data = new_data; \
    _sequence->value = new_data; \
    _sequence->capacity = new_capacity; \
    return true; \
  } \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (new_size == _sequence->size) { \
      return true; \
    } \
    if (new_size > _sequence->size) { \
      if (!STRUCT_NAME ## __reserve(_sequence, new_size)) { \
        return false; \
      } \
      for (size_t _i = _sequence->size; _i < new_size; ++_i) { \
        ELEMENT_TYPE ## __InitOptions _element_options = {0}; \
        _element_options.allocator = &_sequence->_impl.allocator; \
        if (_sequence->_impl.prototype.storage != NULL) { \
          _element_options.external_storage = &_sequence->_impl.prototype.storage[_i]; \
        } \
        if (!ELEMENT_TYPE ## __init_with_options(&_sequence->value[_i], &_element_options)) \
        { \
          for (size_t _j = _i - 1; _j > _sequence->size; --_j) { \
            ELEMENT_TYPE ## __fini(&_sequence->value[_j]); \
          } \
          return false; \
        } \
      } \
    } else { \
      for (size_t _i = _sequence->size - 1; _i >= new_size; --_i) { \
        ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
      } \
    } \
    _sequence->size = new_size; \
    return true; \
  } \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    const ELEMENT_TYPE * value) \
  { \
    if (_sequence == NULL || value == NULL) { \
      return false; \
    } \
    if (!STRUCT_NAME ## __resize(_sequence, _sequence->size + 1U)) { \
      return false; \
    } \
    return ELEMENT_TYPE ## __copy(value, &_sequence->value[_sequence->size - 1]); \
  } \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs) \
  { \
    if (lhs == NULL || rhs == NULL) { \
      return false; \
    } \
    if (lhs->value == NULL || rhs->value == NULL) { \
      return false; \
    } \
    if (lhs->size != rhs->size) { \
      return false; \
    } \
    if (lhs->size == 0U) { \
      return true; \
    } \
    for (size_t _i = 0U; _i < lhs->size; ++_i) { \
      if (!ELEMENT_TYPE ## __are_equal(&lhs->value[_i], &rhs->value[_i])) { \
        return false; \
      } \
    } \
    return true; \
  } \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output) \
  { \
    if (input == NULL || output == NULL) { \
      return false; \
    } \
    if (input->value == NULL || output->value == NULL) { \
      return false; \
    } \
    if (input == output) { \
      return true; \
    } \
    if (!STRUCT_NAME ## __resize(output, input->size)) { \
      return false; \
    } \
    for (size_t _i = 0U; _i < input->size; ++_i) { \
      if (!ELEMENT_TYPE ## __copy(&input->value[_i], &output->value[_i])) { \
        return false; \
      } \
    } \
    return true; \
  }

/// @brief Convenience macro declaring and defining a bounded object sequence in one place.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE(STRUCT_NAME, ELEMENT_TYPE) \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DECLARE(STRUCT_NAME, ELEMENT_TYPE); \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DEFINE(STRUCT_NAME, ELEMENT_TYPE)

/// @brief Convenience macro declaring and defining an unbounded object sequence in one place.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE(STRUCT_NAME, ELEMENT_TYPE) \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_DECLARE(STRUCT_NAME, ELEMENT_TYPE); \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_DEFINE(STRUCT_NAME, ELEMENT_TYPE)

/// @brief Create a type alias for an unbounded sequence type with function forwarding.
/// Generates a typedef and static inline forwarding functions for all sequence operations.
/// This version includes init_region_storage for complex element sequences.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_ALIAS(ALIAS_NAME, BASE_TYPE, ELEMENT_TYPE) \
  typedef BASE_TYPE ALIAS_NAME; \
  typedef BASE_TYPE ## __InitOptions ALIAS_NAME ## __InitOptions; \
  static inline bool ALIAS_NAME ## __init(ALIAS_NAME * _sequence) \
  { \
    return BASE_TYPE ## __init(_sequence); \
  } \
  static inline bool ALIAS_NAME ## __init_with_options( \
    ALIAS_NAME * _sequence, \
    const ALIAS_NAME ## __InitOptions * options) \
  { \
    return BASE_TYPE ## __init_with_options(_sequence, options); \
  } \
  static inline void ALIAS_NAME ## __fini(ALIAS_NAME * _sequence) \
  { \
    BASE_TYPE ## __fini(_sequence); \
  } \
  static inline bool ALIAS_NAME ## __reserve( \
    ALIAS_NAME * _sequence, \
    size_t requested_capacity) \
  { \
    return BASE_TYPE ## __reserve(_sequence, requested_capacity); \
  } \
  static inline bool ALIAS_NAME ## __resize( \
    ALIAS_NAME * _sequence, \
    size_t new_size) \
  { \
    return BASE_TYPE ## __resize(_sequence, new_size); \
  } \
  static inline bool ALIAS_NAME ## __push_back( \
    ALIAS_NAME * _sequence, \
    const ELEMENT_TYPE * value) \
  { \
    return BASE_TYPE ## __push_back(_sequence, value); \
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

/// @brief Create a type alias for a bounded sequence type with function forwarding.
/// Generates a typedef and static inline forwarding functions including upper_bound parameter.
/// This version includes init_region_storage for complex element sequences.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_ALIAS(ALIAS_NAME, BASE_TYPE, ELEMENT_TYPE) \
  typedef BASE_TYPE ALIAS_NAME; \
  typedef BASE_TYPE ## __InitOptions ALIAS_NAME ## __InitOptions; \
  static inline bool ALIAS_NAME ## __init(ALIAS_NAME * _sequence, size_t upper_bound) \
  { \
    return BASE_TYPE ## __init(_sequence, upper_bound); \
  } \
  static inline bool ALIAS_NAME ## __init_with_options( \
    ALIAS_NAME * _sequence, \
    size_t upper_bound, \
    const ALIAS_NAME ## __InitOptions * options) \
  { \
    return BASE_TYPE ## __init_with_options(_sequence, upper_bound, options); \
  } \
  static inline void ALIAS_NAME ## __fini(ALIAS_NAME * _sequence) \
  { \
    BASE_TYPE ## __fini(_sequence); \
  } \
  static inline bool ALIAS_NAME ## __reserve( \
    ALIAS_NAME * _sequence, \
    size_t requested_capacity) \
  { \
    return BASE_TYPE ## __reserve(_sequence, requested_capacity); \
  } \
  static inline bool ALIAS_NAME ## __resize( \
    ALIAS_NAME * _sequence, \
    size_t new_size) \
  { \
    return BASE_TYPE ## __resize(_sequence, new_size); \
  } \
  static inline bool ALIAS_NAME ## __push_back( \
    ALIAS_NAME * _sequence, \
    const ELEMENT_TYPE * value) \
  { \
    return BASE_TYPE ## __push_back(_sequence, value); \
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

/// @brief Declare an unbounded sequence of bounded elements.
/// This macro creates a sequence where elements require a bound parameter during initialization.
/// Used for sequences of BoundedString, BoundedWString, or other bounded types.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_SEQUENCE_DECLARE( \
    STRUCT_NAME, ELEMENT_TYPE) \
  typedef struct STRUCT_NAME ## __InitOptions_s { \
    const rcutils_allocator_t * allocator; \
    const ELEMENT_TYPE ## __ExternalStorage * external_element_storage; \
    size_t external_element_storage_size; \
    const rosidl_memory_region_t * external_storage; \
    void * reserved[4]; \
  } STRUCT_NAME ## __InitOptions; \
  typedef struct STRUCT_NAME ## _s \
  { \
    ELEMENT_TYPE * value; \
    size_t size; \
    size_t capacity; \
    struct \
    { \
      rosidl_runtime_c__experimental__storage_kind_t kind; \
      union \
      { \
        rosidl_memory_region_t region; \
        ELEMENT_TYPE * data; \
      } storage; \
      rcutils_allocator_t allocator; \
      struct { \
        const ELEMENT_TYPE ## __ExternalStorage * storage; \
        size_t max_instances; \
        size_t upper_bound; \
      } prototype; \
    } _impl; \
  } STRUCT_NAME; \
  bool STRUCT_NAME ## __init(STRUCT_NAME * _sequence, size_t element_bound); \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    size_t element_bound, \
    const STRUCT_NAME ## __InitOptions * options); \
  void STRUCT_NAME ## __fini(STRUCT_NAME * _sequence); \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity); \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size); \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    const ELEMENT_TYPE * value); \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs); \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output);

/// @brief Declare a bounded sequence of bounded elements.
/// This macro creates a sequence with an upper bound where elements also require bounds.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_BOUNDED_SEQUENCE_DECLARE( \
    STRUCT_NAME, ELEMENT_TYPE) \
  typedef struct STRUCT_NAME ## __InitOptions_s { \
    const rcutils_allocator_t * allocator; \
    const ELEMENT_TYPE ## __ExternalStorage * external_element_storage; \
    const rosidl_memory_region_t * external_storage; \
    void * reserved[4]; \
  } STRUCT_NAME ## __InitOptions; \
  typedef struct STRUCT_NAME ## _s \
  { \
    ELEMENT_TYPE * value; \
    size_t size; \
    size_t capacity; \
    struct \
    { \
      rosidl_runtime_c__experimental__storage_kind_t kind; \
      union \
      { \
        rosidl_memory_region_t region; \
        ELEMENT_TYPE * data; \
      } storage; \
      rcutils_allocator_t allocator; \
      struct { \
        const ELEMENT_TYPE ## __ExternalStorage * storage; \
        size_t upper_bound; \
      } prototype; \
      size_t upper_bound; \
    } _impl; \
  } STRUCT_NAME; \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound, \
    size_t element_upper_bound); \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound, \
    size_t element_upper_bound, \
    const STRUCT_NAME ## __InitOptions * options); \
  void STRUCT_NAME ## __fini(STRUCT_NAME * _sequence); \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity); \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size); \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    const ELEMENT_TYPE * value); \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs); \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output);


/// @brief Define functions for an unbounded sequence of bounded elements.
/// Elements require a bound parameter during initialization (e.g., BoundedString).
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_SEQUENCE_DEFINE( \
    STRUCT_NAME, ELEMENT_TYPE) \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    size_t element_upper_bound, \
    const STRUCT_NAME ## __InitOptions * options) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (options != NULL && options->allocator != NULL) { \
      if (!rcutils_allocator_is_valid(options->allocator)) { \
        return false; \
      } \
      _sequence->_impl.allocator = *options->allocator; \
    } else { \
      _sequence->_impl.allocator = rcutils_get_default_allocator(); \
    } \
    if (options != NULL && options->external_storage != NULL) { \
      _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL; \
      _sequence->_impl.storage.region = *options->external_storage; \
      _sequence->value = (ELEMENT_TYPE *)_sequence->_impl.storage.region.location.address; \
      _sequence->capacity = _sequence->_impl.storage.region.size / sizeof(ELEMENT_TYPE); \
      _sequence->size = 0U; \
    } else { \
      _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED; \
      _sequence->_impl.storage.data = NULL; \
      _sequence->value = NULL; \
      _sequence->capacity = 0U; \
      _sequence->size = 0U; \
    } \
    _sequence->_impl.prototype.upper_bound = element_upper_bound; \
    if (options != NULL) { \
      _sequence->_impl.prototype.storage = options->external_element_storage; \
      _sequence->_impl.prototype.max_instances = options->external_element_storage_size; \
    } else { \
      _sequence->_impl.prototype.storage = NULL; \
      _sequence->_impl.prototype.max_instances = 0U; \
    } \
    return true; \
  } \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence, \
    size_t element_upper_bound) \
  { \
    return STRUCT_NAME ## __init_with_options(_sequence, element_upper_bound, NULL); \
  } \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _sequence) \
  { \
    if (_sequence == NULL) { \
      return; \
    } \
    if (_sequence->value != NULL) { \
      for (size_t _i = 0U; _i < _sequence->capacity; ++_i) { \
        ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
      } \
    } \
    if (_sequence->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED && \
      _sequence->_impl.storage.data != NULL) \
    { \
      _sequence->_impl.allocator.deallocate( \
        _sequence->_impl.storage.data, _sequence->_impl.allocator.state); \
      _sequence->_impl.storage.data = NULL; \
    } \
    _sequence->value = NULL; \
    _sequence->size = 0U; \
    _sequence->capacity = 0U; \
  } \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (requested_capacity <= _sequence->capacity) { \
      return true; \
    } \
    if (_sequence->_impl.kind != ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED) { \
      return false; \
    } \
    if (_sequence->_impl.prototype.max_instances > 0U && \
      requested_capacity > _sequence->_impl.prototype.max_instances) \
    { \
      return false; \
    } \
    size_t new_capacity = rosidl_runtime_c__experimental__detail__next_capacity( \
      _sequence->capacity, requested_capacity); \
    if (_sequence->_impl.prototype.max_instances > 0U && \
      new_capacity > _sequence->_impl.prototype.max_instances) \
    { \
      new_capacity = _sequence->_impl.prototype.max_instances; \
    } \
    size_t bytes_to_allocate = new_capacity * sizeof(ELEMENT_TYPE); \
    ELEMENT_TYPE * new_data = (ELEMENT_TYPE *)_sequence->_impl.allocator.allocate( \
      bytes_to_allocate, _sequence->_impl.allocator.state); \
    if (new_data == NULL) { \
      return false; \
    } \
    for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
      ELEMENT_TYPE ## __InitOptions _element_options = {0}; \
      _element_options.allocator = &_sequence->_impl.allocator; \
      if (_sequence->_impl.prototype.storage != NULL) { \
        _element_options.external_storage = &_sequence->_impl.prototype.storage[_i]; \
      } \
      if (!ELEMENT_TYPE ## __init_with_options( \
          &new_data[_i], _sequence->_impl.prototype.upper_bound, &_element_options)) { \
        for (size_t _j = 0U; _j < _i; ++_j) { \
          ELEMENT_TYPE ## __fini(&new_data[_j]); \
        } \
        _sequence->_impl.allocator.deallocate( \
          new_data, _sequence->_impl.allocator.state); \
        return false; \
      } \
      if (!ELEMENT_TYPE ## __copy(&_sequence->value[_i], &new_data[_i])) { \
        for (size_t _j = 0U; _j <= _i; ++_j) { \
          ELEMENT_TYPE ## __fini(&new_data[_j]); \
        } \
        _sequence->_impl.allocator.deallocate( \
          new_data, _sequence->_impl.allocator.state); \
        return false; \
      } \
    } \
    for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
      ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
    } \
    _sequence->_impl.storage.data = new_data; \
    _sequence->value = new_data; \
    _sequence->capacity = new_capacity; \
    return true; \
  } \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (new_size == _sequence->size) { \
      return true; \
    } \
    if (new_size > _sequence->size) { \
      if (!STRUCT_NAME ## __reserve(_sequence, new_size)) { \
        return false; \
      } \
      for (size_t _i = _sequence->size; _i < new_size; ++_i) { \
        ELEMENT_TYPE ## __InitOptions _element_options = {0}; \
        _element_options.allocator = &_sequence->_impl.allocator; \
        if (_sequence->_impl.prototype.storage != NULL) { \
          _element_options.external_storage = &_sequence->_impl.prototype.storage[_i]; \
        } \
        if (!ELEMENT_TYPE ## __init_with_options( \
            &_sequence->value[_i], _sequence->_impl.prototype.upper_bound, &_element_options)) \
        { \
          for (size_t _j = _i - 1; _j > _sequence->size; --_j) { \
            ELEMENT_TYPE ## __fini(&_sequence->value[_j]); \
          } \
          return false; \
        } \
      } \
    } else { \
      for (size_t _i = _sequence->size - 1; _i >= new_size; --_i) { \
        ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
      } \
    } \
    _sequence->size = new_size; \
    return true; \
  } \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    const ELEMENT_TYPE * value) \
  { \
    if (_sequence == NULL || value == NULL) { \
      return false; \
    } \
    if (!STRUCT_NAME ## __resize(_sequence, _sequence->size + 1)) { \
      return false; \
    } \
    return ELEMENT_TYPE ## __copy(value, &_sequence->value[_sequence->size - 1]); \
  } \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs) \
  { \
    if (lhs == NULL || rhs == NULL) { \
      return false; \
    } \
    if (lhs->value == NULL || rhs->value == NULL) { \
      return false; \
    } \
    if (lhs == rhs) { \
      return true; \
    } \
    if (lhs->size != rhs->size) { \
      return false; \
    } \
    for (size_t _i = 0U; _i < lhs->size; ++_i) { \
      if (!ELEMENT_TYPE ## __are_equal(&lhs->value[_i], &rhs->value[_i])) { \
        return false; \
      } \
    } \
    return true; \
  } \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output) \
  { \
    if (input == NULL || output == NULL) { \
      return false; \
    } \
    if (input->value == NULL || output->value == NULL) { \
      return false; \
    } \
    if (input == output) { \
      return true; \
    } \
    if (!STRUCT_NAME ## __resize(output, input->size)) { \
      return false; \
    } \
    for (size_t _i = 0U; _i < input->size; ++_i) { \
      if (!ELEMENT_TYPE ## __copy(&input->value[_i], &output->value[_i])) { \
        return false; \
      } \
    } \
    return true; \
  }

/// @brief Define functions for a bounded sequence of bounded elements.
/// Both sequence and elements have upper bounds.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_BOUNDED_SEQUENCE_DEFINE( \
    STRUCT_NAME, ELEMENT_TYPE) \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound, \
    size_t element_upper_bound, \
    const STRUCT_NAME ## __InitOptions * options) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (options != NULL && options->allocator != NULL) { \
      if (!rcutils_allocator_is_valid(options->allocator)) { \
        return false; \
      } \
      _sequence->_impl.allocator = *options->allocator; \
    } else { \
      _sequence->_impl.allocator = rcutils_get_default_allocator(); \
    } \
    _sequence->_impl.upper_bound = upper_bound; \
    if (options != NULL && options->external_storage != NULL) { \
      _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL; \
      _sequence->_impl.storage.region = *options->external_storage; \
      _sequence->value = (ELEMENT_TYPE *)_sequence->_impl.storage.region.location.address; \
      _sequence->capacity = _sequence->_impl.storage.region.size / sizeof(ELEMENT_TYPE); \
      if (_sequence->capacity > _sequence->_impl.upper_bound) { \
        _sequence->capacity = _sequence->_impl.upper_bound; \
      } \
      _sequence->size = 0U; \
    } else { \
      _sequence->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED; \
      _sequence->_impl.storage.data = NULL; \
      _sequence->value = NULL; \
      _sequence->capacity = 0U; \
      _sequence->size = 0U; \
    } \
    _sequence->_impl.prototype.storage = options != NULL ? options->external_element_storage : NULL; \
    _sequence->_impl.prototype.upper_bound = element_upper_bound; \
    return true; \
  } \
  bool STRUCT_NAME ## __init( \
    STRUCT_NAME * _sequence, \
    size_t upper_bound, \
    size_t element_upper_bound) \
  { \
    return STRUCT_NAME ## __init_with_options(_sequence, upper_bound, element_upper_bound, NULL); \
  } \
  void STRUCT_NAME ## __fini( \
    STRUCT_NAME * _sequence) \
  { \
    if (_sequence == NULL) { \
      return; \
    } \
    if (_sequence->value != NULL) { \
      for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
        ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
      } \
    } \
    if (_sequence->_impl.kind == ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED && \
      _sequence->_impl.storage.data != NULL) \
    { \
      _sequence->_impl.allocator.deallocate( \
        _sequence->_impl.storage.data, _sequence->_impl.allocator.state); \
        _sequence->_impl.storage.data = NULL; \
    } \
    _sequence->value = NULL; \
    _sequence->size = 0U; \
    _sequence->capacity = 0U; \
  } \
  bool STRUCT_NAME ## __reserve( \
    STRUCT_NAME * _sequence, \
    size_t requested_capacity) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (requested_capacity <= _sequence->capacity) { \
      return true; \
    } \
    if (requested_capacity > _sequence->_impl.upper_bound) { \
      return false; \
    } \
    if (_sequence->_impl.kind != ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__MANAGED) { \
      return false; \
    } \
    size_t new_capacity = rosidl_runtime_c__experimental__detail__next_capacity( \
      _sequence->capacity, requested_capacity); \
    if (new_capacity > _sequence->_impl.upper_bound) { \
      new_capacity = _sequence->_impl.upper_bound; \
    } \
    size_t bytes_to_allocate = new_capacity * sizeof(ELEMENT_TYPE); \
    ELEMENT_TYPE * new_data = (ELEMENT_TYPE *)_sequence->_impl.allocator.allocate( \
      bytes_to_allocate, _sequence->_impl.allocator.state); \
    if (new_data == NULL) { \
      return false; \
    } \
    for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
      ELEMENT_TYPE ## __InitOptions _element_options = {0}; \
      _element_options.allocator = &_sequence->_impl.allocator; \
      if (_sequence->_impl.prototype.storage != NULL) { \
        _element_options.external_storage = &_sequence->_impl.prototype.storage[_i]; \
      } \
      if (!ELEMENT_TYPE ## __init_with_options( \
          &new_data[_i], _sequence->_impl.prototype.upper_bound, &_element_options)) { \
        for (size_t _j = 0U; _j < _i; ++_j) { \
          ELEMENT_TYPE ## __fini(&new_data[_j]); \
        } \
        _sequence->_impl.allocator.deallocate( \
          new_data, _sequence->_impl.allocator.state); \
        return false; \
      } \
      if (!ELEMENT_TYPE ## __copy(&_sequence->value[_i], &new_data[_i])) { \
        for (size_t _j = 0U; _j <= _i; ++_j) { \
          ELEMENT_TYPE ## __fini(&new_data[_j]); \
        } \
        _sequence->_impl.allocator.deallocate( \
          new_data, _sequence->_impl.allocator.state); \
        return false; \
      } \
    } \
    for (size_t _i = 0U; _i < _sequence->size; ++_i) { \
      ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
    } \
    _sequence->_impl.storage.data = new_data; \
    _sequence->value = new_data; \
    _sequence->capacity = new_capacity; \
    return true; \
  } \
  bool STRUCT_NAME ## __resize( \
    STRUCT_NAME * _sequence, \
    size_t new_size) \
  { \
    if (_sequence == NULL) { \
      return false; \
    } \
    if (new_size == _sequence->size) { \
      return true; \
    } \
    if (new_size > _sequence->size) { \
      if (!STRUCT_NAME ## __reserve(_sequence, new_size)) { \
        return false; \
      } \
      for (size_t _i = _sequence->size; _i < new_size; ++_i) { \
        ELEMENT_TYPE ## __InitOptions _element_options = {0}; \
        _element_options.allocator = &_sequence->_impl.allocator; \
        if (_sequence->_impl.prototype.storage != NULL) { \
          _element_options.external_storage = &_sequence->_impl.prototype.storage[_i]; \
        } \
        if (!ELEMENT_TYPE ## __init_with_options( \
            &_sequence->value[_i], _sequence->_impl.prototype.upper_bound, &_element_options)) \
        { \
          for (size_t _j = _i - 1; _j > _sequence->size; --_j) { \
            ELEMENT_TYPE ## __fini(&_sequence->value[_j]); \
          } \
          return false; \
        } \
      } \
    } else { \
      for (size_t _i = _sequence->size - 1; _i >= new_size; --_i) { \
        ELEMENT_TYPE ## __fini(&_sequence->value[_i]); \
      } \
    } \
    _sequence->size = new_size; \
    return true; \
  } \
  bool STRUCT_NAME ## __push_back( \
    STRUCT_NAME * _sequence, \
    const ELEMENT_TYPE * value) \
  { \
    if (_sequence == NULL || value == NULL) { \
      return false; \
    } \
    if (!STRUCT_NAME ## __resize(_sequence, _sequence->size + 1)) { \
      return false; \
    } \
    return ELEMENT_TYPE ## __copy(value, &_sequence->value[_sequence->size - 1]); \
  } \
  bool STRUCT_NAME ## __are_equal( \
    const STRUCT_NAME * lhs, \
    const STRUCT_NAME * rhs) \
  { \
    if (lhs == NULL || rhs == NULL) { \
      return false; \
    } \
    if (lhs->value == NULL || rhs->value == NULL) { \
      return false; \
    } \
    if (lhs == rhs) { \
      return true; \
    } \
    if (lhs->size != rhs->size) { \
      return false; \
    } \
    for (size_t _i = 0U; _i < lhs->size; ++_i) { \
      if (!ELEMENT_TYPE ## __are_equal(&lhs->value[_i], &rhs->value[_i])) { \
        return false; \
      } \
    } \
    return true; \
  } \
  bool STRUCT_NAME ## __copy( \
    const STRUCT_NAME * input, \
    STRUCT_NAME * output) \
  { \
    if (input == NULL || output == NULL) { \
      return false; \
    } \
    if (input->value == NULL || output->value == NULL) { \
      return false; \
    } \
    if (output == input) { \
      return true; \
    } \
    if (!STRUCT_NAME ## __resize(output, input->size)) { \
      return false; \
    } \
    for (size_t _i = 0U; _i < input->size; ++_i) { \
      if (!ELEMENT_TYPE ## __copy(&input->value[_i], &output->value[_i])) { \
        return false; \
      } \
    } \
    return true; \
  }

// Sequence types for unbounded string/wstring.
ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__String__Sequence,
  rosidl_runtime_c__experimental__String);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__WString__Sequence,
  rosidl_runtime_c__experimental__WString);

// Bounded sequence types for unbounded string/wstring.
ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__String__BoundedSequence,
  rosidl_runtime_c__experimental__String);
ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__WString__BoundedSequence,
  rosidl_runtime_c__experimental__WString);

// Unbounded sequence types for bounded string/wstring.
ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__BoundedString__Sequence,
  rosidl_runtime_c__experimental__BoundedString);
ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__BoundedWString__Sequence,
  rosidl_runtime_c__experimental__BoundedWString);

// Bounded sequence types for bounded string/wstring.
ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__BoundedString__BoundedSequence,
  rosidl_runtime_c__experimental__BoundedString);
ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__BoundedWString__BoundedSequence,
  rosidl_runtime_c__experimental__BoundedWString);

// Sequence types for all primitive ROSIDL C types.
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Float__Sequence, float);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Double__Sequence, double);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__LongDouble__Sequence, long double);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Char__Sequence, char);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__WChar__Sequence, char16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Boolean__Sequence, bool);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__UInt8__Sequence, uint8_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Int8__Sequence, int8_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__UInt16__Sequence, uint16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Int16__Sequence, int16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__UInt32__Sequence, uint32_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Int32__Sequence, int32_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__UInt64__Sequence, uint64_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Int64__Sequence, int64_t);

// Bounded sequence types for all primitive ROSIDL C types.
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Float__BoundedSequence, float);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Double__BoundedSequence, double);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__LongDouble__BoundedSequence, long double);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Char__BoundedSequence, char);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__WChar__BoundedSequence, char16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Boolean__BoundedSequence, bool);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__UInt8__BoundedSequence, uint8_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Int8__BoundedSequence, int8_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__UInt16__BoundedSequence, uint16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Int16__BoundedSequence, int16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__UInt32__BoundedSequence, uint32_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Int32__BoundedSequence, int32_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__UInt64__BoundedSequence, uint64_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(
  rosidl_runtime_c__experimental__Int64__BoundedSequence, int64_t);

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_H_
