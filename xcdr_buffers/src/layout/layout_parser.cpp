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

#include "xcdr_buffers/layout/layout_parser.hpp"

#include <algorithm>
#include <cstring>

namespace xcdr_buffers
{

XCdrResult<XCdrLayoutParser> XCdrLayoutParser::create(
  tcb::span<const uint8_t> buffer,
  std::pmr::memory_resource * mr)
{
  // Validate buffer size
  if (buffer.size() < kXCdrHeaderSize) {
    return error("Buffer too small for XCDR header");
  }

  // Validate header
  auto header_result = read_xcdr_header(buffer);
  if (!header_result) {
    return tl::unexpected(header_result.error());
  }

  // Construct parser (may still throw, but we've pre-validated)
  try {
    return ok(XCdrLayoutParser(buffer, mr));
  } catch (const XCdrError & e) {
    return error(e.message());
  }
}

XCdrLayoutParser::XCdrLayoutParser(
  tcb::span<const uint8_t> buffer,
  std::pmr::memory_resource * mr)
: buffer_(buffer),
  builder_(XCdrEndianness::kLittleEndian, mr),
  read_offset_(0),
  endianness_(XCdrEndianness::kLittleEndian),
  header_parsed_(false),
  field_counter_(0),
  struct_depth_(1)  // Start at depth 1 for implicit top-level struct
{
  // Parse header automatically
  if (buffer_.size() < kXCdrHeaderSize) {
    throw XCdrError("Buffer too small for XCDR header");
  }

  auto result = read_xcdr_header(buffer_);
  if (!result) {
    throw XCdrError(result.error().message());
  }

  endianness_ = *result;
  read_offset_ = kXCdrHeaderSize;
  header_parsed_ = true;

  // Initialize builder with the same endianness
  builder_ = XCdrLayoutBuilder(*result, mr);

  // Parser starts at top level (depth=1), ready to parse struct fields directly
  // No implicit struct context in builder, but struct_depth_=1 enables auto-naming
}

XCdrStatus XCdrLayoutParser::parse_primitive(std::string_view name, XCdrPrimitiveKind kind)
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing primitives");
  }

  const size_t size = get_primitive_size(kind);
  const size_t alignment = get_primitive_alignment(kind);

  // Align read offset
  align_read_offset(alignment);

  // Ensure bytes available
  auto status = ensure_bytes_available(size);
  if (!status) {
    return status;
  }

  // Generate name if needed
  std::string field_name = generate_field_name_if_needed(name);

  // Tell builder to allocate this primitive
  builder_.allocate_primitive(field_name, kind);

  // Advance read offset
  advance(size);

  return ok();
}

XCdrStatus XCdrLayoutParser::parse_string(std::string_view name, XCdrCharKind char_kind)
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing strings");
  }

  // Align to prefix size
  align_read_offset(kStringLengthPrefixSize);

  // Read length prefix
  uint32_t wire_length = 0;
  auto status = read_length_prefix(wire_length);
  if (!status) {
    return status;
  }

  if (wire_length == 0) {
    return error("Invalid string length: 0");
  }

  // Ensure string data is available
  status = ensure_bytes_available(wire_length);
  if (!status) {
    return status;
  }

  // Wire length includes null terminator
  // For char8: wire_length is in bytes, includes 1-byte terminator
  // For char16: wire_length is in bytes, includes 2-byte terminator
  const size_t char_size = get_char_size(char_kind);
  const size_t terminator_size = (char_kind == XCdrCharKind::kChar8) ?
    kStringNullTerminatorSize :
    kWStringNullTerminatorSize;

  if (wire_length < terminator_size) {
    return error("Invalid string length: too small for terminator");
  }

  const size_t string_length_bytes = wire_length - terminator_size;
  if (string_length_bytes % char_size != 0) {
    return error("Invalid string length: not aligned to character size");
  }

  const size_t string_length_chars = string_length_bytes / char_size;

  // Generate name if needed
  std::string field_name = generate_field_name_if_needed(name);

  // Tell builder to allocate this string
  builder_.allocate_string(field_name, string_length_chars, char_kind);

  // Advance past the string data
  advance(wire_length);

  return ok();
}

// Overloads for array/sequence element context (no names)

XCdrStatus XCdrLayoutParser::parse_primitive(XCdrPrimitiveKind kind)
{
  return parse_primitive("", kind);
}

XCdrStatus XCdrLayoutParser::parse_string(XCdrCharKind char_kind)
{
  return parse_string("", char_kind);
}

