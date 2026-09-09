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

#include <cstdint>
#include <string>

#include <sensor_msgs/msg/experimental/image.hpp>
#include <sensor_msgs/msg/image.hpp>

#include "constrained_pubsub_benchmark/sample_record.hpp"

namespace rec = constrained_pubsub_benchmark;

TEST(SampleRecordTest, EventKeys) {
  EXPECT_STREQ(rec::event_key(rec::Event::kPublish), "publish");
  EXPECT_STREQ(rec::event_key(rec::Event::kReceive), "receive");
  EXPECT_STREQ(rec::event_key(rec::Event::kError), "error");
}

TEST(SampleRecordTest, ProcessKeys) {
  EXPECT_STREQ(rec::process_key(rec::Process::kPub), "pub");
  EXPECT_STREQ(rec::process_key(rec::Process::kSub), "sub");
  EXPECT_STREQ(rec::process_key(rec::Process::kIntra), "intra");
}

TEST(SampleRecordTest, MessageKeys) {
  EXPECT_STREQ(
    rec::message_type_key(rec::MessageType::kStandard), "std");
  EXPECT_STREQ(
    rec::message_type_key(rec::MessageType::kExperimental), "exp");
}

TEST(SampleRecordTest, HeaderColumns) {
  EXPECT_EQ(
    "run_id,event,process,direction,config,message,backend,transport,"
    "payload_bytes,target_frequency_hz,step_index,phase,sequence,"
    "observed_utc,observed_mono_ns,send_mono_ns,receive_mono_ns,"
    "middleware_source_ns,middleware_received_ns,status,error",
    rec::csv_header());
}

TEST(SampleRecordTest, EscapeField) {
  EXPECT_EQ("plain", rec::escape_csv_field("plain"));
  EXPECT_EQ("\"a,b\"", rec::escape_csv_field("a,b"));
  EXPECT_EQ("\"a\"\"b\"", rec::escape_csv_field("a\"b"));
  EXPECT_EQ("\"a\nb\"", rec::escape_csv_field("a\nb"));
  EXPECT_EQ("", rec::escape_csv_field(""));
}

TEST(SampleRecordTest, SampleToCsv) {
  rec::Sample s;
  s.run_id = "exp_constrained_pub_sub_xcdr__cpp_to_cpp__400000B__10Hz__shmem";
  s.event = rec::Event::kReceive;
  s.process = rec::Process::kSub;
  s.direction = rec::Direction::kCppToCpp;
  s.config = rec::Config::kConstrainedPubSub;
  s.message = rec::MessageType::kExperimental;
  s.backend = rec::Backend::kXcdr;
  s.transport = rec::Transport::kShmem;
  s.payload_bytes = 400000;
  s.target_frequency_hz = 10.0;
  s.step_index = 3;
  s.sequence = 42;
  s.observed_utc = "2026-09-06T21:30:00.123456Z";
  s.send_mono_ns = 900;
  s.receive_mono_ns = 950;
  // observed_mono_ns derives from the event (receive -> receive_mono_ns),
  // status derives too, phase is always "measured".
  EXPECT_EQ(
    "exp_constrained_pub_sub_xcdr__cpp_to_cpp__400000B__10Hz__shmem,"
    "receive,sub,cpp_to_cpp,constrained_pub_sub,exp,xcdr,shmem,400000,10,3,"
    "measured,42,2026-09-06T21:30:00.123456Z,950,900,950,-1,-1,ok,",
    rec::sample_to_csv(s));
}

TEST(SampleRecordTest, PublishDerivesObservedFromSend) {
  rec::Sample s;
  s.event = rec::Event::kPublish;
  s.send_mono_ns = 100;
  s.receive_mono_ns = -1;
  std::string row = rec::sample_to_csv(s);
  // ...,observed_mono_ns,send_mono_ns,... -> "100,100,-1,..."
  EXPECT_NE(std::string::npos, row.find(",measured,0,,100,100,-1,"));
}

TEST(SampleRecordTest, ErrorRowEscapes) {
  rec::Sample s;
  s.event = rec::Event::kError;
  s.error = "create_publisher failed: a, \"b\"";
  std::string row = rec::sample_to_csv(s);
  EXPECT_NE(std::string::npos, row.find("\"create_publisher failed: a, \"\"b\"\"\""));
  EXPECT_NE(std::string::npos, row.find(",error,\"create_publisher"));
}

