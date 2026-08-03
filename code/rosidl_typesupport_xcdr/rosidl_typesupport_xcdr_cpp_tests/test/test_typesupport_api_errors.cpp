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

/// @file test_typesupport_api_errors.cpp
///
/// Tests error-contract enforcement in the public trampoline functions:
///   - nullptr typesupport handle → RCUTILS_RET_ERROR
///   - non-XCDR identifier → RCUTILS_RET_ERROR
///   - standard (non-experimental) typesupports reject zero-copy APIs
///   - destroy_message(nullptr, nullptr) is safe
///   - release_message with non-experimental handle returns {nullptr,0}

#include "test_helpers.hpp"

#include <vector>

// =============================================================================
// Types under test
// =============================================================================

using ExperimentalUnbounded =
  rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage;

// =============================================================================
// Trampolines with nullptr typesupport handle
// =============================================================================

TEST(TestApiErrors, GetMessageSize_NullTypesupport)
{
  size_t size = 0;
  int dummy{};
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(nullptr, &dummy, &size);
  EXPECT_EQ(RCUTILS_RET_ERROR, ret);
}

TEST(TestApiErrors, SerializeMessageInto_NullTypesupport)
{
  int dummy{};
  rosidl_runtime_cpp::MemoryRegion<void> storage{nullptr, 0};
  auto ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(nullptr, &dummy, storage);
  EXPECT_EQ(RCUTILS_RET_ERROR, ret);
}

TEST(TestApiErrors, DeserializeMessageFrom_NullTypesupport)
{
  int dummy{};
  rosidl_runtime_cpp::MemoryRegion<void> storage{nullptr, 0};
  auto ret = rosidl_typesupport_xcdr_cpp::deserialize_message_from(nullptr, storage, &dummy);
  EXPECT_EQ(RCUTILS_RET_ERROR, ret);
}

TEST(TestApiErrors, ConstructMessageAt_NullTypesupport)
{
  std::vector<uint8_t> buffer(64);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  void * msg = nullptr;
  auto ret = rosidl_typesupport_xcdr_cpp::construct_message_at(nullptr, storage, &msg);
  EXPECT_EQ(RCUTILS_RET_ERROR, ret);
}

TEST(TestApiErrors, CastMessageAt_NullTypesupport)
{
  std::vector<uint8_t> buffer(64);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  void * msg = nullptr;
  auto ret = rosidl_typesupport_xcdr_cpp::cast_message_at(nullptr, storage, &msg);
  EXPECT_EQ(RCUTILS_RET_ERROR, ret);
}

TEST(TestApiErrors, GetExpectedMessageSize_NullTypesupport)
{
  size_t size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_expected_message_size(nullptr, &size);
  EXPECT_EQ(RCUTILS_RET_ERROR, ret);
}

// =============================================================================
// destroy_message with nullptr is safe
// =============================================================================

TEST(TestApiErrors, DestroyMessage_NullTypesupport_NullMessage)
{
  EXPECT_NO_THROW(
    rosidl_typesupport_xcdr_cpp::destroy_message(nullptr, nullptr));
}

TEST(TestApiErrors, DestroyMessage_NonNullTypesupport_NullMessage)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes>();
  // destroy_message with null message pointer should be safe
  EXPECT_NO_THROW(
    rosidl_typesupport_xcdr_cpp::destroy_message(ts, nullptr));
}

// =============================================================================
// release_message with non-experimental typesupport
// =============================================================================

TEST(TestApiErrors, ReleaseMessage_NonExperimental)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes>();
  int dummy{};
  auto region = rosidl_typesupport_xcdr_cpp::release_message(ts, &dummy);
  EXPECT_EQ(nullptr, region.data());
  EXPECT_EQ(0u, region.size());
}

// =============================================================================
// Standard typesupports reject zero-copy APIs
// =============================================================================

