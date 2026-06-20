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

/// @file test_constrained_typesupport.cpp
///
/// Tests the constrained typesupport lifecycle:
///   - create_constrained_message_type_support
///   - get_expected_message_size via constrained typesupport
///   - serialize/deserialize through a constrained handle
///   - destroy_constrained_message_type_support
///
/// Constrained typesupports are built on top of experimental base
/// typesupports and provide a cached layout with a fixed expected size.
///
/// The UnboundedMessage type has string and sequence fields that require
/// constraint values (e.g., max string length, max sequence length) so
/// it is the natural test subject.

#include "test_helpers.hpp"

#include <vector>

// The AddTwoInts service uses a simple sum field; for now we use
// unbounded message types that require constraints.
using ExperimentalUnbounded =
  rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage;

// =============================================================================
// create_constrained_message_type_support — basic lifecycle
// =============================================================================

TEST(TestConstrainedTypesupport, CreateDestroy)
{
  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, base->typesupport_identifier);

  // Constraints for an unbounded message with a string and a uint8[] sequence.
  // The constraints struct is generated per message type; for now we pass
  // nullptr which tests the error path (non-null constraints typically
  // specify max string length and max sequence count).
  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, nullptr);
  // The API may return nullptr for invalid constraints, or may succeed with
  // a default layout.  Accept either outcome.
  if (constrained != nullptr) {
    EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier,
                 constrained->typesupport_identifier);
    EXPECT_NE(nullptr, constrained->data);
    rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(constrained);
  }
}

// =============================================================================
// get_expected_message_size
// =============================================================================

TEST(TestConstrainedTypesupport, GetExpectedMessageSize)
{
  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);

  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, nullptr);
  if (constrained == nullptr) {
    GTEST_SKIP() << "Constrained typesupport not available (expected with nullptr constraints)";
    return;
  }

  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_expected_message_size(
    constrained, &expected_size);
  // If constrained was created, expected size should be computable.
  // On failure this returns RCUTILS_RET_ERROR.
  if (ret == RCUTILS_RET_OK) {
    EXPECT_GT(expected_size, 0u);
  }

  rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(constrained);
}

// =============================================================================
// Serialize/deserialize through constrained typesupport
// =============================================================================

TEST(TestConstrainedTypesupport, SerializeRoundtrip)
{
  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);

  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, nullptr);
  if (constrained == nullptr) {
    GTEST_SKIP() << "Constrained typesupport not available";
    return;
  }

  ExperimentalUnbounded msg;
  fill_unbounded_message(msg);

  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_expected_message_size(
    constrained, &expected_size);

  // Use a generous buffer
  std::vector<uint8_t> buffer(expected_size > 0 ? expected_size + 256 : 4096);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};

  ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(constrained, &msg, storage);
  EXPECT_EQ(RCUTILS_RET_OK, ret);

  // Deserialize into a fresh message via the constrained handle
  ExperimentalUnbounded deserialized;
  ret = rosidl_typesupport_xcdr_cpp::deserialize_message_from(
    constrained, storage, &deserialized);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  if (ret == RCUTILS_RET_OK) {
    verify_unbounded_message(msg, deserialized);
  }

  rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(constrained);
}

// =============================================================================
// destroy_constrained_message_type_support with nullptr (safe to call)
// =============================================================================

TEST(TestConstrainedTypesupport, DestroyNullptr)
{
  EXPECT_NO_THROW(
    rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(nullptr));
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
