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

// Cross-process benchmark publisher (C++ side).
//
// Publishes Image messages on the configured topic at a fixed rate for a
// fixed duration, either as a copy baseline (standard or experimental
// Image, non-loaned) or as a constrained loaned publisher (experimental
// Image).  Emits the
// raw CSV header plus one `publish` event row per sent message on stdout;
// diagnostics go to stderr.  Intended to be paired with a subscriber
// process (C++ or Python) and orchestrated by inter_proc_benchmark.py.

#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
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
  // the publish loop.
  bench::RunContext ctx = bench::RunContext::from_config(
    cfg, bench::Process::kPub, bench::Direction::kCppToCpp, backend,
    static_cast<size_t>(cfg.duration_sec * cfg.publish_rate_hz) + 1);

  printf("%s\n", bench::csv_header().c_str());
  fflush(stdout);

  std::atomic<uint64_t> sent{0};

  // Everything from node construction on is guarded: endpoint creation
  // can fail loudly (e.g. data sharing ON with an unbounded type), and
  // that must surface as an error row, never as an abort.
  try {
    rclcpp::InitOptions init_options;
    bench::apply_backend_to_init_options(init_options, backend);
    rclcpp::init(argc, argv, init_options);
    auto node = std::make_shared<rclcpp::Node>("inter_proc_publisher");
    // One executor for the whole run: spin_some() on the free function
    // would construct and destroy an executor on every call.
    rclcpp::executors::SingleThreadedExecutor exec;
    exec.add_node(node);

    auto qos = rclcpp::QoS(rclcpp::KeepLast(cfg.qos_depth));
    if (cfg.reliability == bench::Reliability::kBestEffort) {
      qos.reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
    }

    // Wait for a matching subscriber before publishing: in a two-process
    // setup DDS discovery takes longer than the first timer tick.
    auto wait_for_match = [&](rclcpp::PublisherBase & pub) {
        auto wait_start = std::chrono::steady_clock::now();
        while (pub.get_subscription_count() == 0 &&
          std::chrono::steady_clock::now() - wait_start < std::chrono::seconds(10))
        {
          exec.spin_some();
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        fprintf(stderr, "DEBUG wait_for_match: sub_count=%zu elapsed=%.2fs\n",
          pub.get_subscription_count(),
          std::chrono::duration<double>(std::chrono::steady_clock::now() - wait_start).count());
        fprintf(stderr, "PUB_READY\n");
        // Barrier: the orchestrator releases the publish loop only after
        // the subscriber signals its window is open. Publishing earlier
        // loses head samples, since discovery is asymmetric: seeing the
        // subscription does not mean the subscription sees us yet.
        {
          std::string go;
          if (!std::getline(std::cin, go)) {
            throw std::runtime_error("stdin closed before go-signal");
          }
        }
      };

    auto emit_sent = [&](uint64_t seq, int64_t send_ns) {
        ctx.emit(bench::Event::kPublish, seq, send_ns, -1, -1, -1);
        sent.fetch_add(1);
      };

    // Pre-allocate once: fill_image overwrites in place, and
    // data.resize() is a no-op once the buffer holds the run's
    // fixed size.  Only the per-sample stamp/frame_id change below.
    auto publish_copy = [&](auto & pub, auto & msg) {
        wait_for_match(*pub);
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
            StdImage msg;
            bench::fill_image(msg, cfg.payload_bytes, 0,
              cfg.fill_encoding, cfg.fill_frame_id, cfg.fill_data_ratio);
            publish_copy(pub, msg);
          } else {
            auto pub = node->create_publisher<bench::ExperimentalImage>(
              cfg.topic, qos);
            bench::ExperimentalImage msg;
            bench::fill_image(msg, cfg.payload_bytes, 0,
              cfg.fill_encoding, cfg.fill_frame_id, cfg.fill_data_ratio);
            publish_copy(pub, msg);
          }
          break;
        }

      case bench::Config::kConstrainedPubOnly:
      case bench::Config::kConstrainedPubSub: {
          auto constraints = bench::make_inter_proc_constraints(cfg.payload_bytes, cfg.strict);
          auto pub = node->create_publisher<bench::ExperimentalImage>(
            cfg.topic, qos, constraints);
          if (!pub->can_loan_messages()) {
            throw std::runtime_error("middleware does not support loaned messages");
          }
          wait_for_match(*pub);
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
    // Single write of every buffered row, after the publish loop.
    ctx.flush_rows();
  } catch (const std::exception & e) {
    ctx.emit(bench::Event::kError, 0, -1, -1, -1, -1, e.what());
    ctx.flush_rows();
    fprintf(stderr, "publisher error: %s\n", e.what());
    rclcpp::shutdown();
    return 1;
  }

  fprintf(stderr, "DONE sent=%s run_id=%s\n",
    std::to_string(sent.load()).c_str(), ctx.run_id.c_str());
  rclcpp::shutdown();
  return 0;
}
