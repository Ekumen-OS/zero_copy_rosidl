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

# Check if this is an experimental message
is_experimental = 'experimental' in '/'.join(message.structure.namespaced_type.namespaces)

# Determine namespace
msg_typename = message.structure.namespaced_type.name
msg_namespace = '::'.join(message.structure.namespaced_type.namespaces)
full_msg_typename = '::'.join(message.structure.namespaced_type.namespaces + [msg_typename])

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
            nested_includes.add(get_nested_typesupport_include(member_type))
    elif isinstance(member.type, (Array, AbstractSequence)):
        elem_type = member_type.value_type
        if isinstance(elem_type, NamespacedType):
            # Skip service messages
            is_nested_service_msg = ('srv' in elem_type.namespaces and
                                    (elem_type.name.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
                                     elem_type.name.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX) or
                                     elem_type.name.endswith(SERVICE_EVENT_MESSAGE_SUFFIX)))
            if not is_nested_service_msg:
                nested_includes.add(get_nested_typesupport_include(elem_type))

}@
@# ===== EmPy macros for field code generation =====

@[def generate_layout_field(member, constraints_prefix='constraints')]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpp.template_helpers import get_xcdr_primitive_kind, get_cpp_type, get_message_type_name }@ @
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
      auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type))>();
      auto nested_callbacks_@(member.name) = static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_@(member.name)->data);
      nested_callbacks_@(member.name)->build_layout_fields(builder, nullptr);
    }
    builder.end_allocate_struct();
@[    end if]@
  builder.end_allocate_array();
@[  end if]@
@[elif isinstance(member.type, (BoundedSequence, AbstractSequence))]@
@[  if isinstance(member.type.value_type, BasicType)]@
  builder.allocate_primitive_sequence("@(member.name)", @(get_xcdr_primitive_kind(member.type.value_type)), @(constraints_prefix).@(member.name).size);
@[  else]@
  builder.begin_allocate_sequence("@(member.name)", @(constraints_prefix).@(member.name).size);
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
@[      if isinstance(member.type.value_type, (BoundedString, BoundedWString))]@
    builder.allocate_string(@(member.type.value_type.maximum_size));
@[      else]@
    builder.allocate_string(@(constraints_prefix).@(member.name).element.size);
@[      end if]@
@[    elif isinstance(member.type.value_type, NamespacedType)]@
    builder.begin_allocate_struct();
    {
      auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type))>();
      auto nested_callbacks_@(member.name) = 
        static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_@(member.name)->data);
      nested_callbacks_@(member.name)->build_layout_fields(builder, nullptr);
    }
    builder.end_allocate_struct();
@[    end if]@
  builder.end_allocate_sequence();
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  // Nested message: @(member.name)
  builder.begin_allocate_struct();
  {
    auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type))>();
    auto nested_callbacks_@(member.name) = 
      static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_@(member.name)->data);
    if (@(constraints_prefix).@(member.name)) {
      nested_callbacks_@(member.name)->build_layout_fields(builder, @(constraints_prefix).@(member.name));
    } else {
      nested_callbacks_@(member.name)->build_layout_fields(builder, nullptr);
    }
  }
  builder.end_allocate_struct();
@[else]@
  // TODO: Handle @(member.name) of type @(member.type)
@[end if]@
@[end def]@

@[def generate_parser_field(member)]@
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
    parser.end_parse_struct();
@[    end if]@
  parser.end_parse_array();
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
  auto @(member.name)_size = parser.begin_parse_sequence();
@[  if isinstance(member.type.value_type, BasicType)]@
    parser.parse_primitive(@(get_xcdr_primitive_kind(member.type.value_type)));
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
    parser.parse_string();
@[  elif isinstance(member.type.value_type, NamespacedType)]@
    parser.begin_parse_struct();
    parser.end_parse_struct();
@[  end if]@
  parser.end_parse_sequence();
@[elif isinstance(member.type, NamespacedType)]@
  parser.begin_parse_struct();
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
@[elif isinstance(member.type, (AbstractString, AbstractWString))]@
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
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
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
    auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type))>();
    auto nested_callbacks_@(member.name) = 
      static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_@(member.name)->data);
    for (size_t i = 0; i < @(member.type.size); ++i) {
      nested_callbacks_@(member.name)->serialize_into_writer(&@(msg_prefix).@(member.name)[i], writer);
    }
  }
