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

/// @file test_compact_traversal.cpp
///
/// Tests the three-mode compaction traversal:
///   Fast path  — all fields at constraint max → release_message (no rewrite)
///   Rewrite    — first field with actual < max → suffix rewrite from field start
///   Fail       — no external storage → null region
///
/// The compact function is invoked through
/// rosidl_typesupport_xcdr_cpp::compact_message_in_place().

#include "test_helpers.hpp"

#include <vector>

#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"

// =============================================================================
// Types under test
// =============================================================================

using ExperimentalUnbounded =
  rosidl_typesupport_xcdr_cpp_tests::msg::experimental::UnboundedMessage;

// =============================================================================
// Helper: construct a constrained message with given constraints, fill it,
// compact it, and return the compacted region.
// =============================================================================

struct CompactFixture
{
  const rosidl_message_type_support_t * base;
  std::shared_ptr<rosidl_message_type_support_t> constrained_owner;
  const rosidl_message_type_support_t * constrained;
  std::vector<uint8_t> buffer;
  rosidl_runtime_cpp::MemoryRegion<void> storage;
  void * msg_ptr;
  ExperimentalUnbounded * msg;

  CompactFixture(
    ExperimentalUnbounded::Constraints & constraints,
    size_t extra_buf = 0)
  : base(rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
      ExperimentalUnbounded>()),
    constrained_owner(nullptr),
    constrained(nullptr),
    buffer(),
    storage(nullptr, 0),
    msg_ptr(nullptr),
    msg(nullptr),
    valid_(false)
  {
    if (!base) { return; }

    rosidl_message_type_constraints_t wrapped;
    wrapped.type_specific = &constraints;
    wrapped.max_string_length = 0;
    wrapped.max_total_size = 0;
    wrapped.strict = false;

    constrained_owner =
      rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
        base, &wrapped);
    constrained = constrained_owner.get();
    if (!constrained) { return; }

    // Determine buffer size from constraints
    size_t expected_size = 0;
    auto ret = rosidl_typesupport_xcdr_cpp::get_expected_message_size(
      constrained, &expected_size);
    if (ret != RCUTILS_RET_OK || expected_size == 0) { return; }

    buffer.resize(expected_size + extra_buf);
    storage = rosidl_runtime_cpp::MemoryRegion<void>{
      buffer.data(), buffer.size()};

    // Construct message in-place (zero-copy loan)
    ret = rosidl_typesupport_xcdr_cpp::construct_message_at(
      constrained, storage, &msg_ptr);
    if (ret != RCUTILS_RET_OK || !msg_ptr) { return; }

    msg = static_cast<ExperimentalUnbounded *>(msg_ptr);
    valid_ = true;
  }

  explicit operator bool() const { return valid_; }
  bool valid() const { return valid_; }

private:
  bool valid_;

public:
  ~CompactFixture()
  {
    // constrained_owner shared_ptr handles cleanup automatically.
    // Don't destroy msg — it should have been consumed by compact or we do it
  }

  rosidl_memory_region_t compact()
  {
    // Note: after compact success, msg_ptr is consumed (deleted internally).
    // After compact failure, msg_ptr is NOT consumed.
    return rosidl_typesupport_xcdr_cpp::compact_message_in_place(
      constrained, msg_ptr);
  }
};

// =============================================================================
// Fast path — all fields at constraint max
// =============================================================================

TEST(TestCompactTraversal, AllFieldsAtMax_FastPath)
{
  // Set constraints to exactly match the fill values.
  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 9;    // "test_name" is 9 chars
  constraints.data.size = 5;    // {1,2,3,4,5} is 5 elements

  CompactFixture fx(constraints);
  EXPECT_TRUE(fx.valid());
  if (!fx.valid()) { return; }
  fill_unbounded_message(*fx.msg);

  // Compact — all fields at max, should fast-path to release_message.
  rosidl_memory_region_t region = fx.compact();

  // Should succeed with a valid blob.
  EXPECT_NE(nullptr, region.location.address);
  EXPECT_GT(region.size, 0u);

  // The blob pointer should match the original buffer (release path).
  EXPECT_EQ(region.location.address, fx.buffer.data());
}

// =============================================================================
// Rewrite path — string shorter than constraint max
// =============================================================================

TEST(TestCompactTraversal, StringShorterThanMax_Rewrite)
{
  // Set generous constraints so the message view has large slots.
  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 256;
  constraints.data.size = 100;

  CompactFixture fx(constraints);
  EXPECT_TRUE(fx.valid());
  if (!fx.valid()) { return; }
  fill_unbounded_message(*fx.msg);  // name="test_name" (9 chars), data={1,2,3,4,5} (5)

  // Capture the expected max (layout) size before compaction.
  size_t max_size = fx.buffer.size();

  // Compact — should rewrite because name is shorter than 256.
  // The rewrite starts at the name field (first variable-length field).
  rosidl_memory_region_t region = fx.compact();

  // Should succeed with a valid blob.
  EXPECT_NE(nullptr, region.location.address);
  EXPECT_GT(region.size, 0u);

  // The compacted size should be smaller than the max layout size.
  EXPECT_LT(region.size, max_size);

  // The blob should point into the original buffer.
  EXPECT_EQ(region.location.address, fx.buffer.data());
}

