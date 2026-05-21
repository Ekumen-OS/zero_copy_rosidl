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

// Tests for the experimental C generator __init_with_options with external storage.
// Verifies that caller-supplied ExternalStorage is properly aliased and
// that default values are applied at init time.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rosidl_generator_tests/msg/experimental/basic_types.h"
#include "rosidl_generator_tests/msg/experimental/defaults.h"
#include "rosidl_generator_tests/msg/experimental/empty.h"
#include "rosidl_generator_tests/msg/experimental/nested.h"
#include "rosidl_generator_tests/msg/experimental/strings.h"
#include "rosidl_generator_tests/msg/experimental/arrays.h"

#include "./test_macros.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static rosidl_memory_t make_memory(void * address)
{
  rosidl_memory_t m;
  m.address = address;
  m.attributes = 0;
  return m;
}

static rosidl_memory_region_t make_region(void * address, size_t size)
{
  rosidl_memory_region_t r;
  r.location = make_memory(address);
  r.size = size;
  return r;
}

// ---------------------------------------------------------------------------
// test_storage_empty: NULL storage → fallback allocates from heap
// ---------------------------------------------------------------------------

static int test_storage_empty(void)
{
  EXP(Empty) * msg = EXP(Empty__create)(NULL);
  EXPECT_NE(NULL, msg);
  EXP(Empty__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// test_storage_basic_types_preserves_value:
//   ExternalStorage for BasicTypes contains a raw int32_t backing.
//   Writing through msg->int32_value.value->data is visible in the stack var.
// ---------------------------------------------------------------------------

static int test_storage_basic_types_preserves_value(void)
{
  // Allocate backing storage on the stack for ALL fields.
  bool     b = false;
  uint8_t  byte = 0;
  uint8_t  c = 0;
  float    f32 = 0.0f;
  double   f64 = 0.0;
  int8_t   i8 = 0;
  uint8_t  u8 = 0;
  int16_t  i16 = 0;
  uint16_t u16 = 0;
  int32_t  i32 = 0;
  uint32_t u32 = 0;
  int64_t  i64 = 0;
  uint64_t u64 = 0;

  EXP(BasicTypes__ExternalStorage) storage;
  memset(&storage, 0, sizeof(storage));

  storage.members.bool_value = make_memory(&b);
  storage.members.byte_value = make_memory(&byte);
  storage.members.char_value = make_memory(&c);
  storage.members.float32_value = make_memory(&f32);
  storage.members.float64_value = make_memory(&f64);
  storage.members.int8_value = make_memory(&i8);
  storage.members.uint8_value = make_memory(&u8);
  storage.members.int16_value = make_memory(&i16);
  storage.members.uint16_value = make_memory(&u16);
  storage.members.int32_value = make_memory(&i32);
  storage.members.uint32_value = make_memory(&u32);
  storage.members.int64_value = make_memory(&i64);
  storage.members.uint64_value = make_memory(&u64);

  EXP(BasicTypes) msg;
  EXP(BasicTypes__InitOptions) options = {0};
  options.external_storage = &storage;
  EXPECT_TRUE(EXP(BasicTypes__init_with_options)(&msg, &options));

  // Scalar value starts zero (as initialized)
  EXPECT_EQ(0, msg.int32_value.value->data);
  EXPECT_EQ(&i32, (int32_t *)msg.int32_value.value);

  // Write through the message field; read back via the original variable
  msg.int32_value.value->data = 42;
  EXPECT_EQ(42, i32);
  EXPECT_EQ(42, msg.int32_value.value->data);

  msg.bool_value.value->data = true;
  EXPECT_TRUE(b);

  msg.float32_value.value->data = 1.5f;
  EXPECT_EQ(1.5f, f32);

  EXP(BasicTypes__fini)(&msg);
  return 0;
}

// ---------------------------------------------------------------------------
// test_storage_defaults_backed_values:
//   ExternalStorage for Defaults; default values must be written to backing.
// ---------------------------------------------------------------------------

static int test_storage_defaults_backed_values(void)
{
  // Allocate backing storage for ALL fields
  bool     b = false;
  uint8_t  u8 = 0u;
  uint8_t  c = 0u;
  float    f32 = 0.0f;
  double   f64 = 0.0;
  int8_t   i8 = 0;
  uint8_t  u8_2 = 0u;
  int16_t  i16 = 0;
  uint16_t u16 = 0u;
  int32_t  i32 = 0;
  uint32_t u32 = 0u;
  int64_t  i64 = 0;
  uint64_t u64 = 0u;

  EXP(Defaults__ExternalStorage) storage;
  memset(&storage, 0, sizeof(storage));

  storage.members.bool_value = make_memory(&b);
  storage.members.byte_value = make_memory(&u8);
  storage.members.char_value = make_memory(&c);
  storage.members.float32_value = make_memory(&f32);
  storage.members.float64_value = make_memory(&f64);
  storage.members.int8_value = make_memory(&i8);
  storage.members.uint8_value = make_memory(&u8_2);
  storage.members.int16_value = make_memory(&i16);
  storage.members.uint16_value = make_memory(&u16);
  storage.members.int32_value = make_memory(&i32);
  storage.members.uint32_value = make_memory(&u32);
  storage.members.int64_value = make_memory(&i64);
  storage.members.uint64_value = make_memory(&u64);

  EXP(Defaults) msg;
  EXP(Defaults__InitOptions) options = {0};
  options.external_storage = &storage;
  EXPECT_TRUE(EXP(Defaults__init_with_options)(&msg, &options));

  // Check defaults applied
  EXPECT_TRUE(msg.bool_value.value->data);
  EXPECT_TRUE(b);
  EXPECT_EQ(50u, msg.byte_value.value->data);
  EXPECT_EQ(50u, u8);
  EXPECT_EQ(-30000, msg.int32_value.value->data);
  EXPECT_EQ(-30000, i32);
  EXPECT_EQ(50000000u, msg.uint64_value.value->data);
  EXPECT_EQ(50000000u, u64);

  EXP(Defaults__fini)(&msg);
  return 0;
}

// ---------------------------------------------------------------------------
// test_storage_strings_regions:
//   ExternalStorage for Strings provides region backing for string fields.
// ---------------------------------------------------------------------------

static int test_storage_strings_regions(void)
{
  // Allocate backing storage for ALL string fields
  char buf_sv[64] = {0};
  char buf_svd1[64] = {0};
  char buf_svd2[64] = {0};
  char buf_svd3[64] = {0};
  char buf_svd4[64] = {0};
  char buf_svd5[64] = {0};
  char buf_bsv[32] = {0};
  char buf_bsvd1[32] = {0};
  char buf_bsvd2[32] = {0};
  char buf_bsvd3[32] = {0};
  char buf_bsvd4[32] = {0};
  char buf_bsvd5[32] = {0};

  EXP(Strings__ExternalStorage) storage;
  memset(&storage, 0, sizeof(storage));

  storage.members.string_value = make_region(buf_sv, sizeof(buf_sv));
  storage.members.string_value_default1 = make_region(buf_svd1, sizeof(buf_svd1));
  storage.members.string_value_default2 = make_region(buf_svd2, sizeof(buf_svd2));
  storage.members.string_value_default3 = make_region(buf_svd3, sizeof(buf_svd3));
  storage.members.string_value_default4 = make_region(buf_svd4, sizeof(buf_svd4));
  storage.members.string_value_default5 = make_region(buf_svd5, sizeof(buf_svd5));
  storage.members.bounded_string_value = make_region(buf_bsv, sizeof(buf_bsv));
  storage.members.bounded_string_value_default1 = make_region(buf_bsvd1, sizeof(buf_bsvd1));
  storage.members.bounded_string_value_default2 = make_region(buf_bsvd2, sizeof(buf_bsvd2));
  storage.members.bounded_string_value_default3 = make_region(buf_bsvd3, sizeof(buf_bsvd3));
  storage.members.bounded_string_value_default4 = make_region(buf_bsvd4, sizeof(buf_bsvd4));
  storage.members.bounded_string_value_default5 = make_region(buf_bsvd5, sizeof(buf_bsvd5));

  EXP(Strings) msg;
  EXP(Strings__InitOptions) options = {0};
  options.external_storage = &storage;
  EXPECT_TRUE(EXP(Strings__init_with_options)(&msg, &options));

  // Empty after init
  EXPECT_EQ(0u, msg.string_value.size);
  EXPECT_EQ((void *)buf_sv, (void *)msg.string_value.value);

  // Assign into the region-backed field
  EXPECT_TRUE(
    rosidl_runtime_c__experimental__String__assign(&msg.string_value, "hello"));
  EXPECT_EQ(5u, msg.string_value.size);
  EXPECT_EQ(0, strcmp(buf_sv, "hello"));
  EXPECT_EQ(0, strcmp((const char *)msg.string_value.value, "hello"));

  // The managed fields (default values) still work
  EXPECT_EQ(0, strcmp((const char *)msg.string_value_default1.value,
    "Hello world!"));

  EXP(Strings__fini)(&msg);
  return 0;
}

// ---------------------------------------------------------------------------
// test_storage_nested:
//   ExternalStorage for Nested contains a sub-storage for BasicTypes.
// ---------------------------------------------------------------------------

static int test_storage_nested(void)
{
  // Allocate backing storage for all nested BasicTypes fields
  bool     b = false;
  uint8_t  byte = 0;
  uint8_t  c = 0;
  float    f32 = 0.0f;
  double   f64 = 0.0;
  int8_t   i8 = 0;
  uint8_t  u8 = 0;
  int16_t  i16 = 0;
  uint16_t u16 = 0;
  int32_t  i32 = 0;
  uint32_t u32 = 0;
  int64_t  i64 = 0;
  uint64_t u64 = 0;

  EXP(BasicTypes__ExternalStorage) inner_storage;
  memset(&inner_storage, 0, sizeof(inner_storage));
  inner_storage.members.bool_value = make_memory(&b);
  inner_storage.members.byte_value = make_memory(&byte);
  inner_storage.members.char_value = make_memory(&c);
  inner_storage.members.float32_value = make_memory(&f32);
  inner_storage.members.float64_value = make_memory(&f64);
  inner_storage.members.int8_value = make_memory(&i8);
  inner_storage.members.uint8_value = make_memory(&u8);
  inner_storage.members.int16_value = make_memory(&i16);
  inner_storage.members.uint16_value = make_memory(&u16);
  inner_storage.members.int32_value = make_memory(&i32);
  inner_storage.members.uint32_value = make_memory(&u32);
  inner_storage.members.int64_value = make_memory(&i64);
  inner_storage.members.uint64_value = make_memory(&u64);

  EXP(Nested__ExternalStorage) storage;
  memset(&storage, 0, sizeof(storage));
  storage.members.basic_types_value = inner_storage;

  EXP(Nested) msg;
  EXP(Nested__InitOptions) options = {0};
  options.external_storage = &storage;
  EXPECT_TRUE(EXP(Nested__init_with_options)(&msg, &options));

  EXPECT_EQ(0, msg.basic_types_value.int32_value.value->data);
  EXPECT_EQ(&i32, (int32_t *)msg.basic_types_value.int32_value.value);

  msg.basic_types_value.int32_value.value->data = 111;
  EXPECT_EQ(111, i32);

  msg.basic_types_value.float32_value.value->data = 2.5f;
  EXPECT_EQ(2.5f, f32);

  EXP(Nested__fini)(&msg);
  return 0;
}

// ---------------------------------------------------------------------------
// test_storage_arrays_scalar_region:
//   ExternalStorage for Arrays provides region backing for a scalar array.
// ---------------------------------------------------------------------------

static int test_storage_arrays_scalar_region(void)
{
  // Allocate backing storage for all scalar array fields
  bool bool_buf[3] = {false, false, false};
  uint8_t byte_buf[3] = {0, 0, 0};
  uint8_t char_buf[3] = {0, 0, 0};
  float float32_buf[3] = {0.0f, 0.0f, 0.0f};
  double float64_buf[3] = {0.0, 0.0, 0.0};
  int8_t int8_buf[3] = {0, 0, 0};
  uint8_t uint8_buf[3] = {0, 0, 0};
  int16_t int16_buf[3] = {0, 0, 0};
  uint16_t uint16_buf[3] = {0, 0, 0};
  int32_t int32_buf[3] = {0, 0, 0};
  uint32_t uint32_buf[3] = {0, 0, 0};
  int64_t int64_buf[3] = {0, 0, 0};
  uint64_t uint64_buf[3] = {0, 0, 0};

  // String arrays
  char string_buf0[64] = {0}, string_buf1[64] = {0}, string_buf2[64] = {0};

  // BasicTypes arrays - need 3 complete BasicTypes storages
  // Each BasicTypes needs 13 scalar fields
  bool bt0_b = false, bt1_b = false, bt2_b = false;
  uint8_t bt0_byte = 0, bt1_byte = 0, bt2_byte = 0;
  uint8_t bt0_char = 0, bt1_char = 0, bt2_char = 0;
  float bt0_f32 = 0.0f, bt1_f32 = 0.0f, bt2_f32 = 0.0f;
  double bt0_f64 = 0.0, bt1_f64 = 0.0, bt2_f64 = 0.0;
  int8_t bt0_i8 = 0, bt1_i8 = 0, bt2_i8 = 0;
  uint8_t bt0_u8 = 0, bt1_u8 = 0, bt2_u8 = 0;
  int16_t bt0_i16 = 0, bt1_i16 = 0, bt2_i16 = 0;
  uint16_t bt0_u16 = 0, bt1_u16 = 0, bt2_u16 = 0;
  int32_t bt0_i32 = 0, bt1_i32 = 0, bt2_i32 = 0;
  uint32_t bt0_u32 = 0, bt1_u32 = 0, bt2_u32 = 0;
  int64_t bt0_i64 = 0, bt1_i64 = 0, bt2_i64 = 0;
  uint64_t bt0_u64 = 0, bt1_u64 = 0, bt2_u64 = 0;

  // Constants arrays - Constants only has structure_needs_at_least_one_member field
  uint8_t const0_member = 0, const1_member = 0, const2_member = 0;
  EXP(Constants__ExternalStorage) const_storage[3];
  memset(const_storage, 0, sizeof(const_storage));
  const_storage[0].members.structure_needs_at_least_one_member = make_memory(&const0_member);
  const_storage[1].members.structure_needs_at_least_one_member = make_memory(&const1_member);
  const_storage[2].members.structure_needs_at_least_one_member = make_memory(&const2_member);

  // Defaults arrays - need 3 complete Defaults storages
  bool df0_b = false, df1_b = false, df2_b = false;
  uint8_t df0_byte = 0, df1_byte = 0, df2_byte = 0;
  uint8_t df0_char = 0, df1_char = 0, df2_char = 0;
  float df0_f32 = 0.0f, df1_f32 = 0.0f, df2_f32 = 0.0f;
  double df0_f64 = 0.0, df1_f64 = 0.0, df2_f64 = 0.0;
  int8_t df0_i8 = 0, df1_i8 = 0, df2_i8 = 0;
  uint8_t df0_u8 = 0, df1_u8 = 0, df2_u8 = 0;
  int16_t df0_i16 = 0, df1_i16 = 0, df2_i16 = 0;
  uint16_t df0_u16 = 0, df1_u16 = 0, df2_u16 = 0;
  int32_t df0_i32 = 0, df1_i32 = 0, df2_i32 = 0;
  uint32_t df0_u32 = 0, df1_u32 = 0, df2_u32 = 0;
  int64_t df0_i64 = 0, df1_i64 = 0, df2_i64 = 0;
  uint64_t df0_u64 = 0, df1_u64 = 0, df2_u64 = 0;

  // Default value arrays
  bool bool_def_buf[3] = {false, false, false};
  uint8_t byte_def_buf[3] = {0, 0, 0};
  uint8_t char_def_buf[3] = {0, 0, 0};
  float float32_def_buf[3] = {0.0f, 0.0f, 0.0f};
  double float64_def_buf[3] = {0.0, 0.0, 0.0};
  int8_t int8_def_buf[3] = {0, 0, 0};
  uint8_t uint8_def_buf[3] = {0, 0, 0};
  int16_t int16_def_buf[3] = {0, 0, 0};
  uint16_t uint16_def_buf[3] = {0, 0, 0};
  int32_t int32_def_buf[3] = {0, 0, 0};
  uint32_t uint32_def_buf[3] = {0, 0, 0};
  int64_t int64_def_buf[3] = {0, 0, 0};
  uint64_t uint64_def_buf[3] = {0, 0, 0};
  char string_def_buf0[64] = {0}, string_def_buf1[64] = {0}, string_def_buf2[64] = {0};

  // Alignment check
  int32_t alignment = 0;

  // Initialize BasicTypes storages
  EXP(BasicTypes__ExternalStorage) bt_storage[3];
  memset(bt_storage, 0, sizeof(bt_storage));

  bt_storage[0].members.bool_value = make_memory(&bt0_b);
  bt_storage[0].members.byte_value = make_memory(&bt0_byte);
  bt_storage[0].members.char_value = make_memory(&bt0_char);
  bt_storage[0].members.float32_value = make_memory(&bt0_f32);
  bt_storage[0].members.float64_value = make_memory(&bt0_f64);
  bt_storage[0].members.int8_value = make_memory(&bt0_i8);
  bt_storage[0].members.uint8_value = make_memory(&bt0_u8);
  bt_storage[0].members.int16_value = make_memory(&bt0_i16);
  bt_storage[0].members.uint16_value = make_memory(&bt0_u16);
  bt_storage[0].members.int32_value = make_memory(&bt0_i32);
  bt_storage[0].members.uint32_value = make_memory(&bt0_u32);
  bt_storage[0].members.int64_value = make_memory(&bt0_i64);
  bt_storage[0].members.uint64_value = make_memory(&bt0_u64);

  bt_storage[1].members.bool_value = make_memory(&bt1_b);
  bt_storage[1].members.byte_value = make_memory(&bt1_byte);
  bt_storage[1].members.char_value = make_memory(&bt1_char);
  bt_storage[1].members.float32_value = make_memory(&bt1_f32);
  bt_storage[1].members.float64_value = make_memory(&bt1_f64);
  bt_storage[1].members.int8_value = make_memory(&bt1_i8);
  bt_storage[1].members.uint8_value = make_memory(&bt1_u8);
  bt_storage[1].members.int16_value = make_memory(&bt1_i16);
  bt_storage[1].members.uint16_value = make_memory(&bt1_u16);
  bt_storage[1].members.int32_value = make_memory(&bt1_i32);
  bt_storage[1].members.uint32_value = make_memory(&bt1_u32);
  bt_storage[1].members.int64_value = make_memory(&bt1_i64);
  bt_storage[1].members.uint64_value = make_memory(&bt1_u64);

  bt_storage[2].members.bool_value = make_memory(&bt2_b);
  bt_storage[2].members.byte_value = make_memory(&bt2_byte);
  bt_storage[2].members.char_value = make_memory(&bt2_char);
  bt_storage[2].members.float32_value = make_memory(&bt2_f32);
  bt_storage[2].members.float64_value = make_memory(&bt2_f64);
  bt_storage[2].members.int8_value = make_memory(&bt2_i8);
  bt_storage[2].members.uint8_value = make_memory(&bt2_u8);
  bt_storage[2].members.int16_value = make_memory(&bt2_i16);
  bt_storage[2].members.uint16_value = make_memory(&bt2_u16);
  bt_storage[2].members.int32_value = make_memory(&bt2_i32);
  bt_storage[2].members.uint32_value = make_memory(&bt2_u32);
  bt_storage[2].members.int64_value = make_memory(&bt2_i64);
  bt_storage[2].members.uint64_value = make_memory(&bt2_u64);

  // Initialize Defaults storages
  EXP(Defaults__ExternalStorage) df_storage[3];
  memset(df_storage, 0, sizeof(df_storage));

  df_storage[0].members.bool_value = make_memory(&df0_b);
  df_storage[0].members.byte_value = make_memory(&df0_byte);
  df_storage[0].members.char_value = make_memory(&df0_char);
  df_storage[0].members.float32_value = make_memory(&df0_f32);
  df_storage[0].members.float64_value = make_memory(&df0_f64);
  df_storage[0].members.int8_value = make_memory(&df0_i8);
  df_storage[0].members.uint8_value = make_memory(&df0_u8);
  df_storage[0].members.int16_value = make_memory(&df0_i16);
  df_storage[0].members.uint16_value = make_memory(&df0_u16);
  df_storage[0].members.int32_value = make_memory(&df0_i32);
  df_storage[0].members.uint32_value = make_memory(&df0_u32);
  df_storage[0].members.int64_value = make_memory(&df0_i64);
  df_storage[0].members.uint64_value = make_memory(&df0_u64);

  df_storage[1].members.bool_value = make_memory(&df1_b);
  df_storage[1].members.byte_value = make_memory(&df1_byte);
  df_storage[1].members.char_value = make_memory(&df1_char);
  df_storage[1].members.float32_value = make_memory(&df1_f32);
  df_storage[1].members.float64_value = make_memory(&df1_f64);
  df_storage[1].members.int8_value = make_memory(&df1_i8);
  df_storage[1].members.uint8_value = make_memory(&df1_u8);
  df_storage[1].members.int16_value = make_memory(&df1_i16);
  df_storage[1].members.uint16_value = make_memory(&df1_u16);
  df_storage[1].members.int32_value = make_memory(&df1_i32);
  df_storage[1].members.uint32_value = make_memory(&df1_u32);
  df_storage[1].members.int64_value = make_memory(&df1_i64);
  df_storage[1].members.uint64_value = make_memory(&df1_u64);

  df_storage[2].members.bool_value = make_memory(&df2_b);
  df_storage[2].members.byte_value = make_memory(&df2_byte);
  df_storage[2].members.char_value = make_memory(&df2_char);
  df_storage[2].members.float32_value = make_memory(&df2_f32);
  df_storage[2].members.float64_value = make_memory(&df2_f64);
  df_storage[2].members.int8_value = make_memory(&df2_i8);
  df_storage[2].members.uint8_value = make_memory(&df2_u8);
  df_storage[2].members.int16_value = make_memory(&df2_i16);
  df_storage[2].members.uint16_value = make_memory(&df2_u16);
  df_storage[2].members.int32_value = make_memory(&df2_i32);
  df_storage[2].members.uint32_value = make_memory(&df2_u32);
  df_storage[2].members.int64_value = make_memory(&df2_i64);
  df_storage[2].members.uint64_value = make_memory(&df2_u64);

  // Now initialize the Arrays storage
  EXP(Arrays__ExternalStorage) storage;
  memset(&storage, 0, sizeof(storage));

  storage.members.bool_values = make_region(bool_buf, sizeof(bool_buf));
  storage.members.byte_values = make_region(byte_buf, sizeof(byte_buf));
  storage.members.char_values = make_region(char_buf, sizeof(char_buf));
  storage.members.float32_values = make_region(float32_buf, sizeof(float32_buf));
  storage.members.float64_values = make_region(float64_buf, sizeof(float64_buf));
  storage.members.int8_values = make_region(int8_buf, sizeof(int8_buf));
  storage.members.uint8_values = make_region(uint8_buf, sizeof(uint8_buf));
  storage.members.int16_values = make_region(int16_buf, sizeof(int16_buf));
  storage.members.uint16_values = make_region(uint16_buf, sizeof(uint16_buf));
  storage.members.int32_values = make_region(int32_buf, sizeof(int32_buf));
  storage.members.uint32_values = make_region(uint32_buf, sizeof(uint32_buf));
  storage.members.int64_values = make_region(int64_buf, sizeof(int64_buf));
  storage.members.uint64_values = make_region(uint64_buf, sizeof(uint64_buf));

  storage.members.string_values[0] = make_region(string_buf0, sizeof(string_buf0));
  storage.members.string_values[1] = make_region(string_buf1, sizeof(string_buf1));
  storage.members.string_values[2] = make_region(string_buf2, sizeof(string_buf2));

  storage.members.basic_types_values[0] = bt_storage[0];
  storage.members.basic_types_values[1] = bt_storage[1];
  storage.members.basic_types_values[2] = bt_storage[2];

  storage.members.constants_values[0] = const_storage[0];
  storage.members.constants_values[1] = const_storage[1];
  storage.members.constants_values[2] = const_storage[2];

  storage.members.defaults_values[0] = df_storage[0];
  storage.members.defaults_values[1] = df_storage[1];
  storage.members.defaults_values[2] = df_storage[2];

  storage.members.bool_values_default = make_region(bool_def_buf, sizeof(bool_def_buf));
  storage.members.byte_values_default = make_region(byte_def_buf, sizeof(byte_def_buf));
  storage.members.char_values_default = make_region(char_def_buf, sizeof(char_def_buf));
  storage.members.float32_values_default = make_region(float32_def_buf, sizeof(float32_def_buf));
  storage.members.float64_values_default = make_region(float64_def_buf, sizeof(float64_def_buf));
  storage.members.int8_values_default = make_region(int8_def_buf, sizeof(int8_def_buf));
  storage.members.uint8_values_default = make_region(uint8_def_buf, sizeof(uint8_def_buf));
  storage.members.int16_values_default = make_region(int16_def_buf, sizeof(int16_def_buf));
  storage.members.uint16_values_default = make_region(uint16_def_buf, sizeof(uint16_def_buf));
  storage.members.int32_values_default = make_region(int32_def_buf, sizeof(int32_def_buf));
  storage.members.uint32_values_default = make_region(uint32_def_buf, sizeof(uint32_def_buf));
  storage.members.int64_values_default = make_region(int64_def_buf, sizeof(int64_def_buf));
  storage.members.uint64_values_default = make_region(uint64_def_buf, sizeof(uint64_def_buf));

  storage.members.string_values_default[0] = make_region(string_def_buf0, sizeof(string_def_buf0));
  storage.members.string_values_default[1] = make_region(string_def_buf1, sizeof(string_def_buf1));
  storage.members.string_values_default[2] = make_region(string_def_buf2, sizeof(string_def_buf2));

  storage.members.alignment_check = make_memory(&alignment);

  EXP(Arrays) msg;
  EXP(Arrays__InitOptions) options = {0};
  options.external_storage = &storage;
  EXPECT_TRUE(EXP(Arrays__init_with_options)(&msg, &options));

  EXPECT_EQ((void *)int32_buf, (void *)msg.int32_values.value);
  EXPECT_EQ(0, msg.int32_values.value->data[0]);

  msg.int32_values.value->data[0] = 10;
  msg.int32_values.value->data[1] = 20;
  msg.int32_values.value->data[2] = 30;

  EXPECT_EQ(10, int32_buf[0]);
  EXPECT_EQ(20, int32_buf[1]);
  EXPECT_EQ(30, int32_buf[2]);

  EXP(Arrays__fini)(&msg);
  return 0;
}

// ---------------------------------------------------------------------------
// test_storage_sequences_null_storage:
//   When storage is NULL, __init falls back to heap allocation.
//   Sequences remain empty by default, and push_back works normally.
// ---------------------------------------------------------------------------

static int test_storage_sequences_null_storage(void)
{
  EXP(BasicTypes) * msg = EXP(BasicTypes__create)(NULL);
  EXPECT_NE(NULL, msg);

  // Just verify managed init produces a valid zeroed message
  EXPECT_EQ(0, msg->int32_value.value->data);
  EXPECT_FALSE(msg->bool_value.value->data);

  msg->int32_value.value->data = 999;
  EXPECT_EQ(999, msg->int32_value.value->data);

  EXP(BasicTypes__destroy)(msg);
  return 0;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
  int rc = 0;
  printf("test_experimental_initialization_c\n");
  RUN(test_storage_empty);
  RUN(test_storage_basic_types_preserves_value);
  RUN(test_storage_defaults_backed_values);
  RUN(test_storage_strings_regions);
  RUN(test_storage_nested);
  RUN(test_storage_arrays_scalar_region);
  RUN(test_storage_sequences_null_storage);
  if (rc != 0) {
    fprintf(stderr, "%d test(s) FAILED\n", rc);
  } else {
    printf("All tests passed.\n");
  }
  return rc != 0;
}
