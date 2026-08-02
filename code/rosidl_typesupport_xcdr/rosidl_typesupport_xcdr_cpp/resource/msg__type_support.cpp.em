@# generated from rosidl_typesupport_xcdr_cpp/resource/msg__type_support.cpp.em
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
    NamespacedType,
    SERVICE_REQUEST_MESSAGE_SUFFIX,
    SERVICE_RESPONSE_MESSAGE_SUFFIX,
    SERVICE_EVENT_MESSAGE_SUFFIX,
)
from rosidl_typesupport_xcdr_cpp.template_helpers import (
    get_xcdr_primitive_kind,
    get_cpp_type,
    needs_constraints,
    get_message_type_name,
    get_nested_typesupport_include,
)

# Get optional force_experimental flag (set by experimental wrapper templates).
# When True, always generate experimental-message code paths regardless of namespace.
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
    # Use a single token (e.g., 'msg_experimental') for C symbol names
    # to avoid breaking the 4-argument ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME macro
    effective_parent_parts = [effective_parent_parts[0] + '_experimental']
# Effective include namespace for #include directives
effective_include_ns = '/'.join(msg_namespace_parts)

# Check if this is a service message by checking if it has srv namespace
# and ends with Request/Response/Event suffix
is_service_message = ('srv' in message.structure.namespaced_type.namespaces and
                     (msg_typename.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
                      msg_typename.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX) or
                      msg_typename.endswith(SERVICE_EVENT_MESSAGE_SUFFIX)))

# For service messages, determine the service name and suffix
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
for member in message.structure.members:
    member_type = member.type
    if isinstance(member.type, NamespacedType):
        # Skip service messages (they are declared in the service header, not individual headers)
        is_nested_service_msg = ('srv' in member_type.namespaces and
                                (member_type.name.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
                                 member_type.name.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX) or
                                 member_type.name.endswith(SERVICE_EVENT_MESSAGE_SUFFIX)))
        if not is_nested_service_msg:
            nested_includes.add(get_nested_typesupport_include(
                member_type, experimental_context=is_experimental))
    elif isinstance(member.type, (Array, AbstractSequence)):
        elem_type = member_type.value_type
        if isinstance(elem_type, NamespacedType):
            # Skip service messages
            is_nested_service_msg = ('srv' in elem_type.namespaces and
                                    (elem_type.name.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
                                     elem_type.name.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX) or
                                     elem_type.name.endswith(SERVICE_EVENT_MESSAGE_SUFFIX)))
            if not is_nested_service_msg:
                nested_includes.add(get_nested_typesupport_include(
                    elem_type, experimental_context=is_experimental))

}@
@{
# Collect unique nested types for extern declarations
nested_decls = set()
for member in message.structure.members:
    member_type = member.type
    if isinstance(member_type, NamespacedType):
        full_name = get_message_type_name(member_type, experimental_context=is_experimental)
        parts = full_name.split('::')
        nested_decls.add(('::'.join(parts[:-1]), parts[-1]))
    elif isinstance(member_type, (Array, AbstractSequence)):
        elem_type = member_type.value_type
        if isinstance(elem_type, NamespacedType):
            full_name = get_message_type_name(elem_type, experimental_context=is_experimental)
            parts = full_name.split('::')
            nested_decls.add(('::'.join(parts[:-1]), parts[-1]))
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
#include <string>
#include <vector>

#include "rcutils/error_handling.h"
#include "rcutils/types/rcutils_ret.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_cpp/experimental/memory.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "rosidl_typesupport_xcdr_c/identifier.h"
#include "rosidl_typesupport_xcdr_c/message_type_support.h"
#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"
#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"
#include "rosidl_typesupport_xcdr_cpp/message_type_support_decl.hpp"
#include "@(package_name)/msg/rosidl_typesupport_xcdr_cpp__visibility_control.h"

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
@# ===== EmPy macros for field code generation =====

@[def generate_layout_field(member, is_experimental=True, constraints_prefix='constraints')]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpp.template_helpers import get_xcdr_primitive_kind, get_cpp_type, get_message_type_name, needs_constraints }@ @
@[if isinstance(member.type, BasicType)]@
  builder.allocate_primitive("@(member.name)", @(get_xcdr_primitive_kind(member.type)));
@[elif isinstance(member.type, (BoundedString, BoundedWString))]@
  builder.allocate_string("@(member.name)", @(member.type.maximum_size));
@[elif isinstance(member.type, (AbstractString, AbstractWString))]@
  builder.allocate_string("@(member.name)", @(constraints_prefix).@(member.name).size);
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  builder.allocate_primitive_array("@(member.name)", @(get_xcdr_primitive_kind(member.type.value_type)), @(member.type.size));
@[  else]@
  builder.begin_allocate_array("@(member.name)", @(member.type.size));
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
@[      if isinstance(member.type.value_type, (BoundedString, BoundedWString))]@
    builder.allocate_string(@(member.type.value_type.maximum_size));
@[      else]@
    builder.allocate_string(@(constraints_prefix).@(member.name).size);
@[      end if]@
@[    elif isinstance(member.type.value_type, NamespacedType)]@
    builder.begin_allocate_struct();
    {
      auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
      auto nested_outer_@(member.name) = static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
      nested_inner_@(member.name)->build_layout_fields(builder, nullptr);
    }
    builder.end_allocate_struct();
@[    end if]@
  builder.end_allocate_array();
@[  end if]@
@[elif isinstance(member.type, (BoundedSequence, AbstractSequence))]@
@[  if isinstance(member.type.value_type, BasicType)]@
@[    if isinstance(member.type, BoundedSequence)]@
@[      if needs_constraints(member.type)]@
  {
    auto & @(member.name)_cs = @(constraints_prefix).@(member.name);
    if (@(member.name)_cs.size > @(member.type.maximum_size)) {
      ret = RCUTILS_RET_ERROR;
    }
    builder.allocate_primitive_sequence("@(member.name)", @(get_xcdr_primitive_kind(member.type.value_type)), @(member.name)_cs.size);
  }
@[      else]@
  builder.allocate_primitive_sequence("@(member.name)", @(get_xcdr_primitive_kind(member.type.value_type)), @(member.type.maximum_size));
@[      end if]@
@[    else]@
  builder.allocate_primitive_sequence("@(member.name)", @(get_xcdr_primitive_kind(member.type.value_type)), @(constraints_prefix).@(member.name).size);
@[    end if]@
@[  else]@
@[    if isinstance(member.type, BoundedSequence)]@
@[      if needs_constraints(member.type)]@
  {
    auto & @(member.name)_cs = @(constraints_prefix).@(member.name);
    if (@(member.name)_cs.size > @(member.type.maximum_size)) {
      ret = RCUTILS_RET_ERROR;
    }
    builder.begin_allocate_sequence("@(member.name)", @(member.name)_cs.size);
  }
@[      else]@
  builder.begin_allocate_sequence("@(member.name)", @(member.type.maximum_size));
@[      end if]@
@[    else]@
  builder.begin_allocate_sequence("@(member.name)", @(constraints_prefix).@(member.name).size);
@[    end if]@
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
@[      if isinstance(member.type.value_type, (BoundedString, BoundedWString))]@
    builder.allocate_string(@(member.type.value_type.maximum_size));
@[      else]@
    builder.allocate_string(@(constraints_prefix).@(member.name).element.size);
@[      end if]@
@[    elif isinstance(member.type.value_type, NamespacedType)]@
    builder.begin_allocate_struct();
    {
      auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
      auto nested_outer_@(member.name) =
        static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
      nested_inner_@(member.name)->build_layout_fields(builder, nullptr);
    }
    builder.end_allocate_struct();
@[    end if]@
  builder.end_allocate_sequence();
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  // Nested message: @(member.name)
  builder.begin_allocate_struct("@(member.name)");
  {
    auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto nested_outer_@(member.name) =
      static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
    nested_inner_@(member.name)->build_layout_fields(builder, &@(constraints_prefix).@(member.name));
  }
  builder.end_allocate_struct();
@[else]@
  // TODO: Handle @(member.name) of type @(member.type)
@[end if]@
@[end def]@

@[def generate_parser_field(member, is_experimental=False)]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpp.template_helpers import get_xcdr_primitive_kind, get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  parser.parse_primitive(@(get_xcdr_primitive_kind(member.type)));
@[elif isinstance(member.type, (AbstractString, AbstractWString))]@
  parser.parse_string();
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  parser.parse_primitive_array(@(get_xcdr_primitive_kind(member.type.value_type)), @(member.type.size));
@[  else]@
  parser.begin_parse_array(@(member.type.size));
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
    parser.parse_string();
