@# generated from rosidl_typesupport_xcdr_cpython/resource/msg__type_support.cpp.em
@# with input from @(package_name):@(interface_path)
@# generated code does not contain a copyright notice

@{
from rosidl_pycommon import convert_camel_case_to_lower_case_underscore
from rosidl_parser.definition import (
    AbstractNestedType,
    AbstractSequence,
    AbstractString,
    AbstractWString,
    Array,
    BasicType,
    BoundedSequence,
    BoundedString,
    BoundedWString,
    EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME,
    NamespacedType,
    SERVICE_REQUEST_MESSAGE_SUFFIX,
    SERVICE_RESPONSE_MESSAGE_SUFFIX,
    SERVICE_EVENT_MESSAGE_SUFFIX,
)
from rosidl_typesupport_xcdr_cpython.template_helpers import (
    get_xcdr_primitive_kind,
    get_cpp_type,
    needs_constraints,
    get_message_type_name,
    get_nested_typesupport_include,
    get_python_module_path,
    get_python_class_name,
    get_python_string_class,
)

# Get optional force_experimental flag (set by experimental wrapper templates).
try:
    force_experimental
except NameError:
    force_experimental = False

# Check if this is an experimental message
is_experimental = force_experimental or 'experimental' in '/'.join(message.structure.namespaced_type.namespaces)

# Determine namespace (includes 'experimental' when forced)
msg_typename = message.structure.namespaced_type.name
msg_namespace_parts = list(message.structure.namespaced_type.namespaces)
if force_experimental and 'experimental' not in msg_namespace_parts:
    msg_namespace_parts.append('experimental')
msg_namespace = '::'.join(msg_namespace_parts)
full_msg_typename = '::'.join(msg_namespace_parts + [msg_typename])
# Effective parent parts for C symbol names and include paths
effective_parent_parts = list(interface_path.parents[0].parts)
if force_experimental:
    effective_parent_parts = [effective_parent_parts[0] + '_experimental']
# Effective include namespace for #include directives
effective_include_ns = '/'.join(msg_namespace_parts)

# Python module path + class name of the message.  This typesupport only
# understands the experimental Python representation (rosidl_runtime_cpython
# containers), which rosidl_generator_py generates for every message under
# the experimental namespace — so both variants resolve there.
py_module = get_python_module_path(message.structure.namespaced_type, experimental_context=True)
py_class = get_python_class_name(message.structure.namespaced_type)

# Check if this is a service message
is_service_message = ('srv' in message.structure.namespaced_type.namespaces and
                     (msg_typename.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
                      msg_typename.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX) or
                      msg_typename.endswith(SERVICE_EVENT_MESSAGE_SUFFIX)))

service_name = None
message_suffix = None
if is_service_message:
    if msg_typename.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX):
        service_name = msg_typename[:-len(SERVICE_REQUEST_MESSAGE_SUFFIX)]
        message_suffix = SERVICE_REQUEST_MESSAGE_SUFFIX
    elif msg_typename.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX):
        service_name = msg_typename[:-len(SERVICE_RESPONSE_MESSAGE_SUFFIX)]
        message_suffix = SERVICE_RESPONSE_MESSAGE_SUFFIX
    elif msg_typename.endswith(SERVICE_EVENT_MESSAGE_SUFFIX):
        service_name = msg_typename[:-len(SERVICE_EVENT_MESSAGE_SUFFIX)]
        message_suffix = SERVICE_EVENT_MESSAGE_SUFFIX

# Calculate has_constraints
has_constraints = False
for member in message.structure.members:
    if needs_constraints(member.type):
        has_constraints = True
        break

# Collect nested message includes (but skip service messages)
nested_includes = set()
nested_functions = set()
for member in message.structure.members:
    member_type = member.type
    if isinstance(member.type, NamespacedType):
        is_nested_service_msg = ('srv' in member_type.namespaces and
                                (member_type.name.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
                                 member_type.name.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX) or
                                 member_type.name.endswith(SERVICE_EVENT_MESSAGE_SUFFIX)))
        if not is_nested_service_msg:
            nested_includes.add(get_nested_typesupport_include(
                member_type, experimental_context=is_experimental))
            _nf = get_message_type_name(member_type, experimental_context=is_experimental)
            _nf_parts = _nf.split('::')
            nested_functions.add(('::'.join(_nf_parts[:-1]), _nf_parts[-1]))
    elif isinstance(member.type, (Array, AbstractSequence)):
        elem_type = member_type.value_type
        if isinstance(elem_type, NamespacedType):
            is_nested_service_msg = ('srv' in elem_type.namespaces and
                                    (elem_type.name.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
                                     elem_type.name.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX) or
                                     elem_type.name.endswith(SERVICE_EVENT_MESSAGE_SUFFIX)))
            if not is_nested_service_msg:
                nested_includes.add(get_nested_typesupport_include(
                    elem_type, experimental_context=is_experimental))
                _nf = get_message_type_name(elem_type, experimental_context=is_experimental)
                _nf_parts = _nf.split('::')
                nested_functions.add(('::'.join(_nf_parts[:-1]), _nf_parts[-1]))
}@
// Copyright 2026 Ekumen Inc.
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

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <memory_resource>
#include <new>
#include <string>
#include <vector>

#include "rcutils/error_handling.h"
#include "rcutils/types/rcutils_ret.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_cpp/experimental/memory.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "rosidl_typesupport_xcdr_c/identifier.h"
#include "rosidl_typesupport_xcdr_c/message_type_support.h"
#include "rosidl_typesupport_xcdr_cpython/identifier.hpp"
#include "rosidl_typesupport_xcdr_cpython/message_type_support.hpp"
#include "rosidl_typesupport_xcdr_cpython/message_type_support_decl.hpp"
#include "rosidl_typesupport_xcdr_cpython/python_helpers.hpp"
#include "@(package_name)/msg/rosidl_typesupport_xcdr_cpython__visibility_control.h"

#include "xcdr_buffers/layout/layout.hpp"
#include "xcdr_buffers/layout/layout_builder.hpp"
#include "xcdr_buffers/layout/layout_parser.hpp"
#include "xcdr_buffers/accessor/accessor.hpp"
#include "xcdr_buffers/accessor/const_accessor.hpp"
#include "xcdr_buffers/serialization/reader.hpp"
#include "xcdr_buffers/serialization/writer.hpp"
#include "xcdr_buffers/common/types.hpp"
#include "xcdr_buffers/common/endianness.hpp"

@[if is_service_message and service_name]@
@[  if is_experimental]@
#include "@(package_name)/srv/experimental/detail/@(convert_camel_case_to_lower_case_underscore(service_name))__struct.hpp"
@[  else]@
#include "@(package_name)/srv/detail/@(convert_camel_case_to_lower_case_underscore(service_name))__struct.hpp"
@[  end if]@
@[else]@
#include "@(effective_include_ns)/@(convert_camel_case_to_lower_case_underscore(msg_typename)).hpp"
@[end if]@

@[for include in sorted(nested_includes)]@
#include "@(include)"
@[end for]@

@[for ns, name in sorted(nested_functions)]@
// Forward declarations: defined in the nested type's own TU.
@[  for part in ns.split('::')]@
namespace @(part) {
@[  end for]@
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC_@(package_name.upper())
py::object get_message_class_@(name)();
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC_@(package_name.upper())
rcutils_ret_t populate_external_storage_@(name)(
  const xcdr_buffers::XCdrConstAccessor & accessor, void * ext_storage_ptr);
@[  if is_experimental]@
extern "C" bool compare_type_specific_constraints_@(name)(
  const void * lhs, const void * rhs);
@[  end if]@
@[  for part in reversed(ns.split('::'))]@
}  // namespace @(part)
@[  end for]@
@[end for]@

namespace @(msg_namespace)
{

// ============================================================================
// @(full_msg_typename) — XCDR CPython typesupport
//
// The message payload is a PyObject * (experimental Python message class
// @(py_module).@(py_class)).  Every function below runs with the GIL held
// (acquired by the outer table callbacks in the runtime library).
// ============================================================================

// Resolve the Python message class from the module (fast: cached in
// sys.modules after the first import; returns a borrowed reference, so no
// INCREF / static-destruction risk).
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC_@(package_name.upper())
py::object
get_message_class_@(msg_typename)()
{
  // Import and attribute lookup raise on failure; the enclosing extern "C"
  // callback translates the exception at the boundary.
  return py::module_::import("@(py_module)").attr("@(py_class)");
}

// ============================================================================
// EmPy macros for per-field code generation
// ============================================================================

@[def generate_writer_field(member, is_experimental, msg_prefix='msg', msg_typename='msg')]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpython.template_helpers import get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _v = _f.attr("value");
    writer.write(_v.cast<@(get_cpp_type(member.type))>());
  }