TEST(SampleRecordTest, ContextBuffersRowsUntilFlush) {
  rec::BenchmarkConfig cfg;
  cfg.run_id = "r";
  cfg.message = rec::MessageType::kStandard;
  cfg.config = rec::Config::kCopy;
  cfg.backend = rec::Backend::kAuto;
  cfg.direction = "cpp_to_cpp";
  cfg.payload_bytes = 400;
  cfg.publish_rate_hz = 10.0;
  cfg.transport = rec::Transport::kShmem;
  rec::RunContext ctx = rec::RunContext::from_config(
    cfg, rec::Process::kPub, rec::Direction::kCppToCpp,
    rec::Backend::kFastCdr, 10);
  // 20% margin lives inside from_config().
  EXPECT_GE(ctx.rows.capacity(), 12u);
  EXPECT_EQ("r", ctx.run_id);
  EXPECT_EQ(rec::MessageType::kStandard, ctx.message);
  EXPECT_EQ(rec::Config::kCopy, ctx.config);
  EXPECT_EQ(rec::Backend::kFastCdr, ctx.backend);
  EXPECT_EQ(400u, ctx.payload_bytes);
  ctx.emit(rec::Event::kPublish, 0, 100, -1, -1, -1);
  ctx.emit(rec::Event::kPublish, 1, 200, -1, -1, -1);
  // Buffered, not yet written: timestamps captured at event time.
  ASSERT_EQ(2u, ctx.rows.size());
  EXPECT_EQ(0u, ctx.rows[0].sequence);
  EXPECT_EQ(100, ctx.rows[0].send_mono_ns);
  EXPECT_EQ(1u, ctx.rows[1].sequence);
  ctx.flush_rows();
  EXPECT_TRUE(ctx.rows.empty());
}

TEST(SampleRecordTest, FromConfigMintsRunId) {
  rec::BenchmarkConfig cfg;
  cfg.message = rec::MessageType::kExperimental;
  cfg.config = rec::Config::kConstrainedPubSub;
  cfg.payload_bytes = 400000;
  cfg.publish_rate_hz = 10.0;
  cfg.transport = rec::Transport::kShmem;
  rec::RunContext ctx = rec::RunContext::from_config(
    cfg, rec::Process::kSub, rec::Direction::kCppToCpp,
    rec::Backend::kXcdr, 10);
  const std::string prefix =
    "exp_constrained_pub_sub_xcdr__cpp_to_cpp__400000B__10Hz__shmem__reliable__pid";
  EXPECT_EQ(prefix, ctx.run_id.substr(0, prefix.size()));
  EXPECT_EQ(rec::Direction::kCppToCpp, ctx.direction);
}

TEST(SampleRecordTest, ParseDirectionOr) {
  using rec::Direction;
  EXPECT_EQ(
    rec::parse_direction_or("", Direction::kIntraCpp), Direction::kIntraCpp);
  EXPECT_EQ(
    rec::parse_direction_or("py_to_py", Direction::kCppToCpp),
    Direction::kPyToPy);
  EXPECT_THROW(
    rec::parse_direction_or("bogus", Direction::kCppToCpp),
    std::invalid_argument);
}

TEST(SampleRecordTest, Clocks) {
  int64_t t0 = rec::mono_now_ns();
  int64_t t1 = rec::mono_now_ns();
  EXPECT_LE(t0, t1);
  EXPECT_GT(t1, 0);
  std::string utc = rec::utc_now_iso8601();
  ASSERT_EQ(27u, utc.size());
  EXPECT_EQ('T', utc[10]);
  EXPECT_EQ('Z', utc[26]);
}

template<typename ImageT>
static void check_metadata_codec()
{
  ImageT msg;
  rec::encode_sample_metadata(msg, 42, 1234567890123456789LL);
  // 16-char frame_id satisfies the c1/c2 bound exactly.
  EXPECT_EQ(16u, msg.header.frame_id.size());
  uint64_t seq = 0;
  int64_t send_ns = 0;
  EXPECT_TRUE(rec::decode_sample_metadata(msg, &seq, &send_ns));
  EXPECT_EQ(42u, seq);
  EXPECT_EQ(1234567890123456789LL, send_ns);
}

TEST(SampleRecordTest, MetadataCodecStdImage) {
  check_metadata_codec<sensor_msgs::msg::Image>();
}

TEST(SampleRecordTest, MetadataCodecExperimentalImage) {
  check_metadata_codec<sensor_msgs::msg::experimental::Image>();
}

TEST(SampleRecordTest, MetadataCodecRejectsForeign) {
  sensor_msgs::msg::Image msg;
  msg.header.frame_id = "benchmark";
  uint64_t seq = 7;
  int64_t send_ns = 7;
  EXPECT_FALSE(rec::decode_sample_metadata(msg, &seq, &send_ns));
  EXPECT_EQ(0u, seq);
  EXPECT_EQ(0, send_ns);

  msg.header.frame_id = "s12345";
  EXPECT_FALSE(rec::decode_sample_metadata(msg, &seq, &send_ns));

  msg.header.frame_id = "s0123456789abcdef";
  EXPECT_FALSE(rec::decode_sample_metadata(msg, &seq, &send_ns));
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
