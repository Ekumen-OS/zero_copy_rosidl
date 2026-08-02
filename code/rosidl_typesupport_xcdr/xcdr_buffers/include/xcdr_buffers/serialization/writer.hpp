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
#include <limits>
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
   * @brief Construct a writer in fixed-size mode with pre-allocated buffer.
   *
   * Writes directly to the provided buffer without dynamic allocation.
   * If data doesn't fit, the writer enters an error state.
   *
   * @param fixed_buffer Fixed-size buffer to write into
   * @param endianness Endianness for output data
   */
  explicit XCdrWriter(
    tcb::span<uint8_t> fixed_buffer,
    XCdrEndianness endianness = XCdrEndianness::kLittleEndian);

  /**
   * @brief Construct a writer in fixed-size mode starting at an absolute offset.
   *
   * Writes directly to the provided buffer starting at @p start_offset without
   * writing an XCDR encapsulation header (the header is assumed to already
   * exist at offset 0).  Ideal for in-place compaction where the header is
   * already present and only the data payload needs to be rewritten.
   *
   * If data doesn't fit in the remaining space, the writer enters an error state.
   *
   * @param fixed_buffer Fixed-size buffer to write into
   * @param start_offset Absolute offset where data writes begin (e.g. 4 to skip header)
   * @param endianness Endianness for output data
   */
  explicit XCdrWriter(
    tcb::span<uint8_t> fixed_buffer,
    size_t start_offset,
    XCdrEndianness endianness = XCdrEndianness::kLittleEndian);

  /**
   * @brief Check if writer encountered an error (only meaningful in fixed-size mode).
   *
   * @return true if buffer overflow occurred in fixed-size mode
   */
  bool has_error() const {return overflow_error_;}

  /**
   * @brief Return the number of bytes written so far (absolute position).
   *
   * In the default growing mode this is the buffer size.
   * In fixed-size mode this is the absolute write cursor position.
   * In offset mode the start offset is counted, so bytes_written() always
   * reflects the total span consumed.
   *
   * @return Total bytes written from the start of the buffer.
   */
  size_t bytes_written() const {return write_position_;}

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

  // ========================================================================
  // Skip API (non-mutating position advancement)
  //
  // Mirrors write() in position advancement but writes nothing to the buffer.
  // No memset for alignment padding — strictly non-mutating in fixed mode.
  // ========================================================================

  /**
   * @brief Skip a primitive value (advance position by sizeof(T) with alignment).
   *
   * @tparam T Arithmetic type (value is not needed, only size/alignment)
   */
  template<typename T>
  void skip();

  /**
   * @brief Skip a string (advance past prefix + data + null terminator).
   *
   * @param char_count Number of characters in the string (excluding null)
   */
  void skip_string(size_t char_count);

  /**
   * @brief Skip a wide string (advance past prefix + per-char data + null).
   *
   * @param char_count Number of characters in the string (excluding null)
   */
  void skip_wstring(size_t char_count);

  /**
   * @brief Skip a contiguous array of primitives (bulk position advance).
   *
   * Advances by alignment + sizeof(T) * count in one call.  Equivalent
   * to calling skip<T>() count times but much cheaper for large arrays.
   * Zero-count is a no-op (does not align).
   *
   * @tparam T Primitive element type (must be arithmetic)
   * @param count Number of elements to skip
   */
  template<typename T>
  void skip_array(size_t count);

  /**
   * @brief Skip a sequence of primitives (bulk position advance + context).
   *
   * Writes the length prefix (via begin_skip_sequence), skips the element
   * payload as a single bulk advance, and ends the sequence.
   *
   * @tparam T Primitive element type (must be arithmetic)
   * @param count Number of elements to skip
   */
  template<typename T>
  void skip_sequence(size_t count);

  /**
   * @brief Skip a sequence length prefix (advance + push context, no byte write).
   *
   * Must be paired with end_write_sequence().
   * @param count Number of elements (element payloads must be skipped separately)
   */
  void begin_skip_sequence(size_t count);

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
  tcb::span<uint8_t> fixed_span_;  // For fixed-size mode
  bool is_fixed_mode_;  // True if using fixed span
  bool overflow_error_;  // True if overflow occurred in fixed mode
  size_t write_position_;  // Current position for fixed mode
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
  /// Non-mutating position advancement (no memset, no writes).
  void skip_advance(size_t alignment, size_t advance_size);
  /// Bulk-write an arithmetic span after a single alignment/reserve.
  template<typename T>
  void write_pod_span(tcb::span<const T> values);
};