@[elif isinstance(member.type, AbstractWString)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    py::array _arr = py::reinterpret_borrow<py::array>(_f.attr("numpy")());
    auto _info = _arr.request();
    writer.write(std::u16string_view(
      static_cast<const char16_t *>(_info.ptr), _info.size));
  }
@[elif isinstance(member.type, AbstractString)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    py::array _arr = py::reinterpret_borrow<py::array>(_f.attr("numpy")());
    auto _info = _arr.request();
    writer.write(std::string_view(
      static_cast<const char *>(_info.ptr), _info.size));
  }
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    py::array _arr = py::reinterpret_borrow<py::array>(_f.attr("numpy")());
    auto _info = _arr.request();
    writer.write_array(tcb::span<const @(get_cpp_type(member.type.value_type))>(
      static_cast<const @(get_cpp_type(member.type.value_type)) *>(_info.ptr), @(member.type.size)));
  }
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    for (size_t _i = 0; _i < @(member.type.size); ++_i) {
      auto _e = _f[py::int_(_i)];
      py::array _arr = py::reinterpret_borrow<py::array>(_e.attr("numpy")());
        auto _info = _arr.request();
@[    if isinstance(member.type.value_type, AbstractWString)]@
      writer.write(std::u16string_view(
        static_cast<const char16_t *>(_info.ptr), _info.size));
@[    else]@
      writer.write(std::string_view(
        static_cast<const char *>(_info.ptr), _info.size));
@[    end if]@
    }
  }
@[  else]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    for (size_t _i = 0; _i < @(member.type.size); ++_i) {
      auto _e = _f[py::int_(_i)];
      auto _ret = _inner->serialize_fields(_e.ptr(), writer);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
  }
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    py::array _arr = py::reinterpret_borrow<py::array>(_f.attr("numpy")());
    auto _info = _arr.request();
    writer.write_sequence(tcb::span<const @(get_cpp_type(member.type.value_type))>(
      static_cast<const @(get_cpp_type(member.type.value_type)) *>(_info.ptr),
      _info.size));
  }
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _len = static_cast<size_t>(py::len(_f));
    writer.begin_write_sequence(_len);
    for (size_t _i = 0; _i < _len; ++_i) {
      auto _e = _f[py::int_(_i)];
      py::array _arr = py::reinterpret_borrow<py::array>(_e.attr("numpy")());
        auto _info = _arr.request();
@[    if isinstance(member.type.value_type, AbstractWString)]@
      writer.write(std::u16string_view(
        static_cast<const char16_t *>(_info.ptr), _info.size));
@[    else]@
      writer.write(std::string_view(
        static_cast<const char *>(_info.ptr), _info.size));
@[    end if]@
    }
    writer.end_write_sequence();
  }
@[  else]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _len = static_cast<size_t>(py::len(_f));
    writer.begin_write_sequence(_len);
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    for (size_t _i = 0; _i < _len; ++_i) {
      auto _e = _f[py::int_(_i)];
      auto _ret = _inner->serialize_fields(_e.ptr(), writer);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
    writer.end_write_sequence();
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    auto _ret = _inner->serialize_fields(_f.ptr(), writer);
    if (_ret != RCUTILS_RET_OK) { return _ret; }
  }
@[else]@
  // TODO: Write @(member.name) of type @(member.type)
@[end if]@
@[end def]@

@[def generate_reader_field(member, is_experimental, msg_prefix='msg', msg_typename='msg')]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpython.template_helpers import get_cpp_type, get_message_type_name, get_python_string_class }@ @
@[if isinstance(member.type, BasicType)]@
  {
    auto _r = reader.read<@(get_cpp_type(member.type))>();
    if (!_r) { return RCUTILS_RET_ERROR; }
    auto _f = @(msg_prefix).attr("@(member.name)");
    _f.attr("value") = py::cast(*_r);
  }
@[elif isinstance(member.type, AbstractWString)]@
  {
    auto _r = reader.read<std::u16string_view>();
    if (!_r) { return RCUTILS_RET_ERROR; }
    auto _f = @(msg_prefix).attr("@(member.name)");
    rosidl_typesupport_xcdr_cpython::string_assign_bytes(
      _f, _r->data(), _r->size() * sizeof(char16_t));
  }
@[elif isinstance(member.type, AbstractString)]@
  {
    auto _r = reader.read<std::string_view>();
    if (!_r) { return RCUTILS_RET_ERROR; }
    auto _f = @(msg_prefix).attr("@(member.name)");
    rosidl_typesupport_xcdr_cpython::string_assign_bytes(
      _f, _r->data(), _r->size());
  }
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto _r = reader.read<tcb::span<const @(get_cpp_type(member.type.value_type)), @(member.type.size)>>();
    if (!_r) { return RCUTILS_RET_ERROR; }
    auto _f = @(msg_prefix).attr("@(member.name)");
    py::array _arr = py::reinterpret_borrow<py::array>(_f.attr("numpy")());
    auto _info = _arr.request();
    std::memcpy(_info.ptr, _r->data(),
      @(member.type.size) * sizeof(@(get_cpp_type(member.type.value_type))));
  }
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    for (size_t _i = 0; _i < @(member.type.size); ++_i) {
      auto _e = _f[py::int_(_i)];
@[    if isinstance(member.type.value_type, AbstractWString)]@
      auto _r = reader.read<std::u16string_view>();
      if (!_r) { return RCUTILS_RET_ERROR; }
      rosidl_typesupport_xcdr_cpython::string_assign_bytes(
      _e, _r->data(), _r->size() * sizeof(char16_t));
@[    else]@
      auto _r = reader.read<std::string_view>();
      if (!_r) { return RCUTILS_RET_ERROR; }
      rosidl_typesupport_xcdr_cpython::string_assign_bytes(
      _e, _r->data(), _r->size());
@[    end if]@
    }
  }
@[  else]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    for (size_t _i = 0; _i < @(member.type.size); ++_i) {
      auto _e = _f[py::int_(_i)];
      auto _ret = _inner->deserialize_fields(reader, _e.ptr());
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
  }
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto _r = reader.read<tcb::span<const @(get_cpp_type(member.type.value_type))>>();
    if (!_r) { return RCUTILS_RET_ERROR; }
    auto _f = @(msg_prefix).attr("@(member.name)");
    _f.attr("resize")(_r->size());
    py::array _arr = py::reinterpret_borrow<py::array>(_f.attr("numpy")());
    auto _info = _arr.request();
    std::memcpy(_info.ptr, _r->data(),
      _r->size() * sizeof(@(get_cpp_type(member.type.value_type))));
  }
@[  else]@
  {
    auto _size = reader.begin_read_sequence();
    if (!_size) { return RCUTILS_RET_ERROR; }
    auto _f = @(msg_prefix).attr("@(member.name)");
    _f.attr("clear")();
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
@[      if isinstance(member.type.value_type, (BoundedString, BoundedWString))]@
    py::object _cls = py::module_::import("rosidl_runtime_cpython.string").attr("@(get_python_string_class(member.type.value_type))");
    for (size_t _i = 0; _i < *_size; ++_i) {
      py::object _elem = _cls(@(member.type.value_type.maximum_size));
@[      else]@
    py::object _cls = py::module_::import("rosidl_runtime_cpython.string").attr("@(get_python_string_class(member.type.value_type))");
    for (size_t _i = 0; _i < *_size; ++_i) {
      py::object _elem = _cls();
@[      end if]@
@[      if isinstance(member.type.value_type, AbstractWString)]@
      auto _r = reader.read<std::u16string_view>();
      if (!_r) { return RCUTILS_RET_ERROR; }
      rosidl_typesupport_xcdr_cpython::string_assign_bytes(
      _elem, _r->data(), _r->size() * sizeof(char16_t));
@[      else]@
      auto _r = reader.read<std::string_view>();
      if (!_r) { return RCUTILS_RET_ERROR; }
      rosidl_typesupport_xcdr_cpython::string_assign_bytes(
      _elem, _r->data(), _r->size());
@[      end if]@
      _f.attr("append")(_elem);
    }
@[    else]@
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    py::object _skip = rosidl_typesupport_xcdr_cpython::message_initialization_skip();
    py::object _cls = _f.attr("dtype");
    for (size_t _i = 0; _i < *_size; ++_i) {
      py::object _elem = _cls(py::arg("_init") = _skip);
      auto _ret = _inner->deserialize_fields(reader, _elem.ptr());
      if (_ret != RCUTILS_RET_OK) { return _ret; }
      _f.attr("append")(_elem);
    }
@[    end if]@
    if (!reader.end_read_sequence()) { return RCUTILS_RET_ERROR; }
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    auto _ret = _inner->deserialize_fields(reader, _f.ptr());
    if (_ret != RCUTILS_RET_OK) { return _ret; }
  }
@[else]@
  // TODO: Read @(member.name) of type @(member.type)
@[end if]@
@[end def]@

@[def generate_size_calculation(member, is_experimental, msg_prefix='msg', msg_typename='msg')]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpython.template_helpers import get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  data_offset = xcdr_buffers::align_to(data_offset, sizeof(@(get_cpp_type(member.type))));
  data_offset += sizeof(@(get_cpp_type(member.type)));
@[elif isinstance(member.type, AbstractWString)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    py::array _arr = py::reinterpret_borrow<py::array>(_f.attr("numpy")());
    auto _info = _arr.request();
    data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kStringLengthPrefixSize);
    data_offset += xcdr_buffers::kStringLengthPrefixSize;
    data_offset += _info.size * sizeof(char16_t);
    data_offset += xcdr_buffers::kWStringNullTerminatorSize;
  }
