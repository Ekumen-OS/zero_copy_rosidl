// Copyright 2026 Ekumen, Inc.
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

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
#include <gtest/gtest.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <cfloat>
#include <climits>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

#include "rosidl_generator_tests/msg/experimental/arrays.hpp"
#include "rosidl_generator_tests/msg/experimental/basic_idl.hpp"
#include "rosidl_generator_tests/msg/experimental/basic_types.hpp"
#include "rosidl_generator_tests/msg/experimental/bounded_plain_sequences.hpp"
#include "rosidl_generator_tests/msg/experimental/bounded_sequences.hpp"
#include "rosidl_generator_tests/msg/experimental/constants.hpp"
#include "rosidl_generator_tests/msg/experimental/defaults.hpp"
#include "rosidl_generator_tests/msg/experimental/empty.hpp"
#include "rosidl_generator_tests/msg/experimental/multi_nested.hpp"
#include "rosidl_generator_tests/msg/experimental/nested.hpp"
#include "rosidl_generator_tests/msg/experimental/small_constant.hpp"
#include "rosidl_generator_tests/msg/experimental/strings.hpp"
#include "rosidl_generator_tests/msg/experimental/unbounded_sequences.hpp"
#include "rosidl_generator_tests/msg/experimental/w_strings.hpp"

namespace experimental = rosidl_generator_tests::msg::experimental;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Round-trip assign+read for a Scalar<T> field.
#define ASSERT_SCALAR_ROUNDTRIP(msg, field, val) \
  do { \
    (msg).field = (val); \
    ASSERT_EQ((val), (msg).field); \
  } while (0)

// ---------------------------------------------------------------------------
// Empty
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_empty, default_construct_and_equality) {
  experimental::Empty a, b;
  EXPECT_EQ(a, b);
  EXPECT_FALSE(a != b);
}

TEST(test_experimental_interfaces_empty, pointer_type_aliases_exist) {
  static_assert(std::is_same_v<experimental::Empty::SharedPtr,
    std::shared_ptr<experimental::Empty>>);
  static_assert(std::is_same_v<experimental::Empty::UniquePtr,
    std::unique_ptr<experimental::Empty>>);
}

// ---------------------------------------------------------------------------
// BasicTypes — all 13 primitive Scalar<T> fields
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_basic_types, default_zero_initialization) {
  experimental::BasicTypes msg;
  EXPECT_FALSE(msg.bool_value);
  EXPECT_EQ(0, msg.byte_value);
  EXPECT_EQ(0, msg.char_value);
  EXPECT_EQ(0.0f, msg.float32_value);
  EXPECT_EQ(0.0, msg.float64_value);
  EXPECT_EQ(0, msg.int8_value);
  EXPECT_EQ(0, msg.uint8_value);
  EXPECT_EQ(0, msg.int16_value);
  EXPECT_EQ(0, msg.uint16_value);
  EXPECT_EQ(0, msg.int32_value);
  EXPECT_EQ(0u, msg.uint32_value);
  EXPECT_EQ(0, msg.int64_value);
  EXPECT_EQ(0u, msg.uint64_value);
}

TEST(test_experimental_interfaces_basic_types, scalar_assignment_max_values) {
  experimental::BasicTypes msg;
  ASSERT_SCALAR_ROUNDTRIP(msg, bool_value, true);
  ASSERT_SCALAR_ROUNDTRIP(msg, byte_value, static_cast<unsigned char>(UINT8_MAX));
  ASSERT_SCALAR_ROUNDTRIP(msg, char_value, static_cast<unsigned char>(UINT8_MAX));
  ASSERT_SCALAR_ROUNDTRIP(msg, float32_value, FLT_MAX);
  ASSERT_SCALAR_ROUNDTRIP(msg, float64_value, DBL_MAX);
  ASSERT_SCALAR_ROUNDTRIP(msg, int8_value, static_cast<int8_t>(INT8_MAX));
  ASSERT_SCALAR_ROUNDTRIP(msg, uint8_value, static_cast<uint8_t>(UINT8_MAX));
  ASSERT_SCALAR_ROUNDTRIP(msg, int16_value, static_cast<int16_t>(INT16_MAX));
  ASSERT_SCALAR_ROUNDTRIP(msg, uint16_value, static_cast<uint16_t>(UINT16_MAX));
  ASSERT_SCALAR_ROUNDTRIP(msg, int32_value, INT32_MAX);
  ASSERT_SCALAR_ROUNDTRIP(msg, uint32_value, UINT32_MAX);
  ASSERT_SCALAR_ROUNDTRIP(msg, int64_value, INT64_MAX);
  ASSERT_SCALAR_ROUNDTRIP(msg, uint64_value, UINT64_MAX);
}

