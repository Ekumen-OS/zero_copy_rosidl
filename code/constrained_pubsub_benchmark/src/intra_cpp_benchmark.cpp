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

// Single-process benchmark (C++ side).
//
// Publishes and subscribes Image messages on one node at a fixed rate for
// a fixed duration, covering all five configurations (standard and
// experimental messages, FastCDR/XCDR backends, copy and constrained
// loaned paths).  ROS intra-process communication is explicitly disabled
// so the selected DDS transport (UDP/SHM/data sharing) is exercised.
// Emits the raw CSV header plus one event row per published/received
// message on stdout; diagnostics go to stderr.

#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/executors/single_threaded_executor.hpp>
#include <sensor_msgs/msg/experimental/image.hpp>
#include <sensor_msgs/msg/image.hpp>

#include "constrained_pubsub_benchmark/benchmark_common.hpp"
#include "constrained_pubsub_benchmark/config.hpp"
#include "constrained_pubsub_benchmark/sample_record.hpp"
#include "constrained_pubsub_benchmark/transport_config.hpp"

namespace bench = constrained_pubsub_benchmark;

using StdImage = sensor_msgs::msg::Image;

int main(int argc, char ** argv)
{
  auto cfg = bench::load_config_or_exit(argc, argv);
  const bench::Backend backend =
    bench::resolve_backend(cfg.message, cfg.config, cfg.backend);

  // One buffer for publish and receive rows (post-processing joins on
  // sequence, so interleaving is fine).  Receives never exceed publishes.
  bench::RunContext ctx = bench::RunContext::from_config(
    cfg, bench::Process::kIntra, bench::Direction::kIntraCpp, backend,
    2 * (static_cast<size_t>(cfg.duration_sec * cfg.publish_rate_hz) + 1));

  printf("%s\n", bench::csv_header().c_str());
  fflush(stdout);

  std::atomic<size_t> received{0};
  std::function<void(uint64_t, bool, int64_t, const rclcpp::MessageInfo &)>
  on_message =
    [&](uint64_t seq, bool decoded, int64_t send_ns,
    const rclcpp::MessageInfo & info)
    {
      const auto & rmw_info = info.get_rmw_message_info();
      int64_t recv_ns = bench::mono_now_ns();
      if (!decoded) {
        ctx.emit(
          bench::Event::kError, 0, send_ns, recv_ns,
          rmw_info.source_timestamp, rmw_info.received_timestamp,
          "foreign frame_id");
        return;
      }
      ctx.emit(
        bench::Event::kReceive, seq, send_ns, recv_ns,
        rmw_info.source_timestamp, rmw_info.received_timestamp);
      received.fetch_add(1);
    };

  std::atomic<uint64_t> sent{0};

  // Everything from node construction on is guarded: endpoint creation
  // can fail loudly (e.g. data sharing ON with an unbounded type), and
  // that must surface as an error row, never as an abort.
  try {
    rclcpp::InitOptions init_options;
    bench::apply_backend_to_init_options(init_options, backend);
    rclcpp::init(argc, argv, init_options);
    // NOTE: intra-process comms stay disabled (the default) so pub/sub
    // traffic traverses the configured DDS transport.
    auto node = std::make_shared<rclcpp::Node>(
      "intra_cpp_benchmark", rclcpp::NodeOptions().use_intra_process_comms(false));
    // One executor for the whole run: spin_some() on the free function
    // would construct and destroy an executor on every call.
    rclcpp::executors::SingleThreadedExecutor exec;
    exec.add_node(node);

    auto qos = rclcpp::QoS(rclcpp::KeepLast(cfg.qos_depth));
    if (cfg.reliability == bench::Reliability::kBestEffort) {
      qos.reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
    }

    auto emit_sent = [&](uint64_t seq, int64_t send_ns) {
        ctx.emit(bench::Event::kPublish, seq, send_ns, -1, -1, -1);
        sent.fetch_add(1);
      };

  // Mutual discovery before the window: same-participant matching still
  // needs a spin to complete; without it the first samples are lost.
    auto wait_for_local_match = [&](rclcpp::PublisherBase & pub,
      rclcpp::SubscriptionBase & sub) {
        auto wait_start = std::chrono::steady_clock::now();
        while ((pub.get_subscription_count() == 0 || sub.get_publisher_count() == 0) &&
          std::chrono::steady_clock::now() - wait_start < std::chrono::seconds(30))
        {
          exec.spin_some();
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        fprintf(stderr, "DEBUG local_match: elapsed=%.2fs\n",
          std::chrono::duration<double>(
            std::chrono::steady_clock::now() - wait_start).count());
        if (pub.get_subscription_count() == 0 || sub.get_publisher_count() == 0) {
          throw std::runtime_error("intra-process endpoints did not match within 30s");
        }
      };

    // The subscription must outlive the publish loop: the grace drain
    // after the loop still dispatches in-flight tail samples.  A
    // branch-local handle would unsubscribe at the closing brace.
    std::shared_ptr<rclcpp::SubscriptionBase> sub;

    // Shared subscription callback for experimental messages; the
    // constrained variant (c2) only adds constraints to the subscription.
    auto on_exp_message =
      [&](const bench::ExperimentalImage & msg, const rclcpp::MessageInfo & info) {
        uint64_t seq = 0;
        int64_t send_ns = 0;
        bool ok = bench::decode_sample_metadata(msg, &seq, &send_ns);
        on_message(seq, ok, send_ns, info);
      };

    // Pre-allocated copy message: fill_image overwrites in place, and only
    // the per-sample stamp/frame_id change inside the loop.
    auto run_copy_loop = [&](auto & pub, auto & msg) {
        bench::run_fixed_rate_loop(exec,
          [&](uint64_t seq) {
            int64_t send_ns = bench::mono_now_ns();
            bench::encode_sample_metadata(msg, seq, send_ns);
            pub->publish(msg);
            emit_sent(seq, send_ns);
          },
          cfg.publish_rate_hz, cfg.duration_sec);
      };

    switch (cfg.config) {
      case bench::Config::kCopy: {
          if (cfg.message == bench::MessageType::kStandard) {
            auto pub = node->create_publisher<StdImage>(cfg.topic, qos);
            sub = node->create_subscription<StdImage>(
              cfg.topic, qos,
              [&](const StdImage & msg, const rclcpp::MessageInfo & info) {
                uint64_t seq = 0;
                int64_t send_ns = 0;
                bool ok = bench::decode_sample_metadata(msg, &seq, &send_ns);
                on_message(seq, ok, send_ns, info);
              });
            wait_for_local_match(*pub, *sub);
            StdImage msg;
            bench::fill_image(msg, cfg.payload_bytes, 0,
              cfg.fill_encoding, cfg.fill_frame_id, cfg.fill_data_ratio);
            run_copy_loop(pub, msg);
          } else {
            auto pub = node->create_publisher<bench::ExperimentalImage>(
              cfg.topic, qos);
            sub = node->create_subscription<bench::ExperimentalImage>(
              cfg.topic, qos, on_exp_message);
            wait_for_local_match(*pub, *sub);
            bench::ExperimentalImage msg;
            bench::fill_image(msg, cfg.payload_bytes, 0,
              cfg.fill_encoding, cfg.fill_frame_id, cfg.fill_data_ratio);
            run_copy_loop(pub, msg);
          }
          break;
        }

      case bench::Config::kConstrainedPubOnly:
      case bench::Config::kConstrainedPubSub: {
          auto constraints =
            bench::make_inter_proc_constraints(cfg.payload_bytes, cfg.strict);
          auto pub = node->create_publisher<bench::ExperimentalImage>(
            cfg.topic, qos, constraints);
          if (!pub->can_loan_messages()) {
            throw std::runtime_error("middleware does not support loaned messages");
          }
          if (cfg.config == bench::Config::kConstrainedPubSub) {
            sub = node->create_subscription<bench::ExperimentalImage>(
              cfg.topic, qos, on_exp_message, constraints);
          } else {
            sub = node->create_subscription<bench::ExperimentalImage>(
              cfg.topic, qos, on_exp_message);
          }
          wait_for_local_match(*pub, *sub);
          bench::run_fixed_rate_loop(exec,
            [&](uint64_t seq) {
              auto loaned = pub->borrow_loaned_message();
              bench::fill_image(loaned.get(), cfg.payload_bytes, seq,
                cfg.fill_encoding, cfg.fill_frame_id, cfg.fill_data_ratio);
              int64_t send_ns = bench::mono_now_ns();
              bench::encode_sample_metadata(loaned.get(), seq, send_ns);
              pub->publish(std::move(loaned));
              emit_sent(seq, send_ns);
            },
            cfg.publish_rate_hz, cfg.duration_sec);
          break;
        }
    }

    // Grace drain to catch in-flight messages.  spin_once with a timeout
    // (rather than a bare spin_some busy loop) so each iteration waits for
    // newly arrived work instead of re-polling a possibly stale snapshot.
    auto grace_end = bench::Clock::now() + bench::Duration(cfg.grace_sec);
    while (bench::Clock::now() < grace_end) {
      exec.spin_once(std::chrono::milliseconds(20));
    }

    // Single write of every buffered row, after the window closes.
    ctx.flush_rows();

    fprintf(stderr, "DONE sent=%s received=%s run_id=%s\n",
      std::to_string(sent.load()).c_str(), std::to_string(received.load()).c_str(),
      ctx.run_id.c_str());
  } catch (const std::exception & e) {
    ctx.emit(bench::Event::kError, 0, -1, -1, -1, -1, e.what());
    ctx.flush_rows();
    fprintf(stderr, "benchmark error: %s\n", e.what());
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