@[elif isinstance(member.type, AbstractString)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    py::array _arr = py::reinterpret_borrow<py::array>(_f.attr("numpy")());
    auto _info = _arr.request();
    data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kStringLengthPrefixSize);
    data_offset += xcdr_buffers::kStringLengthPrefixSize;
    data_offset += _info.size;
    data_offset += xcdr_buffers::kStringNullTerminatorSize;
  }
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  data_offset = xcdr_buffers::align_to(data_offset, sizeof(@(get_cpp_type(member.type.value_type))));
  data_offset += sizeof(@(get_cpp_type(member.type.value_type))) * @(member.type.size);
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    for (size_t _i = 0; _i < @(member.type.size); ++_i) {
      auto _e = _f[py::int_(_i)];
      py::array _arr = py::reinterpret_borrow<py::array>(_e.attr("numpy")());
        auto _info = _arr.request();
      data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kStringLengthPrefixSize);
      data_offset += xcdr_buffers::kStringLengthPrefixSize;
@[    if isinstance(member.type.value_type, AbstractWString)]@
      data_offset += _info.size * sizeof(char16_t);
      data_offset += xcdr_buffers::kWStringNullTerminatorSize;
@[    else]@
      data_offset += _info.size;
      data_offset += xcdr_buffers::kStringNullTerminatorSize;
@[    end if]@
    }
  }
@[  else]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    for (size_t _i = 0; _i < @(member.type.size); ++_i) {
      auto _e = _f[py::int_(_i)];
      size_t _nested_size = 0;
      auto _ret = _inner->compute_serialized_size(_e.ptr(), &_nested_size);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
      data_offset += _nested_size - xcdr_buffers::kXCdrHeaderSize;
    }
  }
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _len = static_cast<size_t>(py::len(_f));
    data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kSequenceLengthPrefixSize);
    data_offset += xcdr_buffers::kSequenceLengthPrefixSize;
    data_offset = xcdr_buffers::align_to(data_offset, sizeof(@(get_cpp_type(member.type.value_type))));
    data_offset += sizeof(@(get_cpp_type(member.type.value_type))) * _len;
  }
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _len = static_cast<size_t>(py::len(_f));
    data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kSequenceLengthPrefixSize);
    data_offset += xcdr_buffers::kSequenceLengthPrefixSize;
    for (size_t _i = 0; _i < _len; ++_i) {
      auto _e = _f[py::int_(_i)];
      py::array _arr = py::reinterpret_borrow<py::array>(_e.attr("numpy")());
        auto _info = _arr.request();
      data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kStringLengthPrefixSize);
      data_offset += xcdr_buffers::kStringLengthPrefixSize;
@[    if isinstance(member.type.value_type, AbstractWString)]@
      data_offset += _info.size * sizeof(char16_t);
      data_offset += xcdr_buffers::kWStringNullTerminatorSize;
@[    else]@
      data_offset += _info.size;
      data_offset += xcdr_buffers::kStringNullTerminatorSize;
@[    end if]@
    }
  }
@[  else]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _len = static_cast<size_t>(py::len(_f));
    data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kSequenceLengthPrefixSize);
    data_offset += xcdr_buffers::kSequenceLengthPrefixSize;
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    for (size_t _i = 0; _i < _len; ++_i) {
      auto _e = _f[py::int_(_i)];
      size_t _nested_size = 0;
      auto _ret = _inner->compute_serialized_size(_e.ptr(), &_nested_size);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
      data_offset += _nested_size - xcdr_buffers::kXCdrHeaderSize;
    }
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  {
    auto _f = @(msg_prefix).attr("@(member.name)");
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    size_t _nested_size = 0;
    auto _ret = _inner->compute_serialized_size(_f.ptr(), &_nested_size);
    if (_ret != RCUTILS_RET_OK) { return _ret; }
    data_offset += _nested_size - xcdr_buffers::kXCdrHeaderSize;
  }
@[else]@
  // TODO: Size for @(member.name)
@[end if]@
@[end def]@

@[def generate_layout_field(member, msg_typename, is_experimental)]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpython.template_helpers import get_xcdr_primitive_kind, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  builder.allocate_primitive("@(member.name)", @(get_xcdr_primitive_kind(member.type)));
@[elif isinstance(member.type, (BoundedString, BoundedWString))]@
@[  if isinstance(member.type, BoundedWString)]@
  builder.allocate_string("@(member.name)", @(member.type.maximum_size), xcdr_buffers::XCdrCharKind::kChar16);
@[  else]@
  builder.allocate_string("@(member.name)", @(member.type.maximum_size));
@[  end if]@
@[elif isinstance(member.type, (AbstractString, AbstractWString))]@
  {
    if (nullptr == constraints_ptr) {
      RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): unbounded string requires constraints");
      return RCUTILS_RET_ERROR;
    }
    auto _cs = rosidl_typesupport_xcdr_cpython::py_borrow(constraints_ptr);
@[  if isinstance(member.type, AbstractWString)]@
    builder.allocate_string("@(member.name)", _cs.attr("@(member.name)").attr("size").cast<size_t>(), xcdr_buffers::XCdrCharKind::kChar16);
@[  else]@
    builder.allocate_string("@(member.name)", _cs.attr("@(member.name)").attr("size").cast<size_t>());
@[  end if]@
  }
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  builder.allocate_primitive_array("@(member.name)", @(get_xcdr_primitive_kind(member.type.value_type)), @(member.type.size));
@[  else]@
  builder.begin_allocate_array("@(member.name)", @(member.type.size));
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
@[      if isinstance(member.type.value_type, BoundedWString)]@
    builder.allocate_string(@(member.type.value_type.maximum_size), xcdr_buffers::XCdrCharKind::kChar16);
@[      elif isinstance(member.type.value_type, BoundedString)]@
    builder.allocate_string(@(member.type.value_type.maximum_size));
@[      elif isinstance(member.type.value_type, AbstractWString)]@
    {
      if (nullptr == constraints_ptr) {
        RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): unbounded string element requires constraints");
        return RCUTILS_RET_ERROR;
      }
      auto _cs = rosidl_typesupport_xcdr_cpython::py_borrow(constraints_ptr);
      builder.allocate_string(_cs.attr("@(member.name)").attr("element").attr("size").cast<size_t>(), xcdr_buffers::XCdrCharKind::kChar16);
    }
@[      else]@
    {
      if (nullptr == constraints_ptr) {
        RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): unbounded string element requires constraints");
        return RCUTILS_RET_ERROR;
      }
      auto _cs = rosidl_typesupport_xcdr_cpython::py_borrow(constraints_ptr);
      builder.allocate_string(_cs.attr("@(member.name)").attr("element").attr("size").cast<size_t>());
    }
@[      end if]@
@[    elif isinstance(member.type.value_type, NamespacedType)]@
    builder.begin_allocate_struct();
    {
      auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
      auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
      auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
      if (nullptr == _inner->build_layout_fields) {
        RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): nested build_layout_fields not available");
        return RCUTILS_RET_ERROR;
      }
      auto _ret = _inner->build_layout_fields(builder, nullptr);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
    builder.end_allocate_struct();
@[    end if]@
  builder.end_allocate_array();
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
@[    if isinstance(member.type, BoundedSequence)]@
  builder.allocate_primitive_sequence("@(member.name)", @(get_xcdr_primitive_kind(member.type.value_type)), @(member.type.maximum_size));
