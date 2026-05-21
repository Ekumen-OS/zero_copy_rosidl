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

// Silence GCC pedantic warnings triggered by gtest internals.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <gtest/gtest.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <string>
#include <tuple>

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
namespace traits = rosidl_generator_traits;

using traits::has_bounded_size;
using traits::has_fixed_size;
using traits::is_message;
using traits::MessageTraits;

// ---------------------------------------------------------------------------
// has_fixed_size
// Fixed: only messages with no dynamic members (scalars, sub-msg of scalars).
// ---------------------------------------------------------------------------

TEST(test_experimental_traits_fixed_size, true_for_all_scalar_messages) {
  static_assert(has_fixed_size<experimental::BasicTypes>::value, "BasicTypes should be fixed");
  static_assert(has_fixed_size<experimental::Defaults>::value, "Defaults should be fixed");
  static_assert(has_fixed_size<experimental::Constants>::value, "Constants should be fixed");
  static_assert(has_fixed_size<experimental::Empty>::value, "Empty should be fixed");
  static_assert(has_fixed_size<experimental::Nested>::value, "Nested should be fixed");
  static_assert(has_fixed_size<experimental::BasicIdl>::value, "BasicIdl should be fixed");
  static_assert(has_fixed_size<experimental::SmallConstant>::value,
    "SmallConstant should be fixed");
}

TEST(test_experimental_traits_fixed_size, false_for_string_messages) {
  static_assert(!has_fixed_size<experimental::Strings>::value, "Strings should NOT be fixed");
  static_assert(!has_fixed_size<experimental::WStrings>::value, "WStrings should NOT be fixed");
}

TEST(test_experimental_traits_fixed_size, false_for_sequence_messages) {
  static_assert(
    !has_fixed_size<experimental::BoundedSequences>::value,
    "BoundedSequences should NOT be fixed");
  static_assert(
    !has_fixed_size<experimental::BoundedPlainSequences>::value,
    "BoundedPlainSequences should NOT be fixed");
  static_assert(
    !has_fixed_size<experimental::UnboundedSequences>::value,
    "UnboundedSequences should NOT be fixed");
}

TEST(test_experimental_traits_fixed_size, false_for_arrays_containing_strings) {
  // Arrays.msg contains string[3] string_values — not fixed.
  static_assert(!has_fixed_size<experimental::Arrays>::value, "Arrays should NOT be fixed");
}

TEST(test_experimental_traits_fixed_size, false_for_multi_nested) {
  static_assert(!has_fixed_size<experimental::MultiNested>::value,
    "MultiNested should NOT be fixed");
}

// ---------------------------------------------------------------------------
// has_bounded_size
// Bounded: no unbounded sequences; unbounded strings make it unbounded.
// BoundedPlainSequences has bounded-seq of scalars → bounded (not fixed).
// Arrays has string[3] → unbounded (String has no max).
// ---------------------------------------------------------------------------

TEST(test_experimental_traits_bounded_size, true_for_scalar_and_nested_messages) {
  static_assert(has_bounded_size<experimental::BasicTypes>::value);
  static_assert(has_bounded_size<experimental::Defaults>::value);
  static_assert(has_bounded_size<experimental::Constants>::value);
  static_assert(has_bounded_size<experimental::Empty>::value);
  static_assert(has_bounded_size<experimental::Nested>::value);
  static_assert(has_bounded_size<experimental::BasicIdl>::value);
  static_assert(has_bounded_size<experimental::SmallConstant>::value);
}

TEST(test_experimental_traits_bounded_size, true_for_bounded_plain_sequences) {
  // All members are bounded sequences of scalars — bounded but not fixed.
  static_assert(has_bounded_size<experimental::BoundedPlainSequences>::value);
  static_assert(!has_fixed_size<experimental::BoundedPlainSequences>::value);
}

TEST(test_experimental_traits_bounded_size, false_for_unbounded_string_messages) {
  static_assert(!has_bounded_size<experimental::Strings>::value,
    "Strings has unbounded string members");
  static_assert(!has_bounded_size<experimental::WStrings>::value,
    "WStrings has unbounded wstring members");
}

TEST(test_experimental_traits_bounded_size, false_for_string_arrays) {
  // Arrays.msg has string[3] string_values with no bound → unbounded.
  static_assert(!has_bounded_size<experimental::Arrays>::value);
}

