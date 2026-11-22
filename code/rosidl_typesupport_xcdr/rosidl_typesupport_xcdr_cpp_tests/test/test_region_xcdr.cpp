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

#include <cstdint>
#include <utility>
#include <vector>

#include "rcutils/error_handling.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_cpp/service_type_support.hpp"
#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/bounded_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/bounded_variable.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/nested_bounded.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/nested_containers.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/nested_last.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/unbounded_message.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/variable_containers.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/bounded_message__region.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/bounded_variable__region.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/nested_bounded__region.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/nested_containers__region.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/unbounded_message__region.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/variable_containers__region.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/variable_containers.h"
#include "rosidl_typesupport_xcdr_cpp_tests/srv/add_two_ints.hpp"
#include \
  "rosidl_typesupport_xcdr_cpp_tests/srv/views/detail/add_two_ints__rosidl_typesupport_xcdr_cpp.hpp"
#include \
  "rosidl_typesupport_xcdr_cpp_tests/srv/views/detail/view_fields__rosidl_typesupport_xcdr_cpp.hpp"

namespace xcdr = rosidl_typesupport_xcdr_cpp;
using Plain = rosidl_typesupport_xcdr_cpp_tests::msg::BoundedMessage;
using View = rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionBoundedMessage;
using BoundedVariable = rosidl_typesupport_xcdr_cpp_tests::msg::BoundedVariable;
using BoundedVariableView = rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionBoundedVariable;
using Unbounded = rosidl_typesupport_xcdr_cpp_tests::msg::UnboundedMessage;
using UnboundedView = rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionUnboundedMessage;
using Nested = rosidl_typesupport_xcdr_cpp_tests::msg::NestedContainers;
using NestedView = rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionNestedContainers;
using Variable = rosidl_typesupport_xcdr_cpp_tests::msg::VariableContainers;
using VariableView = rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionVariableContainers;

extern "C"
{
rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionConstraints *
test_c_region_limits(void);
bool test_c_region_access(void * loan);
}

TEST(RegionXcdr, CRegionAccessAndConstraints)
{
  using CConstraints =
    rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionConstraints;
  auto deleter =
    rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionConstraints__destroy;
  std::unique_ptr<CConstraints, decltype(deleter)> constraints(test_c_region_limits(), deleter);
  ASSERT_NE(nullptr, constraints);
  rosidl_typesupport_xcdr_c__constraints_t wrapped{};
  wrapped.type_specific =
    rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionConstraints__data(
    constraints.get());
  wrapped.strict = true;
  auto base =
    rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__get_type_support();
  auto ts = xcdr::create_constrained_message_type_support(base, &wrapped);
  ASSERT_NE(nullptr, ts) << rcutils_get_error_string().str;
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts.get(), &size));
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts.get(), region, &raw));
  ASSERT_TRUE(test_c_region_access(raw)) << rcutils_get_error_string().str;
  auto compacted = xcdr::compact_message_in_place(ts.get(), raw);
  ASSERT_NE(nullptr, compacted.location.address);
  Variable plain;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::deserialize_message_from(
      xcdr::get_message_type_support_handle<Variable>(),
      rosidl_runtime_cpp::MemoryRegion<void>(compacted), &plain));
  EXPECT_EQ(42u, plain.tail);
  EXPECT_EQ("label", plain.labels[0]);
  ASSERT_EQ(1u, plain.names.size());
  EXPECT_EQ("name", plain.names[0]);
  EXPECT_EQ(u"a\u20ac", plain.wide);
  ASSERT_EQ(1u, plain.items.size());
  EXPECT_EQ(17, plain.items[0].id);
  ASSERT_EQ(1u, plain.nested_variable.size());
  EXPECT_EQ("okay", plain.nested_variable[0].name);
}