@[    else]@
  {
    if (nullptr == constraints_ptr) {
      RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): unbounded sequence requires constraints");
      return RCUTILS_RET_ERROR;
    }
    auto _cs = rosidl_typesupport_xcdr_cpython::py_borrow(constraints_ptr);
    builder.allocate_primitive_sequence("@(member.name)", @(get_xcdr_primitive_kind(member.type.value_type)), _cs.attr("@(member.name)").attr("size").cast<size_t>());
  }
@[    end if]@
@[  else]@
@[    if isinstance(member.type, BoundedSequence)]@
  builder.begin_allocate_sequence("@(member.name)", @(member.type.maximum_size));
@[    else]@
  {
    if (nullptr == constraints_ptr) {
      RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): unbounded sequence requires constraints");
      return RCUTILS_RET_ERROR;
    }
    auto _cs = rosidl_typesupport_xcdr_cpython::py_borrow(constraints_ptr);
    builder.begin_allocate_sequence("@(member.name)", _cs.attr("@(member.name)").attr("size").cast<size_t>());
  }
@[    end if]@
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
@[      if isinstance(member.type.value_type, BoundedWString)]@
    builder.allocate_string(@(member.type.value_type.maximum_size), xcdr_buffers::XCdrCharKind::kChar16);
@[      elif isinstance(member.type.value_type, BoundedString)]@
    builder.allocate_string(@(member.type.value_type.maximum_size));
@[      elif isinstance(member.type.value_type, AbstractWString)]@
    {
      if (nullptr == constraints_ptr) {
        RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): unbounded string element requires constraints");
        return RCUTILS_RET_ERROR;
      }
      auto _cs = rosidl_typesupport_xcdr_cpython::py_borrow(constraints_ptr);
      builder.allocate_string(_cs.attr("@(member.name)").attr("element").attr("size").cast<size_t>(), xcdr_buffers::XCdrCharKind::kChar16);
    }
@[      else]@
    {
      if (nullptr == constraints_ptr) {
        RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): unbounded string element requires constraints");
        return RCUTILS_RET_ERROR;
      }
      auto _cs = rosidl_typesupport_xcdr_cpython::py_borrow(constraints_ptr);
      builder.allocate_string(_cs.attr("@(member.name)").attr("element").attr("size").cast<size_t>());
    }
@[      end if]@
@[    elif isinstance(member.type.value_type, NamespacedType)]@
    builder.begin_allocate_struct();
    {
      auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
      auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
      auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
      if (nullptr == _inner->build_layout_fields) {
        RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): nested build_layout_fields not available");
        return RCUTILS_RET_ERROR;
      }
      auto _ret = _inner->build_layout_fields(builder, nullptr);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
    builder.end_allocate_struct();
@[    end if]@
  builder.end_allocate_sequence();
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  builder.begin_allocate_struct("@(member.name)");
  {
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    if (nullptr == _inner->build_layout_fields) {
      RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): nested build_layout_fields not available");
      return RCUTILS_RET_ERROR;
    }
    if (constraints_ptr) {
      auto _cs = rosidl_typesupport_xcdr_cpython::py_borrow(constraints_ptr);
      auto _ret = _inner->build_layout_fields(builder, _cs.attr("@(member.name)").ptr());
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    } else {
      auto _ret = _inner->build_layout_fields(builder, nullptr);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
  }
  builder.end_allocate_struct();
@[else]@
  // TODO: Layout @(member.name)
@[end if]@
@[end def]@

@[def generate_parser_field(member, msg_typename, is_experimental)]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpython.template_helpers import get_xcdr_primitive_kind, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  parser.parse_primitive(@(get_xcdr_primitive_kind(member.type)));
@[elif isinstance(member.type, AbstractWString)]@
  parser.parse_string(xcdr_buffers::XCdrCharKind::kChar16);
@[elif isinstance(member.type, AbstractString)]@
  parser.parse_string();
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  parser.parse_primitive_array(@(get_xcdr_primitive_kind(member.type.value_type)), @(member.type.size));
@[  else]@
  parser.begin_parse_array(@(member.type.size));
@[    if isinstance(member.type.value_type, AbstractWString)]@
    parser.parse_string(xcdr_buffers::XCdrCharKind::kChar16);
@[    elif isinstance(member.type.value_type, AbstractString)]@
    parser.parse_string();
@[    elif isinstance(member.type.value_type, NamespacedType)]@
    parser.begin_parse_struct();
    {
      auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
      auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
      auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
      if (nullptr == _inner->parse_fields) {
        RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): nested parse_fields not available");
        return RCUTILS_RET_ERROR;
      }
      auto _ret = _inner->parse_fields(parser);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
    parser.end_parse_struct();
@[    end if]@
  parser.end_parse_array();
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
@[    if isinstance(member.type, BoundedSequence)]@
  parser.parse_primitive_sequence(@(get_xcdr_primitive_kind(member.type.value_type)), @(member.type.maximum_size));
@[    else]@
  parser.parse_primitive_sequence(@(get_xcdr_primitive_kind(member.type.value_type)));
@[    end if]@
@[  else]@
  {
    auto _count_result = parser.begin_parse_sequence();
    if (!_count_result) { return RCUTILS_RET_ERROR; }
    for (size_t _i = 0; _i < *_count_result; ++_i) {
@[    if isinstance(member.type.value_type, AbstractWString)]@
      parser.parse_string(xcdr_buffers::XCdrCharKind::kChar16);
@[    elif isinstance(member.type.value_type, AbstractString)]@
      parser.parse_string();
@[    elif isinstance(member.type.value_type, NamespacedType)]@
      parser.begin_parse_struct();
      {
        auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
        auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
        auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
        if (nullptr == _inner->parse_fields) {
          RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): nested parse_fields not available");
          return RCUTILS_RET_ERROR;
        }
        auto _ret = _inner->parse_fields(parser);
        if (_ret != RCUTILS_RET_OK) { return _ret; }
      }
      parser.end_parse_struct();
@[    end if]@
    }
    parser.end_parse_sequence();
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  parser.begin_parse_struct("@(member.name)");
  {
    auto _ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto _outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_ts->data);
    auto _inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_outer->inner);
    if (nullptr == _inner->parse_fields) {
      RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): nested parse_fields not available");
      return RCUTILS_RET_ERROR;
    }
    auto _ret = _inner->parse_fields(parser);
    if (_ret != RCUTILS_RET_OK) { return _ret; }
  }
  parser.end_parse_struct();
@[else]@
  // TODO: Parse @(member.name)
@[end if]@
@[end def]@

@[def generate_external_storage_field(member, index, msg_typename, is_experimental)]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpython.template_helpers import get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  {
    auto _slice = accessor[@(index)].slice();
    py::object _buf = rosidl_typesupport_xcdr_cpython::raw_buffer_from_region(
      rosidl_memory_region_t{{const_cast<void *>(static_cast<const void *>(_slice.data())), 0}, _slice.size()});
    ext_storage.attr("members").attr("@(member.name)") = _buf;
  }
@[elif isinstance(member.type, AbstractWString)]@
  {
    auto _slice = accessor[@(index)].slice();
    auto _data = _slice.subspan(xcdr_buffers::kStringLengthPrefixSize);
    // Exclude the wire null terminator so the container's logical size is
    // exactly the content length (C++ derives this via strnlen).
    py::object _buf = rosidl_typesupport_xcdr_cpython::raw_buffer_from_region(
      rosidl_memory_region_t{
        {const_cast<void *>(static_cast<const void *>(_data.data())), 0},
        _data.size() - xcdr_buffers::kWStringNullTerminatorSize});
    ext_storage.attr("members").attr("@(member.name)") = _buf;
  }
@[elif isinstance(member.type, AbstractString)]@
  {
    auto _slice = accessor[@(index)].slice();
    auto _data = _slice.subspan(xcdr_buffers::kStringLengthPrefixSize);
    // Exclude the wire null terminator (see above).
    py::object _buf = rosidl_typesupport_xcdr_cpython::raw_buffer_from_region(
      rosidl_memory_region_t{
        {const_cast<void *>(static_cast<const void *>(_data.data())), 0},
        _data.size() - xcdr_buffers::kStringNullTerminatorSize});
    ext_storage.attr("members").attr("@(member.name)") = _buf;
  }
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto _slice = accessor[@(index)].slice();
    py::object _buf = rosidl_typesupport_xcdr_cpython::raw_buffer_from_region(
      rosidl_memory_region_t{{const_cast<void *>(static_cast<const void *>(_slice.data())), 0}, _slice.size()});
    ext_storage.attr("members").attr("@(member.name)") = _buf;
  }
