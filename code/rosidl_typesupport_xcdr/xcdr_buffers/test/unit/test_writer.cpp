// Copyright 2026 xcdr_buffers Contributors
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

#include <gtest/gtest.h>

#include "xcdr_buffers/serialization/writer.hpp"
#include "xcdr_buffers/common/endianness.hpp"

using namespace xcdr_buffers;  // NOLINT(build/namespaces)

TEST(XCdrWriter, WritePrimitiveTypes) {
  XCdrWriter writer;

  writer.write<uint8_t>(42);
  writer.write<uint16_t>(1234);
  writer.write<uint32_t>(0xDEADBEEF);
  writer.write<uint64_t>(0x0102030405060708ULL);
  writer.write<float>(3.14f);
  writer.write<double>(2.71828);

  auto buffer = writer.flush();

  // Check header
  ASSERT_GE(buffer.size(), 4);
  EXPECT_EQ(buffer[0], 0x00);
  EXPECT_EQ(buffer[1], 0x01);  // Little endian
  EXPECT_EQ(buffer[2], 0x00);
  EXPECT_EQ(buffer[3], 0x00);

  // Check that data was written (total size should be header + aligned data)
  // With CDR data-relative alignment (relative to position after 4-byte header):
  // Header: 4 bytes
  // uint8_t: 1 byte at data offset 0 → data offset 1
  // uint16_t: align to 2 (1 byte padding) + 2 bytes → data offset 4
  // uint32_t: align to 4 (no padding) + 4 bytes → data offset 8
  // uint64_t: align to 8 (no padding) + 8 bytes → data offset 16
  // float: align to 4 (no padding) + 4 bytes → data offset 20
  // double: align to 8 (4 bytes padding) + 8 bytes → data offset 32
  // Total: 4 (header) + 32 (data) = 36 bytes
  EXPECT_EQ(buffer.size(), 36);
}

TEST(XCdrWriter, WriteString) {
  XCdrWriter writer;

  writer.write(std::string_view("Hello, world!"));

  auto buffer = writer.flush();

  // Check header (4 bytes)
  ASSERT_GE(buffer.size(), 4);

  // After header, should have:
  // - 4-byte length prefix (14, includes null terminator)
  // - 13 bytes of string data
  // - 1 byte null terminator
  // Total: 4 + 4 + 14 = 22 bytes
  EXPECT_EQ(buffer.size(), 22);

  // Check length prefix (at position 4, aligned to 4)
  uint32_t length;
  tcb::span<const uint8_t> length_span(buffer.data() + 4, 4);
  read_from_bytes(&length, length_span, XCdrEndianness::kLittleEndian);
  EXPECT_EQ(length, 14);  // 13 chars + null terminator

  // Check string content
  std::string_view str(reinterpret_cast<const char *>(buffer.data() + 8), 13);
  EXPECT_EQ(str, "Hello, world!");

  // Check null terminator
  EXPECT_EQ(buffer[21], '\0');
}

TEST(XCdrWriter, WriteEmptyString) {
  XCdrWriter writer;

  writer.write(std::string_view(""));

  auto buffer = writer.flush();

  // Header: 4 bytes
  // Length prefix: 4 bytes (value = 1, just null terminator)
  // Null terminator: 1 byte
  // Total: 9 bytes
  EXPECT_EQ(buffer.size(), 9);

  // Check length prefix
  uint32_t length;
  tcb::span<const uint8_t> length_span(buffer.data() + 4, 4);
  read_from_bytes(&length, length_span, XCdrEndianness::kLittleEndian);
  EXPECT_EQ(length, 1);

  // Check null terminator
  EXPECT_EQ(buffer[8], '\0');
}

