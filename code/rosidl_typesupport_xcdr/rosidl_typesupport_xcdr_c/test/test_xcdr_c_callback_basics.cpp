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

/// @file test_xcdr_c_callback_basics.cpp
///
/// Tests the outer C callback ABI layer provided by rosidl_typesupport_xcdr_c:
///   - identifier definition
///   - is_valid_handle contract
///   - trampoline null/identifier checking
///   - trampoline dispatch through mock outer callback instances
///   - constraints comparison
///   - constrained handle lifecycle callbacks

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

#include "rcutils/error_handling.h"

#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/experimental/memory.h"

#include "rosidl_typesupport_xcdr_c/identifier.h"
#include "rosidl_typesupport_xcdr_c/message_type_support.h"

// =============================================================================
// Helpers
// =============================================================================

/// Minimal mock outer callback instance that records which function was called.
static int last_called = -1;  // 0=get_expected_size, 1=get_message_size, ...

static rcutils_ret_t
mock_get_expected_size(
  const rosidl_message_xcdr_type_support_t * outer, size_t * size)
{
  (void)outer;
  last_called = 0;
  *size = 42;
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
mock_get_message_size(
  const rosidl_message_xcdr_type_support_t * outer,
  const void * msg, size_t * size)
{
  (void)outer;
  (void)msg;
  last_called = 1;
  *size = 42;
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
mock_construct_at(
  const rosidl_message_xcdr_type_support_t * outer,
  rosidl_memory_region_t storage, void ** msg)
{
  (void)outer;
  (void)storage;
  last_called = 2;
  *msg = reinterpret_cast<void *>(0xDEAD);
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
mock_cast_at(
  const rosidl_message_xcdr_type_support_t * outer,
  rosidl_memory_region_t storage, void ** msg)
{
  (void)outer;
  (void)storage;
  last_called = 3;
  *msg = reinterpret_cast<void *>(0xDEAD);
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
mock_serialize(
  const rosidl_message_xcdr_type_support_t * outer,
  const void * msg, rosidl_memory_region_t storage)
{
  (void)outer;
  (void)msg;
  (void)storage;
  last_called = 4;
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
mock_deserialize(
  const rosidl_message_xcdr_type_support_t * outer,
  rosidl_memory_region_t storage, void * msg)
{
  (void)outer;
  (void)storage;
  (void)msg;
  last_called = 5;
  return RCUTILS_RET_OK;
}

static void
mock_destroy_message(void * msg)
{
  (void)msg;
  last_called = 6;
}

static rosidl_memory_region_t
mock_release_message(void * msg)
{
  (void)msg;
  last_called = 7;
  return rosidl_memory_region_t{{reinterpret_cast<void *>(0xCAFE), 0}, 100};
}

static rosidl_message_type_support_t *
mock_create_constrained(
  const rosidl_message_xcdr_type_support_t * outer,
  const rosidl_message_type_constraints_t * constraints)
{
  (void)outer;
  (void)constraints;
  last_called = 8;
  // Return a statically "allocated" handle whose data points back to the
  // same outer callback table that was used to create it.
  static rosidl_message_type_support_t dummy;
  dummy.typesupport_identifier = rosidl_typesupport_xcdr_c__identifier;
  dummy.data = outer;
  return &dummy;
}

static void
mock_destroy_constrained(rosidl_message_type_support_t * ts)
{
  (void)ts;
  last_called = 9;
}

static bool
mock_compare_ts(const void * lhs, const void * rhs)
{
  (void)lhs;
  (void)rhs;
  last_called = 10;
  return true;  // compatible
}

static rcutils_ret_t
mock_validate_message(
  const rosidl_message_xcdr_type_support_t * outer,
  const rosidl_message_type_constraints_t * constraints,
  const void * message,
  rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
  void * user_data)
{
  (void)outer;
  (void)constraints;
  (void)message;
  (void)report_cb;
  (void)user_data;
  last_called = 11;
  return RCUTILS_RET_OK;
}

static void
mock_destroy_inner(const rosidl_message_xcdr_type_support_t * outer)
{
  (void)outer;
  last_called = 12;
}

static const rosidl_message_type_constraints_t *
mock_get_constraints(const rosidl_message_xcdr_type_support_t * outer)
{
  (void)outer;
  return nullptr;  // statically generated, no owned constraints
}

/// Build a mock outer callback table wired to the mocks above.
static rosidl_message_xcdr_type_support_t mock_outer = {
  nullptr,                              // inner (tests override as needed)
  mock_get_expected_size,
  mock_get_message_size,
  mock_construct_at,
  mock_cast_at,
  mock_serialize,
  mock_deserialize,
  mock_destroy_message,
  mock_release_message,
  mock_create_constrained,
  mock_destroy_constrained,
  mock_compare_ts,
  mock_validate_message,
  mock_get_constraints,
  mock_destroy_inner,
};

/// Build a minimal valid rosidl_message_type_support_t wrapping mock_outer.
static rosidl_message_type_support_t make_mock_handle()
{
  rosidl_message_type_support_t ts{};
  ts.typesupport_identifier = rosidl_typesupport_xcdr_c__identifier;
  ts.data = &mock_outer;
  return ts;
}

/// Build a handle with a wrong identifier.
static rosidl_message_type_support_t make_wrong_id_handle()
{
  rosidl_message_type_support_t ts{};
  ts.typesupport_identifier = "some_other_typesupport";
  ts.data = &mock_outer;
  return ts;
}

// =============================================================================
// Identifier
// =============================================================================

TEST(TestXcdrCIdentifier, IsDefined)
{
  ASSERT_NE(nullptr, rosidl_typesupport_xcdr_c__identifier);
  EXPECT_STRNE("", rosidl_typesupport_xcdr_c__identifier);
}

// =============================================================================
// is_valid_handle
// =============================================================================

TEST(TestXcdrCIsValidHandle, NullHandle)
{
  EXPECT_FALSE(rosidl_typesupport_xcdr_c_is_valid_handle(nullptr));
}

TEST(TestXcdrCIsValidHandle, WrongIdentifier)
{
  auto ts = make_wrong_id_handle();
  EXPECT_FALSE(rosidl_typesupport_xcdr_c_is_valid_handle(&ts));
}

TEST(TestXcdrCIsValidHandle, NullData)
{
  rosidl_message_type_support_t ts{};
  ts.typesupport_identifier = rosidl_typesupport_xcdr_c__identifier;
  ts.data = nullptr;
  EXPECT_FALSE(rosidl_typesupport_xcdr_c_is_valid_handle(&ts));
}

TEST(TestXcdrCIsValidHandle, Valid)
{
  auto ts = make_mock_handle();
  EXPECT_TRUE(rosidl_typesupport_xcdr_c_is_valid_handle(&ts));
}

// =============================================================================
// Trampolines — null and wrong-identifier rejection
// =============================================================================

TEST(TestXcdrCTrampolines, GetExpectedSize_NullHandle)
{
  size_t size = 0;
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_get_expected_size(nullptr, &size));
}

TEST(TestXcdrCTrampolines, GetExpectedSize_WrongIdentifier)
{
  auto ts = make_wrong_id_handle();
  size_t size = 0;
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_get_expected_size(&ts, &size));
}

TEST(TestXcdrCTrampolines, GetMessageSize_NullHandle)
{
  int dummy{};
  size_t size = 0;
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_get_message_size(nullptr, &dummy, &size));
}

TEST(TestXcdrCTrampolines, SerializeMessageInto_NullHandle)
{
  int dummy{};
  rosidl_memory_region_t storage{{nullptr, 0}, 0};
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_serialize_message_into(nullptr, &dummy, storage));
}

TEST(TestXcdrCTrampolines, DeserializeMessageFrom_NullHandle)
{
  int dummy{};
  rosidl_memory_region_t storage{{nullptr, 0}, 0};
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_deserialize_message_from(nullptr, storage, &dummy));
}

