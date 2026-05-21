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

// Tests for the experimental C generator constraint types.
// Mirrors the C++ test_experimental_constraints.cpp.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rosidl_runtime_c/experimental/constraints.h"

#include "rosidl_generator_tests/msg/experimental/arrays.h"
#include "rosidl_generator_tests/msg/experimental/basic_idl.h"
#include "rosidl_generator_tests/msg/experimental/basic_types.h"
#include "rosidl_generator_tests/msg/experimental/bounded_plain_sequences.h"
#include "rosidl_generator_tests/msg/experimental/bounded_sequences.h"
#include "rosidl_generator_tests/msg/experimental/constants.h"
#include "rosidl_generator_tests/msg/experimental/defaults.h"
#include "rosidl_generator_tests/msg/experimental/empty.h"
#include "rosidl_generator_tests/msg/experimental/multi_nested.h"
#include "rosidl_generator_tests/msg/experimental/nested.h"
#include "rosidl_generator_tests/msg/experimental/strings.h"
#include "rosidl_generator_tests/msg/experimental/unbounded_sequences.h"
#include "rosidl_generator_tests/msg/experimental/w_strings.h"

#include "./test_macros.h"

// Convenience macro for zeroed stack initialization
#define ZERO(var) memset(&(var), 0, sizeof(var))

// ---------------------------------------------------------------------------
// StringConstraint (runtime type)
// ---------------------------------------------------------------------------

static int test_string_constraint_default(void)
{
  rosidl_runtime_c__experimental__StringConstraint c;
  ZERO(c);
  EXPECT_EQ(0u, c.size);
  return 0;
}

static int test_string_constraint_equality(void)
{
  rosidl_runtime_c__experimental__StringConstraint a, b;
  ZERO(a); ZERO(b);
  EXPECT_TRUE(
    rosidl_runtime_c__experimental__StringConstraint__are_equal(&a, &b));
  a.size = 64u;
  EXPECT_FALSE(
    rosidl_runtime_c__experimental__StringConstraint__are_equal(&a, &b));
  b.size = 64u;
  EXPECT_TRUE(
    rosidl_runtime_c__experimental__StringConstraint__are_equal(&a, &b));
  return 0;
}

// ---------------------------------------------------------------------------
// Empty / BasicTypes / Defaults / Constants
// Constraints struct has no fields (all scalar members).
// are_equal always returns true regardless of the placeholder byte.
// ---------------------------------------------------------------------------

static int test_empty_constraints_is_empty(void)
{
  EXP(Empty__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(Empty__Constraints__are_equal)(&c1, &c2));
  return 0;
}

static int test_basic_types_constraints_is_empty(void)
{
  EXP(BasicTypes__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(BasicTypes__Constraints__are_equal)(&c1, &c2));
  return 0;
}

static int test_defaults_constraints_is_empty(void)
{
  EXP(Defaults__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(Defaults__Constraints__are_equal)(&c1, &c2));
  return 0;
}

static int test_constants_constraints_is_empty(void)
{
  EXP(Constants__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(Constants__Constraints__are_equal)(&c1, &c2));
  return 0;
}

// ---------------------------------------------------------------------------
// BoundedSequences / BoundedPlainSequences
// All sequences are bounded → empty Constraints.
// ---------------------------------------------------------------------------

static int test_bounded_sequences_constraints_is_empty(void)
{
  EXP(BoundedSequences__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(BoundedSequences__Constraints__are_equal)(&c1, &c2));
  return 0;
}

static int test_bounded_plain_sequences_constraints_is_empty(void)
{
  EXP(BoundedPlainSequences__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(BoundedPlainSequences__Constraints__are_equal)(&c1, &c2));
  return 0;
}

// ---------------------------------------------------------------------------
// Nested
// One NamespacedType field → Constraints { BasicTypes__Constraints basic_types_value; }
// BasicTypes has no variable members → BasicTypes__Constraints has no fields.
// ---------------------------------------------------------------------------