TEST(XCdrWriter, WriteSequence) {
  XCdrWriter writer;

  std::vector<uint32_t> values = {1, 2, 3, 4, 5};
  writer.write_sequence(tcb::span<const uint32_t>(values.data(), values.size()));

  auto buffer = writer.flush();

  // Header: 4 bytes
  // Sequence length prefix: 4 bytes (value = 5)
  // 5 uint32_t values: 5 * 4 = 20 bytes
  // Total: 4 + 4 + 20 = 28 bytes
  EXPECT_EQ(buffer.size(), 28);

  // Check sequence length
  uint32_t count;
  tcb::span<const uint8_t> count_span(buffer.data() + 4, 4);
  read_from_bytes(&count, count_span, XCdrEndianness::kLittleEndian);
  EXPECT_EQ(count, 5);

  // Check values
  for (size_t i = 0; i < 5; ++i) {
    uint32_t value;
    tcb::span<const uint8_t> value_span(buffer.data() + 8 + i * 4, 4);
    read_from_bytes(&value, value_span, XCdrEndianness::kLittleEndian);
    EXPECT_EQ(value, values[i]);
  }
}

TEST(XCdrWriter, WriteEmptySequence) {
  XCdrWriter writer;

  std::vector<uint32_t> values;
  writer.write_sequence(tcb::span<const uint32_t>(values.data(), values.size()));

  auto buffer = writer.flush();

  // Header: 4 bytes
  // Sequence length prefix: 4 bytes (value = 0)
  // No data
  // Total: 8 bytes
  EXPECT_EQ(buffer.size(), 8);

  // Check sequence length
  uint32_t count;
  tcb::span<const uint8_t> count_span(buffer.data() + 4, 4);
  read_from_bytes(&count, count_span, XCdrEndianness::kLittleEndian);
  EXPECT_EQ(count, 0);
}

TEST(XCdrWriter, WriteArray) {
  XCdrWriter writer;

  std::vector<uint16_t> values = {10, 20, 30};
  writer.write_array(tcb::span<const uint16_t>(values.data(), values.size()));

  auto buffer = writer.flush();

  // Header: 4 bytes
  // 3 uint16_t values, aligned to 2: +0 padding + 6 bytes
  // Total: 4 + 6 = 10 bytes
  EXPECT_EQ(buffer.size(), 10);

  // Check values (arrays have no length prefix)
  for (size_t i = 0; i < 3; ++i) {
    uint16_t value;
    tcb::span<const uint8_t> value_span(buffer.data() + 4 + i * 2, 2);
    read_from_bytes(&value, value_span, XCdrEndianness::kLittleEndian);
    EXPECT_EQ(value, values[i]);
  }
}

TEST(XCdrWriter, WriteStruct) {
  XCdrWriter writer;

  writer.begin_write_struct();
  writer.write<uint32_t>(100);
  writer.write(std::string_view("test"));
  writer.write<double>(1.5);
  writer.end_write_struct();

  auto buffer = writer.flush();

  // With CDR data-relative alignment:
  // Header: 4 bytes
  // uint32_t: align to 4 at data offset 0 (no padding) + 4 bytes → data offset 4
  // string: align to 4 at data offset 4 (no padding) + 4 (length) + 5 (data+null) → data offset 13
  // double: align to 8 at data offset 13 (3 bytes padding) + 8 bytes → data offset 24
  // Total: 4 (header) + 24 (data) = 28 bytes
  EXPECT_EQ(buffer.size(), 28);
}

TEST(XCdrWriter, FlushResetsState) {
  XCdrWriter writer;

  writer.write<uint32_t>(42);
  auto buffer1 = writer.flush();

  EXPECT_EQ(buffer1.size(), 8);  // Header + uint32_t

  // Write again
  writer.write<uint32_t>(99);
  auto buffer2 = writer.flush();

  EXPECT_EQ(buffer2.size(), 8);  // Should be same size (fresh write)

  // Buffers should be different
  uint32_t value1, value2;
  read_from_bytes(&value1, tcb::span<const uint8_t>(buffer1.data() + 4, 4),
    XCdrEndianness::kLittleEndian);
  read_from_bytes(&value2, tcb::span<const uint8_t>(buffer2.data() + 4, 4),
    XCdrEndianness::kLittleEndian);

  EXPECT_EQ(value1, 42);
  EXPECT_EQ(value2, 99);
}

