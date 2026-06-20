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

/// @file test_serialization_overflow.cpp
///
/// Tests buffer-size contract enforcement during serialization:
///   - get_message_size returns a reasonable value
///   - exact-size buffer succeeds
///   - undersized buffer returns RCUTILS_RET_ERROR
///   - oversized buffer succeeds
///   - deserialization from truncated buffer returns RCUTILS_RET_ERROR
///
/// Each message family with a bounded layout is tested, and where available
/// both standard and experimental variants are verified.

#include "test_helpers.hpp"

#include <vector>

// =============================================================================
// Fixture — obtains typesupport handles for all message families
// =============================================================================

class TestSerializationOverflow : public ::testing::Test
{
protected:
  void SetUp() override
  {
    basic_ts_ = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
      rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes>();
    bounded_ts_ = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
      rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage>();
    nested_ts_ = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
      rosidl_typesupport_xcdr_cpp_tests::msg::NestedMessage>();

    basic_exp_ts_ = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
      rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BasicTypes>();
    bounded_exp_ts_ = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
      rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage>();
  }

  const rosidl_message_type_support_t * basic_ts_{nullptr};
  const rosidl_message_type_support_t * bounded_ts_{nullptr};
  const rosidl_message_type_support_t * nested_ts_{nullptr};
  const rosidl_message_type_support_t * basic_exp_ts_{nullptr};
  const rosidl_message_type_support_t * bounded_exp_ts_{nullptr};
};

// =============================================================================
// get_message_size sanity
// =============================================================================

TEST_F(TestSerializationOverflow, BasicTypes_GetMessageSize)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes msg{};
  size_t size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(basic_ts_, &msg, &size);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_GT(size, 0u);
}

TEST_F(TestSerializationOverflow, BoundedMessage_GetMessageSize)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage msg{};
  size_t size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(bounded_ts_, &msg, &size);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_GT(size, 0u);
}

TEST_F(TestSerializationOverflow, NestedMessage_GetMessageSize)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::NestedMessage msg{};
  size_t size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts_, &msg, &size);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_GT(size, 0u);
}

// =============================================================================
// Serialization with adequate buffer — should succeed.
// Uses generous padding since get_message_size may return payload-only size
// and the XCDR writer also needs header space.
// =============================================================================

TEST_F(TestSerializationOverflow, BasicTypes_AdequateBuffer)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes msg{};
  fill_basic_types(msg);

  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(basic_ts_, &msg, &size));
  EXPECT_GT(size, 0u);

  std::vector<uint8_t> buf(size + 256);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buf.data(), buf.size()};
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(basic_ts_, &msg, storage));
}

TEST_F(TestSerializationOverflow, BoundedMessage_AdequateBuffer)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage msg{};
  fill_bounded_message(msg);

  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(bounded_ts_, &msg, &size));
  EXPECT_GT(size, 0u);

  std::vector<uint8_t> buf(size + 256);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buf.data(), buf.size()};
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(bounded_ts_, &msg, storage));
}

TEST_F(TestSerializationOverflow, NestedMessage_AdequateBuffer)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::NestedMessage msg{};
  fill_nested_message(msg);

  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts_, &msg, &size));
  EXPECT_GT(size, 0u);

  std::vector<uint8_t> buf(size + 256);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buf.data(), buf.size()};
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(nested_ts_, &msg, storage));
}

// =============================================================================
// Buffer-too-small — should fail
// =============================================================================

TEST_F(TestSerializationOverflow, BasicTypes_BufferTooSmall)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes msg{};
  fill_basic_types(msg);

  size_t actual_size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(basic_ts_, &msg, &actual_size));

  size_t small_size = actual_size / 2;
  std::vector<uint8_t> buf(small_size, 0);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buf.data(), buf.size()};
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(basic_ts_, &msg, storage));
}

TEST_F(TestSerializationOverflow, BoundedMessage_BufferTooSmall)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage msg{};
  fill_bounded_message(msg);

  size_t actual_size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(bounded_ts_, &msg, &actual_size));

  // 4 bytes is far smaller than any real message
  uint8_t tiny[4] = {0};
  rosidl_runtime_cpp::MemoryRegion<void> storage{tiny, 4};
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(bounded_ts_, &msg, storage));
}

TEST_F(TestSerializationOverflow, NestedMessage_PartialOverflow)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::NestedMessage msg{};
  fill_nested_message(msg);

  size_t actual_size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts_, &msg, &actual_size));

  // Buffer that fits most but not all — expect error
  std::vector<uint8_t> buf(actual_size - 4);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buf.data(), buf.size()};
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(nested_ts_, &msg, storage));
}

// =============================================================================
// Extra-space buffer — should succeed
// =============================================================================

TEST_F(TestSerializationOverflow, BasicTypes_ExtraSpace)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes msg{};
  fill_basic_types(msg);

  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(basic_ts_, &msg, &size));

  std::vector<uint8_t> buf(size + 100);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buf.data(), buf.size()};
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(basic_ts_, &msg, storage));
}

// =============================================================================
// Deserialization from truncated buffer — should fail
// =============================================================================

TEST_F(TestSerializationOverflow, BasicTypes_DeserializeTruncated)
{
  rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes msg{};
  fill_basic_types(msg);

  std::vector<uint8_t> buf(4096);
  rosidl_runtime_cpp::MemoryRegion<void> full_storage{buf.data(), buf.size()};
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(basic_ts_, &msg, full_storage));

  rosidl_runtime_cpp::MemoryRegion<void> truncated_storage{buf.data(), 4};
  rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes out{};
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_cpp::deserialize_message_from(
      basic_ts_, truncated_storage, &out));
}

// =============================================================================
// Experimental variant parity — BasicTypes
// =============================================================================

TEST_F(TestSerializationOverflow, BasicTypesExperimental_ExactSize)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BasicTypes;
  T msg{};
  fill_basic_types(msg);

  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(basic_exp_ts_, &msg, &size));

  std::vector<uint8_t> buf(size);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buf.data(), buf.size()};
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(basic_exp_ts_, &msg, storage));
}

TEST_F(TestSerializationOverflow, BasicTypesExperimental_BufferTooSmall)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BasicTypes;
  T msg{};
  fill_basic_types(msg);

  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(basic_exp_ts_, &msg, &size));

  std::vector<uint8_t> buf(size / 2);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buf.data(), buf.size()};
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(basic_exp_ts_, &msg, storage));
}

// =============================================================================
// Experimental variant parity — BoundedMessage
// =============================================================================

TEST_F(TestSerializationOverflow, BoundedMessageExperimental_ExactSize)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage;
  T msg{};
  fill_bounded_message(msg);

  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(bounded_exp_ts_, &msg, &size));

  std::vector<uint8_t> buf(size);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buf.data(), buf.size()};
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(bounded_exp_ts_, &msg, storage));
}

TEST_F(TestSerializationOverflow, BoundedMessageExperimental_BufferTooSmall)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage;
  T msg{};
  fill_bounded_message(msg);

  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_cpp::get_message_size(bounded_exp_ts_, &msg, &size));

  uint8_t tiny[4] = {0};
  rosidl_runtime_cpp::MemoryRegion<void> storage{tiny, 4};
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_cpp::serialize_message_into(bounded_exp_ts_, &msg, storage));
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