@[  else]@
  {
    auto _arr = accessor[@(index)];
    py::list _bufs;
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
    for (size_t _j = 0; _j < @(member.type.size); ++_j) {
      auto _elem_slice = _arr[_j].slice();
      auto _elem_data = _elem_slice.subspan(xcdr_buffers::kStringLengthPrefixSize);
      py::object _buf = rosidl_typesupport_xcdr_cpython::raw_buffer_from_region(
        rosidl_memory_region_t{
          {const_cast<void *>(static_cast<const void *>(_elem_data.data())), 0},
          _elem_data.size() - xcdr_buffers::kStringNullTerminatorSize});
        _bufs.append(_buf);
    }
@[    elif isinstance(member.type.value_type, NamespacedType)]@
@{
_ns_full = get_message_type_name(member.type.value_type, experimental_context=is_experimental)
_ns_parts = _ns_full.split('::')
_ns_ns = '::'.join(_ns_parts[:-1])
_ns_name = _ns_parts[-1]
}@
    py::object _nested_cls = @(_ns_ns)::get_message_class_@(_ns_name)();
    for (size_t _j = 0; _j < @(member.type.size); ++_j) {
      py::object _sub = _nested_cls.attr("ExternalStorage")();
      _sub.attr("prepopulated") = py::bool_(_prepopulated);
      auto _ret = @(_ns_ns)::populate_external_storage_@(_ns_name)(_arr[_j], _sub.ptr());
      if (_ret != RCUTILS_RET_OK) { return _ret; }
      _bufs.append(_sub);
    }
@[    end if]@
    ext_storage.attr("members").attr("@(member.name)") = _bufs;
  }
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto _seq = accessor[@(index)];
    auto _count_result = _seq.size();
    if (!_count_result) { return RCUTILS_RET_ERROR; }
    auto _slice = accessor[@(index)].slice();
    auto _data = _slice.subspan(
      xcdr_buffers::kSequenceLengthPrefixSize, *_count_result * sizeof(@(get_cpp_type(member.type.value_type))));
    py::object _buf = rosidl_typesupport_xcdr_cpython::raw_buffer_from_region(
      rosidl_memory_region_t{{const_cast<void *>(static_cast<const void *>(_data.data())), 0}, _data.size()});
    ext_storage.attr("members").attr("@(member.name)") = _buf;
  }
@[  else]@
  {
    auto _seq = accessor[@(index)];
    auto _size_result = _seq.size();
    if (!_size_result) { return RCUTILS_RET_ERROR; }
    size_t _size = *_size_result;
    py::list _bufs;
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
    for (size_t _j = 0; _j < _size; ++_j) {
      auto _elem_slice = _seq[_j].slice();
      auto _elem_data = _elem_slice.subspan(xcdr_buffers::kStringLengthPrefixSize);
      py::object _buf = rosidl_typesupport_xcdr_cpython::raw_buffer_from_region(
        rosidl_memory_region_t{
          {const_cast<void *>(static_cast<const void *>(_elem_data.data())), 0},
          _elem_data.size() - xcdr_buffers::kStringNullTerminatorSize});
        _bufs.append(_buf);
    }
@[    elif isinstance(member.type.value_type, NamespacedType)]@
@{
_ns_full = get_message_type_name(member.type.value_type, experimental_context=is_experimental)
_ns_parts = _ns_full.split('::')
_ns_ns = '::'.join(_ns_parts[:-1])
_ns_name = _ns_parts[-1]
}@
    py::object _nested_cls = @(_ns_ns)::get_message_class_@(_ns_name)();
    for (size_t _j = 0; _j < _size; ++_j) {
      py::object _sub = _nested_cls.attr("ExternalStorage")();
      _sub.attr("prepopulated") = py::bool_(_prepopulated);
      auto _ret = @(_ns_ns)::populate_external_storage_@(_ns_name)(_seq[_j], _sub.ptr());
      if (_ret != RCUTILS_RET_OK) { return _ret; }
      _bufs.append(_sub);
    }
@[    end if]@
    ext_storage.attr("members").attr("@(member.name)") = _bufs;
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
@{
_ns_full = get_message_type_name(member.type, experimental_context=is_experimental)
_ns_parts = _ns_full.split('::')
_ns_ns = '::'.join(_ns_parts[:-1])
_ns_name = _ns_parts[-1]
}@
  {
    py::object _nested_cls = @(_ns_ns)::get_message_class_@(_ns_name)();
    py::object _sub = _nested_cls.attr("ExternalStorage")();
    _sub.attr("prepopulated") = py::bool_(_prepopulated);
    auto _ret = @(_ns_ns)::populate_external_storage_@(_ns_name)(accessor[@(index)], _sub.ptr());
    if (_ret != RCUTILS_RET_OK) { return _ret; }
    ext_storage.attr("members").attr("@(member.name)") = _sub;
  }
@[else]@
  // TODO: External storage @(member.name)
@[end if]@
@[end def]@

@[def generate_validate_field(member, msg_typename, is_experimental)]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpython.template_helpers import get_message_type_name }@ @
@[if isinstance(member.type, (AbstractString, AbstractWString))]@
@[  if isinstance(member.type, (BoundedString, BoundedWString))]@
  // @(member.name): bounded string, bound is part of the type.
@[  else]@
  {
    auto _val = py::len(msg.attr("@(member.name)"));
    auto _bound = _cs.attr("@(member.name)").attr("size").cast<size_t>();
    if (static_cast<size_t>(_val) > _bound) {
      if (report_cb) { report_cb(user_data, "@(member.name)", 0); }
      return RCUTILS_RET_ERROR;
    }
  }
@[  end if]@
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
@[    if isinstance(member.type.value_type, (BoundedString, BoundedWString))]@
  // @(member.name): bounded string array, bounds in the type.
@[    else]@
  {
    auto _field = msg.attr("@(member.name)");
    auto _elem_bound = _cs.attr("@(member.name)").attr("element").attr("size").cast<size_t>();
    for (size_t _j = 0; _j < @(member.type.size); ++_j) {
      auto _val = py::len(_field[py::int_(_j)]);
      if (static_cast<size_t>(_val) > _elem_bound) {
        if (report_cb) {
          std::string _path = "@(member.name)." + std::to_string(_j);
          report_cb(user_data, _path.c_str(), 0);
        }
        return RCUTILS_RET_ERROR;
      }
    }
  }
@[    end if]@
@[  elif isinstance(member.type.value_type, NamespacedType)]@
  {
    auto _field = msg.attr("@(member.name)");
    auto _nested_ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto _nested_outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts->data);
    auto _nested_inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_nested_outer->inner);
    if (nullptr == _nested_inner->validate_fields) {
      RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): nested validate_fields not available");
      return RCUTILS_RET_ERROR;
    }
    auto _nested_cs = _cs.attr("@(member.name)");
    for (size_t _j = 0; _j < @(member.type.size); ++_j) {
      auto _ret = _nested_inner->validate_fields(_nested_cs.ptr(), _field[py::int_(_j)].ptr(), report_cb, user_data);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
  }
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto _val = py::len(msg.attr("@(member.name)"));
    auto _bound = _cs.attr("@(member.name)").attr("size").cast<size_t>();
    if (static_cast<size_t>(_val) > _bound) {
      if (report_cb) { report_cb(user_data, "@(member.name)", 0); }
      return RCUTILS_RET_ERROR;
    }
  }
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  {
    auto _field = msg.attr("@(member.name)");
    auto _count = py::len(_field);
@[    if isinstance(member.type, BoundedSequence)]@
    auto _bound = static_cast<size_t>(@(member.type.maximum_size));
@[    else]@
    auto _bound = _cs.attr("@(member.name)").attr("size").cast<size_t>();
@[    end if]@
    if (static_cast<size_t>(_count) > _bound) {
      if (report_cb) { report_cb(user_data, "@(member.name)", 0); }
      return RCUTILS_RET_ERROR;
    }
@[    if not isinstance(member.type, BoundedSequence)]@
    auto _elem_bound = _cs.attr("@(member.name)").attr("element").attr("size").cast<size_t>();
    for (py::ssize_t _j = 0; _j < _count; ++_j) {
      auto _val = py::len(_field[py::int_(_j)]);
      if (static_cast<size_t>(_val) > _elem_bound) {
        if (report_cb) {
          std::string _path = "@(member.name)." + std::to_string(_j);
          report_cb(user_data, _path.c_str(), 0);
        }
        return RCUTILS_RET_ERROR;
      }
    }
@[    end if]@
  }
