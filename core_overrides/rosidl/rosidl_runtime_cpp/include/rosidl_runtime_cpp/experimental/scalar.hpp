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

#ifndef ROSIDL_RUNTIME_CPP__EXPERIMENTAL__SCALAR_HPP_
#define ROSIDL_RUNTIME_CPP__EXPERIMENTAL__SCALAR_HPP_

#include <type_traits>
#include <utility>
#include <variant>

#include "rosidl_runtime_cpp/experimental/memory.hpp"

namespace rosidl_runtime_cpp
{

/// @file
/// @brief Experimental scalar wrapper with optional external backing memory.

/// @brief Scalar value that can live inline or in externally provided memory.
/// @tparam T Arithmetic scalar type.
template<typename T>
class Scalar
{
  static_assert(std::is_arithmetic<T>::value, "Scalar type must be arithmetic");

public:
  /// @brief Construct using inline storage.
  /// @param value Initial value.
  explicit Scalar(T value = T{}) noexcept
  : storage_(value)
  {
  }

  /// @brief Construct using external storage.
  /// @param memory External memory descriptor.
  explicit Scalar(Memory<T> memory) noexcept
  : storage_(memory)
  {
  }

  Scalar(const Scalar & other) noexcept
  : storage_(other.get())
  {
  }

  Scalar(Scalar && other) noexcept
  : storage_(std::move(other.storage_))
  {
    other.storage_ = T{};
  }

  Scalar & operator=(const Scalar & other) noexcept
  {
    get() = other.get();
    return *this;
  }

  Scalar & operator=(Scalar && other) noexcept
  {
    get() = std::move(other.get());
    return *this;
  }

  /// @brief Assign the scalar value.
  /// @param value New value.
  /// @return Reference to this scalar.
  Scalar & operator=(const T & value) noexcept
  {
    get() = value;
    return *this;
  }

  /// @brief Access the scalar value.
  /// @return Mutable value reference.
  T & get() noexcept
  {
    if (std::holds_alternative<Memory<T>>(storage_)) {
      return *std::get<Memory<T>>(storage_).data();
    }
    return std::get<T>(storage_);
  }

  /// @brief Access the scalar value.
  /// @return Immutable value reference.
  const T & get() const noexcept
  {
    if (std::holds_alternative<Memory<T>>(storage_)) {
      return *std::get<Memory<T>>(storage_).data();
    }
    return std::get<T>(storage_);
  }

  /// @brief Implicit conversion to value type.
  /// @return Scalar value copy.
  operator T() const noexcept
  {
    return get();
  }

  void swap(Scalar & other) noexcept
  {
    using std::swap;
    swap(storage_, other.storage_);
  }

  friend void swap(Scalar & lhs, Scalar & rhs) noexcept
  {
    lhs.swap(rhs);
  }

  friend bool operator==(const Scalar & lhs, const Scalar & rhs) noexcept
  {
    return lhs.get() == rhs.get();
  }

  friend bool operator!=(const Scalar & lhs, const Scalar & rhs) noexcept
  {
    return !(lhs == rhs);
  }

private:
  std::variant<Memory<T>, T> storage_;
};

}  // namespace rosidl_runtime_cpp

#endif  // ROSIDL_RUNTIME_CPP__EXPERIMENTAL__SCALAR_HPP_
