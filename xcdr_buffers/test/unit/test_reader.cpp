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

#include "xcdr_buffers/serialization/reader.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

using namespace xcdr_buffers;  // NOLINT(build/namespaces)

TEST(XCdrReader, ReadHeader) {
  XCdrWriter writer;
  writer.write<uint32_t>(42);
  auto buffer = writer.flush();

  // Constructor automatically reads and validates header
  XCdrReader reader(buffer);

  EXPECT_EQ(reader.endianness(), XCdrEndianness::kLittleEndian);
}

TEST(XCdrReader, ReadHeaderBigEndian) {
  XCdrWriter writer(XCdrEndianness::kBigEndian);
  writer.write<uint32_t>(42);
  auto buffer = writer.flush();

  // Constructor automatically reads and validates header
  XCdrReader reader(buffer);

  EXPECT_EQ(reader.endianness(), XCdrEndianness::kBigEndian);
}

TEST(XCdrReader, ReadPrimitives) {
  XCdrWriter writer;
  writer.write<uint8_t>(42);
  writer.write<uint16_t>(1234);
  writer.write<uint32_t>(0xDEADBEEF);
  writer.write<uint64_t>(0x0102030405060708ULL);
  writer.write<float>(3.14f);
  writer.write<double>(2.71828);
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  auto u8_result = reader.read<uint8_t>();
  ASSERT_TRUE(u8_result.has_value());
  EXPECT_EQ(*u8_result, 42);

  auto u16_result = reader.read<uint16_t>();
  ASSERT_TRUE(u16_result.has_value());
  EXPECT_EQ(*u16_result, 1234);

  auto u32_result = reader.read<uint32_t>();
  ASSERT_TRUE(u32_result.has_value());
  EXPECT_EQ(*u32_result, 0xDEADBEEF);

  auto u64_result = reader.read<uint64_t>();
  ASSERT_TRUE(u64_result.has_value());
  EXPECT_EQ(*u64_result, 0x0102030405060708ULL);

  auto float_result = reader.read<float>();
  ASSERT_TRUE(float_result.has_value());
  EXPECT_FLOAT_EQ(*float_result, 3.14f);

  auto double_result = reader.read<double>();
  ASSERT_TRUE(double_result.has_value());
  EXPECT_DOUBLE_EQ(*double_result, 2.71828);
}

TEST(XCdrReader, ReadString) {
  XCdrWriter writer;
  writer.write(std::string_view("Hello, world!"));
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  auto str_result = reader.read<std::string_view>();
  ASSERT_TRUE(str_result.has_value());
  EXPECT_EQ(*str_result, "Hello, world!");
}

TEST(XCdrReader, ReadEmptyString) {
  XCdrWriter writer;
  writer.write(std::string_view(""));
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  auto str_result = reader.read<std::string_view>();
  ASSERT_TRUE(str_result.has_value());
  EXPECT_EQ(*str_result, "");
}

TEST(XCdrReader, ReadSequence) {
  XCdrWriter writer;
  std::vector<uint32_t> values = {1, 2, 3, 4, 5};
  writer.write_sequence(tcb::span<const uint32_t>(values.data(), values.size()));
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  auto seq_result = reader.read<tcb::span<const uint32_t>>();
  ASSERT_TRUE(seq_result.has_value());

  auto seq = *seq_result;
  EXPECT_EQ(seq.size(), 5);
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(seq[i], values[i]);
  }
}

TEST(XCdrReader, ReadEmptySequence) {
  XCdrWriter writer;
  std::vector<uint32_t> values;
  writer.write_sequence(tcb::span<const uint32_t>(values.data(), values.size()));
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  auto seq_result = reader.read<tcb::span<const uint32_t>>();
  ASSERT_TRUE(seq_result.has_value());

  auto seq = *seq_result;
  EXPECT_EQ(seq.size(), 0);
}

