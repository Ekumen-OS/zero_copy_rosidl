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
#include "rosidl_runtime_c/experimental/array.h"

// Primitive fixed-size arrays (N=4).
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__FloatArray4, float, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__DoubleArray4, double, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__LongDoubleArray4, long double, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__CharArray4, char, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__WCharArray4, char16_t, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__BooleanArray4, bool, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__UInt8Array4, uint8_t, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__Int8Array4, int8_t, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__UInt16Array4, uint16_t, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__Int16Array4, int16_t, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__UInt32Array4, uint32_t, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__Int32Array4, int32_t, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__UInt64Array4, uint64_t, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE(
  rosidl_runtime_c__experimental__Int64Array4, int64_t, 4U);

// String/WString fixed-size arrays (N=4).
ROSIDL_RUNTIME_C__EXPERIMENTAL__ARRAY_DECLARE(
  rosidl_runtime_c__experimental__StringArray4,
  rosidl_runtime_c__experimental__String, 4U);
ROSIDL_RUNTIME_C__EXPERIMENTAL__ARRAY_DECLARE(
  rosidl_runtime_c__experimental__WStringArray4,
  rosidl_runtime_c__experimental__WString, 4U);
}  // extern "C"

// =============================================================================
// ArrayWrapper<T> — uniform lifecycle API over any declared array type.
// =============================================================================

template<typename T>
struct ArrayWrapper;

#define DEFINE_ARRAY_WRAPPER(T) \
  template<> \
  struct ArrayWrapper<T> { \
    static bool init(T * a) \
    {return T ## __init(a);} \
    static bool init_from_region(T * a, rosidl_memory_region_t r) \
    { \
      T ## __InitOptions opts = {0}; \
      opts.external_storage = &r; \
      return T ## __init_with_options(a, &opts); \
    } \
    static void fini(T * a) \
    {T ## __fini(a);} \
    static bool are_equal(const T * l, const T * r) \
    {return T ## __are_equal(l, r);} \
    static bool copy(const T * in, T * out) \
    {return T ## __copy(in, out);} \
  };

// clang-format off
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__FloatArray4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__DoubleArray4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__LongDoubleArray4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__CharArray4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__WCharArray4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__BooleanArray4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__UInt8Array4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__Int8Array4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__UInt16Array4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__Int16Array4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__UInt32Array4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__Int32Array4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__UInt64Array4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__Int64Array4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__StringArray4)
DEFINE_ARRAY_WRAPPER(rosidl_runtime_c__experimental__WStringArray4)
// clang-format on

// =============================================================================
// ArrayTestTraits<T> — per-type test data consumed by assertion bodies.
//
// DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS: element_type + test_value() for primitives.
// DEFINE_STRING_ARRAY_TEST_TRAITS: char_type, element-level assign(), literals.
// =============================================================================

template<typename T>
struct ArrayTestTraits;

#define DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(T, ELEM, TEST_VAL) \
  template<> \
  struct ArrayTestTraits<T> { \
    using element_type = ELEM; \
    static constexpr size_t size = 4U; \
    static constexpr element_type test_value() \
    {return static_cast<ELEM>(TEST_VAL);} \
  };

// clang-format off
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__FloatArray4, float, 3.14f)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__DoubleArray4, double, 3.14)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__LongDoubleArray4, long double,
  3.14L)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__CharArray4, char, 'Z')
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__WCharArray4, char16_t, u'Z')
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__BooleanArray4, bool, true)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__UInt8Array4, uint8_t, 42u)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__Int8Array4, int8_t, 42)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__UInt16Array4, uint16_t, 1000u)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__Int16Array4, int16_t, -500)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__UInt32Array4, uint32_t, 100000u)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__Int32Array4, int32_t, -42)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__UInt64Array4, uint64_t,
  1000000000ULL)
DEFINE_PRIMITIVE_ARRAY_TEST_TRAITS(rosidl_runtime_c__experimental__Int64Array4, int64_t,
  -1000000000LL)
// clang-format on

