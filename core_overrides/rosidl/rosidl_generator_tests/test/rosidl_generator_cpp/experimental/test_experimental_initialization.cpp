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

#include <array>
#include <cstddef>
#include <cstring>
#include <memory_resource>

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

struct Arena
{
  alignas(std::max_align_t) std::array<std::byte, 65536> buf {};
  std::pmr::monotonic_buffer_resource res {buf.data(), buf.size()};
  std::pmr::memory_resource * get() {return &res;}
};

TEST(test_experimental_initialization_default, empty) {
  experimental::Empty a, b;
  EXPECT_EQ(a, b);
}

TEST(test_experimental_initialization_default, basic_types_all_zero) {
  experimental::BasicTypes msg;
  EXPECT_FALSE(msg.bool_value);
  EXPECT_EQ(0, msg.int32_value);
  EXPECT_EQ(0.0, msg.float64_value);
  EXPECT_EQ(0u, msg.uint64_value);
}

TEST(test_experimental_initialization_default, defaults_non_zero) {
  experimental::Defaults msg;
  EXPECT_TRUE(msg.bool_value);
  EXPECT_EQ(50, msg.byte_value);
  EXPECT_EQ(-30000, msg.int32_value);
}

TEST(test_experimental_initialization_default, constants_empty_struct) {
  experimental::Constants a, b;
  EXPECT_EQ(a, b);
}

TEST(test_experimental_initialization_default, strings_fields_empty) {
  experimental::Strings msg;
  EXPECT_TRUE(msg.string_value.empty());
  EXPECT_TRUE(msg.bounded_string_value.empty());
}

TEST(test_experimental_initialization_default, wstrings_fields_empty) {
  experimental::WStrings msg;
  EXPECT_TRUE(msg.wstring_value.empty());
  EXPECT_EQ(0u, msg.unbounded_sequence_of_wstrings.size());
}

TEST(test_experimental_initialization_default, nested_sub_message_zero) {
  experimental::Nested msg;
  EXPECT_EQ(0, msg.basic_types_value.int32_value);
}

TEST(test_experimental_initialization_default, arrays_scalar_zero_string_empty) {
  experimental::Arrays msg;
  EXPECT_EQ(0, msg.int32_values[0]);
  EXPECT_TRUE(msg.string_values[0].empty());
}

TEST(test_experimental_initialization_default, bounded_sequences_all_empty) {
  experimental::BoundedSequences msg;
  EXPECT_EQ(0u, msg.int32_values.size());
  EXPECT_EQ(0u, msg.string_values.size());
  EXPECT_EQ(0u, msg.basic_types_values.size());
}

TEST(test_experimental_initialization_default, bounded_plain_sequences_all_empty) {
  experimental::BoundedPlainSequences msg;
  EXPECT_EQ(0u, msg.int32_values.size());
  EXPECT_EQ(0u, msg.basic_types_values.size());
}

TEST(test_experimental_initialization_default, unbounded_sequences_all_empty) {
  experimental::UnboundedSequences msg;
  EXPECT_EQ(0u, msg.int32_values.size());
  EXPECT_EQ(0u, msg.string_values.size());
  EXPECT_EQ(0u, msg.basic_types_values.size());
}

TEST(test_experimental_initialization_default, multi_nested_sequences_empty) {
  experimental::MultiNested msg;
  EXPECT_EQ(0u, msg.bounded_sequence_of_arrays.size());
  EXPECT_EQ(0u, msg.unbounded_sequence_of_bounded_sequences.size());
}

TEST(test_experimental_initialization_pmr, basic_types_no_pmr_members_compiles) {
  // BasicTypes has no PMR members; constructor accepts mem_res without error.
  Arena arena;
  experimental::BasicTypes msg {arena.get()};
  EXPECT_FALSE(msg.bool_value);
  EXPECT_EQ(0, msg.int32_value);
}

