// Copyright 2025 Apex.AI, Inc.
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

#include <cstdint>
#include <string>
#include <vector>

#include "xcdr_buffers/layout/layout_parser.hpp"
#include "xcdr_buffers/serialization/reader.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

using namespace xcdr_buffers;  // NOLINT(build/namespaces)

class WriterParserIntegrationTest : public ::testing::Test
{
protected:
  XCdrWriter writer_;
};

TEST_F(WriterParserIntegrationTest, SimplePrimitives)
{
  // Write primitives (implicit top-level struct)
  writer_.write(static_cast<uint32_t>(42));
  writer_.write(static_cast<double>(3.14159));
  writer_.write(static_cast<uint8_t>(255));

  auto buffer = writer_.flush();

  // Parse the layout (implicit top-level struct)
  XCdrLayoutParser parser(buffer);
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint8));

  auto layout_result = parser.finalize();
  ASSERT_TRUE(layout_result);

  const auto & layout = *layout_result;
  EXPECT_EQ(layout.member_count(), 3);
  EXPECT_EQ(layout.total_size(), buffer.size());

  // Verify we can read the values back using the layout info
  XCdrReader reader(buffer);

  auto val1 = reader.read<uint32_t>();
  ASSERT_TRUE(val1);
  EXPECT_EQ(*val1, 42);

  auto val2 = reader.read<double>();
  ASSERT_TRUE(val2);
  EXPECT_NEAR(*val2, 3.14159, 0.00001);

  auto val3 = reader.read<uint8_t>();
  ASSERT_TRUE(val3);
  EXPECT_EQ(*val3, 255);
}

TEST_F(WriterParserIntegrationTest, StringFields)
{
  // Write strings of different lengths (implicit top-level struct)
  writer_.write(std::string_view("short"));
  writer_.write(std::string_view(""));
  writer_.write(std::string_view("a much longer string with more content"));

  auto buffer = writer_.flush();

  // Parse the layout (implicit top-level struct)
  XCdrLayoutParser parser(buffer);
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_string());

  auto layout_result = parser.finalize();
  ASSERT_TRUE(layout_result);

  const auto & layout = *layout_result;
  EXPECT_EQ(layout.member_count(), 3);

  // Verify string lengths in layout
  auto field0 = layout.get_member(0);
  ASSERT_TRUE(field0);
  ASSERT_TRUE(std::holds_alternative<XCdrStringLayout>(field0->get().layout()));
  EXPECT_EQ(std::get<XCdrStringLayout>(field0->get().layout()).actual_length(), 5);

  auto field1 = layout.get_member(1);
  ASSERT_TRUE(field1);
  ASSERT_TRUE(std::holds_alternative<XCdrStringLayout>(field1->get().layout()));
  EXPECT_EQ(std::get<XCdrStringLayout>(field1->get().layout()).actual_length(), 0);

  auto field2 = layout.get_member(2);
  ASSERT_TRUE(field2);
  ASSERT_TRUE(std::holds_alternative<XCdrStringLayout>(field2->get().layout()));
  EXPECT_EQ(std::get<XCdrStringLayout>(field2->get().layout()).actual_length(), 38);

  // Read back and verify
  XCdrReader reader(buffer);

  EXPECT_EQ(reader.read<std::string_view>(), "short");
  EXPECT_EQ(reader.read<std::string_view>(), "");
  EXPECT_EQ(reader.read<std::string_view>(), "a much longer string with more content");
}