// Template implementations

template<typename T>
void XCdrWriter::write(T value)
{
  static_assert(std::is_arithmetic_v<T>, "write<T> only supports arithmetic types");

  ensure_header_written();

  if (overflow_error_) {
    return;  // Already in error state, don't continue
  }

  align_and_reserve(sizeof(T), sizeof(T));

  if (overflow_error_) {
    return;  // Overflow occurred during reserve
  }

  if (is_fixed_mode_) {
    tcb::span<uint8_t> dst(fixed_span_.data() + write_position_ - sizeof(T), sizeof(T));
    write_to_bytes(dst, value, endianness_);
  } else {
    tcb::span<uint8_t> dst(buffer_.data() + buffer_.size() - sizeof(T), sizeof(T));
    write_to_bytes(dst, value, endianness_);
  }
}

template<typename T>
void XCdrWriter::skip()
{
  static_assert(std::is_arithmetic_v<T>, "skip<T> only supports arithmetic types");

  ensure_header_written();

  if (overflow_error_) {
    return;
  }

  skip_advance(sizeof(T), sizeof(T));
}

template<typename T>
void XCdrWriter::skip_array(size_t count)
{
  static_assert(std::is_arithmetic_v<T>, "skip_array only supports arithmetic types");

  ensure_header_written();

  if (overflow_error_) {
    return;
  }

  if (0 == count) {
    return;  // No-op: avoid alignment drift from skip_advance(x, 0)
  }

  // Guard against multiplication overflow.
  if (count > SIZE_MAX / sizeof(T)) {
    overflow_error_ = true;
    return;
  }

  skip_advance(sizeof(T), sizeof(T) * count);
}

template<typename T>
void XCdrWriter::skip_sequence(size_t count)
{
  static_assert(std::is_arithmetic_v<T>, "skip_sequence only supports arithmetic types");

  begin_skip_sequence(count);
  if (overflow_error_) {return;}
  skip_array<T>(count);
  if (overflow_error_) {return;}
  end_write_sequence();
}

template<typename T>
void XCdrWriter::write_pod_span(tcb::span<const T> values)
{
  static_assert(std::is_arithmetic_v<T>, "write_pod_span only supports arithmetic types");

  if (values.empty()) {
    return;
  }

  ensure_header_written();

  if (overflow_error_) {
    return;
  }

  // Guard against multiplication overflow.
  if (values.size() > (std::numeric_limits<size_t>::max)() / sizeof(T)) {
    overflow_error_ = true;
    return;
  }

  const size_t total = values.size() * sizeof(T);

  align_and_reserve(sizeof(T), total);

  if (overflow_error_) {
    return;
  }

  // The reserved region starts right before the (advanced) write position.
  tcb::span<uint8_t> dst;
  if (is_fixed_mode_) {
    dst = tcb::span<uint8_t>(fixed_span_.data() + write_position_ - total, total);
  } else {
    dst = tcb::span<uint8_t>(buffer_.data() + buffer_.size() - total, total);
  }

  // Bulk copy when the wire endianness matches the host; byte-swap otherwise.
  write_to_bytes(dst, values, endianness_);
}

template<typename T>
void XCdrWriter::write_array(tcb::span<const T> values)
{
  static_assert(std::is_arithmetic_v<T>, "write_array only supports arithmetic types");

  begin_write_array(values.size());

  write_pod_span(values);

  end_write_array();
}

template<typename T>
void XCdrWriter::write_sequence(tcb::span<const T> values)
{
  static_assert(std::is_arithmetic_v<T>, "write_sequence only supports arithmetic types");

  begin_write_sequence(values.size());

  write_pod_span(values);

  end_write_sequence();
}

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__SERIALIZATION__WRITER_HPP_