TEST(test_experimental_interfaces_basic_types, scalar_assignment_min_values) {
  experimental::BasicTypes msg;
  ASSERT_SCALAR_ROUNDTRIP(msg, float32_value, FLT_MIN);
  ASSERT_SCALAR_ROUNDTRIP(msg, float64_value, DBL_MIN);
  ASSERT_SCALAR_ROUNDTRIP(msg, int8_value, static_cast<int8_t>(INT8_MIN));
  ASSERT_SCALAR_ROUNDTRIP(msg, int16_value, static_cast<int16_t>(INT16_MIN));
  ASSERT_SCALAR_ROUNDTRIP(msg, int32_value, INT32_MIN);
  ASSERT_SCALAR_ROUNDTRIP(msg, int64_value, INT64_MIN);
}

TEST(test_experimental_interfaces_basic_types, implicit_conversion_to_underlying_type) {
  experimental::BasicTypes msg;
  msg.int32_value = -42;
  int32_t v = msg.int32_value;  // relies on Scalar<T> implicit operator T()
  EXPECT_EQ(-42, v);
}

TEST(test_experimental_interfaces_basic_types, setters_via_named_parameter_idiom) {
  experimental::BasicTypes msg;
  msg.set__bool_value(rosidl_runtime_cpp::Scalar<bool> {true})
  .set__int32_value(rosidl_runtime_cpp::Scalar<int32_t> {7})
  .set__float32_value(rosidl_runtime_cpp::Scalar<float> {1.0f});
  EXPECT_TRUE(msg.bool_value);
  EXPECT_EQ(7, msg.int32_value);
  EXPECT_EQ(1.0f, msg.float32_value);
}

TEST(test_experimental_interfaces_basic_types, equality_and_inequality) {
  experimental::BasicTypes a, b;
  EXPECT_EQ(a, b);
  a.uint64_value = 123u;
  EXPECT_NE(a, b);
  b.uint64_value = 123u;
  EXPECT_EQ(a, b);
}

TEST(test_experimental_interfaces_basic_types, member_type_aliases) {
  static_assert(
    std::is_same_v<experimental::BasicTypes::_bool_value_type,
    rosidl_runtime_cpp::Scalar<bool>>);
  static_assert(
    std::is_same_v<experimental::BasicTypes::_int32_value_type,
    rosidl_runtime_cpp::Scalar<int32_t>>);
  static_assert(
    std::is_same_v<experimental::BasicTypes::_float64_value_type,
    rosidl_runtime_cpp::Scalar<double>>);
}

// ---------------------------------------------------------------------------
// Defaults — non-zero default values
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_defaults, default_values_are_non_zero) {
  experimental::Defaults msg;
  EXPECT_TRUE(msg.bool_value);
  EXPECT_EQ(50, msg.byte_value);
  EXPECT_EQ(100, msg.char_value);
  EXPECT_EQ(1.125f, msg.float32_value);
  EXPECT_EQ(1.125, msg.float64_value);
  EXPECT_EQ(-50, msg.int8_value);
  EXPECT_EQ(200, msg.uint8_value);
  EXPECT_EQ(-1000, msg.int16_value);
  EXPECT_EQ(2000, msg.uint16_value);
  EXPECT_EQ(-30000, msg.int32_value);
  EXPECT_EQ(60000u, msg.uint32_value);
  EXPECT_EQ(-40000000, msg.int64_value);
  EXPECT_EQ(50000000u, msg.uint64_value);
}

TEST(test_experimental_interfaces_defaults, two_default_constructed_instances_are_equal) {
  experimental::Defaults a, b;
  EXPECT_EQ(a, b);
}

