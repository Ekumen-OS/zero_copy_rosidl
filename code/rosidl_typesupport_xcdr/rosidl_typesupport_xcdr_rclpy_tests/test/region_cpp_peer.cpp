// Copyright 2026 Ekumen, Inc.
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

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "rclcpp/rclcpp.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/bounded_variable__region.hpp"

using View = rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionBoundedVariable;
using Constraints = rosidl_runtime_cpp::views::MessageConstraints<View>;
using namespace std::chrono_literals;

int main(int argc, char ** argv)
{
  if (argc != 2) {
    std::cerr << "Expected publisher or subscriber mode\n";
    return 2;
  }
  std::string mode(argv[1]);
  rclcpp::init(0, nullptr);
  auto node = std::make_shared<rclcpp::Node>("region_cpp_peer");
  Constraints limits;
  auto deadline = std::chrono::steady_clock::now() + 8s;
  bool success = false;
  if (mode == "publisher") {
    auto publisher = node->create_publisher<View>("cpp_to_python", 10, limits);
    while (std::chrono::steady_clock::now() < deadline &&
      publisher->get_subscription_count() == 0)
    {
      std::this_thread::sleep_for(10ms);
    }
    if (publisher->get_subscription_count() > 0) {
      auto loan = publisher->borrow_loaned_message();
      loan.get().name() = "cpp";
      loan.get().tail() = 25;
      publisher->publish(std::move(loan));
      std::this_thread::sleep_for(200ms);
      success = true;
    }
  } else if (mode == "subscriber") {
    auto subscription = node->create_subscription<View>(
      "python_to_cpp", 10,
      [&](const View & view) {
        success = std::string(view.name().c_str()) == "python" && view.tail() == 31;
      }, limits);
    while (std::chrono::steady_clock::now() < deadline && !success) {
      rclcpp::spin_some(node);
      std::this_thread::sleep_for(10ms);
    }
  } else {
    std::cerr << "Unknown mode\n";
  }
  node.reset();
  rclcpp::shutdown();
  return success ? 0 : 1;
}