TEST(XCdrWriter, WriteBigEndian) {
  XCdrWriter writer(XCdrEndianness::kBigEndian);

  writer.write<uint32_t>(0x01020304);

  auto buffer = writer.flush();

  // Check header
  EXPECT_EQ(buffer[1], 0x00);  // Big endian flag

  // Check value (should be in big endian byte order)
  EXPECT_EQ(buffer[4], 0x01);
  EXPECT_EQ(buffer[5], 0x02);
  EXPECT_EQ(buffer[6], 0x03);
  EXPECT_EQ(buffer[7], 0x04);
}

/// Helper: write a valid LE XCDR header at [0..3].
static void write_header(std::vector<uint8_t> & buf)
{
  ASSERT_GE(buf.size(), 4);
  buf[0] = 0x00;
  buf[1] = 0x01;  // little endian
  buf[2] = 0x00;
  buf[3] = 0x00;
}

// ============================================================================
// Skip API tests
//
// Verify that skip advances position identically to write, without mutating
// the buffer, and with identical overflow semantics.
// ============================================================================

/// Helper: snapshot buffer bytes after a write/skip sequence.
static std::vector<uint8_t> snapshot_bytes(const XCdrWriter & w)
{
  auto span = w.data();
  return std::vector<uint8_t>(span.begin(), span.end());
}

TEST(XCdrWriter_Skip, PrimitiveSkip_WrittenSizeMatch)
{
  // Write path — produce baseline bytes + size.
  std::vector<uint8_t> buf_write(64, 0);
  write_header(buf_write);
  XCdrWriter w_writer(tcb::span<uint8_t>(buf_write.data(), buf_write.size()), 4);
  w_writer.write<uint8_t>(42);
  w_writer.write<uint16_t>(0xAABB);
  w_writer.write<uint32_t>(0xDEADBEEF);
  size_t written_size = w_writer.bytes_written();
  auto written_data = snapshot_bytes(w_writer);

  // Skip path — same types, no values, same buffer init.
  std::vector<uint8_t> buf_skip(64, 0);
  write_header(buf_skip);
  XCdrWriter s_writer(tcb::span<uint8_t>(buf_skip.data(), buf_skip.size()), 4);
  s_writer.skip<uint8_t>();
  s_writer.skip<uint16_t>();
  s_writer.skip<uint32_t>();
  size_t skip_size = s_writer.bytes_written();
  auto skip_data = snapshot_bytes(s_writer);

  // Sizes must match.
  EXPECT_EQ(skip_size, written_size);

  // Buffer must NOT have been mutated — only header bytes should differ
  // (header is pre-written, skip path wrote nothing).
  // Header at [0..3] is identical in both.
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(skip_data[i], written_data[i]);
  }
  // After header, written_data has real bytes; skip_data should still be zero.
  for (size_t i = 4; i < skip_data.size(); ++i) {
    EXPECT_EQ(skip_data[i], 0) << "skip path mutated byte at offset " << i;
  }
}

TEST(XCdrWriter_Skip, SkipString_SizeParity)
{
  std::vector<uint8_t> buf_write(64, 0);
  write_header(buf_write);
  XCdrWriter w_writer(tcb::span<uint8_t>(buf_write.data(), buf_write.size()), 4);
  w_writer.write(std::string_view("Hello, world!"));
  size_t written_size = w_writer.bytes_written();

  std::vector<uint8_t> buf_skip(64, 0);
  write_header(buf_skip);
  XCdrWriter s_writer(tcb::span<uint8_t>(buf_skip.data(), buf_skip.size()), 4);
  s_writer.skip_string(13);  // "Hello, world!" is 13 chars
  EXPECT_FALSE(s_writer.has_error());
  size_t skip_size = s_writer.bytes_written();

  EXPECT_EQ(skip_size, written_size);

  // Verify buffer not mutated.
  for (size_t i = 4; i < skip_size; ++i) {
    EXPECT_EQ(buf_skip[i], 0) << "skip_string mutated byte at offset " << i;
  }
}

