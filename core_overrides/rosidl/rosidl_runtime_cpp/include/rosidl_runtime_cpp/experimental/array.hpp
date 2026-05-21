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

#ifndef ROSIDL_RUNTIME_CPP__EXPERIMENTAL__ARRAY_HPP_
#define ROSIDL_RUNTIME_CPP__EXPERIMENTAL__ARRAY_HPP_

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "rosidl_runtime_cpp/experimental/memory.hpp"

namespace rosidl_runtime_cpp
{

/// @file
/// @brief Experimental fixed-size array with optional strided external storage.

/// @brief `std::array`-like container over inline or external memory.
/// @tparam T Element type.
/// @tparam N Number of elements.
template<typename T, std::size_t N>
class Array
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

private:
  struct InternalStorage
  {
    alignas(T) std::byte raw[sizeof(T) * N];
    T * data() noexcept {return reinterpret_cast<T *>(raw);}
    const T * data() const noexcept {return reinterpret_cast<const T *>(raw);}

    InternalStorage()
    {
      for (std::size_t i = 0; i < N; ++i) {
        ::new (data() + i) T();
      }
    }

    InternalStorage(const InternalStorage & other)
    {
      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(raw, other.raw, sizeof(T) * N);
      } else {
        for (std::size_t i = 0; i < N; ++i) {
          ::new (data() + i) T(other.data()[i]);
        }
      }
    }

    InternalStorage(InternalStorage && other) noexcept
    {
      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(raw, other.raw, sizeof(T) * N);
      } else {
        for (std::size_t i = 0; i < N; ++i) {
          ::new (data() + i) T(std::move(other.data()[i]));
        }
      }
    }

    InternalStorage & operator=(const InternalStorage & other)
    {
      if (this != &other) {
        if constexpr (std::is_trivially_copyable_v<T>) {
          std::memcpy(raw, other.raw, sizeof(T) * N);
        } else {
          for (std::size_t i = 0; i < N; ++i) {
            data()[i] = other.data()[i];
          }
        }
      }
      return *this;
    }

    InternalStorage & operator=(InternalStorage && other) noexcept
    {
      if (this != &other) {
        if constexpr (std::is_trivially_copyable_v<T>) {
          std::memcpy(raw, other.raw, sizeof(T) * N);
        } else {
          for (std::size_t i = 0; i < N; ++i) {
            data()[i] = std::move(other.data()[i]);
          }
        }
      }
      return *this;
    }

    ~InternalStorage()
    {
      if constexpr (!std::is_trivially_destructible_v<T>) {
        for (std::size_t i = 0; i < N; ++i) {
          data()[i].~T();
        }
      }
    }

    /// @brief Tag for uninitialized storage (used by the piecewise constructor).
    struct uninit_t {};
    explicit InternalStorage(uninit_t) noexcept {}
  };