@[    elif isinstance(member.type.value_type, NamespacedType)]@
    parser.begin_parse_struct();
    {
      auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
      auto nested_outer_@(member.name) = static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
      if (nullptr == nested_inner_@(member.name)->parse_fields) {
        return RCUTILS_RET_ERROR;
      }
      auto _ret_@(member.name) = nested_inner_@(member.name)->parse_fields(parser);
      if (RCUTILS_RET_OK != _ret_@(member.name)) { return _ret_@(member.name); }
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
  auto @(member.name)_size = parser.begin_parse_sequence();
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
    parser.parse_string();
@[    elif isinstance(member.type.value_type, NamespacedType)]@
    parser.begin_parse_struct();
    {
      auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
      auto nested_outer_@(member.name) = static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
      if (nullptr == nested_inner_@(member.name)->parse_fields) {
        return RCUTILS_RET_ERROR;
      }
      auto _ret_@(member.name) = nested_inner_@(member.name)->parse_fields(parser);
      if (RCUTILS_RET_OK != _ret_@(member.name)) { return _ret_@(member.name); }
    }
    parser.end_parse_struct();
@[    end if]@
  parser.end_parse_sequence();
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  parser.begin_parse_struct("@(member.name)");
  {
    auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto nested_outer_@(member.name) = static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
    auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
    if (nullptr == nested_inner_@(member.name)->parse_fields) {
      return RCUTILS_RET_ERROR;
    }
    auto _ret_@(member.name) = nested_inner_@(member.name)->parse_fields(parser);
    if (RCUTILS_RET_OK != _ret_@(member.name)) { return _ret_@(member.name); }
  }
  parser.end_parse_struct();
@[else]@
  // TODO: Parse @(member.name)
@[end if]@
@[end def]@

@[def generate_writer_field(member, is_experimental, msg_prefix='msg')]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpp.template_helpers import get_xcdr_primitive_kind, get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  writer.write<@(get_cpp_type(member.type))>(@(msg_prefix).@(member.name));
@[elif isinstance(member.type, AbstractWString)]@
@[  if is_experimental]@
  writer.write(std::u16string_view(@(msg_prefix).@(member.name).data(), @(msg_prefix).@(member.name).size()));
@[  else]@
  writer.write(std::u16string_view(@(msg_prefix).@(member.name)));
@[  end if]@
@[elif isinstance(member.type, AbstractString)]@
@[  if is_experimental]@
  writer.write(std::string_view(@(msg_prefix).@(member.name).data(), @(msg_prefix).@(member.name).size()));
@[  else]@
  writer.write(std::string_view(@(msg_prefix).@(member.name)));
@[  end if]@
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  writer.write_array(tcb::span<const @(get_cpp_type(member.type.value_type))>(@(msg_prefix).@(member.name).data(), @(member.type.size)));
@[  else]@
  writer.begin_write_array(@(member.type.size));
@[    if isinstance(member.type.value_type, AbstractWString)]@
@[      if is_experimental]@
  for (size_t i = 0; i < @(member.type.size); ++i) {
    writer.write(std::u16string_view(@(msg_prefix).@(member.name)[i].data(), @(msg_prefix).@(member.name)[i].size()));
  }
@[      else]@
  for (const auto & elem : @(msg_prefix).@(member.name)) {
    writer.write(std::u16string_view(elem));
  }
@[      end if]@
@[    elif isinstance(member.type.value_type, AbstractString)]@
@[      if is_experimental]@
  for (size_t i = 0; i < @(member.type.size); ++i) {
    writer.write(std::string_view(@(msg_prefix).@(member.name)[i].data(), @(msg_prefix).@(member.name)[i].size()));
  }
@[      else]@
  for (const auto & elem : @(msg_prefix).@(member.name)) {
    writer.write(std::string_view(elem));
  }
@[      end if]@
@[    elif isinstance(member.type.value_type, NamespacedType)]@
  {
    auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto nested_outer_@(member.name) =
      static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
    for (size_t i = 0; i < @(member.type.size); ++i) {
      nested_inner_@(member.name)->serialize_fields(&@(msg_prefix).@(member.name)[i], writer);
    }
  }
@[    end if]@
  writer.end_write_array();
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
@[    if get_cpp_type(member.type.value_type) == 'bool']@
  writer.begin_write_sequence(@(msg_prefix).@(member.name).size());
  for (const auto & elem : @(msg_prefix).@(member.name)) {
    writer.write(static_cast<uint8_t>(elem));
  }
  writer.end_write_sequence();
@[    else]@
  writer.write_sequence(tcb::span<const @(get_cpp_type(member.type.value_type))>(@(msg_prefix).@(member.name).data(), @(msg_prefix).@(member.name).size()));
@[    end if]@
@[  else]@
  writer.begin_write_sequence(@(msg_prefix).@(member.name).size());
@[    if isinstance(member.type.value_type, AbstractWString)]@
@[      if is_experimental]@
  for (size_t i = 0; i < @(msg_prefix).@(member.name).size(); ++i) {
    writer.write(std::u16string_view(@(msg_prefix).@(member.name)[i].data(), @(msg_prefix).@(member.name)[i].size()));
  }
@[      else]@
  for (const auto & elem : @(msg_prefix).@(member.name)) {
    writer.write(std::u16string_view(elem));
  }
@[      end if]@
@[    elif isinstance(member.type.value_type, AbstractString)]@
@[      if is_experimental]@
  for (size_t i = 0; i < @(msg_prefix).@(member.name).size(); ++i) {
    writer.write(std::string_view(@(msg_prefix).@(member.name)[i].data(), @(msg_prefix).@(member.name)[i].size()));
  }
@[      else]@
  for (const auto & elem : @(msg_prefix).@(member.name)) {
    writer.write(std::string_view(elem));
  }
@[      end if]@
@[    elif isinstance(member.type.value_type, NamespacedType)]@
  {
    auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto nested_outer_@(member.name) =
      static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
    for (const auto & elem : @(msg_prefix).@(member.name)) {
      nested_inner_@(member.name)->serialize_fields(&elem, writer);
    }
  }
@[    end if]@
  writer.end_write_sequence();
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  // Nested message: @(member.name)
  {
    auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto nested_outer_@(member.name) =
      static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
    nested_inner_@(member.name)->serialize_fields(&@(msg_prefix).@(member.name), writer);
  }
@[else]@
  // TODO: Write @(member.name)
@[end if]@
@[end def]@

@[def generate_reader_field(member, is_experimental, msg_prefix='msg')]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpp.template_helpers import get_xcdr_primitive_kind, get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  {
    auto @(member.name)_result = reader.read<@(get_cpp_type(member.type))>();
    if (!@(member.name)_result) { return RCUTILS_RET_ERROR; }
    @(msg_prefix).@(member.name) = *@(member.name)_result;
  }
@[elif isinstance(member.type, AbstractWString)]@
  {
    auto @(member.name)_result = reader.read<std::u16string_view>();
    if (!@(member.name)_result) { return RCUTILS_RET_ERROR; }
    auto @(member.name)_view = *@(member.name)_result;
    @(msg_prefix).@(member.name).assign(@(member.name)_view);
  }
@[elif isinstance(member.type, AbstractString)]@
  {
    auto @(member.name)_result = reader.read<std::string_view>();
    if (!@(member.name)_result) { return RCUTILS_RET_ERROR; }
    auto @(member.name)_view = *@(member.name)_result;
    @(msg_prefix).@(member.name).assign(@(member.name)_view);
  }
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto @(member.name)_result = reader.read<std::array<@(get_cpp_type(member.type.value_type)), @(member.type.size)> >();
    if (!@(member.name)_result) { return RCUTILS_RET_ERROR; }
    auto @(member.name)_array = *@(member.name)_result;
@[    if is_experimental]@
    std::copy(@(member.name)_array.begin(), @(member.name)_array.end(), @(msg_prefix).@(member.name).begin());
@[    else]@
    @(msg_prefix).@(member.name) = @(member.name)_array;
@[    end if]@
  }
@[  else]@
  reader.begin_read_array(@(member.type.size));
@[    if isinstance(member.type.value_type, AbstractWString)]@
  for (size_t i = 0; i < @(member.type.size); ++i) {
    auto @(member.name)_elem_result = reader.read<std::u16string_view>();
    if (!@(member.name)_elem_result) { return RCUTILS_RET_ERROR; }
    auto elem_view = *@(member.name)_elem_result;
    @(msg_prefix).@(member.name)[i].assign(elem_view);
  }
@[    elif isinstance(member.type.value_type, AbstractString)]@
  for (size_t i = 0; i < @(member.type.size); ++i) {
    auto @(member.name)_elem_result = reader.read<std::string_view>();
    if (!@(member.name)_elem_result) { return RCUTILS_RET_ERROR; }
    auto elem_view = *@(member.name)_elem_result;
    @(msg_prefix).@(member.name)[i].assign(elem_view);
  }
@[    elif isinstance(member.type.value_type, NamespacedType)]@
  {
    auto nested_ts_@(member.name) =
      rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto nested_outer_@(member.name) =
      static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
    for (size_t i = 0; i < @(member.type.size); ++i) {
      nested_inner_@(member.name)->deserialize_fields(reader, &@(msg_prefix).@(member.name)[i]);
    }
  }
@[    end if]@
  reader.end_read_array();
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto @(member.name)_result = reader.read<std::vector<@(get_cpp_type(member.type.value_type))>>();
    if (!@(member.name)_result) { return RCUTILS_RET_ERROR; }
    auto @(member.name)_vec = *@(member.name)_result;
    @(msg_prefix).@(member.name) = std::move(@(member.name)_vec);
  }
