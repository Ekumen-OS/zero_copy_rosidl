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
#include "rosidl_runtime_c/experimental/scalar.h"
}

// =============================================================================
// Typed test infrastructure
//
// ScalarParam<ScalarT, ValueT, kTestValue> is the type parameter.
// The typed tests call the C API directly via the concrete names stored in
// the param — no forwarding wrappers needed.
// =============================================================================
// ScalarWrapper<T> — uniform lifecycle API over any declared scalar type.
// =============================================================================

template<typename T>
struct ScalarWrapper;

#define DEFINE_SCALAR_WRAPPER(T) \
  template<> \
  struct ScalarWrapper<T> { \
    static bool init(T * s) \
    {return T ## __init(s);} \
    static bool init_from_memory(T * s, rosidl_memory_t m) \
    { \
      T ## __InitOptions opts = {.external_memory = &m, .reserved = {NULL, NULL, NULL, NULL}}; \
      return T ## __init_with_options(s, &opts); \
    } \
    static void fini(T * s) \
    {T ## __fini(s);} \
  };

// clang-format off
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__Float)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__Double)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__LongDouble)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__Char)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__WChar)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__Boolean)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__UInt8)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__Int8)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__UInt16)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__Int16)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__UInt32)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__Int32)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__UInt64)
DEFINE_SCALAR_WRAPPER(rosidl_runtime_c__experimental__Int64)
// clang-format on

template<typename T>
struct ScalarTestTraits;

#define DEFINE_SCALAR_TEST_TRAITS(T, VALUE, TEST_VAL) \
  template<> \
  struct ScalarTestTraits<T> { \
    using scalar_type = T; \
    using value_type = VALUE; \
    static constexpr VALUE test_value() {return TEST_VAL;} \
  };

// clang-format off
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__Float, float, 3.14f)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__Double, double, 3.14)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__LongDouble, long double, 3.14L)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__Char, char, 'A')
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__WChar, char16_t, u'A')
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__Boolean, bool, true)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__UInt8, uint8_t, 42u)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__Int8, int8_t, 42)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__UInt16, uint16_t, 1000u)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__Int16, int16_t, -500)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__UInt32, uint32_t, 100000u)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__Int32, int32_t, -42)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__UInt64, uint64_t, 1000000000ULL)
DEFINE_SCALAR_TEST_TRAITS(rosidl_runtime_c__experimental__Int64, int64_t, -1000000000LL)
// clang-format on

template<typename T>
class ScalarTest : public ::testing::Test
{
protected:
  using Scalar = ScalarWrapper<T>;
  using Traits = ScalarTestTraits<T>;
  using S = typename Traits::scalar_type;
  using V = typename Traits::value_type;

  S scalar{};
};

// clang-format off
using ScalarTypes = ::testing::Types<
  rosidl_runtime_c__experimental__Float,
  rosidl_runtime_c__experimental__Double,
  rosidl_runtime_c__experimental__LongDouble,
  rosidl_runtime_c__experimental__Char,
  rosidl_runtime_c__experimental__WChar,
  rosidl_runtime_c__experimental__Boolean,
  rosidl_runtime_c__experimental__UInt8,
  rosidl_runtime_c__experimental__Int8,
  rosidl_runtime_c__experimental__UInt16,
  rosidl_runtime_c__experimental__Int16,
  rosidl_runtime_c__experimental__UInt32,
  rosidl_runtime_c__experimental__Int32,
  rosidl_runtime_c__experimental__UInt64,
  rosidl_runtime_c__experimental__Int64>;
// clang-format on

TYPED_TEST_SUITE(ScalarTest, ScalarTypes);

TYPED_TEST(ScalarTest, local_storage_init_zeroes_value)
{
  using Scalar = ScalarWrapper<TypeParam>;
  using Traits = ScalarTestTraits<TypeParam>;
  ASSERT_TRUE(Scalar::init(&this->scalar));
  ASSERT_NE(nullptr, this->scalar.value);
  EXPECT_EQ(typename Traits::value_type{}, this->scalar.value->data);
  Scalar::fini(&this->scalar);
}

TYPED_TEST(ScalarTest, local_storage_write_and_read)
{
  using Scalar = ScalarWrapper<TypeParam>;
  using Traits = ScalarTestTraits<TypeParam>;
  ASSERT_TRUE(Scalar::init(&this->scalar));
  this->scalar.value->data = Traits::test_value();
  EXPECT_EQ(Traits::test_value(), this->scalar.value->data);
  Scalar::fini(&this->scalar);
}

TYPED_TEST(ScalarTest, external_storage_write_reflects_in_backing_variable)
{
  using Scalar = ScalarWrapper<TypeParam>;
  using Traits = ScalarTestTraits<TypeParam>;
  typename Traits::value_type external{};
  ASSERT_TRUE(Scalar::init_from_memory(&this->scalar, {&external, 0}));
  ASSERT_NE(nullptr, this->scalar.value);
  this->scalar.value->data = Traits::test_value();
  EXPECT_EQ(Traits::test_value(), external);
}

TYPED_TEST(ScalarTest, fini_local_storage_resets_value_to_zero)
{
  using Scalar = ScalarWrapper<TypeParam>;
  using Traits = ScalarTestTraits<TypeParam>;
  ASSERT_TRUE(Scalar::init(&this->scalar));
  this->scalar.value->data = Traits::test_value();
  Scalar::fini(&this->scalar);
  ASSERT_NE(nullptr, this->scalar.value);
  EXPECT_EQ(typename Traits::value_type{}, this->scalar.value->data);
}

TYPED_TEST(ScalarTest, fini_external_storage_nullifies_value_pointer)
{
  using Scalar = ScalarWrapper<TypeParam>;
  using Traits = ScalarTestTraits<TypeParam>;
  typename Traits::value_type external{};
  ASSERT_TRUE(Scalar::init_from_memory(&this->scalar, {&external, 0}));
  Scalar::fini(&this->scalar);
  EXPECT_EQ(nullptr, this->scalar.value);
}

TYPED_TEST(ScalarTest, init_null_returns_false)
{
  EXPECT_FALSE(ScalarWrapper<TypeParam>::init(nullptr));
}

TYPED_TEST(ScalarTest, init_from_memory_null_scalar_returns_false)
{
  using Traits = ScalarTestTraits<TypeParam>;
  typename Traits::value_type v{};
  EXPECT_FALSE(ScalarWrapper<TypeParam>::init_from_memory(nullptr, {&v, 0}));
}

TYPED_TEST(ScalarTest, init_from_memory_null_address_returns_false)
{
  EXPECT_FALSE(ScalarWrapper<TypeParam>::init_from_memory(&this->scalar, {nullptr, 0}));
}

TYPED_TEST(ScalarTest, fini_null_is_safe)
{
  EXPECT_NO_FATAL_FAILURE(ScalarWrapper<TypeParam>::fini(nullptr));
}
