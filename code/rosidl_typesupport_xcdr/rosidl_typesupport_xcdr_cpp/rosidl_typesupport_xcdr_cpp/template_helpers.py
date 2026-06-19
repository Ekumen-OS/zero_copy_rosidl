# Copyright 2024 Ekumen Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Helper functions for rosidl_typesupport_xcdr_cpp templates."""

from rosidl_parser.definition import (
    AbstractSequence,
    AbstractString,
    AbstractWString,
    Array,
    BasicType,
    BoundedSequence,
    BoundedString,
    BoundedWString,
    NamespacedType,
)


def get_xcdr_primitive_kind(basic_type):
    """Map BasicType to XCdr primitive kind."""
    type_map = {
        'boolean': 'kBoolean',
        'octet': 'kUint8',
        'uint8': 'kUint8',
        'int8': 'kInt8',
        'uint16': 'kUint16',
        'int16': 'kInt16',
        'uint32': 'kUint32',
        'int32': 'kInt32',
        'uint64': 'kUint64',
        'int64': 'kInt64',
        'float': 'kFloat',
        'double': 'kDouble',
        'long double': 'kLongDouble',
        'char': 'kChar8',
        'wchar': 'kChar16',
    }
    return 'xcdr_buffers::XCdrPrimitiveKind::' + type_map.get(basic_type.typename, 'kUint8')


def get_cpp_type(basic_type):
    """Map BasicType to C++ type."""
    type_map = {
        'boolean': 'bool',
        'octet': 'uint8_t',
        'uint8': 'uint8_t',
        'int8': 'int8_t',
        'uint16': 'uint16_t',
        'int16': 'int16_t',
        'uint32': 'uint32_t',
        'int32': 'int32_t',
        'uint64': 'uint64_t',
        'int64': 'int64_t',
        'float': 'float',
        'double': 'double',
        'long double': 'long double',
        'char': 'char',
        'wchar': 'char16_t',
    }
    return type_map.get(basic_type.typename, 'uint8_t')


def needs_constraints(member_type):
    """Check if message has variable-length fields (needs constraints)."""
    if isinstance(member_type, (AbstractString, AbstractWString)):
        if not isinstance(member_type, (BoundedString, BoundedWString)):
            return True
    if isinstance(member_type, AbstractSequence):
        if not isinstance(member_type, BoundedSequence):
            return True
        # Check element type recursively
        return needs_constraints(member_type.value_type)
    if isinstance(member_type, Array):
        return needs_constraints(member_type.value_type)
    if isinstance(member_type, NamespacedType):
        # Nested messages might have constraints - be conservative
        return True
    return False


