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

#include "xcdr_buffers/accessor/const_accessor.hpp"
#include "xcdr_buffers/layout/layout_builder.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

using namespace xcdr_buffers;  // NOLINT(build/namespaces)

class ConstAccessorTest : public ::testing::Test
{
protected:
  XCdrWriter writer_;
  XCdrLayoutBuilder builder_{};
};

TEST_F(ConstAccessorTest, WrapValidatesEndianness)
{
  // Create a simple buffer with correct endianness
  writer_.write(static_cast<uint32_t>(42));
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout_result = builder_.finalize();
  // Layout finalized successfully

  // Should succeed
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout_result);
  ASSERT_TRUE(accessor_result);
}

TEST_F(ConstAccessorTest, WrapFailsOnSmallBuffer)
{
  std::vector<uint8_t> small_buffer = {0x00, 0x01};

  auto layout = builder_.finalize();

  auto accessor_result = XCdrConstAccessor::wrap(small_buffer, layout);
  EXPECT_FALSE(accessor_result);
  EXPECT_NE(accessor_result.error().message().find("too small"), std::string::npos);
}

TEST_F(ConstAccessorTest, AccessPrimitiveField)
{
  // Write struct with uint32 field
  writer_.write(static_cast<uint32_t>(12345));
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout = builder_.finalize();

  // Access the field
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  uint32_t value = accessor["field_0"].as<uint32_t>();
  EXPECT_EQ(value, 12345u);
}

TEST_F(ConstAccessorTest, AccessMultiplePrimitives)
{
  // Write multiple primitives
  writer_.write(static_cast<uint32_t>(42));
  writer_.write(3.14);
  writer_.write(static_cast<uint8_t>(255));
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder_.allocate_primitive("value", XCdrPrimitiveKind::kDouble);
  builder_.allocate_primitive("flag", XCdrPrimitiveKind::kUint8);
  auto layout = builder_.finalize();

  // Access fields
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  EXPECT_EQ(accessor["id"].as<uint32_t>(), 42u);
  EXPECT_NEAR(accessor["value"].as<double>(), 3.14, 0.001);
  EXPECT_EQ(accessor["flag"].as<uint8_t>(), 255);
}

TEST_F(ConstAccessorTest, AccessStringField)
{
  // Write struct with string field
  writer_.write(std::string_view("Hello, World!"));
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_string(13);  // "Hello, World!" length
  auto layout = builder_.finalize();

  // Access the string
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  std::string_view value = accessor["field_0"].as<std::string_view>();
  EXPECT_EQ(value, "Hello, World!");
}

TEST_F(ConstAccessorTest, AccessNamedField)
{
  // Write struct with named fields
  writer_.write(static_cast<uint32_t>(42));
  writer_.write(std::string_view("test"));
  auto buffer = writer_.flush();

  // Build layout with field names
  builder_.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder_.allocate_string("name", 4);
  auto layout = builder_.finalize();

  // Access by name
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;

  auto id_result = accessor.member("id");
  ASSERT_TRUE(id_result);
  EXPECT_EQ(id_result->as<uint32_t>(), 42u);

  auto name_result = accessor.member("name");
  ASSERT_TRUE(name_result);
  EXPECT_EQ(name_result->as<std::string_view>(), "test");
}

TEST_F(ConstAccessorTest, AccessNestedStruct)
{
  // Write nested struct
  writer_.write(static_cast<uint32_t>(10));
  writer_.begin_write_struct();
  writer_.write(static_cast<double>(1.5));
  writer_.write(static_cast<double>(2.5));
  writer_.end_write_struct();
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder_.begin_allocate_struct("position");
  builder_.allocate_primitive("x", XCdrPrimitiveKind::kDouble);
  builder_.allocate_primitive("y", XCdrPrimitiveKind::kDouble);
  builder_.end_allocate_struct();
  auto layout = builder_.finalize();

  // Access nested fields
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;

  EXPECT_EQ(accessor["id"].as<uint32_t>(), 10u);

  auto position = accessor["position"];
  EXPECT_NEAR(position["x"].as<double>(), 1.5, 0.001);
  EXPECT_NEAR(position["y"].as<double>(), 2.5, 0.001);
}

TEST_F(ConstAccessorTest, AccessNestedFieldWithPath)
{
  // Write nested struct
  writer_.begin_write_struct();
  writer_.begin_write_struct();
  writer_.write(static_cast<uint32_t>(999));
  writer_.end_write_struct();
  writer_.end_write_struct();
  auto buffer = writer_.flush();

  // Build layout
  builder_.begin_allocate_struct("middle");
  builder_.begin_allocate_struct("inner");
  builder_.allocate_primitive("value", XCdrPrimitiveKind::kUint32);
  builder_.end_allocate_struct();
  builder_.end_allocate_struct();
  auto layout = builder_.finalize();

  // Access using path syntax
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto value_result = accessor.member("middle.inner.value");
  ASSERT_TRUE(value_result);
  EXPECT_EQ(value_result->as<uint32_t>(), 999u);
}

TEST_F(ConstAccessorTest, AccessPrimitiveArray)
{
  // Write array
  std::vector<uint32_t> values = {10, 20, 30, 40, 50};
  writer_.write_array(values);
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive_array(XCdrPrimitiveKind::kUint32, 5);
  auto layout = builder_.finalize();

  // Access array elements
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto array_accessor = accessor["field_0"];

  auto size_result = array_accessor.size();
  ASSERT_TRUE(size_result);
  EXPECT_EQ(*size_result, 5);

  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(array_accessor[i].as<uint32_t>(), values[i]);
  }
}

