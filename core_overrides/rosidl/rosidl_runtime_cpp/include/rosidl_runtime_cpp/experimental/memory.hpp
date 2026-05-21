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

#ifndef ROSIDL_RUNTIME_CPP__EXPERIMENTAL__MEMORY_HPP_
#define ROSIDL_RUNTIME_CPP__EXPERIMENTAL__MEMORY_HPP_

#include <cstddef>

#include "rosidl_runtime_c/experimental/memory.h"

namespace rosidl_runtime_cpp
{

/// @file
/// @brief Experimental C++ wrappers for ROSIDL memory descriptors.

/// @brief Typed memory descriptor wrapper.
///
/// Wraps `rosidl_memory_t` from the C runtime API with typed pointer access.
template<typename T>
class Memory
{
public:
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;

  Memory() noexcept
  : memory_{nullptr, 0} {}

  Memory(rosidl_memory_t memory) noexcept  // NOLINT(runtime/explicit)
  : memory_(memory)
  {
  }

  explicit Memory(void *address, int attributes = 0) noexcept
  : memory_{address, attributes}
  {
  }

  void assign(void *address, int attributes = 0) noexcept
  {
    memory_.address = address;
    memory_.attributes = attributes;
  }

  pointer data() noexcept {return static_cast<pointer>(memory_.address);}
  const_pointer data() const noexcept {return static_cast<const_pointer>(memory_.address);}
  int attributes() const noexcept {return memory_.attributes;}

  const rosidl_memory_t & c_memory() const noexcept {return memory_;}

  explicit operator bool() const noexcept {return memory_.address != nullptr;}

  friend bool operator==(const Memory & lhs, const Memory & rhs) noexcept
  {
    return lhs.data() == rhs.data() && lhs.attributes() == rhs.attributes();
  }

  friend bool operator!=(const Memory & lhs, const Memory & rhs) noexcept
  {
    return !(lhs == rhs);
  }

private:
  rosidl_memory_t memory_;
};

/// @brief Typed memory region descriptor wrapper.
///
/// Wraps `rosidl_memory_region_t` from the C runtime API with typed pointer access.
template<typename T>
class MemoryRegion
{
public:
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using size_type = std::size_t;

  MemoryRegion() noexcept
  : region_{{nullptr, 0}, 0} {}

  MemoryRegion(rosidl_memory_region_t region) noexcept  // NOLINT(runtime/explicit)
  : region_(region)
  {
  }

  MemoryRegion(void *address, size_type size, int attributes = 0) noexcept
  : region_{{address, attributes}, size}
  {}

  void assign(void *address, size_type size, int attributes = 0) noexcept
  {
    region_.location.address = address;
    region_.location.attributes = attributes;
    region_.size = size;
  }

  pointer data() noexcept {return static_cast<pointer>(region_.location.address);}
  const_pointer data() const noexcept {return static_cast<const_pointer>(region_.location.address);}
  int attributes() const noexcept {return region_.location.attributes;}
  size_type size() const noexcept {return region_.size;}
  size_type capacity() const noexcept {return region_.size / sizeof(T);}

  const rosidl_memory_region_t & c_region() const noexcept {return region_;}

  explicit operator bool() const noexcept {return region_.location.address != nullptr;}

  friend bool operator==(const MemoryRegion & lhs, const MemoryRegion & rhs) noexcept
  {
    return lhs.data() == rhs.data() && lhs.attributes() == rhs.attributes() &&
           lhs.size() == rhs.size();
  }

  friend bool operator!=(const MemoryRegion & lhs, const MemoryRegion & rhs) noexcept
  {
    return !(lhs == rhs);
  }

private:
  rosidl_memory_region_t region_;
};

/// @brief Untyped memory descriptor wrapper specialization.
///
/// Like `Memory<T>` but without `value_type` or typed accessors.
/// Use `as<T>()` to obtain a typed pointer to the held address.
template<>
class Memory<void>
{
public:
  Memory() noexcept
  : memory_{nullptr, 0} {}

  Memory(rosidl_memory_t memory) noexcept  // NOLINT(runtime/explicit)
  : memory_(memory)
  {
  }

  explicit Memory(void * address, int attributes = 0) noexcept
  : memory_{address, attributes}
  {
  }

  void assign(void * address, int attributes = 0) noexcept
  {
    memory_.address = address;
    memory_.attributes = attributes;
  }

  void * data() noexcept {return memory_.address;}
  const void * data() const noexcept {return memory_.address;}
  int attributes() const noexcept {return memory_.attributes;}

  /// @brief Cast the held address to a pointer of type `T`.
  template<typename T>
  T * as() noexcept {return static_cast<T *>(memory_.address);}

  /// @brief Cast the held address to a pointer to const `T`.
  template<typename T>
  const T * as() const noexcept {return static_cast<const T *>(memory_.address);}

  const rosidl_memory_t & c_memory() const noexcept {return memory_;}

  explicit operator bool() const noexcept {return memory_.address != nullptr;}

  friend bool operator==(const Memory & lhs, const Memory & rhs) noexcept
  {
    return lhs.data() == rhs.data() && lhs.attributes() == rhs.attributes();
  }

  friend bool operator!=(const Memory & lhs, const Memory & rhs) noexcept
  {
    return !(lhs == rhs);
  }

private:
  rosidl_memory_t memory_;
};

/// @brief Untyped memory region descriptor wrapper specialization.
///
/// Like `MemoryRegion<T>` but without `value_type` or `capacity()`.
/// Use `as<T>()` to obtain a typed pointer to the held address.
template<>
class MemoryRegion<void>
{
public:
  using size_type = std::size_t;

  MemoryRegion() noexcept
  : region_{{nullptr, 0}, 0} {}

  MemoryRegion(rosidl_memory_region_t region) noexcept  // NOLINT(runtime/explicit)
  : region_(region)
  {
  }

  MemoryRegion(void * address, size_type size, int attributes = 0) noexcept
  : region_{{address, attributes}, size}
  {}

  void assign(void * address, size_type size, int attributes = 0) noexcept
  {
    region_.location.address = address;
    region_.location.attributes = attributes;
    region_.size = size;
  }

  void * data() noexcept {return region_.location.address;}
  const void * data() const noexcept {return region_.location.address;}
  int attributes() const noexcept {return region_.location.attributes;}
  size_type size() const noexcept {return region_.size;}

  /// @brief Cast the held address to a pointer of type `T`.
  template<typename T>
  T * as() noexcept {return static_cast<T *>(region_.location.address);}

  /// @brief Cast the held address to a pointer to const `T`.
  template<typename T>
  const T * as() const noexcept {return static_cast<const T *>(region_.location.address);}

  const rosidl_memory_region_t & c_region() const noexcept {return region_;}

  explicit operator bool() const noexcept {return region_.location.address != nullptr;}

  friend bool operator==(const MemoryRegion & lhs, const MemoryRegion & rhs) noexcept
  {
    return lhs.data() == rhs.data() && lhs.attributes() == rhs.attributes() &&
           lhs.size() == rhs.size();
  }

  friend bool operator!=(const MemoryRegion & lhs, const MemoryRegion & rhs) noexcept
  {
    return !(lhs == rhs);
  }

private:
  rosidl_memory_region_t region_;
};

}  // namespace rosidl_runtime_cpp

#endif  // ROSIDL_RUNTIME_CPP__EXPERIMENTAL__MEMORY_HPP_
