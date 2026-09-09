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

#include <unistd.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "constrained_pubsub_benchmark/config.hpp"

namespace constrained_pubsub_benchmark
{

const char * message_type_key(MessageType m)
{
  switch (m) {
    case MessageType::kStandard:
      return "std";
    case MessageType::kExperimental:
      return "exp";
    default:
      return "unknown";
  }
}

MessageType message_type_from_string(const std::string & s)
{
  if (s == "std") {return MessageType::kStandard;}
  if (s == "exp") {return MessageType::kExperimental;}
  throw std::invalid_argument("unknown message: " + s);
}

const char * config_key(Config c)
{
  switch (c) {
    case Config::kCopy:
      return "copy";
    case Config::kConstrainedPubOnly:
      return "constrained_pub_only";
    case Config::kConstrainedPubSub:
      return "constrained_pub_sub";
    default:
      return "unknown";
  }
}

Config config_from_string(const std::string & s)
{
  if (s == "copy") {return Config::kCopy;}
  if (s == "constrained_pub_only") {return Config::kConstrainedPubOnly;}
  if (s == "constrained_pub_sub") {return Config::kConstrainedPubSub;}
  throw std::invalid_argument("unknown config: " + s);
}

const char * backend_key(Backend b)
{
  switch (b) {
    case Backend::kAuto:
      return "auto";
    case Backend::kFastCdr:
      return "fastcdr";
    case Backend::kXcdr:
      return "xcdr";
    default:
      return "unknown";
  }
}

Backend backend_from_string(const std::string & s)
{
  if (s == "auto") {return Backend::kAuto;}
  if (s == "fastcdr") {return Backend::kFastCdr;}
  if (s == "xcdr") {return Backend::kXcdr;}
  throw std::invalid_argument("unknown backend: " + s);
}

Backend resolve_backend(MessageType m, Config c, Backend b)
{
  if (b != Backend::kAuto) {
    if (b == Backend::kFastCdr && c != Config::kCopy) {
      throw std::invalid_argument("fastcdr backend requires the copy config");
    }
    return b;
  }
  if (c == Config::kCopy && m == MessageType::kStandard) {
    return Backend::kFastCdr;
  }
  return Backend::kXcdr;
}

void validate_case(const BenchmarkConfig & cfg)
{
  if (cfg.config != Config::kCopy &&
    cfg.message != MessageType::kExperimental)
  {
    throw std::invalid_argument(
      "constrained configs require --message exp");
  }
  // Backend/mode compatibility goes through the same gate as resolution.
  resolve_backend(cfg.message, cfg.config, cfg.backend);
}

const char * transport_key(Transport t)
{
  switch (t) {
    case Transport::kAuto:
      return "auto";
    case Transport::kUdp:
      return "udp";
    case Transport::kShmem:
      return "shmem";
    case Transport::kShmemDs:
      return "shmem_ds";
    default:
      return "unknown";
  }
}

Transport transport_from_string(const std::string & s)
{
  if (s == "auto") {return Transport::kAuto;}
  if (s == "udp") {return Transport::kUdp;}
  if (s == "shmem") {return Transport::kShmem;}
  if (s == "shmem_ds") {return Transport::kShmemDs;}
  throw std::invalid_argument("unknown transport: " + s);
}

const char * reliability_key(Reliability r)
{
  switch (r) {
    case Reliability::kReliable:
      return "reliable";
    case Reliability::kBestEffort:
      return "best_effort";
    default:
      return "unknown";
  }
}

Reliability reliability_from_string(const std::string & s)
{
  if (s == "reliable") {return Reliability::kReliable;}
  if (s == "best_effort") {return Reliability::kBestEffort;}
  throw std::invalid_argument("unknown reliability: " + s);
}

static bool suffix_ends_with(const std::string & str, const std::string & suffix)
{
  if (str.size() < suffix.size()) {return false;}
  return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static size_t parse_payload(const std::string & s)
{
  // Accept suffixes: K, KB, M, MB
  std::string upper;
  for (char c : s) {
    upper.push_back(static_cast<char>(toupper(c)));
  }

  size_t multiplier = 1;
  size_t num_end = upper.size();

  if (suffix_ends_with(upper, "MB")) {
    multiplier = 1000000;
    num_end = upper.size() - 2;
  } else if (suffix_ends_with(upper, "M")) {
    multiplier = 1000000;
    num_end = upper.size() - 1;
  } else if (suffix_ends_with(upper, "KB")) {
    multiplier = 1000;
    num_end = upper.size() - 2;
  } else if (suffix_ends_with(upper, "K")) {
    multiplier = 1000;
    num_end = upper.size() - 1;
  }

  std::string num_part = s.substr(0, num_end);
  // Trim trailing whitespace
  while (!num_part.empty() && num_part.back() == ' ') {
    num_part.pop_back();
  }

  size_t value = std::stoul(num_part);
  return value * multiplier;
}

static std::string trim_copy(const std::string & s)
{
  size_t begin = s.find_first_not_of(" \t");
  if (begin == std::string::npos) {return "";}
  size_t end = s.find_last_not_of(" \t");
  return s.substr(begin, end - begin + 1);
}

std::vector<size_t> parse_payload_grid(const std::string & s)
{
  std::vector<size_t> out;
  std::istringstream items(s);
  std::string item;
  while (std::getline(items, item, ',')) {
    std::string trimmed = trim_copy(item);
    if (trimmed.empty()) {
      throw std::invalid_argument("empty payload grid entry");
    }
    out.push_back(parse_payload(trimmed));
  }
  if (out.empty()) {
    throw std::invalid_argument("empty payload grid");
  }
  return out;
}

std::vector<double> default_frequency_grid()
{
  std::vector<double> out;
  for (int k = 0; k <= 9; ++k) {
    out.push_back(std::pow(10.0, static_cast<double>(k) / 3.0));
  }
  return out;
}

std::vector<double> parse_frequency_grid(const std::string & s)
{
  std::vector<double> out;
  std::istringstream items(s);
  std::string item;
  while (std::getline(items, item, ',')) {
    std::string trimmed = trim_copy(item);
    if (trimmed.empty()) {
      throw std::invalid_argument("empty frequency grid entry");
    }
    double hz = std::stod(trimmed);
    if (!(hz > 0.0)) {
      throw std::invalid_argument("frequency must be positive: " + item);
    }
    out.push_back(hz);
  }
  if (out.empty()) {
    throw std::invalid_argument("empty frequency grid");
  }
  return out;
}

// Internal: formats a frequency for run ids ("10Hz", "2p15443Hz").
static std::string format_frequency_hz(double hz)
{
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.6g", hz);
  std::string s(buf);
  for (char & c : s) {
    if (c == '.') {c = 'p';}
  }
  return s + "Hz";
}

std::string make_run_id(
  MessageType message, Config config, Backend backend,
  const std::string & direction, size_t payload_bytes, double frequency_hz,
  Transport transport, Reliability reliability)
{
  std::string dir = direction.empty() ? "manual" : direction;
  std::ostringstream os;
  // The case segment carries all three axes ("std_copy_fastcdr"): the old
  // compound ids encoded the backend implicitly, and merged-CSV provenance
  // needs it explicit.  Callers pass the resolved backend.
  os << message_type_key(message) << "_" << config_key(config) << "_"
     << backend_key(backend) << "__"
     << dir << "__" << payload_bytes << "B__"
     << format_frequency_hz(frequency_hz) << "__" << transport_key(transport)
     << "__" << reliability_key(reliability);
  return os.str();
}

std::string auto_run_id(
  MessageType message, Config config, Backend backend,
  const std::string & direction, size_t payload_bytes, double frequency_hz,
  Transport transport, Reliability reliability)
{
  std::ostringstream os;
  os << make_run_id(
    message, config, backend, direction, payload_bytes, frequency_hz,
    transport, reliability)
     << "__pid" << getpid();
  return os.str();
}

const char * direction_key(Direction d)
{
  switch (d) {
    case Direction::kCppToCpp:
      return "cpp_to_cpp";
    case Direction::kCppToPy:
      return "cpp_to_py";
    case Direction::kPyToCpp:
      return "py_to_cpp";
    case Direction::kPyToPy:
      return "py_to_py";
    case Direction::kIntraCpp:
      return "intra_cpp";
    case Direction::kIntraPy:
      return "intra_py";
    default:
      return "unknown";
  }
}

Direction direction_from_string(const std::string & s)
{
  if (s == "cpp_to_cpp") {return Direction::kCppToCpp;}
  if (s == "cpp_to_py") {return Direction::kCppToPy;}
  if (s == "py_to_cpp") {return Direction::kPyToCpp;}
  if (s == "py_to_py") {return Direction::kPyToPy;}
  if (s == "intra_cpp") {return Direction::kIntraCpp;}
  if (s == "intra_py") {return Direction::kIntraPy;}
  throw std::invalid_argument("unknown direction: " + s);
}

bool parse_args(int argc, char ** argv, BenchmarkConfig & cfg)
{
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);

    if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return false;
    }

