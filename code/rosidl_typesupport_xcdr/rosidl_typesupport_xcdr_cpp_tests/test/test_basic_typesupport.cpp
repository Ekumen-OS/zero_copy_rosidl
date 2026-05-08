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

#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"

#include "rosidl_typesupport_xcdr_cpp_tests/msg/basic_types.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/bounded_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/unbounded_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/detail/basic_types__rosidl_typesupport_xcdr_cpp.hpp"
#include \
  "rosidl_typesupport_xcdr_cpp_tests/msg/detail/bounded_message__rosidl_typesupport_xcdr_cpp.hpp"
#include \
  "rosidl_typesupport_xcdr_cpp_tests/msg/detail/unbounded_message__rosidl_typesupport_xcdr_cpp.hpp"

TEST(TestBasicTypesupport, GetTypesupportHandle)
{
  auto ts = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_xcdr_cpp,
    rosidl_typesupport_xcdr_cpp_tests, msg,
    BasicTypes)();

  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
  EXPECT_NE(nullptr, ts->data);
}

TEST(TestBasicTypesupport, GetBoundedMessageTypesupport)
{
  auto ts = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_xcdr_cpp,
    rosidl_typesupport_xcdr_cpp_tests, msg,
    BoundedMessage)();

  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
}

TEST(TestBasicTypesupport, GetUnboundedMessageTypesupport)
{
  auto ts = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_xcdr_cpp,
    rosidl_typesupport_xcdr_cpp_tests, msg,
    UnboundedMessage)();

  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
