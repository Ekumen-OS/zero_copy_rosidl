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

#ifndef TEST_HELPERS_HPP_
#define TEST_HELPERS_HPP_

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"
#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "rosidl_runtime_cpp/experimental/memory.hpp"

// Standard message headers
#include "rosidl_typesupport_xcdr_cpp_tests/msg/basic_types.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/bounded_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/unbounded_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/inner_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/nested_message.hpp"

// Standard XCDR detail headers
#include \
  "rosidl_typesupport_xcdr_cpp_tests/msg/detail/basic_types__rosidl_typesupport_xcdr_cpp.hpp"
#include \
  "rosidl_typesupport_xcdr_cpp_tests/msg/detail/bounded_message__rosidl_typesupport_xcdr_cpp.hpp"
#include \
  "rosidl_typesupport_xcdr_cpp_tests/msg/detail/unbounded_message__rosidl_typesupport_xcdr_cpp.hpp"
#include \
  "rosidl_typesupport_xcdr_cpp_tests/msg/detail/inner_message__rosidl_typesupport_xcdr_cpp.hpp"
#include \
  "rosidl_typesupport_xcdr_cpp_tests/msg/detail/nested_message__rosidl_typesupport_xcdr_cpp.hpp"

// Experimental message headers
#include "rosidl_typesupport_xcdr_cpp_tests/msg/experimental/basic_types.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/experimental/bounded_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/experimental/unbounded_message.hpp"

// =============================================================================
// Fill helpers — populate each message type with canonical test data
// =============================================================================

inline void fill_basic_types(
  rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes & msg)
{
  msg.bool_value = true;
  msg.int8_value = -10;
  msg.uint8_value = 42;
  msg.int16_value = -1000;
  msg.uint16_value = 50000;
  msg.int32_value = -100000;
  msg.uint32_value = 200000;
  msg.int64_value = -1000000;
  msg.uint64_value = 2000000;
  msg.float32_value = 3.14f;
  msg.float64_value = 2.71828;
}

// Also fill experimental variant
inline void fill_basic_types(
  rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BasicTypes & msg)
{
  msg.bool_value = true;
  msg.int8_value = -10;
  msg.uint8_value = 42;
  msg.int16_value = -1000;
  msg.uint16_value = 50000;
  msg.int32_value = -100000;
  msg.uint32_value = 200000;
  msg.int64_value = -1000000;
  msg.uint64_value = 2000000;
  msg.float32_value = 3.14f;
  msg.float64_value = 2.71828;
}

inline void fill_bounded_message(
  rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage & msg)
{
  msg.id = 42;
  msg.value = 3.14;
  for (size_t i = 0; i < 10; ++i) {
    msg.data[i] = static_cast<uint8_t>(i * 10);
  }
}

inline void fill_bounded_message(
  rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage & msg)
{
  msg.id = 42;
  msg.value = 3.14;
  for (size_t i = 0; i < 10; ++i) {
    msg.data[i] = static_cast<uint8_t>(i * 10);
  }
}

inline void fill_unbounded_message(
  rosidl_typesupport_xcdr_cpp_tests::msg::UnboundedMessage & msg)
{
  msg.id = 999;
  msg.name = "test_name";
  msg.data = {1, 2, 3, 4, 5};
}

inline void fill_unbounded_message(
  rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage & msg)
{
  msg.id = 999;
  msg.name = "test_name";
  msg.data = {1, 2, 3, 4, 5};
}

inline void fill_inner_message(
  rosidl_typesupport_xcdr_cpp_tests::msg::InnerMessage & msg)
{
  msg.id = 123;
  msg.value = 2.71828;
}

inline void fill_nested_message(
  rosidl_typesupport_xcdr_cpp_tests::msg::NestedMessage & msg)
{
  msg.name = "test_nested";
  msg.inner.id = 42;
  msg.inner.value = 3.14159;
  msg.count = 100;
}

// =============================================================================
// Verify helpers — compare expected vs actual after roundtrip
// =============================================================================

inline void verify_basic_types(
  const rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes & expected,
  const rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes & actual)
{
  EXPECT_EQ(expected.bool_value, actual.bool_value);
  EXPECT_EQ(expected.int8_value, actual.int8_value);
  EXPECT_EQ(expected.uint8_value, actual.uint8_value);
  EXPECT_EQ(expected.int16_value, actual.int16_value);
  EXPECT_EQ(expected.uint16_value, actual.uint16_value);
  EXPECT_EQ(expected.int32_value, actual.int32_value);
  EXPECT_EQ(expected.uint32_value, actual.uint32_value);
  EXPECT_EQ(expected.int64_value, actual.int64_value);
  EXPECT_EQ(expected.uint64_value, actual.uint64_value);
  EXPECT_FLOAT_EQ(expected.float32_value, actual.float32_value);
  EXPECT_DOUBLE_EQ(expected.float64_value, actual.float64_value);
}

