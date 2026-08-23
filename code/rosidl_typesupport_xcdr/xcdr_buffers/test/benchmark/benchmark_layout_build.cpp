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
#include <memory_resource>
#include <new>
#include <vector>

#include <tcb_span/span.hpp>

#include "benchmark_support.hpp"

using xcdr_buffers::benchmark_support::SchemaSpec;
using xcdr_buffers::benchmark_support::build_layout;
using xcdr_buffers::benchmark_support::kArenaBytes;

/// Builds the schema layout with a fresh monotonic arena per iteration.
///
/// The arena uses std::pmr::null_memory_resource() as upstream: any
/// allocation that does not fit in the fixed buffer throws std::bad_alloc
/// instead of silently falling back to the heap, so the benchmark fails
/// loudly if the arena is ever undersized.
static void BM_LayoutBuild(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),  // name_len
    static_cast<size_t>(state.range(1)),  // seq_count
    static_cast<size_t>(state.range(2))};  // array_count

  alignas(std::max_align_t) std::byte arena[kArenaBytes];

  for (auto _ : state) {
    std::pmr::monotonic_buffer_resource mr(
      arena, sizeof(arena), std::pmr::null_memory_resource());
    {
      auto layout = build_layout(spec, &mr);
      benchmark::DoNotOptimize(layout);
    }
    mr.release();
  }
}

BENCHMARK(BM_LayoutBuild)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);

/// Applies the layout to a pre-sized buffer: writes the XCDR header and the
/// length prefixes for variable-length fields (strings, sequences), without
/// zero initialization. No allocation occurs, so no arena is needed.
static void BM_LayoutApply(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),  // name_len
    static_cast<size_t>(state.range(1)),  // seq_count
    static_cast<size_t>(state.range(2))};  // array_count

  auto layout = build_layout(spec, nullptr);
  std::vector<uint8_t> buffer(layout.total_size());

  for (auto _ : state) {
    auto status = layout.apply(tcb::span<uint8_t>(buffer));
    benchmark::DoNotOptimize(static_cast<bool>(status));
  }
}

BENCHMARK(BM_LayoutApply)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);
