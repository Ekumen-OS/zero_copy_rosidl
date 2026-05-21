@# Included from rosidl_generator_c/resource/idl__experimental_struct.h.em
@{
from rosidl_parser.definition import AbstractNestedType
from rosidl_parser.definition import AbstractSequence
from rosidl_parser.definition import AbstractString
from rosidl_parser.definition import AbstractWString
from rosidl_parser.definition import ACTION_FEEDBACK_SUFFIX
from rosidl_parser.definition import ACTION_GOAL_SUFFIX
from rosidl_parser.definition import ACTION_RESULT_SUFFIX
from rosidl_parser.definition import Array
from rosidl_parser.definition import BasicType
from rosidl_parser.definition import BOOLEAN_TYPE
from rosidl_parser.definition import BoundedSequence
from rosidl_parser.definition import CHARACTER_TYPES
from rosidl_parser.definition import FLOATING_POINT_TYPES
from rosidl_parser.definition import INTEGER_TYPES
from rosidl_parser.definition import NamespacedType
from rosidl_parser.definition import OCTET_TYPE
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX
from rosidl_parser.definition import UnboundedSequence
from rosidl_generator_c import basetype_to_c
from rosidl_generator_c import value_to_c
from rosidl_generator_c.experimental import BASIC_IDL_TYPES_TO_EXPERIMENTAL_C
from rosidl_generator_c.experimental import experimental_constraint_field
from rosidl_generator_c.experimental import experimental_constraints_are_equal_body
from rosidl_generator_c.experimental import experimental_field_declare_macro
from rosidl_generator_c.experimental import experimental_field_in_struct
from rosidl_generator_c.experimental import experimental_field_typename
from rosidl_generator_c.experimental import experimental_storage_field_declaration
from rosidl_generator_c.experimental import idl_structure_type_to_experimental_c_include_prefix
from rosidl_generator_c.experimental import idl_structure_type_to_experimental_c_typename
from rosidl_pycommon import convert_camel_case_to_lower_case_underscore

message_typename = idl_structure_type_to_experimental_c_typename(
    message.structure.namespaced_type)

# Check if this is a service or action internal message type
# Either passed explicitly as a parameter or inferred from name suffix
if 'is_service_or_action_member' in locals():
    is_service_or_action_type = is_service_or_action_member
else:
    is_service_or_action_type = False
}@
@[if message.constants]
// Constants defined in the message
@[  for constant in message.constants]@

/// Constant '@(constant.name)'.
@{comments = constant.get_comment_lines()}@
@[    if comments]@
/**
@[      for line in comments]@
@[        if line]@
  * @(line)
@[        else]@
  *
@[        end if]@
@[      end for]@
 */
@[    end if]@
@[    if isinstance(constant.type, BasicType)]@
@[        if constant.type.typename in (
                *INTEGER_TYPES, *CHARACTER_TYPES, OCTET_TYPE
        )]@
enum
{
  @(message_typename)__@(constant.name) = @(value_to_c(constant.type, constant.value))
};
@[        elif constant.type.typename in (*FLOATING_POINT_TYPES, BOOLEAN_TYPE)]@
static const @(basetype_to_c(constant.type)) @(message_typename)__@(constant.name) = @(value_to_c(constant.type, constant.value));
@[        else]@
@{assert False, 'Unhandled basic type: ' + str(constant.type)}@
@[        end if]@
@[    elif isinstance(constant.type, AbstractString)]@
static const char * const @(message_typename)__@(constant.name) = @(value_to_c(constant.type, constant.value));
@[    end if]@
@[  end for]@