static int test_nested_constraints_sub_message_field(void)
{
  EXP(Nested__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(Nested__Constraints__are_equal)(&c1, &c2));
  // Verify the field exists (access without assertion)
  (void)c1.basic_types_value;
  return 0;
}

// ---------------------------------------------------------------------------
// Strings
// Unbounded string fields → StringConstraint per field.
// Bounded string fields → no entry in Constraints.
// ---------------------------------------------------------------------------

static int test_strings_constraint_fields(void)
{
  EXP(Strings__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(Strings__Constraints__are_equal)(&c1, &c2));

  // Verify all five unbounded string fields exist
  (void)c1.string_value;
  (void)c1.string_value_default1;
  (void)c1.string_value_default2;
  (void)c1.string_value_default3;
  // bounded_string_value and bounded_string_value_default1/2/3 do NOT appear

  c1.string_value.size = 256u;
  EXPECT_FALSE(EXP(Strings__Constraints__are_equal)(&c1, &c2));
  c2.string_value.size = 256u;
  EXPECT_TRUE(EXP(Strings__Constraints__are_equal)(&c1, &c2));

  c1.string_value_default3.size = 128u;
  EXPECT_FALSE(EXP(Strings__Constraints__are_equal)(&c1, &c2));
  c2.string_value_default3.size = 128u;
  EXPECT_TRUE(EXP(Strings__Constraints__are_equal)(&c1, &c2));
  return 0;
}

// ---------------------------------------------------------------------------
// WStrings
// Standalone unbounded wstring fields → StringConstraint.
// array_of_wstrings (WString[3]) → StringConstraint (shared cap for elements).
// bounded_sequence_of_wstrings (bounded) → no entry.
// unbounded_sequence_of_wstrings → SequenceConstraint with StringConstraint element.
// ---------------------------------------------------------------------------

static int test_wstrings_standalone_fields(void)
{
  EXP(WStrings__Constraints) c;
  ZERO(c);
  // All four standalone unbounded wstring fields must exist
  (void)c.wstring_value;
  (void)c.wstring_value_default1;
  (void)c.wstring_value_default2;
  (void)c.wstring_value_default3;
  return 0;
}

static int test_wstrings_array_field(void)
{
  EXP(WStrings__Constraints) c;
  ZERO(c);
  // array_of_wstrings (WString[3]) → single StringConstraint (shared element cap)
  c.array_of_wstrings.size = 64u;
  EXPECT_EQ(64u, c.array_of_wstrings.size);
  return 0;
}

static int test_wstrings_unbounded_sequence_field(void)
{
  EXP(WStrings__Constraints) c;
  ZERO(c);
  // unbounded_sequence_of_wstrings → SequenceConstraint with StringConstraint element
  c.unbounded_sequence_of_wstrings.size = 10u;
  c.unbounded_sequence_of_wstrings.element.size = 32u;
  EXPECT_EQ(10u, c.unbounded_sequence_of_wstrings.size);
  EXPECT_EQ(32u, c.unbounded_sequence_of_wstrings.element.size);
  return 0;
}

static int test_wstrings_equality(void)
{
  EXP(WStrings__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(WStrings__Constraints__are_equal)(&c1, &c2));
  c1.wstring_value.size = 16u;
  EXPECT_FALSE(EXP(WStrings__Constraints__are_equal)(&c1, &c2));
  c2.wstring_value.size = 16u;
  EXPECT_TRUE(EXP(WStrings__Constraints__are_equal)(&c1, &c2));
  return 0;
}

// ---------------------------------------------------------------------------
// Arrays
// string[3] → StringConstraint; SubMsg[3] → SubMsg__Constraints; BasicType[3] → none.
// ---------------------------------------------------------------------------

static int test_arrays_string_fields(void)
{
  EXP(Arrays__Constraints) c;
  ZERO(c);
  // string_values (string[3]) and string_values_default (string[3])
  c.string_values.size = 100u;
  EXPECT_EQ(100u, c.string_values.size);
  (void)c.string_values_default;
  return 0;
}

