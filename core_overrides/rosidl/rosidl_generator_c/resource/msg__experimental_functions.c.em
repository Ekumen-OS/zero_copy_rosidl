@# Included from rosidl_generator_c/resource/idl__experimental_functions.c.em
@{
from ast import literal_eval
from rosidl_parser.definition import AbstractNestedType
from rosidl_parser.definition import AbstractSequence
from rosidl_parser.definition import AbstractString
from rosidl_parser.definition import AbstractWString
from rosidl_parser.definition import ACTION_FEEDBACK_SUFFIX
from rosidl_parser.definition import ACTION_GOAL_SUFFIX
from rosidl_parser.definition import ACTION_RESULT_SUFFIX
from rosidl_parser.definition import Array
from rosidl_parser.definition import BasicType
from rosidl_parser.definition import BoundedSequence
from rosidl_parser.definition import NamespacedType
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX
from rosidl_generator_c import interface_path_to_string
from rosidl_generator_c import value_to_c
from rosidl_generator_c.experimental import BASIC_IDL_TYPES_TO_EXPERIMENTAL_C
from rosidl_generator_c.experimental import experimental_element_c_type
from rosidl_generator_c.experimental import experimental_field_define_macro
from rosidl_generator_c.experimental import experimental_field_needs_allocator
from rosidl_generator_c.experimental import experimental_field_typename
from rosidl_generator_c.experimental import idl_structure_type_to_experimental_c_include_prefix
from rosidl_generator_c.experimental import idl_structure_type_to_experimental_c_typename

message_typename = idl_structure_type_to_experimental_c_typename(
    message.structure.namespaced_type)

# Check if this is a service or action internal message type
is_service_or_action_type = (
    message.structure.namespaced_type.name.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
    message.structure.namespaced_type.name.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX) or
    message.structure.namespaced_type.name.endswith(ACTION_GOAL_SUFFIX) or
    message.structure.namespaced_type.name.endswith(ACTION_RESULT_SUFFIX) or
    message.structure.namespaced_type.name.endswith(ACTION_FEEDBACK_SUFFIX)
)
}@
@# Include directives for NamespacedType member functions
@{
from collections import OrderedDict
includes = OrderedDict()
for member in message.structure.members:
    type_ = member.type
    if isinstance(type_, AbstractNestedType):
        type_ = type_.value_type
    if isinstance(type_, NamespacedType):
        include_prefix = idl_structure_type_to_experimental_c_include_prefix(type_, 'detail')
        member_names = includes.setdefault(include_prefix + '__functions.h', [])
        member_names.append(member.name)
}@
@[if includes]@

// Include directives for member types
@[    for header_file, member_names in includes.items()]@
@[        for member_name in member_names]@
// Member '@(member_name)'
@[        end for]@
@[        if header_file in include_directives]@
// already included above
// @
@[        else]@
@{include_directives.add(header_file)}@
@[        end if]@
#include "@(header_file)"
@[    end for]@
@[end if]@
@{
define_macros = [] 
for member in message.structure.members:
    macro_call = experimental_field_define_macro(message_typename, member)
    if macro_call:
        define_macros.append(macro_call)
}@
@[if define_macros]@

@[  for macro in define_macros]@
@(macro)
@[  end for]@

@[end if]@

