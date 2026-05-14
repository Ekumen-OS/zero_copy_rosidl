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

#ifndef XCDR_BUFFERS__LAYOUT__LAYOUT_PARSER_HPP_
#define XCDR_BUFFERS__LAYOUT__LAYOUT_PARSER_HPP_

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <string>
#include <string_view>

#include "tcb_span/span.hpp"
#include "xcdr_buffers/common/endianness.hpp"
#include "xcdr_buffers/common/result.hpp"
#include "xcdr_buffers/common/types.hpp"
#include "xcdr_buffers/layout/layout.hpp"
#include "xcdr_buffers/layout/layout_builder.hpp"

namespace xcdr_buffers
{

/// Parses XCDR buffers to recover layout information.
///
/// The parser works by reading through a buffer according to user-specified
/// type expectations, discovering sizes/counts from the buffer's length prefixes,
/// and building a layout that describes the buffer's structure.
///
/// Usage:
/// ```cpp
/// std::vector<uint8_t> buffer = /* serialized data */;
/// XCdrLayoutParser parser(buffer);
///
/// auto status = parser.parse_header();
/// if (!status) return status;
///
/// status = parser.begin_parse_struct("MyStruct");
/// status = parser.parse_primitive(XCdrPrimitiveKind::kUint32);
/// status = parser.parse_string();
/// status = parser.begin_parse_sequence();  // Reads count from buffer
/// status = parser.parse_primitive(XCdrPrimitiveKind::kFloat64);
/// status = parser.end_parse_sequence();
/// status = parser.end_parse_struct();
///
/// auto layout_result = parser.finalize();
/// if (layout_result) {
///   XCdrStructLayout layout = std::move(*layout_result);
///   // Use layout...
/// }
/// ```
class XCdrLayoutParser
{
public:
  /// Factory function that creates a parser with XCdrResult-based error handling.
  ///
  /// @param buffer The XCDR buffer to parse (must include header)
  /// @param mr Memory resource for internal allocations (layout storage)
  /// @return Parser on success, error if buffer too small or invalid header
  static XCdrResult<XCdrLayoutParser> create(
    tcb::span<const uint8_t> buffer,
    std::pmr::memory_resource * mr = nullptr);

  /// Constructs a parser for the given buffer.
  ///
  /// Throws XCdrError if buffer is too small or header is invalid.
  ///
  /// @param buffer The XCDR buffer to parse (must include header)
  /// @param mr Memory resource for internal allocations (layout storage)
  explicit XCdrLayoutParser(
    tcb::span<const uint8_t> buffer,
    std::pmr::memory_resource * mr = nullptr);

  /// Template overload to accept any container with data() and size()
  template<typename Container,
    typename = std::enable_if_t<
      std::is_convertible_v<decltype(std::declval<const Container &>().data()), const uint8_t *>&&
      std::is_convertible_v<decltype(std::declval<const Container &>().size()), size_t>>>
  explicit XCdrLayoutParser(
    const Container & buffer,
    std::pmr::memory_resource * mr = nullptr)
  : XCdrLayoutParser(tcb::span<const uint8_t>(buffer.data(), buffer.size()), mr)
  {}

  /// Parses a primitive value (struct context).
  ///
  /// Advances the read offset by the primitive's size (with alignment).
  ///
  /// @param name Field name (empty for auto-generated names)
  /// @param kind The primitive type to parse
  /// @return ok() on success, error() if buffer overflow or header not parsed
  XCdrStatus parse_primitive(std::string_view name, XCdrPrimitiveKind kind);

  /// Parses a primitive value (array/sequence element context).
  ///
  /// Use inside begin/end_parse_array or begin/end_parse_sequence.
  ///
  /// @param kind The primitive type to parse
  /// @return ok() on success, error() if buffer overflow or header not parsed
  XCdrStatus parse_primitive(XCdrPrimitiveKind kind);

