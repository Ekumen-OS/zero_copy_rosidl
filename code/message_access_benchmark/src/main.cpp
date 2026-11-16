// Copyright 2026 Ekumen Inc.
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>
#include <pybind11/embed.h>

#include <exception>
#include <iostream>

namespace message_access_benchmark
{
void register_marker_benchmarks();
void register_python_benchmarks();
}

int main(int argc, char ** argv)
{
  pybind11::scoped_interpreter interpreter;
  try {
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
      return 1;
    }
    pybind11::module_::import("gc").attr("disable")();
    benchmark::AddCustomContext("build_type", BENCHMARK_BUILD_TYPE);
    benchmark::AddCustomContext("compiler", BENCHMARK_COMPILER);
    benchmark::AddCustomContext("python", Py_GetVersion());
    benchmark::AddCustomContext("python_gc", "disabled");
    benchmark::AddCustomContext("transport", "none");
#ifdef MESSAGE_ACCESS_EXPERIMENTAL
    benchmark::AddCustomContext("experimental", "enabled");
#else
    benchmark::AddCustomContext("experimental", "disabled");
#endif
    message_access_benchmark::register_marker_benchmarks();
    message_access_benchmark::register_python_benchmarks();
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
  } catch (const std::exception & error) {
    std::cerr << "message_access: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
