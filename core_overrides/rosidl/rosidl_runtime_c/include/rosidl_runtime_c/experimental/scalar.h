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

#ifndef ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_H_
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_H_

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include "rosidl_runtime_c/experimental/initialization.h"
#include "rosidl_runtime_c/experimental/memory.h"
#include "rosidl_runtime_c/experimental/storage.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// @file
/// @brief Experimental C11 scalar wrapper macros.

/// @brief Initialization options for experimental scalars.
///
/// Provides control over external memory for scalar initialization.
typedef struct rosidl_scalar_init_options_s
{
  /// Optional external memory (NULL for local storage).
  const rosidl_memory_t * external_memory;

  /// Reserved for future expansion (must be NULL).
  void * reserved[4];
} rosidl_scalar_init_options_t;

/// @brief Declare a typed scalar wrapper and its function signatures.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(STRUCT_NAME, VALUE_TYPE) \
  typedef rosidl_scalar_init_options_t STRUCT_NAME ## __InitOptions; \
  typedef struct STRUCT_NAME ## _s \
  { \
    struct { \
      VALUE_TYPE data; \
    } * value; \
    struct \
    { \
      rosidl_runtime_c__experimental__storage_kind_t kind; \
      union \
      { \
        rosidl_memory_t memory; \
        struct \
        { \
          VALUE_TYPE data; \
        } local; \
      } storage; \
    } _impl; \
  } STRUCT_NAME; \
  bool STRUCT_NAME ## __init(STRUCT_NAME * _scalar); \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _scalar, \
    const STRUCT_NAME ## __InitOptions * options); \
  void STRUCT_NAME ## __fini(STRUCT_NAME * _scalar);

/// @brief Define scalar functions declared with ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DEFINE(STRUCT_NAME, VALUE_TYPE) \
  bool STRUCT_NAME ## __init_with_options( \
    STRUCT_NAME * _scalar, \
    const STRUCT_NAME ## __InitOptions * options) \
  { \
    if (_scalar == NULL) { \
      return false; \
    } \
    if (options != NULL && rosidl_memory_is_valid(options->external_memory)) { \
      _scalar->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__EXTERNAL; \
      _scalar->_impl.storage.memory = *options->external_memory; \
      _scalar->value = (void *)_scalar->_impl.storage.memory.address; \
    } else { \
      _scalar->_impl.kind = ROSIDL_RUNTIME_C__EXPERIMENTAL__STORAGE_KIND__LOCAL; \
      _scalar->_impl.storage.local.data = (VALUE_TYPE)0; \
      _scalar->value = (void *)&_scalar->_impl.storage.local; \
    } \
    return true; \
  } \
  bool STRUCT_NAME ## __init(STRUCT_NAME * _scalar) \
  { \
    return STRUCT_NAME ## __init_with_options(_scalar, NULL); \
  } \
  void STRUCT_NAME ## __fini(STRUCT_NAME * _scalar) \
  { \
    if (_scalar == NULL) { \
      return; \
    } \
    _scalar->value = NULL; \
  }

/// @brief Convenience macro declaring and defining a scalar in one place.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR(STRUCT_NAME, VALUE_TYPE) \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(STRUCT_NAME, VALUE_TYPE); \
  ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DEFINE(STRUCT_NAME, VALUE_TYPE)

/// @brief Create a type alias for a scalar type with function forwarding.
/// Generates a typedef and static inline forwarding functions for all scalar operations.
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_ALIAS(ALIAS_NAME, BASE_TYPE) \
  typedef BASE_TYPE ALIAS_NAME; \
  typedef BASE_TYPE ## __InitOptions ALIAS_NAME ## __InitOptions; \
  static inline bool ALIAS_NAME ## __init(ALIAS_NAME * _scalar) \
  { \
    return BASE_TYPE ## __init(_scalar); \
  } \
  static inline bool ALIAS_NAME ## __init_with_options( \
    ALIAS_NAME * _scalar, \
    const ALIAS_NAME ## __InitOptions * options) \
  { \
    return BASE_TYPE ## __init_with_options(_scalar, options); \
  } \
  static inline void ALIAS_NAME ## __fini(ALIAS_NAME * _scalar) \
  { \
    BASE_TYPE ## __fini(_scalar); \
  }

// Scalar types for all primitive ROSIDL C types.
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__Float, float);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__Double, double);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__LongDouble,
  long double);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__Char, char);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__WChar, char16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__Boolean, bool);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__UInt8, uint8_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__Int8, int8_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__UInt16, uint16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__Int16, int16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__UInt32, uint32_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__Int32, int32_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__UInt64, uint64_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_DECLARE(rosidl_runtime_c__experimental__Int64, int64_t);

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_H_