@[  else]@
  {
    auto @(member.name)_size_result = reader.begin_read_sequence();
    if (!@(member.name)_size_result) { return RCUTILS_RET_ERROR; }
    auto @(member.name)_size = *@(member.name)_size_result;
    @(msg_prefix).@(member.name).resize(@(member.name)_size);
@[    if isinstance(member.type.value_type, AbstractWString)]@
    for (size_t i = 0; i < @(member.name)_size; ++i) {
      auto @(member.name)_elem_result = reader.read<std::u16string_view>();
      if (!@(member.name)_elem_result) { return RCUTILS_RET_ERROR; }
      auto elem_view = *@(member.name)_elem_result;
      @(msg_prefix).@(member.name)[i].assign(elem_view);
    }
@[    elif isinstance(member.type.value_type, AbstractString)]@
    for (size_t i = 0; i < @(member.name)_size; ++i) {
      auto @(member.name)_elem_result = reader.read<std::string_view>();
      if (!@(member.name)_elem_result) { return RCUTILS_RET_ERROR; }
      auto elem_view = *@(member.name)_elem_result;
      @(msg_prefix).@(member.name)[i].assign(elem_view);
    }
@[    elif isinstance(member.type.value_type, NamespacedType)]@
    {
      auto nested_ts_@(member.name) =
        rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
      auto nested_outer_@(member.name) =
        static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
      for (size_t i = 0; i < @(member.name)_size; ++i) {
        nested_inner_@(member.name)->deserialize_fields(reader, &@(msg_prefix).@(member.name)[i]);
      }
    }
@[    end if]@
    reader.end_read_sequence();
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  // Nested message: @(member.name)
  {
    auto nested_ts_@(member.name) =
      rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto nested_outer_@(member.name) =
      static_cast<const rosidl_message_xcdr_type_support_t *>(nested_ts_@(member.name)->data);
      auto nested_inner_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(nested_outer_@(member.name)->inner);
    nested_inner_@(member.name)->deserialize_fields(reader, &@(msg_prefix).@(member.name));
  }
@[else]@
  // TODO: Read @(member.name)
@[end if]@
@[end def]@

@[def generate_external_storage_field(member, index, _is_experimental=is_experimental)]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpp.template_helpers import get_xcdr_primitive_kind, get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  {
    auto _@(member.name)_slice = accessor[@(index)].slice();
    ext_storage.members.@(member.name) = rosidl_runtime_cpp::Memory<@(get_cpp_type(member.type))>(
      static_cast<@(get_cpp_type(member.type))*>(const_cast<void*>(static_cast<const void*>(_@(member.name)_slice.data()))), 0);
  }
@[elif isinstance(member.type, AbstractWString)]@
  {
    auto _@(member.name)_slice = accessor[@(index)].slice();
    auto _@(member.name)_data = _@(member.name)_slice.subspan(xcdr_buffers::kStringLengthPrefixSize);
    ext_storage.members.@(member.name) = rosidl_runtime_cpp::MemoryRegion<char16_t>(
      static_cast<char16_t*>(const_cast<void*>(static_cast<const void*>(_@(member.name)_data.data()))),
      _@(member.name)_data.size());
  }
@[elif isinstance(member.type, AbstractString)]@
  {
    auto _@(member.name)_slice = accessor[@(index)].slice();
    auto _@(member.name)_data = _@(member.name)_slice.subspan(xcdr_buffers::kStringLengthPrefixSize);
    ext_storage.members.@(member.name) = rosidl_runtime_cpp::MemoryRegion<char>(
      static_cast<char*>(const_cast<void*>(static_cast<const void*>(_@(member.name)_data.data()))),
      _@(member.name)_data.size());
  }
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto _@(member.name)_slice = accessor[@(index)].slice();
    ext_storage.members.@(member.name) = rosidl_runtime_cpp::MemoryRegion<@(get_cpp_type(member.type.value_type))>(
      static_cast<@(get_cpp_type(member.type.value_type))*>(const_cast<void*>(static_cast<const void*>(_@(member.name)_slice.data()))),
      @(member.type.size));
  }
@[  elif isinstance(member.type.value_type, AbstractString)]@
  {
    auto _@(member.name)_arr = accessor[@(index)];
    for (size_t _@(member.name)_j = 0; _@(member.name)_j < @(member.type.size); ++_@(member.name)_j) {
      auto _@(member.name)_elem_slice = _@(member.name)_arr[_@(member.name)_j].slice();
      auto _@(member.name)_elem_data = _@(member.name)_elem_slice.subspan(xcdr_buffers::kStringLengthPrefixSize);
      ext_storage.members.@(member.name)[_@(member.name)_j] = rosidl_runtime_cpp::MemoryRegion<char>(
        static_cast<char*>(const_cast<void*>(static_cast<const void*>(_@(member.name)_elem_data.data()))),
        _@(member.name)_elem_data.size());
    }
  }
@[  elif isinstance(member.type.value_type, AbstractWString)]@
  {
    auto _@(member.name)_arr = accessor[@(index)];
    for (size_t _@(member.name)_j = 0; _@(member.name)_j < @(member.type.size); ++_@(member.name)_j) {
      auto _@(member.name)_elem_slice = _@(member.name)_arr[_@(member.name)_j].slice();
      auto _@(member.name)_elem_data = _@(member.name)_elem_slice.subspan(xcdr_buffers::kStringLengthPrefixSize);
      ext_storage.members.@(member.name)[_@(member.name)_j] = rosidl_runtime_cpp::MemoryRegion<char16_t>(
        static_cast<char16_t*>(const_cast<void*>(static_cast<const void*>(_@(member.name)_elem_data.data()))),
        _@(member.name)_elem_data.size());
    }
  }
@[  elif isinstance(member.type.value_type, NamespacedType)]@
@{
_ns_arr_fn_full = get_message_type_name(member.type.value_type, experimental_context=_is_experimental)
_ns_arr_fn_parts = _ns_arr_fn_full.split('::')
_ns_arr_fn_ns = '::'.join(_ns_arr_fn_parts[:-1])
_ns_arr_fn_name = _ns_arr_fn_parts[-1]
}@
  {
    auto _@(member.name)_arr = accessor[@(index)];
    for (size_t _@(member.name)_j = 0; _@(member.name)_j < @(member.type.size); ++_@(member.name)_j) {
      auto _@(member.name)_elem = _@(member.name)_arr[_@(member.name)_j];
      auto _ret = @(_ns_arr_fn_ns)::populate_external_storage_@(_ns_arr_fn_name)(_@(member.name)_elem, &ext_storage.members.@(member.name)[_@(member.name)_j]);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
  }
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  {
    auto _@(member.name)_slice = accessor[@(index)].slice();
    auto _@(member.name)_data = _@(member.name)_slice.subspan(xcdr_buffers::kSequenceLengthPrefixSize);
    ext_storage.members.@(member.name) = rosidl_runtime_cpp::MemoryRegion<@(get_cpp_type(member.type.value_type))>(
      static_cast<@(get_cpp_type(member.type.value_type))*>(const_cast<void*>(static_cast<const void*>(_@(member.name)_data.data()))),
      *accessor[@(index)].size() * sizeof(@(get_cpp_type(member.type.value_type))));
  }
@[  elif isinstance(member.type.value_type, AbstractString)]@
  {
    auto _@(member.name)_seq = accessor[@(index)];
    auto _@(member.name)_size_result = _@(member.name)_seq.size();
    if (!_@(member.name)_size_result) { return RCUTILS_RET_ERROR; }
    size_t _@(member.name)_size = *_@(member.name)_size_result;
    ext_storage.members.@(member.name).resize(_@(member.name)_size);
    for (size_t _@(member.name)_j = 0; _@(member.name)_j < _@(member.name)_size; ++_@(member.name)_j) {
      auto _@(member.name)_elem_slice = _@(member.name)_seq[_@(member.name)_j].slice();
      auto _@(member.name)_elem_data = _@(member.name)_elem_slice.subspan(xcdr_buffers::kStringLengthPrefixSize);
      ext_storage.members.@(member.name)[_@(member.name)_j] = rosidl_runtime_cpp::MemoryRegion<char>(
        static_cast<char*>(const_cast<void*>(static_cast<const void*>(_@(member.name)_elem_data.data()))),
        _@(member.name)_elem_data.size());
    }
  }
@[  elif isinstance(member.type.value_type, AbstractWString)]@
  {
    auto _@(member.name)_seq = accessor[@(index)];
    auto _@(member.name)_size_result = _@(member.name)_seq.size();
    if (!_@(member.name)_size_result) { return RCUTILS_RET_ERROR; }
    size_t _@(member.name)_size = *_@(member.name)_size_result;
    ext_storage.members.@(member.name).resize(_@(member.name)_size);
    for (size_t _@(member.name)_j = 0; _@(member.name)_j < _@(member.name)_size; ++_@(member.name)_j) {
      auto _@(member.name)_elem_slice = _@(member.name)_seq[_@(member.name)_j].slice();
      auto _@(member.name)_elem_data = _@(member.name)_elem_slice.subspan(xcdr_buffers::kStringLengthPrefixSize);
      ext_storage.members.@(member.name)[_@(member.name)_j] = rosidl_runtime_cpp::MemoryRegion<char16_t>(
        static_cast<char16_t*>(const_cast<void*>(static_cast<const void*>(_@(member.name)_elem_data.data()))),
        _@(member.name)_elem_data.size());
    }
  }
@[  elif isinstance(member.type.value_type, NamespacedType)]@
@{
_ns_seq_fn_full = get_message_type_name(member.type.value_type, experimental_context=_is_experimental)
_ns_seq_fn_parts = _ns_seq_fn_full.split('::')
_ns_seq_fn_ns = '::'.join(_ns_seq_fn_parts[:-1])
_ns_seq_fn_name = _ns_seq_fn_parts[-1]
}@
  {
    auto _@(member.name)_seq = accessor[@(index)];
    auto _@(member.name)_size_result = _@(member.name)_seq.size();
    if (!_@(member.name)_size_result) { return RCUTILS_RET_ERROR; }
    size_t _@(member.name)_size = *_@(member.name)_size_result;
    ext_storage.members.@(member.name).resize(_@(member.name)_size);
    for (size_t _@(member.name)_j = 0; _@(member.name)_j < _@(member.name)_size; ++_@(member.name)_j) {
      auto _@(member.name)_elem = _@(member.name)_seq[_@(member.name)_j];
      auto _ret = @(_ns_seq_fn_ns)::populate_external_storage_@(_ns_seq_fn_name)(_@(member.name)_elem, &ext_storage.members.@(member.name)[_@(member.name)_j]);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
@{
_ns_fn_full = get_message_type_name(member.type, experimental_context=_is_experimental)
_ns_fn_parts = _ns_fn_full.split('::')
_ns_fn_ns = '::'.join(_ns_fn_parts[:-1])
_ns_fn_name = _ns_fn_parts[-1]
}@
  {
    auto _@(member.name)_nested_acc = accessor[@(index)];
    auto _ret = @(_ns_fn_ns)::populate_external_storage_@(_ns_fn_name)(_@(member.name)_nested_acc, &ext_storage.members.@(member.name));
    if (_ret != RCUTILS_RET_OK) { return _ret; }
  }
@[end if]@
@[end def]@

@[def generate_size_calculation(member, is_experimental=False, msg_prefix='msg')]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpp.template_helpers import get_xcdr_primitive_kind, get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  data_offset = xcdr_buffers::align_to(data_offset, sizeof(@(get_cpp_type(member.type))));
  data_offset += sizeof(@(get_cpp_type(member.type)));
@[elif isinstance(member.type, (AbstractString, AbstractWString))]@
  data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kStringLengthPrefixSize);
  data_offset += xcdr_buffers::kStringLengthPrefixSize;  // length prefix
  data_offset += @(msg_prefix).@(member.name).size();
  data_offset += xcdr_buffers::kStringNullTerminatorSize;  // null terminator
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  data_offset = xcdr_buffers::align_to(data_offset, sizeof(@(get_cpp_type(member.type.value_type))));
  data_offset += sizeof(@(get_cpp_type(member.type.value_type))) * @(member.type.size);
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  for (size_t i = 0; i < @(member.type.size); ++i) {
    data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kStringLengthPrefixSize);
    data_offset += xcdr_buffers::kStringLengthPrefixSize;  // length prefix
    data_offset += @(msg_prefix).@(member.name)[i].size();
    data_offset += xcdr_buffers::kStringNullTerminatorSize;  // null terminator
  }
@[  elif isinstance(member.type.value_type, NamespacedType)]@
  {
    auto nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    for (size_t i = 0; i < @(member.type.size); ++i) {
      size_t nested_size = 0;
      rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts, &@(msg_prefix).@(member.name)[i], &nested_size);
      data_offset += nested_size - xcdr_buffers::kXCdrHeaderSize;  // Inline without XCDR header
    }
  }
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kSequenceLengthPrefixSize);
  data_offset += xcdr_buffers::kSequenceLengthPrefixSize;  // sequence length prefix
  data_offset = xcdr_buffers::align_to(data_offset, sizeof(@(get_cpp_type(member.type.value_type))));
  data_offset += sizeof(@(get_cpp_type(member.type.value_type))) * @(msg_prefix).@(member.name).size();
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kSequenceLengthPrefixSize);
  data_offset += xcdr_buffers::kSequenceLengthPrefixSize;  // sequence length prefix
  for (const auto & elem : @(msg_prefix).@(member.name)) {
    data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kStringLengthPrefixSize);
    data_offset += xcdr_buffers::kStringLengthPrefixSize;  // length prefix
    data_offset += elem.size();
    data_offset += xcdr_buffers::kStringNullTerminatorSize;
  }