  /// Parses a string value (struct context).
  ///
  /// Reads the length prefix (4 bytes) and advances by that many bytes.
  ///
  /// @param name Field name (empty for auto-generated names)
  /// @param char_kind Character kind (char8 or char16)
  /// @return ok() on success, error() if buffer overflow or invalid length
  XCdrStatus parse_string(std::string_view name, XCdrCharKind char_kind = XCdrCharKind::kChar8);

  /// Parses a string value (array/sequence element context).
  ///
  /// Use inside begin/end_parse_array or begin/end_parse_sequence.
  /// Reads the length prefix (4 bytes) and advances by that many bytes.
  ///
  /// @param char_kind Character kind (char8 or char16)
  /// @return ok() on success, error() if buffer overflow or invalid length
  XCdrStatus parse_string(XCdrCharKind char_kind = XCdrCharKind::kChar8);

  /// Parses a primitive array (one-shot operation) with explicit name.
  ///
  /// Use this for arrays of primitives. For arrays of strings or structs,
  /// use begin/end_parse_array().
  ///
  /// @param name Field name (must not be empty)
  /// @param kind Primitive element kind
  /// @param count Number of elements
  /// @return ok() on success, error() if buffer overflow or name is empty
  XCdrStatus parse_primitive_array(
    std::string_view name, XCdrPrimitiveKind kind, size_t count);

  /// Parses a primitive array (one-shot operation) with auto-generated name.
  ///
  /// @param kind Primitive element kind
  /// @param count Number of elements
  /// @return ok() on success, error() if buffer overflow
  XCdrStatus parse_primitive_array(XCdrPrimitiveKind kind, size_t count);

  /// Parses a primitive sequence (one-shot operation) with explicit name.
  ///
  /// Use this for sequences of primitives. For sequences of strings or
  /// structs, use begin/end_parse_sequence().
  /// Reads the sequence count from the buffer and validates against
  /// actual_count.
  ///
  /// @param name Field name (must not be empty)
  /// @param kind Primitive element kind
  /// @param actual_count Expected number of elements
  /// @return ok() on success, error() if buffer overflow, count mismatch, or name is empty
  XCdrStatus parse_primitive_sequence(
    std::string_view name, XCdrPrimitiveKind kind,
    size_t actual_count);

  /// Parses a primitive sequence (one-shot operation) with auto-generated name.
  ///
  /// @param kind Primitive element kind
  /// @param actual_count Expected number of elements
  /// @return ok() on success, error() if buffer overflow or count mismatch
  XCdrStatus parse_primitive_sequence(XCdrPrimitiveKind kind, size_t actual_count);

  /// Begins parsing a fixed-size array (struct context) with explicit name.
  ///
  /// Use this for arrays of non-primitive types (strings, structs).
  /// For primitive arrays, use parse_primitive_array().
  /// After this, call parse_* methods `count` times to parse each element,
  /// then call end_parse_array().
  /// All elements must be the same type category (all strings with same
  /// char_kind, or all structs).
  /// Individual element sizes may vary (e.g., different string lengths).
  ///
  /// @param name Field name (must not be empty)
  /// @param count Number of elements in the array
  /// @return ok() on success, error() if header not parsed
  XCdrStatus begin_parse_array(std::string_view name, size_t count);

  /// Begins parsing a fixed-size array (struct context) with auto-generated
  /// name.
  ///
  /// Same as begin_parse_array(name, count) but uses auto-generated field name
  /// (field_0, field_1, etc.).
  ///
  /// @param count Number of elements in the array
  /// @return ok() on success, error() if header not parsed
  XCdrStatus begin_parse_array(size_t count);

  /// Ends parsing the current array.
  ///
  /// Validates that exactly `count` elements were parsed.
  ///
  /// @return ok() on success, error() if not currently in an array
  XCdrStatus end_parse_array();