@# __init / __init_with_allocator
static bool
@(message_typename)__init_with_allocator(
  @(message_typename) * msg,
  const rcutils_allocator_t * allocator)
{
  if (!msg) {
    return false;
  }
@{
lines = []
for member in message.structure.members:
    field_tn = experimental_field_typename(message_typename, member.name, member.type)
    type_ = member.type

    lines.append('// ' + member.name)

    if isinstance(type_, BasicType):
        lines.append('if (!{}__init_with_options(&msg->{}, NULL)) {{'.format(field_tn, member.name))
        lines.append('  {}__fini(msg);'.format(message_typename))
        lines.append('  return false;')
        lines.append('}')

    elif isinstance(type_, (AbstractString, AbstractWString)):
        # Build init_with_options call with appropriate parameters
        lines.append('{')
        lines.append('  rosidl_string_init_options_t _opts = {')
        lines.append('    .allocator = allocator,')
        lines.append('    .external_storage = NULL,')
        lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
        lines.append('  };')
        if type_.has_maximum_size():
            lines.append('  if (!{}__init_with_options(&msg->{}, {}U, &_opts)) {{'.format(
                field_tn, member.name, type_.maximum_size))
        else:
            lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(field_tn, member.name))
        lines.append('    {}__fini(msg);'.format(message_typename))
        lines.append('    return false;')
        lines.append('  }')
        lines.append('}')

    elif isinstance(type_, NamespacedType):
        sub_tn = idl_structure_type_to_experimental_c_typename(type_)
        lines.append('{')
        lines.append('  {}__InitOptions _opts = {{'.format(sub_tn))
        lines.append('    .init_mode = ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_SKIP,')
        lines.append('    .allocator = allocator,')
        lines.append('    .external_storage = NULL,')
        lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
        lines.append('  };')
        lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(sub_tn, member.name))
        lines.append('    {}__fini(msg);'.format(message_typename))
        lines.append('    return false;')
        lines.append('  }')
        lines.append('}')

    elif isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            lines.append('if (!{}__init_with_options(&msg->{}, NULL)) {{'.format(field_tn, member.name))
            lines.append('  {}__fini(msg);'.format(message_typename))
            lines.append('  return false;')
            lines.append('}')
        else:
            # Arrays of complex elements: use InitOptions
            lines.append('{')
            lines.append('  {}__InitOptions _opts = {{'.format(field_tn))
            lines.append('    .element_allocator = allocator,')
            lines.append('    .external_element_storage = NULL,')
            lines.append('    .external_storage = NULL,')
            lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
            lines.append('  };')
            if isinstance(vt, (AbstractString, AbstractWString)) and vt.has_maximum_size():
                lines.append('  if (!{}__init_with_options(&msg->{}, {}U, &_opts)) {{'.format(
                    field_tn, member.name, vt.maximum_size))
            else:
                lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(field_tn, member.name))
            lines.append('    {}__fini(msg);'.format(message_typename))
            lines.append('    return false;')
            lines.append('  }')
            lines.append('}')

    elif isinstance(type_, AbstractSequence):
        vt = type_.value_type
        # Build InitOptions for sequences
        lines.append('{')
        lines.append('  {}__InitOptions _opts = {{'.format(field_tn))
        lines.append('    .allocator = allocator,')
        if not isinstance(vt, BasicType):
            # Complex element sequences need element storage options
            lines.append('    .external_element_storage = NULL,')
        lines.append('    .external_storage = NULL,')
        lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
        lines.append('  };')
        # Determine init parameters based on sequence and element bounds
        if isinstance(type_, BoundedSequence):
            if isinstance(vt, (AbstractString, AbstractWString)) and vt.has_maximum_size():
                # Bounded sequence of bounded strings: sequence_bound, element_bound
                lines.append('  if (!{}__init_with_options(&msg->{}, {}U, {}U, &_opts)) {{'.format(
                    field_tn, member.name, type_.maximum_size, vt.maximum_size))
            else:
                # Bounded sequence of other types: just sequence_bound
                lines.append('  if (!{}__init_with_options(&msg->{}, {}U, &_opts)) {{'.format(
                    field_tn, member.name, type_.maximum_size))
        else:  # UnboundedSequence
            if isinstance(vt, (AbstractString, AbstractWString)) and vt.has_maximum_size():
                # Unbounded sequence of bounded strings: element_bound
                lines.append('  if (!{}__init_with_options(&msg->{}, {}U, &_opts)) {{'.format(
                    field_tn, member.name, vt.maximum_size))
            else:
                # Unbounded sequence of other types: no bounds
                lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(field_tn, member.name))
        lines.append('    {}__fini(msg);'.format(message_typename))
        lines.append('    return false;')
        lines.append('  }')
        lines.append('}')

    lines.append('')

for line in lines:
    if line:
        print('  ' + line)
    else:
        print('')
}@
  return true;
}

