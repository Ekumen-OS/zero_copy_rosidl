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

/// @file test_serialization_roundtrip.cpp
///
/// Serialize-deserialize roundtrip tests for every message family.
/// Each test creates a message, fills it with canonical data, serialises
/// into a buffer, deserialises into a fresh message, and verifies equality.
///
/// Message families covered (standard + experimental where available):
///   - BasicTypes (all primitive types)
///   - BoundedMessage (fixed-size array, bounded)
///   - UnboundedMessage (string, variable-length sequence)
///   - InnerMessage (simple struct)
///   - NestedMessage (struct composition)

#include "test_helpers.hpp"

// =============================================================================
// BasicTypes
// =============================================================================

TEST(TestSerializationRoundtrip_BasicTypes, Standard)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes;
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<T>();
  T msg;
  test_roundtrip(ts, msg, fill_basic_types, verify_basic_types);
}

TEST(TestSerializationRoundtrip_BasicTypes, Experimental)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BasicTypes;
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<T>();
  T msg;
  test_roundtrip(ts, msg, fill_basic_types, verify_basic_types);
}

// =============================================================================
// BoundedMessage
// =============================================================================

TEST(TestSerializationRoundtrip_BoundedMessage, Standard)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage;
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<T>();
  T msg;
  test_roundtrip(ts, msg, fill_bounded_message, verify_bounded_message);
}

TEST(TestSerializationRoundtrip_BoundedMessage, Experimental)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage;
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<T>();
  T msg;
  test_roundtrip(ts, msg, fill_bounded_message, verify_bounded_message);
}

// =============================================================================
// UnboundedMessage
// =============================================================================

TEST(TestSerializationRoundtrip_UnboundedMessage, Standard)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::UnboundedMessage;
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<T>();
  T msg;
  test_roundtrip(ts, msg, fill_unbounded_message, verify_unbounded_message);
}

TEST(TestSerializationRoundtrip_UnboundedMessage, Experimental)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage;
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<T>();
  T msg;
  test_roundtrip(ts, msg, fill_unbounded_message, verify_unbounded_message);
}

// =============================================================================
// InnerMessage  (standard only)
// =============================================================================

TEST(TestSerializationRoundtrip_InnerMessage, Standard)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::InnerMessage;
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<T>();
  T msg;
  test_roundtrip(ts, msg, fill_inner_message, verify_inner_message);
}

// =============================================================================
// NestedMessage  (standard only)
// =============================================================================

TEST(TestSerializationRoundtrip_NestedMessage, Standard)
{
  using T = rosidl_typesupport_xcdr_cpp_tests::msg::NestedMessage;
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<T>();
  T msg;
  test_roundtrip(ts, msg, fill_nested_message, verify_nested_message);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
