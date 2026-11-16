// Copyright 2026 Ekumen Inc.
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <memory>
#include <string>

#include "message_access_benchmark/marker_workload.hpp"
#include "message_access_benchmark/python_workload.hpp"

namespace message_access_benchmark
{
void python_case(
  benchmark::State & state, const std::string & representation,
  const std::string & operation)
{
  auto module = python_workload();
  const bool experimental = representation != "standard";
  auto message_class = module.attr("message_class")(experimental);
  auto count = static_cast<std::size_t>(state.range(0));
  auto fill = module.attr("fill");
  auto create = module.attr("create");
  auto prepare = module.attr("prepare");
  auto function = operation == "fresh" ? fill : module.attr(operation.c_str());
#ifdef MESSAGE_ACCESS_EXPERIMENTAL
  std::shared_ptr<rosidl_message_type_support_t> support;
  std::unique_ptr<ExternalMarkers> external;
#endif
  py::object msg;
#ifdef MESSAGE_ACCESS_EXPERIMENTAL
  if (representation == "experimental_external") {
    support = marker_support(count);
    external = std::make_unique<ExternalMarkers>(support);
    msg = wrap_external(message_class, &external->get());
    prepare(msg, count, true);
  } else  // NOLINT(readability/braces)
#endif
  {
    msg = create(message_class, count, experimental);
  }
  fill(msg, 1.0);
  module.attr("validate")(msg, count, 1.0);
  double seed = 1.0;
  for (auto _ : state) {
    if (operation == "fresh") {
#ifdef MESSAGE_ACCESS_EXPERIMENTAL
      if (external) {
        ExternalMarkers fresh(support);
        auto wrapped = wrap_external(message_class, &fresh.get());
        prepare(wrapped, count, true);
        fill(wrapped, seed);
      } else  // NOLINT(readability/braces)
#endif
      {
        auto fresh = create(message_class, count, experimental);
        fill(fresh, seed);
      }
    } else {
      auto result = function(msg, seed);
      benchmark::DoNotOptimize(result.ptr());
    }
    seed = seed == 1.0 ? 2.0 : 1.0;
  }
  // Non-owning Python wrappers must die before the backing C++ fixture.
  msg = py::none();
  state.SetItemsProcessed(state.iterations() * count);
  state.counters["markers"] = static_cast<double>(count);
}

void register_python_benchmarks()
{
  for (const auto * representation : {
      "standard",
#ifdef MESSAGE_ACCESS_EXPERIMENTAL
      "experimental_owning", "experimental_external",
#endif
    })
  {
    for (const auto * operation : {"fresh", "fill", "read", "write", "strings"}) {
      std::string name = std::string("python/MarkerArray/") + representation + "/" + operation;
      benchmark::RegisterBenchmark(name.c_str(), python_case,
        std::string(representation), std::string(operation))
      ->Arg(1)->Arg(100)->Arg(1000)->Arg(10000)
      ->UseRealTime()->Unit(benchmark::kMicrosecond);
    }
  }
}
}  // namespace message_access_benchmark