TEST(test_experimental_initialization_pmr, nullptr_falls_back_to_default_resource) {
  experimental::BasicTypes msg {static_cast<std::pmr::memory_resource *>(nullptr)};
  EXPECT_EQ(0, msg.int32_value);
}

TEST(test_experimental_initialization_pmr, strings_pmr_string_is_usable) {
  Arena arena;
  experimental::Strings msg {arena.get()};
  // After PMR construction the string must be assignable and readable.
  msg.string_value = "pmr_test";
  EXPECT_EQ("pmr_test", msg.string_value);
  msg.bounded_string_value = "bounded_pmr";
  EXPECT_EQ("bounded_pmr", msg.bounded_string_value);
}

TEST(test_experimental_initialization_pmr, wstrings_pmr_wstring_is_usable) {
  Arena arena;
  experimental::WStrings msg {arena.get()};
  msg.wstring_value = u"\u30cf\u30ed\u30fc";
  EXPECT_EQ(u"\u30cf\u30ed\u30fc", msg.wstring_value);
  msg.unbounded_sequence_of_wstrings.push_back(rosidl_runtime_cpp::WString {u"a"});
  EXPECT_EQ(1u, msg.unbounded_sequence_of_wstrings.size());
}

TEST(test_experimental_initialization_pmr, nested_pmr_propagates_to_sub_message) {
  // Nested has no PMR members itself, but its sub-message might; verify it compiles.
  Arena arena;
  experimental::Nested msg {arena.get()};
  EXPECT_EQ(0, msg.basic_types_value.int32_value);
}

TEST(test_experimental_initialization_pmr, arrays_pmr_string_array_is_usable) {
  Arena arena;
  experimental::Arrays msg {arena.get()};
  msg.string_values[0] = "first";
  msg.string_values[1] = "second";
  EXPECT_EQ("first", msg.string_values[0]);
  EXPECT_EQ("second", msg.string_values[1]);
}

TEST(test_experimental_initialization_pmr, bounded_sequences_pmr_push_back_works) {
  Arena arena;
  experimental::BoundedSequences msg {arena.get()};
  msg.string_values.push_back(rosidl_runtime_cpp::String {"pmr"});
  ASSERT_EQ(1u, msg.string_values.size());
  EXPECT_EQ("pmr", msg.string_values[0]);
}

TEST(test_experimental_initialization_pmr, unbounded_sequences_pmr_push_back_works) {
  Arena arena;
  experimental::UnboundedSequences msg {arena.get()};
  msg.string_values.push_back(rosidl_runtime_cpp::String {"pmr_unbounded"});
  ASSERT_EQ(1u, msg.string_values.size());
  EXPECT_EQ("pmr_unbounded", msg.string_values[0]);
}

TEST(test_experimental_initialization_pmr, multi_nested_pmr_compiles_and_is_usable) {
  Arena arena;
  experimental::MultiNested msg {arena.get()};
  msg.array_of_arrays[0].string_values[0] = "deep";
  EXPECT_EQ("deep", msg.array_of_arrays[0].string_values[0]);
}

TEST(test_experimental_initialization_storage, basic_types_storage_equality) {
  experimental::BasicTypes::ExternalStorage s1, s2;
  EXPECT_EQ(s1, s2);
  EXPECT_FALSE(s1 != s2);
}

TEST(test_experimental_initialization_storage, defaults_storage_equality) {
  experimental::Defaults::ExternalStorage s1, s2;
  EXPECT_EQ(s1, s2);
}

TEST(test_experimental_initialization_storage, empty_storage_equality) {
  experimental::Empty::ExternalStorage s1, s2;
  EXPECT_EQ(s1, s2);
}

TEST(test_experimental_initialization_storage, strings_storage_equality) {
  experimental::Strings::ExternalStorage s1, s2;
  EXPECT_EQ(s1, s2);
}

TEST(test_experimental_initialization_storage, arrays_storage_equality) {
  experimental::Arrays::ExternalStorage s1, s2;
  EXPECT_EQ(s1, s2);
}

