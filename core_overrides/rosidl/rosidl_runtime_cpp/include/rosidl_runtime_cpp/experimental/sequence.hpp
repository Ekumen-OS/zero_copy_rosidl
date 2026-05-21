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

#ifndef ROSIDL_RUNTIME_CPP__EXPERIMENTAL__SEQUENCE_HPP_
#define ROSIDL_RUNTIME_CPP__EXPERIMENTAL__SEQUENCE_HPP_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory_resource>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "rosidl_runtime_cpp/experimental/memory.hpp"
#include "rosidl_runtime_cpp/experimental/traits.hpp"

// Forward declaration for partial specialization
namespace rosidl_runtime_cpp
{
template<typename CharT, std::size_t UpperBound>
class BasicString;
}  // namespace rosidl_runtime_cpp

namespace rosidl_runtime_cpp
{

/// @file
/// @brief Experimental vector-like sequence with fixed-region or PMR storage.

/// @brief Vector-like sequence with optional upper bound and element storage.
///
/// Storage can be either fixed (`MemoryRegion`) or dynamic (`std::pmr::memory_resource`).
/// When ElementStorage is not void, per-element storage can be provided for initialization.
/// @tparam T Element type.
/// @tparam UpperBound Maximum elements (`0` means unbounded).
/// @tparam ElementStorage Type for per-element storage
/// (auto-detected from T::ExternalStorage, or void).
template<typename T, std::size_t UpperBound = 0,
  typename ElementStorage = external_storage_type_t<T>>
class BasicSequence
{
public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  /// @brief Construct with default PMR resource.
  BasicSequence()
  : shared_storage_pool_(std::pmr::get_default_resource()), storage_(),
    element_storage_pool_(), capacity_(0), size_(0)
  {}

  /// @brief Construct with explicit PMR resource.
  explicit BasicSequence(std::pmr::memory_resource * shared_storage_pool)
  : shared_storage_pool_(shared_storage_pool ? shared_storage_pool :
      std::pmr::get_default_resource()),
    storage_(),
    element_storage_pool_(),
    capacity_(0),
    size_(0)
  {}

  /// @brief Construct with PMR resource and per-element storage.
  /// @param shared_storage_pool PMR resource for sequence storage.
  /// @param element_storage_pool Vector of storage for each element.
  explicit BasicSequence(
    std::pmr::memory_resource * shared_storage_pool,
    std::vector<ElementStorage> element_storage_pool)
  : shared_storage_pool_(shared_storage_pool ? shared_storage_pool :
      std::pmr::get_default_resource()),
    storage_(),
    element_storage_pool_(std::move(element_storage_pool)),
    capacity_(0),
    size_(0)
  {}

  /// @brief Construct with per-element storage using default PMR resource.
  /// @param element_storage_pool Vector of storage for each element.
  explicit BasicSequence(std::vector<ElementStorage> element_storage_pool)
  : shared_storage_pool_(std::pmr::get_default_resource()),
    storage_(),
    element_storage_pool_(std::move(element_storage_pool)),
    capacity_(0),
    size_(0)
  {}

  /// @brief Construct over fixed external storage with per-element storage.
  /// @param storage_region External memory region for element objects.
  /// @param element_storage_pool Vector of storage for each element.
  explicit BasicSequence(
    MemoryRegion<value_type> storage_region,
    std::vector<ElementStorage> element_storage_pool)
  : shared_storage_pool_(nullptr),
    storage_(storage_region),
    element_storage_pool_(std::move(element_storage_pool)),
    capacity_(std::min(storage_region.capacity(), element_storage_pool_.size())),
    size_(0)
  {}

  /// @brief Construct over fixed external storage.
  explicit BasicSequence(MemoryRegion<value_type> storage_region)
  : shared_storage_pool_(nullptr),
    element_storage_pool_(),
    storage_(storage_region),
    capacity_(storage_region.capacity()),
    size_(0)
  {}

  explicit BasicSequence(
    size_type count,
    std::pmr::memory_resource * storage_pool = std::pmr::get_default_resource())
  : BasicSequence(storage_pool)
  {
    resize(count);
  }

  explicit BasicSequence(
    size_type count,
    const value_type & value,
    std::pmr::memory_resource * storage_pool = std::pmr::get_default_resource())
  : BasicSequence(storage_pool)
  {
    assign(count, value);
  }