@[    end if]@
  writer.end_write_array();
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  writer.write_sequence(tcb::span<const @(get_cpp_type(member.type.value_type))>(@(msg_prefix).@(member.name).data(), @(msg_prefix).@(member.name).size()));
@[  else]@
  writer.begin_write_sequence(@(msg_prefix).@(member.name).size());
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
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
    auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type))>();
    auto nested_callbacks_@(member.name) = 
      static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_@(member.name)->data);
    for (const auto & elem : @(msg_prefix).@(member.name)) {
      nested_callbacks_@(member.name)->serialize_into_writer(&elem, writer);
    }
  }
@[    end if]@
  writer.end_write_sequence();
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  // Nested message: @(member.name)
  {
    auto nested_ts_@(member.name) = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type))>();
    auto nested_callbacks_@(member.name) = 
      static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_@(member.name)->data);
    nested_callbacks_@(member.name)->serialize_into_writer(&@(msg_prefix).@(member.name), writer);
  }
@[else]@
  // TODO: Write @(member.name)
@[end if]@
@[end def]@

@[def generate_reader_field(member, is_experimental, msg_prefix='msg')]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpp.template_helpers import get_xcdr_primitive_kind, get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  @(msg_prefix).@(member.name) = *reader.read<@(get_cpp_type(member.type))>();
@[elif isinstance(member.type, (AbstractString, AbstractWString))]@
  auto @(member.name)_view = *reader.read<std::string_view>();
@[  if is_experimental]@
  @(msg_prefix).@(member.name).assign(@(member.name)_view.data(), @(member.name)_view.size());
@[  else]@
  @(msg_prefix).@(member.name).assign(@(member.name)_view);
@[  end if]@
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  auto @(member.name)_array = *reader.read<std::array<@(get_cpp_type(member.type.value_type)), @(member.type.size)> >();
@[    if is_experimental]@
  std::copy(@(member.name)_array.begin(), @(member.name)_array.end(), @(msg_prefix).@(member.name).begin());
@[    else]@
  @(msg_prefix).@(member.name) = @(member.name)_array;
@[    end if]@
@[  else]@
  reader.begin_read_array();
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  for (size_t i = 0; i < @(member.type.size); ++i) {
    auto elem_view = *reader.read<std::string_view>();
@[      if is_experimental]@
    @(msg_prefix).@(member.name)[i].assign(elem_view.data(), elem_view.size());
@[      else]@
    @(msg_prefix).@(member.name)[i].assign(elem_view);
@[      end if]@
  }
@[    elif isinstance(member.type.value_type, NamespacedType)]@
  {
    auto nested_ts_@(member.name) = 
      rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type))>();
    auto nested_callbacks_@(member.name) = 
      static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_@(member.name)->data);
    for (size_t i = 0; i < @(member.type.size); ++i) {
      nested_callbacks_@(member.name)->deserialize_from_reader(reader, &@(msg_prefix).@(member.name)[i]);
    }
  }
@[    end if]@
  reader.end_read_array();
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  auto @(member.name)_vec = *reader.read<std::vector<@(get_cpp_type(member.type.value_type))>>();
@[    if is_experimental]@
  @(msg_prefix).@(member.name).resize(@(member.name)_vec.size());
  std::copy(@(member.name)_vec.begin(), @(member.name)_vec.end(), @(msg_prefix).@(member.name).begin());
@[    else]@
  @(msg_prefix).@(member.name) = @(member.name)_vec;
@[    end if]@
@[  else]@
  auto @(member.name)_size = *reader.begin_read_sequence();
  @(msg_prefix).@(member.name).resize(@(member.name)_size);
@[    if isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  for (size_t i = 0; i < @(member.name)_size; ++i) {
    auto elem_view = *reader.read<std::string_view>();
@[      if is_experimental]@
    @(msg_prefix).@(member.name)[i].assign(elem_view.data(), elem_view.size());
@[      else]@
    @(msg_prefix).@(member.name)[i].assign(elem_view);
@[      end if]@
  }
@[    elif isinstance(member.type.value_type, NamespacedType)]@
  {
    auto nested_ts_@(member.name) = 
      rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type))>();
    auto nested_callbacks_@(member.name) = 
      static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_@(member.name)->data);
    for (size_t i = 0; i < @(member.name)_size; ++i) {
      nested_callbacks_@(member.name)->deserialize_from_reader(reader, &@(msg_prefix).@(member.name)[i]);
    }
  }
