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
#include "xcdr_buffers/accessor/const_accessor.hpp"
#include "xcdr_buffers/layout/layout_builder.hpp"
#include "xcdr_buffers/layout/layout_parser.hpp"
#include "xcdr_buffers/serialization/reader.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

using namespace xcdr_buffers;  // NOLINT(build/namespaces)

class AccessorIntegrationTest : public ::testing::Test
{
};

TEST_F(AccessorIntegrationTest, WriterParserConstAccessorRead)
{
  // Write data using XCdrWriter (implicit top-level struct)
  XCdrWriter writer;
  writer.write(static_cast<uint32_t>(42));
  writer.write(std::string_view("hello"));
  writer.write(static_cast<double>(3.14));
  auto buffer = writer.flush();

  // Parse layout using XCdrLayoutParser (implicit top-level struct)
  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  auto layout = *parser.finalize();

  // Read using XCdrConstAccessor
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  EXPECT_EQ((accessor[0].as<uint32_t>()), 42u);
  EXPECT_EQ((accessor[1].as<std::string_view>()), "hello");
  EXPECT_NEAR((accessor[2].as<double>()), 3.14, 0.001);
}

TEST_F(AccessorIntegrationTest, BuilderAccessorWriteReaderVerify)
{
  // Build layout using XCdrLayoutBuilder (implicit top-level struct)
  XCdrLayoutBuilder builder;
  builder.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder.allocate_string("name", 5);  // "world"
  builder.allocate_primitive("value", XCdrPrimitiveKind::kDouble);
  auto layout = builder.finalize();

  // Create buffer and write using XCdrAccessor
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  accessor[0] = 99u;
  accessor[1] = std::string_view("world");
  accessor[2] = 2.71828;

  // Verify using XCdrReader (implicit top-level struct)
  XCdrReader reader(buffer);
  EXPECT_EQ(*reader.read<uint32_t>(), 99u);
  EXPECT_EQ(reader.read<std::string_view>(), "world");
  EXPECT_NEAR(*reader.read<double>(), 2.71828, 0.00001);
}

TEST_F(AccessorIntegrationTest, FullRoundTripWriteReadModifyRead)
{
  // 1. Write initial data (implicit top-level struct)
  XCdrWriter writer;
  writer.write(static_cast<uint32_t>(100));
  writer.write(std::string_view("original"));
  auto buffer_vec = writer.flush();
  tcb::span<uint8_t> buffer(buffer_vec);

  // 2. Parse layout (implicit top-level struct)
  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.parse_string());
  auto layout = *parser.finalize();

  // 3. Read with const accessor
  {
    auto const_accessor_result = XCdrConstAccessor::wrap(buffer, layout);
    ASSERT_TRUE(const_accessor_result);
    auto const_accessor = *const_accessor_result;
    EXPECT_EQ((const_accessor[0].as<uint32_t>()), 100u);
    EXPECT_EQ((const_accessor[1].as<std::string_view>()), "original");
  }

  // 4. Modify with mutable accessor
  {
    auto accessor_result = XCdrAccessor::wrap(buffer, layout);
    ASSERT_TRUE(accessor_result);
    auto accessor = *accessor_result;
    accessor[0] = 200u;
    accessor[1] = std::string_view("modified");
  }

  // 5. Read again to verify modifications
  {
    auto const_accessor_result = XCdrConstAccessor::wrap(buffer, layout);
    ASSERT_TRUE(const_accessor_result);
    auto const_accessor = *const_accessor_result;
    EXPECT_EQ((const_accessor[0].as<uint32_t>()), 200u);
    EXPECT_EQ((const_accessor[1].as<std::string_view>()), "modified");
  }

  // 6. Verify with Reader (implicit top-level struct)
  XCdrReader reader(buffer);
  EXPECT_EQ(*reader.read<uint32_t>(), 200u);
  EXPECT_EQ(reader.read<std::string_view>(), "modified");
}

