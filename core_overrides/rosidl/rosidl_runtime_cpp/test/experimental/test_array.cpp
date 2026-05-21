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
#include <stdexcept>

#include "rosidl_runtime_cpp/experimental/array.hpp"

TEST(rosidl_runtime_cpp_experimental_array, std_array_interop)
{
  std::array<int, 4> source{{1, 2, 3, 4}};
  rosidl_runtime_cpp::Array<int, 4> array(source);

  EXPECT_EQ(array.size(), 4u);
  EXPECT_EQ(array.front(), 1);
  EXPECT_EQ(array.back(), 4);

  std::array<int, 4> copy = array;
  EXPECT_EQ(copy, source);

  std::array<int, 4> replacement{{8, 9, 10, 11}};
  array = replacement;
  EXPECT_EQ(array[0], 8);
  EXPECT_EQ(array[3], 11);
}

TEST(rosidl_runtime_cpp_experimental_array, at_throws_out_of_range)
{
  rosidl_runtime_cpp::Array<int, 2> array;
  EXPECT_THROW(array.at(2), std::out_of_range);
}

TEST(rosidl_runtime_cpp_experimental_array, external_region_support)
{
  int storage[3] = {1, 2, 3};

  rosidl_memory_region_t region;
  region.location.address = storage;
  region.location.attributes = 0;
  region.size = sizeof(storage);

  rosidl_runtime_cpp::Array<int, 3> array(region);
  EXPECT_EQ(array[0], 1);
  EXPECT_EQ(array[1], 2);
  EXPECT_EQ(array[2], 3);

  array[1] = 20;
  EXPECT_EQ(storage[1], 20);

  auto iterator = array.begin();
  ++iterator;
  EXPECT_EQ(*iterator, 20);
  EXPECT_EQ(*(iterator + 1), 3);
}

TEST(rosidl_runtime_cpp_experimental_array, fill_and_swap)
{
  rosidl_runtime_cpp::Array<int, 3> lhs;
  rosidl_runtime_cpp::Array<int, 3> rhs;

  lhs.fill(5);
  rhs.fill(9);

  lhs.swap(rhs);
  EXPECT_EQ(lhs[0], 9);
  EXPECT_EQ(rhs[0], 5);
}