@[    end if]@
  reader.end_read_sequence();
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  // Nested message: @(member.name)
  {
    auto nested_ts_@(member.name) = 
      rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type))>();
    auto nested_callbacks_@(member.name) = 
      static_cast<const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t *>(nested_ts_@(member.name)->data);
    nested_callbacks_@(member.name)->deserialize_from_reader(reader, &@(msg_prefix).@(member.name));
  }
@[else]@
  // TODO: Read @(member.name)
@[end if]@
@[end def]@

@[def generate_external_storage_field(member, index)]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpp.template_helpers import get_xcdr_primitive_kind, get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  auto @(member.name)_slice = accessor[@(index)].slice();
  ext_storage.members.@(member.name) = rosidl_runtime_cpp::Memory<@(get_cpp_type(member.type))>(static_cast<@(get_cpp_type(member.type))*>(const_cast<void*>(static_cast<const void*>(@(member.name)_slice.data()))), 0);
@[elif isinstance(member.type, (AbstractString, AbstractWString))]@
  auto @(member.name)_slice = accessor[@(index)].slice();
  ext_storage.members.@(member.name) = rosidl_runtime_cpp::MemoryRegion<char>(
    static_cast<char*>(const_cast<void*>(static_cast<const void*>(@(member.name)_slice.data()))),
    @(member.name)_slice.size());
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  auto @(member.name)_slice = accessor[@(index)].slice();
  ext_storage.members.@(member.name) = rosidl_runtime_cpp::MemoryRegion<@(get_cpp_type(member.type.value_type))>(
    static_cast<@(get_cpp_type(member.type.value_type))*>(const_cast<void*>(static_cast<const void*>(@(member.name)_slice.data()))),
    @(member.type.size));
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  auto @(member.name)_slice = accessor[@(index)].slice();
  ext_storage.members.@(member.name).region = rosidl_runtime_cpp::MemoryRegion<@(get_cpp_type(member.type.value_type))>(
    static_cast<@(get_cpp_type(member.type.value_type))*>(const_cast<void*>(static_cast<const void*>(@(member.name)_slice.data()))),
    *accessor[@(index)].size());
@[  end if]@
@[else]@
  // TODO: External storage for @(member.name)
@[end if]@
@[end def]@

@[def generate_size_calculation(member, msg_prefix='msg')]@
@{ from rosidl_parser.definition import BasicType, AbstractString, AbstractWString, BoundedString, BoundedWString, Array, BoundedSequence, AbstractSequence, NamespacedType }@ @
@{ from rosidl_typesupport_xcdr_cpp.template_helpers import get_xcdr_primitive_kind, get_cpp_type, get_message_type_name }@ @
@[if isinstance(member.type, BasicType)]@
  *size += sizeof(@(get_cpp_type(member.type)));
@[elif isinstance(member.type, (AbstractString, AbstractWString))]@
  *size += 4 + @(msg_prefix).@(member.name).size() + 1;
@[elif isinstance(member.type, Array)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  *size += sizeof(@(get_cpp_type(member.type.value_type))) * @(member.type.size);
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  for (size_t i = 0; i < @(member.type.size); ++i) {
    *size += 4 + @(msg_prefix).@(member.name)[i].size() + 1;
  }
@[  elif isinstance(member.type.value_type, NamespacedType)]@
  {
    auto nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type))>();
    for (size_t i = 0; i < @(member.type.size); ++i) {
      size_t nested_size = 0;
      rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts, &@(msg_prefix).@(member.name)[i], &nested_size);
      *size += nested_size - 4;  // Nested messages are inlined without XCDR header
    }
  }
@[  end if]@
@[elif isinstance(member.type, AbstractSequence)]@
@[  if isinstance(member.type.value_type, BasicType)]@
  *size += 4 + sizeof(@(get_cpp_type(member.type.value_type))) * @(msg_prefix).@(member.name).size();