    auto next = [&]() -> std::string {
        if (i + 1 >= argc) {
          throw std::invalid_argument(arg + " requires a value");
        }
        return std::string(argv[++i]);
      };

    if (arg == "--message") {
      cfg.message = message_type_from_string(next());
    } else if (arg == "--config") {
      cfg.config = config_from_string(next());
    } else if (arg == "--backend") {
      cfg.backend = backend_from_string(next());
    } else if (arg == "--transport") {
      cfg.transport = transport_from_string(next());
    } else if (arg == "--run-id") {
      cfg.run_id = next();
    } else if (arg == "--direction") {
      cfg.direction = next();
    } else if (arg == "--step-index") {
      cfg.step_index = std::stoi(next());
      if (cfg.step_index < -1) {
        throw std::invalid_argument("step index must be >= -1");
      }
    } else if (arg == "--strict") {
      auto val = next();
      if (val == "true" || val == "1") {cfg.strict = true;} else if (val == "false" || val == "0") {
        cfg.strict = false;
      } else {throw std::invalid_argument("strict must be true/false: " + val);}
    } else if (arg == "--payload-bytes") {
      cfg.payload_bytes = parse_payload(next());
    } else if (arg == "--fill-encoding") {
      cfg.fill_encoding = std::stoul(next());
    } else if (arg == "--fill-frame-id") {
      cfg.fill_frame_id = std::stoul(next());
    } else if (arg == "--fill-data-ratio") {
      cfg.fill_data_ratio = std::stod(next());
      if (cfg.fill_data_ratio < 0.0 || cfg.fill_data_ratio > 1.0) {
        throw std::invalid_argument("--fill-data-ratio must be in [0.0, 1.0]");
      }
    } else if (arg == "--qos-depth") {
      cfg.qos_depth = std::stoi(next());
      if (cfg.qos_depth <= 0) {
        throw std::invalid_argument("qos depth must be positive");
      }
    } else if (arg == "--reliability") {
      cfg.reliability = reliability_from_string(next());
    } else if (arg == "--publish-rate-hz") {
      cfg.publish_rate_hz = std::stod(next());
    } else if (arg == "--publish-jitter") {
      cfg.publish_jitter = std::stod(next());
      if (cfg.publish_jitter < 0.0 || cfg.publish_jitter >= 1.0) {
        throw std::invalid_argument("--publish-jitter must be in [0.0, 1.0)");
      }
    } else if (arg == "--publish-jitter-seed") {
      cfg.publish_jitter_seed =
        static_cast<uint64_t>(std::stoull(next()));
    } else if (arg == "--duration-sec") {
      cfg.duration_sec = std::stod(next());
    } else if (arg == "--topic") {
      cfg.topic = next();
    } else if (arg == "--grace-sec") {
      cfg.grace_sec = std::stod(next());
      if (cfg.grace_sec < 0.0) {
        throw std::invalid_argument("grace sec must be non-negative");
      }
    } else {
      throw std::invalid_argument("unknown option: " + arg);
    }
  }
  validate_case(cfg);
  return true;
}