@[  elif isinstance(member.type.value_type, NamespacedType)]@
  data_offset = xcdr_buffers::align_to(data_offset, xcdr_buffers::kSequenceLengthPrefixSize);
  data_offset += xcdr_buffers::kSequenceLengthPrefixSize;  // sequence length prefix
  {
    auto nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    for (const auto & elem : @(msg_prefix).@(member.name)) {
      size_t nested_size = 0;
      rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts, &elem, &nested_size);
      data_offset += nested_size - xcdr_buffers::kXCdrHeaderSize;  // Inline without XCDR header
    }
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  {
    auto nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    size_t nested_size = 0;
    rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts, &@(msg_prefix).@(member.name), &nested_size);
    data_offset += nested_size - xcdr_buffers::kXCdrHeaderSize;  // Inline without XCDR header
  }
@[else]@
  // TODO: Size calculation for @(member.name)
@[end if]@
@[end def]@


@[if is_experimental and nested_decls]@
@# Forward-declare nested populate_external_storage functions
@[for ns, name in nested_decls]@
namespace @(ns)
{
rcutils_ret_t populate_external_storage_@(name)(const xcdr_buffers::XCdrConstAccessor & accessor, void * ext_storage_ptr);
}  // namespace @(ns)
@[end for]@
@[end if]@
@[if is_experimental]@
// ========== EXPERIMENTAL MESSAGE (@(full_msg_typename)) ==========

namespace @(msg_namespace)
{

@[  if has_constraints]@
// Build layout from constraints (for messages with variable-length fields)
std::shared_ptr<xcdr_buffers::XCdrStructLayout>
build_layout_@(msg_typename)(const void * constraints_ptr)
{
  if (!constraints_ptr) {
    return nullptr;
  }

  auto & constraints = *static_cast<const @(full_msg_typename)::Constraints *>(constraints_ptr);

  xcdr_buffers::XCdrLayoutBuilder builder;
  rcutils_ret_t ret = RCUTILS_RET_OK;
@[    for member in message.structure.members]@
@(generate_layout_field(member))
@[    end for]@
  if (ret != RCUTILS_RET_OK) {
    return nullptr;
  }
  return std::make_shared<xcdr_buffers::XCdrStructLayout>(builder.finalize());
}

// Private: Build layout fields into existing builder
rcutils_ret_t
build_layout_fields_@(msg_typename)(
  xcdr_buffers::XCdrLayoutBuilder & builder,
  const void * constraints_ptr)
{
  if (!constraints_ptr) {
    return RCUTILS_RET_ERROR;
  }

  auto & constraints = *static_cast<const @(full_msg_typename)::Constraints *>(constraints_ptr);
  rcutils_ret_t ret = RCUTILS_RET_OK;
@[    for member in message.structure.members]@
@(generate_layout_field(member))
@[    end for]@

  return ret;
}
@[  else]@
// Get singleton layout (for fully bounded messages)
std::shared_ptr<xcdr_buffers::XCdrStructLayout>
get_layout_@(msg_typename)()
{
  static auto layout = std::make_shared<xcdr_buffers::XCdrStructLayout>([]() {
    xcdr_buffers::XCdrLayoutBuilder builder;
@[    for member in message.structure.members]@
@(generate_layout_field(member, 'constraints'))
@[    end for]@

    return builder.finalize();
  }());
  return layout;
}

// Private: Build layout fields into existing builder (for fully bounded)
rcutils_ret_t
build_layout_fields_@(msg_typename)(
  xcdr_buffers::XCdrLayoutBuilder & builder,
  const void * constraints_ptr)
{
  (void)constraints_ptr;  // Unused for fully bounded messages
@[    for member in message.structure.members]@
@(generate_layout_field(member, 'constraints'))
@[    end for]@

  return RCUTILS_RET_OK;
}

// Forward declarations (defined below in shared section).
rcutils_ret_t
serialize_fields_into_writer_@(msg_typename)(
  const void * message_ptr,
  xcdr_buffers::XCdrWriter & writer);

extern "C" rosidl_memory_region_t
release_message_@(msg_typename)(void * message_ptr);

// ============================================================================
// compact_fields — internal recursive workhorse (fully bounded, no constraints)
// All fields are fixed-size, so emit never flips — all fields are skipped or
// written based on parent's emit flag only.
// ============================================================================

rcutils_ret_t
compact_fields_@(msg_typename)(
  const void * untyped_msg,
  const xcdr_buffers::XCdrStructLayout & layout,
  xcdr_buffers::XCdrWriter & writer,
  bool & emit)
{
  if (nullptr == untyped_msg) {
    return RCUTILS_RET_ERROR;
  }
  auto & msg = *static_cast<const @(full_msg_typename) *>(untyped_msg);
  (void)layout;
@[  for member in message.structure.members]@
@[    if isinstance(member.type, BasicType)]@
  if (emit) {
    writer.write<@(get_cpp_type(member.type))>(msg.@(member.name));
  } else {
    writer.skip<@(get_cpp_type(member.type))>();
  }

@[    elif isinstance(member.type, Array)]@
@[      if isinstance(member.type.value_type, BasicType)]@
  if (emit) {
    writer.write_array(tcb::span<const @(get_cpp_type(member.type.value_type))>(msg.@(member.name).data(), @(member.type.size)));
  } else {
    writer.skip_array<@(get_cpp_type(member.type.value_type))>(@(member.type.size));
  }

@[      elif isinstance(member.type.value_type, NamespacedType)]@
@{
nested_fn_arr = get_message_type_name(member.type.value_type, experimental_context=is_experimental)
}@
  {
    auto _nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(nested_fn_arr)>();
    auto _nested_outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts->data);
    auto _nested_inner = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(_nested_outer->inner);
    const auto & _arr_layout = std::get<xcdr_buffers::XCdrStructLayout>(
      layout.get_member(@(i))->get().layout());
    for (size_t _j = 0; _j < @(member.type.size); ++_j) {
      const auto & _elem_layout = std::get<xcdr_buffers::XCdrStructLayout>(
        _arr_layout.element_layout(_j)->get());
      _nested_inner->compact_fields_recursive(
        &msg.@(member.name)[_j], _elem_layout, writer, emit);
    }
  }

@[      end if]@

@[    elif isinstance(member.type, NamespacedType)]@
@{
nested_fn = get_message_type_name(member.type, experimental_context=is_experimental)
}@
  {
    auto _nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(nested_fn)>();
    auto _nested_outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts->data);
    auto _nested_inner = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(_nested_outer->inner);
    const auto & _nested_layout = std::get<xcdr_buffers::XCdrStructLayout>(
      layout.get_member(@(i))->get().layout());
    if (emit) {
      _nested_inner->serialize_fields(&msg.@(member.name), writer);
    } else {
      _nested_inner->compact_fields_recursive(
        &msg.@(member.name), _nested_layout, writer, emit);
    }
  }