TEST(test_experimental_initialization_storage, nested_storage_equality) {
  experimental::Nested::ExternalStorage s1, s2;
  EXPECT_EQ(s1, s2);
}

TEST(test_experimental_initialization_storage, bounded_sequences_storage_equality) {
  experimental::BoundedSequences::ExternalStorage s1, s2;
  EXPECT_EQ(s1, s2);
}

TEST(test_experimental_initialization_storage, unbounded_sequences_storage_equality) {
  experimental::UnboundedSequences::ExternalStorage s1, s2;
  EXPECT_EQ(s1, s2);
}

TEST(test_experimental_initialization_storage, multi_nested_storage_equality) {
  experimental::MultiNested::ExternalStorage s1, s2;
  EXPECT_EQ(s1, s2);
}

TEST(test_experimental_initialization_storage_ctor, empty_storage_ctor_compiles) {
  experimental::Empty::ExternalStorage s;
  experimental::Empty msg {s, rosidl_runtime_cpp::MessageInitialization::SKIP};
  (void)msg;
}

TEST(test_experimental_initialization_storage_ctor, constants_storage_ctor_compiles) {
  experimental::Constants::ExternalStorage s;
  experimental::Constants msg {s, rosidl_runtime_cpp::MessageInitialization::SKIP};
  (void)msg;
}

TEST(test_experimental_initialization_storage_ctor, basic_types_storage_ctor_skip_preserves_value) {
  // SKIP leaves the backed region untouched — whatever was there before is
  // still readable through the message interface.
  alignas(int32_t) std::byte int32_buf[sizeof(int32_t)];
  *reinterpret_cast<int32_t *>(int32_buf) = 1234;
  experimental::BasicTypes::ExternalStorage s;
  s.members.int32_value.assign(int32_buf);
  experimental::BasicTypes msg {s, rosidl_runtime_cpp::MessageInitialization::SKIP};
  EXPECT_EQ(1234, msg.int32_value);
}

TEST(test_experimental_initialization_storage_ctor,
  basic_types_storage_ctor_all_with_full_backing) {
  // ALL must zero every field. Back all 13 scalars so reset() has valid targets.
  bool bv = true; uint8_t byv = 255; uint8_t chv = 99;
  float f32v = 1.0f; double f64v = 1.0;
  int8_t i8v = -1; uint8_t u8v = 255;
  int16_t i16v = -1; uint16_t u16v = 65535;
  int32_t i32v = -1; uint32_t u32v = 4294967295u;
  int64_t i64v = -1; uint64_t u64v = 18446744073709551615ull;
  experimental::BasicTypes::ExternalStorage s;
  s.members.bool_value.assign(&bv);
  s.members.byte_value.assign(&byv);
  s.members.char_value.assign(&chv);
  s.members.float32_value.assign(&f32v);
  s.members.float64_value.assign(&f64v);
  s.members.int8_value.assign(&i8v);
  s.members.uint8_value.assign(&u8v);
  s.members.int16_value.assign(&i16v);
  s.members.uint16_value.assign(&u16v);
  s.members.int32_value.assign(&i32v);
  s.members.uint32_value.assign(&u32v);
  s.members.int64_value.assign(&i64v);
  s.members.uint64_value.assign(&u64v);
  experimental::BasicTypes msg {s, rosidl_runtime_cpp::MessageInitialization::ALL};
  EXPECT_FALSE(msg.bool_value);
  EXPECT_EQ(0, msg.int32_value);
  EXPECT_EQ(0.0, msg.float64_value);
  EXPECT_EQ(0u, msg.uint64_value);
}