  template<typename InputIterator>
  explicit BasicSequence(
    InputIterator first,
    InputIterator last,
    std::pmr::memory_resource * storage_pool = std::pmr::get_default_resource())
  : BasicSequence(storage_pool)
  {
    assign(first, last);
  }

  BasicSequence(std::initializer_list<value_type> values) = delete;

  BasicSequence(const std::vector<value_type> & source_vector)  // NOLINT(runtime/explicit)
  : BasicSequence(std::pmr::get_default_resource())
  {
    assign(source_vector.begin(), source_vector.end());
  }

  BasicSequence(std::vector<value_type> && source_vector)  // NOLINT(runtime/explicit)
  : BasicSequence(std::pmr::get_default_resource())
  {
    ensure_capacity_or_fail(source_vector.size());
    for (size_type position = 0; position < source_vector.size(); ++position) {
      emplace_back(std::move(source_vector[position]));
    }
  }

  BasicSequence(const BasicSequence & other)
  : BasicSequence(
      other.shared_storage_pool_ ? other.shared_storage_pool_ :
      std::pmr::get_default_resource())
  {
    assign(other.begin(), other.end());
  }

  BasicSequence(BasicSequence && other) noexcept
  : shared_storage_pool_(other.shared_storage_pool_),
    element_storage_pool_(std::move(other.element_storage_pool_)),
    storage_(other.storage_),
    capacity_(other.capacity_),
    size_(other.size_)
  {
    other.storage_ = MemoryRegion<value_type>();
    other.element_storage_pool_.clear();
    other.capacity_ = 0;
    other.size_ = 0;
  }

  ~BasicSequence()
  {
    clear();
    release_owned_storage();
  }

  BasicSequence & operator=(std::initializer_list<value_type> list)
  {
    assign(list.begin(), list.end());
    return *this;
  }

  BasicSequence & operator=(const BasicSequence & other)
  {
    if (this == &other) {
      return *this;
    }
    assign(other.begin(), other.end());
    return *this;
  }

