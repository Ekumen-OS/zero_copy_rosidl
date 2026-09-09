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

#ifndef CONSTRAINED_PUBSUB_BENCHMARK__BENCHMARK_COMMON_HPP_
#define CONSTRAINED_PUBSUB_BENCHMARK__BENCHMARK_COMMON_HPP_

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/executors/single_threaded_executor.hpp>

#include <constrained_pubsub_benchmark/config.hpp>

namespace constrained_pubsub_benchmark
{

using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::duration<double>;

// ── Message shaping ──────────────────────────────────────────────────────
// Fills a sensor_msgs-style Image message.  The two shape knobs drive XCDR
// compaction behavior for CONSTRAINED (loaned) publishers, under a fixed
// constraint set (encoding bound 64, data bound = payload_bytes):
//   - fill_encoding > 0: encoding string is exactly this many chars.
//     0 = "rgb8" (4 chars).  Compaction fires whenever the actual encoding
//     size is below its bound (64).
//   - fill_frame_id > 0: header.frame_id is exactly this many chars.
//     0 = "benchmark" (9 chars).  Compaction fires whenever the nested
//     frame_id is below its bound (16).
//   - fill_data_ratio in (0,1]: data is exactly this fraction of payload_bytes.
//     0.0 = legacy square-image logic (data = dim*dim*3 <= payload_bytes).
//     Compaction fires whenever data is below its bound (payload_bytes).
//   - fill_encoding 64 AND fill_frame_id 16 AND ratio 1.0: the message fills
//     every bound, so compact_message_in_place() is a true no-op.
template<typename ImageT>
void fill_image(
  ImageT & msg,
  size_t payload_bytes,
  uint64_t seq,
  size_t fill_encoding,
  size_t fill_frame_id,
  double fill_data_ratio)
{
  msg.header.stamp.sec = static_cast<int32_t>(seq >> 32);
  msg.header.stamp.nanosec = static_cast<uint32_t>(seq & 0xFFFFFFFF);
  if (fill_frame_id > 0) {
    std::string padded_frame(fill_frame_id, 'f');
    msg.header.frame_id.assign(padded_frame);
  } else {
    msg.header.frame_id.assign("benchmark");
  }
  msg.is_bigendian = false;

  if (fill_encoding > 0) {
    std::string padded(fill_encoding, 'x');
    msg.encoding.assign(padded);
  } else {
    msg.encoding.assign("rgb8");
  }

  if (fill_data_ratio > 0.0) {
    // Fill exactly payload_bytes * fill_data_ratio bytes with a consistent
    // (approximate) geometry; width is rounded up so the buffer is exact.
    size_t want = static_cast<size_t>(payload_bytes * fill_data_ratio);
    if (want < 1) {want = 1;}
    uint32_t h = static_cast<uint32_t>(std::sqrt(static_cast<double>(want) / 3.0));
    if (h < 1) {h = 1;}
    uint32_t w = static_cast<uint32_t>(
      std::ceil(static_cast<double>(want) / (3.0 * static_cast<double>(h))));
    if (w < 1) {w = 1;}
    msg.height = h;
    msg.width = w;
    msg.step = w * 3;
    msg.data.resize(want);
    std::memset(msg.data.data(), 0xFE, want);
  } else {
    // Legacy: square image whose size is the largest dim*dim*3 <= payload_bytes.
    uint32_t dim = static_cast<uint32_t>(std::sqrt(payload_bytes / 3));
    if (dim < 1) {dim = 1;}
    uint32_t actual = dim * dim * 3;
    msg.height = dim;
    msg.width = dim;
    msg.step = dim * 3;
    msg.data.resize(actual);
    std::memset(msg.data.data(), 0xFE, actual);
  }
}

// ── Publish loop templates ───────────────────────────────────────────────

/// Publishes at a fixed rate for a fixed duration, calling
/// publish_one(seq) with seq = 0, 1, 2, ...  Never bursts to catch up:
/// when behind schedule the next deadline resets to now.  Spins the given
/// executor each iteration so discovery and events stay alive; the caller
/// owns the executor (constructed once, reused everywhere).
///
/// jitter_frac perturbs each deadline by uniform(-jitter_frac,
/// +jitter_frac) * period from a PRNG seeded with jitter_seed, breaking
/// rigid-grid beating against fixed-period middleware timers while keeping
/// the mean rate exact.  0.0 = exact metronome.  The dither stream is
/// deterministic per (seed) so runs stay reproducible.
template<typename PublishFn>
void run_fixed_rate_loop(
  rclcpp::executors::SingleThreadedExecutor & exec,
  PublishFn publish_one,
  double rate_hz,
  double duration_sec,
  double jitter_frac = 0.0,
  uint64_t jitter_seed = 42)
{
  if (!(rate_hz > 0.0)) {
    throw std::invalid_argument("publish rate must be positive");
  }
  if (!(duration_sec > 0.0)) {
    throw std::invalid_argument("duration must be positive");
  }
  if (!(jitter_frac >= 0.0) || !(jitter_frac < 1.0)) {
    throw std::invalid_argument("publish jitter must be in [0.0, 1.0)");
  }
  auto period = std::chrono::microseconds(static_cast<int64_t>(1e6 / rate_hz));
  std::mt19937_64 rng(jitter_seed);
  std::uniform_real_distribution<double> jitter(-jitter_frac, jitter_frac);
  auto t_start = Clock::now();
  auto t_end = t_start + Duration(duration_sec);
  auto next = t_start;
  uint64_t seq = 0;
  while (Clock::now() < t_end) {
    publish_one(seq++);
    exec.spin_some();
    next += period;
    if (jitter_frac > 0.0) {
      next += std::chrono::microseconds(
        static_cast<int64_t>(period.count() * jitter(rng)));
    }
    auto now = Clock::now();
    if (next < now) {
      next = now;  // Do not burst if we fell behind.
    }
    std::this_thread::sleep_until(next);
  }
}

// ── Config loading ────────────────────────────────────────────────────────

BenchmarkConfig load_config_or_exit(int argc, char ** argv)
{
  BenchmarkConfig cfg;
  try {
    if (!parse_args(argc, argv, cfg)) {
      exit(0);
    }
  } catch (const std::exception & e) {
    std::cerr << "Error: " << e.what() << "\n";
    print_usage(argv[0]);
    exit(1);
  }
  return cfg;
}

}  // namespace constrained_pubsub_benchmark

#endif  // CONSTRAINED_PUBSUB_BENCHMARK__BENCHMARK_COMMON_HPP_