TEST(test_experimental_initialization_storage_ctor, defaults_storage_ctor_all_with_full_backing) {
  // ALL must apply field defaults. Back all 13 scalars of Defaults.
  bool bv = false; uint8_t byv = 0; uint8_t chv = 0;
  float f32v = 0.0f; double f64v = 0.0;
  int8_t i8v = 0; uint8_t u8v = 0;
  int16_t i16v = 0; uint16_t u16v = 0;
  int32_t i32v = 0; uint32_t u32v = 0;
  int64_t i64v = 0; uint64_t u64v = 0;
  experimental::Defaults::ExternalStorage s;
  s.members.bool_value.assign(&bv);
  s.members.byte_value.assign(&byv);
  s.members.char_value.assign(&chv);
  s.members.float32_value.assign(&f32v);
  s.members.float64_value.assign(&f64v);
  s.members.int8_value.assign(&i8v);
  s.members.uint8_value.assign(&u8v);
  s.members.int16_value.assign(&i16v);
  s.members.uint16_value.assign(&u16v);
  s.members.int32_value.assign(&i32v);
  s.members.uint32_value.assign(&u32v);
  s.members.int64_value.assign(&i64v);
  s.members.uint64_value.assign(&u64v);
  experimental::Defaults msg {s, rosidl_runtime_cpp::MessageInitialization::ALL};
  EXPECT_TRUE(msg.bool_value);
  EXPECT_EQ(50u, msg.byte_value);
  EXPECT_EQ(-30000, msg.int32_value);
  EXPECT_EQ(50000000u, msg.uint64_value);
}

TEST(test_experimental_initialization_storage_ctor, strings_storage_ctor_all_with_full_backing) {
  // Strings has 5 unbounded + 5 bounded string fields (constants don't need backing).
  // ALL zeroes the non-defaulted ones (empty) and fills the defaulted ones.
  char s0[64] {}, s1[64] {}, s2[64] {}, s3[64] {}, s4[64] {}, s5[64] {};
  char b0[23] {}, b1[23] {}, b2[23] {}, b3[23] {}, b4[23] {}, b5[23] {};
  experimental::Strings::ExternalStorage s;
  s.members.string_value.assign(s0, sizeof(s0));
  s.members.string_value_default1.assign(s1, sizeof(s1));
  s.members.string_value_default2.assign(s2, sizeof(s2));
  s.members.string_value_default3.assign(s3, sizeof(s3));
  s.members.string_value_default4.assign(s4, sizeof(s4));
  s.members.string_value_default5.assign(s5, sizeof(s5));
  s.members.bounded_string_value.assign(b0, sizeof(b0));
  s.members.bounded_string_value_default1.assign(b1, sizeof(b1));
  s.members.bounded_string_value_default2.assign(b2, sizeof(b2));
  s.members.bounded_string_value_default3.assign(b3, sizeof(b3));
  s.members.bounded_string_value_default4.assign(b4, sizeof(b4));
  s.members.bounded_string_value_default5.assign(b5, sizeof(b5));
  experimental::Strings msg {s, rosidl_runtime_cpp::MessageInitialization::ALL};
  EXPECT_TRUE(msg.string_value.empty());
  EXPECT_EQ("Hello world!", msg.string_value_default1);
  EXPECT_TRUE(msg.bounded_string_value.empty());
  EXPECT_EQ("Hello world!", msg.bounded_string_value_default1);
  msg.string_value = "hello";
  EXPECT_EQ("hello", msg.string_value);
}

TEST(test_experimental_initialization_storage_ctor, nested_storage_ctor_all_with_full_backing) {
  // Nested wraps a single BasicTypes sub-message — back all 13 of its scalars.
  bool bv = true; uint8_t byv = 1; uint8_t chv = 1;
  float f32v = 1.0f; double f64v = 1.0;
  int8_t i8v = -1; uint8_t u8v = 1;
  int16_t i16v = -1; uint16_t u16v = 1;
  int32_t i32v = -1; uint32_t u32v = 1;
  int64_t i64v = -1; uint64_t u64v = 1;
  experimental::BasicTypes::ExternalStorage sub;
  sub.members.bool_value.assign(&bv);
  sub.members.byte_value.assign(&byv);
  sub.members.char_value.assign(&chv);
  sub.members.float32_value.assign(&f32v);
  sub.members.float64_value.assign(&f64v);
  sub.members.int8_value.assign(&i8v);
  sub.members.uint8_value.assign(&u8v);
  sub.members.int16_value.assign(&i16v);
  sub.members.uint16_value.assign(&u16v);
  sub.members.int32_value.assign(&i32v);
  sub.members.uint32_value.assign(&u32v);
  sub.members.int64_value.assign(&i64v);
  sub.members.uint64_value.assign(&u64v);
  experimental::Nested::ExternalStorage s;
  s.members.basic_types_value = sub;
  experimental::Nested msg {s, rosidl_runtime_cpp::MessageInitialization::ALL};
  EXPECT_FALSE(msg.basic_types_value.bool_value);
  EXPECT_EQ(0, msg.basic_types_value.int32_value);
}