TEST(TestXcdrCTrampolines, ConstructMessageAt_NullHandle)
{
  std::vector<uint8_t> buf(64);
  rosidl_memory_region_t storage{{buf.data(), 0}, buf.size()};
  void * msg = nullptr;
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_construct_message_at(nullptr, storage, &msg));
}

TEST(TestXcdrCTrampolines, CastMessageAt_NullHandle)
{
  std::vector<uint8_t> buf(64);
  rosidl_memory_region_t storage{{buf.data(), 0}, buf.size()};
  void * msg = nullptr;
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_cast_message_at(nullptr, storage, &msg));
}

TEST(TestXcdrCTrampolines, DestroyMessage_NullSafe)
{
  // destroy_message(NULL, NULL) must not crash.
  rosidl_typesupport_xcdr_c_destroy_message(nullptr, nullptr);
  SUCCEED();
}

TEST(TestXcdrCTrampolines, ReleaseMessage_NullHandle)
{
  auto region = rosidl_typesupport_xcdr_c_release_message(nullptr, nullptr);
  EXPECT_EQ(nullptr, region.location.address);
  EXPECT_EQ(0u, region.size);
}

// =============================================================================
// Trampolines — dispatch through mock outer
// =============================================================================

class TestXcdrCMockDispatch : public ::testing::Test
{
protected:
  void SetUp() override
  {
    last_called = -1;
    rcutils_reset_error();
  }

