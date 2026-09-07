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

#ifndef CONSTRAINED_PUBSUB_BENCHMARK__TRANSPORT_CONFIG_HPP_
#define CONSTRAINED_PUBSUB_BENCHMARK__TRANSPORT_CONFIG_HPP_

#include <cstdio>

#include <sensor_msgs/msg/experimental/image.hpp>

#include <rclcpp/rclcpp.hpp>

#include <rmw_fastrtps_cpp/serialization_backend.hpp>

#include "constrained_pubsub_benchmark/config.hpp"

namespace constrained_pubsub_benchmark
{

using ExperimentalImage = sensor_msgs::msg::experimental::Image;
using ExperimentalImageConstraints =
  rosidl_runtime_cpp::MessageConstraints<ExperimentalImage>;

/// Constraints for the cross-process Image benchmark.
/**
 * Mirrors the single-process benchmark: frame_id bound 16, encoding bound 64,
 * and a data bound equal to the payload size.
 */
ExperimentalImageConstraints
make_inter_proc_constraints(size_t payload_bytes, bool strict)
{
  ExperimentalImage::Constraints per_member;
  per_member.header.frame_id.size = 16;
  per_member.encoding.size = 64;
  per_member.data.size = payload_bytes;
  ExperimentalImageConstraints c(per_member);
  c.max_string_length = 256;
  c.max_total_size = 0;  // 0 = unlimited; use per-member
  c.strict = strict;
  return c;
}

/// Select the serialization backend on init options consumed by rclcpp::init.
/// kXcdr requests XCDR_BUFFERS; anything else keeps the stock FastCDR path.
void
apply_backend_to_init_options(rclcpp::InitOptions & init_options, Backend backend)
{
  if (backend != Backend::kXcdr) {
    return;
  }
  auto * rcl_opts = const_cast<rcl_init_options_t *>(init_options.get_rcl_init_options());
  auto * rmw_opts = rcl_init_options_get_rmw_init_options(rcl_opts);
  if (!rmw_fastrtps_cpp::set_rmw_fastrtps_serialization_backend(
      *rmw_opts, SerializationBackend::XCDR_BUFFERS))
  {
    fprintf(stderr, "WARNING: could not set XCDR serialization backend\n");
  }
}

}  // namespace constrained_pubsub_benchmark

#endif  // CONSTRAINED_PUBSUB_BENCHMARK__TRANSPORT_CONFIG_HPP_