@[  elif isinstance(member.type.value_type, NamespacedType)]@
  {
    auto _field = msg.attr("@(member.name)");
    auto _count = py::len(_field);
    auto _bound = _cs.attr("@(member.name)").attr("size").cast<size_t>();
    if (static_cast<size_t>(_count) > _bound) {
      if (report_cb) { report_cb(user_data, "@(member.name)", 0); }
      return RCUTILS_RET_ERROR;
    }
    auto _nested_ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto _nested_outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts->data);
    auto _nested_inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_nested_outer->inner);
    if (nullptr == _nested_inner->validate_fields) {
      RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): nested validate_fields not available");
      return RCUTILS_RET_ERROR;
    }
    auto _nested_cs = _cs.attr("@(member.name)").attr("element");
    for (py::ssize_t _j = 0; _j < _count; ++_j) {
      auto _ret = _nested_inner->validate_fields(_nested_cs.ptr(), _field[py::int_(_j)].ptr(), report_cb, user_data);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  {
    auto _nested_ts = rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto _nested_outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts->data);
    auto _nested_inner = static_cast<const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t *>(_nested_outer->inner);
    if (nullptr == _nested_inner->validate_fields) {
      RCUTILS_SET_ERROR_MSG("@(msg_typename).@(member.name): nested validate_fields not available");
      return RCUTILS_RET_ERROR;
    }
    auto _ret = _nested_inner->validate_fields(
      _cs.attr("@(member.name)").ptr(), msg.attr("@(member.name)").ptr(), report_cb, user_data);
    if (_ret != RCUTILS_RET_OK) { return _ret; }
  }
@[end if]@
@[end def]@

@[def generate_compare_field(member, msg_typename, is_experimental)]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpython.template_helpers import get_message_type_name }@ @
@[if isinstance(member.type, (AbstractString, AbstractWString))]@
@[  if not isinstance(member.type, (BoundedString, BoundedWString))]@
  if (_cand.attr("@(member.name)").attr("size").cast<size_t>() >
      _base.attr("@(member.name)").attr("size").cast<size_t>()) {
    return false;  // candidate looser than baseline
  }
@[  end if]@
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
@[    if not isinstance(member.type.value_type, (BoundedString, BoundedWString))]@
  if (_cand.attr("@(member.name)").attr("element").attr("size").cast<size_t>() >
      _base.attr("@(member.name)").attr("element").attr("size").cast<size_t>()) {
    return false;
  }
@[    end if]@
@[  elif isinstance(member.type.value_type, NamespacedType)]@
@{
_nc_full = get_message_type_name(member.type.value_type, experimental_context=is_experimental)
_nc_parts = _nc_full.split('::')
_nc_ns = '::'.join(_nc_parts[:-1])
_nc_name = _nc_parts[-1]
}@
  {
    auto _nested_compare = @(_nc_ns)::compare_type_specific_constraints_@(_nc_name);
    if (!_nested_compare(_cand.attr("@(member.name)").ptr(), _base.attr("@(member.name)").ptr())) {
      return false;
    }
  }
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
@[    if not isinstance(member.type, BoundedSequence)]@
  if (_cand.attr("@(member.name)").attr("size").cast<size_t>() >
      _base.attr("@(member.name)").attr("size").cast<size_t>()) {
    return false;
  }
@[    end if]@
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
@[    if not isinstance(member.type, BoundedSequence)]@
  if (_cand.attr("@(member.name)").attr("size").cast<size_t>() >
      _base.attr("@(member.name)").attr("size").cast<size_t>()) {
    return false;
  }
  if (_cand.attr("@(member.name)").attr("element").attr("size").cast<size_t>() >
      _base.attr("@(member.name)").attr("element").attr("size").cast<size_t>()) {
    return false;
  }
@[    end if]@
@[  elif isinstance(member.type.value_type, NamespacedType)]@
@{
_nc_full = get_message_type_name(member.type.value_type, experimental_context=is_experimental)
_nc_parts = _nc_full.split('::')
_nc_ns = '::'.join(_nc_parts[:-1])
_nc_name = _nc_parts[-1]
}@
  if (_cand.attr("@(member.name)").attr("size").cast<size_t>() >
      _base.attr("@(member.name)").attr("size").cast<size_t>()) {
    return false;
  }
  {
    auto _nested_compare = @(_nc_ns)::compare_type_specific_constraints_@(_nc_name);
    if (!_nested_compare(
          _cand.attr("@(member.name)").attr("element").ptr(),
          _base.attr("@(member.name)").attr("element").ptr())) {
      return false;
    }
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
@{
_nc_full = get_message_type_name(member.type, experimental_context=is_experimental)
_nc_parts = _nc_full.split('::')
_nc_ns = '::'.join(_nc_parts[:-1])
_nc_name = _nc_parts[-1]
}@
  {
    auto _nested_compare = @(_nc_ns)::compare_type_specific_constraints_@(_nc_name);
    if (!_nested_compare(_cand.attr("@(member.name)").ptr(), _base.attr("@(member.name)").ptr())) {
      return false;
    }
  }
@[end if]@
@[end def]@

// Private: Serialize message fields into existing writer (no XCDR header).
rcutils_ret_t
serialize_fields_into_writer_@(msg_typename)(
  void * message_ptr,
  xcdr_buffers::XCdrWriter & writer)
{
  if (nullptr == message_ptr) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto msg = rosidl_typesupport_xcdr_cpython::py_borrow(message_ptr);
@[  for member in message.structure.members]@
@[    if len(message.structure.members) == 1 and member.name == EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME]@
@[      continue]@
@[    end if]@
@(generate_writer_field(member, is_experimental, 'msg', msg_typename))
@[  end for]@
    return RCUTILS_RET_OK;
}

// Private: Deserialize message fields from existing reader (no XCDR header).
rcutils_ret_t
deserialize_fields_from_reader_@(msg_typename)(
  xcdr_buffers::XCdrReader & reader,
  void * message_ptr)
{
  if (nullptr == message_ptr) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): message is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto msg = rosidl_typesupport_xcdr_cpython::py_borrow(message_ptr);
@[  for member in message.structure.members]@
@[    if len(message.structure.members) == 1 and member.name == EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME]@
@[      continue]@
@[    end if]@
@(generate_reader_field(member, is_experimental, 'msg', msg_typename))
@[  end for]@
    return RCUTILS_RET_OK;
}

// Compute serialized size.
rcutils_ret_t
compute_serialized_size_@(msg_typename)(
  const void * message_ptr,
  size_t * size)
{
  if (nullptr == message_ptr || nullptr == size) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): message or size is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto msg = rosidl_typesupport_xcdr_cpython::py_borrow(message_ptr);
    // Fast path: external storage already knows the size.
    py::object ext = msg.attr("_external_storage");
    if (!ext.is_none()) {
      py::object block = ext.attr("block");
      if (!block.is_none()) {
        *size = block.attr("size").cast<size_t>();
        return RCUTILS_RET_OK;
      }
    }
    // Compute size with XCDR alignment.
    size_t data_offset = 0;
@[  for member in message.structure.members]@
@[    if len(message.structure.members) == 1 and member.name == EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME]@
@[      continue]@
@[    end if]@
@(generate_size_calculation(member, is_experimental, 'msg', msg_typename))
@[  end for]@
    *size = xcdr_buffers::kXCdrHeaderSize + data_offset;
    return RCUTILS_RET_OK;
}

@[if not has_constraints]@
// Singleton layout for fully bounded messages (no variable-length members).
// Built once and cached; construct_message_at applies it to the storage.
std::shared_ptr<xcdr_buffers::XCdrStructLayout>
get_layout_@(msg_typename)()
{
  static auto layout = std::make_shared<xcdr_buffers::XCdrStructLayout>([]() {
    xcdr_buffers::XCdrLayoutBuilder builder;
@[  for member in message.structure.members]@
@[    if len(message.structure.members) == 1 and member.name == EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME]@
@[      continue]@
@[    end if]@
@(generate_layout_field(member, msg_typename, is_experimental))
@[  end for]@
    return builder.finalize();
  }());
  return layout;
}
@[end if]@

// Private: Build layout fields into existing builder (constraint-aware).
// *constraints_ptr* is a Python `Msg.Constraints` instance, or nullptr for
// fully-bounded members (bounds come from the type).
rcutils_ret_t
build_layout_fields_@(msg_typename)(
  xcdr_buffers::XCdrLayoutBuilder & builder,
  const void * constraints_ptr)
{
@[  for member in message.structure.members]@
@[    if len(message.structure.members) == 1 and member.name == EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME]@
@[      continue]@
@[    end if]@
@(generate_layout_field(member, msg_typename, is_experimental))
@[  end for]@
  return RCUTILS_RET_OK;
}