  rosidl_message_type_support_t handle_ = make_mock_handle();
};

TEST_F(TestXcdrCMockDispatch, GetExpectedSize)
{
  size_t size = 0;
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_c_get_expected_size(&handle_, &size));
  EXPECT_EQ(42u, size);
  EXPECT_EQ(0, last_called);
}

TEST_F(TestXcdrCMockDispatch, GetMessageSize)
{
  int dummy{};
  size_t size = 0;
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_c_get_message_size(&handle_, &dummy, &size));
  EXPECT_EQ(42u, size);
  EXPECT_EQ(1, last_called);
}

TEST_F(TestXcdrCMockDispatch, ConstructMessageAt)
{
  std::vector<uint8_t> buf(64);
  rosidl_memory_region_t storage{{buf.data(), 0}, buf.size()};
  void * msg = nullptr;
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_c_construct_message_at(&handle_, storage, &msg));
  EXPECT_EQ(reinterpret_cast<void *>(0xDEAD), msg);
  EXPECT_EQ(2, last_called);
}

TEST_F(TestXcdrCMockDispatch, CastMessageAt)
{
  std::vector<uint8_t> buf(64);
  rosidl_memory_region_t storage{{buf.data(), 0}, buf.size()};
  void * msg = nullptr;
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_c_cast_message_at(&handle_, storage, &msg));
  EXPECT_EQ(reinterpret_cast<void *>(0xDEAD), msg);
  EXPECT_EQ(3, last_called);
}

TEST_F(TestXcdrCMockDispatch, SerializeMessageInto)
{
  int dummy{};
  rosidl_memory_region_t storage{{nullptr, 0}, 0};
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_c_serialize_message_into(&handle_, &dummy, storage));
  EXPECT_EQ(4, last_called);
}

TEST_F(TestXcdrCMockDispatch, DeserializeMessageFrom)
{
  int dummy{};
  rosidl_memory_region_t storage{{nullptr, 0}, 0};
  EXPECT_EQ(RCUTILS_RET_OK,
    rosidl_typesupport_xcdr_c_deserialize_message_from(&handle_, storage, &dummy));
  EXPECT_EQ(5, last_called);
}

TEST_F(TestXcdrCMockDispatch, DestroyMessage)
{
  int dummy{};
  rosidl_typesupport_xcdr_c_destroy_message(&handle_, &dummy);
  EXPECT_EQ(6, last_called);
}

TEST_F(TestXcdrCMockDispatch, ReleaseMessage)
{
  int dummy{};
  auto region = rosidl_typesupport_xcdr_c_release_message(&handle_, &dummy);
  EXPECT_EQ(reinterpret_cast<void *>(0xCAFE), region.location.address);
  EXPECT_EQ(100u, region.size);
  EXPECT_EQ(7, last_called);
}

TEST_F(TestXcdrCMockDispatch, CreateConstrained)
{
  // constraints is opaque to the C layer — pass a non-null dummy value.
  rosidl_message_type_constraints_t dummy_constraint{};
  auto * constrained =
    rosidl_typesupport_xcdr_c_create_constrained_message_type_support(
      &handle_, &dummy_constraint);
  ASSERT_NE(nullptr, constrained);
  EXPECT_EQ(rosidl_typesupport_xcdr_c__identifier,
    constrained->typesupport_identifier);
  EXPECT_EQ(8, last_called);
}

TEST_F(TestXcdrCMockDispatch, DestroyConstrained)
{
  rosidl_typesupport_xcdr_c_destroy_constrained_message_type_support(nullptr);
  EXPECT_EQ(-1, last_called);  // destroy_constrained(nullptr) must be a no-op.

  rosidl_message_type_constraints_t dummy_constraint{};
  auto * constrained =
    rosidl_typesupport_xcdr_c_create_constrained_message_type_support(
      &handle_, &dummy_constraint);
  ASSERT_NE(nullptr, constrained);
  last_called = -1;
  rosidl_typesupport_xcdr_c_destroy_constrained_message_type_support(constrained);
  EXPECT_EQ(9, last_called);
}

