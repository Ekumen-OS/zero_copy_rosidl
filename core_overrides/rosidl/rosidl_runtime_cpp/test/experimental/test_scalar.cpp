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

#include "rosidl_runtime_cpp/experimental/scalar.hpp"

TEST(rosidl_runtime_cpp_experimental_scalar, inline_storage_roundtrip)
{
  rosidl_runtime_cpp::Scalar<int> scalar(42);
  EXPECT_EQ(scalar.get(), 42);

  scalar = 7;
  EXPECT_EQ(static_cast<int>(scalar), 7);
}

TEST(rosidl_runtime_cpp_experimental_scalar, external_storage_aliasing)
{
  int value = 3;
  rosidl_memory_t memory;
  memory.address = &value;
  memory.attributes = 0;

  rosidl_runtime_cpp::Scalar<int> scalar(memory);
  EXPECT_EQ(scalar.get(), 3);

  scalar = 11;
  EXPECT_EQ(value, 11);

  value = 17;
  EXPECT_EQ(scalar.get(), 17);
}
