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

#ifndef XCDR_BUFFERS__SERIALIZATION__WRITER_HPP_
#define XCDR_BUFFERS__SERIALIZATION__WRITER_HPP_

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <string_view>
#include <string>
#include <vector>

#include <tcb_span/span.hpp>

#include "xcdr_buffers/common/endianness.hpp"
#include "xcdr_buffers/common/types.hpp"

namespace xcdr_buffers
{

/**
 * @brief Stateful writer for streaming XCDR serialization.
 *
 * Manages its own buffer and automatically handles:
 * - XCDR encapsulation header
 * - Alignment requirements
 * - Endianness conversion
 * - Length prefixes for strings and sequences
 */
class XCdrWriter
{
public:
  /**
   * @brief Construct a writer.
   *
   * @param endianness Endianness for output data
   * @param mr Memory resource for internal buffer (default: std::pmr::get_default_resource())
   */
  explicit XCdrWriter(
    XCdrEndianness endianness = XCdrEndianness::kLittleEndian,
    std::pmr::memory_resource * mr = nullptr);

  /**
   * @brief Write a primitive value.
   *
   * @tparam T Arithmetic type
   * @param value Value to write
   */
  template<typename T>
  void write(T value);

  /**
   * @brief Write a string.
   *
   * Writes 4-byte length prefix (including null terminator) followed by string data and null terminator.
   *
   * @param str String to write
   */
  void write(std::string_view str);

  void write(const char * str)
  {
    write(std::string_view(str));
  }

  void write(const std::string & str)
  {
    write(std::string_view(str));
  }

  /**
   * @brief Write a wide string.
   *
   * Writes 4-byte length prefix (including null terminator) followed by wstring data and null terminator.
   *
   * @param str Wide string to write
   */
  void write(std::u16string_view str);

  /**
   * @brief Begin writing an array.
   *
   * Arrays have no length prefix in XCDR.
   *
   * @param count Number of elements (for validation)
   */
  void begin_write_array(size_t count);

  /**
   * @brief Write array elements (convenience method).
   *
   * @tparam T Element type (must be arithmetic)
   * @param values Elements to write
   */
  template<typename T>
  void write_array(tcb::span<const T> values);

  template<typename Container,
    typename T = typename Container::value_type,
    typename = std::enable_if_t<std::is_arithmetic_v<T>>>
  void write_array(const Container & values)
  {
    write_array(tcb::span<const T>(values.data(), values.size()));
  }

  /**
   * @brief End writing an array.
   */
  void end_write_array();

  /**
   * @brief Begin writing a sequence.
   *
   * Writes 4-byte length prefix.
   *
   * @param count Number of elements
   */
  void begin_write_sequence(size_t count);

  /**
   * @brief Write sequence elements (convenience method).
   *
   * @tparam T Element type (must be arithmetic)
   * @param values Elements to write
   */
  template<typename T>
  void write_sequence(tcb::span<const T> values);

  template<typename Container,
    typename T = typename Container::value_type,
    typename = std::enable_if_t<std::is_arithmetic_v<T>>>
  void write_sequence(const Container & values)
  {
    write_sequence(tcb::span<const T>(values.data(), values.size()));
  }

  /**
   * @brief End writing a sequence.
   */
  void end_write_sequence();

  /**
   * @brief Begin writing a struct.
   */
  void begin_write_struct();

  /**
   * @brief End writing a struct.
   */
  void end_write_struct();

  /**
   * @brief Get current buffer contents.
   *
   * @return Span of buffer data
   */
  tcb::span<const uint8_t> data() const;

  /**
   * @brief Finalize and retrieve buffer, resetting writer state.
   *
   * @return Buffer contents
   */
  std::pmr::vector<uint8_t> flush();

  /**
   * @brief Reset writer for reuse.
   *
   * @param endianness Endianness for next write session
   */
  void reset(XCdrEndianness endianness = XCdrEndianness::kLittleEndian);

private:
  std::pmr::vector<uint8_t> buffer_;
  XCdrEndianness endianness_;
  bool header_written_;

  struct WriteContext
  {
    size_t start_pos;
    size_t element_count;
    size_t elements_written;
  };
  std::pmr::vector<WriteContext> context_stack_;

  void ensure_header_written();
  void align_and_reserve(size_t alignment, size_t size);
};

// Template implementations

template<typename T>
void XCdrWriter::write(T value)
{
  static_assert(std::is_arithmetic_v<T>, "write<T> only supports arithmetic types");

  ensure_header_written();
  align_and_reserve(sizeof(T), sizeof(T));

  tcb::span<uint8_t> dst(buffer_.data() + buffer_.size() - sizeof(T), sizeof(T));
  write_to_bytes(dst, value, endianness_);
}

template<typename T>
void XCdrWriter::write_array(tcb::span<const T> values)
{
  static_assert(std::is_arithmetic_v<T>, "write_array only supports arithmetic types");

  begin_write_array(values.size());

  // Write all elements
  for (const auto & value : values) {
    write(value);
  }

  end_write_array();
}

template<typename T>
void XCdrWriter::write_sequence(tcb::span<const T> values)
{
  static_assert(std::is_arithmetic_v<T>, "write_sequence only supports arithmetic types");

  begin_write_sequence(values.size());

  // Write all elements
  for (const auto & value : values) {
    write(value);
  }

  end_write_sequence();
}

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__SERIALIZATION__WRITER_HPP_
