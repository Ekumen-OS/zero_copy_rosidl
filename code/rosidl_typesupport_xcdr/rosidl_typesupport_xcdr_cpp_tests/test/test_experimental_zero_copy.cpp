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
#include <vector>

#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"
#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"
#include "rosidl_runtime_cpp/experimental/memory.hpp"

#include "rosidl_typesupport_xcdr_cpp_tests/msg/experimental/bounded_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/experimental/unbounded_message.hpp"

TEST(TestExperimentalZeroCopy, BoundedMessageRoundtrip)
{
  using BoundedMessage =
    rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage;

  // Create message
  BoundedMessage msg;
  msg.id = 42;
  msg.value = 3.14;
  for (size_t i = 0; i < 10; ++i) {
    msg.data[i] = static_cast<uint8_t>(i * 10);
  }

  // Get typesupport
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<BoundedMessage>();
  ASSERT_NE(nullptr, ts);

  // Allocate buffer
  std::vector<uint8_t> buffer(4096);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};

  // Serialize (zero-copy: writes directly into storage)
  auto ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(ts, &msg, storage);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  // Deserialize (zero-copy: constructs in-place from storage)
  void * deserialized_msg_ptr = nullptr;
  ret = rosidl_typesupport_xcdr_cpp::deserialize_message_from(ts, storage, &deserialized_msg_ptr);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  ASSERT_NE(nullptr, deserialized_msg_ptr);

  auto deserialized_msg = static_cast<BoundedMessage *>(deserialized_msg_ptr);

  // Verify values match
  EXPECT_EQ(msg.id, deserialized_msg->id);
  EXPECT_DOUBLE_EQ(msg.value, deserialized_msg->value);
  for (size_t i = 0; i < 10; ++i) {
    EXPECT_EQ(msg.data[i], deserialized_msg->data[i]);
  }

  // Clean up
  delete deserialized_msg;
}

TEST(TestExperimentalZeroCopy, UnboundedMessageRoundtrip)
{
  using UnboundedMessage =
    rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage;

  // Create message
  UnboundedMessage msg;
  msg.id = 999;
  msg.name = "test_name";
  msg.data = {1, 2, 3, 4, 5};

  // Get typesupport
  auto ts =
    rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<UnboundedMessage>();
  ASSERT_NE(nullptr, ts);

  // Allocate buffer
  std::vector<uint8_t> buffer(4096);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};

  // Serialize (zero-copy)
  auto ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(ts, &msg, storage);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  // Deserialize (zero-copy)
  void * deserialized_msg_ptr = nullptr;
  ret = rosidl_typesupport_xcdr_cpp::deserialize_message_from(ts, storage, &deserialized_msg_ptr);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  ASSERT_NE(nullptr, deserialized_msg_ptr);

  auto deserialized_msg = static_cast<UnboundedMessage *>(deserialized_msg_ptr);

  // Verify values match
  EXPECT_EQ(msg.id, deserialized_msg->id);
  EXPECT_EQ(msg.name, deserialized_msg->name);
  ASSERT_EQ(msg.data.size(), deserialized_msg->data.size());
  for (size_t i = 0; i < msg.data.size(); ++i) {
    EXPECT_EQ(msg.data[i], deserialized_msg->data[i]);
  }

  // Clean up
  delete deserialized_msg;
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
