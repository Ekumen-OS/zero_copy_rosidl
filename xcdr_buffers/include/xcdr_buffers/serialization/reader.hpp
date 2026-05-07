// Copyright 2026 Ekumen Inc.
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

#ifndef XCDR_BUFFERS__SERIALIZATION__READER_HPP_
#define XCDR_BUFFERS__SERIALIZATION__READER_HPP_

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <tcb_span/span.hpp>

#include "xcdr_buffers/common/endianness.hpp"
#include "xcdr_buffers/common/result.hpp"
#include "xcdr_buffers/common/types.hpp"

namespace xcdr_buffers
{

// Type traits for read<T>() dispatch
namespace detail
{
  // Span detection and element extraction
template<typename T>
struct is_span : std::false_type {};
template<typename T, std::size_t E>
struct is_span<tcb::span<T, E>>: std::true_type {};

template<typename T>
struct span_element_type;
template<typename T, std::size_t E>
struct span_element_type<tcb::span<T, E>> { using type = T; };

template<typename T>
struct span_extent;
template<typename T, std::size_t E>
struct span_extent<tcb::span<T, E>> { static constexpr std::size_t value = E; };

  // Array detection and element extraction
template<typename T>
struct is_std_array : std::false_type {};
template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>>: std::true_type {};

template<typename T>
struct array_element_type;
template<typename T, std::size_t N>
struct array_element_type<std::array<T, N>> { using type = T; };

template<typename T>
struct array_extent;
template<typename T, std::size_t N>
struct array_extent<std::array<T, N>> { static constexpr std::size_t value = N; };

  // Vector detection and element extraction
template<typename T>
struct is_std_vector : std::false_type {};
template<typename T>
struct is_std_vector<std::vector<T>>: std::true_type {};

template<typename T>
struct vector_element_type;
template<typename T>
struct vector_element_type<std::vector<T>> { using type = T; };

  // Helper aliases
template<typename T>
constexpr bool is_span_v = is_span<T>::value;

template<typename T>
constexpr bool is_std_array_v = is_std_array<T>::value;

template<typename T>
constexpr bool is_std_vector_v = is_std_vector<T>::value;

template<typename T>
using span_element_type_t = typename span_element_type<T>::type;

template<typename T>
using array_element_type_t = typename array_element_type<T>::type;

template<typename T>
using vector_element_type_t = typename vector_element_type<T>::type;

constexpr std::size_t dynamic_extent = static_cast<std::size_t>(-1);
}  // namespace detail

/**
 * @brief Stateful reader for streaming XCDR deserialization.
 *
 * Does not own the buffer - takes a span and maintains position.
 *
 * The XCDR header is automatically read and validated on construction:
 * - Constructor throws XCdrError if header is invalid (convenient API)
 * - Static wrap() factory returns XCdrResult<XCdrReader> for explicit error handling
 *
 * Automatically handles:
 * - Alignment requirements (CDR data-relative alignment)
 * - Endianness conversion
 * - Length prefix reading for strings and sequences
 */
class XCdrReader
{
public:
  /**
   * @brief Construct a reader from buffer (throws on invalid header).
   *
   * Automatically reads and validates the XCDR header. Throws XCdrError if
   * header is invalid or buffer is too small.
   *
   * @param buffer Buffer to read from (must outlive this reader)
   * @param mr Memory resource for internal state (default: nullptr)
   * @throws XCdrError if header validation fails
   */
  explicit XCdrReader(
    tcb::span<const uint8_t> buffer,
    std::pmr::memory_resource * mr = nullptr);

  /// Template overload to accept any container with data() and size()
  template<typename Container,
    typename = std::enable_if_t<
      std::is_convertible_v<decltype(std::declval<const Container &>().data()), const uint8_t *> &&
      std::is_convertible_v<decltype(std::declval<const Container &>().size()), size_t>>>
  explicit XCdrReader(
    const Container & buffer,
    std::pmr::memory_resource * mr = nullptr)
  : XCdrReader(tcb::span<const uint8_t>(buffer.data(), buffer.size()), mr)
  {}

  /**
   * @brief Factory function to create reader with explicit error handling.
   *
   * Use this instead of constructor when you want to handle header validation
   * errors explicitly without exceptions.
   *
   * @param buffer Buffer to read from
   * @param mr Memory resource for internal state (default: nullptr)
   * @return Result containing reader or error
   */
  static XCdrResult<XCdrReader> wrap(
    tcb::span<const uint8_t> buffer,
    std::pmr::memory_resource * mr = nullptr);

