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

#include <cstdint>

#include "gtest/gtest.h"

extern "C"
{
#include "rcutils/allocator.h"
#include "rosidl_runtime_c/experimental/sequence.h"
}  // extern "C"

// =============================================================================
// SequenceWrapper<T> — uniform lifecycle API over any declared sequence type.
// =============================================================================

template<typename T>
struct SequenceWrapper;

#define DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(SEQ_T, BOUNDED_T, ELEM_T, BOUND) \
  template<> \
  struct SequenceWrapper<SEQ_T> { \
    using bounded_type = BOUNDED_T; \
    static constexpr size_t bound = BOUND; \
    static bool init(SEQ_T * s, const rcutils_allocator_t * a) \
    { \
      SEQ_T ## __InitOptions opts = {.allocator = a, .external_storage = NULL, .reserved = {NULL, NULL, NULL, NULL}}; \
      return SEQ_T ## __init_with_options(s, &opts); \
    } \
    static bool init_from_region(SEQ_T * s, rosidl_memory_region_t r) \
    { \
      SEQ_T ## __InitOptions opts = {.allocator = NULL, .external_storage = &r, .reserved = {NULL, NULL, NULL, NULL}}; \
      return SEQ_T ## __init_with_options(s, &opts); \
    } \
    static void fini(SEQ_T * s) \
    {SEQ_T ## __fini(s);} \
    static bool push_back(SEQ_T * s, ELEM_T v) \
    {return SEQ_T ## __push_back(s, v);} \
    static bool resize(SEQ_T * s, size_t n) \
    {return SEQ_T ## __resize(s, n);} \
    static bool reserve(SEQ_T * s, size_t n) \
    {return SEQ_T ## __reserve(s, n);} \
    static bool are_equal(const SEQ_T * l, const SEQ_T * r) \
    {return SEQ_T ## __are_equal(l, r);} \
    static bool copy(const SEQ_T * in, SEQ_T * out) \
    {return SEQ_T ## __copy(in, out);} \
    static bool bounded_init(BOUNDED_T * s, const rcutils_allocator_t * a) \
    { \
      BOUNDED_T ## __InitOptions opts = {.allocator = a, .external_storage = NULL, .reserved = {NULL, NULL, NULL, NULL}}; \
      return BOUNDED_T ## __init_with_options(s, BOUND, &opts); \
    } \
    static bool bounded_init_from_region(BOUNDED_T * s, rosidl_memory_region_t r) \
    { \
      BOUNDED_T ## __InitOptions opts = {.allocator = NULL, .external_storage = &r, .reserved = {NULL, NULL, NULL, NULL}}; \
      return BOUNDED_T ## __init_with_options(s, BOUND, &opts); \
    } \
    static void bounded_fini(BOUNDED_T * s) \
    {BOUNDED_T ## __fini(s);} \
    static bool bounded_push_back(BOUNDED_T * s, ELEM_T v) \
    {return BOUNDED_T ## __push_back(s, v);} \
    static bool bounded_resize(BOUNDED_T * s, size_t n) \
    {return BOUNDED_T ## __resize(s, n);} \
    static bool bounded_reserve(BOUNDED_T * s, size_t n) \
    {return BOUNDED_T ## __reserve(s, n);} \
  };

// clang-format off
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__Float__Sequence,
  rosidl_runtime_c__experimental__Float__BoundedSequence, float, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__Double__Sequence,
  rosidl_runtime_c__experimental__Double__BoundedSequence, double, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__LongDouble__Sequence,
  rosidl_runtime_c__experimental__LongDouble__BoundedSequence, long double, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__Char__Sequence,
  rosidl_runtime_c__experimental__Char__BoundedSequence, char, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__WChar__Sequence,
  rosidl_runtime_c__experimental__WChar__BoundedSequence, wchar_t, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__Boolean__Sequence,
  rosidl_runtime_c__experimental__Boolean__BoundedSequence, bool, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__UInt8__Sequence,
  rosidl_runtime_c__experimental__UInt8__BoundedSequence, uint8_t, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__Int8__Sequence,
  rosidl_runtime_c__experimental__Int8__BoundedSequence, int8_t, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__UInt16__Sequence,
  rosidl_runtime_c__experimental__UInt16__BoundedSequence, uint16_t, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__Int16__Sequence,
  rosidl_runtime_c__experimental__Int16__BoundedSequence, int16_t, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__UInt32__Sequence,
  rosidl_runtime_c__experimental__UInt32__BoundedSequence, uint32_t, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__Int32__Sequence,
  rosidl_runtime_c__experimental__Int32__BoundedSequence, int32_t, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__UInt64__Sequence,
  rosidl_runtime_c__experimental__UInt64__BoundedSequence, uint64_t, 3U)
DEFINE_PRIMITIVE_SEQUENCE_WRAPPER(rosidl_runtime_c__experimental__Int64__Sequence,
  rosidl_runtime_c__experimental__Int64__BoundedSequence, int64_t, 3U)
// clang-format on

// =============================================================================
// SequenceTestTraits<T> — per-type test data consumed by assertion bodies.
// =============================================================================

template<typename T>
struct SequenceTestTraits;

#define DEFINE_SEQUENCE_TEST_TRAITS(SEQ_T, VALUE_T, VAL_A, VAL_B) \
  template<> \
  struct SequenceTestTraits<SEQ_T> { \
    using sequence_type = SEQ_T; \
    using value_type = VALUE_T; \
    static constexpr VALUE_T value_a() {return VAL_A;} \
    static constexpr VALUE_T value_b() {return VAL_B;} \
  };

// clang-format off
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__Float__Sequence, float, 1.0f, 2.0f)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__Double__Sequence, double, 1.0, 2.0)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__LongDouble__Sequence, long double, 1.0L,
  2.0L)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__Char__Sequence, char, 'a', 'b')
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__WChar__Sequence, char16_t, u'a', u'b')
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__Boolean__Sequence, bool, true, false)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__UInt8__Sequence, uint8_t, 10u, 20u)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__Int8__Sequence, int8_t, 10, 20)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__UInt16__Sequence, uint16_t, 100u, 200u)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__Int16__Sequence, int16_t, -100, 200)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__UInt32__Sequence, uint32_t, 1000u, 2000u)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__Int32__Sequence, int32_t, -1000, 2000)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__UInt64__Sequence, uint64_t, 100000ULL,
  200000ULL)
