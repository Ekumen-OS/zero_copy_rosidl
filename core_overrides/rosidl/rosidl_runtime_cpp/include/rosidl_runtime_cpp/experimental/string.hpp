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

#ifndef ROSIDL_RUNTIME_CPP__EXPERIMENTAL__STRING_HPP_
#define ROSIDL_RUNTIME_CPP__EXPERIMENTAL__STRING_HPP_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "rosidl_runtime_cpp/experimental/memory.hpp"

namespace rosidl_runtime_cpp
{
namespace detail
{

template<typename CharT>
struct is_character_code_unit : std::false_type {};
template<>
struct is_character_code_unit<char>: std::true_type {};
template<>
struct is_character_code_unit<signed char>: std::true_type {};
template<>
struct is_character_code_unit<unsigned char>: std::true_type {};
template<>
struct is_character_code_unit<wchar_t>: std::true_type {};
template<>
struct is_character_code_unit<char16_t>: std::true_type {};
template<>
struct is_character_code_unit<char32_t>: std::true_type {};
#if defined(__cpp_char8_t)
template<>
struct is_character_code_unit<char8_t>: std::true_type {};
#endif

template<typename CharT>
inline constexpr bool is_character_code_unit_v = is_character_code_unit<CharT>::value;

template<typename CharT>
struct inline_string_capacity
{
  static constexpr std::size_t bytes = 64;
  static constexpr std::size_t slots = bytes / sizeof(CharT);
  static constexpr std::size_t value = slots > 1 ? slots - 1 : 1;
};

template<typename CharT>
inline constexpr bool inline_string_capacity_v = inline_string_capacity<CharT>::value;

}  // namespace detail

/// @file
/// @brief Experimental string container with fixed-region or PMR storage.

/// @brief Basic string-like container over ROSIDL memory regions.
///
/// The string is always null-terminated and supports standard character code units.
/// @tparam CharT Character code-unit type.
/// @tparam UpperBound Maximum length excluding null terminator (`0` means unbounded).
template<typename CharT, std::size_t UpperBound = 0>
class BasicString
{
  static_assert(
    detail::is_character_code_unit_v<CharT>,
    "BasicString CharT must be a standard character code unit type");

public:
  using value_type = CharT;
  using size_type = std::size_t;
  using reference = CharT &;
  using const_reference = const CharT &;
  using pointer = CharT *;
  using const_pointer = const CharT *;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Storage type for external memory initialization
  using ExternalStorage = MemoryRegion<CharT>;

  /// @brief Construct using default PMR storage.
  BasicString()
  : storage_pool_(std::pmr::get_default_resource()),
    storage_(InternalStorage{}),
    capacity_(InternalStorage::kCapacity),
    size_(0)
  {
    null_terminate();
  }

  /// @brief Construct using a specific PMR resource.
  explicit BasicString(std::pmr::memory_resource * storage_pool)
  : storage_pool_(storage_pool),
    storage_(InternalStorage{}),
    capacity_(InternalStorage::kCapacity),
    size_(0)
  {
    null_terminate();
  }

  /// @brief Construct over fixed external storage.
  explicit BasicString(MemoryRegion<CharT> storage_region)
  : storage_pool_(nullptr), storage_(storage_region), capacity_(0), size_(0)
  {
    if (storage_region) {
      const size_type total_slots = storage_region.capacity();
      if (total_slots == 0) {
        throw std::invalid_argument(
                "BasicString storage region has no room for null terminator");
      }
      capacity_ = total_slots - 1;
    }
    null_terminate();
  }

  BasicString(const CharT * value)  // NOLINT(runtime/explicit)
  : BasicString(std::pmr::get_default_resource())
  {
    assign(value);
  }

  BasicString(std::basic_string_view<CharT> value)  // NOLINT(runtime/explicit)
  : BasicString(std::pmr::get_default_resource())
  {
    assign(value);
  }

  explicit BasicString(
    std::basic_string_view<CharT> value,
    std::pmr::memory_resource * storage_pool)
  : BasicString(storage_pool)
  {
    assign(value);
  }

  BasicString(const BasicString & other)
  : BasicString(other.storage_pool_)
  {
    assign(other.view());
  }

  BasicString(BasicString && other) noexcept
  : storage_pool_(other.storage_pool_),
    storage_(std::move(other.storage_)),
    capacity_(other.capacity_),
    size_(other.size_)
  {
    other.storage_ = InternalStorage{};
    other.capacity_ = InternalStorage::kCapacity;
    other.size_ = 0;
    other.null_terminate();
  }

  ~BasicString()
  {
    release_owned_storage();
  }

  BasicString & operator=(const BasicString & other)
  {
    if (this == &other) {
      return *this;
    }
    assign(other.view());
    return *this;
  }