TEST(RegionXcdr, ServiceRequestAndResponseInteroperate)
{
  using Service = rosidl_typesupport_xcdr_cpp_tests::srv::AddTwoInts;
  using RegionService = rosidl_typesupport_xcdr_cpp_tests::srv::views::RegionAddTwoInts;
  auto region_service = xcdr::get_service_type_support_handle<RegionService>();
  auto plain_service = xcdr::get_service_type_support_handle<Service>();
  ASSERT_NE(nullptr, region_service);
  ASSERT_NE(nullptr, plain_service);
  auto dispatch = rosidl_typesupport_cpp::get_service_type_support_handle<RegionService>();
  ASSERT_NE(nullptr, dispatch);
  ASSERT_NE(nullptr, dispatch->func);
  EXPECT_EQ(region_service, dispatch->func(dispatch, region_service->typesupport_identifier));
  auto request_dispatch =
    rosidl_typesupport_cpp::get_message_type_support_handle<RegionService::Request>();
  ASSERT_NE(nullptr, request_dispatch);
  ASSERT_NE(nullptr, request_dispatch->func);
  EXPECT_EQ(
    region_service->request_typesupport,
    request_dispatch->func(request_dispatch, region_service->typesupport_identifier));
  ASSERT_NE(nullptr, region_service->request_typesupport);
  ASSERT_NE(nullptr, region_service->response_typesupport);
  EXPECT_EQ(plain_service->event_typesupport, region_service->event_typesupport);
  ASSERT_NE(nullptr, region_service->get_type_hash_func);
  ASSERT_NE(nullptr, plain_service->get_type_hash_func);
  ASSERT_NE(nullptr, region_service->get_type_hash_func(region_service));
  EXPECT_EQ(
    plain_service->get_type_hash_func(plain_service)->version,
    region_service->get_type_hash_func(region_service)->version);

  Service::Request request;
  request.a = 11;
  request.b = 31;
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK,
    xcdr::get_message_size(plain_service->request_typesupport, &request, &size));
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  ASSERT_EQ(RCUTILS_RET_OK,
    xcdr::serialize_message_into(plain_service->request_typesupport, &request, region));
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK,
    xcdr::cast_message_at(region_service->request_typesupport, region, &raw));
  auto * request_view = static_cast<RegionService::Request *>(raw);
  EXPECT_EQ(11, request_view->a());
  EXPECT_EQ(31, request_view->b());
  xcdr::destroy_message(region_service->request_typesupport, request_view);

  ASSERT_EQ(RCUTILS_RET_OK,
    xcdr::get_expected_message_size(region_service->response_typesupport, &size));
  std::vector<uint8_t> response_buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> response_region(response_buffer.data() + 4, size);
  ASSERT_EQ(RCUTILS_RET_OK,
    xcdr::construct_message_at(region_service->response_typesupport, response_region, &raw));
  auto * response_view = static_cast<RegionService::Response *>(raw);
  response_view->sum() = 42;
  auto released = xcdr::release_message(region_service->response_typesupport, response_view);
  Service::Response response;
  ASSERT_EQ(RCUTILS_RET_OK,
    xcdr::deserialize_message_from(plain_service->response_typesupport, released, &response));
  EXPECT_EQ(42, response.sum);
}

TEST(RegionXcdr, ServiceRequestSupportsVariableAndNestedConstraints)
{
  using RegionService = rosidl_typesupport_xcdr_cpp_tests::srv::views::RegionViewFields;
  auto service = xcdr::get_service_type_support_handle<RegionService>();
  ASSERT_NE(nullptr, service);
  RegionService::Request::Constraints constraints;
  constraints.name.size = 4;
  constraints.numbers.size = 2;
  rosidl_typesupport_xcdr_c__constraints_t wrapped{};
  wrapped.type_specific = &constraints;
  wrapped.strict = true;
  auto ts = xcdr::create_constrained_message_type_support(service->request_typesupport, &wrapped);
  ASSERT_NE(nullptr, ts) << rcutils_get_error_string().str;
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts.get(), &size));
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts.get(), region, &raw));
  auto * view = static_cast<RegionService::Request *>(raw);
  view->name().assign("name");
  EXPECT_THROW(view->name().assign("longer"), std::length_error);
  view->numbers().resize(2);
  EXPECT_THROW(view->numbers().resize(3), std::length_error);
  view->numbers()[0] = 17;
  view->inner().id() = 23;
  auto compacted = xcdr::compact_message_in_place(ts.get(), view);
  ASSERT_NE(nullptr, compacted.location.address);
  rosidl_typesupport_xcdr_cpp_tests::srv::ViewFields::Request plain;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::deserialize_message_from(
      xcdr::get_service_type_support_handle<
        rosidl_typesupport_xcdr_cpp_tests::srv::ViewFields>()->request_typesupport,
      rosidl_runtime_cpp::MemoryRegion<void>(compacted), &plain));
  EXPECT_EQ("name", plain.name);
  ASSERT_EQ(2u, plain.numbers.size());
  EXPECT_EQ(17, plain.numbers[0]);
  EXPECT_EQ(23, plain.inner.id);
}

TEST(RegionXcdr, GenericDispatchFindsXcdr)
{
  auto dispatch = rosidl_typesupport_cpp::get_message_type_support_handle<View>();
  ASSERT_NE(nullptr, dispatch);
  auto expected = xcdr::get_message_type_support_handle<View>();
  ASSERT_NE(nullptr, expected);
  ASSERT_NE(nullptr, dispatch->func);
  EXPECT_EQ(expected, dispatch->func(dispatch, expected->typesupport_identifier));
}

