// Copyright 2026 Ekumen Inc.
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <string>

#include "message_access_benchmark/marker_workload.hpp"

namespace message_access_benchmark
{
enum class MarkerOperation {Fresh, Fill, Read, Write, Strings};

template<typename Storage, MarkerOperation operation>
void marker_case(benchmark::State & state)
{
  const auto count = static_cast<std::size_t>(state.range(0));
  double seed = 1.0;
  if constexpr (operation == MarkerOperation::Fresh) {
    auto make = marker_factory<Storage>(count);
    for (auto _ : state) {
      auto fixture = make();
      auto & msg = fixture.get();
      benchmark::DoNotOptimize(seed);
      fill_markers(msg, count, seed);
      benchmark::DoNotOptimize(&msg);
      benchmark::ClobberMemory();
      seed = seed == 1.0 ? 2.0 : 1.0;
    }
  } else {
    Storage fixture(count);
    auto & msg = fixture.get();
    fill_markers(msg, count, seed);
    benchmark::DoNotOptimize(&msg);
    for (auto _ : state) {
      benchmark::DoNotOptimize(seed);
      if constexpr (operation == MarkerOperation::Read) {
        benchmark::ClobberMemory();
        auto sum = read_markers(msg);
        benchmark::DoNotOptimize(sum);
      } else {
        if constexpr (operation == MarkerOperation::Fill) {
          fill_markers(msg, count, seed);
        } else if constexpr (operation == MarkerOperation::Write) {
          write_markers(msg, seed);
        } else {
          assign_strings(msg);
        }
        benchmark::ClobberMemory();
        seed = seed == 1.0 ? 2.0 : 1.0;
      }
    }
  }
  state.SetItemsProcessed(state.iterations() * count);
  state.counters["markers"] = static_cast<double>(count);
}

template<typename Storage>
void register_markers(const std::string & representation)
{
  auto prefix = "cpp/MarkerArray/" + representation;
  auto add = [](const std::string & name, auto function) {
      benchmark::RegisterBenchmark(name.c_str(), function)
      ->Arg(1)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);
    };
  add(prefix + "/fresh", marker_case<Storage, MarkerOperation::Fresh>);
  add(prefix + "/fill", marker_case<Storage, MarkerOperation::Fill>);
  add(prefix + "/read", marker_case<Storage, MarkerOperation::Read>);
  add(prefix + "/write", marker_case<Storage, MarkerOperation::Write>);
  add(prefix + "/strings", marker_case<Storage, MarkerOperation::Strings>);
}

#ifdef MESSAGE_ACCESS_EXPERIMENTAL
void reconstruct_markers(benchmark::State & state)
{
  ExternalMarkers fixture(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    fixture.reconstruct();
    benchmark::DoNotOptimize(&fixture.get());
    benchmark::ClobberMemory();
  }
  state.counters["backing_bytes"] = static_cast<double>(fixture.backing_bytes());
}
#endif

void register_marker_benchmarks()
{
  register_markers<StandardMarkers>("standard");
#ifdef MESSAGE_ACCESS_EXPERIMENTAL
  register_markers<ExperimentalMarkers>("experimental_owning");
  register_markers<ExternalMarkers>("experimental_external");
  benchmark::RegisterBenchmark(
    "cpp/MarkerArray/experimental_external/reconstruct", reconstruct_markers)
  ->Arg(1)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);
#endif
}
}  // namespace message_access_benchmark