// ---------------------------------------------------------------------------
// Constants — all 13 compile-time constants
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_constants, all_constant_values) {
  EXPECT_TRUE(experimental::Constants::BOOL_CONST);
  EXPECT_EQ(50, experimental::Constants::BYTE_CONST);
  EXPECT_EQ(100, experimental::Constants::CHAR_CONST);
  EXPECT_EQ(1.125f, experimental::Constants::FLOAT32_CONST);
  EXPECT_EQ(1.125, experimental::Constants::FLOAT64_CONST);
  EXPECT_EQ(-50, experimental::Constants::INT8_CONST);
  EXPECT_EQ(200u, experimental::Constants::UINT8_CONST);
  EXPECT_EQ(-1000, experimental::Constants::INT16_CONST);
  EXPECT_EQ(2000u, experimental::Constants::UINT16_CONST);
  EXPECT_EQ(-30000, experimental::Constants::INT32_CONST);
  EXPECT_EQ(60000u, experimental::Constants::UINT32_CONST);
  EXPECT_EQ(-40000000, experimental::Constants::INT64_CONST);
  EXPECT_EQ(50000000u, experimental::Constants::UINT64_CONST);
}

TEST(test_experimental_interfaces_constants, default_equality) {
  experimental::Constants a, b;
  EXPECT_EQ(a, b);
}

// ---------------------------------------------------------------------------
// SmallConstant
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_small_constant, constant_value) {
  EXPECT_FLOAT_EQ(0.05f, experimental::SmallConstant::FLOAT32_CONST);
  float f = experimental::SmallConstant::FLOAT32_CONST;
  EXPECT_FLOAT_EQ(0.05f, f);
}

// ---------------------------------------------------------------------------
// Strings — String and BoundedString<N> members
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_strings, default_strings_are_empty) {
  experimental::Strings msg;
  EXPECT_TRUE(msg.string_value.empty());
  EXPECT_TRUE(msg.bounded_string_value.empty());
}

TEST(test_experimental_interfaces_strings, unbounded_string_assign_and_view) {
  experimental::Strings msg;
  msg.string_value = "hello";
  EXPECT_EQ("hello", msg.string_value);
}

TEST(test_experimental_interfaces_strings, bounded_string_assign_and_view) {
  experimental::Strings msg;
  msg.bounded_string_value = "bounded";
  EXPECT_EQ("bounded", msg.bounded_string_value);
}

TEST(test_experimental_interfaces_strings, unbounded_string_defaults) {
  experimental::Strings msg;
  EXPECT_EQ("Hello world!", msg.string_value_default1);
  EXPECT_EQ("Hello'world!", msg.string_value_default2);
  EXPECT_EQ("Hello\"world!", msg.string_value_default3);
  EXPECT_EQ("Hello'world!", msg.string_value_default4);
  EXPECT_EQ("Hello\"world!", msg.string_value_default5);
}

TEST(test_experimental_interfaces_strings, bounded_string_defaults) {
  experimental::Strings msg;
  EXPECT_EQ("Hello world!", msg.bounded_string_value_default1);
  EXPECT_EQ("Hello'world!", msg.bounded_string_value_default2);
  EXPECT_EQ("Hello\"world!", msg.bounded_string_value_default3);
  EXPECT_EQ("Hello'world!", msg.bounded_string_value_default4);
  EXPECT_EQ("Hello\"world!", msg.bounded_string_value_default5);
}

TEST(test_experimental_interfaces_strings, string_constant) {
  EXPECT_EQ("Hello world!", experimental::Strings::STRING_CONST);
}

TEST(test_experimental_interfaces_strings, equality_and_inequality) {
  experimental::Strings a, b;
  EXPECT_EQ(a, b);
  a.string_value = "x";
  EXPECT_NE(a, b);
  b.string_value = "x";
  EXPECT_EQ(a, b);
}

TEST(test_experimental_interfaces_strings, member_type_alias_is_string) {
  static_assert(
    std::is_same_v<
      experimental::Strings::_string_value_type,
      rosidl_runtime_cpp::String>);
  static_assert(
    std::is_same_v<
      experimental::Strings::_bounded_string_value_type,
      rosidl_runtime_cpp::BoundedString<22>>);
}

// ---------------------------------------------------------------------------
// WStrings — WString + fixed array + bounded/unbounded sequences
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_wstrings, default_wstring_is_empty) {
  experimental::WStrings msg;
  EXPECT_TRUE(msg.wstring_value.empty());
}

TEST(test_experimental_interfaces_wstrings, wstring_assign_and_view) {
  experimental::WStrings msg;
  msg.wstring_value = u"wstring_\u2122";
  EXPECT_EQ(u"wstring_\u2122", msg.wstring_value);
}