TEST(RegionXcdr, ConstructQueryReleaseAndCast)
{
  auto ts = xcdr::get_message_type_support_handle<View>();
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts, &size));
  ASSERT_GT(size, 0u);
  std::vector<uint8_t> buffer(size + 4);
  auto * bytes = buffer.data() + 4;
  ASSERT_EQ(0u, reinterpret_cast<std::uintptr_t>(bytes + 4) % alignof(uint64_t));
  rosidl_runtime_cpp::MemoryRegion<void> short_region(bytes, size - 1);
  void * raw = nullptr;
  EXPECT_NE(RCUTILS_RET_OK, xcdr::construct_message_at(ts, short_region, &raw));
  EXPECT_EQ(nullptr, raw);
  rcutils_reset_error();

  rosidl_runtime_cpp::MemoryRegion<void> region(bytes, size, 7);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts, region, &raw));
  ASSERT_NE(nullptr, raw);
  auto * view = static_cast<View *>(raw);
  view->id() = 42;
  view->value() = 1.5;
  view->data()[0] = 9;
  auto backing = xcdr::get_backing_storage(ts, view);
  EXPECT_EQ(bytes, backing.data());
  EXPECT_EQ(7, backing.attributes());
  EXPECT_EQ(42u, view->id());
  auto released = xcdr::release_message(ts, view);
  EXPECT_EQ(bytes, released.data());
  EXPECT_EQ(7, released.attributes());

  ASSERT_EQ(RCUTILS_RET_OK, xcdr::cast_message_at(ts, released, &raw));
  view = static_cast<View *>(raw);
  EXPECT_EQ(42u, view->id());
  EXPECT_DOUBLE_EQ(1.5, view->value());
  EXPECT_EQ(9u, view->data()[0]);
  xcdr::destroy_message(ts, view);
}

TEST(RegionXcdr, SimulatedLoanRemainsOwnedUntilViewIsConsumed)
{
  auto ts = xcdr::get_message_type_support_handle<View>();
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts, &size));
  std::vector<uint8_t> loan(size + 4);
  auto * bytes = loan.data() + 4;
  rosidl_runtime_cpp::MemoryRegion<void> region(bytes, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts, region, &raw));
  auto * view = static_cast<View *>(raw);
  view->id() = 42;
  EXPECT_EQ(bytes, xcdr::get_backing_storage(ts, view).data());
  EXPECT_EQ(bytes, xcdr::get_backing_storage(ts, view).data());
  EXPECT_EQ(42u, view->id());
  xcdr::destroy_message(ts, view);

  ASSERT_EQ(RCUTILS_RET_OK, xcdr::cast_message_at(ts, region, &raw));
  view = static_cast<View *>(raw);
  EXPECT_EQ(42u, view->id());
  auto released = xcdr::release_message(ts, view);
  EXPECT_EQ(bytes, released.data());
  EXPECT_EQ(size, released.size());
  EXPECT_EQ(bytes, loan.data() + 4);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::cast_message_at(ts, released, &raw));
  xcdr::destroy_message(ts, raw);
}

TEST(RegionXcdr, MovedFromLoanReportsErrorsWithoutConsumingTheView)
{
  auto ts = xcdr::get_message_type_support_handle<View>();
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts, &size));
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts, region, &raw));
  auto * view = static_cast<View *>(raw);
  View moved(std::move(*view));

  EXPECT_EQ(nullptr, xcdr::get_backing_storage(ts, view).data());
  EXPECT_TRUE(rcutils_error_is_set());
  rcutils_reset_error();
  EXPECT_EQ(nullptr, xcdr::release_message(ts, view).data());
  EXPECT_TRUE(rcutils_error_is_set());
  rcutils_reset_error();
  EXPECT_EQ(nullptr, xcdr::compact_message_in_place(ts, view).location.address);
  EXPECT_TRUE(rcutils_error_is_set());
  rcutils_reset_error();
  EXPECT_NE(RCUTILS_RET_OK, xcdr::get_message_size(ts, view, &size));
  EXPECT_TRUE(rcutils_error_is_set());
  rcutils_reset_error();
  xcdr::destroy_message(ts, view);
}

TEST(RegionXcdr, OrdinaryMessageInteroperates)
{
  auto view_ts = xcdr::get_message_type_support_handle<View>();
  auto plain_ts = xcdr::get_message_type_support_handle<Plain>();
  Plain plain;
  plain.id = 42;
  plain.value = 1.5;
  plain.data[0] = 9;
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_message_size(plain_ts, &plain, &size));
  std::vector<uint8_t> buffer(size + 4);
  auto * bytes = buffer.data() + 4;
  ASSERT_EQ(0u, reinterpret_cast<std::uintptr_t>(bytes + 4) % alignof(uint64_t));
  rosidl_runtime_cpp::MemoryRegion<void> region(bytes, size);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::serialize_message_into(plain_ts, &plain, region));
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::cast_message_at(view_ts, region, &raw));
  auto * view = static_cast<View *>(raw);
  EXPECT_EQ(plain.id, view->id());
  EXPECT_DOUBLE_EQ(plain.value, view->value());
  EXPECT_EQ(plain.data[0], view->data()[0]);
  xcdr::destroy_message(view_ts, view);
}

