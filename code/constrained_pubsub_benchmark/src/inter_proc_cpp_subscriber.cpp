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

// Cross-process benchmark subscriber (C++ side).
//
// Receives Image messages on the configured topic, either as a copy
// baseline (standard or experimental Image) or with an experimental Image
// subscription (optionally constrained for constrained_pub_sub).  Emits
// the raw CSV header plus
// one `receive` event row per message on stdout; prints READY on stderr
// once the subscription exists.  Intended to be paired with a publisher
// process (C++ or Python) and orchestrated by inter_proc_benchmark.py.

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

  // Buffer every row; a single write at the end keeps stdout I/O out of
  // the subscription callback.  Receives never exceed publishes.
  bench::RunContext ctx = bench::RunContext::from_config(
    cfg, bench::Process::kSub, bench::Direction::kCppToCpp, backend,
    static_cast<size_t>(
      (cfg.duration_sec + cfg.grace_sec) * cfg.publish_rate_hz) + 1);

  printf("%s\n", bench::csv_header().c_str());
  fflush(stdout);

  std::atomic<size_t> received{0};

  // Everything from node construction on is guarded: endpoint creation
  // can fail loudly (e.g. data sharing ON with an unbounded type), and
  // that must surface as an error row, never as an abort.
  try {
    rclcpp::InitOptions init_options;
    bench::apply_backend_to_init_options(init_options, backend);
    rclcpp::init(argc, argv, init_options);
    auto node = std::make_shared<rclcpp::Node>("inter_proc_subscriber");
    // One executor for the whole run: spin_some() on the free function
    // would construct and destroy an executor on every call.
    rclcpp::executors::SingleThreadedExecutor exec;
    exec.add_node(node);

    auto qos = rclcpp::QoS(rclcpp::KeepLast(cfg.qos_depth));
    if (cfg.reliability == bench::Reliability::kBestEffort) {
      qos.reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
    }
    std::shared_ptr<rclcpp::SubscriptionBase> sub_handle;

  // Shared per-message handling: decode metadata, emit one raw row.
  // Foreign traffic (undecodable frame_id) becomes an error row, never a
  // receive row, so sequence joins stay exact.
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

    // The message selects the subscription type; only a constrained
    // subscriber (c2) adds constraints.  A constrained publisher (c1)
    // pairs with a plain experimental subscription here.
    auto subscribe_exp = [&](auto constraints) {
        sub_handle = node->create_subscription<bench::ExperimentalImage>(
        cfg.topic, qos,
          [&](const bench::ExperimentalImage & msg, const rclcpp::MessageInfo & info) {
            uint64_t seq = 0;
            int64_t send_ns = 0;
            bool ok = bench::decode_sample_metadata(msg, &seq, &send_ns);
            on_message(seq, ok, send_ns, info);
        },
        constraints);
      };

    if (cfg.message == bench::MessageType::kStandard) {
      sub_handle = node->create_subscription<StdImage>(
      cfg.topic, qos,
        [&](const StdImage & msg, const rclcpp::MessageInfo & info) {
          uint64_t seq = 0;
          int64_t send_ns = 0;
          bool ok = bench::decode_sample_metadata(msg, &seq, &send_ns);
          on_message(seq, ok, send_ns, info);
      });
    } else if (cfg.config == bench::Config::kConstrainedPubSub) {
      auto constraints =
        bench::make_inter_proc_constraints(cfg.payload_bytes, cfg.strict);
      subscribe_exp(constraints);
    } else {
      subscribe_exp(rclcpp::SubscriptionOptions());
    }

  // Signal readiness to the orchestrator once the subscription exists.
    fprintf(stderr, "READY\n");

  // Mutual discovery: wait for the publisher before starting the receive
  // window, otherwise the window burns while unmatched and head samples
  // are recorded as drops.
    {
      auto wait_start = std::chrono::steady_clock::now();
      while (sub_handle->get_publisher_count() == 0 &&
        std::chrono::steady_clock::now() - wait_start < std::chrono::seconds(30))
      {
        exec.spin_some();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
      fprintf(stderr, "DEBUG wait_for_pub_match: pub_count=%zu elapsed=%.2fs\n",
        sub_handle->get_publisher_count(),
        std::chrono::duration<double>(std::chrono::steady_clock::now() - wait_start).count());
      if (sub_handle->get_publisher_count() == 0) {
        throw std::runtime_error("no matching publisher within 30s");
      }
    }

  // The publisher blocks on a go-signal until it sees this marker, so no
  // sample is published before the receive window opens.
    fprintf(stderr, "SUB_MATCHED\n");

    // spin_once with a timeout (rather than a bare spin_some busy loop)
    // so each iteration waits for newly arrived work instead of re-polling
    // a possibly stale snapshot.  10 ms (not 50): longer quanta phase-lock
    // against rigid publish grids and inflate the tail; measured 50->5 ms
    // taking cpp_to_py 1M@10Hz from ~490 to ~342 mean.
    auto t_start = bench::Clock::now();
    auto t_end = t_start + bench::Duration(cfg.duration_sec + cfg.grace_sec);
    while (bench::Clock::now() < t_end) {
      exec.spin_once(std::chrono::milliseconds(10));
    }
  // One final drain to catch in-flight messages.
    exec.spin_once(std::chrono::milliseconds(100));

    // Single write of every buffered row, after the receive window closes.
    ctx.flush_rows();

    fprintf(stderr, "DONE received=%s run_id=%s\n",
    std::to_string(received.load()).c_str(), ctx.run_id.c_str());
  } catch (const std::exception & e) {
    ctx.emit(bench::Event::kError, 0, -1, -1, -1, -1, e.what());
    ctx.flush_rows();
    fprintf(stderr, "subscriber error: %s\n", e.what());
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
