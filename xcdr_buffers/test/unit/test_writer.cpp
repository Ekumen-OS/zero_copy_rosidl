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

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