  BasicSequence & operator=(BasicSequence && other) noexcept
  {
    if (this == &other) {
      return *this;
    }
    if (!shared_storage_pool_ || shared_storage_pool_ != other.shared_storage_pool_) {
      assign(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
      return *this;
    }
    clear();
    release_owned_storage();
    storage_ = other.storage_;
    element_storage_pool_ = std::move(other.element_storage_pool_);
    capacity_ = other.capacity_;
    size_ = other.size_;
    other.storage_ = MemoryRegion<value_type>();
    other.element_storage_pool_.clear();
    other.capacity_ = 0;
    other.size_ = 0;
    return *this;
  }

  operator std::vector<value_type>() const
  {
    std::vector<value_type> vector;
    vector.reserve(size_);
    for (size_type position = 0; position < size_; ++position) {
      vector.push_back((*this)[position]);
    }
    return vector;
  }

  reference at(size_type position)
  {
    if (position >= size_) {
      throw std::out_of_range("Sequence::at: index out of range");
    }
    return (*this)[position];
  }

  const_reference at(size_type position) const
  {
    if (position >= size_) {
      throw std::out_of_range("Sequence::at: index out of range");
    }
    return (*this)[position];
  }

  reference operator[](size_type position) noexcept
  {
    return data()[position];
  }

  const_reference operator[](size_type position) const noexcept
  {
    return data()[position];
  }

  reference front() noexcept {return data()[0];}
  const_reference front() const noexcept {return data()[0];}
  reference back() noexcept {return data()[size_ - 1];}
  const_reference back() const noexcept {return data()[size_ - 1];}
  pointer data() noexcept {return storage_.data();}
  const_pointer data() const noexcept {return storage_.data();}
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
  size_type capacity() const noexcept {return capacity_;}

  size_type max_size() const noexcept
  {
    const size_type runtime_max_size = std::numeric_limits<size_type>::max() / sizeof(value_type);
    return runtime_max_size;
  }

  bool using_fixed_storage() const noexcept {return !shared_storage_pool_;}
  std::pmr::memory_resource * resource() const noexcept {return shared_storage_pool_;}

  void reserve(size_type new_capacity) {ensure_capacity_or_fail(new_capacity);}

  void shrink_to_fit()
  {
    if (!shared_storage_pool_ || size_ == capacity_) {
      return;
    }
    reallocate_owned_storage(size_);
  }

  void clear() noexcept
  {
    destroy_elements(0, size_);
    size_ = 0;
  }

  template<typename Convertible>
  std::enable_if_t<std::is_constructible_v<value_type, Convertible>>
  push_back(Convertible && value)
  {
    emplace_back(std::forward<Convertible>(value));
  }

  template<typename ... Arguments>
  reference emplace_back(Arguments && ... arguments)
  {
    ensure_capacity_or_fail(size_ + 1);
    pointer element_pointer = data() + size_;
    emplace_value(element_pointer, size_, std::forward<Arguments>(arguments)...);
    ++size_;
    return *element_pointer;
  }

  void pop_back()
  {
    if (size_ == 0) {
      return;
    }
    destroy_elements(size_ - 1, size_);
    --size_;
  }

  void resize(size_type new_size)
  {
    if (new_size < size_) {
      destroy_elements(new_size, size_);
      size_ = new_size;
      return;
    }
    ensure_capacity_or_fail(new_size);
    for (size_type position = size_; position < new_size; ++position) {
      emplace_back();
    }
  }

  void resize(size_type new_size, const value_type & value)
  {
    if (new_size < size_) {
      destroy_elements(new_size, size_);
      size_ = new_size;
      return;
    }
    ensure_capacity_or_fail(new_size);
    for (size_type position = size_; position < new_size; ++position) {
      emplace_back(value);
    }
  }

  void assign(size_type count, const value_type & value)
  {
    clear();
    ensure_capacity_or_fail(count);
    for (size_type position = 0; position < count; ++position) {
      emplace_back(value);
    }
  }

  template<typename InputIterator>
  std::void_t<typename std::iterator_traits<InputIterator>::iterator_category>
  assign(InputIterator first, InputIterator last)
  {
    clear();
    for (auto current = first; current != last; ++current) {
      emplace_back(*current);
    }
  }

  void swap(BasicSequence & other) noexcept
  {
    using std::swap;
    swap(shared_storage_pool_, other.shared_storage_pool_);
    swap(storage_, other.storage_);
    swap(element_storage_pool_, other.element_storage_pool_);
    swap(capacity_, other.capacity_);
    swap(size_, other.size_);
  }

  friend void swap(BasicSequence & lhs, BasicSequence & rhs) noexcept
  {
    lhs.swap(rhs);
  }

  friend bool operator==(const BasicSequence & lhs, const BasicSequence & rhs)
  {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }

  friend bool operator!=(const BasicSequence & lhs, const BasicSequence & rhs)
  {
    return !(lhs == rhs);
  }

private:
  void ensure_capacity_or_fail(size_type requested_capacity)
  {
    if (requested_capacity <= capacity_) {
      return;
    }
    // Check element storage pool limit first (applies to both fixed and PMR storage)
    if (!element_storage_pool_.empty() && requested_capacity > element_storage_pool_.size()) {
      throw std::length_error(
          "Sequence capacity exceeded: no more element storage regions available");
    }
    if (!shared_storage_pool_) {
      throw std::length_error("Sequence capacity exceeded for fixed storage");
    }
    size_type new_capacity = capacity_ == 0 ? requested_capacity : capacity_;
    while (new_capacity < requested_capacity) {
      new_capacity *= 2;
      if (new_capacity == 0) {
        new_capacity = requested_capacity;
        break;
      }
    }
    // Clamp to element_storage_pool size if provided
    if (!element_storage_pool_.empty()) {
      new_capacity = std::min(new_capacity, element_storage_pool_.size());
    }
    reallocate_owned_storage(new_capacity);
  }

  template<typename ... Arguments>
  void emplace_value(pointer element_pointer, size_type position, Arguments && ... arguments)
  {
    if (!element_storage_pool_.empty() && position < element_storage_pool_.size()) {
      ::new (static_cast<void *>(element_pointer)) value_type(element_storage_pool_[position]);
      if constexpr (sizeof...(Arguments) > 0) {
        *element_pointer = value_type(std::forward<Arguments>(arguments)...);
      }
      return;
    }
    if constexpr (std::is_constructible_v<value_type, Arguments..., std::pmr::memory_resource *>) {
      if (shared_storage_pool_) {
        ::new (static_cast<void *>(element_pointer)) value_type(
          std::forward<Arguments>(arguments)..., shared_storage_pool_);
        return;
      }
    }
    ::new (static_cast<void *>(element_pointer)) value_type(std::forward<Arguments>(arguments)...);
  }

  void reallocate_owned_storage(size_type new_capacity)
  {
    pointer new_address = static_cast<pointer>(
      shared_storage_pool_->allocate(new_capacity * sizeof(value_type), alignof(value_type)));
    for (size_type position = 0; position < size_; ++position) {
      ::new (static_cast<void *>(new_address + position)) value_type(std::move(data()[position]));
    }
    destroy_elements(0, size_);

    release_owned_storage();
    storage_ = MemoryRegion<value_type>(new_address, new_capacity * sizeof(value_type));
    capacity_ = new_capacity;
  }

  void release_owned_storage()
  {
    if (!shared_storage_pool_ || !storage_) {
      return;
    }
    shared_storage_pool_->deallocate(data(), storage_.size(), alignof(value_type));
    storage_ = MemoryRegion<value_type>();
    capacity_ = 0;
  }

  void destroy_elements(size_type first, size_type last) noexcept
  {
    for (size_type position = first; position < last; ++position) {
      data()[position].~value_type();
    }
  }

  std::pmr::memory_resource * shared_storage_pool_;
  std::vector<ElementStorage> element_storage_pool_;
  MemoryRegion<value_type> storage_;
  size_type capacity_;
  size_type size_;
};

/// @brief Specialization for types without per-element storage support.
/// @tparam T Element type.
/// @tparam UpperBound Maximum elements (`0` means unbounded).
template<typename T, std::size_t UpperBound>
class BasicSequence<T, UpperBound, void>
{
public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  /// @brief Construct with default PMR resource.
  BasicSequence()
  : shared_storage_pool_(std::pmr::get_default_resource()), storage_(),
    capacity_(0), size_(0)
  {}

