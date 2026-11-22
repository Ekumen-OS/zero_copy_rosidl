// Copyright 2026 Open Source Robotics Foundation, Inc.
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

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "rcpputils/scope_exit.hpp"
#include "rcutils/env.h"
#include "rclcpp/rclcpp.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/bounded_variable.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/bounded_variable__region.hpp"

using Plain = rosidl_typesupport_xcdr_cpp_tests::msg::BoundedVariable;
using View = rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionBoundedVariable;
using Constraints = rosidl_runtime_cpp::views::MessageConstraints<View>;
using namespace std::chrono_literals;

class RegionRclcpp : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rclcpp::init(0, nullptr);
    node = std::make_shared<rclcpp::Node>("region_rclcpp_test");
    executor = std::make_unique<rclcpp::executors::SingleThreadedExecutor>();
    executor->add_node(node);
  }

  void TearDown() override
  {
    executor->remove_node(node);
    executor.reset();
    node.reset();
    if (rclcpp::ok()) {
      rclcpp::shutdown();
    }
  }

  void wait_for_subscribers(const std::shared_ptr<rclcpp::PublisherBase> & publisher, size_t count)
  {
    for (int i = 0; i < 200 && publisher->get_subscription_count() < count; ++i) {
      std::this_thread::sleep_for(10ms);
    }
    ASSERT_GE(publisher->get_subscription_count(), count);
  }

  template<typename Predicate>
  void spin_until(Predicate predicate)
  {
    for (int i = 0; i < 200 && !predicate(); ++i) {
      executor->spin_some();
      std::this_thread::sleep_for(10ms);
    }
    ASSERT_TRUE(predicate());
  }

  std::shared_ptr<rclcpp::Node> node;
  std::unique_ptr<rclcpp::executors::SingleThreadedExecutor> executor;
};

TEST_F(RegionRclcpp, loaned_publisher_reaches_region_and_ordinary_subscribers)
{
  Constraints limits;
  std::string received_view;
  std::string received_plain;
  auto region_sub = node->create_subscription<View>(
    "region_to_both", 10,
    [&](const View & msg) {
      received_view = msg.name().c_str();
      EXPECT_EQ(u"wide", msg.wide().to_string());
      EXPECT_EQ(42u, msg.tail());
    }, limits);
  auto plain_sub = node->create_subscription<Plain>(
    "region_to_both", 10,
    [&](const Plain & msg) {
      received_plain = msg.name;
      EXPECT_EQ(u"wide", msg.wide);
      EXPECT_EQ(42u, msg.tail);
    });
  auto publisher = node->create_publisher<View>("region_to_both", 10, limits);
  wait_for_subscribers(publisher, 2);

  auto loan = publisher->borrow_loaned_message();
  loan.get().name() = "hello";
  loan.get().wide() = u"wide";
  loan.get().tail() = 42;
  publisher->publish(std::move(loan));
  EXPECT_FALSE(loan.is_valid());
  spin_until([&]() {return !received_view.empty() && !received_plain.empty();});
  EXPECT_EQ("hello", received_view);
  EXPECT_EQ("hello", received_plain);
}

TEST_F(RegionRclcpp, ordinary_publisher_reaches_region_subscription)
{
  Constraints limits;
  std::string received;
  auto sub = node->create_subscription<View>(
    "plain_to_region", 10,
    [&](const View & msg, const rclcpp::MessageInfo &) {received = msg.name().c_str();}, limits);
  auto publisher = node->create_publisher<Plain>("plain_to_region", 10);
  wait_for_subscribers(publisher, 1);
  Plain message;
  message.name = "plain";
  publisher->publish(message);
  spin_until([&]() {return !received.empty();});
  EXPECT_EQ("plain", received);
}

TEST_F(RegionRclcpp, subscriber_enforces_its_own_baseline)
{
  Constraints pub_limits;
  pub_limits.type_specific.name.size = 8;
  Constraints sub_limits;
  sub_limits.type_specific.name.size = 2;
  sub_limits.strict = true;
  std::string received;
  int callbacks = 0;
  auto sub = node->create_subscription<View>(
    "region_independent_limits", 10,
    [&](const View & msg) {
      ++callbacks;
      received = msg.name().c_str();
    }, sub_limits);
  auto publisher = node->create_publisher<View>("region_independent_limits", 10, pub_limits);
  wait_for_subscribers(publisher, 1);
  for (const char * name : {"long", "ok"}) {
    auto loan = publisher->borrow_loaned_message();
    loan.get().name() = name;
    publisher->publish(std::move(loan));
  }
  spin_until([&]() {return callbacks > 0;});
  EXPECT_EQ(1, callbacks);
  EXPECT_EQ("ok", received);
}