TEST_F(AccessorIntegrationTest, ComplexNestedStructureWithArrays)
{
  // Write complex structure (implicit top-level): { id, struct { x, y }, array[3] }
  std::vector<uint32_t> arr = {100, 200, 300};
  XCdrWriter writer;
  writer.write(static_cast<uint32_t>(1));
  writer.begin_write_struct();
  writer.write(static_cast<double>(10.0));
  writer.write(static_cast<double>(20.0));
  writer.end_write_struct();
  writer.write_array(tcb::span<const uint32_t>(arr));
  auto buffer = writer.flush();

  // Parse layout (implicit top-level struct)
  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.begin_parse_struct("position"));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  ASSERT_TRUE(parser.end_parse_struct());
  ASSERT_TRUE(parser.begin_parse_array(3));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.end_parse_array());
  auto layout = *parser.finalize();

  // Access nested structure and array
  auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;

  EXPECT_EQ((accessor[0].as<uint32_t>()), 1u);

  auto nested = accessor[1];
  EXPECT_NEAR((nested[0].as<double>()), 10.0, 0.001);
  EXPECT_NEAR((nested[1].as<double>()), 20.0, 0.001);

  auto array = accessor[2];
  EXPECT_EQ(*array.size(), 3);
  EXPECT_EQ((array[0].as<uint32_t>()), 100u);
  EXPECT_EQ((array[1].as<uint32_t>()), 200u);
  EXPECT_EQ((array[2].as<uint32_t>()), 300u);
}

TEST_F(AccessorIntegrationTest, SequenceModificationAndVerification)
{
  // Write sequence (implicit top-level struct)
  XCdrWriter writer;
  writer.write_sequence(tcb::span<const double>({1.1, 2.2, 3.3}));
  auto buffer_vec = writer.flush();
  tcb::span<uint8_t> buffer(buffer_vec);

  // Parse (implicit top-level struct)
  XCdrLayoutParser parser(buffer);

  ASSERT_TRUE(parser.parse_primitive_sequence(XCdrPrimitiveKind::kDouble, 3));
  auto layout = *parser.finalize();

  // Modify sequence elements
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto seq = accessor[0];

  seq[0] = 10.1;
  seq[1] = 20.2;
  seq[2] = 30.3;

  // Verify modifications
  EXPECT_NEAR((seq[0].as<double>()), 10.1, 0.001);
  EXPECT_NEAR((seq[1].as<double>()), 20.2, 0.001);
  EXPECT_NEAR((seq[2].as<double>()), 30.3, 0.001);

  // Verify with Reader (implicit top-level struct)
  XCdrReader reader(buffer);
  auto values = *reader.read<tcb::span<const double>>();
  EXPECT_EQ(values.size(), 3);
  EXPECT_NEAR(values[0], 10.1, 0.001);
  EXPECT_NEAR(values[1], 20.2, 0.001);
  EXPECT_NEAR(values[2], 30.3, 0.001);
}

TEST_F(AccessorIntegrationTest, StringArrayModification)
{
  // Build layout (implicit top-level struct)
  XCdrLayoutBuilder builder;
  builder.begin_allocate_array("strings", 3);
  builder.allocate_string(5);  // "aaaaa"
  builder.allocate_string(5);  // "bbbbb"
  builder.allocate_string(5);  // "ccccc"
  builder.end_allocate_array();
  auto layout = builder.finalize();

  // Create buffer and initialize
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write strings
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto array = accessor[0];

  array[0] = std::string_view("first");
  array[1] = std::string_view("secnd");
  array[2] = std::string_view("third");

  // Read back with const accessor
  auto const_accessor_result = XCdrConstAccessor::wrap(buffer, layout);
  ASSERT_TRUE(const_accessor_result);
  auto const_accessor = *const_accessor_result;
  auto const_array = const_accessor[0];

  EXPECT_EQ((const_array[0].as<std::string_view>()), "first");
  EXPECT_EQ((const_array[1].as<std::string_view>()), "secnd");
  EXPECT_EQ((const_array[2].as<std::string_view>()), "third");
}

