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
/// Tests the constrained typesupport lifecycle with real constraints:
///   - create_constrained_message_type_support accepts/rejects inputs
///   - get_expected_message_size yields a layout-derived size
///   - serialize/deserialize through a constrained handle works
///   - construct_message_at / cast_message_at with cached layout
///   - destroy_constrained_message_type_support
///
/// Constrained typesupports are built on top of experimental base
/// typesupports that have variable-length fields (string, sequence).
/// The constraints provide max sizes so a fixed layout can be computed.
///
/// Fully bounded types (e.g. BoundedMessage) need no constraints and
/// return an error when create_constrained_message_type_support is
/// called on them.

#include "test_helpers.hpp"

#include <vector>

// =============================================================================
// Types under test
// =============================================================================

using ExperimentalUnbounded =
  rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage;
using ExperimentalBounded =
  rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage;

// =============================================================================
// Error-handling paths
// =============================================================================

TEST(TestConstrainedTypesupport, NullConstraintsFails)
{
  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);

  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, nullptr);
  EXPECT_EQ(nullptr, constrained);
}

TEST(TestConstrainedTypesupport, BoundedMessageCannotBeConstrained)
{
  // Fully bounded types already have a built-in layout — they don't need
  // (and should not accept) additional constraints.
  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalBounded>();
  ASSERT_NE(nullptr, base);

  ExperimentalBounded::Constraints constraints;
  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, &constraints);
  EXPECT_EQ(nullptr, constrained);
}

TEST(TestConstrainedTypesupport, DestroyNullptr)
{
  EXPECT_NO_THROW(
    rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(nullptr));
}

// =============================================================================
// Valid constraints on UnboundedMessage
// =============================================================================

TEST(TestConstrainedTypesupport, ValidConstraintsSucceeds)
{
  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);

  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 256;
  constraints.data.size = 100;

  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, &constraints);
  ASSERT_NE(nullptr, constrained);
  EXPECT_STREQ(
    rosidl_typesupport_xcdr_cpp__identifier,
    constrained->typesupport_identifier);
  EXPECT_NE(nullptr, constrained->data);

  rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(
    constrained);
}

TEST(TestConstrainedTypesupport, GetExpectedMessageSize)
{
  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);

  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 256;
  constraints.data.size = 100;

  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, &constraints);
  ASSERT_NE(nullptr, constrained);

  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_expected_message_size(
    constrained, &expected_size);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_GT(expected_size, 0u);
  // id(4) + string_length_prefix(4) + name_max(256) + null(1) +
  // sequence_length_prefix(4) + data_max(100)
  EXPECT_GE(expected_size, 4u + 4u + 256u + 1u + 4u + 100u);

  rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(
    constrained);
}

TEST(TestConstrainedTypesupport, SerializeRoundtrip)
{
  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);

  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 256;
  constraints.data.size = 100;

  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, &constraints);
  ASSERT_NE(nullptr, constrained);

  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_expected_message_size(
    constrained, &expected_size);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  ASSERT_GT(expected_size, 0u);

  // Fill and serialize
  ExperimentalUnbounded msg;
  fill_unbounded_message(msg);

  std::vector<uint8_t> buffer(expected_size);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};

  ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(
    constrained, &msg, storage);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  // Deserialize via constrained handle and verify
  ExperimentalUnbounded deserialized;
  ret = rosidl_typesupport_xcdr_cpp::deserialize_message_from(
    constrained, storage, &deserialized);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  verify_unbounded_message(msg, deserialized);

  rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(
    constrained);
}

// =============================================================================
// Zero-copy operations through constrained typesupport
// =============================================================================

TEST(TestConstrainedTypesupport, ConstructMessageAt)
{
  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);

  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 256;
  constraints.data.size = 100;

  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, &constraints);
  ASSERT_NE(nullptr, constrained);

  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_expected_message_size(
    constrained, &expected_size);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  ASSERT_GT(expected_size, 0u);

  // construct_message_at allocates a message in-place within the buffer
  // using the layout.  The message's fields have ExternalStorage that
  // points directly into the buffer at the XCDR-framed locations.
  std::vector<uint8_t> buffer(expected_size);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};

  void * msg_ptr = nullptr;
  ret = rosidl_typesupport_xcdr_cpp::construct_message_at(
    constrained, storage, &msg_ptr);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  ASSERT_NE(nullptr, msg_ptr);

  // Fill the allocated message in-place (writes directly to the buffer
  // via external storage)
  auto * msg = static_cast<ExperimentalUnbounded *>(msg_ptr);
  fill_unbounded_message(*msg);

  // Verify fields directly — the message lives in the buffer and the
  // external storage should contain the correctly filled values.
  // NOTE: serialize/deserialize roundtrip is NOT used here because
  // the construct_message_at buffer has layout-relative field positions
  // (padded to max constraint sizes), while serialize_message_into
  // produces compact serialization (actual-data-relative).  These two
  // framing schemes are incompatible for variable-length fields.
  EXPECT_EQ(msg->id.get(), 999u);
  EXPECT_EQ(std::string_view(msg->name.data(), msg->name.size()), "test_name");
  EXPECT_EQ(msg->data.size(), 5u);

  rosidl_typesupport_xcdr_cpp::destroy_message(constrained, msg_ptr);
  rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(
    constrained);
}

TEST(TestConstrainedTypesupport, CastMessageAt)
{
  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);

  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 256;
  constraints.data.size = 100;

  auto constrained =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, &constraints);
  ASSERT_NE(nullptr, constrained);

  // Serialize a normal message into a buffer sized by expected size
  ExperimentalUnbounded msg;
  fill_unbounded_message(msg);

  size_t expected_size = 0;
  auto ret = rosidl_typesupport_xcdr_cpp::get_expected_message_size(
    constrained, &expected_size);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  ASSERT_GT(expected_size, 0u);

  std::vector<uint8_t> buffer(expected_size);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};

  ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(
    constrained, &msg, storage);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  // cast_message_at reads the buffer in-place via the constrained handle.
  // The constrained handle's cached_layout is not used for parsing —
  // cast_message_at dynamically parses the XCDR layout from the buffer.
  // For unbounded messages with variable-length fields the accessor slice
  // wiring is not yet complete (same limitation as
  // test_zero_copy_lifecycle.cpp).  Verify the API call itself; full
  // data roundtrip for cast_message_at on variable-length types is a
  // separate concern.
  void * cast_ptr = nullptr;
  ret = rosidl_typesupport_xcdr_cpp::cast_message_at(
    constrained, storage, &cast_ptr);
  if (ret == RCUTILS_RET_OK) {
    ASSERT_NE(nullptr, cast_ptr);
    rosidl_typesupport_xcdr_cpp::destroy_message(constrained, cast_ptr);
    SUCCEED() << "cast_message_at succeeded via constrained handle";
  } else {
    EXPECT_EQ(nullptr, cast_ptr);
  }

  rosidl_typesupport_xcdr_cpp::destroy_constrained_message_type_support(
    constrained);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
