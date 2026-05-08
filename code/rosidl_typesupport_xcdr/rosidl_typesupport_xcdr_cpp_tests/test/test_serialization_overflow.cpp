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

#include <array>
#include <vector>

#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/basic_types.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/bounded_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/nested_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/detail/basic_types__rosidl_typesupport_xcdr_cpp.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/detail/bounded_message__rosidl_typesupport_xcdr_cpp.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/detail/nested_message__rosidl_typesupport_xcdr_cpp.hpp"

using namespace rosidl_typesupport_xcdr_cpp_tests::msg;  // NOLINT

class TestSerializationOverflow : public ::testing::Test
{
protected:
  void SetUp() override
  {
    basic_ts_ = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
      rosidl_typesupport_xcdr_cpp,
      rosidl_typesupport_xcdr_cpp_tests, msg,
      BasicTypes)();
    bounded_ts_ = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
      rosidl_typesupport_xcdr_cpp,
      rosidl_typesupport_xcdr_cpp_tests, msg,
      BoundedMessage)();
    nested_ts_ = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
      rosidl_typesupport_xcdr_cpp,
      rosidl_typesupport_xcdr_cpp_tests, msg,
      NestedMessage)();
  }

  const rosidl_message_type_support_t * basic_ts_;
  const rosidl_message_type_support_t * bounded_ts_;
  const rosidl_message_type_support_t * nested_ts_;
};

TEST_F(TestSerializationOverflow, SerializeBasicTypesExactSize)
{
  // Create a basic message
  BasicTypes msg;
  msg.bool_value = true;
  msg.uint8_value = 42;
  msg.float32_value = 3.14f;
  msg.float64_value = 2.71828;
  msg.int8_value = -10;
  msg.uint8_value = 200;
  msg.int16_value = -1000;
  msg.uint16_value = 50000;
  msg.int32_value = -100000;
  msg.uint32_value = 200000;
  msg.int64_value = -1000000;
  msg.uint64_value = 2000000;

  // Get exact size needed
  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(basic_ts_, &msg, &expected_size);
  ASSERT_EQ(ret, RCUTILS_RET_OK);
  
  // Allocate exact size buffer
  std::vector<uint8_t> buffer(expected_size);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  
  // Serialize should succeed with exact size
  ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(basic_ts_, &msg, storage);
  EXPECT_EQ(ret, RCUTILS_RET_OK);
}

TEST_F(TestSerializationOverflow, SerializeBasicTypesBufferTooSmall)
{
  // Create a basic message with ONE field set only
  BasicTypes msg{};
  msg.bool_value = true;
  // All other fields are zero
  
  // First, get the actual size needed
  size_t actual_size = 0;
  auto size_ret = rosidl_typesupport_xcdr_cpp::get_message_size(basic_ts_, &msg, &actual_size);
  ASSERT_EQ(size_ret, RCUTILS_RET_OK);
  
  // Allocate buffer that's too small (half the needed size)
  size_t buffer_size = actual_size / 2;
  std::vector<uint8_t> buffer(buffer_size, 0);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  
  // Serialize should fail with buffer too small
  auto ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(basic_ts_, &msg, storage);
  EXPECT_EQ(ret, RCUTILS_RET_ERROR) << "Buffer size: " << buffer_size << ", needed: " << actual_size;
}

TEST_F(TestSerializationOverflow, SerializeBoundedMessageExactSize)
{
  // Create a bounded message
  BoundedMessage msg;
  msg.id = 42;
  msg.value = 3.14;
  for (size_t i = 0; i < 10; ++i) {
    msg.data[i] = static_cast<uint8_t>(i * 10);
  }
  
  // Get exact size needed
  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(bounded_ts_, &msg, &expected_size);
  ASSERT_EQ(ret, RCUTILS_RET_OK);
  
  // Allocate exact size buffer
  std::vector<uint8_t> buffer(expected_size);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  
  // Serialize should succeed
  ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(bounded_ts_, &msg, storage);
  EXPECT_EQ(ret, RCUTILS_RET_OK);
}

TEST_F(TestSerializationOverflow, SerializeBoundedMessageOverflow)
{
  // Create a bounded message
  BoundedMessage msg;
  msg.id = 100;
  msg.value = 2.5;
  for (size_t i = 0; i < 10; ++i) {
    msg.data[i] = static_cast<uint8_t>(i);
  }
  
  // Get exact size needed
  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(bounded_ts_, &msg, &expected_size);
  ASSERT_EQ(ret, RCUTILS_RET_OK);
  
  // Allocate buffer that's way too small (just header size)
  uint8_t buffer[4] = {0};  // Only XCDR header
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer, 4};
  
  // Serialize should fail
  ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(bounded_ts_, &msg, storage);
  EXPECT_EQ(ret, RCUTILS_RET_ERROR);
}

TEST_F(TestSerializationOverflow, SerializeNestedMessageExactSize)
{
  // Create nested message
  NestedMessage msg;
  msg.name = "outer";
  msg.inner.id = 42;
  msg.inner.value = 3.14;
  msg.count = 100;
  
  // Get exact size needed
  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts_, &msg, &expected_size);
  ASSERT_EQ(ret, RCUTILS_RET_OK);
  
  // Allocate exact size buffer
  std::vector<uint8_t> buffer(expected_size);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  
  // Serialize should succeed
  ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(nested_ts_, &msg, storage);
  EXPECT_EQ(ret, RCUTILS_RET_OK);
}

TEST_F(TestSerializationOverflow, SerializeNestedMessagePartialOverflow)
{
  // Create nested message
  NestedMessage msg;
  msg.name = "test";
  msg.inner.id = 42;
  msg.inner.value = 3.14;
  msg.count = 100;
  
  // Get exact size needed
  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts_, &msg, &expected_size);
  ASSERT_EQ(ret, RCUTILS_RET_OK);
  
  // Allocate buffer that fits most but not all (e.g., missing last field)
  std::vector<uint8_t> buffer(expected_size - 4);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  
  // Serialize should fail (overflow on last field)
  ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(nested_ts_, &msg, storage);
  EXPECT_EQ(ret, RCUTILS_RET_ERROR);
}

TEST_F(TestSerializationOverflow, SerializeWithExtraSpace)
{
  // Create a basic message
  BasicTypes msg;
  msg.bool_value = true;
  msg.uint8_value = 42;
  
  // Get exact size needed
  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(basic_ts_, &msg, &expected_size);
  ASSERT_EQ(ret, RCUTILS_RET_OK);
  
  // Allocate buffer with extra space (should still succeed)
  std::vector<uint8_t> buffer(expected_size + 100);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  
  // Serialize should succeed with extra space
  ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(basic_ts_, &msg, storage);
  EXPECT_EQ(ret, RCUTILS_RET_OK);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
