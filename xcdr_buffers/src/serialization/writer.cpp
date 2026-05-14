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

#include "xcdr_buffers/serialization/writer.hpp"

#include <cstring>

namespace xcdr_buffers
{

XCdrWriter::XCdrWriter(XCdrEndianness endianness, std::pmr::memory_resource * mr)
: buffer_(mr ? mr : std::pmr::new_delete_resource()),
  endianness_(endianness),
  header_written_(false),
  context_stack_(mr ? mr : std::pmr::new_delete_resource())
{
  buffer_.reserve(256);  // Initial capacity
}

void XCdrWriter::ensure_header_written()
{
  if (!header_written_) {
    buffer_.resize(kXCdrHeaderSize);
    write_xcdr_header(tcb::span<uint8_t>(buffer_.data(), kXCdrHeaderSize), endianness_);
    header_written_ = true;
  }
}

void XCdrWriter::align_and_reserve(size_t alignment, size_t size)
{
  size_t current_pos = buffer_.size();

  // CDR alignment is relative to the data start position (after header)
  size_t data_offset = current_pos - kXCdrHeaderSize;
  size_t aligned_data_offset = align_to(data_offset, alignment);
  size_t aligned_pos = kXCdrHeaderSize + aligned_data_offset;

  // Reserve space for padding + data
  buffer_.resize(aligned_pos + size, 0);  // Zero-fill padding
}

void XCdrWriter::write(std::string_view str)
{
  ensure_header_written();

  // Align to prefix size
  align_and_reserve(kStringLengthPrefixSize, kStringLengthPrefixSize);

  // Write length prefix (includes null terminator)
  uint32_t length = static_cast<uint32_t>(str.size() + kStringNullTerminatorSize);
  size_t length_pos = buffer_.size() - kStringLengthPrefixSize;
  tcb::span<uint8_t> length_dst(buffer_.data() + length_pos, kStringLengthPrefixSize);
  write_to_bytes(length_dst, length, endianness_);

  // Write string data + null terminator (no additional alignment needed)
  buffer_.insert(buffer_.end(), str.begin(), str.end());
  buffer_.push_back('\0');
}

void XCdrWriter::write(std::u16string_view str)
{
  ensure_header_written();

  // Align to prefix size
  align_and_reserve(kStringLengthPrefixSize, kStringLengthPrefixSize);

  // Write length prefix (in bytes, includes null terminator)
  uint32_t length_bytes = static_cast<uint32_t>((str.size() * sizeof(char16_t)) +
    kWStringNullTerminatorSize);
  size_t length_pos = buffer_.size() - kStringLengthPrefixSize;
  tcb::span<uint8_t> length_dst(buffer_.data() + length_pos, kStringLengthPrefixSize);
  write_to_bytes(length_dst, length_bytes, endianness_);

  // Write wstring data (handling endianness for each char16_t)
  for (char16_t ch : str) {
    align_and_reserve(sizeof(char16_t), sizeof(char16_t));
    size_t ch_pos = buffer_.size() - sizeof(char16_t);
    tcb::span<uint8_t> ch_dst(buffer_.data() + ch_pos, sizeof(char16_t));
    write_to_bytes(ch_dst, ch, endianness_);
  }

  // Write null terminator
  align_and_reserve(sizeof(char16_t), sizeof(char16_t));
  size_t term_pos = buffer_.size() - sizeof(char16_t);
  tcb::span<uint8_t> term_dst(buffer_.data() + term_pos, sizeof(char16_t));
  char16_t null_term = u'\0';
  write_to_bytes(term_dst, null_term, endianness_);
}

void XCdrWriter::begin_write_array(size_t count)
{
  ensure_header_written();
  context_stack_.push_back({buffer_.size(), count, 0});
}

void XCdrWriter::end_write_array()
{
  if (context_stack_.empty()) {
    // Error: unmatched end_write_array
    return;
  }

  // Validate element count matches
  // (In a production version, we'd track elements_written and check here)
  context_stack_.pop_back();
}

void XCdrWriter::begin_write_sequence(size_t count)
{
  ensure_header_written();

  // Align to prefix size and write length prefix
  align_and_reserve(kSequenceLengthPrefixSize, kSequenceLengthPrefixSize);

  uint32_t length = static_cast<uint32_t>(count);
  size_t length_pos = buffer_.size() - kSequenceLengthPrefixSize;
  tcb::span<uint8_t> length_dst(buffer_.data() + length_pos, kSequenceLengthPrefixSize);
  write_to_bytes(length_dst, length, endianness_);

  context_stack_.push_back({buffer_.size(), count, 0});
}

void XCdrWriter::end_write_sequence()
{
  if (context_stack_.empty()) {
    // Error: unmatched end_write_sequence
    return;
  }

  context_stack_.pop_back();
}

void XCdrWriter::begin_write_struct()
{
  ensure_header_written();
  context_stack_.push_back({buffer_.size(), 0, 0});
}

void XCdrWriter::end_write_struct()
{
  if (context_stack_.empty()) {
    // Error: unmatched end_write_struct
    return;
  }

  context_stack_.pop_back();
}

tcb::span<const uint8_t> XCdrWriter::data() const
{
  return tcb::span<const uint8_t>(buffer_.data(), buffer_.size());
}

std::pmr::vector<uint8_t> XCdrWriter::flush()
{
  std::pmr::vector<uint8_t> result = std::move(buffer_);
  reset(endianness_);
  return result;
}

void XCdrWriter::reset(XCdrEndianness endianness)
{
  buffer_.clear();
  endianness_ = endianness;
  header_written_ = false;
  context_stack_.clear();
}

}  // namespace xcdr_buffers
