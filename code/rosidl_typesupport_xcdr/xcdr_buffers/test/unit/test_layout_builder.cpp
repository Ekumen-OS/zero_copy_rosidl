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

#include <gtest/gtest.h>

#include "xcdr_buffers/accessor/const_accessor.hpp"
#include "xcdr_buffers/layout/layout_builder.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

using namespace xcdr_buffers;  // NOLINT(build/namespaces)

TEST(LayoutBuilderTest, AllocateSinglePrimitive) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive("a", XCdrPrimitiveKind::kUint32);
  auto layout = builder.finalize();

  // Should have 1 member with the given name
  EXPECT_EQ(layout.member_count(), 1);

  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  const auto & member = member_result->get();
  EXPECT_EQ(member.name(), "a");

  // Total size = header (4) + uint32 (4)
  EXPECT_EQ(layout.total_size(), kXCdrHeaderSize + 4);
}

TEST(LayoutBuilderTest, AllocateMultiplePrimitives) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive("a", XCdrPrimitiveKind::kUint32);
  builder.allocate_primitive("b", XCdrPrimitiveKind::kDouble);
  builder.allocate_primitive("c", XCdrPrimitiveKind::kUint8);
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 3);

  auto m0 = layout.get_member(0);
  ASSERT_TRUE(m0);
  EXPECT_EQ(m0->get().name(), "a");

  auto m1 = layout.get_member(1);
  ASSERT_TRUE(m1);
  EXPECT_EQ(m1->get().name(), "b");

  auto m2 = layout.get_member(2);
  ASSERT_TRUE(m2);
  EXPECT_EQ(m2->get().name(), "c");

  // With CDR data-relative alignment:
  // Total size = header (4) + uint32 at offset 0 (4) + double
  // at offset 8 with 4 padding (8) + uint8 at offset 16 (1) = 21
  EXPECT_EQ(layout.total_size(), 21);
}

TEST(LayoutBuilderTest, AllocateNamedFields) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder.allocate_string("name", 5);
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 2);

  auto m0 = layout.get_member(0);
  ASSERT_TRUE(m0);
  EXPECT_EQ(m0->get().name(), "id");

  auto m1 = layout.get_member(1);
  ASSERT_TRUE(m1);
  EXPECT_EQ(m1->get().name(), "name");
}

TEST(LayoutBuilderTest, AllocateString) {
  XCdrLayoutBuilder builder;
  builder.allocate_string("s", 10);
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 1);

  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  EXPECT_EQ(member_result->get().name(), "s");

  // Total size = header (4) + length prefix (4) + string (10) + null (1)
  EXPECT_EQ(layout.total_size(), kXCdrHeaderSize + 4 + 10 + 1);
}

TEST(LayoutBuilderTest, AllocatePrimitiveArray) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive_array("arr", XCdrPrimitiveKind::kUint32, 5);
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 1);

  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  EXPECT_EQ(member_result->get().name(), "arr");

  // Total size = header (4) + 5 * uint32 (20)
  EXPECT_EQ(layout.total_size(), kXCdrHeaderSize + 5 * 4);
}

TEST(LayoutBuilderTest, AllocateNamedArray) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive_array("values", XCdrPrimitiveKind::kDouble, 3);
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 1);

  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  EXPECT_EQ(member_result->get().name(), "values");
}

TEST(LayoutBuilderTest, AllocateStringArray) {
  XCdrLayoutBuilder builder;
  builder.begin_allocate_array("arr", 3);
  builder.allocate_string(5);
  builder.allocate_string(5);
  builder.allocate_string(5);
  builder.end_allocate_array();
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 1);

  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  EXPECT_EQ(member_result->get().name(), "arr");

  // Total size = header (4) + first string (10) + padding (2) + second
  // string (10) + padding (2) + third string (10)
  // Each string: length prefix (4) + string (5) + null (1) = 10 bytes
  // After each string we align to 4 bytes: 14 -> 16 (pad 2), 26 -> 28 (pad 2)
  EXPECT_EQ(layout.total_size(), 38);
}

