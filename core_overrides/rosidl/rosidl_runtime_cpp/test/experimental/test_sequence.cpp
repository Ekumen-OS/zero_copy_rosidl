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
#include <vector>

#include "rosidl_runtime_cpp/experimental/sequence.hpp"

namespace
{

struct Widget
{
  Widget() = default;
  explicit Widget(int value_in)
  : value(value_in)
  {}

  int value{0};
};

}  // namespace

TEST(rosidl_runtime_cpp_experimental_sequence, pmr_growth_and_vector_conversion)
{
  std::byte buffer[256];
  std::pmr::monotonic_buffer_resource pool(buffer, sizeof(buffer));

  rosidl_runtime_cpp::BoundedSequence<int, 5> sequence(&pool);
  sequence.push_back(1);
  sequence.push_back(2);
  sequence.emplace_back(3);
  sequence.push_back(4);

  EXPECT_FALSE(sequence.using_fixed_storage());
  EXPECT_EQ(sequence.size(), 4u);
  EXPECT_EQ(sequence.front(), 1);
  EXPECT_EQ(sequence.back(), 4);

  std::vector<int> vector = sequence;
  EXPECT_EQ(vector, (std::vector<int>{1, 2, 3, 4}));
}

TEST(rosidl_runtime_cpp_experimental_sequence, upper_bound_is_enforced)
{
  std::byte buffer[256];
  std::pmr::monotonic_buffer_resource pool(buffer, sizeof(buffer));

  rosidl_runtime_cpp::BoundedSequence<int, 3> sequence(&pool);
  sequence.resize(3, 1);
  EXPECT_THROW(sequence.push_back(4), std::length_error);
  EXPECT_THROW(sequence.reserve(4), std::length_error);
}

TEST(rosidl_runtime_cpp_experimental_sequence, fixed_scalar_storage_capacity_is_enforced)
{
  int storage[3] = {0, 0, 0};
  rosidl_memory_region_t region;
  region.location.address = storage;
  region.location.attributes = 0;
  region.size = sizeof(storage);

  rosidl_runtime_cpp::Sequence<int> sequence(region);
  EXPECT_TRUE(sequence.using_fixed_storage());

  sequence.assign(3, 7);
  EXPECT_EQ(storage[0], 7);
  EXPECT_EQ(storage[2], 7);
  EXPECT_THROW(sequence.push_back(8), std::length_error);
}

TEST(rosidl_runtime_cpp_experimental_sequence, fixed_storage_capacity_uses_element_size)
{
  std::byte storage[3 * sizeof(int) + 1] = {};

  rosidl_memory_region_t region;
  region.location.address = storage;
  region.location.attributes = 0;
  region.size = sizeof(storage);

  rosidl_runtime_cpp::Sequence<int> sequence(region);
  sequence.assign(3, 9);
  EXPECT_EQ(sequence.size(), 3u);
  EXPECT_THROW(sequence.push_back(10), std::length_error);
}

TEST(rosidl_runtime_cpp_experimental_sequence, non_scalar_fixed_storage)
{
  alignas(Widget) std::byte storage[2 * sizeof(Widget)] = {};

  rosidl_memory_region_t region;
  region.location.address = storage;
  region.location.attributes = 0;
  region.size = sizeof(storage);

  rosidl_runtime_cpp::Sequence<Widget> sequence(region);
  sequence.emplace_back(10);
  sequence.emplace_back(20);

  EXPECT_EQ(sequence.size(), 2u);
  EXPECT_EQ(sequence[0].value, 10);
  EXPECT_EQ(sequence[1].value, 20);
}