  /// Begins parsing a sequence (struct context) with explicit name.
  ///
  /// Use this for sequences of non-primitive types (strings, structs).
  /// For primitive sequences, use parse_primitive_sequence().
  /// Reads the sequence count (4 bytes) from the buffer and begins the
  /// sequence.
  /// After this, call parse_* methods `count` times to parse each element,
  /// then call end_parse_sequence().
  /// All elements must be the same type category (all strings with same
  /// char_kind, or all structs).
  /// Individual element sizes may vary (e.g., different string lengths).
  ///
  /// @param name Field name (must not be empty)
  /// @return Result containing the sequence count on success, error otherwise
  XCdrResult<size_t> begin_parse_sequence(std::string_view name);

  /// Begins parsing a sequence (struct context) with auto-generated name.
  ///
  /// Same as begin_parse_sequence(name) but uses auto-generated field name
  /// (field_0, field_1, etc.).
  ///
  /// @return Result containing the sequence count on success, error otherwise
  XCdrResult<size_t> begin_parse_sequence();

  /// Ends parsing the current sequence.
  ///
  /// @return ok() on success, error() if not currently in a sequence
  XCdrStatus end_parse_sequence();

  /// Begins parsing a struct (struct context).
  ///
  /// @param name Name of the struct (for top-level) or field (for nested)
  /// @return ok() on success, error() if header not parsed
  XCdrStatus begin_parse_struct(std::string_view name);

  /// Begins parsing a struct (array/sequence element context).
  ///
  /// Use inside begin/end_parse_array or begin/end_parse_sequence.
  ///
  /// @return ok() on success, error() if header not parsed
  XCdrStatus begin_parse_struct();

  /// Ends parsing the current struct.
  ///
  /// @return ok() on success, error() if not currently in a struct
  XCdrStatus end_parse_struct();

  /// Finalizes parsing and returns the constructed layout.
  ///
  /// Resets the parser state after successful finalization.
  ///
  /// @return Layout describing the parsed buffer, or error if parsing incomplete
  XCdrResult<XCdrStructLayout> finalize();

  /// Returns the current read offset in the buffer.
  ///
  /// @return Current offset in bytes
  size_t current_offset() const;

  /// Returns the endianness parsed from the header.
  ///
  /// Only valid after parse_header() succeeds.
  ///
  /// @return Endianness of the buffer
  XCdrEndianness endianness() const;

  /// Returns the total buffer size.
  ///
  /// @return Size in bytes
  size_t buffer_size() const;

  /// Returns bytes remaining from current offset to end of buffer.
  ///
  /// @return Remaining bytes
  size_t bytes_remaining() const;

private:
  /// Reads a uint32_t length prefix from the current offset.
  ///
  /// @param[out] length The parsed length value
  /// @return ok() on success, error() on buffer overflow
  XCdrStatus read_length_prefix(uint32_t & length);

  /// Ensures at least `size` bytes are available from current offset.
  ///
  /// @param size Number of bytes required
  /// @return ok() if available, error() otherwise
  XCdrStatus ensure_bytes_available(size_t size);

  /// Advances the read offset by `size` bytes.
  ///
  /// @param size Number of bytes to skip
  void advance(size_t size);

  /// Aligns the read offset to the specified alignment.
  ///
  /// @param alignment Alignment requirement (must be power of 2)
  void align_read_offset(size_t alignment);

  /// Generates a field name if none provided.
  ///
  /// @param name User-provided name (may be empty)
  /// @return Generated name if empty, otherwise original name
  std::string generate_field_name_if_needed(std::string_view name);

  tcb::span<const uint8_t> buffer_;
  XCdrLayoutBuilder builder_;
  size_t read_offset_;
  XCdrEndianness endianness_;
  bool header_parsed_;
  size_t field_counter_;
  int struct_depth_;  // Track nesting depth (0 = top level)
};

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__LAYOUT__LAYOUT_PARSER_HPP_