  /// @brief Construct with explicit PMR resource.
  /// @param shared_storage_pool Resource pointer (defaults to global PMR resource when null).
  explicit BasicSequence(std::pmr::memory_resource * shared_storage_pool)
  : shared_storage_pool_(shared_storage_pool ? shared_storage_pool :
      std::pmr::get_default_resource()),
    storage_(),
    capacity_(0),
    size_(0)
  {}

  /// @brief Construct over fixed external storage.
  /// @param storage_region External memory region.
  explicit BasicSequence(MemoryRegion<T> storage_region)
  : shared_storage_pool_(nullptr),
    storage_(storage_region),
    capacity_(clamp_to_upper_bound(storage_region.capacity())),
    size_(0)
  {}

  explicit BasicSequence(
    size_type count,
    std::pmr::memory_resource * storage_pool = std::pmr::get_default_resource())
  : BasicSequence(storage_pool)
  {
    resize(count);
  }

  BasicSequence(
    size_type count,
    const value_type & value,
    std::pmr::memory_resource * storage_pool = std::pmr::get_default_resource())
  : BasicSequence(storage_pool)
  {
    assign(count, value);
  }
  template<typename InputIterator>
  BasicSequence(
    InputIterator first,
    InputIterator last,
    std::pmr::memory_resource * storage_pool = std::pmr::get_default_resource())
  : BasicSequence(storage_pool)
  {
    assign(first, last);
  }

  BasicSequence(std::initializer_list<value_type> values) = delete;

  BasicSequence(const std::vector<T> & source_vector)  // NOLINT(runtime/explicit)
  : BasicSequence(std::pmr::get_default_resource())
  {
    assign(source_vector.begin(), source_vector.end());
  }

  BasicSequence(std::vector<T> && source_vector)  // NOLINT(runtime/explicit)
  : BasicSequence(std::pmr::get_default_resource())
  {
    ensure_capacity_or_fail(source_vector.size());
    if (std::is_trivially_copyable<T>::value && !source_vector.empty()) {
      std::memmove(data(), source_vector.data(), source_vector.size() * sizeof(T));
      size_ = source_vector.size();
    } else {
      for (size_type position = 0; position < source_vector.size(); ++position) {
        emplace_back(std::move(source_vector[position]));
      }
    }
  }

  BasicSequence(const BasicSequence & other)
  : BasicSequence(
      other.shared_storage_pool_ ? other.shared_storage_pool_ :
      std::pmr::get_default_resource())
  {
    assign(other.begin(), other.end());
  }