XCdrStatus XCdrLayoutParser::parse_primitive_array(
  std::string_view name, XCdrPrimitiveKind kind,
  size_t count)
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing arrays");
  }

  // Validate name is not empty when explicitly provided
  if (name.empty()) {
    return error(
        "Array name must not be empty. Use no-argument overload for auto-generated names");
  }

  // Use builder's one-shot method
  builder_.allocate_primitive_array(std::string(name), kind, count);

  // Advance read offset
  size_t element_size = get_primitive_size(kind);
  align_read_offset(element_size);
  read_offset_ += element_size * count;

  return ok();
}

XCdrStatus XCdrLayoutParser::parse_primitive_array(XCdrPrimitiveKind kind, size_t count)
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing arrays");
  }

  // Auto-generate field name
  std::string field_name = "field_" + std::to_string(field_counter_++);

  // Use builder's one-shot method
  builder_.allocate_primitive_array(field_name, kind, count);

  // Advance read offset
  size_t element_size = get_primitive_size(kind);
  align_read_offset(element_size);
  read_offset_ += element_size * count;

  return ok();
}

XCdrStatus XCdrLayoutParser::parse_primitive_sequence(
  std::string_view name, XCdrPrimitiveKind kind,
  size_t actual_count)
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing sequences");
  }

  // Validate name is not empty when explicitly provided
  if (name.empty()) {
    return error(
        "Sequence name must not be empty. Use no-argument overload for auto-generated names");
  }

  // Align to prefix size
  align_read_offset(kSequenceLengthPrefixSize);

  // Read sequence count
  if (read_offset_ + kSequenceLengthPrefixSize > buffer_.size()) {
    return error("Buffer too small for sequence length prefix");
  }

  uint32_t sequence_count;
  read_from_bytes(&sequence_count,
                  tcb::span<const uint8_t>(buffer_.data() + read_offset_,
      kSequenceLengthPrefixSize),
                  endianness_);
  read_offset_ += kSequenceLengthPrefixSize;

  // Validate count matches
  if (sequence_count != actual_count) {
    return error("Sequence count mismatch: expected " + std::to_string(actual_count) +
                 ", got " + std::to_string(sequence_count));
  }

  // Use builder's one-shot method
  builder_.allocate_primitive_sequence(std::string(name), kind, actual_count);

  // Align for first element, then advance read offset for all element data
  size_t element_size = get_primitive_size(kind);
  align_read_offset(element_size);
  read_offset_ += element_size * actual_count;

  return ok();
}

XCdrStatus XCdrLayoutParser::parse_primitive_sequence(
  XCdrPrimitiveKind kind,
  size_t actual_count)
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing sequences");
  }

  // Auto-generate field name
  std::string field_name = "field_" + std::to_string(field_counter_++);

  // Align to prefix size
  align_read_offset(kSequenceLengthPrefixSize);

  // Read sequence count
  if (read_offset_ + kSequenceLengthPrefixSize > buffer_.size()) {
    return error("Buffer too small for sequence length prefix");
  }

  uint32_t sequence_count;
  read_from_bytes(&sequence_count,
                  tcb::span<const uint8_t>(buffer_.data() + read_offset_,
      kSequenceLengthPrefixSize),
                  endianness_);
  read_offset_ += kSequenceLengthPrefixSize;

  // Validate count matches
  if (sequence_count != actual_count) {
    return error("Sequence count mismatch: expected " + std::to_string(actual_count) +
                 ", got " + std::to_string(sequence_count));
  }

  // Use builder's one-shot method
  builder_.allocate_primitive_sequence(field_name, kind, actual_count);

  // Align for first element, then advance read offset for all element data
  size_t element_size = get_primitive_size(kind);
  align_read_offset(element_size);
  read_offset_ += element_size * actual_count;

  return ok();
}

XCdrStatus XCdrLayoutParser::begin_parse_array(std::string_view name, size_t count)
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing arrays");
  }

  // Validate name is not empty when explicitly provided
  if (name.empty()) {
    return error("Array name must not be empty. Use no-argument overload for auto-generated names");
  }

  // Use provided name directly (no auto-generation)
  builder_.begin_allocate_array(name, count);
  return ok();
}

XCdrStatus XCdrLayoutParser::begin_parse_array(size_t count)
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing arrays");
  }

  // Auto-generate name (no-name version)
  builder_.begin_allocate_array(count);
  return ok();
}