// String array traits add: char_type, element_type, assign(), and string literals.
#define DEFINE_STRING_ARRAY_TEST_TRAITS(T, ELEM_T, CHAR_T, HELLO, WORLD) \
  template<> \
  struct ArrayTestTraits<T> { \
    using element_type = ELEM_T; \
    using char_type = CHAR_T; \
    static constexpr size_t size = 4U; \
    static constexpr size_t hello_len = 5U; \
    static constexpr size_t world_len = 5U; \
    static const CHAR_T * hello() {return HELLO;} \
    static const CHAR_T * world() {return WORLD;} \
    static bool assign(ELEM_T * e, const CHAR_T * s) \
    {return ELEM_T ## __assign(e, s);} \
  };

DEFINE_STRING_ARRAY_TEST_TRAITS(
  rosidl_runtime_c__experimental__StringArray4,
  rosidl_runtime_c__experimental__String, char, "hello", "world")
DEFINE_STRING_ARRAY_TEST_TRAITS(
  rosidl_runtime_c__experimental__WStringArray4,
  rosidl_runtime_c__experimental__WString, char16_t, u"hello", u"world")

// =============================================================================
// Primitive array typed tests
// =============================================================================

template<typename T>
class PrimitiveArrayTest : public testing::Test {};

// clang-format off
using PrimitiveArrayTypes = testing::Types<
  rosidl_runtime_c__experimental__FloatArray4,
  rosidl_runtime_c__experimental__DoubleArray4,
  rosidl_runtime_c__experimental__LongDoubleArray4,
  rosidl_runtime_c__experimental__CharArray4,
  rosidl_runtime_c__experimental__WCharArray4,
  rosidl_runtime_c__experimental__BooleanArray4,
  rosidl_runtime_c__experimental__UInt8Array4,
  rosidl_runtime_c__experimental__Int8Array4,
  rosidl_runtime_c__experimental__UInt16Array4,
  rosidl_runtime_c__experimental__Int16Array4,
  rosidl_runtime_c__experimental__UInt32Array4,
  rosidl_runtime_c__experimental__Int32Array4,
  rosidl_runtime_c__experimental__UInt64Array4,
  rosidl_runtime_c__experimental__Int64Array4>;
// clang-format on

TYPED_TEST_SUITE(PrimitiveArrayTest, PrimitiveArrayTypes);

TYPED_TEST(PrimitiveArrayTest, local_storage_init_succeeds)
{
  using Array = ArrayWrapper<TypeParam>;
  TypeParam array{};
  ASSERT_TRUE(Array::init(&array));
  EXPECT_NE(nullptr, array.value);
  Array::fini(&array);
}

TYPED_TEST(PrimitiveArrayTest, local_storage_write_and_read)
{
  using Array = ArrayWrapper<TypeParam>;
  using Traits = ArrayTestTraits<TypeParam>;
  TypeParam array{};
  ASSERT_TRUE(Array::init(&array));
  for (size_t i = 0U; i < Traits::size; ++i) {
    array.value->data[i] = Traits::test_value();
  }
  for (size_t i = 0U; i < Traits::size; ++i) {
    EXPECT_EQ(Traits::test_value(), array.value->data[i]);
  }
  Array::fini(&array);
}

TYPED_TEST(PrimitiveArrayTest, external_storage_write_reflects_in_backing_buffer)
{
  using Array = ArrayWrapper<TypeParam>;
  using Traits = ArrayTestTraits<TypeParam>;
  typename Traits::element_type backing[4] = {};
  rosidl_memory_region_t region{};
  region.location.address = backing;
  region.location.attributes = 0;
  region.size = sizeof(backing);

  TypeParam array{};
  ASSERT_TRUE(Array::init_from_region(&array, region));
  ASSERT_NE(nullptr, array.value);
  array.value->data[0U] = Traits::test_value();
  array.value->data[3U] = Traits::test_value();
  EXPECT_EQ(Traits::test_value(), backing[0U]);
  EXPECT_EQ(Traits::test_value(), backing[3U]);
}

TYPED_TEST(PrimitiveArrayTest, external_storage_too_small_fails)
{
  using Array = ArrayWrapper<TypeParam>;
  using Traits = ArrayTestTraits<TypeParam>;
  typename Traits::element_type backing[2] = {};  // room for 2, need 4
  rosidl_memory_region_t region{};
  region.location.address = backing;
  region.location.attributes = 0;
  region.size = sizeof(backing);

  TypeParam array{};
  EXPECT_FALSE(Array::init_from_region(&array, region));
}

TYPED_TEST(PrimitiveArrayTest, fini_local_storage_keeps_value_non_null)
{
  using Array = ArrayWrapper<TypeParam>;
  TypeParam array{};
  ASSERT_TRUE(Array::init(&array));
  Array::fini(&array);
  EXPECT_NE(nullptr, array.value);
}

TYPED_TEST(PrimitiveArrayTest, fini_external_storage_nullifies_value)
{
  using Array = ArrayWrapper<TypeParam>;
  using Traits = ArrayTestTraits<TypeParam>;
  typename Traits::element_type backing[4] = {};
  rosidl_memory_region_t region{};
  region.location.address = backing;
  region.location.attributes = 0;
  region.size = sizeof(backing);

  TypeParam array{};
  ASSERT_TRUE(Array::init_from_region(&array, region));
  Array::fini(&array);
  EXPECT_EQ(nullptr, array.value);
}

TYPED_TEST(PrimitiveArrayTest, init_null_returns_false)
{
  EXPECT_FALSE(ArrayWrapper<TypeParam>::init(nullptr));
}

TYPED_TEST(PrimitiveArrayTest, init_from_region_null_array_returns_false)
{
  using Traits = ArrayTestTraits<TypeParam>;
  typename Traits::element_type backing[4] = {};
  rosidl_memory_region_t region{};
  region.location.address = backing;
  region.location.attributes = 0;
  region.size = sizeof(backing);
  EXPECT_FALSE(ArrayWrapper<TypeParam>::init_from_region(nullptr, region));
}

TYPED_TEST(PrimitiveArrayTest, init_from_region_null_address_returns_false)
{
  TypeParam array{};
  rosidl_memory_region_t region{};
  region.location.address = nullptr;
  region.location.attributes = 0;
  region.size = sizeof(typename ArrayTestTraits<TypeParam>::element_type) * 4U;
  EXPECT_FALSE(ArrayWrapper<TypeParam>::init_from_region(&array, region));
}

TYPED_TEST(PrimitiveArrayTest, fini_null_is_safe)
{
  EXPECT_NO_FATAL_FAILURE(ArrayWrapper<TypeParam>::fini(nullptr));
}

TYPED_TEST(PrimitiveArrayTest, are_equal_and_copy)
{
  using Array = ArrayWrapper<TypeParam>;
  using Traits = ArrayTestTraits<TypeParam>;
  TypeParam lhs{}, rhs{};
  ASSERT_TRUE(Array::init(&lhs));
  ASSERT_TRUE(Array::init(&rhs));
  for (size_t i = 0U; i < Traits::size; ++i) {
    lhs.value->data[i] = Traits::test_value();
  }
  ASSERT_TRUE(Array::copy(&lhs, &rhs));
  EXPECT_TRUE(Array::are_equal(&lhs, &rhs));
  Array::fini(&lhs);
  Array::fini(&rhs);
}

template<typename T>
class StringArrayTest : public testing::Test {};

using StringArrayTypes = testing::Types<
  rosidl_runtime_c__experimental__StringArray4,
  rosidl_runtime_c__experimental__WStringArray4>;

TYPED_TEST_SUITE(StringArrayTest, StringArrayTypes);

TYPED_TEST(StringArrayTest, local_storage_init_initializes_all_elements)
{
  using Array = ArrayWrapper<TypeParam>;
  using Traits = ArrayTestTraits<TypeParam>;
  TypeParam array{};
  ASSERT_TRUE(Array::init(&array));
  ASSERT_NE(nullptr, array.value);
  for (size_t i = 0U; i < Traits::size; ++i) {
    EXPECT_EQ(0U, array.value->data[i].size);
    EXPECT_NE(nullptr, array.value->data[i].value);
  }
  Array::fini(&array);
}

TYPED_TEST(StringArrayTest, elements_are_independently_assignable)
{
  using Array = ArrayWrapper<TypeParam>;
  using Traits = ArrayTestTraits<TypeParam>;
  TypeParam array{};
  ASSERT_TRUE(Array::init(&array));
  ASSERT_TRUE(Traits::assign(&array.value->data[0], Traits::hello()));
  ASSERT_TRUE(Traits::assign(&array.value->data[3], Traits::world()));
  EXPECT_EQ(Traits::hello_len, array.value->data[0].size);
  EXPECT_EQ(Traits::world_len, array.value->data[3].size);
  EXPECT_EQ(0U, array.value->data[1].size);  // untouched element unchanged
  Array::fini(&array);
}

TYPED_TEST(StringArrayTest, fini_local_storage_keeps_value_non_null)
{
  using Array = ArrayWrapper<TypeParam>;
  TypeParam array{};
  ASSERT_TRUE(Array::init(&array));
  Array::fini(&array);
  EXPECT_NE(nullptr, array.value);
}

TYPED_TEST(StringArrayTest, are_equal_and_copy)
{
  using Array = ArrayWrapper<TypeParam>;
  using Traits = ArrayTestTraits<TypeParam>;
  TypeParam lhs{}, rhs{};
  ASSERT_TRUE(Array::init(&lhs));
  ASSERT_TRUE(Array::init(&rhs));
  ASSERT_TRUE(Traits::assign(&lhs.value->data[0], Traits::hello()));
  ASSERT_TRUE(Traits::assign(&lhs.value->data[1], Traits::world()));
  ASSERT_TRUE(Array::copy(&lhs, &rhs));
  EXPECT_TRUE(Array::are_equal(&lhs, &rhs));
  Array::fini(&lhs);
  Array::fini(&rhs);
}

TYPED_TEST(StringArrayTest, init_null_returns_false)
{
  EXPECT_FALSE(ArrayWrapper<TypeParam>::init(nullptr));
}

TYPED_TEST(StringArrayTest, fini_null_is_safe)
{
  EXPECT_NO_FATAL_FAILURE(ArrayWrapper<TypeParam>::fini(nullptr));
}