@# __init_from_storage (must come first, called by __init_with_options)
static bool
@(message_typename)__init_from_storage(
  @(message_typename) * msg,
  const @(message_typename)__ExternalStorage * storage)
{
  if (!msg || !storage) {
    return false;
  }
@{
lines = []
for member in message.structure.members:
    field_tn = experimental_field_typename(message_typename, member.name, member.type)
    type_ = member.type

    lines.append('// ' + member.name)

    if isinstance(type_, BasicType):
        lines.append('{')
        lines.append('  rosidl_scalar_init_options_t _opts = {')
        lines.append('    .external_memory = &storage->members.{},'.format(member.name))
        lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
        lines.append('  };')
        lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(
            field_tn, member.name))
        lines.append('    {}__fini(msg);'.format(message_typename))
        lines.append('    return false;')
        lines.append('  }')
        lines.append('}')

    elif isinstance(type_, (AbstractString, AbstractWString)):
        lines.append('{')
        lines.append('  rosidl_string_init_options_t _opts = {')
        lines.append('    .allocator = NULL,')
        lines.append('    .external_storage = &storage->members.{},'.format(member.name))
        lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
        lines.append('  };')
        if type_.has_maximum_size():
            lines.append('  if (!{}__init_with_options(&msg->{}, {}U, &_opts)) {{'.format(
                field_tn, member.name, type_.maximum_size))
        else:
            lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(
                field_tn, member.name))
        lines.append('    {}__fini(msg);'.format(message_typename))
        lines.append('    return false;')
        lines.append('  }')
        lines.append('}')

    elif isinstance(type_, NamespacedType):
        sub_tn = idl_structure_type_to_experimental_c_typename(type_)
        lines.append('{')
        lines.append('  {}__InitOptions _opts = {{'.format(sub_tn))
        lines.append('    .init_mode = ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_SKIP,')
        lines.append('    .allocator = NULL,')
        lines.append('    .external_storage = &storage->members.{},'.format(member.name))
        lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
        lines.append('  };')
        lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(sub_tn, member.name))
        lines.append('    {}__fini(msg);'.format(message_typename))
        lines.append('    return false;')
        lines.append('  }')
        lines.append('}')

    elif isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            lines.append('{')
            lines.append('  rosidl_primitive_array_init_options_t _opts = {')
            lines.append('    .external_storage = &storage->members.{},'.format(member.name))
            lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
            lines.append('  };')
            lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(
                field_tn, member.name))
            lines.append('    {}__fini(msg);'.format(message_typename))
            lines.append('    return false;')
            lines.append('  }')
            lines.append('}')
        else:
            # Complex element arrays: use InitOptions with external storage
            lines.append('{')
            lines.append('  {}__InitOptions _opts = {{'.format(field_tn))
            lines.append('    .element_allocator = NULL,')
            lines.append('    .external_element_storage = storage->members.{},'.format(member.name))
            lines.append('    .external_storage = NULL,')
            lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
            lines.append('  };')
            if isinstance(vt, (AbstractString, AbstractWString)) and vt.has_maximum_size():
                lines.append('  if (!{}__init_with_options(&msg->{}, {}U, &_opts)) {{'.format(
                    field_tn, member.name, vt.maximum_size))
            else:
                lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(
                    field_tn, member.name))
            lines.append('    {}__fini(msg);'.format(message_typename))
            lines.append('    return false;')
            lines.append('  }')
            lines.append('}')

    elif isinstance(type_, AbstractSequence):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            # Primitive sequence: use InitOptions with external storage
            lines.append('{')
            lines.append('  rosidl_primitive_sequence_init_options_t _opts = {')
            lines.append('    .allocator = NULL,')
            lines.append('    .external_storage = &storage->members.{},'.format(member.name))
            lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
            lines.append('  };')
            if isinstance(type_, BoundedSequence):
                lines.append('  if (!{}__init_with_options(&msg->{}, {}U, &_opts)) {{'.format(
                    field_tn, member.name, type_.maximum_size))
            else:
                lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(
                    field_tn, member.name))
            lines.append('    {}__fini(msg);'.format(message_typename))
            lines.append('    return false;')
            lines.append('  }')
            lines.append('}')
        else:
            # Complex element sequence: use InitOptions with region + element storage pool
            lines.append('{')
            lines.append('  {}__InitOptions _opts = {{'.format(field_tn))
            lines.append('    .allocator = NULL,')
            lines.append('    .external_element_storage = storage->members.{}.element_storage_pool,'.format(member.name))
            lines.append('    .external_storage = &storage->members.{}.region,'.format(member.name))
            lines.append('    .reserved = {NULL, NULL, NULL, NULL}')
            lines.append('  };')
            # Pass bounds based on sequence and element types
            if isinstance(type_, BoundedSequence):
                if isinstance(vt, (AbstractString, AbstractWString)) and vt.has_maximum_size():
                    # Bounded sequence of bounded strings: sequence_bound, element_bound
                    lines.append('  if (!{}__init_with_options(&msg->{}, {}U, {}U, &_opts)) {{'.format(
                        field_tn, member.name, type_.maximum_size, vt.maximum_size))
                else:
                    # Bounded sequence of other complex types: just sequence_bound
                    lines.append('  if (!{}__init_with_options(&msg->{}, {}U, &_opts)) {{'.format(
                        field_tn, member.name, type_.maximum_size))
            else:  # UnboundedSequence
                if isinstance(vt, (AbstractString, AbstractWString)) and vt.has_maximum_size():
                    # Unbounded sequence of bounded strings: element_bound
                    lines.append('  if (!{}__init_with_options(&msg->{}, {}U, &_opts)) {{'.format(
                        field_tn, member.name, vt.maximum_size))
                else:
                    # Unbounded sequence of other types: no bounds
                    lines.append('  if (!{}__init_with_options(&msg->{}, &_opts)) {{'.format(field_tn, member.name))
            lines.append('    {}__fini(msg);'.format(message_typename))
            lines.append('    return false;')
            lines.append('  }')
            lines.append('}')

    lines.append('')

