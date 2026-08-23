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

#include <tcb_span/span.hpp>

#include "benchmark_support.hpp"

using xcdr_buffers::benchmark_support::SchemaSpec;
using xcdr_buffers::benchmark_support::parse_payload;
using xcdr_buffers::benchmark_support::write_payload;
using xcdr_buffers::benchmark_support::kArenaBytes;

/// Parses a pre-generated payload into a layout with a fresh monotonic
/// arena per iteration (null upstream, so heap fallback fails loudly).
static void BM_LayoutParse(benchmark::State & state)
{
  const SchemaSpec spec{
    static_cast<size_t>(state.range(0)),  // name_len
    static_cast<size_t>(state.range(1)),  // seq_count
    static_cast<size_t>(state.range(2))};  // array_count

  // Payload generation is setup, not part of the measured parse.
  auto payload = write_payload(spec, nullptr);
  const tcb::span<const uint8_t> buffer(payload.data(), payload.size());

  alignas(std::max_align_t) std::byte arena[kArenaBytes];

  for (auto _ : state) {
    std::pmr::monotonic_buffer_resource mr(
      arena, sizeof(arena), std::pmr::null_memory_resource());
    {
      auto result = parse_payload(buffer, spec, &mr);
      if (!result) {
        state.SkipWithError("parse_payload failed");
        break;
      }
      benchmark::DoNotOptimize(*result);
    }
    mr.release();
  }
}

BENCHMARK(BM_LayoutParse)
->Args({8, 64, 256})
->Args({32, 1024, 4096})
->Unit(benchmark::kMicrosecond);