@[  elif isinstance(member.type.value_type, (AbstractString, AbstractWString))]@
  *size += 4;  // sequence length
  for (const auto & elem : @(msg_prefix).@(member.name)) {
    *size += 4 + elem.size() + 1;
  }
@[  elif isinstance(member.type.value_type, NamespacedType)]@
  *size += 4;  // sequence length
  {
    auto nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type.value_type))>();
    for (const auto & elem : @(msg_prefix).@(member.name)) {
      size_t nested_size = 0;
      rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts, &elem, &nested_size);
      *size += nested_size - 4;  // Nested messages are inlined without XCDR header
    }
  }
@[  end if]@
@[elif isinstance(member.type, NamespacedType)]@
  {
    auto nested_ts = rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(get_message_type_name(member.type))>();
    size_t nested_size = 0;
    rosidl_typesupport_xcdr_cpp::get_message_size(nested_ts, &@(msg_prefix).@(member.name), &nested_size);
    *size += nested_size - 4;  // Nested messages are inlined without XCDR header
  }
@[else]@
  // TODO: Size calculation for @(member.name)
@[end if]@
@[end def]@

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "rcutils/types/rcutils_ret.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_cpp/experimental/memory.hpp"
#include "rosidl_typesupport_interface/macros.h"
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

@[if is_service_message and service_name]@
#include "@(package_name)/srv/detail/@(convert_camel_case_to_lower_case_underscore(service_name))__struct.hpp"
@[else]@
#include "@('/'.join(message.structure.namespaced_type.namespaces))/@(convert_camel_case_to_lower_case_underscore(msg_typename)).hpp"
@[end if]@

@[for include in sorted(nested_includes)]@
#include "@(include)"
@[end for]@

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
@[    for member in message.structure.members]@
@(generate_layout_field(member))
@[    end for]@
  
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
@[    for member in message.structure.members]@
@(generate_layout_field(member))
@[    end for]@
  
  return RCUTILS_RET_OK;
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
@[  end if]@

// Get expected message size
rcutils_ret_t
get_expected_message_size_@(msg_typename)(
  const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t * callbacks,
  size_t * size)
{
  if (!callbacks->cached_layout) {
    // Error:("Layout not available (message needs constraints)");
    return RCUTILS_RET_ERROR;
  }
  
  *size = callbacks->cached_layout->total_size();
  return RCUTILS_RET_OK;
}

// Get actual message size
rcutils_ret_t
get_message_size_@(msg_typename)(
  const void * message_ptr,
  size_t * size)
{
  // Use XCdrWriter to compute actual size
  auto & msg = *static_cast<const @(full_msg_typename) *>(message_ptr);
  
  xcdr_buffers::XCdrWriter writer;
@[  for member in message.structure.members]@
@(generate_writer_field(member, is_experimental, 'msg'))
@[  end for]@
  
  auto buffer = writer.flush();
  *size = buffer.size();
  
  return RCUTILS_RET_OK;
}

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
  const rosidl_typesupport_xcdr_cpp::message_type_support_callbacks_experimental_t * callbacks,
  rosidl_runtime_cpp::MemoryRegion<void> & storage,
  void ** message_ptr)
{
  if (!callbacks->cached_layout) {
    // Error:("Layout not available (message needs constraints)");
    return RCUTILS_RET_ERROR;
  }
  
  // 1. Initialize buffer with XCDR layout
  auto buffer_span = tcb::span<uint8_t>(
    static_cast<uint8_t*>(storage.data()),
    callbacks->cached_layout->total_size());
  callbacks->cached_layout->apply(buffer_span);
  
  // 2. Create accessor for collecting memory regions
  auto accessor_result = xcdr_buffers::XCdrAccessor::wrap(buffer_span, *callbacks->cached_layout);
  if (!accessor_result) {
    // Error:("Failed to create accessor");
    return RCUTILS_RET_ERROR;
  }
  auto accessor = *accessor_result;
  
  // 3. Build external storage from accessor
  @(full_msg_typename)::ExternalStorage ext_storage;
  ext_storage.block = rosidl_runtime_cpp::MemoryRegion<void>(
    storage.data(),
    callbacks->cached_layout->total_size());
  
@[  for idx, member in enumerate(message.structure.members)]@
@(generate_external_storage_field(member, idx))
@[  end for]@
  
  // 4. Construct message from external storage
  *message_ptr = new @(full_msg_typename)(ext_storage, rosidl_runtime_cpp::MessageInitialization::SKIP);
  
  return RCUTILS_RET_OK;
}