TEST(LayoutBuilderTest, AllocatePrimitiveSequence) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive_sequence(XCdrPrimitiveKind::kUint16, 4);
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 1);

  // Total size = header (4) + length prefix (4) + 4 * uint16 (8)
  EXPECT_EQ(layout.total_size(), kXCdrHeaderSize + 4 + 4 * 2);
}

TEST(LayoutBuilderTest, AllocateNestedStruct) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder.begin_allocate_struct("position");
  builder.allocate_primitive("x", XCdrPrimitiveKind::kDouble);
  builder.allocate_primitive("y", XCdrPrimitiveKind::kDouble);
  builder.end_allocate_struct();
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 2);

  auto m0 = layout.get_member(0);
  ASSERT_TRUE(m0);
  EXPECT_EQ(m0->get().name(), "id");

  auto m1 = layout.get_member(1);
  ASSERT_TRUE(m1);
  EXPECT_EQ(m1->get().name(), "position");

  // With CDR data-relative alignment:
  // Total size = header (4) + uint32 at offset 0 (4) + 4 padding
  // for 8-byte aligned struct + struct(double (8) + double (8)) = 28
  EXPECT_EQ(layout.total_size(), 28);
}

TEST(LayoutBuilderTest, UnnamedFieldsHaveEmptyNames) {
  XCdrLayoutBuilder builder;

  // Unnamed fields are not registered in the name-to-index map
  builder.allocate_primitive(XCdrPrimitiveKind::kUint32);
  builder.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout1 = builder.finalize();

  EXPECT_EQ(layout1.member_count(), 2);
  auto m0 = layout1.get_member(0);
  ASSERT_TRUE(m0);
  EXPECT_EQ(m0->get().name(), "");

  auto m1 = layout1.get_member(1);
  ASSERT_TRUE(m1);
  EXPECT_EQ(m1->get().name(), "");

  // Unnamed fields cannot be looked up by name
  EXPECT_FALSE(layout1.get_member("a"));

  // Reuse the builder for a second layout
  builder.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout2 = builder.finalize();

  EXPECT_EQ(layout2.member_count(), 1);
  auto m0_2 = layout2.get_member(0);
  ASSERT_TRUE(m0_2);
  EXPECT_EQ(m0_2->get().name(), "");
}

TEST(LayoutBuilderTest, MixNamedAndUnnamed) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder.allocate_primitive(XCdrPrimitiveKind::kDouble);  // Unnamed
  builder.allocate_string("name", 10);
  builder.allocate_primitive(XCdrPrimitiveKind::kUint8);  // Unnamed
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 4);

  auto m0 = layout.get_member(0);
  ASSERT_TRUE(m0);
  EXPECT_EQ(m0->get().name(), "id");

  auto m1 = layout.get_member(1);
  ASSERT_TRUE(m1);
  EXPECT_EQ(m1->get().name(), "");  // Unnamed

  auto m2 = layout.get_member(2);
  ASSERT_TRUE(m2);
  EXPECT_EQ(m2->get().name(), "name");

  auto m3 = layout.get_member(3);
  ASSERT_TRUE(m3);
  EXPECT_EQ(m3->get().name(), "");  // Unnamed

  // Only named members are reachable by name
  EXPECT_TRUE(layout.get_member("id"));
  EXPECT_TRUE(layout.get_member("name"));
  EXPECT_FALSE(layout.get_member("field_1"));
  EXPECT_FALSE(layout.get_member("field_3"));
}

TEST(LayoutBuilderTest, ApplyLayoutToBuffer) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive(XCdrPrimitiveKind::kUint32);
  builder.allocate_string(5);
  auto layout = builder.finalize();

  // Create buffer
  std::vector<uint8_t> buffer(layout.total_size());
  auto status = layout.apply(buffer);
  ASSERT_TRUE(status);

  // Check header is written
  EXPECT_EQ(buffer[0], 0x00);  // Little endian marker
  EXPECT_EQ(buffer[1], 0x01);
}

TEST(LayoutBuilderTest, LayoutSizeMatchesWriterSize) {
  // Write data with writer
  XCdrWriter writer;
  writer.write(static_cast<uint32_t>(42));
  writer.write(std::string_view("hello"));
  auto buffer = writer.flush();

  // Build matching layout
  XCdrLayoutBuilder builder;
  builder.allocate_primitive(XCdrPrimitiveKind::kUint32);
  builder.allocate_string(5);
  auto layout = builder.finalize();

  // Layout size should match writer buffer size
  EXPECT_EQ(layout.total_size(), buffer.size());
}

