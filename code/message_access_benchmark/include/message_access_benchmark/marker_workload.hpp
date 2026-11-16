// Copyright 2026 Ekumen Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef MESSAGE_ACCESS_BENCHMARK__MARKER_WORKLOAD_HPP_
#define MESSAGE_ACCESS_BENCHMARK__MARKER_WORKLOAD_HPP_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

#include "visualization_msgs/msg/marker_array.hpp"
#include "message_access_benchmark/storage.hpp"

#ifdef MESSAGE_ACCESS_EXPERIMENTAL
#include "visualization_msgs/msg/experimental/marker_array.hpp"
#include "visualization_msgs/msg/experimental/detail/marker_array__rosidl_typesupport_xcdr_cpp.hpp"
#endif

namespace message_access_benchmark
{

inline const std::string frame = "benchmark_frame";
inline const std::string marker_namespace = "benchmark_cubes";

using StandardMarkers = Owning<visualization_msgs::msg::MarkerArray>;

template<typename Message>
void write_markers(Message & msg, double seed)
{
  for (std::size_t i = 0; i < msg.markers.size(); ++i) {
    auto & marker = msg.markers[i];
    marker.id = static_cast<int32_t>(i);
    marker.type = 1;
    marker.action = 0;
    marker.pose.position.x = seed + static_cast<double>(i);
    marker.pose.position.y = seed;
    marker.pose.position.z = 0.0;
    marker.pose.orientation.x = 0.0;
    marker.pose.orientation.y = 0.0;
    marker.pose.orientation.z = 0.0;
    marker.pose.orientation.w = 1.0;
    marker.scale.x = 1.0;
    marker.scale.y = 1.0;
    marker.scale.z = 1.0;
    marker.color.r = 0.25f;
    marker.color.g = 0.5f;
    marker.color.b = 0.75f;
    marker.color.a = 1.0f;
  }
}

template<typename Message>
void assign_strings(Message & msg)
{
  for (auto & marker : msg.markers) {
    marker.header.frame_id.assign(frame);
    marker.ns.assign(marker_namespace);
  }
}

template<typename Message>
void fill_markers(Message & msg, std::size_t count, double seed)
{
  msg.markers.resize(count);
  write_markers(msg, seed);
  assign_strings(msg);
}

template<typename Message>
double read_markers(const Message & msg)
{
  double sum = 0;
  for (const auto & marker : msg.markers) {
    sum += static_cast<int32_t>(marker.id);
    sum += static_cast<double>(marker.pose.position.x);
    sum += static_cast<double>(marker.pose.position.y);
    sum += static_cast<double>(marker.pose.position.z);
    sum += static_cast<double>(marker.pose.orientation.w);
    sum += static_cast<double>(marker.scale.x);
    sum += static_cast<double>(marker.scale.y);
    sum += static_cast<double>(marker.scale.z);
    sum += static_cast<float>(marker.color.r);
    sum += static_cast<float>(marker.color.g);
    sum += static_cast<float>(marker.color.b);
    sum += static_cast<float>(marker.color.a);
  }
  return sum;
}

inline double expected_marker_sum(std::size_t count, double seed)
{
  return static_cast<double>(count) *
         (static_cast<double>(count) - 1.0 + 2.0 * seed + 6.5);
}

#ifdef MESSAGE_ACCESS_EXPERIMENTAL
using ExperimentalMarkers = Owning<visualization_msgs::msg::experimental::MarkerArray>;

inline std::shared_ptr<rosidl_message_type_support_t> marker_support(std::size_t count)
{
  using Message = visualization_msgs::msg::experimental::MarkerArray;
  Message::Constraints constraints;
  constraints.markers.size = count;
  auto & marker = constraints.markers.element;
  marker.header.frame_id.size = frame.size();
  marker.ns.size = marker_namespace.size();
  marker.points.size = 0;
  marker.colors.size = 0;
  marker.texture_resource.size = 0;
  marker.texture.header.frame_id.size = 0;
  marker.texture.format.size = 0;
  marker.texture.data.size = 0;
  marker.uv_coordinates.size = 0;
  marker.text.size = 0;
  marker.mesh_resource.size = 0;
  marker.mesh_file.filename.size = 0;
  marker.mesh_file.data.size = 0;
  rosidl_message_type_constraints_t wrapped{};
  wrapped.type_specific = &constraints;
  auto support = rosidl_typesupport_xcdr_cpp::create_constrained_message_type_support(
    rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<Message>(), &wrapped);
  if (!support) {
    check_xcdr(RCUTILS_RET_ERROR, "create_constrained_message_type_support");
  }
  return support;
}

class ExternalMarkers : public External<visualization_msgs::msg::experimental::MarkerArray>
{
public:
  explicit ExternalMarkers(std::size_t count)
  : ExternalMarkers(marker_support(count)) {}

  explicit ExternalMarkers(std::shared_ptr<rosidl_message_type_support_t> support)
  : External(support.get(), support) {}
};
#endif

template<typename Storage>
auto marker_factory(std::size_t count)
{
#ifdef MESSAGE_ACCESS_EXPERIMENTAL
  if constexpr (std::is_same_v<Storage, ExternalMarkers>) {
    auto support = marker_support(count);
    return [support]() {return ExternalMarkers(support);};
  } else  // NOLINT(readability/braces)
#endif
  {
    return [count]() {return Storage(count);};
  }
}

}  // namespace message_access_benchmark
#endif  // MESSAGE_ACCESS_BENCHMARK__MARKER_WORKLOAD_HPP_