TEST(XCdrWriter_Skip, SkipEmptyString_SizeParity)
{
  std::vector<uint8_t> buf_write(64, 0);
  write_header(buf_write);
  XCdrWriter w_writer(tcb::span<uint8_t>(buf_write.data(), buf_write.size()), 4);
  w_writer.write(std::string_view(""));
  size_t written_size = w_writer.bytes_written();

  std::vector<uint8_t> buf_skip(64, 0);
  write_header(buf_skip);
  XCdrWriter s_writer(tcb::span<uint8_t>(buf_skip.data(), buf_skip.size()), 4);
  s_writer.skip_string(0);
  EXPECT_FALSE(s_writer.has_error());
  size_t skip_size = s_writer.bytes_written();

  EXPECT_EQ(skip_size, written_size);
}

TEST(XCdrWriter_Skip, SkipWString_SizeParity)
{
  std::vector<uint8_t> buf_write(64, 0);
  write_header(buf_write);
  XCdrWriter w_writer(tcb::span<uint8_t>(buf_write.data(), buf_write.size()), 4);
  w_writer.write(std::u16string_view(u"hi"));
  size_t written_size = w_writer.bytes_written();

  std::vector<uint8_t> buf_skip(64, 0);
  write_header(buf_skip);
  XCdrWriter s_writer(tcb::span<uint8_t>(buf_skip.data(), buf_skip.size()), 4);
  s_writer.skip_wstring(2);
  EXPECT_FALSE(s_writer.has_error());
  size_t skip_size = s_writer.bytes_written();

  EXPECT_EQ(skip_size, written_size);
}

TEST(XCdrWriter_Skip, SequencePrefix_SizeParity)
{
  std::vector<uint8_t> buf_write(64, 0);
  write_header(buf_write);
  XCdrWriter w_writer(tcb::span<uint8_t>(buf_write.data(), buf_write.size()), 4);
  std::vector<uint32_t> vals = {1, 2, 3, 4, 5};
  w_writer.write_sequence(tcb::span<const uint32_t>(vals.data(), vals.size()));
  size_t written_size = w_writer.bytes_written();

  // Skip: advance prefix + per-element primitives.
  std::vector<uint8_t> buf_skip(64, 0);
  write_header(buf_skip);
  XCdrWriter s_writer(tcb::span<uint8_t>(buf_skip.data(), buf_skip.size()), 4);
  s_writer.begin_skip_sequence(5);
  for (int i = 0; i < 5; ++i) {
    s_writer.skip<uint32_t>();
  }
  s_writer.end_write_sequence();
  EXPECT_FALSE(s_writer.has_error());
  size_t skip_size = s_writer.bytes_written();

  EXPECT_EQ(skip_size, written_size);
}

TEST(XCdrWriter_Skip, EmptySequencePrefix_SizeParity)
{
  std::vector<uint8_t> buf_write(64, 0);
  write_header(buf_write);
  XCdrWriter w_writer(tcb::span<uint8_t>(buf_write.data(), buf_write.size()), 4);
  std::vector<uint32_t> vals;
  w_writer.write_sequence(tcb::span<const uint32_t>(vals.data(), vals.size()));
  size_t written_size = w_writer.bytes_written();

  std::vector<uint8_t> buf_skip(64, 0);
  write_header(buf_skip);
  XCdrWriter s_writer(tcb::span<uint8_t>(buf_skip.data(), buf_skip.size()), 4);
  s_writer.begin_skip_sequence(0);
  s_writer.end_write_sequence();
  EXPECT_FALSE(s_writer.has_error());
  size_t skip_size = s_writer.bytes_written();

  EXPECT_EQ(skip_size, written_size);
}

TEST(XCdrWriter_Skip, SkipArray_SizeParity)
{
  // skip_array<uint32_t>(5) should advance same as 5x skip<uint32_t>.
  std::vector<uint8_t> buf_skip1(64, 0);
  write_header(buf_skip1);
  XCdrWriter w1(tcb::span<uint8_t>(buf_skip1.data(), buf_skip1.size()), 4);
  for (int i = 0; i < 5; ++i) {
    w1.skip<uint32_t>();
                                                     }
  size_t per_element_size = w1.bytes_written();

  std::vector<uint8_t> buf_skip2(64, 0);
  write_header(buf_skip2);
  XCdrWriter w2(tcb::span<uint8_t>(buf_skip2.data(), buf_skip2.size()), 4);
  w2.skip_array<uint32_t>(5);
  EXPECT_FALSE(w2.has_error());
  EXPECT_EQ(w2.bytes_written(), per_element_size);
}