TEST(test_experimental_traits_bounded_size, false_for_bounded_sequences_with_strings) {
  // BoundedSequences has string[<=3] string_values (unbounded string elements).
  static_assert(!has_bounded_size<experimental::BoundedSequences>::value);
}

TEST(test_experimental_traits_bounded_size, false_for_unbounded_sequences) {
  static_assert(!has_bounded_size<experimental::UnboundedSequences>::value);
}

TEST(test_experimental_traits_bounded_size, false_for_multi_nested) {
  static_assert(!has_bounded_size<experimental::MultiNested>::value);
}

// ---------------------------------------------------------------------------
// is_message
// ---------------------------------------------------------------------------

TEST(test_experimental_traits_is_message, all_types_are_messages) {
  static_assert(is_message<experimental::BasicTypes>::value);
  static_assert(is_message<experimental::Defaults>::value);
  static_assert(is_message<experimental::Constants>::value);
  static_assert(is_message<experimental::Empty>::value);
  static_assert(is_message<experimental::Nested>::value);
  static_assert(is_message<experimental::Strings>::value);
  static_assert(is_message<experimental::WStrings>::value);
  static_assert(is_message<experimental::Arrays>::value);
  static_assert(is_message<experimental::BoundedSequences>::value);
  static_assert(is_message<experimental::BoundedPlainSequences>::value);
  static_assert(is_message<experimental::UnboundedSequences>::value);
  static_assert(is_message<experimental::MultiNested>::value);
  static_assert(is_message<experimental::BasicIdl>::value);
  static_assert(is_message<experimental::SmallConstant>::value);
}

// ---------------------------------------------------------------------------
// data_type — C++ qualified name with "experimental" namespace
// ---------------------------------------------------------------------------

TEST(test_experimental_traits_data_type, experimental_namespace_in_data_type) {
  EXPECT_STREQ(
    "rosidl_generator_tests::msg::experimental::BasicTypes",
    traits::data_type<experimental::BasicTypes>());
  EXPECT_STREQ(
    "rosidl_generator_tests::msg::experimental::Defaults",
    traits::data_type<experimental::Defaults>());
  EXPECT_STREQ(
    "rosidl_generator_tests::msg::experimental::Empty",
    traits::data_type<experimental::Empty>());
  EXPECT_STREQ(
    "rosidl_generator_tests::msg::experimental::Strings",
    traits::data_type<experimental::Strings>());
  EXPECT_STREQ(
    "rosidl_generator_tests::msg::experimental::Arrays",
    traits::data_type<experimental::Arrays>());
  EXPECT_STREQ(
    "rosidl_generator_tests::msg::experimental::UnboundedSequences",
    traits::data_type<experimental::UnboundedSequences>());
  EXPECT_STREQ(
    "rosidl_generator_tests::msg::experimental::MultiNested",
    traits::data_type<experimental::MultiNested>());
}

// ---------------------------------------------------------------------------
// name — original ROS FQN (no "experimental"), for wire compatibility
// ---------------------------------------------------------------------------

TEST(test_experimental_traits_name, ros_fqn_has_no_experimental_segment) {
  EXPECT_STREQ(
    "rosidl_generator_tests/msg/BasicTypes",
    traits::name<experimental::BasicTypes>());
  EXPECT_STREQ(
    "rosidl_generator_tests/msg/Defaults",
    traits::name<experimental::Defaults>());
  EXPECT_STREQ(
    "rosidl_generator_tests/msg/Empty",
    traits::name<experimental::Empty>());
  EXPECT_STREQ(
    "rosidl_generator_tests/msg/Strings",
    traits::name<experimental::Strings>());
  EXPECT_STREQ(
    "rosidl_generator_tests/msg/Arrays",
    traits::name<experimental::Arrays>());
  EXPECT_STREQ(
    "rosidl_generator_tests/msg/MultiNested",
    traits::name<experimental::MultiNested>());
}

// ---------------------------------------------------------------------------
// MessageTraits — member_count and member_names (compile-time)
// ---------------------------------------------------------------------------

using TraitsEmpty = MessageTraits<experimental::Empty>;
using TraitsBasicTypes = MessageTraits<experimental::BasicTypes>;
using TraitsDefaults = MessageTraits<experimental::Defaults>;
using TraitsNested = MessageTraits<experimental::Nested>;
using TraitsStrings = MessageTraits<experimental::Strings>;
using TraitsArrays = MessageTraits<experimental::Arrays>;
using TraitsBoundedSeqs = MessageTraits<experimental::BoundedSequences>;
using TraitsUnboundedSeqs = MessageTraits<experimental::UnboundedSequences>;

