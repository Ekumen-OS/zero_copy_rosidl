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

#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include <sensor_msgs/msg/image.hpp>

#include "constrained_pubsub_benchmark/benchmark_common.hpp"
#include "constrained_pubsub_benchmark/sample_record.hpp"
#include "constrained_pubsub_benchmark/transport_config.hpp"

namespace bench = constrained_pubsub_benchmark;

#ifdef BENCHMARK_ROLLING_VIEWS
TEST(SmokeTest, LoanedImageReachesRegionAndOrdinarySubscribers) {
  rclcpp::init(0, nullptr);
  {
    auto node = std::make_shared<rclcpp::Node>("benchmark_smoke");
    const auto topic = "/benchmark_smoke_" + std::to_string(getpid());
    auto constraints = bench::make_inter_proc_constraints(256, true);
    auto publisher = node->create_publisher<bench::ExperimentalImage>(topic, 10, constraints);
    bool region_received = false;
    bool ordinary_received = false;
    auto region_subscriber = node->create_subscription<bench::ExperimentalImage>(
      topic, 10,
      [&](const bench::ExperimentalImage & msg, const rclcpp::MessageInfo &) {
        uint64_t seq = 0;
        int64_t send_ns = 0;
        EXPECT_FALSE(msg.owns_storage());
        EXPECT_TRUE(bench::decode_sample_metadata(msg, &seq, &send_ns));
        EXPECT_EQ(7u, seq);
        EXPECT_EQ(1234567890, send_ns);
        EXPECT_EQ(256u, msg.data().size());
        EXPECT_EQ(0xfe, msg.data()[0]);
        const auto block = msg.backing_storage();
        const auto begin = reinterpret_cast<std::uintptr_t>(block.data());
        const auto payload = reinterpret_cast<std::uintptr_t>(msg.data().data());
        EXPECT_GE(payload, begin);
        EXPECT_LE(payload + msg.data().size(), begin + block.size());
        region_received = true;
      }, constraints);
    auto ordinary_subscriber = node->create_subscription<sensor_msgs::msg::Image>(
      topic, 10,
      [&](const sensor_msgs::msg::Image & msg) {
        uint64_t seq = 0;
        int64_t send_ns = 0;
        EXPECT_TRUE(bench::decode_sample_metadata(msg, &seq, &send_ns));
        EXPECT_EQ(7u, seq);
        EXPECT_EQ(1234567890, send_ns);
        ASSERT_EQ(256u, msg.data.size());
        EXPECT_EQ(0xfe, msg.data[0]);
        ordinary_received = true;
      });
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while ((publisher->get_subscription_count() < 2 ||
      region_subscriber->get_publisher_count() == 0 ||
      ordinary_subscriber->get_publisher_count() == 0) &&
      std::chrono::steady_clock::now() < deadline)
    {
      executor.spin_some();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_EQ(2u, publisher->get_subscription_count());
    ASSERT_EQ(1u, region_subscriber->get_publisher_count());
    ASSERT_EQ(1u, ordinary_subscriber->get_publisher_count());
    auto loan = publisher->borrow_loaned_message();
    auto & view = loan.get();
    EXPECT_FALSE(view.owns_storage());
    bench::fill_image(view, 256, 7, 0, 0, 1.0);
    bench::encode_sample_metadata(view, 7, 1234567890);
    const auto block = view.backing_storage();
    const auto begin = reinterpret_cast<std::uintptr_t>(block.data());
    const auto payload = reinterpret_cast<std::uintptr_t>(view.data().data());
    EXPECT_GE(payload, begin);
    EXPECT_LE(payload + view.data().size(), begin + block.size());
    publisher->publish(std::move(loan));
    EXPECT_FALSE(loan.is_valid());
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!(region_received && ordinary_received) &&
      std::chrono::steady_clock::now() < deadline)
    {
      executor.spin_once(std::chrono::milliseconds(20));
    }
    EXPECT_TRUE(region_received);
    EXPECT_TRUE(ordinary_received);
  }
  rclcpp::shutdown();
}
#else
TEST(SmokeTest, CopyImageMetadataRoundTrip) {
  sensor_msgs::msg::Image msg;
  bench::fill_image(msg, 256, 7, 0, 0, 1.0);
  bench::encode_sample_metadata(msg, 7, 1234567890);
  uint64_t seq = 0;
  int64_t send_ns = 0;
  ASSERT_TRUE(bench::decode_sample_metadata(msg, &seq, &send_ns));
  EXPECT_EQ(7u, seq);
  EXPECT_EQ(1234567890, send_ns);
  EXPECT_EQ(256u, msg.data.size());
}
#endif

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