TEST(XCdrWriter_Skip, SkipArray_ZeroCount)
{
  // Zero-count must not advance or align.
  std::vector<uint8_t> buf(64, 0);
  write_header(buf);
  XCdrWriter w(tcb::span<uint8_t>(buf.data(), buf.size()), 4);
  EXPECT_EQ(w.bytes_written(), 4u);
  w.skip_array<uint32_t>(0);
  EXPECT_FALSE(w.has_error());
  EXPECT_EQ(w.bytes_written(), 4u);  // no advance
}

TEST(XCdrWriter_Skip, SkipArray_Overflow)
{
  std::vector<uint8_t> buf(8, 0);
  write_header(buf);  // [0..3], leaves 4 bytes
  XCdrWriter w(tcb::span<uint8_t>(buf.data(), buf.size()), 4);
  w.skip_array<uint8_t>(4);  // fits exactly
  EXPECT_FALSE(w.has_error());
  w.skip_array<uint8_t>(1);  // one byte past end
  EXPECT_TRUE(w.has_error());
}

TEST(XCdrWriter_Skip, SkipArray_NonAlignedPreState)
{
  // Skip a uint8_t first (creating odd offset), then skip_array<uint32_t>.
  // Must align uint32_t correctly (same as per-element loop).
  std::vector<uint8_t> buf_loop(64, 0);
  write_header(buf_loop);
  XCdrWriter w_loop(tcb::span<uint8_t>(buf_loop.data(), buf_loop.size()), 4);
  w_loop.skip<uint8_t>();
  for (int i = 0; i < 3; ++i) {
    w_loop.skip<uint32_t>();
                                                         }
  size_t loop_size = w_loop.bytes_written();

  // Also test the alignment of the first skip alone: a loop skip after uint8
  std::vector<uint8_t> buf_bulk(64, 0);
  write_header(buf_bulk);
  XCdrWriter w_bulk(tcb::span<uint8_t>(buf_bulk.data(), buf_bulk.size()), 4);
  w_bulk.skip<uint8_t>();
  w_bulk.skip_array<uint32_t>(3);
  EXPECT_FALSE(w_bulk.has_error());
  EXPECT_EQ(w_bulk.bytes_written(), loop_size);
}

TEST(XCdrWriter_Skip, SkipSequence_SizeParity)
{
  // skip_sequence<uint32_t>(5) should match write_sequence parity of skip.
  std::vector<uint8_t> buf_write(64, 0);
  write_header(buf_write);
  XCdrWriter w_writer(tcb::span<uint8_t>(buf_write.data(), buf_write.size()), 4);
  w_writer.begin_write_sequence(5);
  for (int i = 0; i < 5; ++i) {
    w_writer.write<uint32_t>(i);
                                                             }
  w_writer.end_write_sequence();
  size_t written_size = w_writer.bytes_written();

  std::vector<uint8_t> buf_skip(64, 0);
  write_header(buf_skip);
  XCdrWriter s_writer(tcb::span<uint8_t>(buf_skip.data(), buf_skip.size()), 4);
  s_writer.skip_sequence<uint32_t>(5);
  EXPECT_FALSE(s_writer.has_error());
  EXPECT_EQ(s_writer.bytes_written(), written_size);
}

TEST(XCdrWriter_Skip, SkipSequence_ZeroCount)
{
  std::vector<uint8_t> buf(64, 0);
  write_header(buf);
  XCdrWriter w(tcb::span<uint8_t>(buf.data(), buf.size()), 4);
  w.skip_sequence<uint32_t>(0);
  EXPECT_FALSE(w.has_error());
  // Length prefix only (4 bytes).
  EXPECT_EQ(w.bytes_written(), 8u);  // header 4 + sequence length 4
}

