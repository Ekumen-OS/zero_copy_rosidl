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

#include <gtest/gtest.h>

#include "rosidl_runtime_c/service_type_support_struct.h"
#include "rosidl_typesupport_xcdr_cpp/service_type_support.hpp"
#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/srv/add_two_ints.hpp"

TEST(TestServiceTypesupport, GetServiceTypesupportHandle)
{
  // Get service type support handle
  auto handle = rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts>();
  
  ASSERT_NE(nullptr, handle);
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier, handle->typesupport_identifier);
  
  // Check that request/response/event typesupports are not null
  ASSERT_NE(nullptr, handle->request_typesupport);
  ASSERT_NE(nullptr, handle->response_typesupport);
  ASSERT_NE(nullptr, handle->event_typesupport);
  
  // Verify request typesupport has XCDR identifier
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier,
               handle->request_typesupport->typesupport_identifier);
  
  // Verify response typesupport has XCDR identifier
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier,
               handle->response_typesupport->typesupport_identifier);
  
  // Verify event typesupport has XCDR identifier
  EXPECT_STREQ(rosidl_typesupport_xcdr_cpp__identifier,
               handle->event_typesupport->typesupport_identifier);
}

TEST(TestServiceTypesupport, GetServiceTypesupportHandleFunction)
{
  // Get service type support handle
  auto handle = rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts>();
  
  // Test with matching identifier
  auto result = rosidl_typesupport_xcdr_cpp::get_service_typesupport_handle_function(
    handle, rosidl_typesupport_xcdr_cpp__identifier);
  EXPECT_EQ(handle, result);
  
  // Test with non-matching identifier
  result = rosidl_typesupport_xcdr_cpp::get_service_typesupport_handle_function(
    handle, "different_identifier");
  EXPECT_EQ(nullptr, result);
}

TEST(TestServiceTypesupport, ServiceRequestResponseSerialization)
{
  // This test verifies that request and response messages can be serialized/deserialized
  // since service typesupport just forwards to message typesupport
  
  auto handle = rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<
    rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts>();
  
  // Create request message
  rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts_Request request;
  request.a = 5;
  request.b = 7;
  
  // Get request typesupport callbacks
  auto * request_callbacks = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(
    handle->request_typesupport->data);
  
  ASSERT_NE(nullptr, request_callbacks);
  ASSERT_NE(nullptr, request_callbacks->serialize_message_into);
  ASSERT_NE(nullptr, request_callbacks->deserialize_message_from);
  
  // Serialize request
  std::vector<uint8_t> buffer(256);
  size_t size = 0;
  auto ret = request_callbacks->serialize_message_into(
    &request,
    buffer.data(),
    buffer.size(),
    &size);
  
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_GT(size, 0u);
  
  // Deserialize request
  rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts_Request request_out;
  ret = request_callbacks->deserialize_message_from(
    buffer.data(),
    size,
    &request_out);
  
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  EXPECT_EQ(request.a, request_out.a);
  EXPECT_EQ(request.b, request_out.b);
}
