// Copyright 2026 Ekumen Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef MESSAGE_ACCESS_BENCHMARK__SCALAR_BENCHMARK_HPP_
#define MESSAGE_ACCESS_BENCHMARK__SCALAR_BENCHMARK_HPP_

#include <benchmark/benchmark.h>

#include <string>

#include "message_access_benchmark/storage.hpp"

namespace message_access_benchmark
{

enum class ScalarOperation {Read, Write, Update};

template<typename Ops, typename Storage, ScalarOperation operation, bool opaque>
void scalar_case(benchmark::State & state)
{
  Storage fixture;
  auto & msg = fixture.get();
  Ops::write(msg, 1.0);
  benchmark::DoNotOptimize(&msg);
  double seed = 1.0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(seed);
    if constexpr (operation == ScalarOperation::Read) {
      benchmark::ClobberMemory();
      double sum;
      if constexpr (opaque) {
        sum = Ops::opaque_read(msg);
      } else {
        sum = Ops::read(msg);
      }
      benchmark::DoNotOptimize(sum);
    } else {
      if constexpr (operation == ScalarOperation::Write) {
        if constexpr (opaque) {
          Ops::opaque_write(msg, seed);
        } else {
          Ops::write(msg, seed);
        }
      } else {
        if constexpr (opaque) {
          Ops::opaque_update(msg, seed);
        } else {
          Ops::update(msg, seed);
        }
      }
      benchmark::ClobberMemory();
      seed = seed == 1.0 ? 2.0 : 1.0;
    }
  }
  state.SetItemsProcessed(state.iterations() * Ops::fields);
  state.counters["fields"] = Ops::fields;
  state.counters["view_bytes"] = sizeof(typename Storage::MessageType);
}

template<typename Ops, typename Storage, bool opaque>
void register_scalar_context(const std::string & prefix)
{
  std::string context = opaque ? "/opaque" : "/visible";
  benchmark::RegisterBenchmark(
    (prefix + "/read" + context).c_str(),
    scalar_case<Ops, Storage, ScalarOperation::Read, opaque>);
  benchmark::RegisterBenchmark(
    (prefix + "/write" + context).c_str(),
    scalar_case<Ops, Storage, ScalarOperation::Write, opaque>);
  benchmark::RegisterBenchmark(
    (prefix + "/update" + context).c_str(),
    scalar_case<Ops, Storage, ScalarOperation::Update, opaque>);
}

template<typename Ops, typename Storage>
void register_scalar_storage(const std::string & representation)
{
  auto prefix = std::string("cpp/") + Ops::name + "/" + representation;
  register_scalar_context<Ops, Storage, false>(prefix);
  register_scalar_context<Ops, Storage, true>(prefix);
}

template<typename Ops>
void register_scalar()
{
  register_scalar_storage<Ops, Owning<typename Ops::Standard>>("standard");
#ifdef MESSAGE_ACCESS_EXPERIMENTAL
  register_scalar_storage<Ops, Owning<typename Ops::Experimental>>("experimental_owning");
  register_scalar_storage<Ops, ExternalScalar<typename Ops::Experimental>>(
    "experimental_external");
#endif
}

}  // namespace message_access_benchmark
#endif  // MESSAGE_ACCESS_BENCHMARK__SCALAR_BENCHMARK_HPP_
