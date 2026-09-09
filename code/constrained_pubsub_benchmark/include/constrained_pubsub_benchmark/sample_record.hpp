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

#ifndef CONSTRAINED_PUBSUB_BENCHMARK__SAMPLE_RECORD_HPP_
#define CONSTRAINED_PUBSUB_BENCHMARK__SAMPLE_RECORD_HPP_

// Per-sample benchmark records.
//
// Every published or received message produces one CSV row; no aggregates,
// no JSON.  Post-processing joins publish/receive rows on (run_id,
// sequence) and derives latency, throughput, drops, and plots.  Rows are
// buffered in memory during the run and written to stdout once at the end:
// per-sample stdout writes (and flushes) inside the publish loop or the
// subscription callback would inject I/O latency into the measurement.
// Diagnostics and lifecycle markers go to stderr, never to stdout.
//
// All fixed-vocabulary columns (event, process, direction, config, message,
// backend, transport) are stored as enums and translated to strings only
// during CSV serialization; only run_id, observed_utc, and error are free
// strings.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "constrained_pubsub_benchmark/config.hpp"

namespace constrained_pubsub_benchmark
{

/// Sample event type.
enum class Event
{
  kPublish,  /// One row per attempted publication.
  kReceive,  /// One row per received message (always decodable).
  kError,    /// Explicit failure or undecodable sample (never seq-joined).
};

inline const char * event_key(Event e)
{
  switch (e) {
    case Event::kPublish:
      return "publish";
    case Event::kReceive:
      return "receive";
    case Event::kError:
      return "error";
    default:
      return "unknown";
  }
}

/// Emitting side of a run step.
enum class Process
{
  kPub,    /// Cross-process publisher.
  kSub,    /// Cross-process subscriber.
  kIntra,  /// Single-process runner (pub + sub on one node).
};

inline const char * process_key(Process p)
{
  switch (p) {
    case Process::kPub:
      return "pub";
    case Process::kSub:
      return "sub";
    case Process::kIntra:
      return "intra";
    default:
      return "unknown";
  }
}

/// One benchmark sample.  Unknown timestamps use -1; the error text is
/// empty unless the event is kError.  Phase is always "measured" and
/// status derives from the event, so neither is stored.
struct Sample
{
  std::string run_id;
  Event event{Event::kPublish};
  Process process{Process::kPub};
  Direction direction{Direction::kCppToCpp};
  Config config{Config::kCopy};
  MessageType message{MessageType::kStandard};
  Backend backend{Backend::kFastCdr};
  Transport transport{Transport::kAuto};
  size_t payload_bytes{0};
  double target_frequency_hz{0.0};
  int step_index{-1};
  uint64_t sequence{0};
  std::string observed_utc;  ///< ISO-8601 UTC of observation.
  int64_t send_mono_ns{-1};
  int64_t receive_mono_ns{-1};
  int64_t middleware_source_ns{-1};
  int64_t middleware_received_ns{-1};
  std::string error;
};

/// Canonical CSV column order.  Stable: post-processing parses by position.
inline std::string csv_header()
{
  return "run_id,event,process,direction,config,message,backend,transport,"
         "payload_bytes,target_frequency_hz,step_index,phase,sequence,"
         "observed_utc,observed_mono_ns,send_mono_ns,receive_mono_ns,"
         "middleware_source_ns,middleware_received_ns,status,error";
}

/// Quote a CSV field when it contains a comma, quote, or newline.
inline std::string escape_csv_field(const std::string & field)
{
  if (field.find_first_of(",\"\n") == std::string::npos) {
    return field;
  }
  std::string out("\"");
  for (char c : field) {
    if (c == '"') {out.push_back('"');}
    out.push_back(c);
  }
  out.push_back('"');
  return out;
}

inline std::string format_hz(double hz)
{
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.6g", hz);
  return std::string(buf);
}

/// Serialize one sample.  Field order matches csv_header().
inline std::string sample_to_csv(const Sample & s)
{
  // Derived columns: phase is always "measured"; status follows the event;
  // observed_mono_ns is the event's own timestamp.
  const char * status = (s.event == Event::kError) ? "error" : "ok";
  const int64_t observed_mono_ns =
    (s.event == Event::kPublish) ? s.send_mono_ns : s.receive_mono_ns;
  std::ostringstream os;
  os << escape_csv_field(s.run_id) << ","
     << event_key(s.event) << ","
     << process_key(s.process) << ","
     << direction_key(s.direction) << ","
     << config_key(s.config) << ","
     << message_type_key(s.message) << ","
     << backend_key(s.backend) << ","
     << transport_key(s.transport) << ","
     << s.payload_bytes << ","
     << format_hz(s.target_frequency_hz) << ","
     << s.step_index << ","
     << "measured" << ","
     << s.sequence << ","
     << escape_csv_field(s.observed_utc) << ","
     << observed_mono_ns << ","
     << s.send_mono_ns << ","
     << s.receive_mono_ns << ","
     << s.middleware_source_ns << ","
     << s.middleware_received_ns << ","
     << status << ","
     << escape_csv_field(s.error);
  return os.str();
}

/// Monotonic clock in nanoseconds (steady_clock epoch).
inline int64_t mono_now_ns()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
}

