// Copyright 2026 xcdr_buffers Contributors
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

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <tcb_span/span.hpp>

#include "xcdr_buffers/accessor/accessor.hpp"
#include "xcdr_buffers/accessor/const_accessor.hpp"

#include "benchmark_support.hpp"

using xcdr_buffers::XCdrAccessor;
using xcdr_buffers::XCdrConstAccessor;
using xcdr_buffers::benchmark_support::SchemaSpec;
using xcdr_buffers::benchmark_support::build_layout;

namespace
{
/// Shared fixture: layout + initialized buffer for the given spec.
struct AccessorFixture
{
  explicit AccessorFixture(const SchemaSpec & spec)
  : layout(build_layout(spec, nullptr)),
    buffer(layout.total_size()),
    name(spec.name_len, 'n')
  {
    layout.apply(tcb::span<uint8_t>(buffer));
  }

  xcdr_buffers::XCdrStructLayout layout;
  std::vector<uint8_t> buffer;
  std::string name;
};
}  // namespace

/// Wraps a const accessor over the buffer (header validation + layout bind).
static void BM_AccessorWrap(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  AccessorFixture fixture(spec);
  if (xcdr_buffers::benchmark_support::skip_on_big_endian(state)) {return;}
  const tcb::span<const uint8_t> buffer(fixture.buffer);

  for (auto _ : state) {
    auto accessor = XCdrConstAccessor::wrap(buffer, fixture.layout);
    benchmark::DoNotOptimize(static_cast<bool>(accessor));
  }
}

/// Reads every member by index (no name lookup).
static void BM_AccessorReadByIndex(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  AccessorFixture fixture(spec);
  if (xcdr_buffers::benchmark_support::skip_on_big_endian(state)) {return;}
  const tcb::span<const uint8_t> buffer(fixture.buffer);
  auto accessor = *XCdrConstAccessor::wrap(buffer, fixture.layout);

  for (auto _ : state) {
    benchmark::DoNotOptimize(accessor[0].as<uint32_t>());
    benchmark::DoNotOptimize(accessor[1].as<std::string_view>());
    benchmark::DoNotOptimize(accessor[2][0].as<double>());
    benchmark::DoNotOptimize(accessor[2][1].as<double>());
    benchmark::DoNotOptimize(accessor[3][0].as<uint32_t>());
    benchmark::DoNotOptimize(accessor[4][0].as<uint8_t>());
  }
}

/// Reads members by name path (linear name scan per lookup).
static void BM_AccessorReadByName(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  AccessorFixture fixture(spec);
  if (xcdr_buffers::benchmark_support::skip_on_big_endian(state)) {return;}
  const tcb::span<const uint8_t> buffer(fixture.buffer);
  auto accessor = *XCdrConstAccessor::wrap(buffer, fixture.layout);

  for (auto _ : state) {
    benchmark::DoNotOptimize(accessor["id"].as<uint32_t>());
    benchmark::DoNotOptimize(accessor["name"].as<std::string_view>());
    benchmark::DoNotOptimize(accessor["pos.x"].as<double>());
    benchmark::DoNotOptimize(accessor["values"][0].as<uint32_t>());
    benchmark::DoNotOptimize(accessor["data"][0].as<uint8_t>());
  }
}

/// Writes primitives and a fixed-length string through a mutable accessor.
static void BM_AccessorWrite(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  AccessorFixture fixture(spec);
  if (xcdr_buffers::benchmark_support::skip_on_big_endian(state)) {return;}
  tcb::span<uint8_t> buffer(fixture.buffer);
  auto accessor = *XCdrAccessor::wrap(buffer, fixture.layout);

  for (auto _ : state) {
    accessor[0] = static_cast<uint32_t>(99);
    accessor[1] = std::string_view(fixture.name);
    accessor[2][0] = static_cast<double>(3.5);
    accessor[3][0] = static_cast<uint32_t>(11);
    accessor[4][0] = static_cast<uint8_t>(0x42);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(BM_AccessorWrap)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_AccessorReadByIndex)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_AccessorReadByName)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_AccessorWrite)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);