@[end if]@
@# Collect include directives for NamespacedType member types
@{
from collections import OrderedDict
includes = OrderedDict()
for member in message.structure.members:
    type_ = member.type
    if isinstance(type_, AbstractNestedType):
        type_ = type_.value_type
    if isinstance(type_, NamespacedType):
        if (
            message.structure.namespaced_type.namespaces[-1] in ['action', 'srv'] and (
            type_.name.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
            type_.name.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX))
        ):
            typename = type_.name.rsplit('_', 1)[0]
            if typename == message.structure.namespaced_type.name.rsplit('_', 1)[0]:
                continue
        include_prefix = idl_structure_type_to_experimental_c_include_prefix(type_, 'detail')
        member_names = includes.setdefault(include_prefix + '__struct.h', [])
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
from rosidl_parser.definition import AbstractGenericString
upper_bounds = []
for member in message.structure.members:
    type_ = member.type
    if isinstance(type_, BoundedSequence):
        upper_bounds.append((
            member.name,
            '%s__%s__MAX_SIZE' % (message_typename, member.name),
            type_.maximum_size,
        ))
    if isinstance(type_, AbstractNestedType):
        type_ = type_.value_type
    if isinstance(type_, AbstractGenericString) and type_.has_maximum_size():
        upper_bounds.append((
            member.name,
            '%s__%s__MAX_STRING_SIZE' % (message_typename, member.name),
            type_.maximum_size,
        ))
}@
@[if upper_bounds]@

// constants for array fields with an upper bound
@[  for field_name, enum_name, enum_value in upper_bounds]@

// @(field_name)
enum
{
  @(enum_name) = @(enum_value)
};
@[  end for]@
@[end if]@
@{
declare_macros = [] 
for member in message.structure.members:
    macro_call = experimental_field_declare_macro(message_typename, member)
    if macro_call:
        declare_macros.append(macro_call)
}@
@[if declare_macros]@

@[  for macro in declare_macros]@
@(macro)
@[  end for]@

@[end if]@
@# ExternalStorage struct
typedef struct @(message_typename)__ExternalStorage_s
{
  /// Optional contiguous block encompassing all members.
  rosidl_memory_region_t block;

  /// Per-member storage descriptors.
  struct
  {
@[for member in message.structure.members]@
    @(experimental_storage_field_declaration(member.name, member.type));
@[end for]@
  } members;
} @(message_typename)__ExternalStorage;

@# Message struct
/// Experimental message struct defined in @(package_name)/@('/'.join(interface_path.parts)).
typedef struct @(message_typename)_s
{
@[for member in message.structure.members]@
  @(experimental_field_in_struct(message_typename, member));
@[end for]@

  /// External storage copy (embedded for lifetime management).
  /// Only valid if _has_external_storage is true.
  @(message_typename)__ExternalStorage _external_storage;

  /// Flag indicating whether _external_storage contains valid data.
  bool _has_external_storage;
} @(message_typename);
@[if not is_service_or_action_type]@

/// Array structure declaration for @(message_typename).
ROSIDL_RUNTIME_C__EXPERIMENTAL__ARRAY_STRUCTURE_DECLARE(@(message_typename)__Array, @(message_typename))

/// Unbounded sequence structure declaration for @(message_typename).
ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_DECLARE(@(message_typename)__Sequence, @(message_typename))

/// Bounded sequence structure declaration for @(message_typename).
ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DECLARE(@(message_typename)__BoundedSequence, @(message_typename))

@[end if]@
@# Collect constraint fields from all message members
@{
constraint_fields = []
for member in message.structure.members:
    constraint_field = experimental_constraint_field(message_typename, member)
    if constraint_field is not None:
        constraint_fields.append(constraint_field)
}@
@# Generate message-level Constraints struct
typedef struct @(message_typename)__Constraints_s
{
@[if constraint_fields]@
@[  for constraint_type, constraint_name in constraint_fields]@
  @(constraint_type) @(constraint_name);
@[  end for]@
@[else]@
  char _placeholder;
@[end if]@
} @(message_typename)__Constraints;

static inline bool
@(message_typename)__Constraints__are_equal(
  const @(message_typename)__Constraints * lhs,
  const @(message_typename)__Constraints * rhs)
{
  @(experimental_constraints_are_equal_body(constraint_fields))
}

@# Generate message-level SequenceConstraint struct
typedef struct @(message_typename)__SequenceConstraint_s
{
  size_t size;
  @(message_typename)__Constraints element;
} @(message_typename)__SequenceConstraint;

static inline bool
@(message_typename)__SequenceConstraint__are_equal(
  const @(message_typename)__SequenceConstraint * lhs,
  const @(message_typename)__SequenceConstraint * rhs)
{
  return lhs->size == rhs->size &&
         @(message_typename)__Constraints__are_equal(&lhs->element, &rhs->element);
}