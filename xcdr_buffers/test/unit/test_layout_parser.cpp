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
#include "xcdr_buffers/serialization/writer.hpp"

using namespace xcdr_buffers;  // NOLINT(build/namespaces)

class LayoutParserTest : public ::testing::Test
{
protected:
  XCdrWriter writer_;
};

TEST_F(LayoutParserTest, ParseHeaderSuccess)
{
  writer_.write(static_cast<uint32_t>(42));
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);
  EXPECT_EQ(parser.endianness(), kSystemEndianness);
  EXPECT_EQ(parser.current_offset(), kXCdrHeaderSize);
}

TEST_F(LayoutParserTest, ParseHeaderBufferTooSmall)
{
  std::vector<uint8_t> small_buffer = {0x00, 0x01};
  EXPECT_THROW({
    XCdrLayoutParser parser(small_buffer);
  }, XCdrError);
}

TEST_F(LayoutParserTest, ParseSinglePrimitive)
{
  writer_.write(static_cast<uint32_t>(42));
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  const auto & layout = *result;
  EXPECT_EQ(layout.member_count(), 1);
  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  const auto & member = member_result->get();
  EXPECT_EQ(member.name(), "field_0");
  EXPECT_EQ(layout.total_size(), kXCdrHeaderSize + 4);
}

TEST_F(LayoutParserTest, ParseMultiplePrimitives)
{
  writer_.write(static_cast<uint32_t>(42));
  writer_.write(static_cast<double>(3.14));
  writer_.write(static_cast<uint8_t>(1));
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint8));

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  const auto & layout = *result;
  EXPECT_EQ(layout.member_count(), 3);
}

TEST_F(LayoutParserTest, ParseString)
{
  writer_.write("Hello");
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.parse_string());

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  const auto & layout = *result;
  EXPECT_EQ(layout.member_count(), 1);

  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  const auto & member = member_result->get();

  ASSERT_TRUE(std::holds_alternative<XCdrStringLayout>(member.layout()));
  const auto & string_layout = std::get<XCdrStringLayout>(member.layout());
  EXPECT_EQ(string_layout.actual_length(), 5);  // "Hello" without null terminator
}

TEST_F(LayoutParserTest, ParseEmptyString)
{
  writer_.write("");
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.parse_string());

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  const auto & layout = *result;
  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  const auto & member = member_result->get();
  const auto & string_layout = std::get<XCdrStringLayout>(member.layout());
  EXPECT_EQ(string_layout.actual_length(), 0);
}

TEST_F(LayoutParserTest, ParsePrimitiveArray)
{
  std::vector<uint32_t> values = {1, 2, 3, 4, 5};
  writer_.write_array(values);
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);
  ASSERT_TRUE(parser.parse_primitive_array(XCdrPrimitiveKind::kUint32, 5));

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  const auto & layout = *result;
  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  const auto & member = member_result->get();
  ASSERT_TRUE(std::holds_alternative<XCdrPrimitiveArrayLayout>(member.layout()));

  const auto & prim_array = std::get<XCdrPrimitiveArrayLayout>(member.layout());
  EXPECT_EQ(prim_array.count(), 5);
  EXPECT_EQ(prim_array.element_kind(), XCdrPrimitiveKind::kUint32);
}

TEST_F(LayoutParserTest, ParseStringArray)
{
  writer_.begin_write_array(3);
  writer_.write("one");
  writer_.write("two");
  writer_.write("three");
  writer_.end_write_array();
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.begin_parse_array(3));
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.end_parse_array());

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  const auto & layout = *result;
  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  const auto & member = member_result->get();
  ASSERT_TRUE(std::holds_alternative<XCdrArrayLayout>(member.layout()));

  const auto & str_array = std::get<XCdrArrayLayout>(member.layout());
  EXPECT_EQ(str_array.count(), 3);

  EXPECT_EQ(std::get<XCdrStringLayout>(str_array.element_layout(0)->get()).actual_length(), 3);
  EXPECT_EQ(std::get<XCdrStringLayout>(str_array.element_layout(1)->get()).actual_length(), 3);
  EXPECT_EQ(std::get<XCdrStringLayout>(str_array.element_layout(2)->get()).actual_length(), 5);
}