TEST_F(WriterParserIntegrationTest, PrimitiveArrays)
{
  // Write arrays of primitives (implicit top-level struct)
  std::vector<uint32_t> arr1 = {1, 2, 3, 4, 5};
  std::vector<double> arr2 = {1.1, 2.2, 3.3};
  writer_.write_array(tcb::span<const uint32_t>(arr1));
  writer_.write_array(tcb::span<const double>(arr2));

  auto buffer = writer_.flush();

  // Parse the layout (implicit top-level struct)
  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.begin_parse_array(5));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.end_parse_array());

  ASSERT_TRUE(parser.begin_parse_array(3));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  ASSERT_TRUE(parser.end_parse_array());

  auto layout_result = parser.finalize();
  ASSERT_TRUE(layout_result);

  const auto & layout = *layout_result;
  EXPECT_EQ(layout.member_count(), 2);

  // Verify array layouts
  auto field0 = layout.get_member(0);
  ASSERT_TRUE(field0);
  ASSERT_TRUE(std::holds_alternative<XCdrPrimitiveArrayLayout>(field0->get().layout()));
  const auto & array0 = std::get<XCdrPrimitiveArrayLayout>(field0->get().layout());
  EXPECT_EQ(array0.count(), 5);

  auto field1 = layout.get_member(1);
  ASSERT_TRUE(field1);
  ASSERT_TRUE(std::holds_alternative<XCdrPrimitiveArrayLayout>(field1->get().layout()));
  const auto & array1 = std::get<XCdrPrimitiveArrayLayout>(field1->get().layout());
  EXPECT_EQ(array1.count(), 3);
}

TEST_F(WriterParserIntegrationTest, Sequences)
{
  // Write sequences of different types (implicit top-level struct)
  const std::vector<uint32_t> seq1 = {10, 20, 30, 40};
  writer_.write_sequence(tcb::span(seq1));

  writer_.begin_write_sequence(2);
  writer_.write(std::string_view("hello"));
  writer_.write(std::string_view("world"));
  writer_.end_write_sequence();

  auto buffer = writer_.flush();

  // Parse the layout (implicit top-level struct)
  XCdrLayoutParser parser(buffer);

  // Primitive sequence shortcut
  ASSERT_TRUE(parser.parse_primitive_sequence(XCdrPrimitiveKind::kUint32, 4));

  // General sequence with string elements
  auto count_result = parser.begin_parse_sequence();
  ASSERT_TRUE(count_result);
  EXPECT_EQ(*count_result, 2);
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.end_parse_sequence());

  auto layout_result = parser.finalize();
  ASSERT_TRUE(layout_result);

  const auto & layout = *layout_result;
  EXPECT_EQ(layout.member_count(), 2);

  // Verify sequence layouts
  auto field0 = layout.get_member(0);
  ASSERT_TRUE(field0);
  ASSERT_TRUE(std::holds_alternative<XCdrPrimitiveSequenceLayout>(field0->get().layout()));
  const auto & seq0 = std::get<XCdrPrimitiveSequenceLayout>(field0->get().layout());
  EXPECT_EQ(seq0.actual_count(), 4);

  auto field1 = layout.get_member(1);
  ASSERT_TRUE(field1);
  ASSERT_TRUE(std::holds_alternative<XCdrSequenceLayout>(field1->get().layout()));
  const auto & str_seq = std::get<XCdrSequenceLayout>(field1->get().layout());
  EXPECT_EQ(str_seq.actual_count(), 2);
  EXPECT_EQ(std::get<XCdrStringLayout>(str_seq.element_layout(0)->get()).actual_length(), 5);
  EXPECT_EQ(std::get<XCdrStringLayout>(str_seq.element_layout(1)->get()).actual_length(), 5);
}

TEST_F(WriterParserIntegrationTest, NestedStructs)
{
  // Write nested structs (implicit top-level struct)
  writer_.write(static_cast<uint32_t>(100));

  writer_.begin_write_struct();
  writer_.write(std::string_view("nested"));
  writer_.write(static_cast<double>(2.71828));
  writer_.end_write_struct();

  writer_.write(static_cast<uint8_t>(42));

  auto buffer = writer_.flush();

  // Parse the layout (implicit top-level struct)
  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));

  ASSERT_TRUE(parser.begin_parse_struct("inner"));
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  ASSERT_TRUE(parser.end_parse_struct());

  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint8));

  auto layout_result = parser.finalize();
  ASSERT_TRUE(layout_result);

  const auto & layout = *layout_result;
  EXPECT_EQ(layout.member_count(), 3);

  // Verify nested struct
  auto field1 = layout.get_member(1);
  ASSERT_TRUE(field1);
  EXPECT_EQ(field1->get().name(), "inner");
  ASSERT_TRUE(std::holds_alternative<XCdrStructLayout>(field1->get().layout()));
  const auto & inner = std::get<XCdrStructLayout>(field1->get().layout());
  EXPECT_EQ(inner.member_count(), 2);
}