// Cast message at storage (zero-copy receiver side)
rcutils_ret_t
cast_message_at_@(msg_typename)(
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void ** message_ptr)
{
  // 1. Parse layout from buffer
  auto buffer_span = tcb::span<const uint8_t>(
    static_cast<const uint8_t*>(storage.data()),
    storage.size());
  
  xcdr_buffers::XCdrLayoutParser parser(buffer_span);
@[  for member in message.structure.members]@
@(generate_parser_field(member))
@[  end for]@
  
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
  
@[  for idx, member in enumerate(message.structure.members)]@
@(generate_external_storage_field(member, idx))
@[  end for]@
  
  // 4. Construct message from external storage
  *message_ptr = new @(full_msg_typename)(ext_storage, rosidl_runtime_cpp::MessageInitialization::SKIP);
  
  return RCUTILS_RET_OK;
}

// Deserialize message from storage (traditional)
// Writes into existing message instead of allocating
rcutils_ret_t
deserialize_message_from_@(msg_typename)(
  const rosidl_runtime_cpp::MemoryRegion<void> & storage,
  void * message_ptr)
{
  auto buffer_span = tcb::span<const uint8_t>(
    static_cast<const uint8_t*>(storage.data()),
    storage.size());
  
  auto reader_result = xcdr_buffers::XCdrReader::wrap(buffer_span);
  if (!reader_result) {
    // Error:("Failed to create XCdrReader");
    return RCUTILS_RET_ERROR;
  }
  auto reader = *reader_result;
  
  // Deserialize fields using private callback
  return deserialize_fields_from_reader_@(msg_typename)(reader, message_ptr);
}

// Serialize message into storage (traditional)
rcutils_ret_t
serialize_message_into_@(msg_typename)(
  const void * message_ptr,
  rosidl_runtime_cpp::MemoryRegion<void> storage)
{
  auto buffer_span = tcb::span<uint8_t>(
    static_cast<uint8_t*>(storage.data()), storage.size());
  
  xcdr_buffers::XCdrWriter writer(buffer_span);
  
  // Serialize fields using private callback
  auto ret = serialize_fields_into_writer_@(msg_typename)(message_ptr, writer);
  if (ret != RCUTILS_RET_OK) {
    return ret;
  }
  
  if (writer.has_error()) {
    // Buffer overflow
    return RCUTILS_RET_ERROR;
  }
  
  return RCUTILS_RET_OK;
}

// Destroy message created by construct_at or cast_at
void
destroy_message_@(msg_typename)(void * message_ptr)
{
  delete static_cast<@(full_msg_typename) *>(message_ptr);
}

// Release message and recover external storage
rosidl_runtime_cpp::MemoryRegion<void>
release_message_@(msg_typename)(void * message_ptr)
{
  auto * msg = static_cast<@(full_msg_typename) *>(message_ptr);
  
  // Extract storage before destruction
  rosidl_runtime_cpp::MemoryRegion<void> storage{nullptr, 0};
  if (msg->_external_storage.has_value()) {
    storage = msg->_external_storage.value().block;
  }
  
  // Destroy message
  delete msg;
  
  return storage;
}

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

// Deserialize message from storage
rcutils_ret_t
deserialize_message_from_@(msg_typename)(
  rosidl_runtime_cpp::MemoryRegion<void> storage,
  void * message_ptr)
{
  auto buffer_span = tcb::span<const uint8_t>(
    static_cast<const uint8_t*>(storage.data()),
    storage.size());
  
  auto reader_result = xcdr_buffers::XCdrReader::wrap(buffer_span);
  if (!reader_result) {
    // Error:("Failed to create XCdrReader");
    return RCUTILS_RET_ERROR;
  }
  auto reader = *reader_result;
  
  // Deserialize fields using private callback
  return deserialize_fields_from_reader_@(msg_typename)(reader, message_ptr);
}