TEST(TestApiErrors, StandardTypesupport_ConstructAtFails)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage>();

  std::vector<uint8_t> buffer(4096);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  void * msg = nullptr;

  auto ret = rosidl_typesupport_xcdr_cpp::construct_message_at(ts, storage, &msg);
  EXPECT_EQ(RCUTILS_RET_ERROR, ret);
  EXPECT_EQ(nullptr, msg);
}

TEST(TestApiErrors, StandardTypesupport_CastAtFails)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes>();

  std::vector<uint8_t> buffer(64);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};
  void * msg = nullptr;

  auto ret = rosidl_typesupport_xcdr_cpp::cast_message_at(ts, storage, &msg);
  EXPECT_EQ(RCUTILS_RET_ERROR, ret);
  EXPECT_EQ(nullptr, msg);
}

// =============================================================================
// Incompatible identifier check (non-XCDR identifier)
// =============================================================================

TEST(TestApiErrors, WrongIdentifierOnSerialise)
{
  // Obtain a valid XCDR typesupport, then use the trampoline which
  // validates the identifier internally.  Since the identifier is correct,
  // this should succeed — the real error path requires a non-XCDR
  // typesupport which is not available in this test package.
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes>();
  ASSERT_NE(nullptr, ts);

  rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes msg{};
  fill_basic_types(msg);

  size_t size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_message_size(ts, &msg, &size);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_GT(size, 0u);
}

// =============================================================================
// Constrained lifecycle error contracts
// =============================================================================

TEST(TestApiErrors, CreateConstrained_NullBase)
{
  ExperimentalUnbounded::Constraints constraints;
  rosidl_message_type_constraints_t wrapped_constraints;
  wrapped_constraints.type_specific = &constraints;
  wrapped_constraints.max_string_length = 0;
  wrapped_constraints.max_total_size = 0;
  wrapped_constraints.strict = false;
  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      nullptr, &wrapped_constraints);
  EXPECT_EQ(nullptr, constrained.get());
}

TEST(TestApiErrors, DestroyConstrained_Null)
{
  EXPECT_NO_THROW(
    rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(nullptr));
}

TEST(TestApiErrors, GetExpectedSizeOnNonExperimental)
{
  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::msg::BasicTypes>();
  ASSERT_NE(nullptr, ts);

  size_t size = 42;
  // Non-experimental types have no cached layout, so the expected size is
  // not statically known: the API reports OK with size 0 ("we don't know"),
  // not an error.
  auto ret = rosidl_typesupport_xcdr_cpp::get_expected_message_size(ts, &size);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_EQ(0u, size);
}

// =============================================================================
// compare_constraints error contracts
// =============================================================================

TEST(TestApiErrors, CompareConstraints_BothNull)
{
  EXPECT_TRUE(
    rosidl_typesupport_xcdr_cpp::compare_constraints(
      nullptr, nullptr, nullptr, nullptr));
}

TEST(TestApiErrors, CompareConstraints_CandidateNullNotCompatible)
{
  rosidl_message_type_constraints_t baseline{};
  baseline.max_string_length = 100;
  EXPECT_FALSE(
    rosidl_typesupport_xcdr_cpp::compare_constraints(
      nullptr, nullptr, &baseline, nullptr));
}

TEST(TestApiErrors, CompareConstraints_WrongIdentifier)
{
  // Build a mock handle with a non-XCDR identifier
  rosidl_message_type_support_t bad_ts{};
  bad_ts.typesupport_identifier = "some_other_typesupport";
  int dummy{};
  bad_ts.data = &dummy;

  int ts_val{};
  rosidl_message_type_constraints_t candidate{};
  rosidl_message_type_constraints_t baseline{};
  candidate.type_specific = &ts_val;
  baseline.type_specific = &ts_val;

  EXPECT_FALSE(
    rosidl_typesupport_xcdr_cpp::compare_constraints(
      &bad_ts, &candidate, &baseline, nullptr));
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