TEST(test_experimental_interfaces_wstrings, wstring_defaults) {
  experimental::WStrings msg;
  EXPECT_EQ(u"Hello world!", msg.wstring_value_default1);
  EXPECT_EQ(u"Hell\u00f6 w\u00f6rld!", msg.wstring_value_default2);
  EXPECT_EQ(u"\u30cf\u30ed\u30fc\u30ef\u30fc\u30eb\u30c9",
    msg.wstring_value_default3);
}

TEST(test_experimental_interfaces_wstrings, array_of_wstrings_size_and_default) {
  experimental::WStrings msg;
  EXPECT_EQ(3u, msg.array_of_wstrings.size());
  for (std::size_t i = 0; i < 3u; ++i) {
    EXPECT_TRUE(msg.array_of_wstrings[i].empty());
  }
}

TEST(test_experimental_interfaces_wstrings, array_element_write_is_isolated) {
  experimental::WStrings msg;
  msg.array_of_wstrings[1] = u"hello";
  EXPECT_TRUE(msg.array_of_wstrings[0].empty());
  EXPECT_EQ(u"hello", msg.array_of_wstrings[1]);
  EXPECT_TRUE(msg.array_of_wstrings[2].empty());
}

TEST(test_experimental_interfaces_wstrings, bounded_sequence_empty_by_default) {
  experimental::WStrings msg;
  EXPECT_EQ(0u, msg.bounded_sequence_of_wstrings.size());
}

TEST(test_experimental_interfaces_wstrings, unbounded_sequence_push_back_and_read) {
  experimental::WStrings msg;
  msg.unbounded_sequence_of_wstrings.push_back(rosidl_runtime_cpp::WString {u"a"});
  msg.unbounded_sequence_of_wstrings.push_back(rosidl_runtime_cpp::WString {u"b"});
  ASSERT_EQ(2u, msg.unbounded_sequence_of_wstrings.size());
  EXPECT_EQ(u"a", msg.unbounded_sequence_of_wstrings[0]);
  EXPECT_EQ(u"b", msg.unbounded_sequence_of_wstrings[1]);
}

TEST(test_experimental_interfaces_wstrings, unbounded_sequence_push_back_from_stl_types) {
  experimental::WStrings msg;
  const std::u16string a_str = u"a";
  const std::u16string_view b_view = u"b";
  msg.unbounded_sequence_of_wstrings.push_back(a_str);
  msg.unbounded_sequence_of_wstrings.push_back(b_view);
  ASSERT_EQ(2u, msg.unbounded_sequence_of_wstrings.size());
  EXPECT_EQ(u"a", msg.unbounded_sequence_of_wstrings[0]);
  EXPECT_EQ(u"b", msg.unbounded_sequence_of_wstrings[1]);
}

TEST(test_experimental_interfaces_wstrings, equality_and_inequality) {
  experimental::WStrings a, b;
  EXPECT_EQ(a, b);
  a.wstring_value = u"different";
  EXPECT_NE(a, b);
}

// ---------------------------------------------------------------------------
// Nested — sub-message field
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_nested, sub_message_zero_by_default) {
  experimental::Nested msg;
  EXPECT_EQ(0, msg.basic_types_value.int32_value);
  EXPECT_EQ(0.0f, msg.basic_types_value.float32_value);
  EXPECT_FALSE(msg.basic_types_value.bool_value);
}

TEST(test_experimental_interfaces_nested, sub_message_write_propagates) {
  experimental::Nested msg;
  msg.basic_types_value.int32_value = 99;
  msg.basic_types_value.float32_value = 1.5f;
  EXPECT_EQ(99, msg.basic_types_value.int32_value);
  EXPECT_EQ(1.5f, msg.basic_types_value.float32_value);
}

TEST(test_experimental_interfaces_nested, equality) {
  experimental::Nested a, b;
  EXPECT_EQ(a, b);
  a.basic_types_value.bool_value = true;
  EXPECT_NE(a, b);
  b.basic_types_value.bool_value = true;
  EXPECT_EQ(a, b);
}

TEST(test_experimental_interfaces_nested, member_type_alias_is_sub_message) {
  static_assert(
    std::is_same_v<experimental::Nested::_basic_types_value_type, experimental::BasicTypes>);
}

// ---------------------------------------------------------------------------
// Arrays — fixed-size arrays of scalars, strings, and sub-messages
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_arrays, scalar_array_size_is_three) {
  experimental::Arrays msg;
  EXPECT_EQ(3u, msg.bool_values.size());
  EXPECT_EQ(3u, msg.int32_values.size());
  EXPECT_EQ(3u, msg.uint64_values.size());
}