void print_usage(const char * program)
{
  std::cerr  << "Usage: " << program << " [options]\n\n"
             << "Options:\n"
             << "  --message std|exp                  ROS message type "
             << "(default: std)\n"
             << "  --config copy|constrained_pub_only|constrained_pub_sub\n"
             << "                                     Data path (default: copy)\n"
             << "  --backend auto|fastcdr|xcdr        Serialization backend "
             << "(default: auto; fastcdr needs copy)\n"
             << "  --transport auto|udp|shmem|shmem_ds  DDS transport profile "
             << "(default: auto)\n"
             << "  --run-id <id>                     Run identifier (default: auto)\n"
             << "  --direction <name>                 Direction label (default: unset)\n"
             << "  --step-index <N>                   Sweep step index, -1 = single "
             << "(default: -1)\n"
             << "  --strict true|false               Enable strict validation "
             << "(constrained configs only)\n"
             << "  --payload-bytes <N>[K|KB|M|MB]    Payload size (default: 1000)\n"
             << "  --fill-encoding <N>               Exact encoding length (0 = \"rgb8\");\n"
             << "                                     64 = at bound (no-op compaction)\n"
             << "  --fill-frame-id <N>               Exact frame_id length (0 = \"benchmark\");\n"
             << "                                     16 = at bound (no-op compaction)\n"
             << "  --fill-data-ratio <F>             Data fill fraction of payload_bytes\n"
             << "                                     [0.0,1.0]; 1.0 = at bound (no-op\n"
             << "                                     compaction), 0.0 = square-image logic\n"
             << "  --qos-depth <N>                   QoS history depth (default: 10)\n"
             << "  --reliability reliable|best_effort (default: reliable)\n"
             << "  --publish-rate-hz <F>             Fixed publish rate (default: 100)\n"
             << "  --publish-jitter <F>            Deadline dither fraction [0.0,1.0)\n"
             << "                                     (default: 0 = exact metronome)\n"
             << "  --publish-jitter-seed <N>       PRNG seed for dither (default: 42)\n"
             << "  --duration-sec <F>                Run duration in seconds (default: 30)\n"
             << "  --topic <name>                    Topic name (default: ~/benchmark)\n"
             << "  --grace-sec <F>                   Extra listen time after the run window\n"
             << "                                     (subscriber, default: 2)\n"
             << "  --help, -h                        This help\n";
}

}  // namespace constrained_pubsub_benchmark
