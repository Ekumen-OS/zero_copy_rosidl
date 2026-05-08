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

#include <string>
#include <vector>
#include <cstring>

#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/inner_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/nested_message.hpp"
#include \
  "rosidl_typesupport_xcdr_cpp_tests/msg/detail/inner_message__rosidl_typesupport_xcdr_cpp.hpp"
#include \
  "rosidl_typesupport_xcdr_cpp_tests/msg/detail/nested_message__rosidl_typesupport_xcdr_cpp.hpp"

using rosidl_typesupport_xcdr_cpp_tests::msg::InnerMessage;
using rosidl_typesupport_xcdr_cpp_tests::msg::NestedMessage;

class TestNestedSerialization : public ::testing::Test {
protected:
  void SetUp() override
  {
    storage.resize(4096);
  }

  std::vector<uint8_t> storage;
};

TEST_F(TestNestedSerialization, SerializeDeserializeNestedMessage) {
  // Get type support
  auto ts = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_xcdr_cpp,
    rosidl_typesupport_xcdr_cpp_tests, msg,
    NestedMessage)();
  ASSERT_NE(nullptr, ts);

  // Create message
  NestedMessage msg;
  msg.name = "test_nested";
  msg.inner.id = 42;
  msg.inner.value = 3.14159;
  msg.count = 100;

  // Serialize
  rosidl_runtime_cpp::MemoryRegion<void> storage_region{storage.data(), storage.size()};
  auto ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(ts, &msg, storage_region);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  // Deserialize into new message
  NestedMessage deserialized_msg;
  ret = rosidl_typesupport_xcdr_cpp::deserialize_message_from(ts, storage_region,
    &deserialized_msg);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  // Verify outer message
  EXPECT_EQ(msg.name, deserialized_msg.name);
  EXPECT_EQ(msg.count, deserialized_msg.count);

  // Verify nested inner message
  EXPECT_EQ(msg.inner.id, deserialized_msg.inner.id);
  EXPECT_DOUBLE_EQ(msg.inner.value, deserialized_msg.inner.value);
}

TEST_F(TestNestedSerialization, SerializeDeserializeInnerMessage) {
  // Get type support
  auto ts = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_xcdr_cpp,
    rosidl_typesupport_xcdr_cpp_tests, msg,
    InnerMessage)();
  ASSERT_NE(nullptr, ts);

  // Create message
  InnerMessage msg;
  msg.id = 123;
  msg.value = 2.71828;

  // Serialize
  rosidl_runtime_cpp::MemoryRegion<void> storage_region{storage.data(), storage.size()};
  auto ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(ts, &msg, storage_region);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  // Deserialize into new message
  InnerMessage deserialized_msg;
  ret = rosidl_typesupport_xcdr_cpp::deserialize_message_from(ts, storage_region,
    &deserialized_msg);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  // Verify
  EXPECT_EQ(msg.id, deserialized_msg.id);
  EXPECT_DOUBLE_EQ(msg.value, deserialized_msg.value);
}