for line in lines:
    if line:
        print('  ' + line)
    else:
        print('')
}@
  return true;
}

@# __reset - Value initialization based on init_mode
bool
@(message_typename)__reset(
  @(message_typename) * msg,
  rosidl_runtime_c__experimental__message_initialization_t init_mode)
{
  if (!msg) {
    return false;
  }

  // Early exit for SKIP mode
  if (init_mode == ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_SKIP) {
    return true;
  }
@{
reset_lines = []
for member in message.structure.members:
    field_tn = experimental_field_typename(message_typename, member.name, member.type)
    type_ = member.type
    has_default = member.has_annotation('default')
    
    reset_lines.append('')
    reset_lines.append('// ' + member.name)
    
    if isinstance(type_, BasicType):
        # BasicType fields
        if has_default:
            default_val = value_to_c(type_, member.get_annotation_value('default')['value'])
            reset_lines.append('switch (init_mode) {')
            reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL:')
            reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_DEFAULTS_ONLY:')
            reset_lines.append('    msg->{}.value->data = {};'.format(member.name, default_val))
            reset_lines.append('    break;')
            reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ZERO:')
            reset_lines.append('    msg->{}.value->data = 0;'.format(member.name))
            reset_lines.append('    break;')
            reset_lines.append('  default:')
            reset_lines.append('    break;')
            reset_lines.append('}')
        else:
            # No default: ALL and ZERO set to zero
            reset_lines.append('if (init_mode == ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL ||')
            reset_lines.append('    init_mode == ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ZERO) {')
            reset_lines.append('  msg->{}.value->data = 0;'.format(member.name))
            reset_lines.append('}')
    
    elif isinstance(type_, (AbstractString, AbstractWString)):
        # String fields
        if has_default:
            default_val = value_to_c(type_, member.get_annotation_value('default')['value'])
            reset_lines.append('switch (init_mode) {')
            reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL:')
            reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_DEFAULTS_ONLY:')
            reset_lines.append('    if (!{}__assign(&msg->{}, {})) {{'.format(field_tn, member.name, default_val))
            reset_lines.append('      return false;')
            reset_lines.append('    }')
            reset_lines.append('    break;')
            reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ZERO:')
            reset_lines.append('    msg->{}.value[0] = \'\\0\';'.format(member.name))
            reset_lines.append('    msg->{}.size = 0;'.format(member.name))
            reset_lines.append('    break;')
            reset_lines.append('  default:')
            reset_lines.append('    break;')
            reset_lines.append('}')
        else:
            # No default: ALL and ZERO set to empty string
            reset_lines.append('if (init_mode == ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL ||')
            reset_lines.append('    init_mode == ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ZERO) {')
            reset_lines.append('  msg->{}.value[0] = \'\\0\';'.format(member.name))
            reset_lines.append('  msg->{}.size = 0;'.format(member.name))
            reset_lines.append('}')
    
    elif isinstance(type_, NamespacedType):
        # Sub-message: always propagate init_mode recursively
        sub_tn = idl_structure_type_to_experimental_c_typename(type_)
        reset_lines.append('if (!{}__reset(&msg->{}, init_mode)) {{'.format(sub_tn, member.name))
        reset_lines.append('  return false;')
        reset_lines.append('}')
    
    elif isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            # Array of BasicType
            if has_default:
                default_vals = literal_eval(member.get_annotation_value('default')['value'])
                reset_lines.append('switch (init_mode) {')
                reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL:')
                reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_DEFAULTS_ONLY:')
                for i, dv in enumerate(default_vals):
                    reset_lines.append('    msg->{}.value->data[{}] = {};'.format(
                        member.name, i, value_to_c(vt, dv)))
                reset_lines.append('    break;')
                reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ZERO:')
                reset_lines.append('    for (size_t i = 0; i < {}U; ++i) {{'.format(type_.size))
                reset_lines.append('      msg->{}.value->data[i] = 0;'.format(member.name))
                reset_lines.append('    }')
                reset_lines.append('    break;')
                reset_lines.append('  default:')
                reset_lines.append('    break;')
                reset_lines.append('}')
            else:
                # No default
                reset_lines.append('if (init_mode == ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL ||')
                reset_lines.append('    init_mode == ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ZERO) {')
                reset_lines.append('  for (size_t i = 0; i < {}U; ++i) {{'.format(type_.size))
                reset_lines.append('    msg->{}.value->data[i] = 0;'.format(member.name))
                reset_lines.append('  }')
                reset_lines.append('}')
        elif isinstance(vt, (AbstractString, AbstractWString)):
            # Array of Strings
            elem_type = experimental_element_c_type(message_typename, member.name, vt)
            if has_default:
                default_vals = literal_eval(member.get_annotation_value('default')['value'])
                reset_lines.append('switch (init_mode) {')
                reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL:')
                reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_DEFAULTS_ONLY:')
                for i, dv in enumerate(default_vals):
                    reset_lines.append('    if (!{}__assign(&msg->{}.value->data[{}], {})) {{'.format(
                        elem_type, member.name, i, value_to_c(vt, dv)))
                    reset_lines.append('      return false;')
                    reset_lines.append('    }')
                reset_lines.append('    break;')
                reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ZERO:')
                reset_lines.append('    for (size_t i = 0; i < {}U; ++i) {{'.format(type_.size))
                reset_lines.append('      msg->{}.value->data[i].value[0] = \'\\0\';'.format(member.name))
                reset_lines.append('      msg->{}.value->data[i].size = 0;'.format(member.name))
                reset_lines.append('    }')
                reset_lines.append('    break;')
                reset_lines.append('  default:')
                reset_lines.append('    break;')
                reset_lines.append('}')
            else:
                # No default
                reset_lines.append('if (init_mode == ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL ||')
                reset_lines.append('    init_mode == ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ZERO) {')
                reset_lines.append('  for (size_t i = 0; i < {}U; ++i) {{'.format(type_.size))
                reset_lines.append('    msg->{}.value->data[i].value[0] = \'\\0\';'.format(member.name))
                reset_lines.append('    msg->{}.value->data[i].size = 0;'.format(member.name))
                reset_lines.append('  }')
                reset_lines.append('}')
        elif isinstance(vt, NamespacedType):
            # Array of sub-messages: propagate to all elements
            sub_tn = idl_structure_type_to_experimental_c_typename(vt)
            reset_lines.append('for (size_t i = 0; i < {}U; ++i) {{'.format(type_.size))
            reset_lines.append('  if (!{}__reset(&msg->{}.value->data[i], init_mode)) {{'.format(sub_tn, member.name))
            reset_lines.append('    return false;')
            reset_lines.append('  }')
            reset_lines.append('}')
    
    elif isinstance(type_, AbstractSequence):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            # Sequence of BasicType
            if has_default:
                default_vals = literal_eval(member.get_annotation_value('default')['value'])
                reset_lines.append('switch (init_mode) {')
                reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL:')
                reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_DEFAULTS_ONLY:')
                reset_lines.append('    if (!{}__resize(&msg->{}, {}U)) {{'.format(
                    field_tn, member.name, len(default_vals)))
                reset_lines.append('      return false;')
                reset_lines.append('    }')
                for i, dv in enumerate(default_vals):
                    reset_lines.append('    msg->{}.value[{}] = {};'.format(
                        member.name, i, value_to_c(vt, dv)))
                reset_lines.append('    break;')
                reset_lines.append('  default:')
                reset_lines.append('    break;')
                reset_lines.append('}')
            # No else: empty sequence is the zero state, already set structurally
        elif isinstance(vt, (AbstractString, AbstractWString)):
            # Sequence of Strings
            string_type = 'WString' if isinstance(vt, AbstractWString) else 'String'
            if has_default:
                default_vals = literal_eval(member.get_annotation_value('default')['value'])
                reset_lines.append('switch (init_mode) {')
                reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL:')
                reset_lines.append('  case ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_DEFAULTS_ONLY:')
                reset_lines.append('    if (!{}__resize(&msg->{}, {}U)) {{'.format(
                    field_tn, member.name, len(default_vals)))
                reset_lines.append('      return false;')
                reset_lines.append('    }')
                for i, dv in enumerate(default_vals):
                    reset_lines.append('    if (!rosidl_runtime_c__experimental__{}__assign(&msg->{}.value[{}], {})) {{'.format(
                        string_type, member.name, i, value_to_c(vt, dv)))
                    reset_lines.append('      return false;')
                    reset_lines.append('    }')
                reset_lines.append('    break;')
                reset_lines.append('  default:')
                reset_lines.append('    break;')
                reset_lines.append('}')
            # No else: empty sequence is the zero state
        elif isinstance(vt, NamespacedType):
            # Sequence of sub-messages: propagate to existing elements
            sub_tn = idl_structure_type_to_experimental_c_typename(vt)
            reset_lines.append('for (size_t i = 0; i < msg->{}.size; ++i) {{'.format(member.name))
            reset_lines.append('  if (!{}__reset(&msg->{}.value[i], init_mode)) {{'.format(sub_tn, member.name))
            reset_lines.append('    return false;')
            reset_lines.append('  }')
            reset_lines.append('}')

for line in reset_lines:
    if line:
        print('  ' + line)
    else:
        print('')
}@
  return true;
}

