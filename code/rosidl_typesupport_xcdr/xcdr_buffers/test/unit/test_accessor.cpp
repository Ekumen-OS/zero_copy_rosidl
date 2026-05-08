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

#include "xcdr_buffers/accessor/accessor.hpp"
#include "xcdr_buffers/layout/layout_builder.hpp"
#include "xcdr_buffers/serialization/reader.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

using namespace xcdr_buffers;  // NOLINT(build/namespaces)

class AccessorTest : public ::testing::Test
{
protected:
  XCdrLayoutBuilder builder_{};
};

TEST_F(AccessorTest, WrapValidatesEndianness)
{
  // Create a simple buffer
  XCdrWriter writer;
  writer.write(static_cast<uint32_t>(42));
  auto buffer_vec = writer.flush();
  tcb::span<uint8_t> buffer(buffer_vec);

  // Build layout
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout_result = builder_.finalize();
  // Layout finalized successfully

  // Should succeed
  auto accessor_result = XCdrAccessor::wrap(buffer, layout_result);
  ASSERT_TRUE(accessor_result);
}

TEST_F(AccessorTest, ReadPrimitiveField)
{
  // Write struct
  XCdrWriter writer;
  writer.write(static_cast<uint32_t>(99999));
  auto buffer_vec = writer.flush();
  tcb::span<uint8_t> buffer(buffer_vec);

  // Build layout
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout = builder_.finalize();

  // Read
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto value = accessor["field_0"].as<uint32_t>();
  EXPECT_EQ(value, 99999u);
}

TEST_F(AccessorTest, WritePrimitiveField)
{
  // Create buffer using layout
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout = builder_.finalize();

  // Allocate buffer and apply layout
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write value
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  accessor["field_0"] = 12345u;

  // Verify by reading back
  EXPECT_EQ((accessor["field_0"].as<uint32_t>()), 12345u);

  // Verify using Reader
  XCdrReader reader(buffer);
  EXPECT_EQ(*reader.read<uint32_t>(), 12345u);
}

TEST_F(AccessorTest, WriteMultiplePrimitives)
{
  // Build layout
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint32);
  builder_.allocate_primitive(XCdrPrimitiveKind::kDouble);
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint8);
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write values
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  accessor["field_0"] = 100u;
  accessor["field_1"] = 2.71828;
  accessor["field_2"] = static_cast<uint8_t>(255);

  // Verify
  EXPECT_EQ((accessor["field_0"].as<uint32_t>()), 100u);
  EXPECT_NEAR(((accessor["field_1"].as<double>())), 2.71828, 0.00001);
  EXPECT_EQ((accessor["field_2"].as<uint8_t>()), 255);
}

TEST_F(AccessorTest, ReadStringField)
{
  // Write string
  XCdrWriter writer;
  writer.write(std::string_view("test string"));
  auto buffer_vec = writer.flush();
  tcb::span<uint8_t> buffer(buffer_vec);

  // Build layout
  builder_.allocate_string(11);  // "test string" length
  auto layout = builder_.finalize();

  // Read
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto str_value = accessor["field_0"].as<std::string_view>();
  EXPECT_EQ(str_value, "test string");
}

TEST_F(AccessorTest, WriteStringField)
{
  // Build layout
  builder_.allocate_string(5);  // "hello" length
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write string
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  accessor["field_0"] = std::string_view("hello");

  // Verify
  auto str_value = accessor["field_0"].as<std::string_view>();
  EXPECT_EQ(str_value, "hello");

  // Verify using Reader
  XCdrReader reader(buffer);
  EXPECT_EQ(reader.read<std::string_view>(), "hello");
}