  BasicSequence(BasicSequence && other) noexcept
  : shared_storage_pool_(other.shared_storage_pool_),
    storage_(other.storage_),
    capacity_(other.capacity_),
    size_(other.size_)
  {
    other.shared_storage_pool_ = std::pmr::get_default_resource();
    other.storage_ = MemoryRegion<T>();
    other.capacity_ = 0;
    other.size_ = 0;
  }

  ~BasicSequence()
  {
    clear();
    release_owned_storage();
  }

  BasicSequence & operator=(std::initializer_list<value_type> list)
  {
    assign(list.begin(), list.end());
    return *this;
  }

  BasicSequence & operator=(const BasicSequence & other)
  {
    if (this == &other) {
      return *this;
    }
    assign(other.begin(), other.end());
    return *this;
  }

  BasicSequence & operator=(BasicSequence && other) noexcept
  {
    if (this == &other) {
      return *this;
    }
    clear();
    release_owned_storage();
    shared_storage_pool_ = other.shared_storage_pool_;
    storage_ = other.storage_;
    capacity_ = other.capacity_;
    size_ = other.size_;
    other.shared_storage_pool_ = std::pmr::get_default_resource();
    other.storage_ = MemoryRegion<T>();
    other.capacity_ = 0;
    other.size_ = 0;
    return *this;
  }

  operator std::vector<T>() const
  {
    std::vector<T> vector;
    vector.reserve(size_);
    for (size_type position = 0; position < size_; ++position) {
      vector.push_back((*this)[position]);
    }
    return vector;
  }
  reference at(size_type position)
  {
    if (position >= size_) {
      throw std::out_of_range("Sequence::at: index out of range");
    }
    return (*this)[position];
  }

  const_reference at(size_type position) const
  {
    if (position >= size_) {
      throw std::out_of_range("Sequence::at: index out of range");
    }
    return (*this)[position];
  }
  reference operator[](size_type position) noexcept
  {
    return data()[position];
  }

  const_reference operator[](size_type position) const noexcept
  {
    return data()[position];
  }
  reference front() noexcept {return data()[0];}
  const_reference front() const noexcept {return data()[0];}
  reference back() noexcept {return data()[size_ - 1];}
  const_reference back() const noexcept {return data()[size_ - 1];}
  pointer data() noexcept {return storage_.data();}
  const_pointer data() const noexcept {return storage_.data();}
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
  size_type capacity() const noexcept {return capacity_;}

  size_type max_size() const noexcept
  {
    const size_type runtime_max_size = std::numeric_limits<size_type>::max() / sizeof(T);
    return UpperBound > 0 ? std::min(runtime_max_size,
        static_cast<size_type>(UpperBound)) : runtime_max_size;
  }

  /// @brief Whether this sequence uses a fixed external region.
  bool using_fixed_storage() const noexcept {return !shared_storage_pool_;}

  /// @brief PMR resource for dynamic storage.
  std::pmr::memory_resource * resource() const noexcept {return shared_storage_pool_;}

  void reserve(size_type new_capacity) {ensure_capacity_or_fail(new_capacity);}

  void shrink_to_fit()
  {
    if (!shared_storage_pool_ || size_ == capacity_) {
      return;
    }
    reallocate_owned_storage(size_);
  }

  void clear() noexcept
  {
    destroy_elements(0, size_);
    size_ = 0;
  }
  void push_back(const value_type & value) {emplace_back(value);}
  void push_back(value_type && value) {emplace_back(std::move(value));}
  template<typename ... Arguments>
  reference emplace_back(Arguments && ... arguments)
  {
    ensure_capacity_or_fail(size_ + 1);
    pointer element_pointer = data() + size_;
    emplace_value(element_pointer, std::forward<Arguments>(arguments)...);
    ++size_;
    return *element_pointer;
  }

  void pop_back()
  {
    if (size_ == 0) {
      return;
    }
    destroy_elements(size_ - 1, size_);
    --size_;
  }

  void resize(size_type new_size)
  {
    if (new_size < size_) {
      destroy_elements(new_size, size_);
      size_ = new_size;
      return;
    }
    ensure_capacity_or_fail(new_size);
    if (std::is_trivially_constructible<T>::value) {
      std::fill_n(data() + size_, new_size - size_, T{});
    } else {
      for (size_type position = size_; position < new_size; ++position) {
        ::new (static_cast<void *>(data() + position)) T();
      }
    }
    size_ = new_size;
  }