  /// Template overload to accept any container with data() and size()
  template<typename Container,
    typename = std::enable_if_t<
      std::is_convertible_v<decltype(std::declval<const Container &>().data()), const uint8_t *> &&
      std::is_convertible_v<decltype(std::declval<const Container &>().size()), size_t>>>
  static XCdrResult<XCdrReader> wrap(
    const Container & buffer,
    std::pmr::memory_resource * mr = nullptr)
  {
    return wrap(tcb::span<const uint8_t>(buffer.data(), buffer.size()), mr);
  }

  /**
   * @brief Get the endianness detected from header.
   *
   * @return Endianness of the buffer
   */
  XCdrEndianness endianness() const {return endianness_;}

  /**
   * @brief Unified read method for all types.
   *
   * Supported types:
   * - Primitives: read<int32_t>(), read<double>(), etc.
   * - Strings (zero-copy): read<std::string_view>(), read<std::u16string_view>()
   * - Strings (allocating): read<std::string>(), read<std::u16string>()
   * - Arrays (zero-copy): read<tcb::span<const T, N>>() - fixed extent
   * - Arrays (allocating): read<std::array<T, N>>()
   * - Sequences (zero-copy): read<tcb::span<const T>>() - dynamic extent
   * - Sequences (allocating): read<std::vector<T>>()
   *
   * @tparam T Type to read
   * @return Result containing value or error
   */
  template<typename T>
  XCdrResult<T> read();

  /**
   * @brief Read directly into an existing lvalue.
   *
   * More efficient than read<T>() for allocating types since it avoids
   * copies and can reuse existing allocations.
   *
   * Supported types:
   * - Primitives: read_into(int32_t&)
   * - Strings (allocating): read_into(std::string&), read_into(std::u16string&)
   * - Arrays (allocating): read_into(std::array<T, N>&)
   * - Sequences (allocating): read_into(std::vector<T>&)
   *
   * @param value Reference to variable to read into
   * @return Status indicating success or error
   */
  template<typename T>
  XCdrStatus read_into(T & value);

  /**
   * @brief Begin reading an array.
   *
   * Arrays have no length prefix, so caller must know expected count.
   *
   * @param expected_count Expected number of elements
   * @return Status indicating success or error
   */
  XCdrStatus begin_read_array(size_t expected_count);

  /**
   * @brief End reading an array.
   *
   * @return Status indicating success or error
   */
  XCdrStatus end_read_array();

  /**
   * @brief Begin reading a sequence.
   *
   * Reads 4-byte length prefix.
   *
   * @return Result containing element count or error
   */
  XCdrResult<size_t> begin_read_sequence();

  /**
   * @brief End reading a sequence.
   *
   * @return Status indicating success or error
   */
  XCdrStatus end_read_sequence();

  /**
   * @brief Begin reading a struct.
   *
   * @return Status indicating success or error
   */
  XCdrStatus begin_read_struct();

  /**
   * @brief End reading a struct.
   *
   * @return Status indicating success or error
   */
  XCdrStatus end_read_struct();

  /**
   * @brief Reset to beginning of buffer.
   *
   * @param buffer New buffer to read from
   */
  void reset(tcb::span<const uint8_t> buffer);

  /**
   * @brief Get number of bytes remaining.
   *
   * @return Bytes remaining from current position
   */
  size_t bytes_remaining() const;

private:
  tcb::span<const uint8_t> buffer_;
  XCdrEndianness endianness_;
  size_t position_;
  size_t data_start_position_;  // Position where data starts (after header)
  bool header_read_;

  struct ReadContext
  {
    size_t start_pos;
    size_t element_count;
    size_t elements_read;
  };
  std::pmr::vector<ReadContext> context_stack_;

  // Private constructor for factory function (no automatic header read)
  XCdrReader(
    tcb::span<const uint8_t> buffer,
    std::pmr::memory_resource * mr,
    bool /* unused tag */);

