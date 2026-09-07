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
#include "constrained_pubsub_benchmark/config.hpp"

TEST(ConfigTest, MessageKeys) {
  using constrained_pubsub_benchmark::MessageType;
  using constrained_pubsub_benchmark::message_type_from_string;
  using constrained_pubsub_benchmark::message_type_key;
  EXPECT_STREQ(message_type_key(MessageType::kStandard), "std");
  EXPECT_STREQ(message_type_key(MessageType::kExperimental), "exp");
  EXPECT_EQ(message_type_from_string("std"), MessageType::kStandard);
  EXPECT_EQ(message_type_from_string("exp"), MessageType::kExperimental);
  EXPECT_THROW(message_type_from_string("bogus"), std::invalid_argument);
}

TEST(ConfigTest, ConfigKeys) {
  using constrained_pubsub_benchmark::Config;
  using constrained_pubsub_benchmark::config_key;
  EXPECT_STREQ(config_key(Config::kCopy), "copy");
  EXPECT_STREQ(
    config_key(Config::kConstrainedPubOnly), "constrained_pub_only");
  EXPECT_STREQ(
    config_key(Config::kConstrainedPubSub), "constrained_pub_sub");
}

TEST(ConfigTest, ConfigFromString) {
  using constrained_pubsub_benchmark::Config;
  using constrained_pubsub_benchmark::config_from_string;
  EXPECT_EQ(config_from_string("copy"), Config::kCopy);
  EXPECT_EQ(
    config_from_string("constrained_pub_only"),
    Config::kConstrainedPubOnly);
  EXPECT_EQ(
    config_from_string("constrained_pub_sub"),
    Config::kConstrainedPubSub);
  EXPECT_THROW(config_from_string("bogus"), std::invalid_argument);
}

TEST(ConfigTest, DirectionKeys) {
  using constrained_pubsub_benchmark::Direction;
  using constrained_pubsub_benchmark::direction_from_string;
  using constrained_pubsub_benchmark::direction_key;
  EXPECT_STREQ(direction_key(Direction::kCppToCpp), "cpp_to_cpp");
  EXPECT_STREQ(direction_key(Direction::kCppToPy), "cpp_to_py");
  EXPECT_STREQ(direction_key(Direction::kPyToCpp), "py_to_cpp");
  EXPECT_STREQ(direction_key(Direction::kPyToPy), "py_to_py");
  EXPECT_STREQ(direction_key(Direction::kIntraCpp), "intra_cpp");
  EXPECT_STREQ(direction_key(Direction::kIntraPy), "intra_py");
  EXPECT_EQ(direction_from_string("cpp_to_cpp"), Direction::kCppToCpp);
  EXPECT_EQ(direction_from_string("py_to_py"), Direction::kPyToPy);
  EXPECT_EQ(direction_from_string("intra_py"), Direction::kIntraPy);
  EXPECT_THROW(direction_from_string("bogus"), std::invalid_argument);
  EXPECT_THROW(direction_from_string(""), std::invalid_argument);
}

TEST(ConfigTest, ResolveBackend) {
  using constrained_pubsub_benchmark::Backend;
  using constrained_pubsub_benchmark::Config;
  using constrained_pubsub_benchmark::MessageType;
  using constrained_pubsub_benchmark::backend_from_string;
  using constrained_pubsub_benchmark::resolve_backend;
  EXPECT_EQ(backend_from_string("auto"), Backend::kAuto);
  EXPECT_EQ(backend_from_string("fastcdr"), Backend::kFastCdr);
  EXPECT_EQ(backend_from_string("xcdr"), Backend::kXcdr);
  EXPECT_THROW(backend_from_string("bogus"), std::invalid_argument);
  // Auto default: FastCDR for a standard-message copy, XCDR elsewhere.
  EXPECT_EQ(
    resolve_backend(
      MessageType::kStandard, Config::kCopy, Backend::kAuto),
    Backend::kFastCdr);
  EXPECT_EQ(
    resolve_backend(
      MessageType::kExperimental, Config::kCopy, Backend::kAuto),
    Backend::kXcdr);
  EXPECT_EQ(
    resolve_backend(
      MessageType::kExperimental, Config::kConstrainedPubSub,
      Backend::kAuto),
    Backend::kXcdr);
  // Explicit wins, except FastCDR with a constrained config.
  EXPECT_EQ(
    resolve_backend(
      MessageType::kStandard, Config::kCopy, Backend::kXcdr),
    Backend::kXcdr);
  EXPECT_EQ(
    resolve_backend(
      MessageType::kExperimental, Config::kCopy, Backend::kFastCdr),
    Backend::kFastCdr);
  EXPECT_THROW(
    resolve_backend(
      MessageType::kExperimental, Config::kConstrainedPubOnly,
      Backend::kFastCdr),
    std::invalid_argument);
  EXPECT_THROW(
    resolve_backend(
      MessageType::kExperimental, Config::kConstrainedPubSub,
      Backend::kFastCdr),
    std::invalid_argument);
}

