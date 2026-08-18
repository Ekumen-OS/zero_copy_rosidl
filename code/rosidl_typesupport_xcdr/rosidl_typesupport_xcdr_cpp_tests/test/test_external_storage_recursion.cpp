// Copyright 2026 Ekumen Inc.
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

/// @file test_external_storage_recursion.cpp
///
/// Tests that populate_external_storage_<Msg> correctly and recursively
/// populates ExternalStorage members for nested types.

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <vector>

#include "tcb_span/span.hpp"
#include "xcdr_buffers/accessor/const_accessor.hpp"
#include "xcdr_buffers/layout/layout.hpp"
#include "xcdr_buffers/layout/layout_builder.hpp"

#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_cpp/experimental/memory.hpp"

#include "rosidl_typesupport_xcdr_cpp_tests/msg/experimental/inner_value.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/experimental/nested_containers.hpp"

// ============================================================================
// Typedefs
// ============================================================================

using InnerValue = rosidl_typesupport_xcdr_cpp_tests::msg::experimental::InnerValue;
using NestedContainers = rosidl_typesupport_xcdr_cpp_tests::msg::experimental::NestedContainers;

// Forward-declare the per-type populate functions (defined in generated .cpp).
namespace rosidl_typesupport_xcdr_cpp_tests::msg::experimental
{
rcutils_ret_t populate_external_storage_InnerValue(
  const xcdr_buffers::XCdrConstAccessor & accessor, void * ext_storage_ptr);
rcutils_ret_t populate_external_storage_NestedContainers(
  const xcdr_buffers::XCdrConstAccessor & accessor, void * ext_storage_ptr);
}  // namespace rosidl_typesupport_xcdr_cpp_tests::msg::experimental

// ============================================================================
// Helper: obtain a struct layout for any message type
// ============================================================================

static std::shared_ptr<xcdr_buffers::XCdrStructLayout> get_shared_layout(
  const rosidl_message_type_support_t * ts)
{
  auto * outer = static_cast<const rosidl_message_xcdr_type_support_t *>(ts->data);
  auto * inner =
    static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(
    outer->inner);

  if (inner->cached_layout) {return inner->cached_layout;}

  // Constrained message: build from constraints.
  EXPECT_NE(nullptr, inner->build_constrained);
  NestedContainers::Constraints dummy{};
  auto layout = inner->build_constrained(&dummy);
  EXPECT_NE(nullptr, layout);
  return layout;
}

// ============================================================================
// Test 1: Leaf message (no nesting)
// ============================================================================

TEST(TestExternalStorageRecursion, InnerValue)
{
  auto * ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<InnerValue>();
  ASSERT_NE(nullptr, ts);
  auto layout_ptr = get_shared_layout(ts);
  const auto & layout = *layout_ptr;

  std::vector<uint8_t> buf(layout.total_size(), 0);
  layout.apply(tcb::span<uint8_t>(buf.data(), buf.size()));

  auto acc_result = xcdr_buffers::XCdrConstAccessor::wrap(
    tcb::span<const uint8_t>(buf.data(), buf.size()), layout);
  ASSERT_TRUE(acc_result);
  auto accessor = *acc_result;

  InnerValue::ExternalStorage ext;
  ext.block = rosidl_runtime_cpp::MemoryRegion<void>(buf.data(), buf.size());
  auto ret =
    rosidl_typesupport_xcdr_cpp_tests::msg::experimental::populate_external_storage_InnerValue(
    accessor, &ext);
  EXPECT_EQ(RCUTILS_RET_OK, ret);

  EXPECT_NE(nullptr, ext.members.id.data());
  EXPECT_NE(nullptr, ext.members.value.data());
  EXPECT_EQ(buf.data(), ext.block.data());
}

// ============================================================================
// Test 2: Single nested message (NamespacedType) via populate
// ============================================================================

TEST(TestExternalStorageRecursion, SingleNested)
{
  auto * ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<NestedContainers>();
  ASSERT_NE(nullptr, ts);
  auto layout_ptr = get_shared_layout(ts);
  const auto & layout = *layout_ptr;

  std::vector<uint8_t> buf(layout.total_size(), 0);
  layout.apply(tcb::span<uint8_t>(buf.data(), buf.size()));

  auto acc_result = xcdr_buffers::XCdrConstAccessor::wrap(
    tcb::span<const uint8_t>(buf.data(), buf.size()), layout);
  ASSERT_TRUE(acc_result);
  auto accessor = *acc_result;

  // Verify accessor navigation works for member 0
  auto m0 = accessor[0];  // single (InnerValue)
  ASSERT_NO_THROW(m0[0]);  // id
  ASSERT_NO_THROW(m0[1]);  // value

  NestedContainers::ExternalStorage ext;
  ext.block = rosidl_runtime_cpp::MemoryRegion<void>(buf.data(), buf.size());
  auto ret =
    rosidl_typesupport_xcdr_cpp_tests::msg::experimental::populate_external_storage_NestedContainers
    (
    accessor, &ext);
  EXPECT_EQ(RCUTILS_RET_OK, ret);

  // Verify nested fields
  EXPECT_NE(nullptr, ext.members.single.members.id.data());
  EXPECT_NE(nullptr, ext.members.single.members.value.data());
  EXPECT_EQ(buf.data(), ext.block.data());
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