  BasicString & operator=(BasicString && other) noexcept
  {
    if (this == &other) {
      return *this;
    }
    if (!storage_pool_ || storage_pool_ != other.storage_pool_) {
      assign(other.view());
      return *this;
    }
    clear();
    release_owned_storage();
    storage_ = std::move(other.storage_);
    size_ = other.size_;
    capacity_ = other.capacity_;
    other.storage_ = InternalStorage{};
    other.size_ = 0;
    other.capacity_ = InternalStorage::kCapacity;
    other.null_terminate();
    return *this;
  }

  operator std::basic_string<CharT>() const {return std::basic_string<CharT>(data(), size_);}
  std::basic_string_view<CharT> view() const {return std::basic_string_view<CharT>(data(), size_);}
  const_pointer c_str() const noexcept {return data();}
  pointer data() noexcept {return mutable_data_pointer();}
  const_pointer data() const noexcept {return const_data_pointer();}
  iterator begin() noexcept {return data();}
  const_iterator begin() const noexcept {return data();}
  const_iterator cbegin() const noexcept {return begin();}
  iterator end() noexcept {return data() + size_;}
  const_iterator end() const noexcept {return data() + size_;}
  const_iterator cend() const noexcept {return end();}
  reverse_iterator rbegin() noexcept {return reverse_iterator(end());}
  const_reverse_iterator rbegin() const noexcept {return const_reverse_iterator(end());}
  const_reverse_iterator crbegin() const noexcept {return rbegin();}
  reverse_iterator rend() noexcept {return reverse_iterator(begin());}
  const_reverse_iterator rend() const noexcept {return const_reverse_iterator(begin());}
  const_reverse_iterator crend() const noexcept {return rend();}
  bool empty() const noexcept {return size_ == 0;}
  size_type size() const noexcept {return size_;}
  size_type length() const noexcept {return size_;}
  size_type capacity() const noexcept {return capacity_;}
  reference operator[](size_type position) noexcept {return data()[position];}
  const_reference operator[](size_type position) const noexcept {return data()[position];}

  reference at(size_type position)
  {
    if (position >= size_) {
      throw std::out_of_range("BasicString::at: index out of range");
    }
    return (*this)[position];
  }

  const_reference at(size_type position) const
  {
    if (position >= size_) {
      throw std::out_of_range("BasicString::at: index out of range");
    }
    return (*this)[position];
  }

  reference front() noexcept {return (*this)[0];}
  const_reference front() const noexcept {return (*this)[0];}
  reference back() noexcept {return (*this)[size_ - 1];}
  const_reference back() const noexcept {return (*this)[size_ - 1];}

  void clear() noexcept
  {
    size_ = 0;
    null_terminate();
  }

  void reserve(size_type new_capacity)
  {
    ensure_capacity_or_fail(new_capacity);
  }

  void resize(size_type new_size)
  {
    if (new_size < size_) {
      size_ = new_size;
      null_terminate();
      return;
    }
    ensure_capacity_or_fail(new_size);
    std::fill_n(data() + size_, new_size - size_, CharT{});
    size_ = new_size;
    null_terminate();
  }

  void resize(size_type new_size, CharT character)
  {
    if (new_size < size_) {
      size_ = new_size;
      null_terminate();
      return;
    }
    ensure_capacity_or_fail(new_size);
    std::fill_n(data() + size_, new_size - size_, character);
    size_ = new_size;
    null_terminate();
  }

  void push_back(CharT character)
  {
    ensure_capacity_or_fail(size_ + 1);
    data()[size_] = character;
    ++size_;
    null_terminate();
  }

  BasicString & append(std::basic_string_view<CharT> suffix)
  {
    if (!suffix.empty()) {
      const size_type old_size = size_;
      const size_type new_size = size_ + suffix.size();
      ensure_capacity_or_fail(new_size);
      std::memmove(data() + old_size, suffix.data(), suffix.size() * sizeof(CharT));
      size_ = new_size;
      null_terminate();
    }
    return *this;
  }

  BasicString & assign(std::basic_string_view<CharT> value)
  {
    clear();
    ensure_capacity_or_fail(value.size());
    if (!value.empty()) {
      std::memmove(data(), value.data(), value.size() * sizeof(CharT));
    }
    size_ = value.size();
    null_terminate();
    return *this;
  }

  void swap(BasicString & other) noexcept
  {
    using std::swap;
    swap(storage_pool_, other.storage_pool_);
    swap(storage_, other.storage_);
    swap(capacity_, other.capacity_);
    swap(size_, other.size_);
  }

  friend void swap(BasicString & lhs, BasicString & rhs) noexcept
  {
    lhs.swap(rhs);
  }

  friend bool operator==(const BasicString & lhs, const BasicString & rhs)
  {
    return lhs.view() == rhs.view();
  }