TEST(XCdrReader, ReadArray) {
  XCdrWriter writer;
  std::vector<uint16_t> values = {10, 20, 30};
  writer.write_array(tcb::span<const uint16_t>(values.data(), values.size()));
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  auto arr_result = reader.read<tcb::span<const uint16_t, 3>>();
  ASSERT_TRUE(arr_result.has_value());

  auto arr = *arr_result;
  EXPECT_EQ(arr.size(), 3);
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(arr[i], values[i]);
  }
}

TEST(XCdrReader, ReadStruct) {
  XCdrWriter writer;
  writer.begin_write_struct();
  writer.write<uint32_t>(100);
  writer.write(std::string_view("test"));
  writer.write<double>(1.5);
  writer.end_write_struct();
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  ASSERT_TRUE(reader.begin_read_struct().has_value());

  auto u32_result = reader.read<uint32_t>();
  ASSERT_TRUE(u32_result.has_value());
  EXPECT_EQ(*u32_result, 100);

  auto str_result = reader.read<std::string_view>();
  ASSERT_TRUE(str_result.has_value());
  EXPECT_EQ(*str_result, "test");

  auto double_result = reader.read<double>();
  ASSERT_TRUE(double_result.has_value());
  EXPECT_DOUBLE_EQ(*double_result, 1.5);

  ASSERT_TRUE(reader.end_read_struct().has_value());
}

TEST(XCdrReader, ReadWriteRoundTrip) {
  // Write complex structure
  XCdrWriter writer;
  writer.begin_write_struct();
  writer.write<uint32_t>(42);
  writer.write(std::string_view("name"));
  std::vector<double> coords = {1.0, 2.0, 3.0};
  writer.write_sequence(tcb::span<const double>(coords.data(), coords.size()));
  writer.end_write_struct();
  auto buffer = writer.flush();

  // Read it back
  XCdrReader reader(buffer);

  ASSERT_TRUE(reader.begin_read_struct().has_value());

  auto id = reader.read<uint32_t>();
  ASSERT_TRUE(id.has_value());
  EXPECT_EQ(*id, 42);

  auto name = reader.read<std::string_view>();
  ASSERT_TRUE(name.has_value());
  EXPECT_EQ(*name, "name");

  auto seq = reader.read<tcb::span<const double>>();
  ASSERT_TRUE(seq.has_value());
  EXPECT_EQ(seq->size(), 3);
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_DOUBLE_EQ((*seq)[i], coords[i]);
  }

  ASSERT_TRUE(reader.end_read_struct().has_value());
}

TEST(XCdrReader, ErrorBufferTooSmall) {
  std::vector<uint8_t> small_buffer = {0x00, 0x01};  // Only 2 bytes

  // Constructor should throw on invalid header
  EXPECT_THROW({
    XCdrReader reader(tcb::span<const uint8_t>(small_buffer.data(), small_buffer.size()));
  }, XCdrError);

  // Or use wrap() for explicit error handling
  auto result = XCdrReader::wrap(tcb::span<const uint8_t>(small_buffer.data(),
    small_buffer.size()));
  EXPECT_FALSE(result.has_value());
}

TEST(XCdrReader, Reset) {
  XCdrWriter writer1;
  writer1.write<uint32_t>(42);
  auto buffer1 = writer1.data();

  XCdrWriter writer2;
  writer2.write<uint32_t>(99);
  auto buffer2 = writer2.data();

  XCdrReader reader(buffer1);

  auto val1 = reader.read<uint32_t>();
  ASSERT_TRUE(val1.has_value());
  EXPECT_EQ(*val1, 42);

  // Reset with second buffer
  reader.reset(buffer2);

  auto val2 = reader.read<uint32_t>();
  ASSERT_TRUE(val2.has_value());
  EXPECT_EQ(*val2, 99);
}

