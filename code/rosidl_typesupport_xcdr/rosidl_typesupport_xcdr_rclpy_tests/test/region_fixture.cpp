// Copyright 2026 Ekumen, Inc.
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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "rosidl_runtime_cpython/bridge.hpp"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/bounded_variable__region.hpp"

namespace py = pybind11;
using View = rosidl_typesupport_xcdr_cpp_tests::msg::views::RegionBoundedVariable;
using Constraints = View::Constraints;
using State = rosidl_runtime_cpython::BorrowedViewState;
using Name = std::remove_reference_t<decltype(std::declval<View &>().name())>;

struct NameView
{
  rosidl_runtime_cpython::BorrowedView<Name> value;
};

struct MessageView
{
  rosidl_runtime_cpython::BorrowedView<View> value;
  bool read_only;
};

const auto bridge = rosidl_runtime_cpython::make_message_type_bridge<View>(
  []() {return rosidl_typesupport_cpp::get_message_type_support_handle<View>();},
  [](void * message, std::shared_ptr<State> state, bool read_only) -> PyObject * {
    return py::cast(MessageView{
    {static_cast<View *>(message), std::move(state)}, read_only}).release().ptr();
  },
  [](PyObject * object) -> const void * {
    if (!py::isinstance<Constraints>(py::handle(object))) {
      throw py::type_error("Expected BoundedVariable constraints");
    }
    return py::cast<const Constraints *>(py::handle(object));
  },
  []() {return rosidl_typesupport_cpp::get_message_type_support_handle<View::PlainMessage>();});

PYBIND11_MODULE(_region_fixture, module)
{
  py::class_<Constraints>(module, "Constraints", py::module_local())
  .def(py::init<>())
  .def_property(
    "name_size", [](const Constraints & c) {return c.name.size;},
    [](Constraints & c, size_t size) {c.name.size = size;});

  py::class_<NameView>(module, "NameView")
  .def("__str__", [](const NameView & name) {
      auto & string = name.value.get();
      return std::string(string.data(), string.size());
    });

  py::class_<MessageView> cls(module, "BoundedVariable");
  cls.def_property(
    "name",
    [](const MessageView & message) {
      auto & name = message.value.get().name();
      return std::string(name.data(), name.size());
    },
    [](const MessageView & message, const std::string & name) {
      if (message.read_only) {
        throw py::type_error("Subscription views are read-only");
      }
      message.value.get().name().assign(name);
    })
  .def_property_readonly(
    "name_view", [](const MessageView & message) {
      return NameView{{&message.value.get().name(), message.value.state()}};
    })
  .def_property(
    "tail", [](const MessageView & message) {return message.value.get().tail();},
    [](const MessageView & message, uint32_t tail) {
      if (message.read_only) {
        throw py::type_error("Subscription views are read-only");
      }
      message.value.get().tail() = tail;
    });
  cls.attr("__region_type_bridge__") =
    py::capsule(&bridge, rosidl_runtime_cpython::kMessageTypeBridgeCapsule);
  cls.attr("Constraints") = module.attr("Constraints");
}
