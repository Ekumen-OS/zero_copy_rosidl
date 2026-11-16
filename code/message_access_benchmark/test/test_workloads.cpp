// Copyright 2026 Ekumen Inc.
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>
#include <pybind11/embed.h>

#include <string>

#ifdef MESSAGE_ACCESS_EXPERIMENTAL
#include "builtin_interfaces/msg/experimental/time.hpp"
#include "rosidl_runtime_cpp/experimental/sequence.hpp"
#endif

#include "message_access_benchmark/marker_workload.hpp"
#include "message_access_benchmark/python_workload.hpp"
#include "wide16_kernels.hpp"
#include "wide4096_kernels.hpp"

namespace bench = message_access_benchmark;

template<typename Storage>
void check_markers()
{
  Storage fixture(10000);
  auto & msg = fixture.get();
  bench::fill_markers(msg, 10000, 2.0);
  ASSERT_EQ(msg.markers.size(), 10000u);
  EXPECT_DOUBLE_EQ(bench::read_markers(msg), bench::expected_marker_sum(10000, 2.0));
  const auto & marker = msg.markers[9999];
  EXPECT_EQ(static_cast<int32_t>(marker.id), 9999);
  EXPECT_EQ(std::string(marker.header.frame_id.data(), marker.header.frame_id.size()),
    bench::frame);
  EXPECT_EQ(std::string(marker.ns.data(), marker.ns.size()), bench::marker_namespace);
  EXPECT_TRUE(marker.points.empty());
  EXPECT_TRUE(marker.colors.empty());
  EXPECT_TRUE(marker.texture.data.empty());
  EXPECT_TRUE(marker.mesh_file.data.empty());
  bench::write_markers(msg, 1.0);
  EXPECT_DOUBLE_EQ(bench::read_markers(msg), bench::expected_marker_sum(10000, 1.0));
}

TEST(MarkerWorkload, Standard)
{
  check_markers<bench::StandardMarkers>();
}

template<typename Ops, typename Storage>
void check_scalars()
{
  Storage fixture;
  auto & msg = fixture.get();
  Ops::write(msg, 2.0);
  const double count = Ops::fields;
  EXPECT_DOUBLE_EQ(Ops::read(msg), count * (count - 1) / 2 + 2 * count);
  Ops::update(msg, 3.0);
  EXPECT_DOUBLE_EQ(Ops::read(msg), count * (count - 1) / 2 + 5 * count);
}

TEST(ScalarWorkload, Standard)
{
  check_scalars<Wide16Ops, bench::Owning<Wide16Ops::Standard>>();
  check_scalars<Wide4096Ops, bench::Owning<Wide4096Ops::Standard>>();
}

#ifdef MESSAGE_ACCESS_EXPERIMENTAL
TEST(ExternalSequence, AllocatesViewsAndPreservesPrepopulatedValues)
{
  using Time = builtin_interfaces::msg::experimental::Time;
  int32_t seconds[] = {17, 23};
  uint32_t nanoseconds[] = {31, 47};
  std::vector<Time::ExternalStorage> pool(2);
  for (size_t i = 0; i < pool.size(); ++i) {
    pool[i].members.sec.assign(&seconds[i]);
    pool[i].members.nanosec.assign(&nanoseconds[i]);
  }
  rosidl_runtime_cpp::Sequence<Time> received(pool, 2);
  ASSERT_EQ(received.size(), 2u);
  EXPECT_EQ(received[0].sec.get(), 17);
  EXPECT_EQ(received[1].nanosec.get(), 47u);
  EXPECT_EQ(&received[1].sec.get(), &seconds[1]);
  received.clear();
  rosidl_runtime_cpp::Sequence<Time> fresh(pool, 0);
  fresh.resize(2);
  EXPECT_EQ(seconds[0], 0);
  EXPECT_EQ(nanoseconds[1], 0u);
  EXPECT_THROW(fresh.resize(3), std::length_error);
}

TEST(MarkerWorkload, ExperimentalOwning)
{
  check_markers<bench::ExperimentalMarkers>();
}

TEST(MarkerWorkload, ExperimentalExternal)
{
  check_markers<bench::ExternalMarkers>();
  bench::ExternalMarkers fixture(1);
  bench::fill_markers(fixture.get(), 1, 2.0);
  auto & marker = fixture.get().markers[0];
  EXPECT_TRUE(fixture.contains(&marker.id.get()));
  EXPECT_TRUE(fixture.contains(marker.ns.data()));
  EXPECT_DOUBLE_EQ(marker.pose.position.x.get(), 2.0);
  fixture.reconstruct();
  bench::fill_markers(fixture.get(), 1, 1.0);
  EXPECT_DOUBLE_EQ(bench::read_markers(fixture.get()), bench::expected_marker_sum(1, 1.0));
}

TEST(ScalarWorkload, Experimental)
{
  check_scalars<Wide16Ops, bench::Owning<Wide16Ops::Experimental>>();
  check_scalars<Wide4096Ops, bench::Owning<Wide4096Ops::Experimental>>();
  check_scalars<Wide16Ops, bench::ExternalScalar<Wide16Ops::Experimental>>();
  check_scalars<Wide4096Ops, bench::ExternalScalar<Wide4096Ops::Experimental>>();
  bench::ExternalScalar<Wide16Ops::Experimental> fixture;
  EXPECT_TRUE(fixture.contains(&fixture.get().field_0.get()));
  EXPECT_TRUE(fixture.contains(&fixture.get().field_15.get()));
}
#endif

TEST(PythonWorkload, Owning)
{
  auto module = bench::python_workload();
  for (bool experimental : {
    false,
#ifdef MESSAGE_ACCESS_EXPERIMENTAL
    true,
#endif
  })
  {
    auto cls = module.attr("message_class")(experimental);
    auto msg = module.attr("create")(cls, 10000, experimental);
    module.attr("fill")(msg, 2.0);
    EXPECT_NO_THROW(module.attr("validate")(msg, 10000, 2.0));
  }
}

#ifdef MESSAGE_ACCESS_EXPERIMENTAL
TEST(PythonWorkload, ExternalMutationsReachBacking)
{
  auto module = bench::python_workload();
  bench::ExternalMarkers fixture(10000);
  auto msg = bench::wrap_external(module.attr("message_class")(true), &fixture.get());
  module.attr("prepare")(msg, 10000, true);
  module.attr("fill")(msg, 2.0);
  EXPECT_NO_THROW(module.attr("validate")(msg, 10000, 2.0));
  EXPECT_DOUBLE_EQ(
    bench::read_markers(fixture.get()), bench::expected_marker_sum(10000, 2.0));
  pybind11::object nested = msg.attr("markers")[pybind11::int_(9999)];
  msg = pybind11::none();
  nested.attr("id") = 17;
  EXPECT_EQ(fixture.get().markers[9999].id.get(), 17);
}
#endif

int main(int argc, char ** argv)
{
  pybind11::scoped_interpreter interpreter;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