// Empty has no data members (structure required member handled internally).
// static_assert(TraitsEmpty::member_count == 1u);

// BasicTypes: 13 primitive fields.
static_assert(TraitsBasicTypes::member_count == 13u);
static_assert(TraitsBasicTypes::member_names[0] == "bool_value");
static_assert(TraitsBasicTypes::member_names[6] == "uint8_value");
static_assert(TraitsBasicTypes::member_names[12] == "uint64_value");

// Defaults: same 13 fields as BasicTypes.
static_assert(TraitsDefaults::member_count == 13u);
static_assert(TraitsDefaults::member_names[0] == "bool_value");
static_assert(TraitsDefaults::member_names[12] == "uint64_value");

// Nested: one sub-message field.
static_assert(TraitsNested::member_count == 1u);
static_assert(TraitsNested::member_names[0] == "basic_types_value");

// Strings: 12 members (5 unbounded + 1 constant placeholder + 6 bounded, no
// constant in data). Actual layout from Strings.msg:
//   string_value, string_value_default1..5, bounded_string_value,
//   bounded_string_value_default1..5  → 12 members.
static_assert(TraitsStrings::member_count == 12u);
static_assert(TraitsStrings::member_names[0] == "string_value");
static_assert(TraitsStrings::member_names[6] == "bounded_string_value");

// Arrays.msg has 34 members (13 undefaulted + 13 defaulted + string[3]
// string_values + string[3] string_values_default + alignment_check... let
// compiler tell us the truth at runtime to avoid fragile hardcoding).
TEST(test_experimental_traits_message_traits, arrays_member_names_spot_check) {
  EXPECT_EQ("bool_values", TraitsArrays::member_names[0]);
  EXPECT_EQ("string_values", TraitsArrays::member_names[13]);
  // alignment_check is always the last member.
  EXPECT_EQ("alignment_check",
    TraitsArrays::member_names[TraitsArrays::member_count - 1]);
}

TEST(test_experimental_traits_message_traits, bounded_sequences_member_names_spot_check) {
  EXPECT_EQ("bool_values", TraitsBoundedSeqs::member_names[0]);
  EXPECT_EQ("string_values", TraitsBoundedSeqs::member_names[13]);
  EXPECT_EQ("alignment_check",
    TraitsBoundedSeqs::member_names[TraitsBoundedSeqs::member_count - 1]);
}

TEST(test_experimental_traits_message_traits, unbounded_sequences_member_names_spot_check) {
  EXPECT_EQ("bool_values", TraitsUnboundedSeqs::member_names[0]);
  EXPECT_EQ("string_values", TraitsUnboundedSeqs::member_names[13]);
  EXPECT_EQ("alignment_check",
    TraitsUnboundedSeqs::member_names[TraitsUnboundedSeqs::member_count - 1]);
}

// ---------------------------------------------------------------------------
// to_yaml — block and flow style, covering all member categories
// ---------------------------------------------------------------------------

namespace
{
void normalize_float(std::string & s)
{
  // MSVC may emit 6 decimal places; normalise to 5.
  std::string::size_type pos = 0;
  while ((pos = s.find("0.000000", pos)) != std::string::npos) {
    s.replace(pos, 8u, "0.00000");
    pos += 7u;
  }
}
}  // namespace

TEST(test_experimental_traits_to_yaml, empty_block_style) {
  using experimental::to_yaml;
  EXPECT_EQ("null\n", to_yaml(experimental::Empty {}));
}

TEST(test_experimental_traits_to_yaml, empty_flow_style) {
  using experimental::to_yaml;
  EXPECT_EQ("null", to_yaml(experimental::Empty {}, /*use_flow_style=*/true));
}

TEST(test_experimental_traits_to_yaml, basic_types_all_zero_block) {
  using experimental::to_yaml;
  experimental::BasicTypes msg;
  std::string yaml = to_yaml(msg);
  normalize_float(yaml);
  EXPECT_EQ(
    "bool_value: false\n"
    "byte_value: 0x00\n"
    "char_value: 0\n"
    "float32_value: 0.00000\n"
    "float64_value: 0.00000\n"
    "int8_value: 0\n"
    "uint8_value: 0\n"
    "int16_value: 0\n"
    "uint16_value: 0\n"
    "int32_value: 0\n"
    "uint32_value: 0\n"
    "int64_value: 0\n"
    "uint64_value: 0\n",
    yaml);
}