static int test_arrays_sub_message_fields(void)
{
  EXP(Arrays__Constraints) c;
  ZERO(c);
  // basic_types_values, constants_values, defaults_values
  (void)c.basic_types_values;
  (void)c.constants_values;
  (void)c.defaults_values;
  return 0;
}

static int test_arrays_equality(void)
{
  EXP(Arrays__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(Arrays__Constraints__are_equal)(&c1, &c2));
  c1.string_values_default.size = 22u;
  EXPECT_FALSE(EXP(Arrays__Constraints__are_equal)(&c1, &c2));
  c2.string_values_default.size = 22u;
  EXPECT_TRUE(EXP(Arrays__Constraints__are_equal)(&c1, &c2));
  return 0;
}

// ---------------------------------------------------------------------------
// UnboundedSequences
// scalar[] → per-field SequenceConstraint with only size.
// string[] → per-field SequenceConstraint with size + StringConstraint element.
// SubMsg[] → per-field SequenceConstraint with size + SubMsg__Constraints element.
// ---------------------------------------------------------------------------

static int test_unbounded_sequences_scalar_fields(void)
{
  EXP(UnboundedSequences__Constraints) c;
  ZERO(c);
  c.int32_values.size = 50u;
  EXPECT_EQ(50u, c.int32_values.size);
  (void)c.bool_values;
  (void)c.float64_values;
  return 0;
}

static int test_unbounded_sequences_string_field(void)
{
  EXP(UnboundedSequences__Constraints) c;
  ZERO(c);
  c.string_values.size = 20u;
  c.string_values.element.size = 64u;
  EXPECT_EQ(20u, c.string_values.size);
  EXPECT_EQ(64u, c.string_values.element.size);
  return 0;
}

static int test_unbounded_sequences_sub_message_fields(void)
{
  EXP(UnboundedSequences__Constraints) c;
  ZERO(c);
  c.basic_types_values.size = 8u;
  EXPECT_EQ(8u, c.basic_types_values.size);
  (void)c.defaults_values;
  (void)c.constants_values;
  return 0;
}

static int test_unbounded_sequences_equality(void)
{
  EXP(UnboundedSequences__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(UnboundedSequences__Constraints__are_equal)(&c1, &c2));
  c1.string_values.element.size = 32u;
  EXPECT_FALSE(EXP(UnboundedSequences__Constraints__are_equal)(&c1, &c2));
  c2.string_values.element.size = 32u;
  EXPECT_TRUE(EXP(UnboundedSequences__Constraints__are_equal)(&c1, &c2));
  return 0;
}

// ---------------------------------------------------------------------------
// MultiNested
// Arrays[3]             → Arrays__Constraints
// BoundedSequences[3]   → BoundedSequences__Constraints (empty)
// UnboundedSequences[3] → UnboundedSequences__Constraints
// Arrays[<=3]           → nothing (bounded)
// UnboundedSequences[]  → SequenceConstraint<UnboundedSequences>
// ---------------------------------------------------------------------------

static int test_multi_nested_array_fields(void)
{
  EXP(MultiNested__Constraints) c;
  ZERO(c);
  (void)c.array_of_arrays;
  (void)c.array_of_bounded_sequences;
  (void)c.array_of_unbounded_sequences;
  return 0;
}

static int test_multi_nested_unbounded_sequence_fields(void)
{
  EXP(MultiNested__Constraints) c;
  ZERO(c);
  (void)c.unbounded_sequence_of_arrays;
  (void)c.unbounded_sequence_of_bounded_sequences;
  (void)c.unbounded_sequence_of_unbounded_sequences;
  return 0;
}

