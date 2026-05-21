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

#include <cstring>
#include <cuchar>

#include "gtest/gtest.h"

extern "C"
{
#include "rcutils/allocator.h"
#include "rosidl_runtime_c/experimental/string.h"
}  // extern "C"

// =============================================================================
// StringWrapper<T> — uniform lifecycle API over any declared string type.
// =============================================================================

template<typename T>
struct StringWrapper;

#define DEFINE_STRING_WRAPPER(STR_T, BOUNDED_T, BOUND) \
  template<> \
  struct StringWrapper<STR_T> { \
    using bounded_type = BOUNDED_T; \
    static constexpr size_t bound = BOUND; \
    static bool init(STR_T * s, const rcutils_allocator_t * a) \
    { \
      STR_T ## __InitOptions opts = {.allocator = a, .external_storage = NULL, .reserved = {NULL, NULL, NULL, NULL}}; \
      return STR_T ## __init_with_options(s, &opts); \
    } \
    static bool init_from_region(STR_T * s, rosidl_memory_region_t r) \
    { \
      STR_T ## __InitOptions opts = {.allocator = NULL, .external_storage = &r, .reserved = {NULL, NULL, NULL, NULL}}; \
      return STR_T ## __init_with_options(s, &opts); \
    } \
    static void fini(STR_T * s) \
    {STR_T ## __fini(s);} \
    static bool assignn(STR_T * s, const auto * v, size_t n) \
    {return STR_T ## __assignn(s, v, n);} \
    static bool assign(STR_T * s, const auto * v) \
    {return STR_T ## __assign(s, v);} \
    static bool appendn(STR_T * s, const auto * v, size_t n) \
    {return STR_T ## __appendn(s, v, n);} \
    static bool resize(STR_T * s, size_t n) \
    {return STR_T ## __resize(s, n);} \
    static bool are_equal(const STR_T * l, const STR_T * r) \
    {return STR_T ## __are_equal(l, r);} \
    static bool copy(const STR_T * in, STR_T * out) \
    {return STR_T ## __copy(in, out);} \
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
    static bool bounded_assignn(BOUNDED_T * s, const auto * v, size_t n) \
    {return BOUNDED_T ## __assignn(s, v, n);} \
    static bool bounded_assign(BOUNDED_T * s, const auto * v) \
    {return BOUNDED_T ## __assign(s, v);} \
    static bool bounded_appendn(BOUNDED_T * s, const auto * v, size_t n) \
    {return BOUNDED_T ## __appendn(s, v, n);} \
    static bool bounded_resize(BOUNDED_T * s, size_t n) \
    {return BOUNDED_T ## __resize(s, n);} \
    static bool bounded_reserve(BOUNDED_T * s, size_t n) \
    {return BOUNDED_T ## __reserve(s, n);} \
    static bool bounded_copy(const BOUNDED_T * in, BOUNDED_T * out) \
    {return BOUNDED_T ## __copy(in, out);} \
    static bool bounded_are_equal(const BOUNDED_T * l, const BOUNDED_T * r) \
    {return BOUNDED_T ## __are_equal(l, r);} \
  };

DEFINE_STRING_WRAPPER(
  rosidl_runtime_c__experimental__String,
  rosidl_runtime_c__experimental__BoundedString, 5U)
DEFINE_STRING_WRAPPER(
  rosidl_runtime_c__experimental__WString,
  rosidl_runtime_c__experimental__BoundedWString, 5U)

// =============================================================================
// StringTestTraits<T> — per-type test data consumed by assertion bodies.
// =============================================================================

template<typename T>
struct StringTestTraits;

template<>
struct StringTestTraits<rosidl_runtime_c__experimental__String>
{
  using string_type = rosidl_runtime_c__experimental__String;
  using char_type = char;
  static const char * hello() {return "hello";}
  static const char * world() {return " world";}
  static const char * abcde() {return "abcde";}
  static const char * abcdef() {return "abcdef";}
  static constexpr size_t hello_len = 5U;
  static constexpr size_t world_len = 6U;
  static constexpr size_t abcde_len = 5U;
  static constexpr size_t abcdef_len = 6U;
};

template<>
struct StringTestTraits<rosidl_runtime_c__experimental__WString>
{
  using string_type = rosidl_runtime_c__experimental__WString;
  using char_type = char16_t;
  static const char16_t * hello() {return u"hello";}
  static const char16_t * world() {return u" world";}
  static const char16_t * abcde() {return u"abcde";}
  static const char16_t * abcdef() {return u"abcdef";}
  static constexpr size_t hello_len = 5U;
  static constexpr size_t world_len = 6U;
  static constexpr size_t abcde_len = 5U;
  static constexpr size_t abcdef_len = 6U;
};

// =============================================================================
// String typed tests
// =============================================================================

template<typename T>
class StringTest : public ::testing::Test
{
protected:
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  T str{};
};

using StringTypes = ::testing::Types<
  rosidl_runtime_c__experimental__String,
  rosidl_runtime_c__experimental__WString>;

TYPED_TEST_SUITE(StringTest, StringTypes);

TYPED_TEST(StringTest, managed_init_empty)
{
  using String = StringWrapper<TypeParam>;
  using Traits = StringTestTraits<TypeParam>;
  ASSERT_TRUE(String::init(&this->str, &this->allocator));
  EXPECT_EQ(0U, this->str.size);
  ASSERT_NE(nullptr, this->str.value);
  EXPECT_EQ(typename Traits::char_type{}, this->str.value[0]);
  String::fini(&this->str);
}

TYPED_TEST(StringTest, managed_assign_and_append)
{
  using String = StringWrapper<TypeParam>;
  using Traits = StringTestTraits<TypeParam>;
  ASSERT_TRUE(String::init(&this->str, &this->allocator));
  ASSERT_TRUE(String::assignn(&this->str, Traits::hello(), Traits::hello_len));
  ASSERT_TRUE(String::appendn(&this->str, Traits::world(), Traits::world_len));
  EXPECT_EQ(Traits::hello_len + Traits::world_len, this->str.size);
  String::fini(&this->str);
}

TYPED_TEST(StringTest, managed_resize_zeroes_new_chars)
{
  using String = StringWrapper<TypeParam>;
  using Traits = StringTestTraits<TypeParam>;
  ASSERT_TRUE(String::init(&this->str, &this->allocator));
  ASSERT_TRUE(String::assign(&this->str, Traits::hello()));
  ASSERT_TRUE(String::resize(&this->str, 3U));
  EXPECT_EQ(3U, this->str.size);
  EXPECT_EQ(typename Traits::char_type{}, this->str.value[3U]);
  String::fini(&this->str);
}

TYPED_TEST(StringTest, managed_copy_and_equality)
{
  using String = StringWrapper<TypeParam>;
  using Traits = StringTestTraits<TypeParam>;
  TypeParam other{};
  ASSERT_TRUE(String::init(&this->str, &this->allocator));
  ASSERT_TRUE(String::init(&other, &this->allocator));
  ASSERT_TRUE(String::assignn(&this->str, Traits::hello(), Traits::hello_len));
  ASSERT_TRUE(String::copy(&this->str, &other));
  EXPECT_TRUE(String::are_equal(&this->str, &other));
  other.value[0] = typename Traits::char_type{'X'};
  EXPECT_FALSE(String::are_equal(&this->str, &other));
  String::fini(&this->str);
  String::fini(&other);
}

TYPED_TEST(StringTest, external_region_enforces_fixed_capacity)
{
  using String = StringWrapper<TypeParam>;
  using Traits = StringTestTraits<TypeParam>;
  typename Traits::char_type storage[8] = {};
  rosidl_memory_region_t region{{storage, 0}, sizeof(storage)};
  ASSERT_TRUE(String::init_from_region(&this->str, region));
  ASSERT_TRUE(String::assignn(&this->str, Traits::abcde(), Traits::abcde_len));
  EXPECT_FALSE(String::appendn(&this->str, Traits::abcdef(), Traits::abcdef_len));
}

TYPED_TEST(StringTest, bounded_upper_bound_is_enforced)
{
  using String = StringWrapper<TypeParam>;
  using Traits = StringTestTraits<TypeParam>;
  typename String::bounded_type bounded{};
  ASSERT_TRUE(String::bounded_init(&bounded, &this->allocator));
  ASSERT_TRUE(String::bounded_assignn(&bounded, Traits::abcde(), Traits::abcde_len));
  EXPECT_FALSE(String::bounded_appendn(&bounded, Traits::abcde(), 1U));
  EXPECT_FALSE(String::bounded_reserve(&bounded, 6U));
  String::bounded_fini(&bounded);
}

TYPED_TEST(StringTest, bounded_external_region_is_clamped_to_upper_bound)
{
  using String = StringWrapper<TypeParam>;
  using Traits = StringTestTraits<TypeParam>;
  typename Traits::char_type storage[32] = {};
  rosidl_memory_region_t region{{storage, 0}, sizeof(storage)};
  typename String::bounded_type bounded{};
  ASSERT_TRUE(String::bounded_init_from_region(&bounded, region));
  EXPECT_EQ(5U, bounded._impl.capacity);
  ASSERT_TRUE(String::bounded_assignn(&bounded, Traits::abcde(), Traits::abcde_len));
  EXPECT_FALSE(String::bounded_assignn(&bounded, Traits::abcdef(), Traits::abcdef_len));
}

TYPED_TEST(StringTest, bounded_copy_and_equality)
{
  using String = StringWrapper<TypeParam>;
  using Traits = StringTestTraits<TypeParam>;
  typename String::bounded_type lhs{}, rhs{};
  ASSERT_TRUE(String::bounded_init(&lhs, &this->allocator));
  ASSERT_TRUE(String::bounded_init(&rhs, &this->allocator));
  ASSERT_TRUE(String::bounded_assign(&lhs, Traits::hello()));
  ASSERT_TRUE(String::bounded_copy(&lhs, &rhs));
  EXPECT_TRUE(String::bounded_are_equal(&lhs, &rhs));
  rhs.value[0] = typename Traits::char_type{'X'};
  EXPECT_FALSE(String::bounded_are_equal(&lhs, &rhs));
  String::bounded_fini(&lhs);
  String::bounded_fini(&rhs);
}

TYPED_TEST(StringTest, init_null_returns_false)
{
  EXPECT_FALSE(StringWrapper<TypeParam>::init(nullptr, &this->allocator));
}

TYPED_TEST(StringTest, init_from_region_null_string_returns_false)
{
  using Traits = StringTestTraits<TypeParam>;
  typename Traits::char_type storage[8] = {};
  rosidl_memory_region_t region{{storage, 0}, sizeof(storage)};
  EXPECT_FALSE(StringWrapper<TypeParam>::init_from_region(nullptr, region));
}

TYPED_TEST(StringTest, init_from_region_null_address_returns_false)
{
  rosidl_memory_region_t region{{nullptr, 0}, 8U};
  EXPECT_FALSE(StringWrapper<TypeParam>::init_from_region(&this->str, region));
}

TYPED_TEST(StringTest, fini_null_is_safe)
{
  EXPECT_NO_FATAL_FAILURE(StringWrapper<TypeParam>::fini(nullptr));
}
