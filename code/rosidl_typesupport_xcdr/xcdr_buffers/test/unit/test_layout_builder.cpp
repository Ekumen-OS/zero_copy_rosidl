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

#include "xcdr_buffers/layout/layout_builder.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

using namespace xcdr_buffers;  // NOLINT(build/namespaces)

TEST(LayoutBuilderTest, AllocateSinglePrimitive) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout = builder.finalize();

  // Should have 1 member with auto-generated name
  EXPECT_EQ(layout.member_count(), 1);

  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  const auto & member = member_result->get();
  EXPECT_EQ(member.name(), "field_0");

  // Total size = header (4) + uint32 (4)
  EXPECT_EQ(layout.total_size(), kXCdrHeaderSize + 4);
}

TEST(LayoutBuilderTest, AllocateMultiplePrimitives) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive(XCdrPrimitiveKind::kUint32);
  builder.allocate_primitive(XCdrPrimitiveKind::kDouble);
  builder.allocate_primitive(XCdrPrimitiveKind::kUint8);
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 3);

  auto m0 = layout.get_member(0);
  ASSERT_TRUE(m0);
  EXPECT_EQ(m0->get().name(), "field_0");

  auto m1 = layout.get_member(1);
  ASSERT_TRUE(m1);
  EXPECT_EQ(m1->get().name(), "field_1");

  auto m2 = layout.get_member(2);
  ASSERT_TRUE(m2);
  EXPECT_EQ(m2->get().name(), "field_2");

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
  builder.allocate_string(10);
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 1);

  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  EXPECT_EQ(member_result->get().name(), "field_0");

  // Total size = header (4) + length prefix (4) + string (10) + null (1)
  EXPECT_EQ(layout.total_size(), kXCdrHeaderSize + 4 + 10 + 1);
}

TEST(LayoutBuilderTest, AllocatePrimitiveArray) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive_array(XCdrPrimitiveKind::kUint32, 5);
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 1);

  auto member_result = layout.get_member(0);
  ASSERT_TRUE(member_result);
  EXPECT_EQ(member_result->get().name(), "field_0");

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
  builder.begin_allocate_array(3);
  builder.allocate_string(5);
  builder.allocate_string(5);
  builder.allocate_string(5);
  builder.end_allocate_array();
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 1);

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

TEST(LayoutBuilderTest, FieldCounterResets) {
  XCdrLayoutBuilder builder;

  // First build
  builder.allocate_primitive(XCdrPrimitiveKind::kUint32);
  builder.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout1 = builder.finalize();

  EXPECT_EQ(layout1.member_count(), 2);
  auto m0 = layout1.get_member(0);
  ASSERT_TRUE(m0);
  EXPECT_EQ(m0->get().name(), "field_0");

  auto m1 = layout1.get_member(1);
  ASSERT_TRUE(m1);
  EXPECT_EQ(m1->get().name(), "field_1");

  // Second build - field counter should reset
  builder.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout2 = builder.finalize();

  EXPECT_EQ(layout2.member_count(), 1);
  auto m0_2 = layout2.get_member(0);
  ASSERT_TRUE(m0_2);
  EXPECT_EQ(m0_2->get().name(), "field_0");  // Should restart at field_0
}

TEST(LayoutBuilderTest, MixNamedAndUnnamed) {
  XCdrLayoutBuilder builder;
  builder.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder.allocate_primitive(XCdrPrimitiveKind::kDouble);  // Auto-named
  builder.allocate_string("name", 10);
  builder.allocate_primitive(XCdrPrimitiveKind::kUint8);  // Auto-named
  auto layout = builder.finalize();

  EXPECT_EQ(layout.member_count(), 4);

  auto m0 = layout.get_member(0);
  ASSERT_TRUE(m0);
  EXPECT_EQ(m0->get().name(), "id");

  auto m1 = layout.get_member(1);
  ASSERT_TRUE(m1);
  EXPECT_EQ(m1->get().name(), "field_1");  // Auto-generated

  auto m2 = layout.get_member(2);
  ASSERT_TRUE(m2);
  EXPECT_EQ(m2->get().name(), "name");

  auto m3 = layout.get_member(3);
  ASSERT_TRUE(m3);
  EXPECT_EQ(m3->get().name(), "field_3");  // Auto-generated
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

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