TEST(XCdrWriter_Skip, SkipArray_GrowingMode)
{
  // In growing mode, skip_array must produce same buffer size as per-element skip.
  XCdrWriter per_element_writer;
  for (int i = 0; i < 8; ++i) {
    per_element_writer.skip<uint16_t>();
                                                                     }
  per_element_writer.flush();
  size_t per_element_size = per_element_writer.bytes_written();

  XCdrWriter bulk_writer;
  bulk_writer.skip_array<uint16_t>(8);
  EXPECT_FALSE(bulk_writer.has_error());
  EXPECT_EQ(bulk_writer.bytes_written(), per_element_size);
}

TEST(XCdrWriter_Skip, SkipSequence_GrowingMode)
{
  XCdrWriter per_element_writer;
  per_element_writer.begin_skip_sequence(6);
  for (int i = 0; i < 6; ++i) {
    per_element_writer.skip<uint16_t>();
                                                                     }
  per_element_writer.end_write_sequence();
  size_t per_element_size = per_element_writer.bytes_written();

  XCdrWriter bulk_writer;
  bulk_writer.skip_sequence<uint16_t>(6);
  EXPECT_FALSE(bulk_writer.has_error());
  EXPECT_EQ(bulk_writer.bytes_written(), per_element_size);
}

TEST(XCdrWriter_Skip, NonMutatingInFixedMode)
{
  // Fill buffer with known bytes (0xAA).
  std::vector<uint8_t> buf(64, 0xAA);
  write_header(buf);
  tcb::span<uint8_t> span(buf.data(), buf.size());

  XCdrWriter writer(span, 4);
  writer.skip<uint8_t>();
  writer.skip<uint16_t>();
  writer.skip<uint32_t>();
  writer.skip_string(8);
  writer.skip_wstring(3);

  // All bytes at/after offset 4 must still be 0xAA.
  for (size_t i = 4; i < buf.size(); ++i) {
    EXPECT_EQ(buf[i], 0xAA) << "non-mutating check failed at offset " << i;
  }
}

// Better overflow test:
TEST(XCdrWriter_Skip, Overflow_AtBufferEnd)
{
  std::vector<uint8_t> buf(10, 0);
  write_header(buf);  // [0..3]
  tcb::span<uint8_t> span(buf.data(), buf.size());

  XCdrWriter writer(span, 4);
  // Skip 6 bytes (positions 4..9 inclusive).
  writer.skip<uint32_t>();  // 4 bytes → pos 8
  EXPECT_FALSE(writer.has_error());
  writer.skip<uint16_t>();  // 2 bytes → pos 10
  EXPECT_FALSE(writer.has_error());

  // Next skip should overflow.
  writer.skip<uint8_t>();   // 1 byte → pos 11 > 10
  EXPECT_TRUE(writer.has_error());
}

TEST(XCdrWriter_Skip, SkipThenWrite_PositionContinuity)
{
  // Skip some fields, then write: position must be continuous.
  std::vector<uint8_t> buf(64, 0);
  write_header(buf);
  tcb::span<uint8_t> span(buf.data(), buf.size());

  XCdrWriter writer(span, 4);
  writer.skip<uint32_t>();   // 4 bytes → pos 8
  writer.skip<uint16_t>();   // 2 bytes → pos 10
  writer.write<uint32_t>(42);  // align to 4 at data offset 6 → pad 2 + 4 = pos 16
  // data offsets:
  //   pos 4 → data offset 0, skip uint32 -> align 4, +4 = data offset 4 → abs 8
  //   pos 8 → data offset 4, skip uint16 -> align 2 (-0), +2 = data offset 6 → abs 10
  //   pos 10 → data offset 6, write uint32 -> align 4 -> data offset 8, +4 = data offset 12
  //         → abs 16
  // Total: 16
  EXPECT_EQ(writer.bytes_written(), 16);
  EXPECT_FALSE(writer.has_error());

  // Verify the written uint32 is at the right position.
  uint32_t val;
  read_from_bytes(&val, writer.data().subspan(12, 4), XCdrEndianness::kLittleEndian);
  EXPECT_EQ(val, 42);
}