TEST(test_experimental_traits_to_yaml, defaults_block) {
  using experimental::to_yaml;
  experimental::Defaults msg;
  // Override float64 to a representable value to avoid platform drift.
  msg.float64_value = 1.0;
  std::string yaml = to_yaml(msg);
  normalize_float(yaml);
  EXPECT_EQ(
    "bool_value: true\n"
    "byte_value: 0x32\n"
    "char_value: 100\n"
    "float32_value: 1.12500\n"
    "float64_value: 1.00000\n"
    "int8_value: -50\n"
    "uint8_value: 200\n"
    "int16_value: -1000\n"
    "uint16_value: 2000\n"
    "int32_value: -30000\n"
    "uint32_value: 60000\n"
    "int64_value: -40000000\n"
    "uint64_value: 50000000\n",
    yaml);
}

TEST(test_experimental_traits_to_yaml, defaults_flow) {
  using experimental::to_yaml;
  experimental::Defaults msg;
  msg.float64_value = 1.0;
  std::string yaml = to_yaml(msg, /*use_flow_style=*/true);
  normalize_float(yaml);
  EXPECT_EQ(
    "{bool_value: true, byte_value: 0x32, char_value: 100, "
    "float32_value: 1.12500, float64_value: 1.00000, int8_value: -50, "
    "uint8_value: 200, int16_value: -1000, uint16_value: 2000, "
    "int32_value: -30000, uint32_value: 60000, int64_value: -40000000, "
    "uint64_value: 50000000}",
    yaml);
}

TEST(test_experimental_traits_to_yaml, nested_block) {
  using experimental::to_yaml;
  experimental::Nested msg;
  std::string yaml = to_yaml(msg);
  normalize_float(yaml);
  EXPECT_EQ(
    "basic_types_value:\n"
    "  bool_value: false\n"
    "  byte_value: 0x00\n"
    "  char_value: 0\n"
    "  float32_value: 0.00000\n"
    "  float64_value: 0.00000\n"
    "  int8_value: 0\n"
    "  uint8_value: 0\n"
    "  int16_value: 0\n"
    "  uint16_value: 0\n"
    "  int32_value: 0\n"
    "  uint32_value: 0\n"
    "  int64_value: 0\n"
    "  uint64_value: 0\n",
    yaml);
}

TEST(test_experimental_traits_to_yaml, nested_flow) {
  using experimental::to_yaml;
  experimental::Nested msg;
  std::string yaml = to_yaml(msg, /*use_flow_style=*/true);
  normalize_float(yaml);
  EXPECT_EQ(
    "{basic_types_value: {bool_value: false, byte_value: 0x00, "
    "char_value: 0, float32_value: 0.00000, float64_value: 0.00000, "
    "int8_value: 0, uint8_value: 0, int16_value: 0, uint16_value: 0, "
    "int32_value: 0, uint32_value: 0, int64_value: 0, uint64_value: 0}}",
    yaml);
}

TEST(test_experimental_traits_to_yaml, strings_block) {
  using experimental::to_yaml;
  experimental::Strings msg;
  msg.string_value = "Hello\nworld";
  EXPECT_EQ(
    R"(string_value: "Hello
world"
string_value_default1: "Hello world!"
string_value_default2: "Hello'world!"
string_value_default3: "Hello\"world!"
string_value_default4: "Hello'world!"
string_value_default5: "Hello\"world!"
bounded_string_value: ""
bounded_string_value_default1: "Hello world!"
bounded_string_value_default2: "Hello'world!"
bounded_string_value_default3: "Hello\"world!"
bounded_string_value_default4: "Hello'world!"
bounded_string_value_default5: "Hello\"world!"
)",
    to_yaml(msg));
}