@# __init_with_options
bool
@(message_typename)__init_with_options(
  @(message_typename) * msg,
  const @(message_typename)__InitOptions * options)
{
  if (!msg) {
    return false;
  }

  // Determine init_mode (default to ALL if not specified)
  rosidl_runtime_c__experimental__message_initialization_t init_mode =
    ROSIDL_RUNTIME_C__EXPERIMENTAL__MESSAGE_INITIALIZATION_ALL;
  if (options && options->init_mode != 0) {
    init_mode = options->init_mode;
  }

  // Clear external storage flag initially
  msg->_has_external_storage = false;

  // Phase 1: Structural initialization
  bool success;
  if (options && options->external_storage) {
    // Copy external storage into message (embedded copy)
    msg->_external_storage = *options->external_storage;
    msg->_has_external_storage = true;
    
    // Initialize from the embedded copy
    success = @(message_typename)__init_from_storage(msg, &msg->_external_storage);
  } else {
    // Otherwise use allocator-based initialization
    const rcutils_allocator_t * allocator = 
      (options && options->allocator) ? options->allocator : NULL;
    success = @(message_typename)__init_with_allocator(msg, allocator);
  }

  if (!success) {
    return false;
  }

  // Phase 2: Value initialization based on init_mode
  if (!@(message_typename)__reset(msg, init_mode)) {
    @(message_typename)__fini(msg);
    return false;
  }

  return true;
}