// Forward declaration (defined below).
rcutils_ret_t
populate_external_storage_@(msg_typename)(
  const xcdr_buffers::XCdrConstAccessor & accessor, void * ext_storage_ptr);

// Construct a Python message at storage (zero-copy sender side).
rcutils_ret_t
construct_message_@(msg_typename)(
  const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t * impl,
  rosidl_runtime_cpp::MemoryRegion<void> & storage,
  void ** message_ptr)
{
  if (nullptr == impl || nullptr == impl->cached_layout) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): layout not available (message requires constraints)");
    return RCUTILS_RET_ERROR;
  }
  auto buffer_span = tcb::span<uint8_t>(
    static_cast<uint8_t *>(storage.data()), impl->cached_layout->total_size());
  if (!impl->cached_layout->apply(buffer_span)) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): layout apply failed");
    return RCUTILS_RET_ERROR;
  }
  auto accessor_result = xcdr_buffers::XCdrConstAccessor::wrap(
    tcb::span<const uint8_t>(buffer_span.data(), buffer_span.size()), *impl->cached_layout);
  if (!accessor_result) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): failed to create const accessor");
    return RCUTILS_RET_ERROR;
  }
  py::object msg_class = get_message_class_@(msg_typename)();
  py::object ext_storage = msg_class.attr("ExternalStorage")();
  rosidl_typesupport_xcdr_cpython::external_storage_set_block(
    ext_storage,
    rosidl_memory_region_t{{storage.data(), 0}, impl->cached_layout->total_size()});
  auto _ret = populate_external_storage_@(msg_typename)(*accessor_result, ext_storage.ptr());
  if (_ret != RCUTILS_RET_OK) { return _ret; }
  py::object msg = rosidl_typesupport_xcdr_cpython::message_from_external_storage(
    msg_class, ext_storage);
  *message_ptr = msg.release().ptr();   // new reference, owned by the caller
  return RCUTILS_RET_OK;
}

// Populate a Python ExternalStorage instance from a const accessor.
// Shared between the construct and cast paths.
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC_@(package_name.upper())
rcutils_ret_t
populate_external_storage_@(msg_typename)(
  const xcdr_buffers::XCdrConstAccessor & accessor, void * ext_storage_ptr)
{
  if (nullptr == ext_storage_ptr) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): ext_storage is nullptr");
    return RCUTILS_RET_ERROR;
  }
  auto ext_storage = rosidl_typesupport_xcdr_cpython::py_borrow(ext_storage_ptr);
    // The construct path leaves this false; the cast path sets it true before
    // calling populate, so nested ExternalStorage objects get the same flag.
    bool _prepopulated = py::cast<bool>(ext_storage.attr("prepopulated"));
@[  for idx, member in enumerate(message.structure.members)]@
@[    if len(message.structure.members) == 1 and member.name == EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME]@
@[      continue]@
@[    end if]@
@(generate_external_storage_field(member, idx, msg_typename, is_experimental))
@[  end for]@
    return RCUTILS_RET_OK;
}

// Parse layout fields from buffer (zero-copy receiver side).
// Recurses into nested struct members via the nested type's own parse_fields,
// so variable-length members anywhere in the message tree get correctly
// inferred offsets from the wire data.
rcutils_ret_t
parse_fields_@(msg_typename)(
  xcdr_buffers::XCdrLayoutParser & parser)
{
@[  for member in message.structure.members]@
@[    if len(message.structure.members) == 1 and member.name == EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME]@
@[      continue]@
@[    end if]@
@(generate_parser_field(member, msg_typename, is_experimental))
@[  end for]@
  return RCUTILS_RET_OK;
}

// Cast a serialized XCDR buffer into a Python message (zero-copy receiver side).
rcutils_ret_t
cast_message_@(msg_typename)(
  const rosidl_typesupport_xcdr_cpython::rosidl_message_xcdr_cpython_type_support_t * impl,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void ** message_ptr)
{
  auto buffer_span = tcb::span<const uint8_t>(
    static_cast<const uint8_t *>(storage.data()), storage.size());
  // Stack-backed pool for the temporary parsed layout (released at exit).
  alignas(std::max_align_t) std::byte _cast_pool_buffer[8192];
  std::pmr::monotonic_buffer_resource _cast_pool(_cast_pool_buffer, sizeof(_cast_pool_buffer));
  xcdr_buffers::XCdrLayoutParser parser(buffer_span, &_cast_pool);
  if (nullptr == impl || nullptr == impl->parse_fields) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): parse_fields callback not available");
    return RCUTILS_RET_ERROR;
  }
  auto _parse_ret = impl->parse_fields(parser);
  if (_parse_ret != RCUTILS_RET_OK) { return _parse_ret; }
  auto layout_result = parser.finalize();
  if (!layout_result) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): failed to parse layout from buffer");
    return RCUTILS_RET_ERROR;
  }
  auto accessor_result = xcdr_buffers::XCdrConstAccessor::wrap(buffer_span, *layout_result);
  if (!accessor_result) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): failed to create accessor from parsed layout");
    return RCUTILS_RET_ERROR;
  }
  py::object msg_class = get_message_class_@(msg_typename)();
  py::object ext_storage = msg_class.attr("ExternalStorage")();
  rosidl_typesupport_xcdr_cpython::external_storage_set_block(
    ext_storage,
    rosidl_memory_region_t{{const_cast<void *>(storage.data()), 0}, storage.size()});
  // Zero-copy cast: containers expose the sizes already present in the wire
  // buffer (the Python analog of C++ prepopulated == true).  Set before
  // populate so nested ExternalStorage objects get the same flag.
  ext_storage.attr("prepopulated") = py::bool_(true);
  auto _ret = populate_external_storage_@(msg_typename)(*accessor_result, ext_storage.ptr());
  if (_ret != RCUTILS_RET_OK) { return _ret; }
  py::object msg = rosidl_typesupport_xcdr_cpython::message_from_external_storage(
    msg_class, ext_storage);
  // Validate against constraints if the handle carries owned constraint state
  // (rmw creates a per-loan constrained handle to enforce upper bounds).
  if (impl && impl->owned_constraints && impl->owned_constraints->type_specific &&
      impl->validate_fields)
  {
    if (RCUTILS_RET_OK != impl->validate_fields(
        impl->owned_constraints->type_specific, msg.ptr(), nullptr, nullptr))
    {
      return RCUTILS_RET_ERROR;
    }
  }
  *message_ptr = msg.release().ptr();   // new reference, owned by the caller
  return RCUTILS_RET_OK;
}

@[if is_experimental]@
// Release a message and return its backing storage (consumes the message).
// Implementation (no exception handling): Python failures raise and are
// translated by the extern "C" wrapper.  Defined only for the experimental
// variant to avoid C-linkage symbol collisions between the standard and
// experimental TUs of the same message.
rosidl_memory_region_t
release_message_@(msg_typename)_impl(void * message_ptr)
{
  rosidl_memory_region_t null_region = {{nullptr, 0}, 0};
  if (nullptr == message_ptr) { return null_region; }
  auto msg = rosidl_typesupport_xcdr_cpython::py_borrow(message_ptr);
  py::object ext = msg.attr("_external_storage");
  rosidl_memory_region_t region = null_region;
  if (!ext.is_none()) {
    py::object block = ext.attr("block");
    if (!block.is_none()) {
      region = rosidl_memory_region_t{
        {reinterpret_cast<void *>(block.attr("address").cast<uintptr_t>()), 0},
        block.attr("size").cast<size_t>()};
    }
  }
  Py_DECREF(static_cast<PyObject *>(message_ptr));
  return region;
}

extern "C" rosidl_memory_region_t
release_message_@(msg_typename)(void * message_ptr)
{
  // The C trampoline does not hold the GIL; the guard also covers the catch
  // block so the caught py::error_already_set restores the pending exception
  // safely.
  py::gil_scoped_acquire acquire;
  try {
    return release_message_@(msg_typename)_impl(message_ptr);
  } catch (const std::exception &) {
    rosidl_typesupport_xcdr_cpython::translate_pybind_error("release @(msg_typename)");
    return rosidl_memory_region_t{{nullptr, 0}, 0};
  }
}

// Return the backing storage of a message without releasing it.
// Implementation (no exception handling): see release_message_@(msg_typename).
rosidl_memory_region_t
get_backing_storage_@(msg_typename)_impl(const void * message_ptr)
{
  rosidl_memory_region_t null_region = {{nullptr, 0}, 0};
  if (nullptr == message_ptr) { return null_region; }
  auto msg = rosidl_typesupport_xcdr_cpython::py_borrow(message_ptr);
  py::object ext = msg.attr("_external_storage");
  if (ext.is_none()) { return null_region; }
  py::object block = ext.attr("block");
  if (block.is_none()) { return null_region; }
  return rosidl_memory_region_t{
    {reinterpret_cast<void *>(block.attr("address").cast<uintptr_t>()), 0},
    block.attr("size").cast<size_t>()};
}