TEST(XCdrWriter_Skip, NestedStructSkip_SizeParity)
{
  // Write: struct { uint32, string, double }
  std::vector<uint8_t> buf_write(64, 0);
  write_header(buf_write);
  XCdrWriter w_writer(tcb::span<uint8_t>(buf_write.data(), buf_write.size()), 4);
  w_writer.begin_write_struct();
  w_writer.write<uint32_t>(100);
  w_writer.write(std::string_view("test"));
  w_writer.write<double>(1.5);
  w_writer.end_write_struct();
  size_t written_size = w_writer.bytes_written();

  // Skip same structure.
  std::vector<uint8_t> buf_skip(64, 0);
  write_header(buf_skip);
  XCdrWriter s_writer(tcb::span<uint8_t>(buf_skip.data(), buf_skip.size()), 4);
  s_writer.begin_write_struct();
  s_writer.skip<uint32_t>();
  s_writer.skip_string(4);  // "test" is 4 chars
  s_writer.skip<double>();
  s_writer.end_write_struct();
  EXPECT_FALSE(s_writer.has_error());
  size_t skip_size = s_writer.bytes_written();
  EXPECT_EQ(skip_size, written_size);
}

// ============================================================================
// Fixed-mode offset constructor tests
// ============================================================================

TEST(XCdrWriter, FixedMode_Offset_NoHeader_Primitives)
{
  // Pre-populate buffer — some fixed-size data plus a pre-written header.
  std::vector<uint8_t> buf(64, 0);
  write_header(buf);

  // Create writer that starts at offset 4 (right after header).
  tcb::span<uint8_t> span(buf.data(), buf.size());
  XCdrWriter writer(span, 4);

  EXPECT_FALSE(writer.has_error());

  writer.write<uint32_t>(0xDEADBEEF);
  writer.write<uint16_t>(1234);

  EXPECT_FALSE(writer.has_error());

  // data() should span from buffer start to current write position.
  auto result = writer.data();
  EXPECT_GE(result.size(), 10);

  // Header at [0..3] must be untouched.
  EXPECT_EQ(result[0], 0x00);
  EXPECT_EQ(result[1], 0x01);
  EXPECT_EQ(result[2], 0x00);
  EXPECT_EQ(result[3], 0x00);

  // uint32_t at offset 4 (aligned to 4, no padding).
  uint32_t val32;
  read_from_bytes(&val32, result.subspan(4, 4), XCdrEndianness::kLittleEndian);
  EXPECT_EQ(val32, 0xDEADBEEF);

  // uint16_t at offset 8 (aligned to 2, no padding).
  uint16_t val16;
  read_from_bytes(&val16, result.subspan(8, 2), XCdrEndianness::kLittleEndian);
  EXPECT_EQ(val16, 1234);
}

TEST(XCdrWriter, FixedMode_Offset_OutOfRange)
{
  std::vector<uint8_t> buf(8, 0);
  tcb::span<uint8_t> span(buf.data(), buf.size());

  // Start offset past the end.
  XCdrWriter writer(span, 16);
  EXPECT_TRUE(writer.has_error());
  EXPECT_EQ(writer.bytes_written(), 0);
}

TEST(XCdrWriter, FixedMode_Offset_BytesWritten)
{
  std::vector<uint8_t> buf(64, 0);
  write_header(buf);
  tcb::span<uint8_t> span(buf.data(), buf.size());

  XCdrWriter writer(span, 4);
  EXPECT_EQ(writer.bytes_written(), 4);

  writer.write<uint32_t>(100);
  // Header (4) + uint32 (4) = 8
  EXPECT_EQ(writer.bytes_written(), 8);

  writer.write<uint16_t>(200);
  // + 2 bytes = 10
  EXPECT_EQ(writer.bytes_written(), 10);
}

