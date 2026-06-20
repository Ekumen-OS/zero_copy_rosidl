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

/// @file test_typesupport_handle.cpp
///
/// Tests retrieving message typesupport handles via the template function
/// get_message_type_support_handle<T>() and the macro
/// ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME.
///
/// Each suite covers one message family (BasicTypes, BoundedMessage,
/// UnboundedMessage, InnerMessage, NestedMessage) and, where applicable,
/// both the standard and experimental namespaces.

#include "test_helpers.hpp"

// =============================================================================
// BasicTypes — standard
// =============================================================================

TEST(TestTypesupportHandle_BasicTypes, Standard_GetHandle)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes>();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
  EXPECT_NE(nullptr, ts->data);
}

TEST(TestTypesupportHandle_BasicTypes, Standard_MacroLookup)
{
  auto ts = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_xcdr_cpp,
    rosidl_typesupport_xcdr_cpp_tests, msg,
    BasicTypes)();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
  EXPECT_NE(nullptr, ts->data);
}

TEST(TestTypesupportHandle_BasicTypes, Experimental_GetHandle)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BasicTypes>();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
  EXPECT_NE(nullptr, ts->data);
}

// =============================================================================
// BoundedMessage — standard + experimental
// =============================================================================

TEST(TestTypesupportHandle_BoundedMessage, Standard_GetHandle)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage>();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
  EXPECT_NE(nullptr, ts->data);
}

TEST(TestTypesupportHandle_BoundedMessage, Standard_MacroLookup)
{
  auto ts = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_xcdr_cpp,
    rosidl_typesupport_xcdr_cpp_tests, msg,
    BoundedMessage)();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
}

TEST(TestTypesupportHandle_BoundedMessage, Experimental_GetHandle)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage>();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
  EXPECT_NE(nullptr, ts->data);
}

// =============================================================================
// UnboundedMessage — standard + experimental
// =============================================================================

TEST(TestTypesupportHandle_UnboundedMessage, Standard_GetHandle)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::UnboundedMessage>();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
  EXPECT_NE(nullptr, ts->data);
}

TEST(TestTypesupportHandle_UnboundedMessage, Standard_MacroLookup)
{
  auto ts = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_xcdr_cpp,
    rosidl_typesupport_xcdr_cpp_tests, msg,
    UnboundedMessage)();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
}

TEST(TestTypesupportHandle_UnboundedMessage, Experimental_GetHandle)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage>();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
  EXPECT_NE(nullptr, ts->data);
}

// =============================================================================
// InnerMessage — standard only (no experimental variant)
// =============================================================================

TEST(TestTypesupportHandle_InnerMessage, Standard_GetHandle)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::InnerMessage>();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
  EXPECT_NE(nullptr, ts->data);
}

TEST(TestTypesupportHandle_InnerMessage, Standard_MacroLookup)
{
  auto ts = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_xcdr_cpp,
    rosidl_typesupport_xcdr_cpp_tests, msg,
    InnerMessage)();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
}

// =============================================================================
// NestedMessage — standard only (no experimental variant)
// =============================================================================

TEST(TestTypesupportHandle_NestedMessage, Standard_GetHandle)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::NestedMessage>();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
  EXPECT_NE(nullptr, ts->data);
}

TEST(TestTypesupportHandle_NestedMessage, Standard_MacroLookup)
{
  auto ts = ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_xcdr_cpp,
    rosidl_typesupport_xcdr_cpp_tests, msg,
    NestedMessage)();
  ASSERT_NE(nullptr, ts);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, ts->typesupport_identifier);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