def generate_layout_field(member, constraints_prefix='constraints'):
    """Generate layout builder code for a single field."""
    member_name = member.name
    member_type = member.type

    if isinstance(member_type, BasicType):
        return f'  builder.allocate_primitive("{member_name}", {get_xcdr_primitive_kind(member_type)});'

    elif isinstance(member_type, (BoundedString, BoundedWString)):
        max_size = member_type.maximum_size
        return f'  builder.allocate_string("{member_name}", {max_size});'

    elif isinstance(member_type, (AbstractString, AbstractWString)):
        return f'  builder.allocate_string("{member_name}", {constraints_prefix}.{member_name}.size);'

    elif isinstance(member_type, Array):
        if isinstance(member_type.value_type, BasicType):
            return f'  builder.allocate_primitive_array("{member_name}", {get_xcdr_primitive_kind(member_type.value_type)}, {member_type.size});'
        else:
            lines = [f'  builder.begin_allocate_array("{member_name}", {member_type.size});']
            # Generate code for element type
            if isinstance(member_type.value_type, (AbstractString, AbstractWString)):
                if isinstance(member_type.value_type, (BoundedString, BoundedWString)):
                    lines.append(f'    builder.allocate_string({member_type.value_type.maximum_size});')
                else:
                    lines.append(f'    builder.allocate_string({constraints_prefix}.{member_name}.size);')
            elif isinstance(member_type.value_type, NamespacedType):
                # Nested message array element
                msg_type = get_message_type_name(member_type.value_type)
                lines.append('    builder.begin_allocate_struct();')
                lines.append('    {')
                lines.append(f'      auto nested_ts_{member_name} = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
                lines.append(f'      auto nested_callbacks_{member_name} = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_{member_name}->data);')
                lines.append(f'      nested_callbacks_{member_name}->build_layout_fields(builder, nullptr);')
                lines.append('    }')
                lines.append('    builder.end_allocate_struct();')
            lines.append('  builder.end_allocate_array();')
            return '\n'.join(lines)

    elif isinstance(member_type, BoundedSequence):
        if isinstance(member_type.value_type, BasicType):
            return f'  builder.allocate_primitive_sequence("{member_name}", {get_xcdr_primitive_kind(member_type.value_type)}, {constraints_prefix}.{member_name}.size);'
        else:
            lines = [f'  builder.begin_allocate_sequence("{member_name}", {constraints_prefix}.{member_name}.size);']
            if isinstance(member_type.value_type, (AbstractString, AbstractWString)):
                if isinstance(member_type.value_type, (BoundedString, BoundedWString)):
                    lines.append(f'    builder.allocate_string({member_type.value_type.maximum_size});')
                else:
                    lines.append(f'    builder.allocate_string({constraints_prefix}.{member_name}.element.size);')
            elif isinstance(member_type.value_type, NamespacedType):
                # Nested message sequence element
                msg_type = get_message_type_name(member_type.value_type)
                lines.append('    builder.begin_allocate_struct();')
                lines.append('    {')
                lines.append(f'      auto nested_ts_{member_name} = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
                lines.append(f'      auto nested_callbacks_{member_name} = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_{member_name}->data);')
                lines.append(f'      nested_callbacks_{member_name}->build_layout_fields(builder, nullptr);')
                lines.append('    }')
                lines.append('    builder.end_allocate_struct();')
            lines.append('  builder.end_allocate_sequence();')
            return '\n'.join(lines)

    elif isinstance(member_type, AbstractSequence):
        if isinstance(member_type.value_type, BasicType):
            return f'  builder.allocate_primitive_sequence("{member_name}", {get_xcdr_primitive_kind(member_type.value_type)}, {constraints_prefix}.{member_name}.size);'
        else:
            lines = [f'  builder.begin_allocate_sequence("{member_name}", {constraints_prefix}.{member_name}.size);']
            if isinstance(member_type.value_type, (AbstractString, AbstractWString)):
                if isinstance(member_type.value_type, (BoundedString, BoundedWString)):
                    lines.append(f'    builder.allocate_string({member_type.value_type.maximum_size});')
                else:
                    lines.append(f'    builder.allocate_string({constraints_prefix}.{member_name}.element.size);')
            elif isinstance(member_type.value_type, NamespacedType):
                # Nested message sequence element
                msg_type = get_message_type_name(member_type.value_type)
                lines.append('    builder.begin_allocate_struct();')
                lines.append('    {')
                lines.append(f'      auto nested_ts_{member_name} = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
                lines.append(f'      auto nested_callbacks_{member_name} = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_{member_name}->data);')
                lines.append(f'      nested_callbacks_{member_name}->build_layout_fields(builder, nullptr);')
                lines.append('    }')
                lines.append('    builder.end_allocate_struct();')
            lines.append('  builder.end_allocate_sequence();')
            return '\n'.join(lines)

    elif isinstance(member_type, NamespacedType):
        # Nested message: use build_layout_fields callback for recursion
        msg_type = get_message_type_name(member_type)
        lines = [f'  // Nested message: {member_name}']
        lines.append('  builder.begin_allocate_struct();')
        lines.append('  {')
        lines.append(f'    auto nested_ts_{member_name} = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
        lines.append(f'    auto nested_callbacks_{member_name} = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_{member_name}->data);')
        # Check if nested message has constraints
        if_stmt = f'    if ({constraints_prefix}.{member_name}) {{'
        lines.append(if_stmt)
        lines.append(f'      nested_callbacks_{member_name}->build_layout_fields(builder, {constraints_prefix}.{member_name});')
        lines.append('    } else {')
        lines.append(f'      nested_callbacks_{member_name}->build_layout_fields(builder, nullptr);')
        lines.append('    }')
        lines.append('  }')
        lines.append('  builder.end_allocate_struct();')
        return '\n'.join(lines)

    return f'  // TODO: Handle {member_name} of type {member_type}'


def generate_parser_field(member):
    """Generate layout parser code for a single field."""
    member_type = member.type

    if isinstance(member_type, BasicType):
        return f'  parser.parse_primitive({get_xcdr_primitive_kind(member_type)});'

    elif isinstance(member_type, (AbstractString, AbstractWString)):
        return f'  parser.parse_string();'

    elif isinstance(member_type, Array):
        if isinstance(member_type.value_type, BasicType):
            return f'  parser.parse_primitive_array({get_xcdr_primitive_kind(member_type.value_type)}, {member_type.size});'
        else:
            lines = [f'  parser.begin_parse_array({member_type.size});']
            if isinstance(member_type.value_type, (AbstractString, AbstractWString)):
                lines.append('    parser.parse_string();')
            elif isinstance(member_type.value_type, NamespacedType):
                lines.append('    parser.begin_parse_struct();')
                lines.append('    parser.end_parse_struct();')
            lines.append('  parser.end_parse_array();')
            return '\n'.join(lines)

    elif isinstance(member_type, AbstractSequence):
        if isinstance(member_type.value_type, BasicType):
            lines = [f'  auto {member.name}_size = parser.begin_parse_sequence();']
            lines.append(f'    parser.parse_primitive({get_xcdr_primitive_kind(member_type.value_type)});')
            lines.append('  parser.end_parse_sequence();')
            return '\n'.join(lines)
        else:
            lines = [f'  auto {member.name}_size = parser.begin_parse_sequence();']
            if isinstance(member_type.value_type, (AbstractString, AbstractWString)):
                lines.append('    parser.parse_string();')
            elif isinstance(member_type.value_type, NamespacedType):
                lines.append('    parser.begin_parse_struct();')
                lines.append('    parser.end_parse_struct();')
            lines.append('  parser.end_parse_sequence();')
            return '\n'.join(lines)

    elif isinstance(member_type, NamespacedType):
        # Nested message
        lines = [f'  parser.begin_parse_struct();']
        lines.append('  parser.end_parse_struct();')
        return '\n'.join(lines)

    return f'  // TODO: Parse {member.name}'


def generate_writer_field(member, is_experimental, msg_prefix='msg'):
    """Generate XCdrWriter serialization for a field."""
    member_name = member.name
    member_type = member.type

    if isinstance(member_type, BasicType):
        cpp_type = get_cpp_type(member_type)
        return f'  writer.write<{cpp_type}>({msg_prefix}.{member_name});'

    elif isinstance(member_type, (AbstractString, AbstractWString)):
        # String fields in experimental messages
        if is_experimental:
            return f'  writer.write(std::string_view({msg_prefix}.{member_name}.data(), {msg_prefix}.{member_name}.size()));'
        else:
            return f'  writer.write(std::string_view({msg_prefix}.{member_name}));'

    elif isinstance(member_type, Array):
        if isinstance(member_type.value_type, BasicType):
            cpp_type = get_cpp_type(member_type.value_type)
            if is_experimental:
                return f'  writer.write_array(tcb::span<const {cpp_type}>({msg_prefix}.{member_name}.data(), {member_type.size}));'
            else:
                return f'  writer.write_array(tcb::span<const {cpp_type}>({msg_prefix}.{member_name}.data(), {member_type.size}));'
        else:
            lines = [f'  writer.begin_write_array({member_type.size});']
            if isinstance(member_type.value_type, (AbstractString, AbstractWString)):
                if is_experimental:
                    lines.append(f'  for (size_t i = 0; i < {member_type.size}; ++i) {{')
                    lines.append(f'    writer.write(std::string_view({msg_prefix}.{member_name}[i].data(), {msg_prefix}.{member_name}[i].size()));')
                else:
                    lines.append(f'  for (const auto & elem : {msg_prefix}.{member_name}) {{')
                    lines.append('    writer.write(std::string_view(elem));')
                lines.append('  }')
            elif isinstance(member_type.value_type, NamespacedType):
                msg_type = get_message_type_name(member_type.value_type)
                lines.append('  {')
                lines.append(f'    auto nested_ts_{member_name} = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
                lines.append(f'    auto nested_callbacks_{member_name} = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_{member_name}->data);')
                lines.append(f'    for (size_t i = 0; i < {member_type.size}; ++i) {{')
                lines.append(f'      nested_callbacks_{member_name}->serialize_into_writer(&{msg_prefix}.{member_name}[i], writer);')
                lines.append('    }')
                lines.append('  }')
            lines.append('  writer.end_write_array();')
            return '\n'.join(lines)

    elif isinstance(member_type, AbstractSequence):
        if isinstance(member_type.value_type, BasicType):
            cpp_type = get_cpp_type(member_type.value_type)
            if is_experimental:
                return f'  writer.write_sequence(tcb::span<const {cpp_type}>({msg_prefix}.{member_name}.data(), {msg_prefix}.{member_name}.size()));'
            else:
                return f'  writer.write_sequence(tcb::span<const {cpp_type}>({msg_prefix}.{member_name}.data(), {msg_prefix}.{member_name}.size()));'
        else:
            if is_experimental:
                lines = [f'  writer.begin_write_sequence({msg_prefix}.{member_name}.size());']
            else:
                lines = [f'  writer.begin_write_sequence({msg_prefix}.{member_name}.size());']
            if isinstance(member_type.value_type, (AbstractString, AbstractWString)):
                if is_experimental:
                    lines.append(f'  for (size_t i = 0; i < {msg_prefix}.{member_name}.size(); ++i) {{')
                    lines.append(f'    writer.write(std::string_view({msg_prefix}.{member_name}[i].data(), {msg_prefix}.{member_name}[i].size()));')
                else:
                    lines.append(f'  for (const auto & elem : {msg_prefix}.{member_name}) {{')
                    lines.append('    writer.write(std::string_view(elem));')
                lines.append('  }')
            elif isinstance(member_type.value_type, NamespacedType):
                msg_type = get_message_type_name(member_type.value_type)
                lines.append('  {')
                lines.append(f'    auto nested_ts_{member_name} = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
                lines.append(f'    auto nested_callbacks_{member_name} = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_{member_name}->data);')
                lines.append(f'    for (const auto & elem : {msg_prefix}.{member_name}) {{')
                lines.append(f'      nested_callbacks_{member_name}->serialize_into_writer(&elem, writer);')
                lines.append('    }')
                lines.append('  }')
            lines.append('  writer.end_write_sequence();')
            return '\n'.join(lines)

    elif isinstance(member_type, NamespacedType):
        # Nested message: serialize inline using private callback
        msg_type = get_message_type_name(member_type)
        lines = [f'  // Nested message: {member_name}']
        lines.append('  {')
        lines.append(f'    auto nested_ts_{member_name} = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
        lines.append(f'    auto nested_callbacks_{member_name} = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_{member_name}->data);')
        lines.append(f'    nested_callbacks_{member_name}->serialize_into_writer(&{msg_prefix}.{member_name}, writer);')
        lines.append('  }')
        return '\n'.join(lines)

    return f'  // TODO: Write {member_name}'


def generate_reader_field(member, is_experimental, msg_prefix='msg'):
    """Generate XCdrReader deserialization for a field."""
    member_name = member.name
    member_type = member.type

    if isinstance(member_type, BasicType):
        cpp_type = get_cpp_type(member_type)
        return f'  {msg_prefix}.{member_name} = *reader.read<{cpp_type}>();'

    elif isinstance(member_type, (AbstractString, AbstractWString)):
        if is_experimental:
            lines = [f'  auto {member_name}_view = *reader.read<std::string_view>();']
            lines.append(f'  {msg_prefix}.{member_name}.assign({member_name}_view.data(), {member_name}_view.size());')
            return '\n'.join(lines)
        else:
            lines = [f'  auto {member_name}_view = *reader.read<std::string_view>();']
            lines.append(f'  {msg_prefix}.{member_name}.assign({member_name}_view);')
            return '\n'.join(lines)

    elif isinstance(member_type, Array):
        if isinstance(member_type.value_type, BasicType):
            cpp_type = get_cpp_type(member_type.value_type)
            lines = [f'  auto {member_name}_array = *reader.read<std::array<{cpp_type}, {member_type.size}> >();']
            if is_experimental:
                lines.append(f'  std::copy({member_name}_array.begin(), {member_name}_array.end(), {msg_prefix}.{member_name}.begin());')
            else:
                lines.append(f'  {msg_prefix}.{member_name} = {member_name}_array;')
            return '\n'.join(lines)
        else:
            lines = [f'  reader.begin_read_array();']
            if isinstance(member_type.value_type, (AbstractString, AbstractWString)):
                lines.append(f'  for (size_t i = 0; i < {member_type.size}; ++i) {{')
                lines.append('    auto elem_view = *reader.read<std::string_view>();')
                if is_experimental:
                    lines.append(f'    {msg_prefix}.{member_name}[i].assign(elem_view.data(), elem_view.size());')
                else:
                    lines.append(f'    {msg_prefix}.{member_name}[i].assign(elem_view);')
                lines.append('  }')
            elif isinstance(member_type.value_type, NamespacedType):
                msg_type = get_message_type_name(member_type.value_type)
                lines.append('  {')
                lines.append(f'    auto nested_ts_{member_name} = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
                lines.append(f'    auto nested_callbacks_{member_name} = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_{member_name}->data);')
                lines.append(f'    for (size_t i = 0; i < {member_type.size}; ++i) {{')
                lines.append(f'      nested_callbacks_{member_name}->deserialize_from_reader(reader, &{msg_prefix}.{member_name}[i]);')
                lines.append('    }')
                lines.append('  }')
            lines.append('  reader.end_read_array();')
            return '\n'.join(lines)

    elif isinstance(member_type, AbstractSequence):
        if isinstance(member_type.value_type, BasicType):
            cpp_type = get_cpp_type(member_type.value_type)
            lines = [f'  auto {member_name}_vec = *reader.read<std::vector<{cpp_type}>>();']
            if is_experimental:
                lines.append(f'  {msg_prefix}.{member_name}.resize({member_name}_vec.size());')
                lines.append(f'  std::copy({member_name}_vec.begin(), {member_name}_vec.end(), {msg_prefix}.{member_name}.begin());')
            else:
                lines.append(f'  {msg_prefix}.{member_name} = {member_name}_vec;')
            return '\n'.join(lines)
        else:
            lines = [f'  auto {member_name}_size = *reader.begin_read_sequence();']
            if is_experimental:
                lines.append(f'  {msg_prefix}.{member_name}.resize({member_name}_size);')
            else:
                lines.append(f'  {msg_prefix}.{member_name}.resize({member_name}_size);')
            if isinstance(member_type.value_type, (AbstractString, AbstractWString)):
                lines.append(f'  for (size_t i = 0; i < {member_name}_size; ++i) {{')
                lines.append('    auto elem_view = *reader.read<std::string_view>();')
                if is_experimental:
                    lines.append(f'    {msg_prefix}.{member_name}[i].assign(elem_view.data(), elem_view.size());')
                else:
                    lines.append(f'    {msg_prefix}.{member_name}[i].assign(elem_view);')
                lines.append('  }')
            elif isinstance(member_type.value_type, NamespacedType):
                msg_type = get_message_type_name(member_type.value_type)
                lines.append('  {')
                lines.append(f'    auto nested_ts_{member_name} = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
                lines.append(f'    auto nested_callbacks_{member_name} = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_{member_name}->data);')
                lines.append(f'    for (size_t i = 0; i < {member_name}_size; ++i) {{')
                lines.append(f'      nested_callbacks_{member_name}->deserialize_from_reader(reader, &{msg_prefix}.{member_name}[i]);')
                lines.append('    }')
                lines.append('  }')
            lines.append('  reader.end_read_sequence();')
            return '\n'.join(lines)

    elif isinstance(member_type, NamespacedType):
        # Nested message: deserialize inline using private callback
        msg_type = get_message_type_name(member_type)
        lines = [f'  // Nested message: {member_name}']
        lines.append('  {')
        lines.append(f'    auto nested_ts_{member_name} = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
        lines.append(f'    auto nested_callbacks_{member_name} = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_{member_name}->data);')
        lines.append(f'    nested_callbacks_{member_name}->deserialize_from_reader(reader, &{msg_prefix}.{member_name});')
        lines.append('  }')
        return '\n'.join(lines)

    return f'  // TODO: Read {member_name}'


def generate_external_storage_field(member, index):
    """Generate external storage population code."""
    member_name = member.name
    member_type = member.type

    if isinstance(member_type, BasicType):
        cpp_type = get_cpp_type(member_type)
        lines = [f'  auto {member_name}_slice = accessor[{index}].slice();']
        lines.append(f'  ext_storage.members.{member_name} = rosidl_runtime_cpp::Memory<{cpp_type}>(static_cast<{cpp_type}*>(const_cast<void*>(static_cast<const void*>({member_name}_slice.data()))), 0);')
        return '\n'.join(lines)

    elif isinstance(member_type, (AbstractString, AbstractWString)):
        lines = [f'  auto {member_name}_slice = accessor[{index}].slice();']
        lines.append(f'  ext_storage.members.{member_name} = rosidl_runtime_cpp::MemoryRegion<char>(')
        lines.append(f'    static_cast<char*>(const_cast<void*>(static_cast<const void*>({member_name}_slice.data()))),')
        lines.append(f'    {member_name}_slice.size());')
        return '\n'.join(lines)

    elif isinstance(member_type, Array):
        if isinstance(member_type.value_type, BasicType):
            cpp_type = get_cpp_type(member_type.value_type)
            lines = [f'  auto {member_name}_slice = accessor[{index}].slice();']
            lines.append(f'  ext_storage.members.{member_name} = rosidl_runtime_cpp::MemoryRegion<{cpp_type}>(')
            lines.append(f'    static_cast<{cpp_type}*>(const_cast<void*>(static_cast<const void*>({member_name}_slice.data()))),')
            lines.append(f'    {member_type.size});')
            return '\n'.join(lines)

    elif isinstance(member_type, AbstractSequence):
        if isinstance(member_type.value_type, BasicType):
            cpp_type = get_cpp_type(member_type.value_type)
            lines = [f'  auto {member_name}_slice = accessor[{index}].slice();']
            lines.append(f'  ext_storage.members.{member_name}.region = rosidl_runtime_cpp::MemoryRegion<{cpp_type}>(')
            lines.append(f'    static_cast<{cpp_type}*>(const_cast<void*>(static_cast<const void*>({member_name}_slice.data()))),')
            lines.append(f'    *accessor[{index}].size());')
            return '\n'.join(lines)

    return f'  // TODO: External storage for {member_name}'


def get_message_type_name(namespaced_type):
    """Get fully qualified C++ type name for a message."""
    return '::'.join(namespaced_type.namespaced_name())


def get_nested_typesupport_include(namespaced_type):
    """Get include path for nested message's XCDR typesupport."""
    from rosidl_pycommon import convert_camel_case_to_lower_case_underscore
    parts = list(namespaced_type.namespaces)
    filename = convert_camel_case_to_lower_case_underscore(namespaced_type.name)
    parts.append('detail')
    parts.append(f'{filename}__rosidl_typesupport_xcdr_cpp')
    return '/'.join(parts) + '.hpp'


def generate_size_calculation(member, msg_prefix='msg'):
    """Generate code to calculate message member size for serialization."""
    member_name = member.name
    member_type = member.type

    if isinstance(member_type, BasicType):
        cpp_type = get_cpp_type(member_type)
        return f'  *size += sizeof({cpp_type});'

    elif isinstance(member_type, (AbstractString, AbstractWString)):
        # String: 4-byte length + data + null terminator
        return f'  *size += 4 + {msg_prefix}.{member_name}.size() + 1;'

    elif isinstance(member_type, Array):
        if isinstance(member_type.value_type, BasicType):
            cpp_type = get_cpp_type(member_type.value_type)
            return f'  *size += sizeof({cpp_type}) * {member_type.size};'
        elif isinstance(member_type.value_type, (AbstractString, AbstractWString)):
            lines = [f'  for (size_t i = 0; i < {member_type.size}; ++i) {{']
            lines.append(f'    *size += 4 + {msg_prefix}.{member_name}[i].size() + 1;')
            lines.append('  }')
            return '\n'.join(lines)
        elif isinstance(member_type.value_type, NamespacedType):
            msg_type = get_message_type_name(member_type.value_type)
            lines = [f'  {{']
            lines.append(f'    auto nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
            lines.append(f'    for (size_t i = 0; i < {member_type.size}; ++i) {{')
            lines.append('      size_t nested_size = 0;')
            lines.append(f'      rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts, &{msg_prefix}.{member_name}[i], &nested_size);')
            lines.append('      *size += nested_size - 4;  // Nested messages are inlined without XCDR header')
            lines.append('    }')
            lines.append('  }')
            return '\n'.join(lines)

    elif isinstance(member_type, AbstractSequence):
        if isinstance(member_type.value_type, BasicType):
            cpp_type = get_cpp_type(member_type.value_type)
            return f'  *size += 4 + sizeof({cpp_type}) * {msg_prefix}.{member_name}.size();'
        elif isinstance(member_type.value_type, (AbstractString, AbstractWString)):
            lines = [f'  *size += 4;  // sequence length']
            lines.append(f'  for (const auto & elem : {msg_prefix}.{member_name}) {{')
            lines.append('    *size += 4 + elem.size() + 1;')
            lines.append('  }')
            return '\n'.join(lines)
        elif isinstance(member_type.value_type, NamespacedType):
            msg_type = get_message_type_name(member_type.value_type)
            lines = [f'  *size += 4;  // sequence length']
            lines.append('  {')
            lines.append(f'    auto nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
            lines.append(f'    for (const auto & elem : {msg_prefix}.{member_name}) {{')
            lines.append('      size_t nested_size = 0;')
            lines.append('      rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts, &elem, &nested_size);')
            lines.append('      *size += nested_size - 4;  // Nested messages are inlined without XCDR header')
            lines.append('    }')
            lines.append('  }')
            return '\n'.join(lines)

    elif isinstance(member_type, NamespacedType):
        # Nested message: size without XCDR header (inlined in parent)
        msg_type = get_message_type_name(member_type)
        lines = [f'  {{']
        lines.append(f'    auto nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<{msg_type}>();')
        lines.append('    size_t nested_size = 0;')
        lines.append(f'    rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts, &{msg_prefix}.{member_name}, &nested_size);')
        lines.append('    *size += nested_size - 4;  // Nested messages are inlined without XCDR header')
        lines.append('  }')
        return '\n'.join(lines)

    return f'  // TODO: Size calculation for {member_name}'