TEST(LayoutBuilderTest, NestedSequenceOfStructsRetainsMembersAndPrefixes) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive("prefix", XCdrPrimitiveKind::kUint8);
  builder.begin_allocate_struct("holder");
  builder.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder.allocate_primitive_array("bytes", XCdrPrimitiveKind::kUint8, 3);
  builder.begin_allocate_sequence("messages", 3);
  builder.begin_allocate_struct();
  builder.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder.allocate_string("name", 3);
  builder.allocate_primitive_sequence("data", XCdrPrimitiveKind::kUint8, 2);
  builder.begin_allocate_sequence("empty", 0);
  builder.begin_allocate_struct();
  builder.allocate_primitive("value", XCdrPrimitiveKind::kDouble);
  builder.end_allocate_struct();
  builder.end_allocate_sequence();
  builder.end_allocate_struct();
  builder.end_allocate_sequence();
  builder.allocate_primitive("tail", XCdrPrimitiveKind::kUint32);
  builder.end_allocate_struct();
  auto layout = builder.finalize();
  ASSERT_EQ(layout.member_count(), 2u);
  std::vector<uint8_t> buffer(layout.total_size(), 0xFF);
  ASSERT_TRUE(layout.apply(buffer, true));
  auto accessor = *XCdrConstAccessor::wrap(buffer, layout);
  auto holder = accessor["holder"];
  ASSERT_EQ(*holder["bytes"].size(), 3u);
  auto messages = holder["messages"];
  ASSERT_EQ(*messages.size(), 3u);
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(messages[i]["name"].as<std::string_view>(), std::string(3, '\0'));
    EXPECT_EQ(*messages[i]["data"].size(), 2u);
    EXPECT_EQ(*messages[i]["empty"].size(), 0u);
    EXPECT_EQ(messages[i]["id"].as<uint32_t>(), 0u);
  }
  EXPECT_EQ(holder["tail"].as<uint32_t>(), 0u);
}

TEST(LayoutBuilderTest, UniformStructSequenceSpanIncludesInitialPadding) {
  XCdrLayoutBuilder builder;
  builder.begin_allocate_sequence("values", 3);
  builder.begin_allocate_struct();
  builder.allocate_primitive("value", XCdrPrimitiveKind::kDouble);
  builder.allocate_primitive("tag", XCdrPrimitiveKind::kUint8);
  builder.end_allocate_struct();
  builder.end_allocate_sequence();
  auto layout = builder.finalize();
  // Header + prefix + initial alignment + two strides + unpadded final element.
  EXPECT_EQ(layout.total_size(), 4u + 4u + 4u + 2u * 16u + 9u);
  std::vector<uint8_t> buffer(layout.total_size());
  ASSERT_TRUE(layout.apply(buffer));
  auto accessor = *XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_EQ(*accessor["values"].size(), 3u);
  EXPECT_EQ(accessor["values"][2]["tag"].slice().data() + 1,
    buffer.data() + buffer.size());
}

TEST(LayoutBuilderTest, EmptyStructSequenceDoesNotOccupyTemplateSpace) {
  XCdrLayoutBuilder builder;
  builder.begin_allocate_sequence("values", 0);
  builder.begin_allocate_struct();
  builder.allocate_primitive("value", XCdrPrimitiveKind::kDouble);
  builder.end_allocate_struct();
  builder.end_allocate_sequence();
  builder.allocate_primitive("tail", XCdrPrimitiveKind::kUint32);
  auto layout = builder.finalize();
  EXPECT_EQ(layout.total_size(), 12u);
  ASSERT_EQ(layout.member_count(), 2u);
  std::vector<uint8_t> buffer(layout.total_size(), 0xFF);
  ASSERT_TRUE(layout.apply(buffer, true));
  auto accessor = *XCdrConstAccessor::wrap(buffer, layout);
  EXPECT_EQ(*accessor["values"].size(), 0u);
  EXPECT_EQ(accessor["tail"].as<uint32_t>(), 0u);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