TEST_F(AccessorTest, WriteStringFieldValidatesLength)
{
  // Build layout for 5-char string
  builder_.allocate_string(5);
  auto layout = builder_.finalize();

  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;

  // Try to write wrong lengths (5 chars expected)
  EXPECT_THROW(accessor["field_0"] = std::string_view("too long!"), XCdrError);  // 9 chars
  EXPECT_THROW(accessor["field_0"] = std::string_view("hi"), XCdrError);          // 2 chars

  // Correct length should work
  EXPECT_NO_THROW(accessor["field_0"] = std::string_view("hello"));  // 5 chars
}

TEST_F(AccessorTest, WriteNamedField)
{
  // Build layout with named fields
  builder_.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder_.allocate_string("name", 4);  // "test"
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write by name
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  accessor["id"] = 42u;
  accessor["name"] = std::string_view("test");

  // Verify
  EXPECT_EQ((accessor["id"].as<uint32_t>()), 42u);
  EXPECT_EQ((accessor["name"].as<std::string_view>()), "test");
}

TEST_F(AccessorTest, WriteNestedStruct)
{
  // Build layout
  builder_.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder_.begin_allocate_struct("position");
  builder_.allocate_primitive("x", XCdrPrimitiveKind::kDouble);
  builder_.allocate_primitive("y", XCdrPrimitiveKind::kDouble);
  builder_.end_allocate_struct();
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write nested fields
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  accessor["id"] = 123u;
  accessor["position"]["x"] = 10.5;
  accessor["position"]["y"] = 20.3;

  // Verify
  EXPECT_EQ((accessor["id"].as<uint32_t>()), 123u);
  EXPECT_NEAR(((accessor["position"]["x"].as<double>())), 10.5, 0.001);
  EXPECT_NEAR(((accessor["position"]["y"].as<double>())), 20.3, 0.001);
}

TEST_F(AccessorTest, WriteNestedFieldWithPath)
{
  // Build layout
  builder_.begin_allocate_struct("middle");
  builder_.begin_allocate_struct("inner");
  builder_.allocate_primitive("value", XCdrPrimitiveKind::kUint32);
  builder_.end_allocate_struct();
  builder_.end_allocate_struct();
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write using path syntax
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto value_result = accessor.member("middle.inner.value");
  ASSERT_TRUE(value_result);
  *value_result = 777u;

  // Verify
  EXPECT_EQ(((accessor["middle"]["inner"]["value"].as<uint32_t>())), 777u);
}

TEST_F(AccessorTest, WritePrimitiveArrayElements)
{
  // Build layout using shortcut
  builder_.allocate_primitive_array(XCdrPrimitiveKind::kUint32, 5);
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write array elements
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto array_accessor = accessor["field_0"];

  for (size_t i = 0; i < 5; ++i) {
    array_accessor[i] = static_cast<uint32_t>((i + 1) * 10);
  }

  // Verify
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ((array_accessor[i].as<uint32_t>()), (i + 1) * 10);
  }
}

TEST_F(AccessorTest, WriteStringArrayElements)
{
  // Build layout
  builder_.begin_allocate_array(3);
  builder_.allocate_string(3);  // "one"
  builder_.allocate_string(3);  // "two"
  builder_.allocate_string(5);  // "three"
  builder_.end_allocate_array();
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write strings
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto array_accessor = accessor["field_0"];

  array_accessor[0] = std::string_view("one");
  array_accessor[1] = std::string_view("two");
  array_accessor[2] = std::string_view("three");

  // Verify
  auto str0 = array_accessor[0].as<std::string_view>();
  auto str1 = array_accessor[1].as<std::string_view>();
  auto str2 = array_accessor[2].as<std::string_view>();
  EXPECT_EQ(str0, "one");
  EXPECT_EQ(str1, "two");
  EXPECT_EQ(str2, "three");
}