TEST(test_experimental_interfaces_arrays, scalar_array_zero_init) {
  experimental::Arrays msg;
  for (std::size_t i = 0; i < 3u; ++i) {
    EXPECT_EQ(0, msg.int32_values[i]);
    EXPECT_FALSE(msg.bool_values[i]);
  }
}

TEST(test_experimental_interfaces_arrays, scalar_array_element_write_is_isolated) {
  experimental::Arrays msg;
  msg.int32_values[1] = 77;
  EXPECT_EQ(0, msg.int32_values[0]);
  EXPECT_EQ(77, msg.int32_values[1]);
  EXPECT_EQ(0, msg.int32_values[2]);
}

TEST(test_experimental_interfaces_arrays, scalar_array_iterate_and_sum) {
  experimental::Arrays msg;
  msg.uint8_values[0] = 10u;
  msg.uint8_values[1] = 20u;
  msg.uint8_values[2] = 30u;
  uint8_t sum = 0;
  for (auto v : msg.uint8_values) {
    sum = static_cast<uint8_t>(sum + static_cast<uint8_t>(v));
  }
  EXPECT_EQ(60u, sum);
}

TEST(test_experimental_interfaces_arrays, scalar_defaults_all_fields) {
  experimental::Arrays msg;
  // byte
  EXPECT_EQ(0u, msg.byte_values_default[0]);
  EXPECT_EQ(1u, msg.byte_values_default[1]);
  EXPECT_EQ(255u, msg.byte_values_default[2]);
  // float32
  EXPECT_EQ(1.125f, msg.float32_values_default[0]);
  EXPECT_EQ(0.0f, msg.float32_values_default[1]);
  EXPECT_EQ(-1.125f, msg.float32_values_default[2]);
  // int32
  EXPECT_EQ(0, msg.int32_values_default[0]);
  EXPECT_EQ(2147483647, msg.int32_values_default[1]);
  EXPECT_EQ(-2147483648, msg.int32_values_default[2]);
  // uint64
  EXPECT_EQ(0u, msg.uint64_values_default[0]);
  EXPECT_EQ(1u, msg.uint64_values_default[1]);
  EXPECT_EQ(18446744073709551615u, msg.uint64_values_default[2]);
}

TEST(test_experimental_interfaces_arrays, string_array_size_and_empty_default) {
  experimental::Arrays msg;
  EXPECT_EQ(3u, msg.string_values.size());
  EXPECT_TRUE(msg.string_values[0].empty());
}

TEST(test_experimental_interfaces_arrays, string_array_element_write) {
  experimental::Arrays msg;
  msg.string_values[0] = "first";
  msg.string_values[2] = "third";
  EXPECT_EQ("first", msg.string_values[0]);
  EXPECT_TRUE(msg.string_values[1].empty());
  EXPECT_EQ("third", msg.string_values[2]);
}

TEST(test_experimental_interfaces_arrays, string_array_defaults) {
  experimental::Arrays msg;
  EXPECT_TRUE(msg.string_values_default[0].empty());
  EXPECT_EQ("max value", msg.string_values_default[1]);
  EXPECT_EQ("min value", msg.string_values_default[2]);
}

TEST(test_experimental_interfaces_arrays, sub_message_array_element_write_is_isolated) {
  experimental::Arrays msg;
  msg.basic_types_values[1].int32_value = 42;
  EXPECT_EQ(0, msg.basic_types_values[0].int32_value);
  EXPECT_EQ(42, msg.basic_types_values[1].int32_value);
  EXPECT_EQ(0, msg.basic_types_values[2].int32_value);
}

TEST(test_experimental_interfaces_arrays, alignment_check_field_read_write) {
  experimental::Arrays msg;
  EXPECT_EQ(0, msg.alignment_check);
  msg.alignment_check = INT32_MIN;
  EXPECT_EQ(INT32_MIN, msg.alignment_check);
}

TEST(test_experimental_interfaces_arrays, equality_and_inequality) {
  experimental::Arrays a, b;
  EXPECT_EQ(a, b);
  a.int16_values[0] = 5;
  EXPECT_NE(a, b);
  b.int16_values[0] = 5;
  EXPECT_EQ(a, b);
}

