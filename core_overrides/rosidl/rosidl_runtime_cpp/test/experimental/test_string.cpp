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

#include <cstddef>
#include <memory_resource>
#include <stdexcept>
#include <string>

#include "rosidl_runtime_cpp/experimental/string.hpp"

TEST(rosidl_runtime_cpp_experimental_string, null_termination_and_basic_operations)
{
  rosidl_runtime_cpp::BoundedString<12> string;
  string.assign("ros");
  string.push_back('2');
  string.append("!");

  EXPECT_EQ(string.size(), 5u);
  EXPECT_STREQ(string.c_str(), "ros2!");

  string.resize(3);
  EXPECT_STREQ(string.c_str(), "ros");

  string.clear();
  EXPECT_TRUE(string.empty());
  EXPECT_STREQ(string.c_str(), "");
}

TEST(rosidl_runtime_cpp_experimental_string, upper_bound_is_enforced)
{
  rosidl_runtime_cpp::BoundedString<3> string;
  string.assign("abc");
  EXPECT_THROW(string.push_back('d'), std::length_error);
  EXPECT_THROW(string.reserve(4), std::length_error);
}

TEST(rosidl_runtime_cpp_experimental_string, fixed_storage_capacity_is_enforced)
{
  char storage[4] = {};
  rosidl_memory_region_t region;
  region.location.address = storage;
  region.location.attributes = 0;
  region.size = sizeof(storage);

  rosidl_runtime_cpp::String string(region);
  string.assign("abc");
  EXPECT_STREQ(string.c_str(), "abc");
  EXPECT_THROW(string.push_back('d'), std::length_error);
}

TEST(rosidl_runtime_cpp_experimental_string, supports_wide_characters)
{
  char16_t storage[6] = {};
  rosidl_memory_region_t region;
  region.location.address = storage;
  region.location.attributes = 0;
  region.size = sizeof(storage);

  rosidl_runtime_cpp::WString string(region);
  string.assign(u"hi");
  string.append(u"!");

  EXPECT_EQ(std::u16string(string), u"hi!");
  EXPECT_EQ(storage[3], u'\0');
}

TEST(rosidl_runtime_cpp_experimental_string, empty_region_throws)
{
  char storage[8] = {};
  rosidl_memory_region_t region;
  region.location.address = storage;
  region.location.attributes = 0;
  region.size = 0;

  EXPECT_THROW((rosidl_runtime_cpp::String(region)), std::invalid_argument);
}

TEST(rosidl_runtime_cpp_experimental_string, pmr_backed_growth)
{
  std::byte buffer[256];
  std::pmr::monotonic_buffer_resource pool(buffer, sizeof(buffer));

  rosidl_runtime_cpp::String string(&pool);
  string.assign("0123456789");
  string.append("abcdefghij");

  EXPECT_EQ(string.size(), 20u);
  EXPECT_EQ(string.view(), std::string_view("0123456789abcdefghij"));
}

TEST(rosidl_runtime_cpp_experimental_string, basic_string_supports_other_code_units)
{
  rosidl_runtime_cpp::BasicString<char16_t, 8> string;
  string.assign(u"abc");
  string.push_back(u'd');

  EXPECT_EQ(string.size(), 4u);
  EXPECT_EQ(std::u16string(string), u"abcd");
}
