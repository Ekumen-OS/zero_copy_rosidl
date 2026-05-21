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

// Tests for the experimental C generator output.
// Covers: scalar/string/array/sequence/nested field access patterns,
// __init/__fini/__create/__destroy, __are_equal, and __copy.

#include <float.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

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
#include "rosidl_generator_tests/msg/experimental/small_constant.h"
#include "rosidl_generator_tests/msg/experimental/strings.h"
#include "rosidl_generator_tests/msg/experimental/unbounded_sequences.h"
#include "rosidl_generator_tests/msg/experimental/w_strings.h"

#include "./test_macros.h"

// ---------------------------------------------------------------------------
// Empty
// ---------------------------------------------------------------------------

static int test_empty(void)
{
  EXP(Empty) * msg = EXP(Empty__create)(NULL);
  EXPECT_NE(NULL, msg);

  EXPECT_TRUE(EXP(Empty__are_equal)(msg, msg));
  EXPECT_FALSE(EXP(Empty__are_equal)(msg, NULL));
  EXPECT_FALSE(EXP(Empty__are_equal)(NULL, msg));

  EXP(Empty) * copy = EXP(Empty__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_TRUE(EXP(Empty__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(Empty__copy)(msg, copy));
  EXPECT_TRUE(EXP(Empty__are_equal)(msg, copy));

  EXP(Empty__destroy)(copy);
  EXP(Empty__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// BasicTypes
// ---------------------------------------------------------------------------

static int test_basic_types(void)
{
  EXP(BasicTypes) * msg = EXP(BasicTypes__create)(NULL);
  EXPECT_NE(NULL, msg);

  // Default initialization: all scalars must be zero
  EXPECT_FALSE(msg->bool_value.value->data);
  EXPECT_EQ(0, msg->byte_value.value->data);
  EXPECT_EQ(0, msg->char_value.value->data);
  EXPECT_EQ(0.0f, msg->float32_value.value->data);
  EXPECT_EQ(0.0, msg->float64_value.value->data);
  EXPECT_EQ(0, msg->int8_value.value->data);
  EXPECT_EQ(0u, msg->uint8_value.value->data);
  EXPECT_EQ(0, msg->int16_value.value->data);
  EXPECT_EQ(0u, msg->uint16_value.value->data);
  EXPECT_EQ(0, msg->int32_value.value->data);
  EXPECT_EQ(0u, msg->uint32_value.value->data);
  EXPECT_EQ(0, msg->int64_value.value->data);
  EXPECT_EQ(0u, msg->uint64_value.value->data);

  // Round-trip assignments
  msg->bool_value.value->data = true;
  msg->byte_value.value->data = 200u;
  msg->int32_value.value->data = -30000;
  msg->uint64_value.value->data = 50000000u;
  msg->float32_value.value->data = 1.125f;
  msg->float64_value.value->data = 3.14159;

  EXPECT_TRUE(msg->bool_value.value->data);
  EXPECT_EQ(200u, msg->byte_value.value->data);
  EXPECT_EQ(-30000, msg->int32_value.value->data);
  EXPECT_EQ(50000000u, msg->uint64_value.value->data);
  EXPECT_EQ(1.125f, msg->float32_value.value->data);
  EXPECT_EQ(3.14159, msg->float64_value.value->data);

  // are_equal / copy
  EXPECT_FALSE(EXP(BasicTypes__are_equal)(NULL, NULL));
  EXPECT_FALSE(EXP(BasicTypes__are_equal)(msg, NULL));
  EXPECT_TRUE(EXP(BasicTypes__are_equal)(msg, msg));

  EXP(BasicTypes) * copy = EXP(BasicTypes__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_FALSE(EXP(BasicTypes__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(BasicTypes__copy)(msg, copy));
  EXPECT_TRUE(EXP(BasicTypes__are_equal)(msg, copy));

  EXP(BasicTypes__destroy)(copy);
  EXP(BasicTypes__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// Defaults
// ---------------------------------------------------------------------------

static int test_defaults(void)
{
  EXP(Defaults) * msg = EXP(Defaults__create)(NULL);
  EXPECT_NE(NULL, msg);

  EXPECT_TRUE(msg->bool_value.value->data);
  EXPECT_EQ(50u, msg->byte_value.value->data);
  EXPECT_EQ(-30000, msg->int32_value.value->data);
  EXPECT_EQ(50000000u, msg->uint64_value.value->data);

  EXP(Defaults__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// Strings
// ---------------------------------------------------------------------------

static int test_strings(void)
{
  EXP(Strings) * msg = EXP(Strings__create)(NULL);
  EXPECT_NE(NULL, msg);

  // Empty after init
  EXPECT_EQ(0u, msg->string_value.size);
  EXPECT_EQ(0, strcmp((const char *)msg->string_value.value, ""));
  EXPECT_EQ(0u, msg->bounded_string_value.size);

  // Default-valued strings
  EXPECT_EQ(0, strcmp((const char *)msg->string_value_default1.value,
    "Hello world!"));
  EXPECT_EQ(0, strcmp((const char *)msg->bounded_string_value_default1.value,
    "Hello world!"));

  // Assign and re-read
  EXPECT_TRUE(EXP(Strings__string_value__assign)(&msg->string_value, "rosidl"));
  EXPECT_EQ(6u, msg->string_value.size);
  EXPECT_EQ(0, strcmp((const char *)msg->string_value.value, "rosidl"));

  // are_equal / copy
  EXPECT_FALSE(EXP(Strings__are_equal)(NULL, NULL));
  EXPECT_FALSE(EXP(Strings__are_equal)(msg, NULL));
  EXPECT_TRUE(EXP(Strings__are_equal)(msg, msg));

  EXP(Strings) * copy = EXP(Strings__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_FALSE(EXP(Strings__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(Strings__copy)(msg, copy));
  EXPECT_TRUE(EXP(Strings__are_equal)(msg, copy));

  EXP(Strings__destroy)(copy);
  EXP(Strings__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// Nested
// ---------------------------------------------------------------------------

static int test_nested(void)
{
  EXP(Nested) * msg = EXP(Nested__create)(NULL);
  EXPECT_NE(NULL, msg);

  EXPECT_FALSE(msg->basic_types_value.bool_value.value->data);
  EXPECT_EQ(0, msg->basic_types_value.int32_value.value->data);

  msg->basic_types_value.int32_value.value->data = 99;
  msg->basic_types_value.float32_value.value->data = 1.5f;

  EXPECT_EQ(99, msg->basic_types_value.int32_value.value->data);
  EXPECT_EQ(1.5f, msg->basic_types_value.float32_value.value->data);

  EXPECT_FALSE(EXP(Nested__are_equal)(NULL, NULL));
  EXPECT_FALSE(EXP(Nested__are_equal)(msg, NULL));
  EXPECT_TRUE(EXP(Nested__are_equal)(msg, msg));

  EXP(Nested) * copy = EXP(Nested__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_FALSE(EXP(Nested__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(Nested__copy)(msg, copy));
  EXPECT_TRUE(EXP(Nested__are_equal)(msg, copy));

  EXP(Nested__destroy)(copy);
  EXP(Nested__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// Arrays
// ---------------------------------------------------------------------------

static int test_arrays(void)
{
  EXP(Arrays) * msg = EXP(Arrays__create)(NULL);
  EXPECT_NE(NULL, msg);

  // Scalar array: value->data[i]
  EXPECT_EQ(0, msg->int32_values.value->data[0]);
  msg->int32_values.value->data[0] = 10;
  msg->int32_values.value->data[1] = 20;
  msg->int32_values.value->data[2] = 30;
  EXPECT_EQ(10, msg->int32_values.value->data[0]);
  EXPECT_EQ(20, msg->int32_values.value->data[1]);
  EXPECT_EQ(30, msg->int32_values.value->data[2]);

  // Scalar array defaults
  EXPECT_EQ(0, msg->byte_values_default.value->data[0]);
  EXPECT_EQ(1u, msg->byte_values_default.value->data[1]);
  EXPECT_EQ(255u, msg->byte_values_default.value->data[2]);
  EXPECT_EQ(1.125f, msg->float32_values_default.value->data[0]);
  EXPECT_EQ(0.0f, msg->float32_values_default.value->data[1]);
  EXPECT_EQ(-1.125f, msg->float32_values_default.value->data[2]);
  EXPECT_EQ(0, msg->int32_values_default.value->data[0]);
  EXPECT_EQ(INT32_MAX, msg->int32_values_default.value->data[1]);
  EXPECT_EQ(INT32_MIN, msg->int32_values_default.value->data[2]);
  EXPECT_EQ(0u, msg->uint64_values_default.value->data[0]);
  EXPECT_EQ(1u, msg->uint64_values_default.value->data[1]);
  EXPECT_EQ(UINT64_MAX, msg->uint64_values_default.value->data[2]);

  // String array (ARRAY): access via value->data[i]
  EXPECT_EQ(0u, msg->string_values.value->data[0].size);
  EXPECT_TRUE(
    rosidl_runtime_c__experimental__String__assign(
      &msg->string_values.value->data[0], "first"));
  EXPECT_TRUE(
    rosidl_runtime_c__experimental__String__assign(
      &msg->string_values.value->data[2], "third"));
  EXPECT_EQ(0, strcmp((const char *)msg->string_values.value->data[0].value, "first"));
  EXPECT_EQ(0u, msg->string_values.value->data[1].size);
  EXPECT_EQ(0, strcmp((const char *)msg->string_values.value->data[2].value, "third"));

  // String array defaults
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values_default.value->data[0].value, ""));
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values_default.value->data[1].value, "max value"));
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values_default.value->data[2].value, "min value"));

  // Sub-message array (ARRAY): access via value->data[i]
  EXPECT_EQ(0, msg->basic_types_values.value->data[0].int32_value.value->data);
  msg->basic_types_values.value->data[1].int32_value.value->data = 42;
  EXPECT_EQ(0, msg->basic_types_values.value->data[0].int32_value.value->data);
  EXPECT_EQ(42, msg->basic_types_values.value->data[1].int32_value.value->data);
  EXPECT_EQ(0, msg->basic_types_values.value->data[2].int32_value.value->data);

  EXPECT_TRUE(EXP(Arrays__are_equal)(msg, msg));
  EXPECT_FALSE(EXP(Arrays__are_equal)(msg, NULL));

  EXP(Arrays) * copy = EXP(Arrays__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_FALSE(EXP(Arrays__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(Arrays__copy)(msg, copy));
  EXPECT_TRUE(EXP(Arrays__are_equal)(msg, copy));

  EXP(Arrays__destroy)(copy);
  EXP(Arrays__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// BoundedSequences
// ---------------------------------------------------------------------------

static int test_bounded_sequences(void)
{
  EXP(BoundedSequences) * msg = EXP(BoundedSequences__create)(NULL);
  EXPECT_NE(NULL, msg);

  // All sequences empty by default
  EXPECT_EQ(0u, msg->bool_values.size);
  EXPECT_EQ(0u, msg->int32_values.size);
  EXPECT_EQ(0u, msg->string_values.size);
  EXPECT_EQ(0u, msg->basic_types_values.size);

  // Scalar push_back (by value)
  EXPECT_TRUE(EXP(BoundedSequences__int32_values__push_back)(
    &msg->int32_values, 10));
  EXPECT_TRUE(EXP(BoundedSequences__int32_values__push_back)(
    &msg->int32_values, -5));
  EXPECT_EQ(2u, msg->int32_values.size);
  EXPECT_EQ(10, msg->int32_values.value[0]);
  EXPECT_EQ(-5, msg->int32_values.value[1]);

  // String push_back (by pointer to initialized elem)
  {
    rosidl_runtime_c__experimental__String elem;
    EXPECT_TRUE(rosidl_runtime_c__experimental__String__init(&elem));
    EXPECT_TRUE(rosidl_runtime_c__experimental__String__assign(
      &elem, "hello"));
    EXPECT_TRUE(EXP(BoundedSequences__string_values__push_back)(
      &msg->string_values, &elem));
    rosidl_runtime_c__experimental__String__fini(&elem);
  }
  EXPECT_EQ(1u, msg->string_values.size);
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values.value[0].value, "hello"));

  // Sub-message push_back (by pointer)
  {
    EXP(BasicTypes) sub;
    EXPECT_TRUE(EXP(BasicTypes__init)(&sub));
    sub.int32_value.value->data = 77;
    EXPECT_TRUE(EXP(BoundedSequences__basic_types_values__push_back)(
      &msg->basic_types_values, &sub));
    EXP(BasicTypes__fini)(&sub);
  }
  EXPECT_EQ(1u, msg->basic_types_values.size);
  EXPECT_EQ(77, msg->basic_types_values.value[0].int32_value.value->data);

  // Scalar defaults are pre-populated
  EXPECT_EQ(3u, msg->bool_values_default.size);
  EXPECT_FALSE(msg->bool_values_default.value[0]);
  EXPECT_TRUE(msg->bool_values_default.value[1]);
  EXPECT_FALSE(msg->bool_values_default.value[2]);
  EXPECT_EQ(3u, msg->int32_values_default.size);
  EXPECT_EQ(0, msg->int32_values_default.value[0]);
  EXPECT_EQ(INT32_MAX, msg->int32_values_default.value[1]);
  EXPECT_EQ(INT32_MIN, msg->int32_values_default.value[2]);

  // String defaults are pre-populated
  EXPECT_EQ(3u, msg->string_values_default.size);
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values_default.value[0].value, ""));
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values_default.value[1].value, "max value"));
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values_default.value[2].value, "min value"));

  EXPECT_TRUE(EXP(BoundedSequences__are_equal)(msg, msg));
  EXPECT_FALSE(EXP(BoundedSequences__are_equal)(msg, NULL));

  EXP(BoundedSequences) * copy = EXP(BoundedSequences__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_FALSE(EXP(BoundedSequences__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(BoundedSequences__copy)(msg, copy));
  EXPECT_TRUE(EXP(BoundedSequences__are_equal)(msg, copy));

  EXP(BoundedSequences__destroy)(copy);
  EXP(BoundedSequences__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// BoundedPlainSequences
// ---------------------------------------------------------------------------

static int test_bounded_plain_sequences(void)
{
  EXP(BoundedPlainSequences) * msg = EXP(BoundedPlainSequences__create)(NULL);
  EXPECT_NE(NULL, msg);

  EXPECT_EQ(0u, msg->bool_values.size);
  EXPECT_EQ(0u, msg->uint64_values.size);
  EXPECT_EQ(0u, msg->basic_types_values.size);

  EXPECT_TRUE(EXP(BoundedPlainSequences__int32_values__push_back)(
    &msg->int32_values, 1));
  EXPECT_TRUE(EXP(BoundedPlainSequences__int32_values__push_back)(
    &msg->int32_values, 2));
  EXPECT_TRUE(EXP(BoundedPlainSequences__int32_values__push_back)(
    &msg->int32_values, 3));
  EXPECT_EQ(3u, msg->int32_values.size);
  EXPECT_EQ(1 + 2 + 3,
    msg->int32_values.value[0] +
    msg->int32_values.value[1] +
    msg->int32_values.value[2]);

  {
    EXP(BasicTypes) sub;
    EXPECT_TRUE(EXP(BasicTypes__init)(&sub));
    sub.uint16_value.value->data = 500u;
    EXPECT_TRUE(EXP(BoundedPlainSequences__basic_types_values__push_back)(
      &msg->basic_types_values, &sub));
    EXP(BasicTypes__fini)(&sub);
  }
  EXPECT_EQ(1u, msg->basic_types_values.size);
  EXPECT_EQ(500u, msg->basic_types_values.value[0].uint16_value.value->data);

  EXP(BoundedPlainSequences) * copy = EXP(BoundedPlainSequences__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_FALSE(EXP(BoundedPlainSequences__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(BoundedPlainSequences__copy)(msg, copy));
  EXPECT_TRUE(EXP(BoundedPlainSequences__are_equal)(msg, copy));

  EXP(BoundedPlainSequences__destroy)(copy);
  EXP(BoundedPlainSequences__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// UnboundedSequences
// ---------------------------------------------------------------------------

static int test_unbounded_sequences(void)
{
  EXP(UnboundedSequences) * msg = EXP(UnboundedSequences__create)(NULL);
  EXPECT_NE(NULL, msg);

  EXPECT_EQ(0u, msg->bool_values.size);
  EXPECT_EQ(0u, msg->string_values.size);
  EXPECT_EQ(0u, msg->basic_types_values.size);

  // Scalar push/grow/read
  EXPECT_TRUE(EXP(UnboundedSequences__int64_values__push_back)(
    &msg->int64_values, 100));
  EXPECT_TRUE(EXP(UnboundedSequences__int64_values__push_back)(
    &msg->int64_values, 200));
  EXPECT_TRUE(EXP(UnboundedSequences__int64_values__push_back)(
    &msg->int64_values, 300));
  EXPECT_TRUE(EXP(UnboundedSequences__int64_values__push_back)(
    &msg->int64_values, 400));
  EXPECT_EQ(4u, msg->int64_values.size);
  EXPECT_EQ(300, msg->int64_values.value[2]);

  // String push_back
  {
    rosidl_runtime_c__experimental__String e;
    EXPECT_TRUE(rosidl_runtime_c__experimental__String__init(&e));
    EXPECT_TRUE(rosidl_runtime_c__experimental__String__assign(
      &e, "alpha"));
    EXPECT_TRUE(EXP(UnboundedSequences__string_values__push_back)(
      &msg->string_values, &e));
    rosidl_runtime_c__experimental__String__fini(&e);
  }
  {
    rosidl_runtime_c__experimental__String e;
    EXPECT_TRUE(rosidl_runtime_c__experimental__String__init(&e));
    EXPECT_TRUE(rosidl_runtime_c__experimental__String__assign(
      &e, "beta"));
    EXPECT_TRUE(EXP(UnboundedSequences__string_values__push_back)(
      &msg->string_values, &e));
    rosidl_runtime_c__experimental__String__fini(&e);
  }
  EXPECT_EQ(2u, msg->string_values.size);
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values.value[0].value, "alpha"));
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values.value[1].value, "beta"));

  // Sub-message push_back
  {
    EXP(BasicTypes) sub;
    EXPECT_TRUE(EXP(BasicTypes__init)(&sub));
    sub.float64_value.value->data = 3.14;
    EXPECT_TRUE(EXP(UnboundedSequences__basic_types_values__push_back)(
      &msg->basic_types_values, &sub));
    EXP(BasicTypes__fini)(&sub);
  }
  EXPECT_EQ(1u, msg->basic_types_values.size);
  EXPECT_EQ(3.14, msg->basic_types_values.value[0].float64_value.value->data);

  // Scalar defaults pre-populated
  EXPECT_EQ(3u, msg->float32_values_default.size);
  EXPECT_EQ(1.125f, msg->float32_values_default.value[0]);
  EXPECT_EQ(0.0f, msg->float32_values_default.value[1]);
  EXPECT_EQ(-1.125f, msg->float32_values_default.value[2]);
  EXPECT_EQ(3u, msg->uint64_values_default.size);
  EXPECT_EQ(0u, msg->uint64_values_default.value[0]);
  EXPECT_EQ(1u, msg->uint64_values_default.value[1]);
  EXPECT_EQ(UINT64_MAX, msg->uint64_values_default.value[2]);

  // String defaults pre-populated
  EXPECT_EQ(3u, msg->string_values_default.size);
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values_default.value[0].value, ""));
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values_default.value[1].value, "max value"));
  EXPECT_EQ(0,
    strcmp((const char *)msg->string_values_default.value[2].value, "min value"));

  EXPECT_TRUE(EXP(UnboundedSequences__are_equal)(msg, msg));
  EXP(UnboundedSequences) * copy = EXP(UnboundedSequences__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_FALSE(EXP(UnboundedSequences__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(UnboundedSequences__copy)(msg, copy));
  EXPECT_TRUE(EXP(UnboundedSequences__are_equal)(msg, copy));

  EXP(UnboundedSequences__destroy)(copy);
  EXP(UnboundedSequences__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// MultiNested
// ---------------------------------------------------------------------------

static int test_multi_nested(void)
{
  EXP(MultiNested) * msg = EXP(MultiNested__create)(NULL);
  EXPECT_NE(NULL, msg);

  // Array of Arrays (ARRAY of Arrays sub-messages)
  EXPECT_EQ(0, msg->array_of_arrays.value->data[0].int32_values.value->data[0]);
  msg->array_of_arrays.value->data[0].int32_values.value->data[1] = 55;
  EXPECT_EQ(55, msg->array_of_arrays.value->data[0].int32_values.value->data[1]);
  // Adjacent elements untouched
  EXPECT_EQ(0, msg->array_of_arrays.value->data[0].int32_values.value->data[0]);
  EXPECT_EQ(0, msg->array_of_arrays.value->data[1].int32_values.value->data[1]);

  // Sequences of sub-messages start empty
  EXPECT_EQ(0u, msg->bounded_sequence_of_arrays.size);
  EXPECT_EQ(0u, msg->unbounded_sequence_of_bounded_sequences.size);
  EXPECT_EQ(0u, msg->unbounded_sequence_of_unbounded_sequences.size);

  // Unbounded sequence of BoundedSequences: push + read
  {
    EXP(BoundedSequences) sub;
    EXPECT_TRUE(EXP(BoundedSequences__init)(&sub));
    EXPECT_TRUE(EXP(BoundedSequences__int32_values__push_back)(
      &sub.int32_values, 9));
    EXPECT_TRUE(
      EXP(MultiNested__unbounded_sequence_of_bounded_sequences__push_back)(
        &msg->unbounded_sequence_of_bounded_sequences, &sub));
    EXP(BoundedSequences__fini)(&sub);
  }
  EXPECT_EQ(1u, msg->unbounded_sequence_of_bounded_sequences.size);
  EXPECT_EQ(9,
    msg->unbounded_sequence_of_bounded_sequences.value[0].int32_values.value[0]);

  EXPECT_TRUE(EXP(MultiNested__are_equal)(msg, msg));
  EXP(MultiNested) * copy = EXP(MultiNested__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_FALSE(EXP(MultiNested__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(MultiNested__copy)(msg, copy));
  EXPECT_TRUE(EXP(MultiNested__are_equal)(msg, copy));

  EXP(MultiNested__destroy)(copy);
  EXP(MultiNested__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// BasicIdl — single float field via IDL syntax
// ---------------------------------------------------------------------------

static int test_basic_idl(void)
{
  EXP(BasicIdl) * msg = EXP(BasicIdl__create)(NULL);
  EXPECT_NE(NULL, msg);

  EXPECT_EQ(0.0f, msg->x.value->data);
  msg->x.value->data = 2.718f;
  EXPECT_EQ(2.718f, msg->x.value->data);

  EXP(BasicIdl) * copy = EXP(BasicIdl__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_FALSE(EXP(BasicIdl__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(BasicIdl__copy)(msg, copy));
  EXPECT_TRUE(EXP(BasicIdl__are_equal)(msg, copy));

  EXP(BasicIdl__destroy)(copy);
  EXP(BasicIdl__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// WStrings
// ---------------------------------------------------------------------------

static int test_wstrings(void)
{
  EXP(WStrings) * msg = EXP(WStrings__create)(NULL);
  EXPECT_NE(NULL, msg);

  EXPECT_EQ(0u, msg->wstring_value.size);
  EXPECT_EQ(0u, msg->unbounded_sequence_of_wstrings.size);

  const char16_t hello[] = u"hello";
  EXPECT_TRUE(
    EXP(WStrings__wstring_value__assign)(&msg->wstring_value, hello));
  EXPECT_EQ(5u, msg->wstring_value.size);
  EXPECT_EQ(0, memcmp(msg->wstring_value.value, hello, 5 * sizeof(char16_t)));

  // Wstring array (ARRAY)
  EXPECT_EQ(0u, msg->array_of_wstrings.value->data[0].size);
  const char16_t world[] = u"world";
  EXPECT_TRUE(rosidl_runtime_c__experimental__WString__assign(
    &msg->array_of_wstrings.value->data[1], world));
  EXPECT_EQ(0u, msg->array_of_wstrings.value->data[0].size);
  EXPECT_EQ(5u, msg->array_of_wstrings.value->data[1].size);
  EXPECT_EQ(0u, msg->array_of_wstrings.value->data[2].size);

  EXPECT_TRUE(EXP(WStrings__are_equal)(msg, msg));
  EXP(WStrings) * copy = EXP(WStrings__create)(NULL);
  EXPECT_NE(NULL, copy);
  EXPECT_FALSE(EXP(WStrings__are_equal)(msg, copy));
  EXPECT_TRUE(EXP(WStrings__copy)(msg, copy));
  EXPECT_TRUE(EXP(WStrings__are_equal)(msg, copy));

  EXP(WStrings__destroy)(copy);
  EXP(WStrings__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
  int rc = 0;
  printf("test_experimental_interfaces_c\n");
  RUN(test_empty);
  RUN(test_basic_types);
  RUN(test_defaults);
  RUN(test_strings);
  RUN(test_nested);
  RUN(test_arrays);
  RUN(test_bounded_sequences);
  RUN(test_bounded_plain_sequences);
  RUN(test_unbounded_sequences);
  RUN(test_multi_nested);
  RUN(test_basic_idl);
  RUN(test_wstrings);
  if (rc != 0) {
    fprintf(stderr, "%d test(s) FAILED\n", rc);
  } else {
    printf("All tests passed.\n");
  }
  return rc != 0;
}