TEST(RegionXcdr, CompactShortenedBoundedFields)
{
  auto ts = xcdr::get_message_type_support_handle<BoundedVariableView>();
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts, &size));
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts, region, &raw));
  auto * view = static_cast<BoundedVariableView *>(raw);
  view->name() = "a";
  view->values().resize(1);
  view->values()[0] = 17;
  view->wide() = u"w";
  view->wide_array()[0] = u"x";
  view->wide_array()[1] = u"yz";
  view->tail() = 99;
  auto compacted = xcdr::compact_message_in_place(ts, view);
  ASSERT_NE(nullptr, compacted.location.address) << rcutils_get_error_string().str;
  BoundedVariable plain;
  auto plain_ts = xcdr::get_message_type_support_handle<BoundedVariable>();
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::deserialize_message_from(
      plain_ts, rosidl_runtime_cpp::MemoryRegion<void>(compacted), &plain));
  EXPECT_EQ("a", plain.name);
  ASSERT_EQ(1u, plain.values.size());
  EXPECT_EQ(17u, plain.values[0]);
  EXPECT_EQ(u"w", plain.wide);
  EXPECT_EQ(u"x", plain.wide_array[0]);
  EXPECT_EQ(u"yz", plain.wide_array[1]);
  EXPECT_EQ(99u, plain.tail);
}

TEST(RegionXcdr, CompactFullWideStringsWithoutRewrite)
{
  auto ts = xcdr::get_message_type_support_handle<BoundedVariableView>();
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts, &size));
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts, region, &raw));
  auto * view = static_cast<BoundedVariableView *>(raw);
  view->name() = "12345678";
  view->values().resize(3);
  view->wide() = u"12345678";
  view->wide_array()[0] = u"abcdefgh";
  view->wide_array()[1] = u"ABCDEFGH";
  view->tail() = 99;
  auto compacted = xcdr::compact_message_in_place(ts, view);
  ASSERT_NE(nullptr, compacted.location.address) << rcutils_get_error_string().str;
  EXPECT_EQ(size, compacted.size);
  BoundedVariable plain;
  auto plain_ts = xcdr::get_message_type_support_handle<BoundedVariable>();
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::deserialize_message_from(
      plain_ts, rosidl_runtime_cpp::MemoryRegion<void>(compacted), &plain));
  EXPECT_EQ(u"12345678", plain.wide);
  EXPECT_EQ(u"abcdefgh", plain.wide_array[0]);
  EXPECT_EQ(u"ABCDEFGH", plain.wide_array[1]);
  EXPECT_EQ(99u, plain.tail);
}

TEST(RegionXcdr, FullyBoundedMessageSupportsTighterConstraints)
{
  auto base = xcdr::get_message_type_support_handle<BoundedVariableView>();
  size_t maximum_size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(base, &maximum_size));
  BoundedVariableView::Constraints constraints;
  constraints.name.size = 3;
  constraints.values.size = 1;
  constraints.wide.size = 2;
  constraints.wide_array.size = 2;
  rosidl_typesupport_xcdr_c__constraints_t wrapped{};
  wrapped.type_specific = &constraints;
  auto ts = xcdr::create_constrained_message_type_support(base, &wrapped);
  ASSERT_NE(nullptr, ts) << rcutils_get_error_string().str;
  size_t constrained_size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts.get(), &constrained_size));
  EXPECT_LT(constrained_size, maximum_size);
  std::vector<uint8_t> buffer(constrained_size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, constrained_size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts.get(), region, &raw));
  auto * view = static_cast<BoundedVariableView *>(raw);
  EXPECT_EQ(1u, view->values().capacity());
  EXPECT_THROW(view->name().assign("long"), std::length_error);
  EXPECT_THROW(view->wide().assign(u"abc"), std::length_error);
  EXPECT_THROW(view->wide_array()[0].assign(u"abc"), std::length_error);
  xcdr::destroy_message(ts.get(), view);

  constraints.name.size = 9;
  EXPECT_EQ(nullptr, xcdr::create_constrained_message_type_support(base, &wrapped));
  rcutils_reset_error();
}

TEST(RegionXcdr, NestedBoundedConstraintsLimitBorrowedStorage)
{
  using NestedBoundedView =
    rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionNestedBounded;
  auto base = xcdr::get_message_type_support_handle<NestedBoundedView>();
  NestedBoundedView::Constraints constraints;
  constraints.nested.name.size = 2;
  constraints.nested.values.size = 1;
  rosidl_typesupport_xcdr_c__constraints_t wrapped{};
  wrapped.type_specific = &constraints;
  auto ts = xcdr::create_constrained_message_type_support(base, &wrapped);
  ASSERT_NE(nullptr, ts) << rcutils_get_error_string().str;
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts.get(), &size));
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts.get(), region, &raw));
  auto * view = static_cast<NestedBoundedView *>(raw);
  EXPECT_EQ(1u, view->nested().values().capacity());
  EXPECT_THROW(view->nested().name().assign("abc"), std::length_error);
  xcdr::destroy_message(ts.get(), view);
}