XCdrStatus XCdrLayoutParser::end_parse_array()
{
  builder_.end_allocate_array();
  return ok();
}

XCdrResult<size_t> XCdrLayoutParser::begin_parse_sequence(std::string_view name)
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing sequences");
  }

  // Validate name is not empty when explicitly provided
  if (name.empty()) {
    return error(
        "Sequence name must not be empty. Use no-argument overload for auto-generated names");
  }

  // Align to prefix size
  align_read_offset(kSequenceLengthPrefixSize);

  // Read sequence count
  uint32_t count = 0;
  auto status = read_length_prefix(count);
  if (!status) {
    return tl::unexpected(status.error());
  }

  // Use provided name directly (no auto-generation)
  builder_.begin_allocate_sequence(name, count);
  return ok(static_cast<size_t>(count));
}

XCdrStatus XCdrLayoutParser::end_parse_sequence()
{
  builder_.end_allocate_sequence();
  return ok();
}

XCdrResult<size_t> XCdrLayoutParser::begin_parse_sequence()
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing sequences");
  }

  // Align to prefix size
  align_read_offset(kSequenceLengthPrefixSize);

  // Read sequence count
  uint32_t count = 0;
  auto status = read_length_prefix(count);
  if (!status) {
    return tl::unexpected(status.error());
  }

  // Auto-generate name (no-name version)
  builder_.begin_allocate_sequence(count);
  return ok(static_cast<size_t>(count));
}

XCdrStatus XCdrLayoutParser::begin_parse_struct(std::string_view name)
{
  if (!header_parsed_) {
    return error("Must call parse_header() before parsing structs");
  }

  // Always call builder to create a struct context
  builder_.begin_allocate_struct(name);
  struct_depth_++;

  return ok();
}

XCdrStatus XCdrLayoutParser::end_parse_struct()
{
  if (struct_depth_ <= 1) {
    return error("Unmatched end_parse_struct");
  }

  struct_depth_--;
  builder_.end_allocate_struct();

  return ok();
}

XCdrStatus XCdrLayoutParser::begin_parse_struct()
{
  return begin_parse_struct("");  // Name unused in array/sequence context
}

XCdrResult<XCdrStructLayout> XCdrLayoutParser::finalize()
{
  // Finalize the builder to get the top-level struct layout
  auto layout = builder_.finalize();

  // Reset parser state
  read_offset_ = 0;
  header_parsed_ = false;
  field_counter_ = 0;
  struct_depth_ = 0;

  return ok(std::move(layout));
}

size_t XCdrLayoutParser::current_offset() const
{
  return read_offset_;
}

XCdrEndianness XCdrLayoutParser::endianness() const
{
  return endianness_;
}

size_t XCdrLayoutParser::buffer_size() const
{
  return buffer_.size();
}

size_t XCdrLayoutParser::bytes_remaining() const
{
  if (read_offset_ >= buffer_.size()) {
    return 0;
  }
  return buffer_.size() - read_offset_;
}

XCdrStatus XCdrLayoutParser::read_length_prefix(uint32_t & length)
{
  auto status = ensure_bytes_available(sizeof(uint32_t));
  if (!status) {
    return status;
  }

  auto src_span = buffer_.subspan(read_offset_, sizeof(uint32_t));
  read_from_bytes(&length, src_span, endianness_);
  advance(sizeof(uint32_t));

  return ok();
}

XCdrStatus XCdrLayoutParser::ensure_bytes_available(size_t size)
{
  if (bytes_remaining() < size) {
    return error(
      "Buffer overflow: need " + std::to_string(size) +
      " bytes, have " + std::to_string(bytes_remaining()));
  }
  return ok();
}

void XCdrLayoutParser::advance(size_t size)
{
  read_offset_ += size;
}

void XCdrLayoutParser::align_read_offset(size_t alignment)
{
  // CDR alignment is relative to the data start position (after header)
  size_t relative_offset = read_offset_ - kXCdrHeaderSize;
  size_t aligned_relative_offset = align_to(relative_offset, alignment);
  read_offset_ = kXCdrHeaderSize + aligned_relative_offset;
}

std::string XCdrLayoutParser::generate_field_name_if_needed(std::string_view name)
{
  if (!name.empty()) {
    return std::string(name);
  }

  // Generate name only if we're in a struct context (depth > 0)
  if (struct_depth_ > 0) {
    return "field_" + std::to_string(field_counter_++);
  }

  return "";
}

}  // namespace xcdr_buffers