TEST(XCdrReader, ReadIntoPrimitive) {
  XCdrWriter writer;
  writer.write<int32_t>(42);
  writer.write<double>(3.14);
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  int32_t i = 0;
  ASSERT_TRUE(reader.read_into(i));
  EXPECT_EQ(i, 42);

  double d = 0.0;
  ASSERT_TRUE(reader.read_into(d));
  EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(XCdrReader, ReadIntoString) {
  XCdrWriter writer;
  writer.write(std::string_view("hello"));
  writer.write(std::string_view("world"));
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  // Test reusing string buffer
  std::string str;
  ASSERT_TRUE(reader.read_into(str));
  EXPECT_EQ(str, "hello");

  // Read into same string (should reuse allocation)
  ASSERT_TRUE(reader.read_into(str));
  EXPECT_EQ(str, "world");
}

TEST(XCdrReader, ReadIntoArray) {
  XCdrWriter writer;
  std::array<uint16_t, 3> write_arr = {10, 20, 30};
  writer.write_array(tcb::span<const uint16_t>(write_arr.data(), write_arr.size()));
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  std::array<uint16_t, 3> read_arr = {0, 0, 0};
  ASSERT_TRUE(reader.read_into(read_arr));
  EXPECT_EQ(read_arr[0], 10);
  EXPECT_EQ(read_arr[1], 20);
  EXPECT_EQ(read_arr[2], 30);
}

TEST(XCdrReader, ReadIntoVector) {
  XCdrWriter writer;
  std::vector<double> write_vec = {1.1, 2.2, 3.3, 4.4};
  writer.write_sequence(tcb::span<const double>(write_vec.data(), write_vec.size()));
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  std::vector<double> read_vec;
  ASSERT_TRUE(reader.read_into(read_vec));
  ASSERT_EQ(read_vec.size(), 4);
  EXPECT_DOUBLE_EQ(read_vec[0], 1.1);
  EXPECT_DOUBLE_EQ(read_vec[1], 2.2);
  EXPECT_DOUBLE_EQ(read_vec[2], 3.3);
  EXPECT_DOUBLE_EQ(read_vec[3], 4.4);
}

TEST(XCdrReader, ReadIntoVectorReuse) {
  XCdrWriter writer;
  std::vector<int32_t> vec1 = {1, 2, 3};
  std::vector<int32_t> vec2 = {4, 5};
  writer.write_sequence(tcb::span<const int32_t>(vec1.data(), vec1.size()));
  writer.write_sequence(tcb::span<const int32_t>(vec2.data(), vec2.size()));
  auto buffer = writer.flush();

  XCdrReader reader(buffer);

  // Read first sequence
  std::vector<int32_t> read_vec;
  ASSERT_TRUE(reader.read_into(read_vec));
  ASSERT_EQ(read_vec.size(), 3);
  EXPECT_EQ(read_vec[0], 1);
  EXPECT_EQ(read_vec[1], 2);
  EXPECT_EQ(read_vec[2], 3);

  // Read second sequence into same vector (should clear and reuse)
  ASSERT_TRUE(reader.read_into(read_vec));
  ASSERT_EQ(read_vec.size(), 2);
  EXPECT_EQ(read_vec[0], 4);
  EXPECT_EQ(read_vec[1], 5);
}

TEST(XCdrReader, CompareReadVsReadInto) {
  // Verify read<T>() and read_into() produce same results
  XCdrWriter writer;
  writer.write<int32_t>(99);
  writer.write(std::string_view("test"));
  auto buffer = writer.flush();

  // Test read<T>()
  XCdrReader reader1(buffer);
  auto i1 = reader1.read<int32_t>();
  auto s1 = reader1.read<std::string>();
  ASSERT_TRUE(i1.has_value());
  ASSERT_TRUE(s1.has_value());

  // Test read_into()
  XCdrReader reader2(buffer);
  int32_t i2 = 0;
  std::string s2;
  ASSERT_TRUE(reader2.read_into(i2));
  ASSERT_TRUE(reader2.read_into(s2));

  // Should be identical
  EXPECT_EQ(*i1, i2);
  EXPECT_EQ(*s1, s2);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