TEST(RegionXcdr, CastsWideStringsInNestedMessages)
{
  using NestedMessage = rosidl_typesupport_xcdr_cpp_tests::msg::NestedBounded;
  using NestedMessageView = rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionNestedBounded;
  NestedMessage plain;
  plain.nested.wide = u"nested";
  plain.items.resize(1);
  plain.items[0].wide = std::u16string(u"a\0b", 3);
  plain.items[0].wide_array[1] = u"array";
  plain.items[0].tail = 42;
  auto plain_ts = xcdr::get_message_type_support_handle<NestedMessage>();
  auto view_ts = xcdr::get_message_type_support_handle<NestedMessageView>();
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_message_size(plain_ts, &plain, &size));
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::serialize_message_into(plain_ts, &plain, region));
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::cast_message_at(view_ts, region, &raw))
    << rcutils_get_error_string().str;
  auto * view = static_cast<NestedMessageView *>(raw);
  EXPECT_EQ(u"nested", view->nested().wide().to_string());
  ASSERT_EQ(1u, view->items().size());
  EXPECT_EQ(std::u16string(u"a\0b", 3), view->items()[0].wide().to_string());
  EXPECT_EQ(u"array", view->items()[0].wide_array()[1].to_string());
  EXPECT_EQ(42u, view->items()[0].tail());
  xcdr::destroy_message(view_ts, view);
}

TEST(RegionXcdr, RejectsInvalidElementConstraintForEmptySequence)
{
  using NestedBoundedView =
    rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionNestedBounded;
  NestedBoundedView::Constraints constraints;
  constraints.items.element.name.size = 9;
  EXPECT_FALSE(constraints.CheckCompatible(NestedBoundedView::Constraints{}));
  rosidl_typesupport_xcdr_c__constraints_t wrapped{};
  wrapped.type_specific = &constraints;
  auto base = xcdr::get_message_type_support_handle<NestedBoundedView>();
  EXPECT_EQ(nullptr, xcdr::create_constrained_message_type_support(base, &wrapped));
  rcutils_reset_error();
}

TEST(RegionXcdr, CastRejectsMalformedAndIncompatibleBuffers)
{
  auto view_ts = xcdr::get_message_type_support_handle<View>();
  auto plain_ts = xcdr::get_message_type_support_handle<Plain>();
  Plain plain;
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_message_size(plain_ts, &plain, &size));
  std::vector<uint8_t> buffer(size + 5);
  auto * bytes = buffer.data() + 4;
  ASSERT_EQ(0u, reinterpret_cast<std::uintptr_t>(bytes + 4) % alignof(uint64_t));
  rosidl_runtime_cpp::MemoryRegion<void> region(bytes, size);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::serialize_message_into(plain_ts, &plain, region));

  void * raw = nullptr;
  rosidl_runtime_cpp::MemoryRegion<void> short_region(bytes, 3);
  EXPECT_NO_THROW({
    EXPECT_NE(RCUTILS_RET_OK, xcdr::cast_message_at(view_ts, short_region, &raw));
  });
  EXPECT_EQ(nullptr, raw);
  rcutils_reset_error();

  bytes[1] = 0;
  auto ret = xcdr::cast_message_at(view_ts, region, &raw);
  EXPECT_NE(RCUTILS_RET_OK, ret);
  if (raw) {
    xcdr::destroy_message(view_ts, raw);
    raw = nullptr;
  }
  rcutils_reset_error();

  rosidl_runtime_cpp::MemoryRegion<void> unaligned(bytes + 1, size);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::serialize_message_into(plain_ts, &plain, unaligned));
  ret = xcdr::cast_message_at(view_ts, unaligned, &raw);
  EXPECT_NE(RCUTILS_RET_OK, ret);
  if (raw) {
    xcdr::destroy_message(view_ts, raw);
  }
  rcutils_reset_error();
}

TEST(RegionXcdr, PlainDeserializerRejectsTruncatedNestedField)
{
  using NestedLast = rosidl_typesupport_xcdr_cpp_tests::msg::NestedLast;
  auto ts = xcdr::get_message_type_support_handle<NestedLast>();
  NestedLast input;
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_message_size(ts, &input, &size));
  std::vector<uint8_t> buffer(size);
  rosidl_runtime_cpp::MemoryRegion<void> full(buffer.data(), buffer.size());
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::serialize_message_into(ts, &input, full));
  rosidl_runtime_cpp::MemoryRegion<void> truncated(buffer.data(), size - 1);
  NestedLast output;
  EXPECT_NE(RCUTILS_RET_OK, xcdr::deserialize_message_from(ts, truncated, &output));
  rcutils_reset_error();
}