// ---------------------------------------------------------------------------
// BoundedSequences
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_bounded_sequences, all_sequences_empty_by_default) {
  experimental::BoundedSequences msg;
  EXPECT_EQ(0u, msg.bool_values.size());
  EXPECT_EQ(0u, msg.int32_values.size());
  EXPECT_EQ(0u, msg.string_values.size());
  EXPECT_EQ(0u, msg.basic_types_values.size());
}

TEST(test_experimental_interfaces_bounded_sequences, scalar_push_back_and_index) {
  experimental::BoundedSequences msg;
  msg.int32_values.push_back(10);
  msg.int32_values.push_back(-5);
  ASSERT_EQ(2u, msg.int32_values.size());
  EXPECT_EQ(10, msg.int32_values[0]);
  EXPECT_EQ(-5, msg.int32_values[1]);
}

TEST(test_experimental_interfaces_bounded_sequences, string_push_back_and_view) {
  experimental::BoundedSequences msg;
  msg.string_values.push_back(rosidl_runtime_cpp::String {"hello"});
  ASSERT_EQ(1u, msg.string_values.size());
  EXPECT_EQ("hello", msg.string_values[0]);
}

TEST(test_experimental_interfaces_bounded_sequences, string_push_back_from_stl_types) {
  experimental::BoundedSequences msg;
  const std::string s = "hello";
  const std::string_view sv = "world";
  msg.string_values.push_back(s);
  msg.string_values.push_back(sv);
  ASSERT_EQ(2u, msg.string_values.size());
  EXPECT_EQ("hello", msg.string_values[0]);
  EXPECT_EQ("world", msg.string_values[1]);
}

TEST(test_experimental_interfaces_bounded_sequences, sub_message_push_back_and_read) {
  experimental::BoundedSequences msg;
  experimental::BasicTypes bt;
  bt.int32_value = 77;
  msg.basic_types_values.push_back(bt);
  ASSERT_EQ(1u, msg.basic_types_values.size());
  EXPECT_EQ(77, msg.basic_types_values[0].int32_value);
}

TEST(test_experimental_interfaces_bounded_sequences, clear_empties_sequence) {
  experimental::BoundedSequences msg;
  msg.uint8_values.push_back(1u);
  msg.uint8_values.push_back(2u);
  msg.uint8_values.clear();
  EXPECT_EQ(0u, msg.uint8_values.size());
}

TEST(test_experimental_interfaces_bounded_sequences, scalar_defaults_preloaded) {
  experimental::BoundedSequences msg;
  ASSERT_EQ(3u, msg.bool_values_default.size());
  EXPECT_FALSE(msg.bool_values_default[0]);
  EXPECT_TRUE(msg.bool_values_default[1]);
  EXPECT_FALSE(msg.bool_values_default[2]);
  ASSERT_EQ(3u, msg.int32_values_default.size());
  EXPECT_EQ(0, msg.int32_values_default[0]);
  EXPECT_EQ(2147483647, msg.int32_values_default[1]);
  EXPECT_EQ(-2147483648, msg.int32_values_default[2]);
}

TEST(test_experimental_interfaces_bounded_sequences, string_defaults_preloaded) {
  experimental::BoundedSequences msg;
  ASSERT_EQ(3u, msg.string_values_default.size());
  EXPECT_TRUE(msg.string_values_default[0].empty());
  EXPECT_EQ("max value", msg.string_values_default[1]);
  EXPECT_EQ("min value", msg.string_values_default[2]);
}

TEST(test_experimental_interfaces_bounded_sequences, alignment_check_field) {
  experimental::BoundedSequences msg;
  EXPECT_EQ(0, msg.alignment_check);
  msg.alignment_check = 7;
  EXPECT_EQ(7, msg.alignment_check);
}

TEST(test_experimental_interfaces_bounded_sequences, equality_and_inequality) {
  experimental::BoundedSequences a, b;
  EXPECT_EQ(a, b);
  a.float32_values.push_back(1.0f);
  EXPECT_NE(a, b);
  b.float32_values.push_back(1.0f);
  EXPECT_EQ(a, b);
}

// ---------------------------------------------------------------------------
// BoundedPlainSequences
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_bounded_plain_sequences, all_sequences_empty_by_default) {
  experimental::BoundedPlainSequences msg;
  EXPECT_EQ(0u, msg.bool_values.size());
  EXPECT_EQ(0u, msg.uint64_values.size());
  EXPECT_EQ(0u, msg.basic_types_values.size());
}