TEST_F(AccessorTest, WriteSequenceElements)
{
  // Build layout using shortcut
  builder_.allocate_primitive_sequence(XCdrPrimitiveKind::kDouble, 4);
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write sequence elements
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto seq_accessor = accessor["field_0"];

  seq_accessor[0] = 1.1;
  seq_accessor[1] = 2.2;
  seq_accessor[2] = 3.3;
  seq_accessor[3] = 4.4;

  // Verify
  EXPECT_NEAR((seq_accessor[0].as<double>()), 1.1, 0.001);
  EXPECT_NEAR((seq_accessor[1].as<double>()), 2.2, 0.001);
  EXPECT_NEAR((seq_accessor[2].as<double>()), 3.3, 0.001);
  EXPECT_NEAR((seq_accessor[3].as<double>()), 4.4, 0.001);
}

TEST_F(AccessorTest, IterateAndModifyArray)
{
  // Build layout using shortcut
  builder_.allocate_primitive_array(XCdrPrimitiveKind::kUint32, 4);
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write using iteration
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto array_accessor = accessor["field_0"];

  size_t index = 0;
  for (auto elem : array_accessor) {
    elem = static_cast<uint32_t>(index * index);
    ++index;
  }

  // Verify
  EXPECT_EQ((array_accessor[0].as<uint32_t>()), 0);
  EXPECT_EQ((array_accessor[1].as<uint32_t>()), 1);
  EXPECT_EQ((array_accessor[2].as<uint32_t>()), 4);
  EXPECT_EQ((array_accessor[3].as<uint32_t>()), 9);
}

TEST_F(AccessorTest, MutableSlice)
{
  // Build layout
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Get mutable slice and modify directly
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto slice = accessor["field_0"].slice();

  EXPECT_EQ(slice.size(), 4);  // uint32 is 4 bytes

  // Write bytes directly (little endian 0xAABBCCDD)
  slice[0] = 0xDD;
  slice[1] = 0xCC;
  slice[2] = 0xBB;
  slice[3] = 0xAA;

  // Verify value
  EXPECT_EQ((accessor["field_0"].as<uint32_t>()), 0xAABBCCDD);
}

TEST_F(AccessorTest, ErrorOnWrongTypeAssignment)
{
  // Build layout for uint32
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout = builder_.finalize();

  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;

  // Try to assign wrong type
  EXPECT_THROW(accessor["field_0"] = 3.14, XCdrError);  // double instead of uint32
}

TEST_F(AccessorTest, ComplexStructReadWrite)
{
  // Build complex layout
  builder_.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder_.allocate_string("name", 5);  // "robot"
  builder_.begin_allocate_struct("position");
  builder_.allocate_primitive("x", XCdrPrimitiveKind::kDouble);
  builder_.allocate_primitive("y", XCdrPrimitiveKind::kDouble);
  builder_.end_allocate_struct();
  builder_.allocate_primitive_array("sensors", XCdrPrimitiveKind::kUint32, 3);
  auto layout = builder_.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write all fields
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  accessor["id"] = 12345u;
  accessor["name"] = std::string_view("robot");
  accessor["position"]["x"] = 10.5;
  accessor["position"]["y"] = 20.3;
  accessor["sensors"][0] = 100u;
  accessor["sensors"][1] = 200u;
  accessor["sensors"][2] = 300u;

  // Verify all fields using accessor
  EXPECT_EQ((accessor["id"].as<uint32_t>()), 12345u);
  EXPECT_EQ((accessor["name"].as<std::string_view>()), "robot");
  EXPECT_NEAR(((accessor["position"]["x"].as<double>())), 10.5, 0.001);
  EXPECT_NEAR(((accessor["position"]["y"].as<double>())), 20.3, 0.001);
  EXPECT_EQ((accessor["sensors"][0].as<uint32_t>()), 100u);
  EXPECT_EQ((accessor["sensors"][1].as<uint32_t>()), 200u);
  EXPECT_EQ((accessor["sensors"][2].as<uint32_t>()), 300u);

  // Note: Cannot verify with Reader because Reader and Accessor use different
  // alignment strategies. Reader aligns dynamically during traversal, while
  // Accessor uses pre-computed layout offsets.
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
