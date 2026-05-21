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

#ifndef ROSIDL_RUNTIME_C__EXPERIMENTAL__CONSTRAINTS_H_
#define ROSIDL_RUNTIME_C__EXPERIMENTAL__CONSTRAINTS_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/// @file
/// @brief Runtime constraint types for experimental C message fields.

/// @brief Constraints for a string (narrow or wide) member.
///
/// Carries the maximum character length for an unbounded string.
/// Using a struct rather than a bare `size_t` keeps the access syntax
/// consistent with other constraint types: `constraints.my_string.size`.
typedef struct rosidl_runtime_c__experimental__StringConstraint_s
{
  /// Maximum character length (0 = no limit imposed at this layer).
  size_t size;
} rosidl_runtime_c__experimental__StringConstraint;

static inline bool
rosidl_runtime_c__experimental__StringConstraint__are_equal(
  const rosidl_runtime_c__experimental__StringConstraint * lhs,
  const rosidl_runtime_c__experimental__StringConstraint * rhs)
{
  return lhs->size == rhs->size;
}

/// @brief Constraints for an unbounded sequence with no element constraints.
///
/// Used for sequence<basic_type> or sequence<bounded_string>.
/// Carries only the maximum sequence size; elements have no runtime constraints.
typedef struct rosidl_runtime_c__experimental__SequenceConstraint_s
{
  /// Maximum sequence size (0 = no limit imposed at this layer).
  size_t size;
} rosidl_runtime_c__experimental__SequenceConstraint;

static inline bool
rosidl_runtime_c__experimental__SequenceConstraint__are_equal(
  const rosidl_runtime_c__experimental__SequenceConstraint * lhs,
  const rosidl_runtime_c__experimental__SequenceConstraint * rhs)
{
  return lhs->size == rhs->size;
}

/// @brief Constraints for sequence<string> (unbounded strings).
///
/// Carries both the maximum sequence size and per-element string constraints.
typedef struct rosidl_runtime_c__experimental__StringSequenceConstraint_s
{
  /// Maximum sequence size (0 = no limit imposed at this layer).
  size_t size;
  /// Constraints for each string element in the sequence.
  rosidl_runtime_c__experimental__StringConstraint element;
} rosidl_runtime_c__experimental__StringSequenceConstraint;

static inline bool
rosidl_runtime_c__experimental__StringSequenceConstraint__are_equal(
  const rosidl_runtime_c__experimental__StringSequenceConstraint * lhs,
  const rosidl_runtime_c__experimental__StringSequenceConstraint * rhs)
{
  return lhs->size == rhs->size &&
         rosidl_runtime_c__experimental__StringConstraint__are_equal(&lhs->element, &rhs->element);
}

/// @brief Constraints for sequence<wstring> (unbounded wide strings).
///
/// Carries both the maximum sequence size and per-element string constraints.
typedef struct rosidl_runtime_c__experimental__WStringSequenceConstraint_s
{
  /// Maximum sequence size (0 = no limit imposed at this layer).
  size_t size;
  /// Constraints for each wstring element in the sequence.
  rosidl_runtime_c__experimental__StringConstraint element;
} rosidl_runtime_c__experimental__WStringSequenceConstraint;

static inline bool
rosidl_runtime_c__experimental__WStringSequenceConstraint__are_equal(
  const rosidl_runtime_c__experimental__WStringSequenceConstraint * lhs,
  const rosidl_runtime_c__experimental__WStringSequenceConstraint * rhs)
{
  return lhs->size == rhs->size &&
         rosidl_runtime_c__experimental__StringConstraint__are_equal(&lhs->element, &rhs->element);
}

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_RUNTIME_C__EXPERIMENTAL__CONSTRAINTS_H_