TEST(RegionXcdr, NestedFieldsAndPrimitiveArrays)
{
  auto base = xcdr::get_message_type_support_handle<NestedView>();
  NestedView::Constraints constraints;
  rosidl_typesupport_xcdr_c__constraints_t wrapped{};
  wrapped.type_specific = &constraints;
  auto ts = xcdr::create_constrained_message_type_support(base, &wrapped);
  ASSERT_NE(nullptr, ts);
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts.get(), &size));
  std::vector<uint8_t> buffer(size + 4);
  auto * bytes = buffer.data() + 4;
  ASSERT_EQ(0u, reinterpret_cast<std::uintptr_t>(bytes + 4) % alignof(uint64_t));
  rosidl_runtime_cpp::MemoryRegion<void> region(bytes, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts.get(), region, &raw));
  auto * view = static_cast<NestedView *>(raw);
  view->single().id() = 17;
  view->single().value() = 2.5;
  view->int_buf()[2] = 123456;
  auto released = xcdr::release_message(ts.get(), view);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::cast_message_at(ts.get(), released, &raw));
  view = static_cast<NestedView *>(raw);
  EXPECT_EQ(17u, view->single().id());
  EXPECT_DOUBLE_EQ(2.5, view->single().value());
  EXPECT_EQ(123456, view->int_buf()[2]);
  xcdr::destroy_message(ts.get(), view);
  Nested plain;
  auto plain_ts = xcdr::get_message_type_support_handle<Nested>();
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::deserialize_message_from(plain_ts, released, &plain));
  EXPECT_EQ(17u, plain.single.id);
  EXPECT_DOUBLE_EQ(2.5, plain.single.value);
  EXPECT_EQ(123456, plain.int_buf[2]);
}

TEST(RegionXcdr, CastRejectsUnterminatedString)
{
  auto plain_ts = xcdr::get_message_type_support_handle<Unbounded>();
  auto view_ts = xcdr::get_message_type_support_handle<UnboundedView>();
  Unbounded plain;
  plain.name = "hello";
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_message_size(plain_ts, &plain, &size));
  std::vector<uint8_t> buffer(size + 4);
  auto * bytes = buffer.data() + 4;
  rosidl_runtime_cpp::MemoryRegion<void> region(bytes, size);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::serialize_message_into(plain_ts, &plain, region));
  bytes[4 + 4 + 4 + plain.name.size()] = 'x';
  void * raw = nullptr;
  EXPECT_NO_THROW({
    EXPECT_NE(RCUTILS_RET_OK, xcdr::cast_message_at(view_ts, region, &raw));
  });
  if (raw) {
    xcdr::destroy_message(view_ts, raw);
  }
  rcutils_reset_error();

  plain.name = std::string("a\0b", 3);
  size_t embedded_size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_message_size(plain_ts, &plain, &embedded_size));
  ASSERT_LE(embedded_size, size);
  rosidl_runtime_cpp::MemoryRegion<void> embedded(bytes, embedded_size);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::serialize_message_into(plain_ts, &plain, embedded));
  raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::cast_message_at(view_ts, embedded, &raw))
    << rcutils_get_error_string().str;
  auto * view = static_cast<UnboundedView *>(raw);
  EXPECT_EQ(std::string_view("a\0b", 3), view->name().view());
  xcdr::destroy_message(view_ts, view);
}

TEST(RegionXcdr, CastsArraysAndSequencesOfVariableElements)
{
  Variable plain;
  plain.labels = {"a", std::string("la\0el", 5)};
  plain.names = {"x", std::string("se\0ond", 6)};
  plain.items.resize(2);
  plain.items[0].id = 3;
  plain.items[0].value = 1.5;
  plain.items[1].id = 7;
  plain.items[1].value = 2.5;
  plain.tail = 99;
  plain.numbers = {1234, 5678};
  plain.wide = std::u16string(u"w\0z", 3);
  plain.wide_labels = {u"first", std::u16string(u"a\0b", 3)};
  plain.wide_names = {u"one", std::u16string(u"x\0y", 3)};
  auto plain_ts = xcdr::get_message_type_support_handle<Variable>();
  auto view_ts = xcdr::get_message_type_support_handle<VariableView>();
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_message_size(plain_ts, &plain, &size));
  std::vector<uint8_t> buffer(size + 4);
  auto * bytes = buffer.data() + 4;
  rosidl_runtime_cpp::MemoryRegion<void> region(bytes, size);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::serialize_message_into(plain_ts, &plain, region));
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::cast_message_at(view_ts, region, &raw))
    << rcutils_get_error_string().str;
  auto * view = static_cast<VariableView *>(raw);
  EXPECT_EQ(std::string_view("la\0el", 5), view->labels()[1].view());
  ASSERT_EQ(2u, view->names().size());
  EXPECT_EQ(std::string_view("se\0ond", 6), view->names()[1].view());
  ASSERT_EQ(2u, view->items().size());
  EXPECT_EQ(7, view->items()[1].id());
  EXPECT_DOUBLE_EQ(1.5, view->items()[0].value());
  EXPECT_DOUBLE_EQ(2.5, view->items()[1].value());
  EXPECT_EQ(99u, view->tail());
  ASSERT_EQ(2u, view->numbers().size());
  EXPECT_EQ(5678u, view->numbers()[1]);
  EXPECT_EQ(std::u16string(u"w\0z", 3), view->wide().to_string());
  EXPECT_EQ(std::u16string(u"a\0b", 3), view->wide_labels()[1].to_string());
  ASSERT_EQ(2u, view->wide_names().size());
  EXPECT_EQ(std::u16string(u"x\0y", 3), view->wide_names()[1].to_string());
  xcdr::destroy_message(view_ts, view);
}