DEFINE_SEQUENCE_TEST_TRAITS(rosidl_runtime_c__experimental__Int64__Sequence, int64_t, -100000LL,
  200000LL)
// clang-format on

// =============================================================================
// Sequence typed tests
// =============================================================================

template<typename T>
class SequenceTest : public ::testing::Test
{
protected:
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  T seq{};
};

// clang-format off
using SequenceTypes = ::testing::Types<
  rosidl_runtime_c__experimental__Float__Sequence,
  rosidl_runtime_c__experimental__Double__Sequence,
  rosidl_runtime_c__experimental__LongDouble__Sequence,
  rosidl_runtime_c__experimental__Char__Sequence,
  rosidl_runtime_c__experimental__WChar__Sequence,
  rosidl_runtime_c__experimental__Boolean__Sequence,
  rosidl_runtime_c__experimental__UInt8__Sequence,
  rosidl_runtime_c__experimental__Int8__Sequence,
  rosidl_runtime_c__experimental__UInt16__Sequence,
  rosidl_runtime_c__experimental__Int16__Sequence,
  rosidl_runtime_c__experimental__UInt32__Sequence,
  rosidl_runtime_c__experimental__Int32__Sequence,
  rosidl_runtime_c__experimental__UInt64__Sequence,
  rosidl_runtime_c__experimental__Int64__Sequence>;
// clang-format on

TYPED_TEST_SUITE(SequenceTest, SequenceTypes);

TYPED_TEST(SequenceTest, managed_init_empty)
{
  using Sequence = SequenceWrapper<TypeParam>;
  ASSERT_TRUE(Sequence::init(&this->seq, &this->allocator));
  EXPECT_EQ(0U, this->seq.size);
  EXPECT_EQ(0U, this->seq.capacity);
  Sequence::fini(&this->seq);
}

TYPED_TEST(SequenceTest, managed_push_back_and_read)
{
  using Sequence = SequenceWrapper<TypeParam>;
  using Traits = SequenceTestTraits<TypeParam>;
  ASSERT_TRUE(Sequence::init(&this->seq, &this->allocator));
  ASSERT_TRUE(Sequence::push_back(&this->seq, Traits::value_a()));
  ASSERT_TRUE(Sequence::push_back(&this->seq, Traits::value_b()));
  EXPECT_EQ(2U, this->seq.size);
  EXPECT_EQ(Traits::value_a(), this->seq.value[0U]);
  EXPECT_EQ(Traits::value_b(), this->seq.value[1U]);
  Sequence::fini(&this->seq);
}

