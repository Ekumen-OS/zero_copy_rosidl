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

#ifndef CONSTRAINED_PUBSUB_BENCHMARK__CONFIG_HPP_
#define CONSTRAINED_PUBSUB_BENCHMARK__CONFIG_HPP_

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace constrained_pubsub_benchmark
{

/// Benchmark case: three orthogonal axes select the full run.
///   Message (--message std|exp): the ROS message type on the topic.
///   Config (--config copy|constrained_pub_only|constrained_pub_sub): the
///     data path (plain copy vs constrained loaned publisher, optionally
///     with a constrained subscription).  Constrained configs require the
///     experimental message (constraints are defined for it only).
///   Backend (--backend fastcdr|xcdr): the rmw_fastrtps serialization
///     backend.  FastCDR is only valid with the copy path.
enum class MessageType
{
  kStandard,      /// sensor_msgs::msg::Image.
  kExperimental,  /// sensor_msgs::msg::experimental::Image.
};

/// Convert message type to a string key ("std", "exp").
const char * message_type_key(MessageType m);

/// Parse a string into MessageType.  Throws std::invalid_argument on unknown.
MessageType message_type_from_string(const std::string & s);

/// Data path: plain copy vs constrained (loaned) publisher/subscriber.
enum class Config
{
  kCopy,              /// Plain copy publish/subscribe.
  kConstrainedPubOnly,  /// Constrained loaned publisher, plain subscriber.
  kConstrainedPubSub,   /// Constrained loaned publisher and subscriber.
};

/// Convert config to its string id ("copy", "constrained_pub_only",
/// "constrained_pub_sub").
const char * config_key(Config c);

/// Parse a string into Config.  Throws std::invalid_argument on unknown.
Config config_from_string(const std::string & s);

/// Benchmark direction: which halves form the run step.
enum class Direction
{
  kCppToCpp,
  kCppToPy,
  kPyToCpp,
  kPyToPy,
  kIntraCpp,
  kIntraPy,
};

/// Convert direction to its label (e.g. "cpp_to_cpp").
const char * direction_key(Direction d);

/// Parse a string into Direction.  Throws std::invalid_argument on unknown.
Direction direction_from_string(const std::string & s);

/// Serialization backend selection.
enum class Backend
{
  kAuto,     /// Default: FastCDR for a standard-message copy, XCDR elsewhere.
  kFastCdr,  /// Force the stock FastCDR backend.
  kXcdr,     /// Force the XCDR backend.
};

/// Convert backend to a string key ("auto", "fastcdr", "xcdr").
const char * backend_key(Backend b);

/// Parse a string into Backend.  Throws std::invalid_argument on unknown.
Backend backend_from_string(const std::string & s);

/// Resolve the effective backend: explicit wins, otherwise the default
/// (FastCDR for a standard-message copy, XCDR everywhere else).
/// Throws std::invalid_argument when FastCDR is requested with a
/// constrained config.
Backend resolve_backend(MessageType m, Config c, Backend b);

/// DDS transport selection.  Applied via Fast DDS XML profiles; kAuto leaves
/// the environment untouched (system default, diagnostics only).
enum class Transport
{
  kAuto,     /// No profile override.
  kUdp,      /// UDPv4 only, data sharing off.
  kShmem,    /// Shared memory only, data sharing off.
  kShmemDs,  /// Shared memory with data sharing on (fail if unavailable).
};

/// Convert transport to a string key ("auto", "udp", "shmem", "shmem_ds").
const char * transport_key(Transport t);

/// Parse a string into Transport.  Throws std::invalid_argument on unknown.
Transport transport_from_string(const std::string & s);

/// Parse a comma-separated payload grid (each item accepts K/KB/M/MB).
/// Throws std::invalid_argument on empty or malformed input.
std::vector<size_t> parse_payload_grid(const std::string & s);

/// Default stepped frequency grid: 10^(k/3) Hz for k = 0..9
/// (1, 2.15, 4.64, 10, 21.5, 46.4, 100, 215, 464, 1000 Hz).
std::vector<double> default_frequency_grid();

/// Parse a comma-separated frequency grid in Hz.  All values must be > 0.
/// Throws std::invalid_argument on empty or malformed input.
std::vector<double> parse_frequency_grid(const std::string & s);

/// Build a run id from its coordinates, e.g.
/// "exp_constrained_pub_sub_xcdr__cpp_to_cpp__400000B__10Hz__shmem".
/// Callers pass the resolved backend (see resolve_backend).
std::string make_run_id(
  MessageType message, Config config, Backend backend,
  const std::string & direction, size_t payload_bytes, double frequency_hz,
  Transport transport);

/// Build a run id with the process id appended (manual runs only; the
/// orchestrator always passes an explicit id so pub/sub rows join).
std::string auto_run_id(
  MessageType message, Config config, Backend backend,
  const std::string & direction, size_t payload_bytes, double frequency_hz,
  Transport transport);

/// QoS reliability.
enum class Reliability
{
  kReliable,
  kBestEffort,
};

const char * reliability_key(Reliability r);

Reliability reliability_from_string(const std::string & s);

/// Complete runtime configuration for a single benchmark run.
struct BenchmarkConfig
{
  // ── Case ────────────────────────────────────────────────────────────────
  /// ROS message type on the topic.
  MessageType message{MessageType::kStandard};
  /// Data path: copy vs constrained loaned pub/sub.
  Config config{Config::kCopy};
  bool strict{false};  ///< Only meaningful for constrained configs.
  /// Explicit backend override; kAuto resolves from the config.
  Backend backend{Backend::kAuto};
  /// DDS transport profile; kAuto leaves the environment untouched.
  Transport transport{Transport::kAuto};
  /// Run identifier shared by all processes of one run step.  Empty means
  /// the process auto-generates one (manual runs only; the orchestrator
  /// always passes an explicit id so pub/sub rows join).
  std::string run_id;
  /// Direction label (e.g. "cpp_to_cpp", "intra_py").  Set by the
  /// orchestrator; empty means unset.
  std::string direction;
  /// Sweep step index; -1 means a standalone (non-sweep) run.
  int step_index{-1};

  // ── Message ─────────────────────────────────────────────────────────────
  /// Size in bytes for the variable-length payload (Image.data).
  size_t payload_bytes{1000};

  // ── Shape controls (drive XCDR compaction) ──────────────────────────────
  // For constrained (loaned) publishers, compaction fires when the actual
  // message is smaller than the constraint upper bounds, and is a no-op when
  // the message fills all bounds.  These knobs shape the message under a
  // FIXED constraint set (encoding bound 64, data bound = payload_bytes):
  /// Exact length of the Image.encoding string; 0 = "rgb8" (4 chars).
  /// Encoding at its bound (64) removes the encoding-driven compaction trigger.
  size_t fill_encoding{0};
  /// Exact length of the header.frame_id string; 0 = "benchmark" (9 chars).
  /// Frame_id at its bound (16) removes the nested-header compaction trigger.
  size_t fill_frame_id{0};
  /// Fraction of payload_bytes to fill Image.data (0.0 < ratio <= 1.0).
  /// 1.0 = data at its bound (no data-driven compaction); 0.0 = legacy
  /// square-image logic (data = dim*dim*3 <= payload_bytes).
  double fill_data_ratio{0.0};

  // ── QoS ─────────────────────────────────────────────────────────────────
  int qos_depth{10};
  Reliability reliability{Reliability::kReliable};

  // ── Run control ─────────────────────────────────────────────────────────
  /// Fixed publish rate in Hz for the run step.
  double publish_rate_hz{100.0};
  /// Fixed length of the run step in seconds.
  double duration_sec{30.0};
  /// Topic name (expanded/remapped by rcl).  Use an absolute name for
  /// cross-process benchmarks so both processes resolve the same topic.
  std::string topic{"~/benchmark"};
  /// Extra time (seconds) a subscriber keeps listening after the expected
  /// run window to catch in-flight messages (two-process benchmarks).
  double grace_sec{2.0};
};

/// Validate a parsed case: constrained configs require the experimental
/// message, and FastCDR is only valid with the copy path.
/// Throws std::invalid_argument on violation.
void validate_case(const BenchmarkConfig & cfg);

/// Parse command-line arguments into a BenchmarkConfig.
/// Throws std::invalid_argument on invalid or missing required args.
/// Prints usage to stderr and returns false for --help.
bool parse_args(int argc, char ** argv, BenchmarkConfig & cfg);

/// Print usage to stderr.
void print_usage(const char * program);

}  // namespace constrained_pubsub_benchmark

#endif  // CONSTRAINED_PUBSUB_BENCHMARK__CONFIG_HPP_