TEST_F(ConstAccessorTest, IterateOverPrimitiveArray)
{
  // Write array
  std::vector<uint32_t> values = {1, 2, 3};
  writer_.write_array(values);
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive_array(XCdrPrimitiveKind::kUint32, 3);
  auto layout = builder_.finalize();

  // Iterate
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto array_accessor = accessor["field_0"];

  std::vector<uint32_t> retrieved;
  for (auto elem : array_accessor) {
    retrieved.push_back(elem.as<uint32_t>());
  }

  EXPECT_EQ(retrieved, values);
}

TEST_F(ConstAccessorTest, AccessStringArray)
{
  // Write string array
  writer_.begin_write_array(3);
  writer_.write(std::string_view("one"));
  writer_.write(std::string_view("two"));
  writer_.write(std::string_view("three"));
  writer_.end_write_array();
  auto buffer = writer_.flush();

  // Build layout
  builder_.begin_allocate_array(3);
  builder_.allocate_string(3);
  builder_.allocate_string(3);
  builder_.allocate_string(5);
  builder_.end_allocate_array();
  auto layout = builder_.finalize();

  // Access string array
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto array_accessor = accessor["field_0"];

  EXPECT_EQ(*array_accessor.size(), 3);
  EXPECT_EQ(array_accessor[0].as<std::string_view>(), "one");
  EXPECT_EQ(array_accessor[1].as<std::string_view>(), "two");
  EXPECT_EQ(array_accessor[2].as<std::string_view>(), "three");
}

TEST_F(ConstAccessorTest, AccessPrimitiveSequence)
{
  // Write sequence
  std::vector<double> values = {1.1, 2.2, 3.3, 4.4};
  writer_.write_sequence(values);
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive_sequence(XCdrPrimitiveKind::kDouble, 4);
  auto layout = builder_.finalize();

  // Access sequence
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto seq_accessor = accessor["field_0"];

  EXPECT_EQ(*seq_accessor.size(), 4);
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_NEAR(seq_accessor[i].as<double>(), values[i], 0.001);
  }
}

TEST_F(ConstAccessorTest, AccessStringSequence)
{
  // Write string sequence
  writer_.begin_write_sequence(2);
  writer_.write(std::string_view("hello"));
  writer_.write(std::string_view("world"));
  writer_.end_write_sequence();
  auto buffer = writer_.flush();

  // Build layout
  builder_.begin_allocate_sequence(2);
  builder_.allocate_string(5);
  builder_.allocate_string(5);
  builder_.end_allocate_sequence();
  auto layout = builder_.finalize();

  // Access
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto seq_accessor = accessor["field_0"];

  EXPECT_EQ(*seq_accessor.size(), 2);
  EXPECT_EQ(seq_accessor[0].as<std::string_view>(), "hello");
  EXPECT_EQ(seq_accessor[1].as<std::string_view>(), "world");
}

TEST_F(ConstAccessorTest, ErrorOnWrongType)
{
  // Write uint32
  writer_.write(static_cast<uint32_t>(42));
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout = builder_.finalize();

  // Try to access as wrong type
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  EXPECT_THROW(accessor["field_0"].as<double>(), XCdrError);
  EXPECT_THROW(accessor["field_0"].as<std::string_view>(), XCdrError);
}

TEST_F(ConstAccessorTest, ErrorOnArrayOutOfBounds)
{
  // Write array with 3 elements
  std::vector<uint32_t> values = {1, 2, 3};
  writer_.write_array(tcb::span<const uint32_t>(values));
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive_array(XCdrPrimitiveKind::kUint32, 3);
  auto layout = builder_.finalize();

  // Try to access out of bounds
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto array_accessor = accessor["field_0"];

  auto result = array_accessor.item(3);  // Out of bounds
  EXPECT_FALSE(result);
  EXPECT_NE(result.error().message().find("out of bounds"), std::string::npos);

  EXPECT_THROW(array_accessor[3], XCdrError);
}

TEST_F(ConstAccessorTest, ErrorOnMemberNotFound)
{
  // Write struct
  writer_.write(static_cast<uint32_t>(42));
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive("value", XCdrPrimitiveKind::kUint32);
  auto layout = builder_.finalize();

  // Try to access non-existent field
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto result = accessor.member("nonexistent");
  EXPECT_FALSE(result);
  EXPECT_NE(result.error().message().find("not found"), std::string::npos);
}

TEST_F(ConstAccessorTest, SliceReturnsRawBuffer)
{
  // Write primitive
  writer_.write(static_cast<uint32_t>(0x12345678));
  auto buffer = writer_.flush();

  // Build layout
  builder_.allocate_primitive(XCdrPrimitiveKind::kUint32);
  auto layout = builder_.finalize();

  // Get slice
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto slice = accessor["field_0"].slice();

  EXPECT_EQ(slice.size(), 4);  // uint32 is 4 bytes

  // Verify bytes (little endian)
  EXPECT_EQ(slice[0], 0x78);
  EXPECT_EQ(slice[1], 0x56);
  EXPECT_EQ(slice[2], 0x34);
  EXPECT_EQ(slice[3], 0x12);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
