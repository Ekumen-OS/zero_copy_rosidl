// Copyright 2026 Ekumen Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef MESSAGE_ACCESS_BENCHMARK__PYTHON_WORKLOAD_HPP_
#define MESSAGE_ACCESS_BENCHMARK__PYTHON_WORKLOAD_HPP_

#include <pybind11/embed.h>

#include <cstdint>
#include <stdexcept>

#include "ament_index_cpp/get_package_share_directory.hpp"
#ifdef MESSAGE_ACCESS_EXPERIMENTAL
#include "rosidl_runtime_cpython/bridge.hpp"
#endif

namespace message_access_benchmark
{
namespace py = pybind11;

inline py::module_ python_workload()
{
  auto scripts = ament_index_cpp::get_package_share_directory("message_access_benchmark") +
    "/scripts";
  py::module_::import("sys").attr("path").attr("insert")(0, scripts);
  return py::module_::import("marker_workload");
}

#ifdef MESSAGE_ACCESS_EXPERIMENTAL
inline py::object wrap_external(py::handle message_class, void * message)
{
  auto address = message_class.attr("__cpython_bridge__").cast<std::uintptr_t>();
  const auto * bridge =
    reinterpret_cast<const rosidl_runtime_cpython::MessageTypeBridge *>(address);
  if (!bridge || !bridge->wrap_cpp_message) {
    throw std::runtime_error("experimental MarkerArray has no Python message bridge");
  }
  auto * wrapped = bridge->wrap_cpp_message(message);
  if (!wrapped) {
    throw py::error_already_set();
  }
  return py::reinterpret_steal<py::object>(wrapped);
}
#endif
}  // namespace message_access_benchmark
#endif  // MESSAGE_ACCESS_BENCHMARK__PYTHON_WORKLOAD_HPP_
