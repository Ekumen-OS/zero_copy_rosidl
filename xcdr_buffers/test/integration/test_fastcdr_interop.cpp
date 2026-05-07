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

#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>

#include "xcdr_buffers/serialization/writer.hpp"
#include "xcdr_buffers/serialization/reader.hpp"

using namespace xcdr_buffers;  // NOLINT(build/namespaces)

TEST(FastCDRInterop, XCdrWriterToFastCDR_Primitives) {
  // Write with XCdrWriter (implicit top-level struct)
  XCdrWriter writer;
  writer.write<uint32_t>(42);
  writer.write<double>(3.14);
  auto buffer = writer.flush();

  // FastCDR in DDS_CDR mode doesn't include the encapsulation header in the buffer,
  // so we need to skip the 4-byte header from XCdrWriter output
  const uint8_t * data_start = buffer.data() + kXCdrHeaderSize;
  size_t data_size = buffer.size() - kXCdrHeaderSize;

  // Read with FastCDR - need to copy to mutable buffer for FastCDR
  std::vector<char> mutable_buffer(data_start, data_start + data_size);
  eprosima::fastcdr::FastBuffer fast_buffer(mutable_buffer.data(), mutable_buffer.size());
  eprosima::fastcdr::Cdr cdr(fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::DDS_CDR);

  uint32_t value1;
  double value2;
  cdr >> value1 >> value2;

  EXPECT_EQ(value1, 42);
  EXPECT_DOUBLE_EQ(value2, 3.14);
}

TEST(FastCDRInterop, XCdrWriterToFastCDR_String) {
  // Write with XCdrWriter (implicit top-level struct)
  XCdrWriter writer;
  writer.write(std::string_view("Hello, FastCDR!"));
  auto buffer = writer.flush();

  // Skip the 4-byte XCdr header for FastCDR compatibility
  const uint8_t * data_start = buffer.data() + kXCdrHeaderSize;
  size_t data_size = buffer.size() - kXCdrHeaderSize;

  // Read with FastCDR
  eprosima::fastcdr::FastBuffer fast_buffer(
    reinterpret_cast<char *>(const_cast<uint8_t *>(data_start)), data_size);
  eprosima::fastcdr::Cdr cdr(fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::DDS_CDR);

  std::string str;
  cdr >> str;

  EXPECT_EQ(str, "Hello, FastCDR!");
}

TEST(FastCDRInterop, XCdrWriterToFastCDR_Sequence) {
  // Write with XCdrWriter (implicit top-level struct)
  XCdrWriter writer;
  std::vector<uint32_t> values = {1, 2, 3, 4, 5};
  writer.write_sequence(tcb::span<const uint32_t>(values.data(), values.size()));
  auto buffer = writer.flush();

  // Skip the 4-byte XCdr header for FastCDR compatibility
  const uint8_t * data_start = buffer.data() + kXCdrHeaderSize;
  size_t data_size = buffer.size() - kXCdrHeaderSize;

  // Read with FastCDR
  eprosima::fastcdr::FastBuffer fast_buffer(
    reinterpret_cast<char *>(const_cast<uint8_t *>(data_start)), data_size);
  eprosima::fastcdr::Cdr cdr(fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::DDS_CDR);

  std::vector<uint32_t> result;
  cdr >> result;

  ASSERT_EQ(result.size(), 5);
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(result[i], values[i]);
  }
}

TEST(FastCDRInterop, FastCDRToXCdrReader_Primitives) {
  // Write with FastCDR
  std::vector<char> buffer(256);
  eprosima::fastcdr::FastBuffer fast_buffer(buffer.data(), buffer.size());
  eprosima::fastcdr::Cdr cdr(fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::DDS_CDR);

  // Serialize encapsulation header first (required for XCdrReader)
  cdr.serialize_encapsulation();

  uint32_t value1 = 42;
  double value2 = 3.14;
  cdr << value1 << value2;

  size_t serialized_size = cdr.get_serialized_data_length();

  // Read with XCdrReader (implicit top-level struct)
  XCdrReader reader(tcb::span<const uint8_t>(reinterpret_cast<const uint8_t *>(buffer.data()),
    serialized_size));

  auto result1 = reader.read<uint32_t>();
  ASSERT_TRUE(result1.has_value());
  EXPECT_EQ(*result1, 42);

  auto result2 = reader.read<double>();
  ASSERT_TRUE(result2.has_value());
  EXPECT_DOUBLE_EQ(*result2, 3.14);
}