TEST(XCdrWriter, FixedMode_Offset_String)
{
  std::vector<uint8_t> buf(64, 0);
  write_header(buf);
  tcb::span<uint8_t> span(buf.data(), buf.size());

  XCdrWriter writer(span, 4);
  writer.write(std::string_view("ab"));

  EXPECT_FALSE(writer.has_error());

  auto result = writer.data();
  // Header (4) + length prefix (4) + "ab" (2) + null (1) = 11
  EXPECT_EQ(result.size(), 11);

  // Length prefix at offset 4 (value = 2 + 1 = 3).
  uint32_t length;
  read_from_bytes(&length, result.subspan(4, 4), XCdrEndianness::kLittleEndian);
  EXPECT_EQ(length, 3);

  // String data at offset 8.
  std::string_view str(reinterpret_cast<const char *>(result.data() + 8), 2);
  EXPECT_EQ(str, "ab");
}

TEST(XCdrWriter, FixedMode_Offset_WString)
{
  std::vector<uint8_t> buf(64, 0);
  write_header(buf);
  tcb::span<uint8_t> span(buf.data(), buf.size());

  XCdrWriter writer(span, 4);
  writer.write(std::u16string_view(u"hi"));

  EXPECT_FALSE(writer.has_error());

  auto result = writer.data();
  // Header (4) + length prefix (4) + 2*wchar (4) + null (2) = 14
  EXPECT_EQ(result.size(), 14);

  // Length prefix at offset 4 (value = 4 + 2 = 6 bytes).
  uint32_t length;
  read_from_bytes(&length, result.subspan(4, 4), XCdrEndianness::kLittleEndian);
  EXPECT_EQ(length, 6);

  // char16_t at offset 8.
  char16_t ch;
  read_from_bytes(&ch, result.subspan(8, 2), XCdrEndianness::kLittleEndian);
  EXPECT_EQ(ch, u'h');

  read_from_bytes(&ch, result.subspan(10, 2), XCdrEndianness::kLittleEndian);
  EXPECT_EQ(ch, u'i');

  // Null terminator at offset 12.
  read_from_bytes(&ch, result.subspan(12, 2), XCdrEndianness::kLittleEndian);
  EXPECT_EQ(ch, u'\0');
}

TEST(XCdrWriter, FixedMode_Offset_Sequence)
{
  std::vector<uint8_t> buf(64, 0);
  write_header(buf);
  tcb::span<uint8_t> span(buf.data(), buf.size());

  XCdrWriter writer(span, 4);
  std::vector<uint32_t> vals = {10, 20, 30};
  writer.write_sequence(tcb::span<const uint32_t>(vals.data(), vals.size()));

  EXPECT_FALSE(writer.has_error());

  auto result = writer.data();
  // Header (4) + seq length (4) + 3*uint32 (12) = 20
  EXPECT_EQ(result.size(), 20);

  uint32_t count;
  read_from_bytes(&count, result.subspan(4, 4), XCdrEndianness::kLittleEndian);
  EXPECT_EQ(count, 3);
}

TEST(XCdrWriter, FixedMode_Offset_Overflow)
{
  std::vector<uint8_t> buf(8, 0);
  write_header(buf);
  tcb::span<uint8_t> span(buf.data(), buf.size());

  // Buffer is 8 bytes total, offset 4 → 4 bytes available.
  XCdrWriter writer(span, 4);
  writer.write<uint32_t>(42);   // uses 4 bytes, fits exactly
  EXPECT_FALSE(writer.has_error());

  writer.write<uint8_t>(1);     // 1 byte, overflows
  EXPECT_TRUE(writer.has_error());
}

TEST(XCdrWriter, FixedMode_Offset_DataIncludesHeader)
{
  std::vector<uint8_t> buf(16, 0);
  write_header(buf);
  tcb::span<uint8_t> span(buf.data(), buf.size());

  XCdrWriter writer(span, 4);
  writer.write<uint32_t>(0xAABB);

  auto result = writer.data();
  // data() must cover from buffer start (including header) to write position.
  EXPECT_EQ(result.size(), 8);
  // Header visible.
  EXPECT_EQ(result[0], 0x00);
  EXPECT_EQ(result[1], 0x01);

  // bytes_written() also returns absolute position.
  EXPECT_EQ(writer.bytes_written(), 8);
}


int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
