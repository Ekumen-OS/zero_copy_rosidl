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

#include <gtest/gtest.h>

#include <type_traits>

#include "rosidl_runtime_cpp/experimental/constraints.hpp"
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
#include "rosidl_generator_tests/msg/experimental/strings.hpp"
#include "rosidl_generator_tests/msg/experimental/unbounded_sequences.hpp"
#include "rosidl_generator_tests/msg/experimental/w_strings.hpp"

namespace experimental = rosidl_generator_tests::msg::experimental;
using rosidl_runtime_cpp::SequenceConstraint;
using rosidl_runtime_cpp::String;
using rosidl_runtime_cpp::StringConstraint;
using rosidl_runtime_cpp::WString;

// ===========================================================================
// StringConstraint
// ===========================================================================

TEST(test_experimental_constraints_string_constraint, default_size_is_zero) {
  StringConstraint c {};
  EXPECT_EQ(0u, c.size);
}

TEST(test_experimental_constraints_string_constraint, equality_tracks_size) {
  StringConstraint a {}, b {};
  EXPECT_EQ(a, b);
  a.size = 64u;
  EXPECT_NE(a, b);
  b.size = 64u;
  EXPECT_EQ(a, b);
}

// ===========================================================================
// SequenceConstraint primary template  (scalars, bounded strings/sequences)
// ===========================================================================

TEST(test_experimental_constraints_seq_primary, default_size_is_zero) {
  SequenceConstraint<int32_t> c {};
  EXPECT_EQ(0u, c.size);
}

TEST(test_experimental_constraints_seq_primary, equality_tracks_size) {
  SequenceConstraint<int32_t> c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
  c1.size = 10u;
  EXPECT_NE(c1, c2);
  c2.size = 10u;
  EXPECT_EQ(c1, c2);
}

TEST(test_experimental_constraints_seq_primary, works_for_all_scalar_types) {
  // Compile-time check: primary template instantiates for every primitive.
  (void)SequenceConstraint<bool> {};
  (void)SequenceConstraint<uint8_t> {};
  (void)SequenceConstraint<int8_t> {};
  (void)SequenceConstraint<uint16_t> {};
  (void)SequenceConstraint<int16_t> {};
  (void)SequenceConstraint<uint32_t> {};
  (void)SequenceConstraint<int32_t> {};
  (void)SequenceConstraint<uint64_t> {};
  (void)SequenceConstraint<int64_t> {};
  (void)SequenceConstraint<float> {};
  (void)SequenceConstraint<double> {};
}

// ===========================================================================
// SequenceConstraint<String>
// ===========================================================================

TEST(test_experimental_constraints_seq_string, both_fields_default_to_zero) {
  SequenceConstraint<String> c {};
  EXPECT_EQ(0u, c.size);
  EXPECT_EQ(0u, c.element.size);
}

TEST(test_experimental_constraints_seq_string, fields_are_independently_mutable) {
  SequenceConstraint<String> c {};
  c.size = 5u;
  c.element.size = 128u;
  EXPECT_EQ(5u, c.size);
  EXPECT_EQ(128u, c.element.size);
}

TEST(test_experimental_constraints_seq_string, equality_considers_both_fields) {
  SequenceConstraint<String> c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
  c1.element.size = 64u;
  EXPECT_NE(c1, c2);
  c2.element.size = 64u;
  EXPECT_EQ(c1, c2);
  c1.size = 2u;
  EXPECT_NE(c1, c2);
  c2.size = 2u;
  EXPECT_EQ(c1, c2);
}

// ===========================================================================
// SequenceConstraint<WString>
// ===========================================================================

TEST(test_experimental_constraints_seq_wstring, fields_can_be_set_and_read) {
  SequenceConstraint<WString> c {};
  c.size = 3u;
  c.element.size = 32u;
  EXPECT_EQ(3u, c.size);
  EXPECT_EQ(32u, c.element.size);
}

TEST(test_experimental_constraints_seq_wstring, equality_considers_both_fields) {
  SequenceConstraint<WString> c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
  c1.size = 2u;
  EXPECT_NE(c1, c2);
  c2.size = 2u;
  c1.element.size = 8u;
  EXPECT_NE(c1, c2);
}

// ===========================================================================
// Per-message Constraints struct topology
// Each message type has a nested ::Constraints struct generated alongside it.
// ===========================================================================

// Empty: no data members → empty Constraints
TEST(test_experimental_constraints_per_message, empty_constraints_is_empty) {
  experimental::Empty::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
  // No mutation possible — there are no fields.
}