TEST(test_experimental_interfaces_bounded_plain_sequences, scalar_push_back_iterate) {
  experimental::BoundedPlainSequences msg;
  for (int32_t v : {1, 2, 3}) {
    msg.int32_values.push_back(v);
  }
  int32_t sum = 0;
  for (auto v : msg.int32_values) {
    sum += v;
                                            }
  EXPECT_EQ(6, sum);
}

TEST(test_experimental_interfaces_bounded_plain_sequences, sub_message_push_back) {
  experimental::BoundedPlainSequences msg;
  experimental::BasicTypes bt;
  bt.uint16_value = 500u;
  msg.basic_types_values.push_back(bt);
  ASSERT_EQ(1u, msg.basic_types_values.size());
  EXPECT_EQ(500u, msg.basic_types_values[0].uint16_value);
}

TEST(test_experimental_interfaces_bounded_plain_sequences, equality) {
  experimental::BoundedPlainSequences a, b;
  EXPECT_EQ(a, b);
  a.int16_values.push_back(7);
  EXPECT_NE(a, b);
  b.int16_values.push_back(7);
  EXPECT_EQ(a, b);
}

// ---------------------------------------------------------------------------
// UnboundedSequences
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_unbounded_sequences, all_sequences_empty_by_default) {
  experimental::UnboundedSequences msg;
  EXPECT_EQ(0u, msg.bool_values.size());
  EXPECT_EQ(0u, msg.string_values.size());
  EXPECT_EQ(0u, msg.basic_types_values.size());
}

TEST(test_experimental_interfaces_unbounded_sequences, scalar_push_grow_clear) {
  experimental::UnboundedSequences msg;
  for (int64_t v : {100LL, 200LL, 300LL, 400LL}) {
    msg.int64_values.push_back(v);
  }
  ASSERT_EQ(4u, msg.int64_values.size());
  EXPECT_EQ(300, msg.int64_values[2]);
  msg.int64_values.clear();
  EXPECT_EQ(0u, msg.int64_values.size());
}

TEST(test_experimental_interfaces_unbounded_sequences, string_push_back_and_view) {
  experimental::UnboundedSequences msg;
  msg.string_values.push_back(rosidl_runtime_cpp::String {"alpha"});
  msg.string_values.push_back(rosidl_runtime_cpp::String {"beta"});
  ASSERT_EQ(2u, msg.string_values.size());
  EXPECT_EQ("alpha", msg.string_values[0]);
  EXPECT_EQ("beta", msg.string_values[1]);
}

TEST(test_experimental_interfaces_unbounded_sequences, string_push_back_from_stl_types) {
  experimental::UnboundedSequences msg;
  const std::string s = "alpha";
  const std::string_view sv = "beta";
  msg.string_values.push_back(s);
  msg.string_values.push_back(sv);
  ASSERT_EQ(2u, msg.string_values.size());
  EXPECT_EQ("alpha", msg.string_values[0]);
  EXPECT_EQ("beta", msg.string_values[1]);
}

TEST(test_experimental_interfaces_unbounded_sequences, sub_message_push_back_and_read) {
  experimental::UnboundedSequences msg;
  experimental::BasicTypes bt;
  bt.float64_value = 3.14;
  msg.basic_types_values.push_back(bt);
  ASSERT_EQ(1u, msg.basic_types_values.size());
  EXPECT_EQ(3.14, msg.basic_types_values[0].float64_value);
}

TEST(test_experimental_interfaces_unbounded_sequences, scalar_defaults_preloaded) {
  experimental::UnboundedSequences msg;
  ASSERT_EQ(3u, msg.float32_values_default.size());
  EXPECT_EQ(1.125f, msg.float32_values_default[0]);
  EXPECT_EQ(0.0f, msg.float32_values_default[1]);
  EXPECT_EQ(-1.125f, msg.float32_values_default[2]);
  ASSERT_EQ(3u, msg.uint64_values_default.size());
  EXPECT_EQ(0u, msg.uint64_values_default[0]);
  EXPECT_EQ(1u, msg.uint64_values_default[1]);
  EXPECT_EQ(18446744073709551615u, msg.uint64_values_default[2]);
}

TEST(test_experimental_interfaces_unbounded_sequences, string_defaults_preloaded) {
  experimental::UnboundedSequences msg;
  ASSERT_EQ(3u, msg.string_values_default.size());
  EXPECT_TRUE(msg.string_values_default[0].empty());
  EXPECT_EQ("max value", msg.string_values_default[1]);
  EXPECT_EQ("min value", msg.string_values_default[2]);
}