TEST(ConfigTest, ValidateCase) {
  using constrained_pubsub_benchmark::Backend;
  using constrained_pubsub_benchmark::BenchmarkConfig;
  using constrained_pubsub_benchmark::Config;
  using constrained_pubsub_benchmark::MessageType;
  using constrained_pubsub_benchmark::validate_case;
  BenchmarkConfig ok;
  ok.message = MessageType::kExperimental;
  ok.config = Config::kConstrainedPubSub;
  ok.backend = Backend::kXcdr;
  EXPECT_NO_THROW(validate_case(ok));
  // Constrained configs require the experimental message.
  BenchmarkConfig bad_msg;
  bad_msg.message = MessageType::kStandard;
  bad_msg.config = Config::kConstrainedPubOnly;
  EXPECT_THROW(validate_case(bad_msg), std::invalid_argument);
  // FastCDR requires the copy config.
  BenchmarkConfig bad_backend;
  bad_backend.message = MessageType::kExperimental;
  bad_backend.config = Config::kConstrainedPubSub;
  bad_backend.backend = Backend::kFastCdr;
  EXPECT_THROW(validate_case(bad_backend), std::invalid_argument);
}

TEST(ConfigTest, TransportKeys) {
  using constrained_pubsub_benchmark::Transport;
  using constrained_pubsub_benchmark::transport_from_string;
  using constrained_pubsub_benchmark::transport_key;
  EXPECT_STREQ(transport_key(Transport::kAuto), "auto");
  EXPECT_STREQ(transport_key(Transport::kUdp), "udp");
  EXPECT_STREQ(transport_key(Transport::kShmem), "shmem");
  EXPECT_STREQ(transport_key(Transport::kShmemDs), "shmem_ds");
  EXPECT_EQ(transport_from_string("udp"), Transport::kUdp);
  EXPECT_EQ(transport_from_string("shmem_ds"), Transport::kShmemDs);
  EXPECT_THROW(transport_from_string("bogus"), std::invalid_argument);
}

TEST(ConfigTest, ParsePayloadGrid) {
  using constrained_pubsub_benchmark::parse_payload_grid;
  auto grid = parse_payload_grid("4,40,400,4K,40K,400K,4M,40M");
  ASSERT_EQ(8u, grid.size());
  EXPECT_EQ(4u, grid[0]);
  EXPECT_EQ(40u, grid[1]);
  EXPECT_EQ(400u, grid[2]);
  EXPECT_EQ(4000u, grid[3]);
  EXPECT_EQ(40000u, grid[4]);
  EXPECT_EQ(400000u, grid[5]);
  EXPECT_EQ(4000000u, grid[6]);
  EXPECT_EQ(40000000u, grid[7]);
  EXPECT_THROW(parse_payload_grid(""), std::invalid_argument);
  EXPECT_THROW(parse_payload_grid("4K,,40"), std::invalid_argument);
}

TEST(ConfigTest, FrequencyGrid) {
  using constrained_pubsub_benchmark::default_frequency_grid;
  using constrained_pubsub_benchmark::parse_frequency_grid;
  auto grid = default_frequency_grid();
  ASSERT_EQ(10u, grid.size());
  EXPECT_DOUBLE_EQ(1.0, grid[0]);
  EXPECT_DOUBLE_EQ(10.0, grid[3]);
  EXPECT_DOUBLE_EQ(100.0, grid[6]);
  EXPECT_DOUBLE_EQ(1000.0, grid[9]);
  for (size_t i = 1; i < grid.size(); ++i) {
    EXPECT_GT(grid[i], grid[i - 1]);
  }
  auto custom = parse_frequency_grid("1,10,100,1000");
  ASSERT_EQ(4u, custom.size());
  EXPECT_DOUBLE_EQ(100.0, custom[2]);
  EXPECT_THROW(parse_frequency_grid(""), std::invalid_argument);
  EXPECT_THROW(parse_frequency_grid("10,0"), std::invalid_argument);
  EXPECT_THROW(parse_frequency_grid("10,-5"), std::invalid_argument);
}

TEST(ConfigTest, RunId) {
  using constrained_pubsub_benchmark::Backend;
  using constrained_pubsub_benchmark::Config;
  using constrained_pubsub_benchmark::MessageType;
  using constrained_pubsub_benchmark::Transport;
  using constrained_pubsub_benchmark::make_run_id;
  EXPECT_EQ(
    "exp_constrained_pub_sub_xcdr__cpp_to_cpp__400000B__10Hz__shmem",
    make_run_id(
      MessageType::kExperimental, Config::kConstrainedPubSub,
      Backend::kXcdr, "cpp_to_cpp", 400000, 10.0, Transport::kShmem));
  EXPECT_EQ(
    "std_copy_fastcdr__manual__40B__1Hz__auto",
    make_run_id(
      MessageType::kStandard, Config::kCopy, Backend::kFastCdr, "", 40,
      1.0, Transport::kAuto));
}