static int test_multi_nested_deep_write_and_equality(void)
{
  EXP(MultiNested__Constraints) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_TRUE(EXP(MultiNested__Constraints__are_equal)(&c1, &c2));

  // array_of_unbounded_sequences is UnboundedSequences__Constraints
  // which has string_values of type SequenceConstraint with StringConstraint element.
  c1.array_of_unbounded_sequences.string_values.element.size = 128u;
  EXPECT_FALSE(EXP(MultiNested__Constraints__are_equal)(&c1, &c2));
  c2.array_of_unbounded_sequences.string_values.element.size = 128u;
  EXPECT_TRUE(EXP(MultiNested__Constraints__are_equal)(&c1, &c2));

  // unbounded_sequence_of_unbounded_sequences.element is UnboundedSequences__Constraints.
  c1.unbounded_sequence_of_unbounded_sequences.element.int32_values.size = 16u;
  EXPECT_FALSE(EXP(MultiNested__Constraints__are_equal)(&c1, &c2));
  c2.unbounded_sequence_of_unbounded_sequences.element.int32_values.size = 16u;
  EXPECT_TRUE(EXP(MultiNested__Constraints__are_equal)(&c1, &c2));
  return 0;
}

// ---------------------------------------------------------------------------
// Per-message SequenceConstraint
// ---------------------------------------------------------------------------

static int test_seq_constraint_basic_types(void)
{
  EXP(BasicTypes__SequenceConstraint) c1, c2;
  ZERO(c1); ZERO(c2);
  EXPECT_EQ(0u, c1.size);
  EXPECT_TRUE(EXP(BasicTypes__SequenceConstraint__are_equal)(&c1, &c2));
  c1.size = 8u;
  EXPECT_FALSE(EXP(BasicTypes__SequenceConstraint__are_equal)(&c1, &c2));
  c2.size = 8u;
  EXPECT_TRUE(EXP(BasicTypes__SequenceConstraint__are_equal)(&c1, &c2));
  // element is BasicTypes__Constraints (empty)
  (void)c1.element;
  return 0;
}

static int test_seq_constraint_strings_element_has_string_constraints(void)
{
  EXP(Strings__SequenceConstraint) c;
  ZERO(c);
  c.size = 5u;
  c.element.string_value.size = 64u;
  EXPECT_EQ(5u, c.size);
  EXPECT_EQ(64u, c.element.string_value.size);
  return 0;
}

static int test_seq_constraint_multi_nested_deep_chain(void)
{
  EXP(MultiNested__SequenceConstraint) c;
  ZERO(c);
  // element.array_of_unbounded_sequences.string_values.element.size
  c.element.array_of_unbounded_sequences.string_values.element.size = 32u;
  EXPECT_EQ(
    32u,
    c.element.array_of_unbounded_sequences.string_values.element.size);
  return 0;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
  int rc = 0;
  printf("test_experimental_constraints_c\n");

  RUN(test_string_constraint_default);
  RUN(test_string_constraint_equality);

  RUN(test_empty_constraints_is_empty);
  RUN(test_basic_types_constraints_is_empty);
  RUN(test_defaults_constraints_is_empty);
  RUN(test_constants_constraints_is_empty);
  RUN(test_bounded_sequences_constraints_is_empty);
  RUN(test_bounded_plain_sequences_constraints_is_empty);

  RUN(test_nested_constraints_sub_message_field);

  RUN(test_strings_constraint_fields);

  RUN(test_wstrings_standalone_fields);
  RUN(test_wstrings_array_field);
  RUN(test_wstrings_unbounded_sequence_field);
  RUN(test_wstrings_equality);

  RUN(test_arrays_string_fields);
  RUN(test_arrays_sub_message_fields);
  RUN(test_arrays_equality);

  RUN(test_unbounded_sequences_scalar_fields);
  RUN(test_unbounded_sequences_string_field);
  RUN(test_unbounded_sequences_sub_message_fields);
  RUN(test_unbounded_sequences_equality);

  RUN(test_multi_nested_array_fields);
  RUN(test_multi_nested_unbounded_sequence_fields);
  RUN(test_multi_nested_deep_write_and_equality);

  RUN(test_seq_constraint_basic_types);
  RUN(test_seq_constraint_strings_element_has_string_constraints);
  RUN(test_seq_constraint_multi_nested_deep_chain);

  if (rc != 0) {
    fprintf(stderr, "%d test(s) FAILED\n", rc);
  } else {
    printf("All tests passed.\n");
  }
  return rc != 0;
}