TEST(test_experimental_interfaces_unbounded_sequences, alignment_check_field) {
  experimental::UnboundedSequences msg;
  EXPECT_EQ(0, msg.alignment_check);
  msg.alignment_check = -7;
  EXPECT_EQ(-7, msg.alignment_check);
}

TEST(test_experimental_interfaces_unbounded_sequences, equality_and_inequality) {
  experimental::UnboundedSequences a, b;
  EXPECT_EQ(a, b);
  a.bool_values.push_back(true);
  EXPECT_NE(a, b);
  b.bool_values.push_back(true);
  EXPECT_EQ(a, b);
}

// ---------------------------------------------------------------------------
// MultiNested — arrays and sequences of complex sub-messages
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_multi_nested, array_of_arrays_has_three_elements) {
  experimental::MultiNested msg;
  EXPECT_EQ(3u, msg.array_of_arrays.size());
  EXPECT_EQ(3u, msg.array_of_bounded_sequences.size());
  EXPECT_EQ(3u, msg.array_of_unbounded_sequences.size());
}

TEST(test_experimental_interfaces_multi_nested, array_element_is_writable) {
  experimental::MultiNested msg;
  msg.array_of_arrays[0].int32_values[1] = 55;
  EXPECT_EQ(55, msg.array_of_arrays[0].int32_values[1]);
  // Adjacent elements untouched
  EXPECT_EQ(0, msg.array_of_arrays[0].int32_values[0]);
  EXPECT_EQ(0, msg.array_of_arrays[1].int32_values[1]);
}

TEST(test_experimental_interfaces_multi_nested, bounded_sequence_of_arrays_empty_by_default) {
  experimental::MultiNested msg;
  EXPECT_EQ(0u, msg.bounded_sequence_of_arrays.size());
  EXPECT_EQ(0u, msg.bounded_sequence_of_bounded_sequences.size());
  EXPECT_EQ(0u, msg.bounded_sequence_of_unbounded_sequences.size());
}

TEST(test_experimental_interfaces_multi_nested, unbounded_sequence_of_bounded_push_and_read) {
  experimental::MultiNested msg;
  experimental::BoundedSequences bs;
  bs.int32_values.push_back(9);
  msg.unbounded_sequence_of_bounded_sequences.push_back(bs);
  ASSERT_EQ(1u, msg.unbounded_sequence_of_bounded_sequences.size());
  EXPECT_EQ(9, msg.unbounded_sequence_of_bounded_sequences[0].int32_values[0]);
}

TEST(test_experimental_interfaces_multi_nested, unbounded_sequence_of_unbounded_push_and_read) {
  experimental::MultiNested msg;
  experimental::UnboundedSequences us;
  us.int64_values.push_back(42);
  msg.unbounded_sequence_of_unbounded_sequences.push_back(us);
  ASSERT_EQ(1u, msg.unbounded_sequence_of_unbounded_sequences.size());
  EXPECT_EQ(42, msg.unbounded_sequence_of_unbounded_sequences[0].int64_values[0]);
}

TEST(test_experimental_interfaces_multi_nested, equality_sensitive_to_deep_change) {
  experimental::MultiNested a, b;
  EXPECT_EQ(a, b);
  a.array_of_arrays[2].bool_values[0] = true;
  EXPECT_NE(a, b);
  b.array_of_arrays[2].bool_values[0] = true;
  EXPECT_EQ(a, b);
}

// ---------------------------------------------------------------------------
// BasicIdl — single float field defined via IDL syntax
// ---------------------------------------------------------------------------

TEST(test_experimental_interfaces_basic_idl, default_and_assignment) {
  experimental::BasicIdl msg;
  EXPECT_EQ(0.0f, msg.x);
  msg.x = 2.718f;
  EXPECT_EQ(2.718f, msg.x);
}

TEST(test_experimental_interfaces_basic_idl, implicit_conversion) {
  experimental::BasicIdl msg;
  msg.x = 1.0f;
  float f = msg.x;
  EXPECT_EQ(1.0f, f);
}

TEST(test_experimental_interfaces_basic_idl, equality) {
  experimental::BasicIdl a, b;
  EXPECT_EQ(a, b);
  a.x = 9.0f;
  EXPECT_NE(a, b);
}