TEST_F(RegionRclcpp, loans_are_returned_and_refinements_are_enforced)
{
  Constraints limits;
  limits.type_specific.name.size = 8;
  limits.strict = true;
  auto publisher = node->create_publisher<View>("region_loan_lifetime", 10, limits);
  {
    auto loan = publisher->borrow_loaned_message();
    loan.get().name() = "unused";
  }
  auto other = node->create_publisher<View>("region_other_publisher", 10, limits);
  {
    auto loan = publisher->borrow_loaned_message();
    EXPECT_THROW(other->publish(std::move(loan)), std::invalid_argument);
    EXPECT_TRUE(loan.is_valid());
  }
  Constraints tighter;
  tighter.type_specific.name.size = 2;
  {
    auto loan = publisher->borrow_loaned_message(tighter);
    loan.get().name() = "abc";
    EXPECT_THROW(publisher->publish(std::move(loan)), std::exception);
    EXPECT_FALSE(loan.is_valid());
  }
  EXPECT_TRUE(publisher->borrow_loaned_message().is_valid());
  Constraints looser;
  looser.type_specific.name.size = 9;
  EXPECT_THROW(publisher->borrow_loaned_message(looser), std::exception);
}

TEST_F(RegionRclcpp, callback_exception_returns_subscription_loan)
{
  Constraints limits;
  bool fail_callback = true;
  int callbacks = 0;
  auto sub = node->create_subscription<View>(
    "region_throwing_callback", 10,
    [&](const View &) {
      ++callbacks;
      if (fail_callback) {
        throw std::runtime_error("callback failure");
      }
    }, limits);
  auto publisher = node->create_publisher<View>("region_throwing_callback", 10, limits);
  wait_for_subscribers(publisher, 1);
  auto first = publisher->borrow_loaned_message();
  first.get().name() = "first";
  publisher->publish(std::move(first));
  bool threw = false;
  for (int i = 0; i < 200 && !threw; ++i) {
    try {
      executor->spin_some();
    } catch (const std::runtime_error & error) {
      EXPECT_STREQ("callback failure", error.what());
      threw = true;
    }
    std::this_thread::sleep_for(10ms);
  }
  ASSERT_TRUE(threw);
  fail_callback = false;
  auto second = publisher->borrow_loaned_message();
  second.get().name() = "second";
  publisher->publish(std::move(second));
  spin_until([&]() {return callbacks == 2;});
}

TEST_F(RegionRclcpp, publisher_loans_can_be_returned_after_shutdown)
{
  Constraints limits;
  auto publisher = node->create_publisher<View>("region_shutdown", 10, limits);
  void * raw = nullptr;
  ASSERT_EQ(
    RCL_RET_OK, rcl_borrow_loaned_message_with_context(
      publisher->get_publisher_handle().get(),
      rosidl_typesupport_cpp::get_message_type_support_handle<View>(), nullptr, &raw));
  auto loan = publisher->borrow_loaned_message();
  ASSERT_TRUE(rclcpp::shutdown());
  EXPECT_THROW(publisher->publish(std::move(loan)), std::exception);
  EXPECT_TRUE(loan.is_valid());
  EXPECT_EQ(
    RCL_RET_OK,
    rcl_return_loaned_message_from_publisher(publisher->get_publisher_handle().get(), raw));
}

TEST_F(RegionRclcpp, explicitly_disabled_loans_are_rejected)
{
  const char * previous = nullptr;
  ASSERT_EQ(nullptr, rcutils_get_env("ROS_DISABLE_LOANED_MESSAGES", &previous));
  const std::string saved = previous;
  auto restore = rcpputils::make_scope_exit([&saved]() {
        EXPECT_TRUE(rcutils_set_env("ROS_DISABLE_LOANED_MESSAGES", saved.c_str()));
    });
  ASSERT_TRUE(rcutils_set_env("ROS_DISABLE_LOANED_MESSAGES", "1"));
  Constraints limits;
  EXPECT_THROW(
    node->create_subscription<View>(
      "disabled_region", 10, [](const View &) {}, limits),
    std::runtime_error);
  EXPECT_THROW(
    node->create_publisher<View>("disabled_region", 10, limits), std::runtime_error);
}