  friend bool operator!=(const BasicString & lhs, const BasicString & rhs)
  {
    return !(lhs == rhs);
  }

  friend bool operator==(const BasicString & lhs, std::basic_string_view<CharT> rhs)
  {
    return lhs.view() == rhs;
  }

  friend bool operator!=(const BasicString & lhs, std::basic_string_view<CharT> rhs)
  {
    return !(lhs == rhs);
  }

  friend bool operator==(std::basic_string_view<CharT> lhs, const BasicString & rhs)
  {
    return lhs == rhs.view();
  }

  friend bool operator!=(std::basic_string_view<CharT> lhs, const BasicString & rhs)
  {
    return !(lhs == rhs);
  }

  friend bool operator==(const BasicString & lhs, const CharT * rhs)
  {
    return lhs.view() == std::basic_string_view<CharT>(rhs);
  }

  friend bool operator!=(const BasicString & lhs, const CharT * rhs)
  {
    return !(lhs == rhs);
  }

  friend bool operator==(const CharT * lhs, const BasicString & rhs)
  {
    return std::basic_string_view<CharT>(lhs) == rhs.view();
  }

  friend bool operator!=(const CharT * lhs, const BasicString & rhs)
  {
    return !(lhs == rhs);
  }

private:
  struct InternalStorage
  {
    static constexpr size_t kCapacity =
      UpperBound > 0 ?
      ((UpperBound < detail::inline_string_capacity_v<CharT>) ?
      UpperBound : detail::inline_string_capacity_v<CharT>) :
      detail::inline_string_capacity_v<CharT>;
    CharT data[kCapacity + 1]{};
  };

  void null_terminate()
  {
    if (capacity_ == 0) {
      return;
    }
    data()[size_] = CharT{};
  }

  void ensure_capacity_or_fail(size_type requested_capacity)
  {
    if (UpperBound > 0 && requested_capacity > static_cast<size_type>(UpperBound)) {
      throw std::length_error("BasicString upper bound exceeded");
    }
    if (requested_capacity <= capacity_) {
      return;
    }
    if (!storage_pool_) {
      throw std::length_error("BasicString capacity exceeded for fixed storage");
    }
    size_type new_capacity = capacity_ == 0 ? requested_capacity : capacity_;
    while (new_capacity < requested_capacity) {
      new_capacity *= 2;
      if (new_capacity == 0) {
        new_capacity = requested_capacity;
        break;
      }
    }
    if (UpperBound > 0) {
      new_capacity = std::min(new_capacity, static_cast<size_type>(UpperBound));
    }
    reallocate_owned_storage(new_capacity);
  }

  void reallocate_owned_storage(size_type new_capacity)
  {
    pointer new_address = static_cast<pointer>(
      storage_pool_->allocate((new_capacity + 1) * sizeof(CharT), alignof(CharT)));
    if (size_ > 0) {
      std::memmove(new_address, data(), size_ * sizeof(CharT));
    }
    new_address[size_] = CharT{};

    release_owned_storage();

    storage_ = MemoryRegion<CharT>(new_address, (new_capacity + 1) * sizeof(CharT));
    capacity_ = new_capacity;
  }

  void release_owned_storage()
  {
    if (!storage_pool_ || !std::holds_alternative<MemoryRegion<CharT>>(storage_)) {
      return;
    }
    auto & region = std::get<MemoryRegion<CharT>>(storage_);
    if (!region) {
      return;
    }
    storage_pool_->deallocate(region.data(), region.size(), alignof(CharT));
    storage_ = InternalStorage{};
    capacity_ = InternalStorage::kCapacity;
  }

  pointer mutable_data_pointer() noexcept
  {
    if (std::holds_alternative<MemoryRegion<CharT>>(storage_)) {
      return std::get<MemoryRegion<CharT>>(storage_).data();
    }
    return std::get<InternalStorage>(storage_).data;
  }

  const_pointer const_data_pointer() const noexcept
  {
    if (std::holds_alternative<MemoryRegion<CharT>>(storage_)) {
      return std::get<MemoryRegion<CharT>>(storage_).data();
    }
    return std::get<InternalStorage>(storage_).data;
  }
  std::pmr::memory_resource * storage_pool_;
  std::variant<MemoryRegion<CharT>, InternalStorage> storage_;
  size_type capacity_;
  size_type size_;
};

using String = BasicString<char>;
using WString = BasicString<char16_t>;

template<std::size_t UpperBound>
using BoundedString = BasicString<char, UpperBound>;

template<std::size_t UpperBound>
using BoundedWString = BasicString<char16_t, UpperBound>;

}  // namespace rosidl_runtime_cpp

#endif  // ROSIDL_RUNTIME_CPP__EXPERIMENTAL__STRING_HPP_