// =============================================================================
// Rewrite path — sequence shorter than constraint max (first field at max)
// =============================================================================

TEST(TestCompactTraversal, SecondFieldShorterThanMax_Rewrite)
{
  // Set constraints: name at exact size, data with generous max.
  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 9;    // exactly "test_name"
  constraints.data.size = 100;

  CompactFixture fx(constraints);
  EXPECT_TRUE(fx.valid());
  if (!fx.valid()) { return; }
  fill_unbounded_message(*fx.msg);  // name=9 chars (at max), data=5 elements

  size_t max_size = fx.buffer.size();

  // Compact — first field (name) matches, second field (data) is shorter.
  // Rewrite starts at the data field.
  rosidl_memory_region_t region = fx.compact();

  EXPECT_NE(nullptr, region.location.address);
  EXPECT_GT(region.size, 0u);
  EXPECT_LT(region.size, max_size);
  EXPECT_EQ(region.location.address, fx.buffer.data());
}

// =============================================================================
// Fail path — no external storage (stack-allocated message)
// =============================================================================

TEST(TestCompactTraversal, NoExternalStorage_Fails)
{
  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 256;
  constraints.data.size = 100;

  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);

  rosidl_message_type_constraints_t wrapped_const;
  wrapped_const.type_specific = &constraints;
  wrapped_const.max_string_length = 0;
  wrapped_const.max_total_size = 0;
  wrapped_const.strict = false;

  auto constrained_owner =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, &wrapped_const);
  auto * constrained = constrained_owner.get();
  ASSERT_NE(nullptr, constrained);

  // Create a stack-allocated message (no external storage).
  ExperimentalUnbounded msg;
  fill_unbounded_message(msg);

  // Compact should return null region because there's no external storage.
  rosidl_memory_region_t region =
    rosidl_typesupport_xcdr_cpp::compact_message_in_place(
      constrained, &msg);
  EXPECT_EQ(nullptr, region.location.address);
  EXPECT_EQ(0u, region.size);

  // Message should NOT be consumed (caller still owns it).
  EXPECT_EQ(msg.name.size(), 9u);
}

// =============================================================================
// Fail path — nullptr message
// =============================================================================

TEST(TestCompactTraversal, NullptrMessage_Fails)
{
  ExperimentalUnbounded::Constraints constraints;
  constraints.name.size = 256;
  constraints.data.size = 100;

  auto base = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalUnbounded>();
  ASSERT_NE(nullptr, base);

  rosidl_message_type_constraints_t wrapped_const;
  wrapped_const.type_specific = &constraints;
  wrapped_const.max_string_length = 0;
  wrapped_const.max_total_size = 0;
  wrapped_const.strict = false;

  auto constrained_owner =
    rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
      base, &wrapped_const);
  auto * constrained = constrained_owner.get();
  ASSERT_NE(nullptr, constrained);

  rosidl_memory_region_t region =
    rosidl_typesupport_xcdr_cpp::compact_message_in_place(
      constrained, nullptr);
  EXPECT_EQ(nullptr, region.location.address);
  EXPECT_EQ(0u, region.size);
}

// =============================================================================
// Fully bounded experimental message — always fast path
// =============================================================================

TEST(TestCompactTraversal, FullyBounded_FastPath)
{
  using ExperimentalBounded =
    rosidl_typesupport_xcdr_cpp_tests::msg::experimental::BoundedMessage;

  auto ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<
    ExperimentalBounded>();
  ASSERT_NE(nullptr, ts);

  ExperimentalBounded msg;
  fill_bounded_message(msg);

  // Serialize into buffer (so compact has external storage)
  std::vector<uint8_t> buffer(4096);
  rosidl_runtime_cpp::MemoryRegion<void> storage{buffer.data(), buffer.size()};

  auto ret = rosidl_typesupport_xcdr_cpp::serialize_message_into(ts, &msg, storage);
  ASSERT_EQ(RCUTILS_RET_OK, ret);

  // Compact on a non-constructed message: fails because no external storage.
  // For fully bounded experimental messages, compact calls release_message
  // which returns the block storage if external storage exists.
  // Since this message was serialized (not constructed), there's no external
  // storage block, so compact should return null region.
  rosidl_memory_region_t region =
    rosidl_typesupport_xcdr_cpp::compact_message_in_place(ts, &msg);
  EXPECT_EQ(nullptr, region.location.address);
  EXPECT_EQ(0u, region.size);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