TEST(test_experimental_traits_to_yaml, strings_flow) {
  using experimental::to_yaml;
  experimental::Strings msg;
  msg.string_value = "Hello\nworld";
  EXPECT_EQ(
    R"({string_value: "Hello
world", string_value_default1: "Hello world!", )"
    R"(string_value_default2: "Hello'world!", )"
    R"(string_value_default3: "Hello\"world!", )"
    R"(string_value_default4: "Hello'world!", )"
    R"(string_value_default5: "Hello\"world!", )"
    R"(bounded_string_value: "", )"
    R"(bounded_string_value_default1: "Hello world!", )"
    R"(bounded_string_value_default2: "Hello'world!", )"
    R"(bounded_string_value_default3: "Hello\"world!", )"
    R"(bounded_string_value_default4: "Hello'world!", )"
    R"(bounded_string_value_default5: "Hello\"world!"})",
    to_yaml(msg, /*use_flow_style=*/true));
}

TEST(test_experimental_traits_to_yaml, wstrings_block) {
  using experimental::to_yaml;
  experimental::WStrings msg;
  msg.wstring_value = u"Hello\nw\u00f6rld";
  EXPECT_EQ(
    R"(wstring_value: "Hello
w\xf6rld"
wstring_value_default1: "Hello world!"
wstring_value_default2: "Hell\xf6 w\xf6rld!"
wstring_value_default3: "\u30cf\u30ed\u30fc\u30ef\u30fc\u30eb\u30c9"
array_of_wstrings:
- ""
- ""
- ""
bounded_sequence_of_wstrings: []
unbounded_sequence_of_wstrings: []
)",
    to_yaml(msg));
}

TEST(test_experimental_traits_to_yaml, wstrings_flow) {
  using experimental::to_yaml;
  experimental::WStrings msg;
  msg.wstring_value = u"Hello\nw\u00f6rld";
  EXPECT_EQ(
    R"({wstring_value: "Hello
w\xf6rld", wstring_value_default1: "Hello world!", )"
    R"(wstring_value_default2: "Hell\xf6 w\xf6rld!", )"
    R"(wstring_value_default3: "\u30cf\u30ed\u30fc\u30ef\u30fc\u30eb\u30c9", )"
    R"(array_of_wstrings: ["", "", ""], bounded_sequence_of_wstrings: [], )"
    R"(unbounded_sequence_of_wstrings: []})",
    to_yaml(msg, /*use_flow_style=*/true));
}

TEST(test_experimental_traits_to_yaml, empty_bounded_sequences_block) {
  using experimental::to_yaml;
  experimental::BoundedSequences msg;
  // Spot-check a few lines; all sequences empty, defaults populated.
  std::string yaml = to_yaml(msg);
  normalize_float(yaml);
  // Empty sequences render as []
  EXPECT_NE(std::string::npos, yaml.find("bool_values: []\n"));
  EXPECT_NE(std::string::npos, yaml.find("string_values: []\n"));
  EXPECT_NE(std::string::npos, yaml.find("basic_types_values: []\n"));
  // Default values are populated
  EXPECT_NE(std::string::npos, yaml.find("- false\n"));
  EXPECT_NE(std::string::npos, yaml.find("alignment_check: 0\n"));
}

TEST(test_experimental_traits_to_yaml, bounded_sequences_with_one_defaults_element_block) {
  using experimental::to_yaml;
  experimental::BoundedSequences msg;
  msg.defaults_values.push_back(experimental::Defaults {});
  std::string yaml = to_yaml(msg);
  normalize_float(yaml);
  EXPECT_NE(std::string::npos, yaml.find("defaults_values:\n"));
  EXPECT_NE(std::string::npos, yaml.find("  bool_value: true\n"));
}

// ---------------------------------------------------------------------------
// as_tuple_ref — structured-binding support
// ---------------------------------------------------------------------------

TEST(test_experimental_traits_as_tuple_ref, modifies_original_message) {
  using rosidl_generator_tests::msg::experimental::as_tuple_ref;
  experimental::Defaults msg;
  EXPECT_EQ(-1000, msg.int16_value);
  // Zero-initialise all fields via tuple reference.
  std::apply(
    [] (auto & ... f) {((f = {}), ...);},
    as_tuple_ref(msg));
  EXPECT_EQ(0, msg.int16_value);
  EXPECT_FALSE(msg.bool_value);
}

// TEST(test_experimental_traits_as_tuple_ref, empty_message_tuple_is_empty) {
//   using rosidl_generator_tests::msg::experimental::as_tuple_ref;
//   experimental::Empty msg;
//   auto t = as_tuple_ref(msg);
//   static_assert(std::tuple_size_v<decltype(t)> == 0u);
// }