TEST_F(LayoutParserTest, ParsePrimitiveSequence)
{
  std::vector<uint32_t> values = {10, 20, 30};
  writer_.write_sequence(values);
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);

  // Use parse_primitive_sequence which reads count from buffer
  ASSERT_TRUE(parser.parse_primitive_sequence(XCdrPrimitiveKind::kUint32, 3));

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  const auto & layout = *result;
  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  const auto & member = member_result->get();
  ASSERT_TRUE(std::holds_alternative<XCdrPrimitiveSequenceLayout>(member.layout()));

  const auto & prim_seq = std::get<XCdrPrimitiveSequenceLayout>(member.layout());
  EXPECT_EQ(prim_seq.actual_count(), 3);
}

TEST_F(LayoutParserTest, ParseStringSequence)
{
  writer_.begin_write_sequence(2);
  writer_.write("hello");
  writer_.write("world");
  writer_.end_write_sequence();
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.begin_parse_sequence());
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.end_parse_sequence());

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  const auto & layout = *result;
  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  const auto & member = member_result->get();
  ASSERT_TRUE(std::holds_alternative<XCdrSequenceLayout>(member.layout()));

  const auto & str_seq = std::get<XCdrSequenceLayout>(member.layout());
  EXPECT_EQ(str_seq.actual_count(), 2);
  const auto & elem0_layout = std::get<XCdrStringLayout>(str_seq.element_layout(0)->get());
  const auto & elem1_layout = std::get<XCdrStringLayout>(str_seq.element_layout(1)->get());
  EXPECT_EQ(elem0_layout.actual_length(), 5);  // "hello" without null terminator
  EXPECT_EQ(elem1_layout.actual_length(), 5);  // "world" without null terminator
}

TEST_F(LayoutParserTest, ParseNestedStruct)
{
  // Using implicit top-level struct
  writer_.write(static_cast<uint32_t>(42));
  writer_.begin_write_struct();
  writer_.write("nested");
  writer_.end_write_struct();
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);

  // Parse top-level members
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.begin_parse_struct("inner"));
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.end_parse_struct());

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  const auto & layout = *result;
  EXPECT_EQ(layout.member_count(), 2);

  // Check nested struct
  auto inner_member_result = layout.get_member(1);
  ASSERT_TRUE(inner_member_result);
  const auto & inner_member = inner_member_result->get();
  EXPECT_EQ(inner_member.name(), "inner");

  ASSERT_TRUE(std::holds_alternative<XCdrStructLayout>(inner_member.layout()));
  const auto & inner_struct = std::get<XCdrStructLayout>(inner_member.layout());
  EXPECT_EQ(inner_struct.member_count(), 1);
}

TEST_F(LayoutParserTest, ParseComplexStructure)
{
  // Write: struct { uint32, string, sequence<double>, array<string>[2] }
  // Using implicit top-level struct
  writer_.write(static_cast<uint32_t>(123));
  writer_.write("test");
  const std::vector<double> double_seq = {1.1, 2.2, 3.3};
  writer_.write_sequence(double_seq);
  writer_.begin_write_array(2);
  writer_.write("a");
  writer_.write("bb");
  writer_.end_write_array();
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);

  // Parse top-level members
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.parse_string());

  // For primitive sequence, use shortcut API
  ASSERT_TRUE(parser.parse_primitive_sequence(XCdrPrimitiveKind::kDouble, 3));

  // For string array, parse each element
  ASSERT_TRUE(parser.begin_parse_array(2));
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.end_parse_array());

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  const auto & layout = *result;
  EXPECT_EQ(layout.member_count(), 4);
}

TEST_F(LayoutParserTest, ParseBufferOverflow)
{
  writer_.write(static_cast<uint32_t>(42));
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));

  // Try to parse another primitive when buffer is exhausted
  auto status = parser.parse_primitive(XCdrPrimitiveKind::kUint32);
  EXPECT_FALSE(status);
  EXPECT_NE(status.error().message().find("overflow"), std::string::npos);
}

TEST_F(LayoutParserTest, FinalizeResetsState)
{
  writer_.write(static_cast<uint32_t>(42));
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));

  auto result = parser.finalize();
  ASSERT_TRUE(result);

  // After finalize, parser is reset
  EXPECT_EQ(parser.current_offset(), 0);
}

TEST_F(LayoutParserTest, BytesRemaining)
{
  writer_.write(static_cast<uint32_t>(42));
  auto buffer = writer_.flush();

  XCdrLayoutParser parser(buffer);
  // Header is already parsed in constructor
  EXPECT_EQ(parser.bytes_remaining(), buffer.size() - kXCdrHeaderSize);

  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  EXPECT_EQ(parser.bytes_remaining(), 0);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