extern "C" rosidl_memory_region_t
get_backing_storage_@(msg_typename)(const void * message_ptr)
{
  py::gil_scoped_acquire acquire;
  try {
    return get_backing_storage_@(msg_typename)_impl(message_ptr);
  } catch (const std::exception &) {
    rosidl_typesupport_xcdr_cpython::translate_pybind_error("get_backing_storage @(msg_typename)");
    return rosidl_memory_region_t{{nullptr, 0}, 0};
  }
}
@[end if]@

@[if is_experimental]@
// ============================================================================
// Constrained-handle support (experimental variant only — the C-linkage
// symbols would otherwise collide between the standard and experimental TUs).
// ============================================================================

// Build a constrained layout from a Python Msg.Constraints instance.
std::shared_ptr<xcdr_buffers::XCdrStructLayout>
build_constrained_@(msg_typename)(const void * constraints_ptr)
{
  if (nullptr == constraints_ptr) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): constraints is nullptr");
    return nullptr;
  }
  xcdr_buffers::XCdrLayoutBuilder builder;
  auto ret = build_layout_fields_@(msg_typename)(builder, constraints_ptr);
  if (ret != RCUTILS_RET_OK) {
    return nullptr;
  }
  return std::make_shared<xcdr_buffers::XCdrStructLayout>(builder.finalize());
}

// Clone full constraints (blanket + type-specific) into handle-owned storage.
std::shared_ptr<rosidl_message_type_constraints_t>
clone_constraints_@(msg_typename)(const rosidl_message_type_constraints_t * src)
{
  if (nullptr == src) {
    return nullptr;
  }
  auto * clone = new rosidl_message_type_constraints_t();
  clone->type_specific = src->type_specific;
  clone->max_string_length = src->max_string_length;
  clone->max_total_size = src->max_total_size;
  clone->strict = src->strict;
  if (src->type_specific) {
    // Keep the Python Constraints alive for the handle lifetime.  The deleter
    // below DECREFs it while the interpreter is alive; during finalisation it
    // is deliberately leaked (unreachable at that point anyway).
    Py_INCREF(static_cast<PyObject *>(src->type_specific));
  }
  return std::shared_ptr<rosidl_message_type_constraints_t>(
    clone, [](rosidl_message_type_constraints_t * p) {
      if (rosidl_typesupport_xcdr_cpython::interpreter_alive()) {
        py::gil_scoped_acquire acquire;
        Py_DECREF(static_cast<PyObject *>(p->type_specific));
      }
      delete p;
    });
}

// Validate a message instance against its Python Constraints.
// Runs with the GIL held (the runtime outer callback acquires it).
extern "C" rcutils_ret_t
validate_message_@(msg_typename)(
  const void * type_specific,
  const void * message_ptr,
  rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
  void * user_data)
{
  if (nullptr == type_specific || nullptr == message_ptr) {
    RCUTILS_SET_ERROR_MSG("@(msg_typename): constraints or message is nullptr");
    return RCUTILS_RET_ERROR;
  }
    auto _cs = rosidl_typesupport_xcdr_cpython::py_borrow(type_specific);
    auto msg = rosidl_typesupport_xcdr_cpython::py_borrow(message_ptr);
@[  for member in message.structure.members]@
@[    if len(message.structure.members) == 1 and member.name == EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME]@
@[      continue]@
@[    end if]@
@[    if needs_constraints(member.type)]@
@(generate_validate_field(member, msg_typename, is_experimental))
@[    end if]@
@[  end for]@
    return RCUTILS_RET_OK;
}

// Compare two Python Constraints instances (candidate vs baseline).
extern "C" bool
compare_type_specific_constraints_@(msg_typename)(
  const void * lhs,
  const void * rhs)
{
  if (nullptr == lhs || nullptr == rhs) {
    return false;
  }
  // The C trampoline does not hold the GIL; the guard also covers the catch
  // block so the caught py::error_already_set restores the pending exception
  // safely.
  py::gil_scoped_acquire acquire;
  try {
    auto _cand = rosidl_typesupport_xcdr_cpython::py_borrow(lhs);
    auto _base = rosidl_typesupport_xcdr_cpython::py_borrow(rhs);
@[  for member in message.structure.members]@
@[    if len(message.structure.members) == 1 and member.name == EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME]@
@[      continue]@
@[    end if]@
@[    if needs_constraints(member.type)]@
@(generate_compare_field(member, msg_typename, is_experimental))
@[    end if]@
@[  end for]@
    return true;
  } catch (const std::exception &) {
    rosidl_typesupport_xcdr_cpython::translate_pybind_error(
      "compare_type_specific_constraints @(msg_typename)");
    return false;
  }
}
@[end if]@

}  // namespace @(msg_namespace)

// Template specialization with bundled static initialization
namespace rosidl_typesupport_xcdr_cpython
{

namespace
{

inline const rosidl_message_xcdr_cpython_type_support_t & get_inner_@(msg_typename)()
{
  static const auto inner = []() {
    auto tmp = rosidl_message_xcdr_cpython_type_support_t{};
    tmp.serialize_fields = &@(msg_namespace)::serialize_fields_into_writer_@(msg_typename);
    tmp.deserialize_fields = &@(msg_namespace)::deserialize_fields_from_reader_@(msg_typename);
    tmp.build_layout_fields = &@(msg_namespace)::build_layout_fields_@(msg_typename);
    tmp.parse_fields = &@(msg_namespace)::parse_fields_@(msg_typename);
    tmp.compute_serialized_size = &@(msg_namespace)::compute_serialized_size_@(msg_typename);
    tmp.construct_message = &@(msg_namespace)::construct_message_@(msg_typename);
    tmp.cast_message = &@(msg_namespace)::cast_message_@(msg_typename);
    tmp.populate_external_storage = &@(msg_namespace)::populate_external_storage_@(msg_typename);
@[if is_experimental]@
    tmp.build_constrained = &@(msg_namespace)::build_constrained_@(msg_typename);
    tmp.clone_constraints = &@(msg_namespace)::clone_constraints_@(msg_typename);
    tmp.validate_fields = &@(msg_namespace)::validate_message_@(msg_typename);
@[end if]@
@[if not has_constraints]@
    tmp.cached_layout = @(msg_namespace)::get_layout_@(msg_typename)();
@[end if]@
    return tmp;
  }();
  return inner;
}

}  // anonymous namespace

template<>
ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC_@(package_name.upper())
const rosidl_message_type_support_t *
get_message_type_support_handle<@(full_msg_typename)>()
{
  // Thread-safe lazy initialization (C++11 guarantees this for function-local statics)
  static const auto & inner = get_inner_@(msg_typename)();

  static const rosidl_message_xcdr_type_support_t outer = []() {
    auto tmp = *rosidl_typesupport_xcdr_cpython::get_xcdr_cpython_type_support_prototype();
    tmp.inner = const_cast<rosidl_message_xcdr_cpython_type_support_t *>(&inner);
    tmp.message_namespace = "@(msg_namespace)";
    tmp.message_name = "@(msg_typename)";
@[if is_experimental]@
    // Generated overrides: return the external-storage block on release /
    // backing-storage queries (experimental messages only).
    tmp.release_message = &@(msg_namespace)::release_message_@(msg_typename);
    tmp.get_backing_storage = &@(msg_namespace)::get_backing_storage_@(msg_typename);
    tmp.compare_type_specific_constraints =
      &@(msg_namespace)::compare_type_specific_constraints_@(msg_typename);
@[end if]@
    return tmp;
  }();

  static const rosidl_message_type_support_t handle = {
    rosidl_typesupport_xcdr_cpython__identifier,
    &outer,
    get_message_typesupport_handle_function,
    xcdr_cpython_default_get_type_hash,                // safe fallback (returns zero hash)
    xcdr_cpython_default_get_type_description,         // safe fallback (returns nullptr)
    xcdr_cpython_default_get_type_description_sources, // safe fallback (returns nullptr)
  };

  return &handle;
}

}  // namespace rosidl_typesupport_xcdr_cpython

// C symbol export
#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_XCDR_CPYTHON_PUBLIC_@(package_name.upper())
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpython,
  @(', '.join([package_name] + effective_parent_parts)),
  @(msg_typename))()
{
  return rosidl_typesupport_xcdr_cpython::get_message_type_support_handle<@(full_msg_typename)>();
}

#ifdef __cplusplus
}
#endif