/// Wall clock as ISO-8601 UTC with microsecond precision.
inline std::string utc_now_iso8601()
{
  auto now = std::chrono::system_clock::now();
  auto secs = std::chrono::duration_cast<std::chrono::seconds>(
    now.time_since_epoch());
  auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
    now.time_since_epoch() - secs);
  std::time_t t = static_cast<std::time_t>(secs.count());
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &t);
#else
  gmtime_r(&t, &tm);
#endif
  char buf[48];
  std::snprintf(
    buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%.6dZ",
    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
    tm.tm_hour, tm.tm_min, tm.tm_sec,
    static_cast<int>(micros.count()));
  return std::string(buf);
}

/// Parse a direction label, defaulting to `fallback` when empty (manual
/// runs omit --direction).  Throws std::invalid_argument on unknown labels.
inline Direction parse_direction_or(
  const std::string & label, Direction fallback)
{
  if (label.empty()) {
    return fallback;
  }
  return direction_from_string(label);
}

/// Shared run coordinates for one emitter (publisher, subscriber, or
/// intra-process runner).  Fill once, reserve() upfront, then emit() per
/// sample; emit() only buffers (timestamps are still captured at event
/// time).  Call flush_rows() once after the run to write every buffered
/// row to stdout.  Never write inside the hot path.
struct RunContext
{
  std::string run_id;
  Process process{Process::kPub};
  Direction direction{Direction::kCppToCpp};
  Config config{Config::kCopy};
  MessageType message{MessageType::kStandard};
  Backend backend{Backend::kFastCdr};
  Transport transport{Transport::kAuto};
  size_t payload_bytes{0};
  double target_frequency_hz{0.0};
  int step_index{-1};
  std::vector<Sample> rows;  ///< Buffered samples, flushed once at end.

  /// Build a context from the parsed run configuration: resolves the
  /// run id (explicit wins, otherwise minted from the resolved backend),
  /// the direction label (explicit wins, otherwise the fallback), and
  /// pre-sizes the row buffer.  Callers pass expected_samples * 1.2
  /// margin inside; the vector still grows if exceeded, so data is
  /// never lost.
  static RunContext from_config(
    const BenchmarkConfig & cfg, Process process, Direction direction_fallback,
    Backend backend, size_t expected_samples)
  {
    RunContext ctx;
    // Manual runs omit --direction; the orchestrator always sets it.
    // The resolved direction feeds the minted run id too.
    ctx.direction = parse_direction_or(cfg.direction, direction_fallback);
    ctx.run_id = cfg.run_id.empty() ?
      auto_run_id(
      cfg.message, cfg.config, backend, direction_key(ctx.direction),
      cfg.payload_bytes, cfg.publish_rate_hz, cfg.transport,
      cfg.reliability) :
      cfg.run_id;
    ctx.process = process;
    ctx.message = cfg.message;
    ctx.config = cfg.config;
    ctx.backend = backend;
    ctx.transport = cfg.transport;
    ctx.payload_bytes = cfg.payload_bytes;
    ctx.target_frequency_hz = cfg.publish_rate_hz;
    ctx.step_index = cfg.step_index;
    ctx.rows.reserve(static_cast<size_t>(expected_samples * 1.2) + 8);
    return ctx;
  }