  void resize(size_type new_size, const value_type & value)
  {
    if (new_size < size_) {
      destroy_elements(new_size, size_);
      size_ = new_size;
      return;
    }
    ensure_capacity_or_fail(new_size);
    if (std::is_trivially_copyable<T>::value) {
      std::fill_n(data() + size_, new_size - size_, value);
    } else {
      for (size_type position = size_; position < new_size; ++position) {
        ::new (static_cast<void *>(data() + position)) T(value);
      }
    }
    size_ = new_size;
  }

  void assign(size_type count, const value_type & value)
  {
    clear();
    ensure_capacity_or_fail(count);
    if (std::is_trivially_copyable<T>::value) {
      std::fill_n(data(), count, value);
      size_ = count;
      return;
    }
    for (size_type position = 0; position < count; ++position) {
      emplace_back(value);
    }
  }

  template<typename InputIterator>
  std::void_t<typename std::iterator_traits<InputIterator>::iterator_category>
  assign(InputIterator first, InputIterator last)
  {
    clear();
    for (auto current = first; current != last; ++current) {
      emplace_back(*current);
    }
  }

  void swap(BasicSequence & other) noexcept
  {
    using std::swap;
    swap(shared_storage_pool_, other.shared_storage_pool_);
    swap(storage_, other.storage_);
    swap(capacity_, other.capacity_);
    swap(size_, other.size_);
  }

  friend void swap(BasicSequence & lhs, BasicSequence & rhs) noexcept
  {
    lhs.swap(rhs);
  }

  friend bool operator==(const BasicSequence & lhs, const BasicSequence & rhs)
  {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }

  friend bool operator!=(const BasicSequence & lhs, const BasicSequence & rhs)
  {
    return !(lhs == rhs);
  }

private:
  static size_type clamp_to_upper_bound(size_type capacity)
  {
    return UpperBound > 0 ? std::min(capacity, static_cast<size_type>(UpperBound)) : capacity;
  }

  void ensure_capacity_or_fail(size_type requested_capacity)
  {
    if (UpperBound > 0 && requested_capacity > static_cast<size_type>(UpperBound)) {
      throw std::length_error("Sequence upper bound exceeded");
    }
    if (requested_capacity <= capacity_) {
      return;
    }
    if (!shared_storage_pool_) {
      throw std::length_error("Sequence capacity exceeded for fixed storage");
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
  template<typename ... Arguments>
  void emplace_value(pointer element_pointer, Arguments && ... arguments)
  {
    if (std::is_trivially_constructible<T>::value) {
      *element_pointer = T(std::forward<Arguments>(arguments)...);
    } else {
      ::new (static_cast<void *>(element_pointer)) T(std::forward<Arguments>(arguments)...);
    }
  }

  void reallocate_owned_storage(size_type new_capacity)
  {
    pointer new_address = static_cast<pointer>(
      shared_storage_pool_->allocate(new_capacity * sizeof(T), alignof(T)));
    if (std::is_trivially_copyable<T>::value) {
      if (size_ > 0 && data() != nullptr) {
        std::memmove(new_address, data(), size_ * sizeof(T));
      }
    } else {
      for (size_type position = 0; position < size_; ++position) {
        ::new (static_cast<void *>(new_address + position)) T(std::move(data()[position]));
      }
      destroy_elements(0, size_);
    }

    release_owned_storage();
    storage_ = MemoryRegion<T>(new_address, new_capacity * sizeof(T));
    capacity_ = new_capacity;
  }

  void release_owned_storage()
  {
    if (!shared_storage_pool_ || !storage_) {
      return;
    }
    shared_storage_pool_->deallocate(data(), storage_.size(), alignof(T));
    storage_ = MemoryRegion<T>();
    capacity_ = 0;
  }

  void destroy_elements(size_type first, size_type last) noexcept
  {
    if (std::is_trivially_destructible<T>::value) {
      return;
    }
    for (size_type position = first; position < last; ++position) {
      data()[position].~T();
    }
  }

  std::pmr::memory_resource * shared_storage_pool_;
  MemoryRegion<T> storage_;
  size_type capacity_;
  size_type size_;
};

template<typename T>
using Sequence = BasicSequence<T>;

template<typename T, std::size_t UpperBound>
using BoundedSequence = BasicSequence<T, UpperBound>;

}  // namespace rosidl_runtime_cpp

#endif  // ROSIDL_RUNTIME_CPP__EXPERIMENTAL__SEQUENCE_HPP_
