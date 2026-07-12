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

/// @file test_outer_callback_integration.cpp
///
/// Tests the outer+inner callback integration in rosidl_typesupport_xcdr_cpp:
///   - Constrained handles created through the outer+inner model
///   - C trampolines work on C++ generated handles
///   - Zero-copy lifecycle through constrained handles

#include "test_helpers.hpp"

#include <vector>

#include "rosidl_typesupport_xcdr_c/identifier.h"
#include "rosidl_typesupport_xcdr_c/message_type_support.h"

// =============================================================================
// Types under test
// =============================================================================

using ExperimentalUnbounded =
  rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage;
using ExperimentalBounded =
  rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage;

// =============================================================================
// Serialization through new-style handles
// =============================================================================

TEST(TestOuterCallbackIntegration, SerializeThroughConstrainedHandle)
{
  auto old_handle = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, old_handle);

  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 256;
  constraints.data.size = 100;

  rosidl_message_type_constraints_t wrapped_constraints;
  wrapped_constraints.type_specific = &constraints;
  wrapped_constraints.max_string_length = 0;
  wrapped_constraints.max_total_size = 0;
  wrapped_constraints.strict = false;
  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      old_handle, &wrapped_constraints);
  ASSERT_NE(nullptr, constrained.get());
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier,
    constrained->typesupport_identifier);

  // Roundtrip serialization through the constrained handle.
  ExperimentalUnbounded original;
  fill_unbounded_message(original);

  std::vector<uint8_t> buffer(4096);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};

  auto ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(
    constrained.get(), &original, storage);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  ExperimentalUnbounded deserialized;
  ret = rosidl_typesupport_xcdr_cpp::deserialize_message_from(
    constrained.get(), storage, &deserialized);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  verify_unbounded_message(original, deserialized);

}

// =============================================================================
// C trampolines through generated handles
// =============================================================================

TEST(TestOuterCallbackIntegration, CppTrampolineOnGeneratedHandle)
{
  auto handle = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalBounded>();
  ASSERT_NE(nullptr, handle);

  // Verify the outer struct is accessible through ts->data.
  auto * outer = static_cast<const rosidl_message_xcdr_type_support_t *>(
    handle->data);
  ASSERT_NE(nullptr, outer);
  EXPECT_NE(nullptr, outer->get_message_size);

  ExperimentalBounded msg;
  fill_bounded_message(msg);

  size_t msg_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(
    handle, &msg, &msg_size);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
}

// =============================================================================
// Zero-copy lifecycle through constrained handles
// =============================================================================

TEST(TestOuterCallbackIntegration, ConstructAndCastThroughConstrained)
{
  auto old_handle = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, old_handle);

  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 256;
  constraints.data.size = 100;

  rosidl_message_type_constraints_t wrapped_constraints;
  wrapped_constraints.type_specific = &constraints;
  wrapped_constraints.max_string_length = 0;
  wrapped_constraints.max_total_size = 0;
  wrapped_constraints.strict = false;
  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      old_handle, &wrapped_constraints);
  ASSERT_NE(nullptr, constrained.get());
  auto constrained_ptr = constrained.get();

  // Get expected size from layout.
  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_expected_message_size(
    constrained_ptr, &expected_size);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_GT(expected_size, 0u);

  // Construct message at storage (zero-copy sender side).
  std::vector<uint8_t> buffer(expected_size);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  void * constructed = nullptr;
  ret = rosidl_typesupport_xcdr_cpp::construct_message_at(
    constrained.get(), storage, &constructed);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  ASSERT_NE(nullptr, constructed);

  // Fill and serialize.
  auto * typed = static_cast<ExperimentalUnbounded *>(constructed);
  fill_unbounded_message(*typed);
  ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(
    constrained.get(), constructed, storage);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  // Cast from storage (zero-copy receiver side).
  void * casted = nullptr;
  ret = rosidl_typesupport_xcdr_cpp::cast_message_at(
    constrained.get(), storage, &casted);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  ASSERT_NE(nullptr, casted);

  auto * typed_casted = static_cast<ExperimentalUnbounded *>(casted);
  verify_unbounded_message(*typed, *typed_casted);

  // Clean up.
  rosidl_typesupport_xcdr_cpp::destroy_message(constrained.get(), constructed);
  rosidl_typesupport_xcdr_cpp::destroy_message(constrained.get(), casted);
}