// Serialize message into storage
rcutils_ret_t
serialize_message_into_@(msg_typename)(
  const void * message_ptr,
  rosidl_runtime_cpp::MemoryRegion<void> storage)
{
  auto buffer_span = tcb::span<uint8_t>(
    static_cast<uint8_t*>(storage.data()), storage.size());
  
  xcdr_buffers::XCdrWriter writer(buffer_span);
  
  // Serialize fields using private callback
  auto ret = serialize_fields_into_writer_@(msg_typename)(message_ptr, writer);
  if (ret != RCUTILS_RET_OK) {
    return ret;
  }
  
  if (writer.has_error()) {
    // Buffer overflow
    return RCUTILS_RET_ERROR;
  }
  
  return RCUTILS_RET_OK;
}

// Get message size (for non-experimental messages)
rcutils_ret_t
get_message_size_@(msg_typename)(
  const void * message_ptr,
  size_t * size)
{
  if (!message_ptr || !size) {
    return RCUTILS_RET_ERROR;
  }
  
  auto & msg = *static_cast<const @(full_msg_typename) *>(message_ptr);
  (void)msg;
  *size = 4;  // XCDR header

@[  for member in message.structure.members]@
@(generate_size_calculation(member, 'msg'))
@[  end for]@
  
  return RCUTILS_RET_OK;
}

}  // namespace @(msg_namespace)

@[end if]@

// Template specialization with bundled static initialization
namespace rosidl_typesupport_xcdr_cpp
{

template<>
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_message_type_support_t *
get_message_type_support_handle<@(full_msg_typename)>()
{
  // Thread-safe lazy initialization (C++11 guarantees this for function-local statics)
  static const message_type_support_callbacks_experimental_t callbacks = {
    "@(msg_namespace)",  // message_namespace
    "@(msg_typename)",  // message_name
@[if is_experimental]@
@[  if has_constraints]@
    nullptr,  // cached_layout (will be built from constraints)
    &@(msg_namespace)::build_layout_@(msg_typename),  // build_constrained_layout
@[  else]@
    @(msg_namespace)::get_layout_@(msg_typename)(),  // cached_layout (singleton)
    nullptr,  // build_constrained_layout (not needed for bounded)
@[  end if]@
    false,  // is_dynamically_allocated
    &@(msg_namespace)::get_expected_message_size_@(msg_typename),
    &@(msg_namespace)::get_message_size_@(msg_typename),
    &@(msg_namespace)::construct_message_at_@(msg_typename),
    &@(msg_namespace)::cast_message_at_@(msg_typename),
    &@(msg_namespace)::deserialize_message_from_@(msg_typename),
    &@(msg_namespace)::serialize_message_into_@(msg_typename),
    &@(msg_namespace)::destroy_message_@(msg_typename),
    &@(msg_namespace)::serialize_fields_into_writer_@(msg_typename),
    &@(msg_namespace)::deserialize_fields_from_reader_@(msg_typename),
    &@(msg_namespace)::build_layout_fields_@(msg_typename),
    &@(msg_namespace)::release_message_@(msg_typename),
@[else]@
    nullptr,  // cached_layout
    nullptr,  // build_constrained_layout
    false,  // is_dynamically_allocated
    nullptr,  // get_expected_message_size
    &@(msg_namespace)::get_message_size_@(msg_typename),
    nullptr,  // construct_message_at
    nullptr,  // cast_message_at
    &@(msg_namespace)::deserialize_message_from_@(msg_typename),
    &@(msg_namespace)::serialize_message_into_@(msg_typename),
    nullptr,  // destroy_message (not needed for non-experimental)
    &@(msg_namespace)::serialize_fields_into_writer_@(msg_typename),
    &@(msg_namespace)::deserialize_fields_from_reader_@(msg_typename),
    &@(msg_namespace)::build_layout_fields_@(msg_typename),
    nullptr,  // release_message (not needed for non-experimental)
@[end if]@
  };
  
  static const rosidl_message_type_support_t handle = {
    rosidl_typesupport_xcdr_cpp__identifier,
    &callbacks,
    get_message_typesupport_handle_function,
    nullptr,  // get_type_hash_func
    nullptr,  // get_type_description_func
    nullptr,  // get_type_description_sources_func
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
  @(', '.join([package_name] + list(interface_path.parents[0].parts))),
  @(msg_typename))()
{
  return rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(full_msg_typename)>();
}

#ifdef __cplusplus
}
#endif
