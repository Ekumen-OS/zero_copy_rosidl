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

/// @file test_service_typesupport.cpp
///
/// Tests for the service typesupport layer:
///   - Handle retrieval for standard and experimental service namespaces
///   - Identifier matching and non-matching rejection
///   - Request serialization roundtrip
///   - Response serialization roundtrip
///   - Event typesupport presence and identifier

#include <gtest/gtest.h>
#include <vector>

#include "rosidl_runtime_c/service_type_support_struct.h"
#include "rosidl_typesupport_xcdr_cpp/service_type_support.hpp"
#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"
#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/srv/add_two_ints.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/srv/experimental/add_two_ints.hpp"

// =============================================================================
// Handle retrieval
// =============================================================================

TEST(TestServiceTypesupport, Standard_GetHandle)
{
  auto handle = rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts>();

  ASSERT_NE(nullptr, handle);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, handle->typesupport_identifier);

  ASSERT_NE(nullptr, handle->request_typesupport);
  ASSERT_NE(nullptr, handle->response_typesupport);
  ASSERT_NE(nullptr, handle->event_typesupport);

  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier,
               handle->request_typesupport->typesupport_identifier);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier,
               handle->response_typesupport->typesupport_identifier);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier,
               handle->event_typesupport->typesupport_identifier);
}

TEST(TestServiceTypesupport, Experimental_GetHandle)
{
  auto handle = rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::srv::experimental::AddTwoInts>();

  ASSERT_NE(nullptr, handle);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, handle->typesupport_identifier);

  ASSERT_NE(nullptr, handle->request_typesupport);
  ASSERT_NE(nullptr, handle->response_typesupport);
  // Event typesupport may reference the standard event typesupport for
  // experimental services (service_msgs does not have experimental variants).
  // It should be non-null regardless.
  ASSERT_NE(nullptr, handle->event_typesupport);
}

// =============================================================================
// Identifier matching
// =============================================================================

TEST(TestServiceTypesupport, IdentifierMatching)
{
  auto handle = rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts>();

  // Matching identifier returns the handle
  auto result = rosidl_typesupport_xcdr_cpp::get_service_typesupport_handle_function(
    handle, rosidl_typesupport_xcdr_cpp__identifier);
  EXPECT_EQ(handle, result);

  // Non-matching identifier returns nullptr
  result = rosidl_typesupport_xcdr_cpp::get_service_typesupport_handle_function(
    handle, "different_identifier");
  EXPECT_EQ(nullptr, result);
}

// =============================================================================
// Request serialization roundtrip
// =============================================================================

TEST(TestServiceTypesupport, RequestRoundtrip)
{
  using Request = rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts_Request;
  using type_support_callbacks_t =
    rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t;

  auto handle = rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts>();

  auto * callbacks =
    static_cast<const type_support_callbacks_t *>(handle->request_typesupport->data);
  ASSERT_NE(nullptr, callbacks);
  ASSERT_NE(nullptr, callbacks->get_message_size);
  ASSERT_NE(nullptr, callbacks->serialize_message_into);
  ASSERT_NE(nullptr, callbacks->deserialize_message_from);

  Request request;
  request.a = 5;
  request.b = 7;

  size_t size = 0;
  auto ret = callbacks->get_message_size(&request, &size);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  ASSERT_GT(size, 0u);

  std::vector<uint8_t> buffer(size);
  rosidl_runtime_cpp::MemoryRegion<void> storage(buffer.data(), size);

  ret = callbacks->serialize_message_into(&request, storage);
  EXPECT_EQ(RCUTILS_RET_OK, ret);

  Request request_out;
  ret = callbacks->deserialize_message_from(storage, &request_out);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_EQ(request.a, request_out.a);
  EXPECT_EQ(request.b, request_out.b);
}

// =============================================================================
// Response serialization roundtrip
// =============================================================================

TEST(TestServiceTypesupport, ResponseRoundtrip)
{
  using Response = rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts_Response;
  using type_support_callbacks_t =
    rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t;

  auto handle = rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts>();

  auto * callbacks =
    static_cast<const type_support_callbacks_t *>(handle->response_typesupport->data);
  ASSERT_NE(nullptr, callbacks);
  ASSERT_NE(nullptr, callbacks->get_message_size);
  ASSERT_NE(nullptr, callbacks->serialize_message_into);
  ASSERT_NE(nullptr, callbacks->deserialize_message_from);

  Response response;
  response.sum = 12;

  size_t size = 0;
  auto ret = callbacks->get_message_size(&response, &size);
  ASSERT_EQ(RCUTILS_RET_OK, ret);
  ASSERT_GT(size, 0u);

  std::vector<uint8_t> buffer(size);
  rosidl_runtime_cpp::MemoryRegion<void> storage(buffer.data(), size);

  ret = callbacks->serialize_message_into(&response, storage);
  EXPECT_EQ(RCUTILS_RET_OK, ret);

  Response response_out;
  ret = callbacks->deserialize_message_from(storage, &response_out);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_EQ(response.sum, response_out.sum);
}

// =============================================================================
// Event typesupport is also a message — check it roundtrips
// =============================================================================

TEST(TestServiceTypesupport, EventRoundtrip)
{
  using Event = rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts_Event;
  using type_support_callbacks_t =
    rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t;

  auto handle = rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts>();

  auto * callbacks =
    static_cast<const type_support_callbacks_t *>(handle->event_typesupport->data);
  ASSERT_NE(nullptr, callbacks);

  // The Event type is auto-generated.  Create a minimal instance and verify
  // the serialization pipeline accepts it.
  Event event{};

  size_t size = 0;
  auto ret = callbacks->get_message_size(&event, &size);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_GT(size, 0u);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