@# __init
bool
@(message_typename)__init(@(message_typename) * msg)
{
  return @(message_typename)__init_with_options(msg, NULL);
}

@# __fini
void
@(message_typename)__fini(@(message_typename) * msg)
{
  if (!msg) {
    return;
  }
@{
lines = []
for member in message.structure.members:
    field_tn = experimental_field_typename(message_typename, member.name, member.type)
    type_ = member.type

    lines.append('// ' + member.name)
    if isinstance(type_, BasicType):
        lines.append('{}__fini(&msg->{});'.format(field_tn, member.name))
    elif isinstance(type_, (AbstractString, AbstractWString)):
        lines.append('{}__fini(&msg->{});'.format(field_tn, member.name))
    elif isinstance(type_, NamespacedType):
        sub_tn = idl_structure_type_to_experimental_c_typename(type_)
        lines.append('{}__fini(&msg->{});'.format(sub_tn, member.name))
    elif isinstance(type_, Array):
        lines.append('{}__fini(&msg->{});'.format(field_tn, member.name))
    elif isinstance(type_, AbstractSequence):
        lines.append('{}__fini(&msg->{});'.format(field_tn, member.name))
    lines.append('')

for line in lines:
    if line:
        print('  ' + line)
    else:
        print('')
}@
  // Clear external storage flag
  msg->_has_external_storage = false;
  return;
}