TEST(RegionXcdr, CastsEmptyVariableSequences)
{
  Variable plain;
  plain.labels = {"a", "b"};
  plain.tail = 99;
  auto plain_ts = xcdr::get_message_type_support_handle<Variable>();
  auto view_ts = xcdr::get_message_type_support_handle<VariableView>();
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_message_size(plain_ts, &plain, &size));
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::serialize_message_into(plain_ts, &plain, region));
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::cast_message_at(view_ts, region, &raw))
    << rcutils_get_error_string().str;
  auto * view = static_cast<VariableView *>(raw);
  EXPECT_EQ(0u, view->names().size());
  EXPECT_EQ(0u, view->items().size());
  EXPECT_EQ(99u, view->tail());
  EXPECT_EQ(0u, view->numbers().size());
  EXPECT_EQ(0u, view->wide_names().size());
  xcdr::destroy_message(view_ts, view);
}

TEST(RegionXcdr, ConstructsNestedVariableSequences)
{
  auto base = xcdr::get_message_type_support_handle<VariableView>();
  VariableView::Constraints constraints;
  constraints.nested_variable.element.name.size = 4;
  constraints.nested_variable.element.data.size = 2;
  rosidl_typesupport_xcdr_c__constraints_t wrapped{};
  wrapped.type_specific = &constraints;
  wrapped.strict = true;
  auto ts = xcdr::create_constrained_message_type_support(base, &wrapped);
  ASSERT_NE(nullptr, ts);
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts.get(), &size))
    << rcutils_get_error_string().str;
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts.get(), region, &raw))
    << rcutils_get_error_string().str;
  auto * view = static_cast<VariableView *>(raw);
  view->labels()[0].assign("12345678");
  view->labels()[1].assign("abcdefgh");
  view->names().resize(3);
  view->names()[0].assign("12345678");
  view->names()[1].assign("short");
  view->names()[2].assign("abcdefgh");
  view->items().resize(2);
  view->items()[0].id() = 12;
  view->items()[1].id() = 34;
  EXPECT_EQ(34, view->items()[1].id());
  view->nested_variable().resize(2);
  view->nested_variable()[0].name().assign("one");
  view->nested_variable()[1].name().assign("two");
  EXPECT_EQ("two", view->nested_variable()[1].name().view());
  auto compacted = xcdr::compact_message_in_place(ts.get(), view);
  ASSERT_EQ(region.data(), compacted.location.address) << rcutils_get_error_string().str;
  rosidl_runtime_cpp::MemoryRegion<void> wire(compacted.location.address, compacted.size);
  Variable plain;
  auto plain_ts = xcdr::get_message_type_support_handle<Variable>();
  ASSERT_EQ(RCUTILS_RET_OK,
    xcdr::deserialize_message_from(plain_ts, wire, &plain));
  ASSERT_EQ(2u, plain.nested_variable.size());
  EXPECT_EQ("two", plain.nested_variable[1].name);
  ASSERT_EQ(3u, plain.names.size());
  EXPECT_EQ("short", plain.names[1]);
  EXPECT_EQ("abcdefgh", plain.names[2]);
  EXPECT_EQ(34, plain.items[1].id);
}