TEST_F(WriterParserIntegrationTest, ComplexNestedStructure)
{
  // Write a complex structure (implicit top-level struct):
  // {
  //   uint32_t id;
  //   string name;
  //   struct {
  //     double x;
  //     double y;
  //   } position;
  //   sequence<uint32_t> values;
  //   array<string>[3] tags;
  // }

  writer_.write(static_cast<uint32_t>(12345));
  writer_.write(std::string_view("robot"));

  writer_.begin_write_struct();
  writer_.write(static_cast<double>(10.5));
  writer_.write(static_cast<double>(20.3));
  writer_.end_write_struct();

  const std::vector<uint32_t> seq2 = {1, 2, 3, 4, 5};
  writer_.write_sequence(tcb::span(seq2));

  writer_.begin_write_array(3);
  writer_.write(std::string_view("tag1"));
  writer_.write(std::string_view("tag2"));
  writer_.write(std::string_view("tag3"));
  writer_.end_write_array();

  auto buffer = writer_.flush();

  // Parse the layout (implicit top-level struct)
  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.parse_string());

  ASSERT_TRUE(parser.begin_parse_struct("position"));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  ASSERT_TRUE(parser.end_parse_struct());

  ASSERT_TRUE(parser.parse_primitive_sequence(XCdrPrimitiveKind::kUint32, 5));

  ASSERT_TRUE(parser.begin_parse_array(3));
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.end_parse_array());

  auto layout_result = parser.finalize();
  ASSERT_TRUE(layout_result);

  const auto & layout = *layout_result;
  EXPECT_EQ(layout.member_count(), 5);
  EXPECT_EQ(layout.total_size(), buffer.size());

  // Read back and verify all values
  XCdrReader reader(buffer);

  EXPECT_EQ(*reader.read<uint32_t>(), 12345);
  EXPECT_EQ(reader.read<std::string_view>(), "robot");

  ASSERT_TRUE(reader.begin_read_struct());
  EXPECT_NEAR(*reader.read<double>(), 10.5, 0.0001);
  EXPECT_NEAR(*reader.read<double>(), 20.3, 0.0001);
  ASSERT_TRUE(reader.end_read_struct());

  auto seq_result = reader.read<tcb::span<const uint32_t>>();
  ASSERT_TRUE(seq_result);
  EXPECT_EQ(seq_result->size(), 5);

  ASSERT_TRUE(reader.begin_read_array(3));
  EXPECT_EQ(reader.read<std::string_view>(), "tag1");
  EXPECT_EQ(reader.read<std::string_view>(), "tag2");
  EXPECT_EQ(reader.read<std::string_view>(), "tag3");
  ASSERT_TRUE(reader.end_read_array());
}

TEST_F(WriterParserIntegrationTest, EmptySequences)
{
  // Write with empty sequences (implicit top-level struct)
  const std::vector<uint32_t> empty_seq;
  writer_.write_sequence(tcb::span(empty_seq));
  writer_.begin_write_sequence(0);
  writer_.end_write_sequence();

  auto buffer = writer_.flush();

  // Parse the layout (implicit top-level struct)
  XCdrLayoutParser parser(buffer);

  // Empty primitive sequence - shortcut method
  ASSERT_TRUE(parser.parse_primitive_sequence(XCdrPrimitiveKind::kUint32, 0));

  // Empty general sequence - begin/end without elements
  auto count_result = parser.begin_parse_sequence();
  ASSERT_TRUE(count_result);
  EXPECT_EQ(*count_result, 0);
  ASSERT_TRUE(parser.end_parse_sequence());

  auto layout_result = parser.finalize();
  ASSERT_TRUE(layout_result);

  const auto & layout = *layout_result;

  // Note: Empty sequences are skipped by the builder (no type information for elements)
  // So even though we wrote 2 sequences, the layout has 0 members
  EXPECT_EQ(layout.member_count(), 0);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