public:
  /// @brief Construct with internal storage.
  Array()
  : storage_(InternalStorage{})
  {}

  /// @brief Copy constructor.
  /// @param other Source array.
  Array(const Array & other)
  : storage_(std::is_trivially_copyable_v<T> ?
      InternalStorage{typename InternalStorage::uninit_t{}} :
      InternalStorage{})
  {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(data(), other.data(), sizeof(T) * N);
    } else {
      for (size_type i = 0; i < N; ++i) {
        (*this)[i] = other[i];
      }
    }
  }

  /// @brief Move constructor.
  /// @param other Source array.
  Array(Array && other) noexcept
  : storage_(std::move(other.storage_))
  {
    other.storage_ = InternalStorage{};
  }

  /// @brief Construct from `std::array`.
  /// @param array Source array.
  Array(const std::array<T, N> & array)  // NOLINT(runtime/explicit)
  : storage_(std::is_trivially_copyable_v<T> ?
      InternalStorage{typename InternalStorage::uninit_t{}} :
      InternalStorage{})
  {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(data(), array.data(), sizeof(T) * N);
    } else {
      for (size_type i = 0; i < N; ++i) {
        (*this)[i] = array[i];
      }
    }
  }

  /// @brief Construct from moved `std::array`.
  /// @param array Source array.
  Array(std::array<T, N> && array)  // NOLINT(runtime/explicit)
  : storage_(std::is_trivially_copyable_v<T> ?
      InternalStorage{typename InternalStorage::uninit_t{}} :
      InternalStorage{})
  {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(data(), array.data(), sizeof(T) * N);
    } else {
      for (size_type i = 0; i < N; ++i) {
        (*this)[i] = std::move(array[i]);
      }
    }
  }

  /// @brief Construct over an external memory region.
  /// @param region External memory region.
  explicit Array(MemoryRegion<T> region)
  : storage_(region)
  {}

  /// @brief Piecewise in-place constructor: constructs each element from its own
  ///        arg tuple, in index order, with no intermediate default construction.
  ///
  /// Exactly N tuples must be provided, one per element.
  /// @code
  ///   Array<Msg, 3>(std::piecewise_construct,
  ///     std::make_tuple(arg0), std::make_tuple(arg1), std::make_tuple(arg2));
  /// @endcode
  template<
    typename ... ArgTuples,
    std::enable_if_t<sizeof...(ArgTuples) == N, int> = 0>
  explicit Array(std::piecewise_construct_t, ArgTuples && ... arg_tuples)
  : storage_(InternalStorage{typename InternalStorage::uninit_t{}})
  {
    T * ptr = std::get<InternalStorage>(storage_).data();
    std::size_t i = 0;
    (..., std::apply(
      [&](auto && ... args) {
        ::new (static_cast<void *>(ptr + i)) T(std::forward<decltype(args)>(args)...);
        ++i;
        },
        std::forward<ArgTuples>(arg_tuples)));
  }

  Array & operator=(const Array & array)
  {
    if constexpr (std::is_trivially_copyable_v<T>) {
      if (this != &array) {
        std::memcpy(data(), array.data(), sizeof(T) * N);
      }
    } else {
      for (size_type i = 0; i < N; ++i) {
        (*this)[i] = array[i];
      }
    }
    return *this;
  }

  Array & operator=(Array && array)
  {
    if constexpr (std::is_trivially_copyable_v<T>) {
      if (this != &array) {
        std::memcpy(data(), array.data(), sizeof(T) * N);
      }
    } else {
      for (size_type i = 0; i < N; ++i) {
        (*this)[i] = std::move(array[i]);
      }
    }
    return *this;
  }

  Array & operator=(const T(& array)[N])
  {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(data(), array, sizeof(T) * N);
    } else {
      for (size_type i = 0; i < N; ++i) {
        (*this)[i] = array[i];
      }
    }
    return *this;
  }

  Array & operator=(T(&& array)[N])
  {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(data(), array, sizeof(T) * N);
    } else {
      for (size_type i = 0; i < N; ++i) {
        (*this)[i] = std::move(array[i]);
      }
    }
    return *this;
  }

  /// @brief Convert to `std::array`.
  /// @return Copied `std::array` value.
  operator std::array<T, N>() const
  {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::array<T, N> result;
      std::memcpy(result.data(), data(), sizeof(T) * N);
      return result;
    } else {
      std::array<T, N> array{};
      for (size_type i = 0; i < N; ++i) {
        array[i] = (*this)[i];
      }
      return array;
    }
  }

  /// @brief Bounds-checked element access.
  /// @param position Element index.
  /// @return Element reference.
  reference at(size_type position)
  {
    if (position >= N) {
      throw std::out_of_range("Array::at: index out of range");
    }
    return (*this)[position];
  }

  /// @brief Bounds-checked element access.
  /// @param position Element index.
  /// @return Element reference.
  const_reference at(size_type position) const
  {
    if (position >= N) {
      throw std::out_of_range("Array::at: index out of range");
    }
    return (*this)[position];
  }

  /// @brief Unchecked element access.
  reference operator[](size_type position) noexcept
  {
    if (std::holds_alternative<MemoryRegion<T>>(storage_)) {
      return std::get<MemoryRegion<T>>(storage_).data()[position];
    }
    return std::get<InternalStorage>(storage_).data()[position];
  }

  /// @brief Unchecked element access.
  const_reference operator[](size_type position) const noexcept
  {
    if (std::holds_alternative<MemoryRegion<T>>(storage_)) {
      return std::get<MemoryRegion<T>>(storage_).data()[position];
    }
    return std::get<InternalStorage>(storage_).data()[position];
  }

  /// @brief First element.
  reference front() noexcept {return (*this)[0];}

  /// @brief First element.
  const_reference front() const noexcept {return (*this)[0];}

  /// @brief Last element.
  reference back() noexcept {return (*this)[N - 1];}

  /// @brief Last element.
  const_reference back() const noexcept {return (*this)[N - 1];}

  /// @brief Pointer to first element.
  pointer data() noexcept
  {
    if (std::holds_alternative<MemoryRegion<T>>(storage_)) {
      return std::get<MemoryRegion<T>>(storage_).data();
    }
    return std::get<InternalStorage>(storage_).data();
  }

  /// @brief Pointer to first element.
  const_pointer data() const noexcept
  {
    if (std::holds_alternative<MemoryRegion<T>>(storage_)) {
      return std::get<MemoryRegion<T>>(storage_).data();
    }
    return std::get<InternalStorage>(storage_).data();
  }

  iterator begin() noexcept {return data();}
  const_iterator begin() const noexcept {return data();}
  const_iterator cbegin() const noexcept {return begin();}
  iterator end() noexcept {return data() + N;}
  const_iterator end() const noexcept {return data() + N;}
  const_iterator cend() const noexcept {return end();}
  reverse_iterator rbegin() noexcept {return reverse_iterator(end());}
  const_reverse_iterator rbegin() const noexcept {return const_reverse_iterator(end());}
  const_reverse_iterator crbegin() const noexcept {return rbegin();}
  reverse_iterator rend() noexcept {return reverse_iterator(begin());}
  const_reverse_iterator rend() const noexcept {return const_reverse_iterator(begin());}
  const_reverse_iterator crend() const noexcept {return rend();}
  constexpr bool empty() const noexcept {return N == 0;}
  constexpr size_type size() const noexcept {return N;}
  constexpr size_type max_size() const noexcept {return N;}

  /// @brief Fill all elements.
  /// @param value Value to assign.
  void fill(const value_type & value)
  {
    for (auto & element : *this) {
      element = value;
    }
  }

  /// @brief Swap storage backing and values.
  /// @param other Array to swap with.
  void swap(Array & other) noexcept
  {
    using std::swap;
    swap(storage_, other.storage_);
  }

  friend void swap(Array & lhs, Array & rhs) noexcept
  {
    lhs.swap(rhs);
  }

  friend bool operator==(const Array & lhs, const Array & rhs)
  {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }

  friend bool operator!=(const Array & lhs, const Array & rhs)
  {
    return !(lhs == rhs);
  }

private:
  std::variant<MemoryRegion<T>, InternalStorage> storage_;
};

}  // namespace rosidl_runtime_cpp

#endif  // ROSIDL_RUNTIME_CPP__EXPERIMENTAL__ARRAY_HPP_