TEST_F(TestXcdrCMockDispatch, CompareTypeSpecificConstraints)
{
  int ts_a{}, ts_b{};
  rosidl_message_type_constraints_t candidate{};
  rosidl_message_type_constraints_t baseline{};
  candidate.type_specific = &ts_a;
  baseline.type_specific = &ts_b;

  bool result = rosidl_typesupport_xcdr_c_compare_constraints(
    &handle_, &candidate, &baseline, nullptr, nullptr);
  EXPECT_TRUE(result);  // dispatches to mock_compare_ts which returns true
  EXPECT_EQ(10, last_called);
}

// =============================================================================
// Wrong identifier rejection (all trampolines)
// =============================================================================

class TestXcdrCWrongIdentifier : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rcutils_reset_error();
  }

  rosidl_message_type_support_t handle_ = make_wrong_id_handle();
};

TEST_F(TestXcdrCWrongIdentifier, GetExpectedSize)
{
  size_t size = 0;
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_get_expected_size(&handle_, &size));
}

TEST_F(TestXcdrCWrongIdentifier, GetMessageSize)
{
  int dummy{};
  size_t size = 0;
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_get_message_size(&handle_, &dummy, &size));
}

TEST_F(TestXcdrCWrongIdentifier, ConstructMessageAt)
{
  rosidl_memory_region_t storage{{nullptr, 0}, 0};
  void * msg = nullptr;
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_construct_message_at(&handle_, storage, &msg));
}

TEST_F(TestXcdrCWrongIdentifier, CastMessageAt)
{
  rosidl_memory_region_t storage{{nullptr, 0}, 0};
  void * msg = nullptr;
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_cast_message_at(&handle_, storage, &msg));
}

TEST_F(TestXcdrCWrongIdentifier, SerializeMessageInto)
{
  int dummy{};
  rosidl_memory_region_t storage{{nullptr, 0}, 0};
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_serialize_message_into(&handle_, &dummy, storage));
}

TEST_F(TestXcdrCWrongIdentifier, DeserializeMessageFrom)
{
  int dummy{};
  rosidl_memory_region_t storage{{nullptr, 0}, 0};
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_deserialize_message_from(&handle_, storage, &dummy));
}

TEST_F(TestXcdrCWrongIdentifier, CreateConstrained)
{
  // Wrong identifier test — nullptr constraints is fine since the
  // identifier check runs first.
  EXPECT_EQ(nullptr,
    rosidl_typesupport_xcdr_c_create_constrained_message_type_support(
      &handle_, nullptr));
}

TEST_F(TestXcdrCWrongIdentifier, CompareConstraints)
{
  int ts_a{}, ts_b{};
  rosidl_message_type_constraints_t candidate{};
  rosidl_message_type_constraints_t baseline{};
  candidate.type_specific = &ts_a;
  baseline.type_specific = &ts_b;
  // Wrong identifier (doesn't contain "xcdr") => fails.
  EXPECT_FALSE(
    rosidl_typesupport_xcdr_c_compare_constraints(
      &handle_, &candidate, &baseline, nullptr, nullptr));
}

// DestroyMessage/DestroyConstrained/ReleaseMessage with wrong id must not crash
// and must not dispatch to mock callbacks.
TEST_F(TestXcdrCWrongIdentifier, DestroyMessage)
{
  int dummy{};
  EXPECT_NO_THROW(
    rosidl_typesupport_xcdr_c_destroy_message(&handle_, &dummy));
}

TEST_F(TestXcdrCWrongIdentifier, DestroyConstrained)
{
  EXPECT_NO_THROW(
    rosidl_typesupport_xcdr_c_destroy_constrained_message_type_support(&handle_));
}

TEST_F(TestXcdrCWrongIdentifier, ReleaseMessage)
{
  auto region = rosidl_typesupport_xcdr_c_release_message(&handle_, nullptr);
  EXPECT_EQ(nullptr, region.location.address);
  EXPECT_EQ(0u, region.size);
}

// =============================================================================
// Null callback pointer path
// =============================================================================

TEST(TestXcdrCNullCallbacks, GetExpectedSize)
{
  // Build a handle whose outer struct has get_expected_size = nullptr
  rosidl_message_xcdr_type_support_t null_cb = mock_outer;
  null_cb.get_expected_size = nullptr;
  rosidl_message_type_support_t ts{};
  ts.typesupport_identifier = rosidl_typesupport_xcdr_c__identifier;
  ts.data = &null_cb;

  size_t size = 0;
  EXPECT_EQ(RCUTILS_RET_ERROR,
    rosidl_typesupport_xcdr_c_get_expected_size(&ts, &size));
}