inline void verify_basic_types(
  const rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BasicTypes & expected,
  const rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BasicTypes & actual)
{
  EXPECT_EQ(expected.bool_value, actual.bool_value);
  EXPECT_EQ(expected.int8_value, actual.int8_value);
  EXPECT_EQ(expected.uint8_value, actual.uint8_value);
  EXPECT_EQ(expected.int16_value, actual.int16_value);
  EXPECT_EQ(expected.uint16_value, actual.uint16_value);
  EXPECT_EQ(expected.int32_value, actual.int32_value);
  EXPECT_EQ(expected.uint32_value, actual.uint32_value);
  EXPECT_EQ(expected.int64_value, actual.int64_value);
  EXPECT_EQ(expected.uint64_value, actual.uint64_value);
  EXPECT_FLOAT_EQ(expected.float32_value, actual.float32_value);
  EXPECT_DOUBLE_EQ(expected.float64_value, actual.float64_value);
}

inline void verify_bounded_message(
  const rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage & expected,
  const rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage & actual)
{
  EXPECT_EQ(expected.id, actual.id);
  EXPECT_DOUBLE_EQ(expected.value, actual.value);
  for (size_t i = 0; i < 10; ++i) {
    EXPECT_EQ(expected.data[i], actual.data[i]);
  }
}

inline void verify_bounded_message(
  const rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage & expected,
  const rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage & actual)
{
  EXPECT_EQ(expected.id, actual.id);
  EXPECT_DOUBLE_EQ(expected.value, actual.value);
  for (size_t i = 0; i < 10; ++i) {
    EXPECT_EQ(expected.data[i], actual.data[i]);
  }
}

inline void verify_unbounded_message(
  const rosidl_typesupport_xcdr_cpp_tests::msg::UnboundedMessage & expected,
  const rosidl_typesupport_xcdr_cpp_tests::msg::UnboundedMessage & actual)
{
  EXPECT_EQ(expected.id, actual.id);
  EXPECT_EQ(expected.name, actual.name);
  ASSERT_EQ(expected.data.size(), actual.data.size());
  for (size_t i = 0; i < expected.data.size(); ++i) {
    EXPECT_EQ(expected.data[i], actual.data[i]);
  }
}

inline void verify_unbounded_message(
  const rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage & expected,
  const rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage & actual)
{
  EXPECT_EQ(expected.id, actual.id);
  EXPECT_EQ(expected.name, actual.name);
  ASSERT_EQ(expected.data.size(), actual.data.size());
  for (size_t i = 0; i < expected.data.size(); ++i) {
    EXPECT_EQ(expected.data[i], actual.data[i]);
  }
}

inline void verify_inner_message(
  const rosidl_typesupport_xcdr_cpp_tests::msg::InnerMessage & expected,
  const rosidl_typesupport_xcdr_cpp_tests::msg::InnerMessage & actual)
{
  EXPECT_EQ(expected.id, actual.id);
  EXPECT_DOUBLE_EQ(expected.value, actual.value);
}

inline void verify_nested_message(
  const rosidl_typesupport_xcdr_cpp_tests::msg::NestedMessage & expected,
  const rosidl_typesupport_xcdr_cpp_tests::msg::NestedMessage & actual)
{
  EXPECT_EQ(expected.name, actual.name);
  EXPECT_EQ(expected.count, actual.count);
  EXPECT_EQ(expected.inner.id, actual.inner.id);
  EXPECT_DOUBLE_EQ(expected.inner.value, actual.inner.value);
}

// =============================================================================
// Roundtrip template — serialise, deserialise, verify any message type
// =============================================================================

template<typename MsgT>
void test_roundtrip(
  const rosidl_message_type_support_t * ts,
  MsgT & original,
  void (*fill)(MsgT &),
  void (*verify)(const MsgT &, const MsgT &),
  size_t buffer_size = 4096)
{
  ASSERT_NE(nullptr, ts);
  fill(original);

  std::vector<uint8_t> buffer(buffer_size);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};

  auto ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(ts, &original, storage);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  MsgT deserialized;
  ret = rosidl_typesupport_xcdr_cpp::deserialize_message_from(ts, storage, &deserialized);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  verify(original, deserialized);
}

#endif  // TEST_HELPERS_HPP_