@[    end if]@
@[  end for]@
  return RCUTILS_RET_OK;
}

// Consume message view by compacting in-place (fully bounded, no constraints).
// All fields are fixed-size so compact == layout always — fast path via release.
extern "C" rosidl_memory_region_t
compact_message_@(msg_typename)(
  void * message_ptr,
  const xcdr_buffers::XCdrStructLayout * /* cached_layout */)
{
  rosidl_memory_region_t null_region = {{nullptr, 0}, 0};
  if (nullptr == message_ptr) {
    RCUTILS_SET_ERROR_MSG("message_ptr is nullptr");
    return null_region;
  }
  auto & msg = *static_cast<@(full_msg_typename) *>(message_ptr);
  if (!msg._external_storage.has_value()) {
    RCUTILS_SET_ERROR_MSG("no external storage to release");
    return null_region;
  }
  return release_message_@(msg_typename)(message_ptr);
}
@[  end if]@

// Forward declaration for populate_external_storage (defined below in shared section).
rcutils_ret_t
populate_external_storage_@(msg_typename)(
  const xcdr_buffers::XCdrConstAccessor & accessor,
  void * ext_storage_ptr);

// Private: Serialize message fields into existing writer (no XCDR header)
rcutils_ret_t
serialize_fields_into_writer_@(msg_typename)(
  const void * message_ptr,
  xcdr_buffers::XCdrWriter & writer)
{
  auto & msg = *static_cast<const @(full_msg_typename) *>(message_ptr);
@[  for member in message.structure.members]@
@(generate_writer_field(member, is_experimental, 'msg'))
@[  end for]@

  return RCUTILS_RET_OK;
}

// Private: Deserialize message fields from existing reader (no XCDR header)
rcutils_ret_t
deserialize_fields_from_reader_@(msg_typename)(
  xcdr_buffers::XCdrReader & reader,
  void * message_ptr)
{
  auto & msg = *static_cast<@(full_msg_typename) *>(message_ptr);
@[  for member in message.structure.members]@
@(generate_reader_field(member, is_experimental, 'msg'))
@[  end for]@

  return RCUTILS_RET_OK;
}

// Construct message at storage (zero-copy sender side)
rcutils_ret_t
construct_message_at_@(msg_typename)(
  const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t * impl,
  rosidl_runtime_cpp::MemoryRegion<void> & storage,
  void ** message_ptr)
{
  if (!impl || !impl->cached_layout) {
    // Error:("Layout not available (message needs constraints)");
    return RCUTILS_RET_ERROR;
  }

  // 1. Initialize buffer with XCDR layout
  auto buffer_span = tcb::span<uint8_t>(
    static_cast<uint8_t*>(storage.data()),
    impl->cached_layout->total_size());
  impl->cached_layout->apply(buffer_span);

  // 2. Create const accessor for populating external storage
  auto const_accessor_result = xcdr_buffers::XCdrConstAccessor::wrap(
    tcb::span<const uint8_t>(buffer_span.data(), buffer_span.size()),
    *impl->cached_layout);
  if (!const_accessor_result) {
    // Error:("Failed to create const accessor");
    return RCUTILS_RET_ERROR;
  }
  auto accessor = *const_accessor_result;

  // 3. Build external storage from accessor
  @(full_msg_typename)::ExternalStorage ext_storage;
  ext_storage.block = rosidl_runtime_cpp::MemoryRegion<void>(
    storage.data(),
    impl->cached_layout->total_size());

  // Populate external storage from accessor
  {
    auto _populate_ret = populate_external_storage_@(msg_typename)(accessor, &ext_storage);
    if (_populate_ret != RCUTILS_RET_OK) { return _populate_ret; }
  }

  // 4. Construct message from external storage (prepopulated=false by default)
  *message_ptr = new @(full_msg_typename)(ext_storage, rosidl_runtime_cpp::MessageInitialization::SKIP);

  return RCUTILS_RET_OK;
}

// Populate external storage from accessor (shared between construct and cast)
rcutils_ret_t
populate_external_storage_@(msg_typename)(
  const xcdr_buffers::XCdrConstAccessor & accessor,
  void * ext_storage_ptr)
{
  auto & ext_storage = *static_cast<@(full_msg_typename)::ExternalStorage *>(ext_storage_ptr);

@[  for idx, member in enumerate(message.structure.members)]@
@(generate_external_storage_field(member, idx))
@[  end for]@

  return RCUTILS_RET_OK;
}

// Parse layout fields from buffer (zero-copy receiver side)
//
// Recurses into nested struct members via the nested type's own parse_fields
// callback, so variable-length members anywhere in the message tree get
// correctly inferred offsets from the wire data.
rcutils_ret_t
parse_fields_@(msg_typename)(
  xcdr_buffers::XCdrLayoutParser & parser)
{
@[  for member in message.structure.members]@
@(generate_parser_field(member, is_experimental))
@[  end for]@

  return RCUTILS_RET_OK;
}

// Cast message at storage (zero-copy receiver side)
rcutils_ret_t
cast_message_at_@(msg_typename)(
  const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t * impl,
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void ** message_ptr)
{
  // 1. Parse layout from buffer
  auto buffer_span = tcb::span<const uint8_t>(
    static_cast<const uint8_t*>(storage.data()),
    storage.size());

  xcdr_buffers::XCdrLayoutParser parser(buffer_span);
  if (nullptr == impl || nullptr == impl->parse_fields) {
    RCUTILS_SET_ERROR_MSG("parse_fields callback not available");
    return RCUTILS_RET_ERROR;
  }
  auto _parse_ret = impl->parse_fields(parser);
  if (RCUTILS_RET_OK != _parse_ret) {
    return _parse_ret;
  }

  auto layout_result = parser.finalize();
  if (!layout_result) {
    // Error:("Failed to parse layout from buffer");
    return RCUTILS_RET_ERROR;
  }

  // 2. Create accessor from parsed layout
  auto accessor_result = xcdr_buffers::XCdrConstAccessor::wrap(buffer_span, *layout_result);
  if (!accessor_result) {
    // Error:("Failed to create accessor from parsed layout");
    return RCUTILS_RET_ERROR;
  }
  auto accessor = *accessor_result;

  // 3. Build external storage from accessor
  @(full_msg_typename)::ExternalStorage ext_storage;
  ext_storage.block = rosidl_runtime_cpp::MemoryRegion<void>(
    const_cast<void*>(storage.data()),
    storage.size());

  // Populate external storage from accessor
  {
    auto _populate_ret = populate_external_storage_@(msg_typename)(accessor, &ext_storage);
    if (_populate_ret != RCUTILS_RET_OK) { return _populate_ret; }
  }

  // 4. Mark external storage as prepopulated (data already in buffer)
  ext_storage.prepopulated = true;

  // 5. Construct message from external storage
  auto * msg = new @(full_msg_typename)(ext_storage, rosidl_runtime_cpp::MessageInitialization::SKIP);

  // 6. Validate against constraints if the handle carries owned constraint state.
  // When rmw creates a per-loan constrained handle, owned_constraints is set
  // to enforce upper bounds during the cast path.
  if (impl && impl->owned_constraints && impl->owned_constraints->type_specific) {
    if (RCUTILS_RET_OK != impl->validate_fields(
        impl->owned_constraints->type_specific, msg, nullptr, nullptr))
    {
      delete msg;
      return RCUTILS_RET_ERROR;
    }
  }

  *message_ptr = msg;
  return RCUTILS_RET_OK;
}

// Destroy message created by construct_at or cast_at (C-linkage compatible)
extern "C" void
destroy_message_@(msg_typename)(void * message_ptr)
{
  delete static_cast<@(full_msg_typename) *>(message_ptr);
}

// Release message and recover external storage (C-compatible return type)
extern "C" rosidl_memory_region_t
release_message_@(msg_typename)(void * message_ptr)
{
  auto * msg = static_cast<@(full_msg_typename) *>(message_ptr);

  // Extract storage before destruction
  rosidl_memory_region_t region{{nullptr, 0}, 0};
  if (msg->_external_storage.has_value()) {
    auto & block = msg->_external_storage.value().block;
    region.location.address = block.data();
    region.size = block.size();
    region.location.attributes = block.attributes();
  }

  // Destroy message
  delete msg;

  return region;
}

