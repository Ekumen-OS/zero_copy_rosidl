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

#include "xcdr_buffers/serialization/reader.hpp"

namespace xcdr_buffers
{

// Public constructor - throws on invalid header
XCdrReader::XCdrReader(tcb::span<const uint8_t> buffer, std::pmr::memory_resource * mr)
: buffer_(buffer),
  endianness_(XCdrEndianness::kLittleEndian),
  position_(0),
  data_start_position_(0),
  header_read_(false),
  context_stack_(mr ? mr : std::pmr::new_delete_resource())
{
  // Read header immediately and throw on failure
  auto header_result = read_header();
  if (!header_result) {
    throw XCdrError(header_result.error().message());
  }
}

// Private constructor for factory function - no automatic header read
XCdrReader::XCdrReader(
  tcb::span<const uint8_t> buffer,
  std::pmr::memory_resource * mr,
  bool /* unused tag */)
: buffer_(buffer),
  endianness_(XCdrEndianness::kLittleEndian),
  position_(0),
  data_start_position_(0),
  header_read_(false),
  context_stack_(mr ? mr : std::pmr::new_delete_resource())
{
  // No automatic header read - caller will handle it
}

// Static factory function for explicit error handling
XCdrResult<XCdrReader> XCdrReader::wrap(
  tcb::span<const uint8_t> buffer,
  std::pmr::memory_resource * mr)
{
  // Use private constructor (tag dispatch with unused bool)
  XCdrReader reader(buffer, mr, false);

  // Try to read header
  auto header_result = reader.read_header();
  if (!header_result) {
    return tl::unexpected(header_result.error());
  }

  return ok(std::move(reader));
}

XCdrResult<XCdrEndianness> XCdrReader::read_header()
{
  if (header_read_) {
    return ok(endianness_);  // Return cached endianness if already read
  }

  if (buffer_.size() < kXCdrHeaderSize) {
    return error("Buffer too small for XCDR header");
  }

  auto endianness_result = read_xcdr_header(buffer_);
  if (!endianness_result) {
    return endianness_result;
  }

  endianness_ = *endianness_result;
  position_ = kXCdrHeaderSize;
  data_start_position_ = kXCdrHeaderSize;  // Data starts after header
  header_read_ = true;

  return ok(endianness_);
}

XCdrStatus XCdrReader::begin_read_array(size_t expected_count)
{
  if (!header_read_) {
    return error("Header must be read before reading data");
  }

  context_stack_.push_back({position_, expected_count, 0});
  return ok();
}

XCdrStatus XCdrReader::end_read_array()
{
  if (context_stack_.empty()) {
    return error("Unmatched end_read_array");
  }

  context_stack_.pop_back();
  return ok();
}

XCdrResult<size_t> XCdrReader::begin_read_sequence()
{
  if (!header_read_) {
    return error("Header must be read before reading data");
  }

  // Align to prefix size
  auto status = align_position(kSequenceLengthPrefixSize);
  if (!status) {
    return tl::unexpected(status.error());
  }

  // Read length prefix
  status = ensure_available(kSequenceLengthPrefixSize);
  if (!status) {
    return tl::unexpected(status.error());
  }

  uint32_t count;
  tcb::span<const uint8_t> count_src(buffer_.data() + position_, kSequenceLengthPrefixSize);
  read_from_bytes(&count, count_src, endianness_);
  position_ += kSequenceLengthPrefixSize;

  context_stack_.push_back({position_, count, 0});
  return ok(static_cast<size_t>(count));
}

XCdrStatus XCdrReader::end_read_sequence()
{
  if (context_stack_.empty()) {
    return error("Unmatched end_read_sequence");
  }

  context_stack_.pop_back();
  return ok();
}

XCdrStatus XCdrReader::begin_read_struct()
{
  if (!header_read_) {
    return error("Header must be read before reading data");
  }

  context_stack_.push_back({position_, 0, 0});
  return ok();
}

XCdrStatus XCdrReader::end_read_struct()
{
  if (context_stack_.empty()) {
    return error("Unmatched end_read_struct");
  }

  context_stack_.pop_back();
  return ok();
}

void XCdrReader::reset(tcb::span<const uint8_t> buffer)
{
  buffer_ = buffer;
  position_ = 0;
  data_start_position_ = 0;
  header_read_ = false;
  context_stack_.clear();

  // Automatically read new header
  auto header_result = read_header();
  if (!header_result) {
    throw XCdrError(header_result.error().message());
  }
}

size_t XCdrReader::bytes_remaining() const
{
  if (position_ >= buffer_.size()) {
    return 0;
  }
  return buffer_.size() - position_;
}

XCdrStatus XCdrReader::align_position(size_t alignment)
{
  // CDR alignment is relative to the data start position (after header)
  size_t relative_pos = position_ - data_start_position_;
  size_t aligned_relative_pos = align_to(relative_pos, alignment);
  size_t aligned_pos = data_start_position_ + aligned_relative_pos;

  if (aligned_pos > buffer_.size()) {
    return error("Alignment would exceed buffer size");
  }
  position_ = aligned_pos;
  return ok();
}

XCdrStatus XCdrReader::ensure_available(size_t size)
{
  if (position_ + size > buffer_.size()) {
    return error("Insufficient data in buffer");
  }
  return ok();
}

}  // namespace xcdr_buffers