TEST(test_experimental_initialization_storage_ctor,
  arrays_storage_ctor_skip_scalar_region_preserves_values) {
  // Partial backing (int32_values only) — SKIP must leave the values untouched.
  std::array<int32_t, 3> buf {10, 20, 30};
  experimental::Arrays::ExternalStorage s;
  s.members.int32_values.assign(buf.data(), 3);
  experimental::Arrays msg {s, rosidl_runtime_cpp::MessageInitialization::SKIP};
  EXPECT_EQ(10, msg.int32_values[0]);
  EXPECT_EQ(20, msg.int32_values[1]);
  EXPECT_EQ(30, msg.int32_values[2]);
}

TEST(test_experimental_initialization_storage_ctor,
  arrays_storage_ctor_skip_string_region_is_usable) {
  // Partial backing (string_values only) — SKIP must leave the regions usable.
  char bufs[3][64] = {};
  experimental::Arrays::ExternalStorage s;
  for (int i = 0; i < 3; ++i) {
    s.members.string_values[i].assign(bufs[i], sizeof(bufs[i]));
  }
  experimental::Arrays msg {s, rosidl_runtime_cpp::MessageInitialization::SKIP};
  EXPECT_TRUE(msg.string_values[0].empty());
  msg.string_values[0] = "first";
  EXPECT_EQ("first", msg.string_values[0]);
}

TEST(test_experimental_initialization_storage_ctor, bounded_sequences_storage_ctor_skip_empty) {
  // Without full backing, SKIP is the only safe init.
  // size() reads stored length metadata — safe even with null data pointer.
  experimental::BoundedSequences::ExternalStorage s;
  experimental::BoundedSequences msg {s, rosidl_runtime_cpp::MessageInitialization::SKIP};
  EXPECT_EQ(0u, msg.int32_values.size());
  EXPECT_EQ(0u, msg.string_values.size());
  EXPECT_EQ(0u, msg.basic_types_values.size());
  EXPECT_EQ(0u, msg.int32_values_default.size());
}

TEST(test_experimental_initialization_storage_ctor, unbounded_sequences_storage_ctor_skip_empty) {
  // Same as bounded: SKIP with null regions; check size only.
  experimental::UnboundedSequences::ExternalStorage s;
  experimental::UnboundedSequences msg {s, rosidl_runtime_cpp::MessageInitialization::SKIP};
  EXPECT_EQ(0u, msg.int32_values.size());
  EXPECT_EQ(0u, msg.string_values.size());
  EXPECT_EQ(0u, msg.basic_types_values.size());
  EXPECT_EQ(0u, msg.int32_values_default.size());
}

TEST(test_experimental_initialization_storage_ctor,
  multi_nested_storage_ctor_skip_sequences_empty) {
  // No backing for array-of-submessage regions; SKIP only.
  experimental::MultiNested::ExternalStorage s;
  experimental::MultiNested msg {s, rosidl_runtime_cpp::MessageInitialization::SKIP};
  EXPECT_EQ(0u, msg.bounded_sequence_of_arrays.size());
  EXPECT_EQ(0u, msg.bounded_sequence_of_bounded_sequences.size());
  EXPECT_EQ(0u, msg.unbounded_sequence_of_arrays.size());
  EXPECT_EQ(0u, msg.unbounded_sequence_of_bounded_sequences.size());
}