// Return backing storage without destroying the message (C-compatible return type)
extern "C" rosidl_memory_region_t
get_backing_storage_@(msg_typename)(const void * message_ptr)
{
  // Non-destructive read — we only extract the block pointer without mutating.
  auto * msg = static_cast<const @(full_msg_typename) *>(message_ptr);

  rosidl_memory_region_t region{{nullptr, 0}, 0};
  if (msg->_external_storage.has_value()) {
    const auto & block = msg->_external_storage.value().block;
    region.location.address = const_cast<void *>(block.data());
    region.size = block.size();
    region.location.attributes = block.attributes();
  }
  // For inline-only messages (no external storage), return message pointer
  // with size 0 as a zero-length marker.
  if (nullptr == region.location.address) {
    region.location.address = const_cast<void *>(message_ptr);
    region.size = 0;
  }
  return region;
}

// Compute serialized size (with external-storage fast path)
rcutils_ret_t
compute_serialized_size_@(msg_typename)(
  const void * message_ptr,
  size_t * size)
{
  if (nullptr == message_ptr || nullptr == size) {
    return RCUTILS_RET_ERROR;
  }
  auto & msg = *static_cast<const @(full_msg_typename) *>(message_ptr);

  // Fast path: external storage already knows the size.
  if (msg._external_storage.has_value()) {
    *size = msg._external_storage.value().block.size();
    return RCUTILS_RET_OK;
  }

  // Compute size with XCDR alignment.
  size_t data_offset = 0;
@[for member in message.structure.members]@
@(generate_size_calculation(member, is_experimental))
@[end for]@
  *size = xcdr_buffers::kXCdrHeaderSize + data_offset;
  return RCUTILS_RET_OK;
}

@[  if has_constraints]@
// Compare type-specific constraints using generated CheckCompatible API.
extern "C" bool
compare_type_specific_constraints_@(msg_typename)(
  const void * lhs,
  const void * rhs)
{
  if (nullptr == lhs || nullptr == rhs) {
    return false;
  }
  auto & candidate = *static_cast<const @(full_msg_typename)::Constraints *>(lhs);
  auto & baseline = *static_cast<const @(full_msg_typename)::Constraints *>(rhs);

  return candidate.CheckCompatible(baseline);
}

// Forward declaration for serialize function defined below.
rcutils_ret_t
serialize_fields_into_writer_@(msg_typename)(
  const void * message_ptr,
  xcdr_buffers::XCdrWriter & writer);

// Validate a message instance against type-specific constraints.
extern "C" rcutils_ret_t
validate_message_@(msg_typename)(
  const void * type_specific,
  const void * message_ptr,
  rosidl_typesupport_xcdr_c_constraint_report_callback_t report_cb,
  void * user_data)
{
  if (nullptr == type_specific || nullptr == message_ptr) {
    return RCUTILS_RET_ERROR;
  }
  auto & constraints = *static_cast<const @(full_msg_typename)::Constraints *>(type_specific);
  auto & msg = *static_cast<const @(full_msg_typename) *>(message_ptr);

  // Bridge C callback + user_data into a std::function for CheckCompatible.
  rosidl_runtime_cpp::ConstraintReportCallback cb = nullptr;
  if (report_cb) {
    // The path views produced by CheckCompatible always originate from
    // std::string data, so they are guaranteed null-terminated.  Passing
    // .data() directly is safe.
    cb = [report_cb, user_data](std::string_view path, int reason_code) -> void {
      report_cb(user_data, path.data(), reason_code);
    };
  }
  return constraints.CheckCompatible(msg, cb)
    ? RCUTILS_RET_OK : RCUTILS_RET_ERROR;
}

// Clone full constraints (blanket + type-specific) into handle-owned storage.
extern "C" std::shared_ptr<rosidl_message_type_constraints_t>
clone_constraints_@(msg_typename)(
  const rosidl_message_type_constraints_t * src)
{
  if (nullptr == src) {
    return nullptr;
  }
  // Deep-copy type_specific into a typed clone.
  auto * ts = new @(full_msg_typename)::Constraints(
    *static_cast<const @(full_msg_typename)::Constraints *>(src->type_specific));
  auto * clone = new rosidl_message_type_constraints_t();
  clone->type_specific = ts;
  clone->max_string_length = src->max_string_length;
  clone->max_total_size = src->max_total_size;
  clone->strict = src->strict;
  return std::shared_ptr<rosidl_message_type_constraints_t>(
    clone,
    [](rosidl_message_type_constraints_t * p) {
      delete static_cast<@(full_msg_typename)::Constraints *>(p->type_specific);
      delete p;
    });
}

// ============================================================================
// compact_fields — internal recursive workhorse
//
// Single-pass traversal: while emit==false, skip fields that match their
// constraint maximum and set emit=true on the first undersized field.
// After emit==true, serialize all remaining fields through the writer.
// Returns error if any field violates its constraint bound.
// ============================================================================

rcutils_ret_t
compact_fields_@(msg_typename)(
  const void * untyped_msg,
  const xcdr_buffers::XCdrStructLayout & layout,
  xcdr_buffers::XCdrWriter & writer,
  bool & emit)
{
  if (nullptr == untyped_msg) {
    return RCUTILS_RET_ERROR;
  }
  auto & msg = *static_cast<const @(full_msg_typename) *>(untyped_msg);
  (void)layout;
@[for i, member in enumerate(message.structure.members)]@
@[  if isinstance(member.type, BasicType)]@
@# Primitive field: no constraint check needed
  if (emit) {
    writer.write<@(get_cpp_type(member.type))>(msg.@(member.name));
  } else {
    writer.skip<@(get_cpp_type(member.type))>();
  }

@[  elif isinstance(member.type, AbstractString)]@
@[    if isinstance(member.type, BoundedString)]@
@# Bounded string: always at max, just skip or write
  if (emit) {
    writer.write(msg.@(member.name).view());
  } else {
    writer.skip_string(msg.@(member.name).size());
  }

@[    else]@
@# Unbounded string: check bound from layout
  {
    auto _actual_sz = msg.@(member.name).size();
    auto _bound = std::get<xcdr_buffers::XCdrStringLayout>(
      layout.get_member(@(i))->get().layout()).actual_length();
    if (_actual_sz > _bound) { return RCUTILS_RET_ERROR; }
    if (!emit && _actual_sz < _bound) { emit = true; }
    if (emit) {
      writer.write(std::string_view(msg.@(member.name).data(), msg.@(member.name).size()));
    } else {
      writer.skip_string(_actual_sz);
    }
  }

@[    end if]@
@[  elif isinstance(member.type, AbstractWString)]@
@[    if isinstance(member.type, BoundedWString)]@
@# Bounded wstring: always at max
  if (emit) {
    writer.write(msg.@(member.name).view());
  } else {
    writer.skip_wstring(msg.@(member.name).size());
  }

@[    else]@
@# Unbounded wstring: check bound from layout
  {
    auto _actual_sz = msg.@(member.name).size();
    auto _bound = std::get<xcdr_buffers::XCdrStringLayout>(
      layout.get_member(@(i))->get().layout()).actual_length();
    if (_actual_sz > _bound) { return RCUTILS_RET_ERROR; }
    if (!emit && _actual_sz < _bound) { emit = true; }
    if (emit) {
      writer.write(std::u16string_view(msg.@(member.name).data(), msg.@(member.name).size()));
    } else {
      writer.skip_wstring(_actual_sz);
    }
  }

@[    end if]@
@[  elif isinstance(member.type, Array)]@
@# Array: fixed size, no constraint check; skip or write elements
@[    if isinstance(member.type.value_type, BasicType)]@
@# Array of primitives
  if (emit) {
    writer.write_array(tcb::span<const @(get_cpp_type(member.type.value_type))>(msg.@(member.name).data(), @(member.type.size)));
  } else {
    writer.skip_array<@(get_cpp_type(member.type.value_type))>(@(member.type.size));
  }

@[    elif isinstance(member.type.value_type, AbstractString)]@
@# Array of strings
  if (emit) {
    for (const auto & _elem : msg.@(member.name)) {
      writer.write(_elem.view());
    }
  } else {
    for (const auto & _elem : msg.@(member.name)) {
      writer.skip_string(_elem.size());
    }
  }

@[    elif isinstance(member.type.value_type, AbstractWString)]@
@# Array of wstrings
  if (emit) {
    for (const auto & _elem : msg.@(member.name)) {
      writer.write(_elem.view());
    }
  } else {
    for (const auto & _elem : msg.@(member.name)) {
      writer.skip_wstring(_elem.size());
    }
  }

@[    elif isinstance(member.type.value_type, NamespacedType)]@
@# Array of nested messages
@[      if 'experimental' in member.type.value_type.namespaces]@
@{
nested_ns_arr = '::'.join(member.type.value_type.namespaces)
nested_short_arr = member.type.value_type.name
nested_ts_name_arr = get_message_type_name(member.type.value_type, experimental_context=is_experimental)
}@
  {
    auto _nested_ts_arr = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(nested_ts_name_arr)>();
    auto _nested_outer_arr = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts_arr->data);
    auto _nested_inner_arr = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(_nested_outer_arr->inner);
    const auto & _arr_layout = std::get<xcdr_buffers::XCdrStructLayout>(
      layout.get_member(@(i))->get().layout());
    for (size_t _j = 0; _j < @(member.type.size); ++_j) {
      const auto & _elem_layout = std::get<xcdr_buffers::XCdrStructLayout>(
        _arr_layout.element_layout(_j)->get());
      if (emit) {
        _nested_inner_arr->serialize_fields(&msg.@(member.name)[_j], writer);
      } else {
        auto _ret = _nested_inner_arr->compact_fields_recursive(
          &msg.@(member.name)[_j], _elem_layout, writer, emit);
        if (_ret != RCUTILS_RET_OK) { return _ret; }
      }
    }
  }