TEST(ConfigTest, ParseNewFlags) {
  using constrained_pubsub_benchmark::Backend;
  using constrained_pubsub_benchmark::Config;
  using constrained_pubsub_benchmark::MessageType;
  using constrained_pubsub_benchmark::Transport;
  constrained_pubsub_benchmark::BenchmarkConfig cfg;
  const char * argv[] = {
    "test",
    "--message", "exp",
    "--config", "constrained_pub_sub",
    "--backend", "xcdr",
    "--transport", "shmem",
    "--run-id", "run1",
    "--direction", "cpp_to_cpp",
    "--step-index", "3",
  };
  EXPECT_TRUE(constrained_pubsub_benchmark::parse_args(15, const_cast<char **>(argv), cfg));
  EXPECT_EQ(MessageType::kExperimental, cfg.message);
  EXPECT_EQ(Config::kConstrainedPubSub, cfg.config);
  EXPECT_EQ(Backend::kXcdr, cfg.backend);
  EXPECT_EQ(Transport::kShmem, cfg.transport);
  EXPECT_EQ("run1", cfg.run_id);
  EXPECT_EQ("cpp_to_cpp", cfg.direction);
  EXPECT_EQ(3, cfg.step_index);
}

TEST(ConfigTest, ParseDefaults) {
  using constrained_pubsub_benchmark::Backend;
  using constrained_pubsub_benchmark::Config;
  using constrained_pubsub_benchmark::MessageType;
  using constrained_pubsub_benchmark::Transport;
  constrained_pubsub_benchmark::BenchmarkConfig cfg;
  const char * argv[] = {"test"};
  EXPECT_TRUE(constrained_pubsub_benchmark::parse_args(1, const_cast<char **>(argv), cfg));
  EXPECT_EQ(MessageType::kStandard, cfg.message);
  EXPECT_EQ(Config::kCopy, cfg.config);
  EXPECT_EQ(Backend::kAuto, cfg.backend);
  EXPECT_EQ(Transport::kAuto, cfg.transport);
}

TEST(ConfigTest, ParseRejectsInvalidCases) {
  constrained_pubsub_benchmark::BenchmarkConfig cfg;
  // Constrained configs require --message exp.
  const char * bad_argv[] = {"test", "--config", "constrained_pub_only"};
  EXPECT_THROW(
    constrained_pubsub_benchmark::parse_args(3, const_cast<char **>(bad_argv), cfg),
    std::invalid_argument);
  // FastCDR requires the copy config.
  const char * bad_argv2[] = {
    "test", "--message", "exp", "--config", "constrained_pub_sub",
    "--backend", "fastcdr",
  };
  EXPECT_THROW(
    constrained_pubsub_benchmark::parse_args(7, const_cast<char **>(bad_argv2), cfg),
    std::invalid_argument);
}

TEST(ConfigTest, ParseRejectsUnknownFlags) {
  constrained_pubsub_benchmark::BenchmarkConfig cfg;
  const char * bad_argv[] = {"test", "--variant", "c1"};
  EXPECT_THROW(
    constrained_pubsub_benchmark::parse_args(3, const_cast<char **>(bad_argv), cfg),
    std::invalid_argument);
  const char * bad_argv2[] = {"test", "--mode", "latency"};
  EXPECT_THROW(
    constrained_pubsub_benchmark::parse_args(3, const_cast<char **>(bad_argv2), cfg),
    std::invalid_argument);
}

TEST(ConfigTest, ParseReliability) {
  EXPECT_EQ(
    constrained_pubsub_benchmark::reliability_from_string("reliable"),
    constrained_pubsub_benchmark::Reliability::kReliable);
  EXPECT_EQ(
    constrained_pubsub_benchmark::reliability_from_string("best_effort"),
    constrained_pubsub_benchmark::Reliability::kBestEffort);
  EXPECT_THROW(
    constrained_pubsub_benchmark::reliability_from_string("bogus"),
    std::invalid_argument);
}

TEST(ConfigTest, ParseShapeKnobs) {
  constrained_pubsub_benchmark::BenchmarkConfig cfg;
  const char * argv[] = {
    "test",
    "--fill-encoding", "64",
    "--fill-frame-id", "16",
    "--fill-data-ratio", "1.0",
    "--fill-data-ratio", "0.5",
  };
  // Parse with duplicate --fill-data-ratio to verify the last one wins.
  EXPECT_TRUE(constrained_pubsub_benchmark::parse_args(9, const_cast<char **>(argv), cfg));
  EXPECT_EQ(64u, cfg.fill_encoding);
  EXPECT_EQ(16u, cfg.fill_frame_id);
  EXPECT_DOUBLE_EQ(0.5, cfg.fill_data_ratio);

  // Out-of-range ratio is rejected.
  const char * bad_argv[] = {"test", "--fill-data-ratio", "1.5"};
  EXPECT_THROW(
    constrained_pubsub_benchmark::parse_args(3, const_cast<char **>(bad_argv), cfg),
    std::invalid_argument);
  const char * bad_argv2[] = {"test", "--fill-data-ratio", "-0.1"};
  EXPECT_THROW(
    constrained_pubsub_benchmark::parse_args(3, const_cast<char **>(bad_argv2), cfg),
    std::invalid_argument);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