  XCdrStatus align_position(size_t alignment);
  XCdrStatus ensure_available(size_t size);
  XCdrResult<XCdrEndianness> read_header();  // Now private, called by constructor
};

// Template implementations

template<typename T>
XCdrResult<T> XCdrReader::read()
{
  if (!header_read_) {
    return error("Header must be read before reading data");
  }

  // Dispatch based on type
  if constexpr (std::is_arithmetic_v<T>) {
    // Primitive types
    auto status = align_position(sizeof(T));
    if (!status) {
      return tl::unexpected(status.error());
    }

    status = ensure_available(sizeof(T));
    if (!status) {
      return tl::unexpected(status.error());
    }

    T value;
    tcb::span<const uint8_t> src(buffer_.data() + position_, sizeof(T));
    read_from_bytes(&value, src, endianness_);

    position_ += sizeof(T);
    return ok(value);

  } else if constexpr (std::is_same_v<T, std::string_view>) {
    // String (zero-copy)
    auto status = align_position(kStringLengthPrefixSize);
    if (!status) {
      return tl::unexpected(status.error());
    }

    status = ensure_available(kStringLengthPrefixSize);
    if (!status) {
      return tl::unexpected(status.error());
    }

    uint32_t length;
    tcb::span<const uint8_t> length_src(buffer_.data() + position_, kStringLengthPrefixSize);
    read_from_bytes(&length, length_src, endianness_);
    position_ += kStringLengthPrefixSize;

    if (length == 0) {
      return error("Invalid string length: 0");
    }

    size_t str_length = length - kStringNullTerminatorSize;

    status = ensure_available(length);
    if (!status) {
      return tl::unexpected(status.error());
    }

    if (buffer_[position_ + str_length] != '\0') {
      return error("String not null-terminated");
    }

    std::string_view result(reinterpret_cast<const char *>(buffer_.data() + position_), str_length);
    position_ += length;

    return ok(result);

  } else if constexpr (std::is_same_v<T, std::u16string_view>) {
    // Wide string (zero-copy)
    auto status = align_position(kStringLengthPrefixSize);
    if (!status) {
      return tl::unexpected(status.error());
    }

    status = ensure_available(kStringLengthPrefixSize);
    if (!status) {
      return tl::unexpected(status.error());
    }

    uint32_t length_bytes;
    tcb::span<const uint8_t> length_src(buffer_.data() + position_, kStringLengthPrefixSize);
    read_from_bytes(&length_bytes, length_src, endianness_);
    position_ += kStringLengthPrefixSize;

    if (length_bytes == 0) {
      return error("Invalid wstring length: 0");
    }

    if (length_bytes < kWStringNullTerminatorSize) {
      return error("Invalid wstring length: too small for terminator");
    }

    if (length_bytes % sizeof(char16_t) != 0) {
      return error("Invalid wstring length: not aligned to char16_t");
    }

    size_t char_count = (length_bytes - kWStringNullTerminatorSize) / sizeof(char16_t);

    status = ensure_available(length_bytes);
    if (!status) {
      return tl::unexpected(status.error());
    }

    const char16_t * data_ptr = reinterpret_cast<const char16_t *>(buffer_.data() + position_);
    std::u16string_view result(data_ptr, char_count);
    position_ += length_bytes;

    return ok(result);

  } else if constexpr (std::is_same_v<T, std::string>) {
    // Allocating string - delegate to read_into
    T result;
    auto status = read_into(result);
    if (!status) {
      return tl::unexpected(status.error());
    }
    return ok(std::move(result));

  } else if constexpr (std::is_same_v<T, std::u16string>) {
    // Allocating wide string - delegate to read_into
    T result;
    auto status = read_into(result);
    if (!status) {
      return tl::unexpected(status.error());
    }
    return ok(std::move(result));

  } else if constexpr (detail::is_span_v<T>) {
    // tcb::span<const T, N> or tcb::span<const T> (dynamic extent)
    using ElemType = std::remove_const_t<typename detail::span_element_type<T>::type>;
    constexpr std::size_t Extent = detail::span_extent<T>::value;

    static_assert(std::is_arithmetic_v<ElemType>, "Span element type must be arithmetic");

    if constexpr (Extent == detail::dynamic_extent) {
      // Dynamic extent - sequence
      auto count_result = begin_read_sequence();
      if (!count_result) {
        return tl::unexpected(count_result.error());
      }

      size_t count = *count_result;

      if (endianness_ != kSystemEndianness) {
        return error(
            "Dynamic-extent span with mismatched endianness requires element-by-element reading");
      }

      auto status = align_position(alignof(ElemType));
      if (!status) {
        return tl::unexpected(status.error());
      }

      size_t total_size = count * sizeof(ElemType);
      status = ensure_available(total_size);
      if (!status) {
        return tl::unexpected(status.error());
      }

      const ElemType * data_ptr = reinterpret_cast<const ElemType *>(buffer_.data() + position_);
      T result(data_ptr, count);
      position_ += total_size;

      status = end_read_sequence();
      if (!status) {
        return tl::unexpected(status.error());
      }

      return ok(result);
    } else {
      // Fixed extent - array
      auto status = begin_read_array(Extent);
      if (!status) {
        return tl::unexpected(status.error());
      }

      if (endianness_ != kSystemEndianness) {
        return error(
            "Fixed-extent span with mismatched endianness requires element-by-element reading");
      }

      status = align_position(alignof(ElemType));
      if (!status) {
        return tl::unexpected(status.error());
      }

      constexpr size_t total_size = Extent * sizeof(ElemType);
      status = ensure_available(total_size);
      if (!status) {
        return tl::unexpected(status.error());
      }

      const ElemType * data_ptr = reinterpret_cast<const ElemType *>(buffer_.data() + position_);
      T result(data_ptr, Extent);
      position_ += total_size;

      status = end_read_array();
      if (!status) {
        return tl::unexpected(status.error());
      }

      return ok(result);
    }

  } else if constexpr (detail::is_std_array_v<T>) {
    // std::array<T, N> - delegate to read_into
    T result;
    auto status = read_into(result);
    if (!status) {
      return tl::unexpected(status.error());
    }
    return ok(std::move(result));

  } else if constexpr (detail::is_std_vector_v<T>) {
    // std::vector<T> - delegate to read_into
    T result;
    auto status = read_into(result);
    if (!status) {
      return tl::unexpected(status.error());
    }
    return ok(std::move(result));

  } else {
    static_assert(detail::is_span_v<T>|| detail::is_std_array_v<T>|| detail::is_std_vector_v<T>,
                  "read<T> requires T to be arithmetic, string, span, std::array, or std::vector");
  }
}

// read_into() implementations

template<typename T>
XCdrStatus XCdrReader::read_into(T & value)
{
  if (!header_read_) {
    return error("Header must be read before reading data");
  }

  // Dispatch based on type
  if constexpr (std::is_arithmetic_v<T>) {
    // Primitive types - read directly into reference
    auto status = align_position(sizeof(T));
    if (!status) {
      return status;
    }

    status = ensure_available(sizeof(T));
    if (!status) {
      return status;
    }

    tcb::span<const uint8_t> src(buffer_.data() + position_, sizeof(T));
    read_from_bytes(&value, src, endianness_);

    position_ += sizeof(T);
    return ok();

  } else if constexpr (std::is_same_v<T, std::string>) {
    // Allocating string - read string_view then assign
    auto view_result = read<std::string_view>();
    if (!view_result) {
      return tl::unexpected(view_result.error());
    }
    value.assign(view_result->begin(), view_result->end());
    return ok();

  } else if constexpr (std::is_same_v<T, std::u16string>) {
    // Allocating wide string - read u16string_view then assign
    auto view_result = read<std::u16string_view>();
    if (!view_result) {
      return tl::unexpected(view_result.error());
    }
    value.assign(view_result->begin(), view_result->end());
    return ok();

  } else if constexpr (detail::is_std_array_v<T>) {
    // std::array<T, N> - read elements into array
    using ElemType = typename detail::array_element_type<T>::type;
    constexpr std::size_t N = detail::array_extent<T>::value;

    auto status = begin_read_array(N);
    if (!status) {
      return status;
    }

    for (std::size_t i = 0; i < N; ++i) {
      if constexpr (std::is_arithmetic_v<ElemType>) {
        status = read_into(value[i]);
      } else {
        auto elem_result = read<ElemType>();
        if (!elem_result) {
          return tl::unexpected(elem_result.error());
        }
        value[i] = *elem_result;
      }
      if (!status) {
        return status;
      }
    }

    return end_read_array();

  } else if constexpr (detail::is_std_vector_v<T>) {
    // std::vector<T> - read sequence into vector
    using ElemType = typename detail::vector_element_type<T>::type;

    auto count_result = begin_read_sequence();
    if (!count_result) {
      return tl::unexpected(count_result.error());
    }

    size_t count = *count_result;
    value.clear();
    value.reserve(count);

    for (size_t i = 0; i < count; ++i) {
      if constexpr (std::is_arithmetic_v<ElemType>) {
        ElemType elem;
        auto status = read_into(elem);
        if (!status) {
          return status;
        }
        value.push_back(elem);
      } else {
        auto elem_result = read<ElemType>();
        if (!elem_result) {
          return tl::unexpected(elem_result.error());
        }
        value.push_back(*elem_result);
      }
    }

    return end_read_sequence();

  } else {
    static_assert(std::is_arithmetic_v<T>||
                  std::is_same_v<T, std::string>||
                  std::is_same_v<T, std::u16string>||
                  detail::is_std_array_v<T>||
                  detail::is_std_vector_v<T>,
                  "read_into() requires T to be arithmetic, std::string, "
                  "std::u16string, std::array, or std::vector");
    return error("Unsupported type for read_into()");
  }
}

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__SERIALIZATION__READER_HPP_