TEST(RegionXcdr, ConstrainedBoundedPrimitiveSequenceUsesTighterLimit)
{
  auto base = xcdr::get_message_type_support_handle<VariableView>();
  VariableView::Constraints constraints;
  constraints.numbers.size = 1;
  constraints.names.size = 1;
  constraints.names.element.size = 2;
  constraints.labels.size = 3;
  rosidl_typesupport_xcdr_c__constraints_t wrapped{};
  wrapped.type_specific = &constraints;
  auto ts = xcdr::create_constrained_message_type_support(base, &wrapped);
  ASSERT_NE(nullptr, ts) << rcutils_get_error_string().str;
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts.get(), &size));
  std::vector<uint8_t> buffer(size + 4);
  rosidl_runtime_cpp::MemoryRegion<void> region(buffer.data() + 4, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts.get(), region, &raw));
  auto * view = static_cast<VariableView *>(raw);
  EXPECT_EQ(1u, view->numbers().capacity());
  view->names().resize(1);
  EXPECT_THROW(view->names().resize(2), std::length_error);
  EXPECT_THROW(view->names()[0].assign("abc"), std::length_error);
  EXPECT_THROW(view->labels()[0].assign("abcd"), std::length_error);
  xcdr::destroy_message(ts.get(), view);

  constraints.numbers.size = 3;
  EXPECT_EQ(nullptr, xcdr::create_constrained_message_type_support(base, &wrapped));
  rcutils_reset_error();
  constraints.numbers.size = 1;
  constraints.names.element.size = 9;
  EXPECT_EQ(nullptr, xcdr::create_constrained_message_type_support(base, &wrapped));
  rcutils_reset_error();
}

TEST(RegionXcdr, ConstrainedVariableFieldsCompactAndDeserialize)
{
  auto base = xcdr::get_message_type_support_handle<UnboundedView>();
  UnboundedView::Constraints constraints;
  constraints.name.size = 8;
  constraints.data.size = 5;
  rosidl_typesupport_xcdr_c__constraints_t wrapped{};
  wrapped.type_specific = &constraints;
  wrapped.strict = true;
  auto ts = xcdr::create_constrained_message_type_support(base, &wrapped);
  ASSERT_NE(nullptr, ts);
  ASSERT_NE(nullptr, ts->func);
  EXPECT_EQ(ts.get(), ts->func(ts.get(), base->typesupport_identifier));
  auto owned = rosidl_typesupport_xcdr_c_get_constraints(ts.get());
  ASSERT_NE(nullptr, owned);
  ASSERT_NE(nullptr, owned->type_specific);
  EXPECT_NE(owned->type_specific, wrapped.type_specific);
  auto * owned_limits = static_cast<const UnboundedView::Constraints *>(owned->type_specific);
  constraints.name.size = 99;
  EXPECT_EQ(8u, owned_limits->name.size);
  constraints.name.size = 8;
  auto plain_ts = xcdr::get_message_type_support_handle<Unbounded>();
  const auto * plain_hash = plain_ts->get_type_hash_func(plain_ts);
  const auto * view_hash = base->get_type_hash_func(base);
  const auto * constrained_hash = ts->get_type_hash_func(ts.get());
  ASSERT_NE(nullptr, plain_hash);
  ASSERT_NE(nullptr, view_hash);
  ASSERT_NE(nullptr, constrained_hash);
  EXPECT_NE(0u, plain_hash->version);
  EXPECT_EQ(plain_hash->version, view_hash->version);
  EXPECT_EQ(plain_hash->version, constrained_hash->version);
  EXPECT_EQ(plain_ts->get_type_description_func(plain_ts), ts->get_type_description_func(ts.get()));
  size_t size = 0;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::get_expected_message_size(ts.get(), &size));
  std::vector<uint8_t> buffer(size + 4);
  auto * bytes = buffer.data() + 4;
  ASSERT_EQ(0u, reinterpret_cast<std::uintptr_t>(bytes + 4) % alignof(uint64_t));
  rosidl_runtime_cpp::MemoryRegion<void> region(bytes, size);
  void * raw = nullptr;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::construct_message_at(ts.get(), region, &raw));
  auto * view = static_cast<UnboundedView *>(raw);
  view->id() = 23;
  view->name().assign("hello");
  view->data().resize(2);
  view->data()[0] = 4;
  view->data()[1] = 8;
  ASSERT_EQ(RCUTILS_RET_OK, xcdr::validate_message(ts.get(), &wrapped, view));
  auto compacted = xcdr::compact_message_in_place(ts.get(), view);
  ASSERT_EQ(bytes, compacted.location.address);
  EXPECT_LT(compacted.size, size);

  Unbounded plain;
  ASSERT_EQ(
    RCUTILS_RET_OK, xcdr::deserialize_message_from(
      plain_ts, rosidl_runtime_cpp::MemoryRegion<void>(compacted), &plain));
  EXPECT_EQ(23u, plain.id);
  EXPECT_EQ("hello", plain.name);
  ASSERT_EQ(2u, plain.data.size());
  EXPECT_EQ(4u, plain.data[0]);
  EXPECT_EQ(8u, plain.data[1]);

  ASSERT_EQ(
    RCUTILS_RET_OK, xcdr::cast_message_at(
      ts.get(), rosidl_runtime_cpp::MemoryRegion<void>(compacted), &raw));
  view = static_cast<UnboundedView *>(raw);
  EXPECT_EQ("hello", view->name().view());
  ASSERT_EQ(2u, view->data().size());
  EXPECT_EQ(8u, view->data()[1]);
  xcdr::destroy_message(ts.get(), view);
}