@[      else]@
@# Non-experimental nested in array: no savings possible, just serialize
  {
    auto _nested_ts_arr2 = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto _nested_outer_arr2 = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts_arr2->data);
    auto _nested_inner_arr2 = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(_nested_outer_arr2->inner);
    for (const auto & _elem : msg.@(member.name)) {
      _nested_inner_arr2->serialize_fields(&_elem, writer);
    }
  }

@[      end if]@
@[    end if]@

@[  elif isinstance(member.type, AbstractSequence)]@
@# Sequence: may have constraint check on count
@[    if isinstance(member.type, BoundedSequence)]@
@# Bounded sequence: count is always at max, no constraint check
@[    else]@
@# Unbounded sequence: check count from layout
@[    if isinstance(member.type.value_type, BasicType)]@
  {
    size_t _actual_cnt = msg.@(member.name).size();
    size_t _bound = std::get<xcdr_buffers::XCdrPrimitiveSequenceLayout>(
      layout.get_member(@(i))->get().layout()).actual_count();
    if (_actual_cnt > _bound) { return RCUTILS_RET_ERROR; }
    if (!emit && _actual_cnt < _bound) { emit = true; }
  }
@[    else]@
  {
    size_t _actual_cnt = msg.@(member.name).size();
    size_t _bound = std::get<xcdr_buffers::XCdrSequenceLayout>(
      layout.get_member(@(i))->get().layout()).actual_count();
    if (_actual_cnt > _bound) { return RCUTILS_RET_ERROR; }
    if (!emit && _actual_cnt < _bound) { emit = true; }
  }
@[    end if]@
@[    end if]@
@[    if isinstance(member.type.value_type, BasicType)]@
@# Primitive sequence
  if (emit) {
@[      if get_cpp_type(member.type.value_type) == 'bool']@
    writer.begin_write_sequence(msg.@(member.name).size());
    for (const auto & _elem : msg.@(member.name)) {
      writer.write(static_cast<uint8_t>(_elem));
    }
    writer.end_write_sequence();
@[      else]@
    writer.write_sequence(tcb::span<const @(get_cpp_type(member.type.value_type))>(
      msg.@(member.name).data(), msg.@(member.name).size()));
@[      end if]@
  } else {
    writer.skip_sequence<@(get_cpp_type(member.type.value_type))>(msg.@(member.name).size());
  }

@[    elif isinstance(member.type.value_type, AbstractString)]@
@# Sequence of strings
  if (emit) {
    writer.begin_write_sequence(msg.@(member.name).size());
    for (const auto & _elem : msg.@(member.name)) {
      writer.write(_elem.view());
    }
    writer.end_write_sequence();
  } else {
    writer.begin_skip_sequence(msg.@(member.name).size());
    for (const auto & _elem : msg.@(member.name)) {
      writer.skip_string(_elem.size());
    }
    writer.end_write_sequence();
  }

@[    elif isinstance(member.type.value_type, AbstractWString)]@
@# Sequence of wstrings
  if (emit) {
    writer.begin_write_sequence(msg.@(member.name).size());
    for (const auto & _elem : msg.@(member.name)) {
      writer.write(_elem.view());
    }
    writer.end_write_sequence();
  } else {
    writer.begin_skip_sequence(msg.@(member.name).size());
    for (const auto & _elem : msg.@(member.name)) {
      writer.skip_wstring(_elem.size());
    }
    writer.end_write_sequence();
  }

@[    elif isinstance(member.type.value_type, NamespacedType)]@
@# Sequence of nested messages
@[      if 'experimental' in member.type.value_type.namespaces]@
@{
nested_ns_seq = '::'.join(member.type.value_type.namespaces)
nested_short_seq = member.type.value_type.name
nested_ts_name_seq = get_message_type_name(member.type.value_type, experimental_context=is_experimental)
}@
  {
    auto _nested_ts_seq = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(nested_ts_name_seq)>();
    auto _nested_outer_seq = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts_seq->data);
    auto _nested_inner_seq = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(_nested_outer_seq->inner);
    const auto & _seq_layout = std::get<xcdr_buffers::XCdrSequenceLayout>(
      layout.get_member(@(i))->get().layout());
    if (emit) {
      writer.begin_write_sequence(msg.@(member.name).size());
      for (const auto & _elem : msg.@(member.name)) {
        _nested_inner_seq->serialize_fields(&_elem, writer);
      }
      writer.end_write_sequence();
    } else {
      writer.begin_skip_sequence(msg.@(member.name).size());
      for (size_t _sj = 0; _sj < msg.@(member.name).size(); ++_sj) {
        const auto & _elem_layout = std::get<xcdr_buffers::XCdrStructLayout>(
          _seq_layout.element_layout(_sj)->get());
        auto _ret = _nested_inner_seq->compact_fields_recursive(
          &msg.@(member.name)[_sj], _elem_layout, writer, emit);
        if (_ret != RCUTILS_RET_OK) { return _ret; }
      }
      writer.end_write_sequence();
    }
  }

@[      else]@
@# Non-experimental nested in sequence: no savings possible, always serialize
  {
    auto _nested_ts_seq2 = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type, experimental_context=is_experimental))>();
    auto _nested_outer_seq2 = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts_seq2->data);
    auto _nested_inner_seq2 = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(_nested_outer_seq2->inner);
    writer.begin_write_sequence(msg.@(member.name).size());
    for (const auto & _elem : msg.@(member.name)) {
      _nested_inner_seq2->serialize_fields(&_elem, writer);
    }
    writer.end_write_sequence();
  }

@[      end if]@
@[    end if]@

@[  elif isinstance(member.type, NamespacedType)]@
@# Nested message member
@[    if 'experimental' in member.type.namespaces]@
@{
nested_ns = '::'.join(member.type.namespaces)
nested_short = member.type.name
nested_ts_name = get_message_type_name(member.type, experimental_context=is_experimental)
}@
  {
    const auto & _nested_layout = std::get<xcdr_buffers::XCdrStructLayout>(
      layout.get_member(@(i))->get().layout());
    auto _nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(nested_ts_name)>();
    auto _nested_outer = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts->data);
    auto _nested_inner = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(_nested_outer->inner);
    if (emit) {
      _nested_inner->serialize_fields(&msg.@(member.name), writer);
    } else {
      auto _ret = _nested_inner->compact_fields_recursive(
        &msg.@(member.name), _nested_layout, writer, emit);
      if (_ret != RCUTILS_RET_OK) { return _ret; }
    }
  }

@[    else]@
@# Non-experimental nested: no savings possible (all fixed-size)
  {
    auto _nested_ts3 = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type, experimental_context=is_experimental))>();
    auto _nested_outer3 = static_cast<const rosidl_message_xcdr_type_support_t *>(_nested_ts3->data);
    auto _nested_inner3 = static_cast<const rosidl_typesupport_xcdr_cpp::rosidl_message_xcdr_cpp_type_support_t *>(_nested_outer3->inner);
    if (emit) {
      _nested_inner3->serialize_fields(&msg.@(member.name), writer);
    } else {
      // Fixed-size nested — no compaction possible; skip the whole struct
      _nested_inner3->serialize_fields(&msg.@(member.name), writer);
    }
  }

@[    end if]@
@[  end if]@

@[end for]@
  return RCUTILS_RET_OK;
}

// ============================================================================
// compact_message — top-level consume-and-compact callback (layout-driven)
//
// Uses cached layout from the constrained handle for per-field maximum bounds.
// Creates a writer at the header offset, runs compact_fields with emit=false,
// then either release_message (if emit stays false) or consumes the message
// and returns the compacted region.
// ============================================================================

extern "C" rosidl_memory_region_t
compact_message_@(msg_typename)(
  void * message_ptr,
  const xcdr_buffers::XCdrStructLayout * cached_layout)
{
  rosidl_memory_region_t null_region = {{nullptr, 0}, 0};

  if (nullptr == message_ptr) {
    RCUTILS_SET_ERROR_MSG("message_ptr is nullptr");
    return null_region;
  }

  auto & msg = *static_cast<@(full_msg_typename) *>(message_ptr);

  // Get backing buffer from external storage.
  if (!msg._external_storage.has_value()) {
    RCUTILS_SET_ERROR_MSG("message has no external storage to compact");
    return null_region;
  }
  auto & block = msg._external_storage.value().block;

  auto buffer_span = tcb::span<uint8_t>(
    static_cast<uint8_t *>(const_cast<void *>(block.data())), block.size());
  constexpr size_t kHeaderSize = xcdr_buffers::kXCdrHeaderSize;
  if (buffer_span.size() <= kHeaderSize) {
    RCUTILS_SET_ERROR_MSG("backing buffer too small for CDR header");
    return null_region;
  }

  // The constrained handle must carry a cached layout.
  if (nullptr == cached_layout) {
    RCUTILS_SET_ERROR_MSG("cached_layout not available for compaction");
    return null_region;
  }

  // Single-pass traversal: start with emit=false, one writer.
  xcdr_buffers::XCdrWriter writer(buffer_span, kHeaderSize);
  bool emit = false;

  auto ret = compact_fields_@(msg_typename)(&msg, *cached_layout, writer, emit);
  if (ret != RCUTILS_RET_OK) {
    // Layout-bound violation — message is NOT consumed.
    return null_region;
  }

  if (!emit) {
    // All fields at constraint maximum — fast path: release existing blob.
    return release_message_@(msg_typename)(message_ptr);
  }

  // Rewrite path: compacted data was written into the buffer.
  if (writer.has_error()) {
    RCUTILS_SET_ERROR_MSG("XCdrWriter overflow during compaction rewrite");
    return null_region;
  }

  void * blob = const_cast<void *>(block.data());
  size_t compacted_size = writer.bytes_written();
  delete &msg;

  rosidl_memory_region_t result = {{blob, 0}, compacted_size};
  return result;
}
@[  end if]@

}  // namespace @(msg_namespace)

