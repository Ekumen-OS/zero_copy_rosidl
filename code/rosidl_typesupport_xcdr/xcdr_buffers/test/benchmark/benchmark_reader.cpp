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

#include "xcdr_buffers/serialization/reader.hpp"

#include "benchmark_support.hpp"

using xcdr_buffers::XCdrReader;
using xcdr_buffers::benchmark_support::SchemaSpec;
using xcdr_buffers::benchmark_support::write_payload;

namespace
{
/// Shared fixture: pre-generated payload for the given spec.
struct ReaderFixture
{
  explicit ReaderFixture(const SchemaSpec & spec)
  : payload(write_payload(spec, nullptr)),
    buffer(payload.data(), payload.size())
  {
  }

  std::pmr::vector<uint8_t> payload;
  tcb::span<const uint8_t> buffer;
};
}  // namespace

/// Constructs a reader (header validation) over the payload.
static void BM_ReaderConstruct(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  ReaderFixture fixture(spec);
  if (xcdr_buffers::benchmark_support::skip_on_big_endian(state)) {return;}

  for (auto _ : state) {
    XCdrReader reader(fixture.buffer);
    benchmark::DoNotOptimize(reader.bytes_remaining());
  }
}

/// Sequential zero-copy reads: primitives, string_view, span.
///
/// Note: the schema's fixed array has no length prefix, and the reader's
/// dynamic-extent span path reads a sequence prefix, so the array is not
/// read here (accessor benchmarks cover fixed-array access).
static void BM_ReaderReadZeroCopy(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  ReaderFixture fixture(spec);
  if (xcdr_buffers::benchmark_support::skip_on_big_endian(state)) {return;}

  for (auto _ : state) {
    XCdrReader reader(fixture.buffer);
    benchmark::DoNotOptimize(*reader.read<uint32_t>());
    benchmark::DoNotOptimize(reader.read<std::string_view>()->size());
    benchmark::DoNotOptimize(*reader.read<double>());
    benchmark::DoNotOptimize(*reader.read<double>());
    benchmark::DoNotOptimize(reader.read<tcb::span<const uint32_t>>()->size());
  }
}

/// Sequential allocating reads: std::string and std::vector.
static void BM_ReaderReadAllocating(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  ReaderFixture fixture(spec);
  if (xcdr_buffers::benchmark_support::skip_on_big_endian(state)) {return;}

  for (auto _ : state) {
    XCdrReader reader(fixture.buffer);
    benchmark::DoNotOptimize(*reader.read<uint32_t>());
    benchmark::DoNotOptimize(reader.read<std::string>()->size());
    benchmark::DoNotOptimize(*reader.read<double>());
    benchmark::DoNotOptimize(*reader.read<double>());
    benchmark::DoNotOptimize(reader.read<std::vector<uint32_t>>()->size());
  }
}

/// read_into() with pre-allocated containers (reuses allocations).
static void BM_ReaderReadInto(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  ReaderFixture fixture(spec);
  if (xcdr_buffers::benchmark_support::skip_on_big_endian(state)) {return;}

  uint32_t id = 0;
  std::string name;
  double x = 0.0;
  double y = 0.0;
  std::vector<uint32_t> values;

  for (auto _ : state) {
    XCdrReader reader(fixture.buffer);
    benchmark::DoNotOptimize(reader.read_into(id));
    benchmark::DoNotOptimize(reader.read_into(name));
    benchmark::DoNotOptimize(reader.read_into(x));
    benchmark::DoNotOptimize(reader.read_into(y));
    benchmark::DoNotOptimize(reader.read_into(values));
  }
}

BENCHMARK(BM_ReaderConstruct)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_ReaderReadZeroCopy)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_ReaderReadAllocating)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_ReaderReadInto)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);
