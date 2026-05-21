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

#ifndef ROSIDL_RUNTIME_CPP__EXPERIMENTAL__CONSTRAINTS_HPP_
#define ROSIDL_RUNTIME_CPP__EXPERIMENTAL__CONSTRAINTS_HPP_

#include <cstddef>

#include "rosidl_runtime_cpp/experimental/string.hpp"

namespace rosidl_runtime_cpp
{

/// @brief Constraints for a string member.
///
/// Carries the maximum character length for an unbounded string (narrow or wide).
/// Using a struct rather than a bare `std::size_t` keeps the access syntax
/// consistent with other constraint types: `constraints.my_string.size`.
struct StringConstraint
{
  /// Maximum character length of the string.
  std::size_t size{0};

  bool operator==(const StringConstraint & other) const
  {
    return size == other.size;
  }

  bool operator!=(const StringConstraint & other) const
  {
    return !(*this == other);
  }
};

/// @brief Constraints for a sequence of elements of type T.
///
/// The primary template covers scalar, bounded-string, and bounded-sequence
/// element types, where only the maximum sequence length is relevant.
/// Full specializations for `String` and `WString` add a per-element
/// character length limit. Specializations for message types are generated
/// alongside each message and add a per-element `Constraints` sub-struct.
///
/// @tparam T Element type of the sequence.
template<typename T>
struct SequenceConstraint
{
  /// Maximum number of elements in the sequence.
  std::size_t size{0};

  bool operator==(const SequenceConstraint & other) const
  {
    return size == other.size;
  }

  bool operator!=(const SequenceConstraint & other) const
  {
    return !(*this == other);
  }
};

/// @brief Constraints for a sequence of (unbounded) narrow strings.
///
/// Adds a per-element character length limit on top of the sequence size limit.
/// Only applies to `String` (unbounded); `BoundedString<N>` sequences use the
/// primary template since the element bound is already part of the type.
template<>
struct SequenceConstraint<String>
{
  /// Maximum number of elements in the sequence.
  std::size_t size{0};
  /// Constraints applied to each string element.
  StringConstraint element{};

  bool operator==(const SequenceConstraint & other) const
  {
    return size == other.size && element == other.element;
  }

  bool operator!=(const SequenceConstraint & other) const
  {
    return !(*this == other);
  }
};

/// @brief Constraints for a sequence of (unbounded) wide strings.
///
/// Adds a per-element character length limit on top of the sequence size limit.
/// Only applies to `WString` (unbounded); `BoundedWString<N>` sequences use the
/// primary template since the element bound is already part of the type.
template<>
struct SequenceConstraint<WString>
{
  /// Maximum number of elements in the sequence.
  std::size_t size{0};
  /// Constraints applied to each wide string element.
  StringConstraint element{};

  bool operator==(const SequenceConstraint & other) const
  {
    return size == other.size && element == other.element;
  }

  bool operator!=(const SequenceConstraint & other) const
  {
    return !(*this == other);
  }
};

}  // namespace rosidl_runtime_cpp

#endif  // ROSIDL_RUNTIME_CPP__EXPERIMENTAL__CONSTRAINTS_HPP_