@[else]@
// ========== NON-EXPERIMENTAL MESSAGE (@(full_msg_typename)) ==========

namespace @(msg_namespace)
{

// Private: Serialize message fields into existing writer (no XCDR header)
rcutils_ret_t
serialize_fields_into_writer_@(msg_typename)(
  const void * message_ptr,
  xcdr_buffers::XCdrWriter & writer)
{
  auto & msg = *static_cast<const @(full_msg_typename) *>(message_ptr);
@[  for member in message.structure.members]@
@(generate_writer_field(member, is_experimental, 'msg'))
@[  end for]@

  return RCUTILS_RET_OK;
}

// Private: Deserialize message fields from existing reader (no XCDR header)
rcutils_ret_t
deserialize_fields_from_reader_@(msg_typename)(
  xcdr_buffers::XCdrReader & reader,
  void * message_ptr)
{
  auto & msg = *static_cast<@(full_msg_typename) *>(message_ptr);
@[  for member in message.structure.members]@
@(generate_reader_field(member, is_experimental, 'msg'))
@[  end for]@

  return RCUTILS_RET_OK;
}

// Private: Build layout fields into existing builder (not used for non-experimental)
rcutils_ret_t
build_layout_fields_@(msg_typename)(
  xcdr_buffers::XCdrLayoutBuilder & builder,
  const void * constraints_ptr)
{
  (void)builder;
  (void)constraints_ptr;
  // Non-experimental messages don't use layouts
  return RCUTILS_RET_ERROR;
}

// Compute serialized size
rcutils_ret_t
compute_serialized_size_@(msg_typename)(
  const void * message_ptr,
  size_t * size)
{
  if (nullptr == message_ptr || nullptr == size) {
    return RCUTILS_RET_ERROR;
  }
  auto & msg = *static_cast<const @(full_msg_typename) *>(message_ptr);
  (void)msg;

  size_t data_offset = 0;
@[for member in message.structure.members]@
@(generate_size_calculation(member, is_experimental))
@[end for]@
  *size = xcdr_buffers::kXCdrHeaderSize + data_offset;
  return RCUTILS_RET_OK;
}

}  // namespace @(msg_namespace)

@[end if]@

// Template specialization with bundled static initialization
namespace rosidl_typesupport_xcdr_cpp
{

namespace
{
@[if is_experimental]@
@[  if has_constraints]@
/// Build constrained layout and return the cached layout.
inline const rosidl_message_xcdr_cpp_type_support_t & get_inner_@(msg_typename)()
{
  static const auto inner = []() {
    auto tmp = rosidl_message_xcdr_cpp_type_support_t{};
    tmp.build_constrained = &@(msg_namespace)::build_layout_@(msg_typename);
    tmp.clone_constraints = &@(msg_namespace)::clone_constraints_@(msg_typename);
    tmp.build_layout_fields = &@(msg_namespace)::build_layout_fields_@(msg_typename);
    tmp.serialize_fields = &@(msg_namespace)::serialize_fields_into_writer_@(msg_typename);
    tmp.deserialize_fields = &@(msg_namespace)::deserialize_fields_from_reader_@(msg_typename);
    tmp.parse_fields = &@(msg_namespace)::parse_fields_@(msg_typename);
    tmp.construct_message = [](const rosidl_message_xcdr_cpp_type_support_t * impl,
                                rosidl_runtime_cpp::MemoryRegion<void> & s, void ** m) {
      return @(msg_namespace)::construct_message_at_@(msg_typename)(impl, s, m);
    };
    tmp.cast_message = [](const rosidl_message_xcdr_cpp_type_support_t * impl,
                           rosidl_runtime_cpp::MemoryRegion<void> s, void ** m) {
      return @(msg_namespace)::cast_message_at_@(msg_typename)(impl, s, m);
    };
    tmp.compute_serialized_size = &@(msg_namespace)::compute_serialized_size_@(msg_typename);
    tmp.validate_fields = &@(msg_namespace)::validate_message_@(msg_typename);
    tmp.compact_fields = &@(msg_namespace)::compact_message_@(msg_typename);
    tmp.compact_fields_recursive = &@(msg_namespace)::compact_fields_@(msg_typename);
    return tmp;
  }();
  return inner;
}
@[  else]@
/// Fully bounded: return the singleton inner struct.
inline const rosidl_message_xcdr_cpp_type_support_t & get_inner_@(msg_typename)()
{
  static const auto inner = []() {
    auto tmp = rosidl_message_xcdr_cpp_type_support_t{};
    tmp.cached_layout = @(msg_namespace)::get_layout_@(msg_typename)();
    tmp.build_layout_fields = &@(msg_namespace)::build_layout_fields_@(msg_typename);
    tmp.serialize_fields = &@(msg_namespace)::serialize_fields_into_writer_@(msg_typename);
    tmp.deserialize_fields = &@(msg_namespace)::deserialize_fields_from_reader_@(msg_typename);
    tmp.parse_fields = &@(msg_namespace)::parse_fields_@(msg_typename);
    tmp.construct_message = [](const rosidl_message_xcdr_cpp_type_support_t * impl,
                                rosidl_runtime_cpp::MemoryRegion<void> & s, void ** m) {
      return @(msg_namespace)::construct_message_at_@(msg_typename)(impl, s, m);
    };
    tmp.cast_message = [](const rosidl_message_xcdr_cpp_type_support_t * impl,
                           rosidl_runtime_cpp::MemoryRegion<void> s, void ** m) {
      return @(msg_namespace)::cast_message_at_@(msg_typename)(impl, s, m);
    };
    tmp.compute_serialized_size = &@(msg_namespace)::compute_serialized_size_@(msg_typename);
    tmp.compact_fields = &@(msg_namespace)::compact_message_@(msg_typename);
    tmp.compact_fields_recursive = &@(msg_namespace)::compact_fields_@(msg_typename);
    return tmp;
  }();
  return inner;
}
@[  end if]@
@[else]@
/// Non-experimental: inner struct with field callbacks only.
inline const rosidl_message_xcdr_cpp_type_support_t & get_inner_@(msg_typename)()
{
  static const auto inner = []() {
    auto tmp = rosidl_message_xcdr_cpp_type_support_t{};
    tmp.build_layout_fields = &@(msg_namespace)::build_layout_fields_@(msg_typename);
    tmp.serialize_fields = &@(msg_namespace)::serialize_fields_into_writer_@(msg_typename);
    tmp.deserialize_fields = &@(msg_namespace)::deserialize_fields_from_reader_@(msg_typename);
    tmp.compute_serialized_size = &@(msg_namespace)::compute_serialized_size_@(msg_typename);
    return tmp;
  }();
  return inner;
}
@[end if]@
}  // anonymous namespace

template<>
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_message_type_support_t *
get_message_type_support_handle<@(full_msg_typename)>()
{
  // Thread-safe lazy initialization (C++11 guarantees this for function-local statics)
  static const auto & inner = get_inner_@(msg_typename)();

  static const rosidl_message_xcdr_type_support_t outer = []() {
    auto tmp = *rosidl_typesupport_xcdr_cpp::get_xcdr_cpp_type_support_prototype();
    tmp.inner = const_cast<rosidl_message_xcdr_cpp_type_support_t *>(&inner);
    tmp.message_namespace = "@(msg_namespace)";
    tmp.message_name = "@(msg_typename)";
@[if is_experimental]@
    tmp.destroy_message = &@(msg_namespace)::destroy_message_@(msg_typename);
    tmp.release_message = &@(msg_namespace)::release_message_@(msg_typename);
    tmp.get_backing_storage = &@(msg_namespace)::get_backing_storage_@(msg_typename);
@[  if has_constraints]@
    tmp.compare_type_specific_constraints = &@(msg_namespace)::compare_type_specific_constraints_@(msg_typename);
@[  end if]@
@[end if]@
    return tmp;
  }();

  static const rosidl_message_type_support_t handle = {
    rosidl_typesupport_xcdr_cpp__identifier,
    &outer,
    get_message_typesupport_handle_function,
    xcdr_default_get_type_hash,            // safe fallback (returns zero hash)
    xcdr_default_get_type_description,     // safe fallback (returns nullptr)
    xcdr_default_get_type_description_sources,  // safe fallback (returns nullptr)
  };

  return &handle;
}

}  // namespace rosidl_typesupport_xcdr_cpp

// C symbol export
#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpp,
  @(', '.join([package_name] + effective_parent_parts)),
  @(msg_typename))()
{
  return rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(full_msg_typename)>();
}

#ifdef __cplusplus
}
#endif