TEST(TestXcdrCNullCallbacks, CompareConstraints_NullCallback)
{
  rosidl_message_xcdr_type_support_t null_cb = mock_outer;
  null_cb.compare_type_specific_constraints = nullptr;
  rosidl_message_type_support_t ts{};
  ts.typesupport_identifier = rosidl_typesupport_xcdr_c__identifier;
  ts.data = &null_cb;

  int ts_a{}, ts_b{};
  rosidl_message_type_constraints_t candidate{};
  rosidl_message_type_constraints_t baseline{};
  candidate.type_specific = &ts_a;
  baseline.type_specific = &ts_b;
  EXPECT_FALSE(
    rosidl_typesupport_xcdr_c_compare_constraints(
      &ts, &candidate, &baseline, nullptr, nullptr));
}

TEST(TestXcdrCNullCallbacks, CreateConstrained_NullCallback)
{
  rosidl_message_xcdr_type_support_t null_cb = mock_outer;
  null_cb.create_constrained = nullptr;
  rosidl_message_type_support_t ts{};
  ts.typesupport_identifier = rosidl_typesupport_xcdr_c__identifier;
  ts.data = &null_cb;

  rosidl_message_type_constraints_t constraints{};
  EXPECT_EQ(nullptr,
    rosidl_typesupport_xcdr_c_create_constrained_message_type_support(
      &ts, &constraints));
}

// =============================================================================
// Constraint comparison
// =============================================================================

TEST(TestXcdrCConstraints, NullBaselineIsAlwaysCompatible)
{
  rosidl_message_type_constraints_t candidate{};
  EXPECT_TRUE(
    rosidl_typesupport_xcdr_c_compare_constraints(
      nullptr, &candidate, nullptr, nullptr, nullptr));
}

TEST(TestXcdrCConstraints, NullCandidateIsLooser)
{
  rosidl_message_type_constraints_t baseline{};
  baseline.max_string_length = 100;
  EXPECT_FALSE(
    rosidl_typesupport_xcdr_c_compare_constraints(
      nullptr, nullptr, &baseline, nullptr, nullptr));
}

TEST(TestXcdrCConstraints, BlanketStringLengthCheck)
{
  rosidl_message_type_constraints_t baseline{};
  baseline.max_string_length = 100;

  rosidl_message_type_constraints_t candidate{};
  candidate.max_string_length = 200;

  EXPECT_FALSE(
    rosidl_typesupport_xcdr_c_compare_constraints(
      nullptr, &candidate, &baseline, nullptr, nullptr));
}

TEST(TestXcdrCConstraints, BlanketTotalSizeCheck)
{
  rosidl_message_type_constraints_t baseline{};
  baseline.max_total_size = 1000;

  rosidl_message_type_constraints_t candidate{};
  candidate.max_total_size = 2000;

  EXPECT_FALSE(
    rosidl_typesupport_xcdr_c_compare_constraints(
      nullptr, &candidate, &baseline, nullptr, nullptr));
}

TEST(TestXcdrCConstraints, TypeSpecificNoHandleFails)
{
  int ts_a{};
  int ts_b{};

  rosidl_message_type_constraints_t baseline{};
  baseline.type_specific = &ts_a;

  rosidl_message_type_constraints_t candidate{};
  candidate.type_specific = &ts_b;

  // Without a handle, different type_specific pointers => incompatible.
  EXPECT_FALSE(
    rosidl_typesupport_xcdr_c_compare_constraints(
      nullptr, &candidate, &baseline, nullptr, nullptr));
}

TEST(TestXcdrCConstraints, Compatible)
{
  rosidl_message_type_constraints_t baseline{};
  baseline.max_string_length = 100;
  baseline.max_total_size = 1000;

  rosidl_message_type_constraints_t candidate{};
  candidate.max_string_length = 50;
  candidate.max_total_size = 500;

  EXPECT_TRUE(
    rosidl_typesupport_xcdr_c_compare_constraints(
      nullptr, &candidate, &baseline, nullptr, nullptr));
}

TEST(TestXcdrCConstraints, SameTypeSpecificPointerCompatibleWithoutHandle)
{
  int ts_val{};

  rosidl_message_type_constraints_t baseline{};
  baseline.type_specific = &ts_val;

  rosidl_message_type_constraints_t candidate{};
  candidate.type_specific = &ts_val;  // same pointer => identity match

  EXPECT_TRUE(
    rosidl_typesupport_xcdr_c_compare_constraints(
      nullptr, &candidate, &baseline, nullptr, nullptr));
}