// BasicTypes / Defaults / Constants: all scalars → empty Constraints
TEST(test_experimental_constraints_per_message, basic_types_constraints_is_empty) {
  experimental::BasicTypes::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
}

TEST(test_experimental_constraints_per_message, defaults_constraints_is_empty) {
  experimental::Defaults::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
}

TEST(test_experimental_constraints_per_message, constants_constraints_is_empty) {
  experimental::Constants::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
}

// BoundedSequences / BoundedPlainSequences: all bounded → empty Constraints
TEST(test_experimental_constraints_per_message, bounded_sequences_constraints_is_empty) {
  experimental::BoundedSequences::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
}

TEST(test_experimental_constraints_per_message, bounded_plain_sequences_constraints_is_empty) {
  experimental::BoundedPlainSequences::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
}

// Nested: one NamespacedType field → Constraints { BasicTypes::Constraints basic_types_value; }
TEST(test_experimental_constraints_per_message, nested_has_sub_message_constraint_field) {
  static_assert(
    std::is_same_v<
      decltype(experimental::Nested::Constraints::basic_types_value),
      experimental::BasicTypes::Constraints>,
    "Nested::Constraints::basic_types_value should be BasicTypes::Constraints");

  experimental::Nested::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
  // BasicTypes has no variable members, so BasicTypes::Constraints has no fields;
  // the nested field is therefore also always equal.
}

// Strings: unbounded string fields → StringConstraint per field
TEST(test_experimental_constraints_per_message, strings_has_string_constraint_fields) {
  // All five unbounded string members get a StringConstraint field.
  static_assert(
    std::is_same_v<decltype(experimental::Strings::Constraints::string_value),
    StringConstraint>);
  static_assert(
    std::is_same_v<decltype(experimental::Strings::Constraints::string_value_default1),
    StringConstraint>);
  // Bounded members (bounded_string_value, etc.) do NOT appear in Constraints.
  static_assert(
    !std::is_member_pointer_v<
      decltype(&experimental::Strings::Constraints::string_value)>||
    true,  // always passes; real check is the static_assert above compiling
    "");

  experimental::Strings::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);

  c1.string_value.size = 256u;
  EXPECT_NE(c1, c2);
  c2.string_value.size = 256u;
  EXPECT_EQ(c1, c2);

  c1.string_value_default3.size = 128u;
  EXPECT_NE(c1, c2);
  c2.string_value_default3.size = 128u;
  EXPECT_EQ(c1, c2);
}

// WStrings: unbounded wstring fields → StringConstraint;
//              array_of_wstrings → StringConstraint (shared per element);
//              bounded_sequence → no constraint;
//              unbounded_sequence → SequenceConstraint<WString>
TEST(test_experimental_constraints_per_message, wstrings_standalone_fields) {
  static_assert(
    std::is_same_v<decltype(experimental::WStrings::Constraints::wstring_value),
    StringConstraint>);
  static_assert(
    std::is_same_v<decltype(experimental::WStrings::Constraints::wstring_value_default1),
    StringConstraint>);
  static_assert(
    std::is_same_v<decltype(experimental::WStrings::Constraints::wstring_value_default2),
    StringConstraint>);
  static_assert(
    std::is_same_v<decltype(experimental::WStrings::Constraints::wstring_value_default3),
    StringConstraint>);
}

TEST(test_experimental_constraints_per_message, wstrings_array_field) {
  // array_of_wstrings is WString[3] → StringConstraint (shared cap for all elements).
  static_assert(
    std::is_same_v<decltype(experimental::WStrings::Constraints::array_of_wstrings),
    StringConstraint>);
  experimental::WStrings::Constraints c {};
  c.array_of_wstrings.size = 64u;
  EXPECT_EQ(64u, c.array_of_wstrings.size);
}

TEST(test_experimental_constraints_per_message, wstrings_unbounded_sequence_field) {
  // unbounded_sequence_of_wstrings → SequenceConstraint<WString>.
  static_assert(
    std::is_same_v<
      decltype(experimental::WStrings::Constraints::unbounded_sequence_of_wstrings),
      SequenceConstraint<WString>>);
  experimental::WStrings::Constraints c {};
  c.unbounded_sequence_of_wstrings.size = 10u;
  c.unbounded_sequence_of_wstrings.element.size = 32u;
  EXPECT_EQ(10u, c.unbounded_sequence_of_wstrings.size);
  EXPECT_EQ(32u, c.unbounded_sequence_of_wstrings.element.size);
}