  void emit(
    Event event, uint64_t sequence,
    int64_t send_mono_ns, int64_t receive_mono_ns,
    int64_t middleware_source_ns, int64_t middleware_received_ns,
    const std::string & error = "")
  {
    Sample s;
    s.run_id = run_id;
    s.event = event;
    s.process = process;
    s.direction = direction;
    s.config = config;
    s.message = message;
    s.backend = backend;
    s.transport = transport;
    s.payload_bytes = payload_bytes;
    s.target_frequency_hz = target_frequency_hz;
    s.step_index = step_index;
    s.sequence = sequence;
    s.observed_utc = utc_now_iso8601();
    s.send_mono_ns = send_mono_ns;
    s.receive_mono_ns = receive_mono_ns;
    s.middleware_source_ns = middleware_source_ns;
    s.middleware_received_ns = middleware_received_ns;
    s.error = error;
    rows.push_back(std::move(s));
  }

  /// Write every buffered row to stdout, in emission order.  Call once at
  /// the end of the run (and on the error path, after the error row).
  void flush_rows()
  {
    for (const auto & s : rows) {
      printf("%s\n", sample_to_csv(s).c_str());
    }
    fflush(stdout);
    rows.clear();
  }
};

// ── Message metadata ─────────────────────────────────────────────────────
// The sample sequence travels in header.frame_id as a fixed-width 16-char
// string ('s' + 15 zero-padded digits), which satisfies the c1/c2
// frame_id bound of 16 while staying a valid plain string for
// unconstrained runs.  The monotonic send timestamp travels in
// header.stamp.  Both work on standard and experimental Image messages.

// Maximum sequence representable in the 15-digit frame_id suffix.
constexpr uint64_t kMaxMetadataSequence = 999999999999999ULL;

// Nanoseconds per second for stamp encode/decode (fits int32 seconds).
constexpr int64_t kNanosPerSecond = 1000000000;

/// Stamp a message with its sequence and monotonic send time.
/// Call after fill_image (overwrites its stamp/frame_id placeholders).
template<typename ImageT>
void encode_sample_metadata(ImageT & msg, uint64_t seq, int64_t send_mono_ns)
{
  std::ostringstream frame_id;
  frame_id << 's' << std::setw(15) << std::setfill('0') << seq;
  msg.header.frame_id.assign(frame_id.str());
  msg.header.stamp.sec =
    static_cast<int32_t>(send_mono_ns / kNanosPerSecond);
  msg.header.stamp.nanosec =
    static_cast<uint32_t>(send_mono_ns % kNanosPerSecond);
}

/// Decode sequence and send time.  Returns false when the frame_id does
/// not carry benchmark metadata (foreign traffic), leaving outputs zero.
template<typename ImageT>
bool decode_sample_metadata(
  const ImageT & msg, uint64_t * seq, int64_t * send_mono_ns)
{
  *seq = 0;
  *send_mono_ns = 0;
  std::string frame_id(msg.header.frame_id.data(), msg.header.frame_id.size());
  if (frame_id.size() != 16 || frame_id[0] != 's') {
    return false;
  }
  uint64_t value = 0;
  for (size_t i = 1; i < 16; ++i) {
    if (frame_id[i] < '0' || frame_id[i] > '9') {
      *seq = 0;
      return false;
    }
    value = value * 10 + static_cast<uint64_t>(frame_id[i] - '0');
  }
  *seq = value;
  *send_mono_ns = static_cast<int64_t>(msg.header.stamp.sec) * kNanosPerSecond +
    static_cast<int64_t>(msg.header.stamp.nanosec);
  return true;
}

}  // namespace constrained_pubsub_benchmark

#endif  // CONSTRAINED_PUBSUB_BENCHMARK__SAMPLE_RECORD_HPP_
