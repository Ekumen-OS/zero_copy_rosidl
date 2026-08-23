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

#include "xcdr_buffers/serialization/writer.hpp"

#include "benchmark_support.hpp"

using xcdr_buffers::XCdrWriter;
using xcdr_buffers::benchmark_support::SchemaSpec;
using xcdr_buffers::benchmark_support::write_payload;

namespace
{
/// Shared fixture: value vectors + a fixed buffer sized from the payload.
struct WriterFixture
{
  explicit WriterFixture(const SchemaSpec & spec)
  : name(spec.name_len, 'n'),
    values(spec.seq_count, 7u),
    data(spec.array_count, static_cast<uint8_t>(0xAB)),
    payload(write_payload(spec, nullptr)),
    fixed_buffer(payload.size())
  {
  }

  std::string name;
  std::vector<uint32_t> values;
  std::vector<uint8_t> data;
  std::pmr::vector<uint8_t> payload;
  std::vector<uint8_t> fixed_buffer;
};

/// Writes the schema fields into a growing writer (no flush).
static void BM_WriterWriteGrowing(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  WriterFixture fixture(spec);

  for (auto _ : state) {
    XCdrWriter writer;
    writer.write(static_cast<uint32_t>(42));
    writer.write(std::string_view(fixture.name));
    writer.begin_write_struct();
    writer.write(static_cast<double>(1.5));
    writer.write(static_cast<double>(2.5));
    writer.end_write_struct();
    writer.write_sequence(tcb::span<const uint32_t>(fixture.values));
    writer.write_array(tcb::span<const uint8_t>(fixture.data));
    benchmark::DoNotOptimize(writer.bytes_written());
  }
}

/// Writes the schema fields and flushes (buffer move + reset).
static void BM_WriterWriteFlush(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  WriterFixture fixture(spec);

  for (auto _ : state) {
    XCdrWriter writer;
    writer.write(static_cast<uint32_t>(42));
    writer.write(std::string_view(fixture.name));
    writer.begin_write_struct();
    writer.write(static_cast<double>(1.5));
    writer.write(static_cast<double>(2.5));
    writer.end_write_struct();
    writer.write_sequence(tcb::span<const uint32_t>(fixture.values));
    writer.write_array(tcb::span<const uint8_t>(fixture.data));
    auto buffer = writer.flush();
    benchmark::DoNotOptimize(buffer.size());
  }
}

/// Writes into a pre-sized fixed span (no dynamic allocation).
static void BM_WriterWriteFixed(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  WriterFixture fixture(spec);

  for (auto _ : state) {
    XCdrWriter writer(tcb::span<uint8_t>(fixture.fixed_buffer));
    writer.write(static_cast<uint32_t>(42));
    writer.write(std::string_view(fixture.name));
    writer.begin_write_struct();
    writer.write(static_cast<double>(1.5));
    writer.write(static_cast<double>(2.5));
    writer.end_write_struct();
    writer.write_sequence(tcb::span<const uint32_t>(fixture.values));
    writer.write_array(tcb::span<const uint8_t>(fixture.data));
    benchmark::DoNotOptimize(writer.bytes_written());
  }
}

/// Writes into a fixed span at an offset (compaction-style: header already
/// present, only the data payload is rewritten).
static void BM_WriterWriteFixedOffset(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),
    static_cast<size_t>(state.range(1)),
    static_cast<size_t>(state.range(2))};
  WriterFixture fixture(spec);

  for (auto _ : state) {
    XCdrWriter writer(tcb::span<uint8_t>(fixture.fixed_buffer), 4);
    writer.write(static_cast<uint32_t>(42));
    writer.write(std::string_view(fixture.name));
    writer.begin_write_struct();
    writer.write(static_cast<double>(1.5));
    writer.write(static_cast<double>(2.5));
    writer.end_write_struct();
    writer.write_sequence(tcb::span<const uint32_t>(fixture.values));
    writer.write_array(tcb::span<const uint8_t>(fixture.data));
    benchmark::DoNotOptimize(writer.bytes_written());
  }
}

BENCHMARK(BM_WriterWriteGrowing)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_WriterWriteFlush)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_WriterWriteFixed)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_WriterWriteFixedOffset)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

}  // namespace