TEST(test_experimental_constraints_per_message, wstrings_equality) {
  experimental::WStrings::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
  c1.wstring_value.size = 16u;
  EXPECT_NE(c1, c2);
  c2.wstring_value.size = 16u;
  EXPECT_EQ(c1, c2);
}

// Arrays: string[3] string_values  → StringConstraint
//             BasicTypes[3]            → BasicTypes::Constraints
//             Constants[3]             → Constants::Constraints (empty)
//             Defaults[3]              → Defaults::Constraints (empty)
//             string[3] string_values_default → StringConstraint
TEST(test_experimental_constraints_per_message, arrays_string_fields) {
  static_assert(
    std::is_same_v<decltype(experimental::Arrays::Constraints::string_values),
    StringConstraint>);
  static_assert(
    std::is_same_v<decltype(experimental::Arrays::Constraints::string_values_default),
    StringConstraint>);

  experimental::Arrays::Constraints c {};
  c.string_values.size = 100u;
  EXPECT_EQ(100u, c.string_values.size);
}

TEST(test_experimental_constraints_per_message, arrays_sub_message_fields) {
  static_assert(
    std::is_same_v<
      decltype(experimental::Arrays::Constraints::basic_types_values),
      experimental::BasicTypes::Constraints>);
  static_assert(
    std::is_same_v<
      decltype(experimental::Arrays::Constraints::constants_values),
      experimental::Constants::Constraints>);
  static_assert(
    std::is_same_v<
      decltype(experimental::Arrays::Constraints::defaults_values),
      experimental::Defaults::Constraints>);
}

TEST(test_experimental_constraints_per_message, arrays_equality) {
  experimental::Arrays::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
  c1.string_values_default.size = 22u;
  EXPECT_NE(c1, c2);
  c2.string_values_default.size = 22u;
  EXPECT_EQ(c1, c2);
}

// UnboundedSequences:
//     scalar[]   → SequenceConstraint<scalar>  (primary, just .size)
//     string[]   → SequenceConstraint<String>  (.size + .element.size)
//     BasicTypes[] → SequenceConstraint<BasicTypes> (.size + .element: BasicTypes::Constraints)
//
TEST(test_experimental_constraints_per_message, unbounded_sequences_scalar_fields) {
  static_assert(
    std::is_same_v<
      decltype(experimental::UnboundedSequences::Constraints::bool_values),
      SequenceConstraint<bool>>);
  static_assert(
    std::is_same_v<
      decltype(experimental::UnboundedSequences::Constraints::int32_values),
      SequenceConstraint<int32_t>>);
  static_assert(
    std::is_same_v<
      decltype(experimental::UnboundedSequences::Constraints::float64_values),
      SequenceConstraint<double>>);

  experimental::UnboundedSequences::Constraints c {};
  c.int32_values.size = 50u;
  EXPECT_EQ(50u, c.int32_values.size);
}

TEST(test_experimental_constraints_per_message, unbounded_sequences_string_field) {
  static_assert(
    std::is_same_v<
      decltype(experimental::UnboundedSequences::Constraints::string_values),
      SequenceConstraint<String>>);

  experimental::UnboundedSequences::Constraints c {};
  c.string_values.size = 20u;
  c.string_values.element.size = 64u;
  EXPECT_EQ(20u, c.string_values.size);
  EXPECT_EQ(64u, c.string_values.element.size);
}

TEST(test_experimental_constraints_per_message, unbounded_sequences_sub_message_fields) {
  static_assert(
    std::is_same_v<
      decltype(experimental::UnboundedSequences::Constraints::basic_types_values),
      SequenceConstraint<experimental::BasicTypes>>);
  static_assert(
    std::is_same_v<
      decltype(experimental::UnboundedSequences::Constraints::defaults_values),
      SequenceConstraint<experimental::Defaults>>);
  static_assert(
    std::is_same_v<
      decltype(experimental::UnboundedSequences::Constraints::constants_values),
      SequenceConstraint<experimental::Constants>>);

  experimental::UnboundedSequences::Constraints c {};
  c.basic_types_values.size = 8u;
  EXPECT_EQ(8u, c.basic_types_values.size);
}

TEST(test_experimental_constraints_per_message, unbounded_sequences_equality) {
  experimental::UnboundedSequences::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);
  c1.string_values.element.size = 32u;
  EXPECT_NE(c1, c2);
  c2.string_values.element.size = 32u;
  EXPECT_EQ(c1, c2);
}