TEST(FastCDRInterop, FastCDRToXCdrReader_String) {
  // Write with FastCDR
  std::vector<char> buffer(256);
  eprosima::fastcdr::FastBuffer fast_buffer(buffer.data(), buffer.size());
  eprosima::fastcdr::Cdr cdr(fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::DDS_CDR);

  // Serialize encapsulation header first (required for XCdrReader)
  cdr.serialize_encapsulation();

  std::string str = "Hello, XCdrReader!";
  cdr << str;

  size_t serialized_size = cdr.get_serialized_data_length();

  // Read with XCdrReader (implicit top-level struct)
  XCdrReader reader(tcb::span<const uint8_t>(reinterpret_cast<const uint8_t *>(buffer.data()),
    serialized_size));

  auto result = reader.read<std::string_view>();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Hello, XCdrReader!");
}

TEST(FastCDRInterop, FastCDRToXCdrReader_Sequence) {
  // Write with FastCDR
  std::vector<char> buffer(256);
  eprosima::fastcdr::FastBuffer fast_buffer(buffer.data(), buffer.size());
  eprosima::fastcdr::Cdr cdr(fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::DDS_CDR);

  // Serialize encapsulation header first (required for XCdrReader)
  cdr.serialize_encapsulation();

  std::vector<uint32_t> values = {1, 2, 3, 4, 5};
  cdr << values;

  size_t serialized_size = cdr.get_serialized_data_length();

  // Read with XCdrReader (implicit top-level struct)
  XCdrReader reader(tcb::span<const uint8_t>(reinterpret_cast<const uint8_t *>(buffer.data()),
    serialized_size));

  auto result = reader.read<tcb::span<const uint32_t>>();
  ASSERT_TRUE(result.has_value());

  ASSERT_EQ(result->size(), 5);
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ((*result)[i], values[i]);
  }
}

TEST(FastCDRInterop, RoundTripComplex) {
  // Write complex structure with XCdrWriter (implicit top-level struct)
  XCdrWriter writer;
  writer.write<uint32_t>(42);
  writer.write(std::string_view("test"));
  std::vector<double> coords = {1.0, 2.0, 3.0};
  writer.write_sequence(tcb::span<const double>(coords.data(), coords.size()));
  auto xcdr_buffer = writer.flush();

  // Skip XCdr header for FastCDR
  const uint8_t * data_start = xcdr_buffer.data() + kXCdrHeaderSize;
  size_t data_size = xcdr_buffer.size() - kXCdrHeaderSize;

  // Read with FastCDR - copy to mutable buffer
  std::vector<char> mutable_buffer(data_start, data_start + data_size);
  eprosima::fastcdr::FastBuffer fast_buffer(mutable_buffer.data(), mutable_buffer.size());
  eprosima::fastcdr::Cdr cdr_read(fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::DDS_CDR);

  uint32_t id;
  std::string name;
  std::vector<double> values;

  cdr_read >> id >> name >> values;

  EXPECT_EQ(id, 42);
  EXPECT_EQ(name, "test");
  ASSERT_EQ(values.size(), 3);
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_DOUBLE_EQ(values[i], coords[i]);
  }

  // Write same structure with FastCDR
  std::vector<char> fastcdr_buffer(256);
  eprosima::fastcdr::FastBuffer fast_buffer2(fastcdr_buffer.data(), fastcdr_buffer.size());
  eprosima::fastcdr::Cdr cdr_write(fast_buffer2, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::DDS_CDR);

  // Serialize encapsulation header first for XCdrReader
  cdr_write.serialize_encapsulation();

  cdr_write << id << name << values;
  size_t serialized_size = cdr_write.get_serialized_data_length();

  // Read with XCdrReader (implicit top-level struct)
  XCdrReader reader(tcb::span<const uint8_t>(
    reinterpret_cast<const uint8_t *>(fastcdr_buffer.data()), serialized_size));

  auto id_result = reader.read<uint32_t>();
  ASSERT_TRUE(id_result.has_value());
  EXPECT_EQ(*id_result, 42);

  auto name_result = reader.read<std::string_view>();
  ASSERT_TRUE(name_result.has_value());
  EXPECT_EQ(*name_result, "test");

  auto seq_result = reader.read<tcb::span<const double>>();
  ASSERT_TRUE(seq_result.has_value());
  ASSERT_EQ(seq_result->size(), 3);
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_DOUBLE_EQ((*seq_result)[i], coords[i]);
  }
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