@# __create / __destroy
@(message_typename) *
@(message_typename)__create()
{
  rcutils_allocator_t alloc = rcutils_get_default_allocator();
  @(message_typename) * msg =
    (@(message_typename) *)alloc.zero_allocate(1U, sizeof(@(message_typename)), alloc.state);
  if (!msg) {
    return NULL;
  }
  if (!@(message_typename)__init(msg)) {
    alloc.deallocate(msg, alloc.state);
    return NULL;
  }
  return msg;
}

void
@(message_typename)__destroy(@(message_typename) * msg)
{
  if (msg) {
    @(message_typename)__fini(msg);
  }
  rcutils_allocator_t alloc = rcutils_get_default_allocator();
  alloc.deallocate(msg, alloc.state);
}

@# __are_equal
bool
@(message_typename)__are_equal(
  const @(message_typename) * lhs,
  const @(message_typename) * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
@[for member in message.structure.members]@
@{
field_tn = experimental_field_typename(message_typename, member.name, member.type)
type_ = member.type
}@
  // @(member.name)
@[  if isinstance(type_, BasicType)]@
  if (lhs->@(member.name).value->data != rhs->@(member.name).value->data) {
    return false;
  }
@[  elif isinstance(type_, (AbstractString, AbstractWString, AbstractSequence))]@
  if (!@(field_tn)__are_equal(&lhs->@(member.name), &rhs->@(member.name))) {
    return false;
  }
@[  elif isinstance(type_, NamespacedType)]@
@{sub_tn = idl_structure_type_to_experimental_c_typename(type_)}@
  if (!@(sub_tn)__are_equal(&lhs->@(member.name), &rhs->@(member.name))) {
    return false;
  }
@[  else]@
  if (!@(field_tn)__are_equal(&lhs->@(member.name), &rhs->@(member.name))) {
    return false;
  }
@[  end if]@
@[end for]@
  return true;
}

@# __copy
bool
@(message_typename)__copy(
  const @(message_typename) * input,
  @(message_typename) * output)
{
  if (!input || !output) {
    return false;
  }
@[for member in message.structure.members]@
@{
field_tn = experimental_field_typename(message_typename, member.name, member.type)
type_ = member.type
}@
  // @(member.name)
@[  if isinstance(type_, BasicType)]@
  output->@(member.name).value->data = input->@(member.name).value->data;
@[  elif isinstance(type_, (AbstractString, AbstractWString, AbstractSequence))]@
  if (!@(field_tn)__copy(&input->@(member.name), &output->@(member.name))) {
    return false;
  }
@[  elif isinstance(type_, NamespacedType)]@
@{sub_tn = idl_structure_type_to_experimental_c_typename(type_)}@
  if (!@(sub_tn)__copy(&input->@(member.name), &output->@(member.name))) {
    return false;
  }
@[  else]@
  if (!@(field_tn)__copy(&input->@(member.name), &output->@(member.name))) {
    return false;
  }
@[  end if]@
@[end for]@
  return true;
}
@[if not is_service_or_action_type]@

ROSIDL_RUNTIME_C__EXPERIMENTAL__ARRAY_STRUCTURE_DEFINE(@(message_typename)__Array, @(message_typename))

ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_DEFINE(@(message_typename)__Sequence, @(message_typename))

ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DEFINE(@(message_typename)__BoundedSequence, @(message_typename))
@[end if]@