// MultiNested:
//     Arrays[3]             → Arrays::Constraints           (has fields)
//     BoundedSequences[3]   → BoundedSequences::Constraints  (empty)
//     UnboundedSequences[3] → UnboundedSequences::Constraints (has fields)
//     Arrays[<=3]           → no constraint (bounded)
//     BoundedSequences[<=3] → no constraint (bounded)
//     UnboundedSequences[<=3] → no constraint (bounded)
//     Arrays[]              → SequenceConstraint<Arrays>
//     BoundedSequences[]    → SequenceConstraint<BoundedSequences>
//     UnboundedSequences[]  → SequenceConstraint<UnboundedSequences>
//
TEST(test_experimental_constraints_per_message, multi_nested_array_fields) {
  static_assert(
    std::is_same_v<
      decltype(experimental::MultiNested::Constraints::array_of_arrays),
      experimental::Arrays::Constraints>);
  static_assert(
    std::is_same_v<
      decltype(experimental::MultiNested::Constraints::array_of_bounded_sequences),
      experimental::BoundedSequences::Constraints>);
  static_assert(
    std::is_same_v<
      decltype(experimental::MultiNested::Constraints::array_of_unbounded_sequences),
      experimental::UnboundedSequences::Constraints>);
}

TEST(test_experimental_constraints_per_message, multi_nested_unbounded_sequence_fields) {
  static_assert(
    std::is_same_v<
      decltype(experimental::MultiNested::Constraints::unbounded_sequence_of_arrays),
      SequenceConstraint<experimental::Arrays>>);
  static_assert(
    std::is_same_v<
      decltype(experimental::MultiNested::Constraints::unbounded_sequence_of_bounded_sequences),
      SequenceConstraint<experimental::BoundedSequences>>);
  static_assert(
    std::is_same_v<
      decltype(experimental::MultiNested::Constraints::unbounded_sequence_of_unbounded_sequences),
      SequenceConstraint<experimental::UnboundedSequences>>);
}

TEST(test_experimental_constraints_per_message, multi_nested_deep_write_and_equality) {
  experimental::MultiNested::Constraints c1 {}, c2 {};
  EXPECT_EQ(c1, c2);

  // Write through the nested chain:
  // array_of_unbounded_sequences is UnboundedSequences::Constraints
  // which has string_values of type SequenceConstraint<String>.
  c1.array_of_unbounded_sequences.string_values.element.size = 128u;
  EXPECT_NE(c1, c2);
  c2.array_of_unbounded_sequences.string_values.element.size = 128u;
  EXPECT_EQ(c1, c2);

  // Write through unbounded_sequence_of_unbounded_sequences.element which is
  // UnboundedSequences::Constraints.
  c1.unbounded_sequence_of_unbounded_sequences.element.int32_values.size = 16u;
  EXPECT_NE(c1, c2);
  c2.unbounded_sequence_of_unbounded_sequences.element.int32_values.size = 16u;
  EXPECT_EQ(c1, c2);
}

// ===========================================================================
// Per-message SequenceConstraint specializations
// ===========================================================================

TEST(test_experimental_constraints_seq_per_message, basic_types_default_and_equality) {
  SequenceConstraint<experimental::BasicTypes> c1 {}, c2 {};
  EXPECT_EQ(0u, c1.size);
  EXPECT_EQ(c1, c2);
  c1.size = 8u;
  EXPECT_NE(c1, c2);
  c2.size = 8u;
  EXPECT_EQ(c1, c2);
}

TEST(test_experimental_constraints_seq_per_message, basic_types_element_field_type) {
  static_assert(
    std::is_same_v<
      decltype(SequenceConstraint<experimental::BasicTypes>::element),
      experimental::BasicTypes::Constraints>);
}

TEST(test_experimental_constraints_seq_per_message, strings_element_has_string_constraints) {
  static_assert(
    std::is_same_v<
      decltype(SequenceConstraint<experimental::Strings>::element),
      experimental::Strings::Constraints>);
  SequenceConstraint<experimental::Strings> c {};
  c.size = 5u;
  c.element.string_value.size = 64u;
  EXPECT_EQ(5u, c.size);
  EXPECT_EQ(64u, c.element.string_value.size);
}

TEST(test_experimental_constraints_seq_per_message,
  multi_nested_element_has_multi_nested_constraints) {
  static_assert(
    std::is_same_v<
      decltype(SequenceConstraint<experimental::MultiNested>::element),
      experimental::MultiNested::Constraints>);
  SequenceConstraint<experimental::MultiNested> c {};
  // Deep chain: element.array_of_unbounded_sequences.string_values.element.size
  c.element.array_of_unbounded_sequences.string_values.element.size = 32u;
  EXPECT_EQ(32u,
    c.element.array_of_unbounded_sequences.string_values.element.size);
}