TEST_F(AccessorIntegrationTest, IterationOverModifiedData)
{
  // Build layout with array (implicit top-level struct)
  XCdrLayoutBuilder builder;
  builder.allocate_primitive_array("data", XCdrPrimitiveKind::kUint32, 5);
  auto layout = builder.finalize();

  // Create buffer
  std::vector<uint8_t> buffer_vec(layout.total_size());
  tcb::span<uint8_t> buffer(buffer_vec);
  layout.apply(buffer);

  // Write using iterator
  auto accessor_result = XCdrAccessor::wrap(buffer, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;
  auto array = accessor[0];

  size_t value = 10;
  for (auto elem : array) {
    elem = static_cast<uint32_t>(value);
    value += 10;
  }

  // Read back using iterator
  std::vector<uint32_t> result;
  for (auto elem : array) {
    result.push_back((elem.as<uint32_t>()));
  }

  EXPECT_EQ(result, (std::vector<uint32_t>{10, 20, 30, 40, 50}));
}

TEST_F(AccessorIntegrationTest, WriterParserAccessorComplexStruct)
{
  // Write complex structure (implicit top-level struct)
  XCdrWriter writer;
  writer.write(static_cast<uint32_t>(12345));
  writer.write(std::string_view("robot"));
  writer.begin_write_struct();
  writer.write(static_cast<double>(1.5));
  writer.write(static_cast<double>(2.5));
  writer.end_write_struct();
  const std::vector<uint32_t> tag_ids = {10, 20, 30};
  writer.write_sequence(tcb::span(tag_ids));
  writer.begin_write_array(2);
  writer.write(std::string_view("tag1"));
  writer.write(std::string_view("tag2"));
  writer.end_write_array();
  auto buffer_vec = writer.flush();

  // Parse layout (implicit top-level struct)
  XCdrLayoutParser parser(buffer_vec);

  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kUint32));
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.begin_parse_struct("position"));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  ASSERT_TRUE(parser.parse_primitive(XCdrPrimitiveKind::kDouble));
  ASSERT_TRUE(parser.end_parse_struct());
  ASSERT_TRUE(parser.parse_primitive_sequence(XCdrPrimitiveKind::kUint32, 3));
  ASSERT_TRUE(parser.begin_parse_array(2));
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.parse_string());
  ASSERT_TRUE(parser.end_parse_array());
  auto layout = *parser.finalize();

  // Read using const accessor
  auto accessor_result = XCdrConstAccessor::wrap(buffer_vec, layout);
  ASSERT_TRUE(accessor_result);
  auto accessor = *accessor_result;

  EXPECT_EQ((accessor[0].as<uint32_t>()), 12345u);
  EXPECT_EQ((accessor[1].as<std::string_view>()), "robot");
  EXPECT_NEAR((accessor[2][0].as<double>()), 1.5, 0.001);
  EXPECT_NEAR((accessor[2][1].as<double>()), 2.5, 0.001);

  auto seq = accessor[3];
  EXPECT_EQ(*seq.size(), 3);
  EXPECT_EQ((seq[0].as<uint32_t>()), 10u);
  EXPECT_EQ((seq[1].as<uint32_t>()), 20u);
  EXPECT_EQ((seq[2].as<uint32_t>()), 30u);

  auto tags = accessor[4];
  EXPECT_EQ((tags[0].as<std::string_view>()), "tag1");
  EXPECT_EQ((tags[1].as<std::string_view>()), "tag2");

  // Modify using mutable accessor
  tcb::span<uint8_t> mut_buffer(buffer_vec);
  auto mut_accessor_result = XCdrAccessor::wrap(mut_buffer, layout);
  ASSERT_TRUE(mut_accessor_result);
  auto mut_accessor = *mut_accessor_result;

  mut_accessor[0] = 99999u;
  mut_accessor[3][1] = 999u;

  // Verify modifications
  EXPECT_EQ((mut_accessor[0].as<uint32_t>()), 99999u);
  EXPECT_EQ((mut_accessor[3][1].as<uint32_t>()), 999u);

  // Verify with Reader (implicit top-level struct)
  XCdrReader reader(buffer_vec);
  EXPECT_EQ(*reader.read<uint32_t>(), 99999u);  // Modified value
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