TYPED_TEST(SequenceTest, managed_resize_zeroes_new_elements)
{
  using Sequence = SequenceWrapper<TypeParam>;
  using Traits = SequenceTestTraits<TypeParam>;
  ASSERT_TRUE(Sequence::init(&this->seq, &this->allocator));
  ASSERT_TRUE(Sequence::resize(&this->seq, 3U));
  EXPECT_EQ(3U, this->seq.size);
  for (size_t i = 0U; i < 3U; ++i) {
    EXPECT_EQ(typename Traits::value_type{}, this->seq.value[i]);
  }
  Sequence::fini(&this->seq);
}

TYPED_TEST(SequenceTest, managed_copy_and_equality)
{
  using Sequence = SequenceWrapper<TypeParam>;
  using Traits = SequenceTestTraits<TypeParam>;
  TypeParam other{};
  ASSERT_TRUE(Sequence::init(&this->seq, &this->allocator));
  ASSERT_TRUE(Sequence::init(&other, &this->allocator));
  ASSERT_TRUE(Sequence::push_back(&this->seq, Traits::value_a()));
  ASSERT_TRUE(Sequence::push_back(&this->seq, Traits::value_b()));
  ASSERT_TRUE(Sequence::copy(&this->seq, &other));
  EXPECT_TRUE(Sequence::are_equal(&this->seq, &other));
  Sequence::fini(&this->seq);
  Sequence::fini(&other);
}

TYPED_TEST(SequenceTest, external_region_gives_fixed_capacity)
{
  using Sequence = SequenceWrapper<TypeParam>;
  using Traits = SequenceTestTraits<TypeParam>;
  typename Traits::value_type backing[3] = {};
  rosidl_memory_region_t region{{backing, 0}, sizeof(backing)};
  ASSERT_TRUE(Sequence::init_from_region(&this->seq, region));
  EXPECT_EQ(3U, this->seq.capacity);
  EXPECT_TRUE(Sequence::resize(&this->seq, 3U));
  EXPECT_FALSE(Sequence::push_back(&this->seq, Traits::value_a()));
}

TYPED_TEST(SequenceTest, bounded_upper_bound_is_enforced)
{
  using Sequence = SequenceWrapper<TypeParam>;
  using Traits = SequenceTestTraits<TypeParam>;
  typename Sequence::bounded_type bounded{};
  ASSERT_TRUE(Sequence::bounded_init(&bounded, &this->allocator));
  EXPECT_TRUE(Sequence::bounded_push_back(&bounded, Traits::value_a()));
  EXPECT_TRUE(Sequence::bounded_push_back(&bounded, Traits::value_b()));
  EXPECT_TRUE(Sequence::bounded_push_back(&bounded, Traits::value_a()));
  EXPECT_FALSE(Sequence::bounded_push_back(&bounded, Traits::value_b()));
  EXPECT_FALSE(Sequence::bounded_reserve(&bounded, 4U));
  Sequence::bounded_fini(&bounded);
}

TYPED_TEST(SequenceTest, bounded_external_region_is_clamped_to_upper_bound)
{
  using Sequence = SequenceWrapper<TypeParam>;
  using Traits = SequenceTestTraits<TypeParam>;
  typename Traits::value_type backing[8] = {};
  rosidl_memory_region_t region{{backing, 0}, sizeof(backing)};
  typename Sequence::bounded_type bounded{};
  ASSERT_TRUE(Sequence::bounded_init_from_region(&bounded, region));
  EXPECT_EQ(3U, bounded.capacity);
  EXPECT_TRUE(Sequence::bounded_resize(&bounded, 3U));
  EXPECT_FALSE(Sequence::bounded_resize(&bounded, 4U));
}

TYPED_TEST(SequenceTest, init_null_returns_false)
{
  EXPECT_FALSE(SequenceWrapper<TypeParam>::init(nullptr, &this->allocator));
}

TYPED_TEST(SequenceTest, init_from_region_null_sequence_returns_false)
{
  using Traits = SequenceTestTraits<TypeParam>;
  typename Traits::value_type backing[4] = {};
  rosidl_memory_region_t region{{backing, 0}, sizeof(backing)};
  EXPECT_FALSE(SequenceWrapper<TypeParam>::init_from_region(nullptr, region));
}

TYPED_TEST(SequenceTest, init_from_region_null_address_returns_false)
{
  rosidl_memory_region_t region{{nullptr, 0}, 16U};
  EXPECT_FALSE(SequenceWrapper<TypeParam>::init_from_region(&this->seq, region));
}

TYPED_TEST(SequenceTest, fini_null_is_safe)
{
  EXPECT_NO_FATAL_FAILURE(SequenceWrapper<TypeParam>::fini(nullptr));
}
