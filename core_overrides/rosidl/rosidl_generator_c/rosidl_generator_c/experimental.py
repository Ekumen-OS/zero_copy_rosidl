# Copyright 2026 Ekumen, Inc.
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

"""Experimental code generation helpers for rosidl_generator_c."""

from rosidl_parser.definition import AbstractSequence
from rosidl_parser.definition import AbstractString
from rosidl_parser.definition import AbstractWString
from rosidl_parser.definition import Array
from rosidl_parser.definition import BasicType
from rosidl_parser.definition import BoundedSequence
from rosidl_parser.definition import NamespacedType
from rosidl_parser.definition import UnboundedSequence
from rosidl_pycommon import convert_camel_case_to_lower_case_underscore

# Mapping from IDL basic type names to C types for experimental message members.
# 'char' maps to 'char' (matching rosidl_runtime_c__experimental__Char) and
# 'wchar' maps to 'char16_t' (matching rosidl_runtime_c__experimental__WChar),
# unlike the regular C generator ('signed char' / 'uint16_t').
BASIC_IDL_TYPES_TO_EXPERIMENTAL_C = {
    "float": "float",
    "double": "double",
    "long double": "long double",
    "char": "char",
    "wchar": "char16_t",
    "boolean": "bool",
    "octet": "uint8_t",
    "uint8": "uint8_t",
    "int8": "int8_t",
    "uint16": "uint16_t",
    "int16": "int16_t",
    "uint32": "uint32_t",
    "int32": "int32_t",
    "uint64": "uint64_t",
    "int64": "int64_t",
}

# Mapping from IDL basic type names to pre-declared scalar type names in
# rosidl_runtime_c/experimental/scalar.h.  These types are always available, so
# scalar fields can use them directly without per-field redeclaration.
BASIC_IDL_TYPES_TO_EXPERIMENTAL_C_SCALAR = {
    "float": "rosidl_runtime_c__experimental__Float",
    "double": "rosidl_runtime_c__experimental__Double",
    "long double": "rosidl_runtime_c__experimental__LongDouble",
    "char": "rosidl_runtime_c__experimental__Char",
    "wchar": "rosidl_runtime_c__experimental__WChar",
    "boolean": "rosidl_runtime_c__experimental__Boolean",
    "octet": "rosidl_runtime_c__experimental__UInt8",
    "uint8": "rosidl_runtime_c__experimental__UInt8",
    "int8": "rosidl_runtime_c__experimental__Int8",
    "uint16": "rosidl_runtime_c__experimental__UInt16",
    "int16": "rosidl_runtime_c__experimental__Int16",
    "uint32": "rosidl_runtime_c__experimental__UInt32",
    "int32": "rosidl_runtime_c__experimental__Int32",
    "uint64": "rosidl_runtime_c__experimental__UInt64",
    "int64": "rosidl_runtime_c__experimental__Int64",
}

# Mapping from IDL basic type names to pre-declared primitive sequence type names in
# rosidl_runtime_c/experimental/sequence.h.  These types are always available, so
# fields of type sequence<primitive> can use them directly without per-field redeclaration.
BASIC_IDL_TYPES_TO_EXPERIMENTAL_C_SEQUENCE = {
    "float": "rosidl_runtime_c__experimental__Float__Sequence",
    "double": "rosidl_runtime_c__experimental__Double__Sequence",
    "long double": "rosidl_runtime_c__experimental__LongDouble__Sequence",
    "char": "rosidl_runtime_c__experimental__Char__Sequence",
    "wchar": "rosidl_runtime_c__experimental__WChar__Sequence",
    "boolean": "rosidl_runtime_c__experimental__Boolean__Sequence",
    "octet": "rosidl_runtime_c__experimental__UInt8__Sequence",
    "uint8": "rosidl_runtime_c__experimental__UInt8__Sequence",
    "int8": "rosidl_runtime_c__experimental__Int8__Sequence",
    "uint16": "rosidl_runtime_c__experimental__UInt16__Sequence",
    "int16": "rosidl_runtime_c__experimental__Int16__Sequence",
    "uint32": "rosidl_runtime_c__experimental__UInt32__Sequence",
    "int32": "rosidl_runtime_c__experimental__Int32__Sequence",
    "uint64": "rosidl_runtime_c__experimental__UInt64__Sequence",
    "int64": "rosidl_runtime_c__experimental__Int64__Sequence",
}

# Mapping from IDL basic type names to pre-declared primitive array type names in
# rosidl_runtime_c/experimental/array.h.  These types are always available for creating
# fixed-size array aliases.
BASIC_IDL_TYPES_TO_EXPERIMENTAL_C_ARRAY = {
    "float": "rosidl_runtime_c__experimental__FloatArray",
    "double": "rosidl_runtime_c__experimental__DoubleArray",
    "long double": "rosidl_runtime_c__experimental__LongDoubleArray",
    "char": "rosidl_runtime_c__experimental__CharArray",
    "wchar": "rosidl_runtime_c__experimental__WCharArray",
    "boolean": "rosidl_runtime_c__experimental__BooleanArray",
    "octet": "rosidl_runtime_c__experimental__UInt8Array",
    "uint8": "rosidl_runtime_c__experimental__UInt8Array",
    "int8": "rosidl_runtime_c__experimental__Int8Array",
    "uint16": "rosidl_runtime_c__experimental__UInt16Array",
    "int16": "rosidl_runtime_c__experimental__Int16Array",
    "uint32": "rosidl_runtime_c__experimental__UInt32Array",
    "int32": "rosidl_runtime_c__experimental__Int32Array",
    "uint64": "rosidl_runtime_c__experimental__UInt64Array",
    "int64": "rosidl_runtime_c__experimental__Int64Array",
}


def idl_structure_type_to_experimental_c_typename(namespaced_type):
    """Return the C experimental typename for a namespaced IDL type.

    Inserts 'experimental' between the namespaces and the type name.
    Example: std_msgs::msg::String  ->  std_msgs__msg__experimental__String
    """
    parts = list(namespaced_type.namespaces) + ["experimental", namespaced_type.name]
    return "__".join(parts)


def idl_structure_type_to_experimental_c_include_prefix(
    namespaced_type, subdirectory=None
):
    """Return the include path prefix for an experimental C message type.

    Example: std_msgs::msg::String, 'detail'  ->  std_msgs/msg/experimental/detail/string
    """
    parts = [
        convert_camel_case_to_lower_case_underscore(x)
        for x in namespaced_type.namespaced_name()
    ]
    # Insert 'experimental' before the last component (type name)
    parts[-1:-1] = ["experimental"]
    if subdirectory is not None:
        parts[-1:-1] = [subdirectory]
    include_prefix = "/".join(parts)
    # Strip service / action suffixes (same as the regular C generator)
    # Note: convert_camel_case_to_lower_case_underscore converts SendGoal -> send_goal (single _)
    for suffix in (
        "__request",
        "__response",
        "__goal",
        "__result",
        "__feedback",
        "__send_goal",
        "__get_result",
    ):
        if include_prefix.endswith(suffix):
            include_prefix = include_prefix[: -len(suffix)]
            break
    return include_prefix


def experimental_field_typename(
    message_experimental_typename, member_name, member_type=None
):
    """Return the per-field typedef name for a message member.

    Always returns the per-field typedef name. The experimental_field_declare_macro
    function will generate appropriate ALIAS macros for predefined types.

    Example: pkg__msg__experimental__Msg, foo, BasicType('float')  ->  pkg__msg__experimental__Msg__foo
    Example: pkg__msg__experimental__Msg, bar, NamespacedType(...)  ->  pkg__msg__experimental__Msg__bar
    """
    return "{}__{}".format(message_experimental_typename, member_name)


def experimental_element_c_type(message_experimental_typename, member_name, vt):
    """Return the C type name for elements of a complex-element array or sequence.

    For bounded strings, use the base runtime type directly (no per-field typedef needed).
    For unbounded strings use the pre-declared runtime type.
    For NamespacedType the experimental typename is returned.
    """
    if isinstance(vt, AbstractString):
        if vt.has_maximum_size():
            return "rosidl_runtime_c__experimental__BoundedString"
        return "rosidl_runtime_c__experimental__String"
    if isinstance(vt, AbstractWString):
        if vt.has_maximum_size():
            return "rosidl_runtime_c__experimental__BoundedWString"
        return "rosidl_runtime_c__experimental__WString"
    if isinstance(vt, NamespacedType):
        return idl_structure_type_to_experimental_c_typename(vt)
    assert False, "Unexpected element type: " + str(vt)


def experimental_field_declare_macro(message_typename, member):
    """Return the single DECLARE/ALIAS macro call for this member's per-field type.

    Generates type aliases for predefined types to ensure every field has a consistent
    per-field typename. Only generates new type declarations for specialized types
    (bounded strings, arrays, sequences with bounds or bounded elements).

    Returns None for NamespacedType members (sub-message type used directly).

    Rules
    -----
    BasicType          -> SCALAR_ALIAS(field_tn, predefined_scalar_type)
    String             -> STRING_ALIAS(field_tn, String)
    String<N>          -> BASIC_BOUNDED_STRING_DECLARE(field_tn, char, N)
    WString            -> STRING_ALIAS(field_tn, WString)
    WString<N>         -> BASIC_BOUNDED_STRING_DECLARE(field_tn, char16_t, N)
    NamespacedType     -> None
    Array<Basic,N>     -> PRIMITIVE_ARRAY_DECLARE(field_tn, c_type, N)
    Array<String,N>    -> ARRAY_DECLARE(field_tn, String, N)
    Array<String<B>,N> -> BASIC_BOUNDED_STRING_DECLARE(elem_tn, char, B)
                         ARRAY_DECLARE(field_tn, elem_tn, N)
    Array<Sub,N>       -> ARRAY_DECLARE(field_tn, sub_tn, N)
    Seq<Basic>         -> PRIMITIVE_SEQUENCE_ALIAS(field_tn, predefined_seq)
    Seq<Basic,B>       -> PRIMITIVE_BOUNDED_SEQUENCE_DECLARE(field_tn, c_type, B)
    Seq<String>        -> SEQUENCE_ALIAS(field_tn, StringSequence)
    Seq<String,B>      -> BOUNDED_SEQUENCE_DECLARE(field_tn, String, B)
    Seq<String<B>>     -> BASIC_BOUNDED_STRING_DECLARE(elem_tn, char, B)
                         SEQUENCE_DECLARE(field_tn, elem_tn)
    Seq<String<B1>,B2> -> BASIC_BOUNDED_STRING_DECLARE(elem_tn, char, B1)
                         BOUNDED_SEQUENCE_DECLARE(field_tn, elem_tn, B2)
    Seq<Sub>           -> SEQUENCE_ALIAS(field_tn, SubSequence)
    Seq<Sub,B>         -> BOUNDED_SEQUENCE_DECLARE(field_tn, sub_tn, B)
    """
    type_ = member.type
    field_tn = experimental_field_typename(message_typename, member.name)

    if isinstance(type_, BasicType):
        scalar_type = BASIC_IDL_TYPES_TO_EXPERIMENTAL_C_SCALAR[type_.typename]
        return "ROSIDL_RUNTIME_C__EXPERIMENTAL__SCALAR_ALIAS({}, {})".format(
            field_tn, scalar_type
        )

    if isinstance(type_, AbstractString):
        if type_.has_maximum_size():
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_STRING_ALIAS({}, rosidl_runtime_c__experimental__BoundedString, char)".format(
                field_tn
            )
        return "ROSIDL_RUNTIME_C__EXPERIMENTAL__STRING_ALIAS({}, rosidl_runtime_c__experimental__String, char)".format(
            field_tn
        )

    if isinstance(type_, AbstractWString):
        if type_.has_maximum_size():
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_STRING_ALIAS({}, rosidl_runtime_c__experimental__BoundedWString, char16_t)".format(
                field_tn
            )
        return "ROSIDL_RUNTIME_C__EXPERIMENTAL__STRING_ALIAS({}, rosidl_runtime_c__experimental__WString, char16_t)".format(
            field_tn
        )

    if isinstance(type_, NamespacedType):
        return None  # sub-message type used directly in the struct

    if isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            c_type = BASIC_IDL_TYPES_TO_EXPERIMENTAL_C[vt.typename]
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_DECLARE({}, {}, {}U)".format(
                field_tn, c_type, type_.size
            )
        # Complex element array: optional elem typedef, then appropriate ARRAY_DECLARE macro
        lines = []
        if isinstance(vt, AbstractString):
            if vt.has_maximum_size():
                # For bounded strings, use BOUNDED_ELEMENT_ARRAY_DECLARE
                elem_tn = "rosidl_runtime_c__experimental__BoundedString"
                macro_name = "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_ARRAY_DECLARE"
            else:
                elem_tn = "rosidl_runtime_c__experimental__String"
                macro_name = "ROSIDL_RUNTIME_C__EXPERIMENTAL__ARRAY_DECLARE"
        elif isinstance(vt, AbstractWString):
            if vt.has_maximum_size():
                # For bounded wstrings, use BOUNDED_ELEMENT_ARRAY_DECLARE
                elem_tn = "rosidl_runtime_c__experimental__BoundedWString"
                macro_name = "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_ARRAY_DECLARE"
            else:
                elem_tn = "rosidl_runtime_c__experimental__WString"
                macro_name = "ROSIDL_RUNTIME_C__EXPERIMENTAL__ARRAY_DECLARE"
        else:
            # NamespacedType
            elem_tn = idl_structure_type_to_experimental_c_typename(vt)
            macro_name = "ROSIDL_RUNTIME_C__EXPERIMENTAL__ARRAY_DECLARE"
        lines.append(
            "{}({}, {}, {}U)".format(
                macro_name, field_tn, elem_tn, type_.size
            )
        )
        return "\n".join(lines)

    if isinstance(type_, BoundedSequence):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            seq_type = BASIC_IDL_TYPES_TO_EXPERIMENTAL_C_SEQUENCE[vt.typename]
            # Use bounded sequence type alias
            bounded_seq_type = seq_type.replace("Sequence", "BoundedSequence")
            value_type = BASIC_IDL_TYPES_TO_EXPERIMENTAL_C[vt.typename]
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_BOUNDED_SEQUENCE_ALIAS({}, {}, {})".format(
                field_tn, bounded_seq_type, value_type
            )
        elem_c = experimental_element_c_type(message_typename, member.name, vt)
        # For bounded sequence of strings - need appropriate sequence declare
        if isinstance(vt, (AbstractString, AbstractWString)):
            if vt.has_maximum_size():
                # Bounded sequence of bounded strings - use bounded-element macro
                return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_BOUNDED_SEQUENCE_DECLARE({}, {})".format(
                    field_tn, elem_c
                )
            else:
                # Bounded sequence of unbounded strings - use regular bounded sequence macro
                return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DECLARE({}, {})".format(
                    field_tn, elem_c
                )
        else:
            # bounded sequence of sub-messages - alias to predefined BoundedSequence
            sub_tn = idl_structure_type_to_experimental_c_typename(vt)
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DECLARE({}, {})".format(
                field_tn, sub_tn
            )

    if isinstance(type_, UnboundedSequence):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            seq_type = BASIC_IDL_TYPES_TO_EXPERIMENTAL_C_SEQUENCE[vt.typename]
            value_type = BASIC_IDL_TYPES_TO_EXPERIMENTAL_C[vt.typename]
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_SEQUENCE_ALIAS({}, {}, {})".format(
                field_tn, seq_type, value_type
            )
        if isinstance(vt, AbstractString) and not vt.has_maximum_size():
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_ALIAS({}, rosidl_runtime_c__experimental__String__Sequence, rosidl_runtime_c__experimental__String)".format(
                field_tn
            )
        if isinstance(vt, AbstractWString) and not vt.has_maximum_size():
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_ALIAS({}, rosidl_runtime_c__experimental__WString__Sequence, rosidl_runtime_c__experimental__WString)".format(
                field_tn
            )
        if isinstance(vt, NamespacedType):
            # Use alias to the message type's predefined sequence
            sub_tn = idl_structure_type_to_experimental_c_typename(vt)
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__SEQUENCE_ALIAS({}, {}__Sequence, {})".format(
                field_tn, sub_tn, sub_tn
            )
        # Only bounded strings reach here: sequence<string<N>> or sequence<wstring<N>>
        elem_c = experimental_element_c_type(message_typename, member.name, vt)
        if isinstance(vt, AbstractString):
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_SEQUENCE_DECLARE({}, {})".format(
                field_tn, elem_c
            )
        else:  # AbstractWString
            return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_SEQUENCE_DECLARE({}, {})".format(
                field_tn, elem_c
            )

    assert False, "Unhandled member type: " + str(type_)


def experimental_field_define_macro(message_typename, member):
    """Return the single DEFINE macro call for this member's per-field type.

    Returns None for:
    - Alias macros (no implementation needed, just inline forwarding)
    - Static-inline declarations (PRIMITIVE_ARRAY_DECLARE, ARRAY_DECLARE)
    - Externally defined types (NamespacedType sub-messages, predefined types)

    Only returns DEFINE macros for types that need separate implementation:
    - Bounded strings (capacity checking logic)
    - Bounded sequences of primitives (capacity checking logic)
    - Bounded/unbounded sequences of bounded strings (element + sequence impl)
    - Bounded sequences of complex types (element management logic)
    """
    type_ = member.type
    field_tn = experimental_field_typename(message_typename, member.name)

    if isinstance(type_, BasicType):
        return None

    if isinstance(type_, AbstractString):
        return None  # All strings use ALIAS (bounded or unbounded)

    if isinstance(type_, AbstractWString):
        return None  # All wstrings use ALIAS (bounded or unbounded)

    if isinstance(type_, NamespacedType):
        return None  # sub-message functions defined in sub-message code

    if isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            return None  # PRIMITIVE_ARRAY_DECLARE emits static-inline definitions
        # Complex-element arrays use static-inline definitions from ARRAY_DECLARE, no DEFINE needed
        return None

    if isinstance(type_, BoundedSequence):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            return None  # Uses PRIMITIVE_BOUNDED_SEQUENCE_ALIAS
        # Complex element bounded sequences
        if isinstance(vt, (AbstractString, AbstractWString)):
            if vt.has_maximum_size():
                # Bounded sequence of bounded strings - need BOUNDED_ELEMENT macro
                elem_c = experimental_element_c_type(message_typename, member.name, vt)
                return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_BOUNDED_SEQUENCE_DEFINE({}, {})".format(
                    field_tn, elem_c
                )
            else:
                # Bounded sequence of unbounded strings - use regular BOUNDED_SEQUENCE macro
                elem_c = experimental_element_c_type(message_typename, member.name, vt)
                return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DEFINE({}, {})".format(
                    field_tn, elem_c
                )
        # Bounded sequence of sub-messages
        sub_tn = idl_structure_type_to_experimental_c_typename(vt)
        return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_SEQUENCE_DEFINE({}, {})".format(
            field_tn, sub_tn
        )

    if isinstance(type_, UnboundedSequence):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            return None  # Alias to predefined sequence, no define needed
        if isinstance(vt, AbstractString) and not vt.has_maximum_size():
            return None  # Alias to StringSequence, no define needed
        if isinstance(vt, AbstractWString) and not vt.has_maximum_size():
            return None  # Alias to WStringSequence, no define needed
        if isinstance(vt, NamespacedType):
            return None  # Alias to MessageTypeSequence, no define needed
        # Only bounded strings reach here: sequence<string<N>> or sequence<wstring<N>>
        # Need DEFINE for bounded-element sequence
        elem_c = experimental_element_c_type(message_typename, member.name, vt)
        return "ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_SEQUENCE_DEFINE({}, {})".format(
            field_tn, elem_c
        )

    assert False, "Unhandled member type: " + str(type_)


def experimental_field_in_struct(message_experimental_typename, member):
    """Return the declaration string for a field inside the experimental message struct body."""
    type_ = member.type
    if isinstance(type_, BasicType):
        scalar_typename = BASIC_IDL_TYPES_TO_EXPERIMENTAL_C_SCALAR[type_.typename]
        return "{} {}".format(scalar_typename, member.name)
    if isinstance(type_, NamespacedType):
        sub_typename = idl_structure_type_to_experimental_c_typename(type_)
        return "{} {}".format(sub_typename, member.name)
    if isinstance(type_, AbstractString) and not type_.has_maximum_size():
        return "rosidl_runtime_c__experimental__String {}".format(member.name)
    if isinstance(type_, AbstractWString) and not type_.has_maximum_size():
        return "rosidl_runtime_c__experimental__WString {}".format(member.name)
    if isinstance(type_, UnboundedSequence) and isinstance(type_.value_type, BasicType):
        seq_typename = BASIC_IDL_TYPES_TO_EXPERIMENTAL_C_SEQUENCE[
            type_.value_type.typename
        ]
        return "{} {}".format(seq_typename, member.name)
    field_typename = experimental_field_typename(
        message_experimental_typename, member.name
    )
    return "{} {}".format(field_typename, member.name)


def experimental_field_needs_allocator(member_type):
    """Return True if this member type's __init requires a rcutils_allocator_t * argument."""
    if isinstance(member_type, BasicType):
        return False  # Scalar: inline local storage, no allocator
    if isinstance(member_type, Array):
        return not isinstance(member_type.value_type, BasicType)
    return True  # strings, sequences, sub-messages all need allocators


def experimental_constraint_field(msg_tn, member):
    """Return (constraint_type_name, field_name) for the Constraints struct, or None.

    Only members with at least one variable dimension contribute a field:
      - Unbounded string/wstring                  → StringConstraint
      - NamespacedType (sub-message)              → {sub_tn}__Constraints
      - Array of unbounded string/wstring         → StringConstraint (shared cap)
      - Array of NamespacedType                   → {sub_tn}__Constraints
      - UnboundedSequence<basic/bounded>          → SequenceConstraint
      - UnboundedSequence<string>                 → StringSequenceConstraint
      - UnboundedSequence<wstring>                → WStringSequenceConstraint
      - UnboundedSequence<SubMessage>             → {sub_tn}__SequenceConstraint
      - BasicType, bounded string, BoundedSequence → None (fully fixed)
    """
    type_ = member.type
    name = member.name

    if isinstance(type_, BasicType):
        return None

    if isinstance(type_, (AbstractString, AbstractWString)):
        if type_.has_maximum_size():
            return None
        return ("rosidl_runtime_c__experimental__StringConstraint", name)

    if isinstance(type_, NamespacedType):
        sub_tn = idl_structure_type_to_experimental_c_typename(type_)
        return ("{}__Constraints".format(sub_tn), name)

    if isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            return None
        if isinstance(vt, (AbstractString, AbstractWString)):
            if vt.has_maximum_size():
                return None
            return ("rosidl_runtime_c__experimental__StringConstraint", name)
        if isinstance(vt, NamespacedType):
            sub_tn = idl_structure_type_to_experimental_c_typename(vt)
            return ("{}__Constraints".format(sub_tn), name)
        return None

    if isinstance(type_, BoundedSequence):
        return None  # fully bounded — no runtime constraint

    if isinstance(type_, UnboundedSequence):
        vt = type_.value_type

        # Basic types or bounded strings -> use generic SequenceConstraint
        if isinstance(vt, BasicType):
            return ("rosidl_runtime_c__experimental__SequenceConstraint", name)

        # Bounded string/wstring -> use generic SequenceConstraint
        if isinstance(vt, (AbstractString, AbstractWString)) and vt.has_maximum_size():
            return ("rosidl_runtime_c__experimental__SequenceConstraint", name)

        # Unbounded string -> use StringSequenceConstraint
        if isinstance(vt, AbstractString) and not vt.has_maximum_size():
            return ("rosidl_runtime_c__experimental__StringSequenceConstraint", name)

        # Unbounded wstring -> use WStringSequenceConstraint
        if isinstance(vt, AbstractWString) and not vt.has_maximum_size():
            return ("rosidl_runtime_c__experimental__WStringSequenceConstraint", name)

        # Sub-message -> use SubMessage__SequenceConstraint
        if isinstance(vt, NamespacedType):
            sub_tn = idl_structure_type_to_experimental_c_typename(vt)
            return ("{}__SequenceConstraint".format(sub_tn), name)

        return None

    return None


def experimental_constraints_are_equal_body(constraint_fields):
    """Build the body of the __Constraints__are_equal function.

    Args:
        constraint_fields: List of (constraint_type, field_name) tuples

    Returns:
        String containing the function body
    """
    if not constraint_fields:
        return "(void)lhs;\n  (void)rhs;\n  return true;"

    # Build list of comparison expressions
    comparisons = [
        "{}__are_equal(&lhs->{}, &rhs->{})".format(ctype, cname, cname)
        for ctype, cname in constraint_fields
    ]

    # Join with && and proper line breaks
    if len(comparisons) == 1:
        return "return {};".format(comparisons[0])

    # Multi-line with proper formatting
    lines = ["return"]
    for i, comp in enumerate(comparisons):
        if i < len(comparisons) - 1:
            lines.append("    {} &&".format(comp))
        else:
            lines.append("    {};".format(comp))

    return "\n".join(lines)


def experimental_storage_field_declaration(member_name, member_type):
    """Return the C declaration for a member's field inside ExternalStorage.

    Uses type-erased rosidl_memory_t / rosidl_memory_region_t structs throughout;
    no generated-type names appear here.
    For fixed-size arrays of non-primitive types a C-style array suffix is used.
    For sequences of non-primitive types, a struct with both the sequence region
    and element storage pool array is used, matching the C++ implementation.
    """
    type_ = member_type
    if isinstance(type_, BasicType):
        return "rosidl_memory_t {}".format(member_name)
    if isinstance(type_, (AbstractString, AbstractWString)):
        return "rosidl_memory_region_t {}".format(member_name)
    if isinstance(type_, NamespacedType):
        sub_tn = idl_structure_type_to_experimental_c_typename(type_)
        return "{}__ExternalStorage {}".format(sub_tn, member_name)
    if isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            return "rosidl_memory_region_t {}".format(member_name)
        if isinstance(vt, (AbstractString, AbstractWString)):
            return "rosidl_memory_region_t {}[{}U]".format(member_name, type_.size)
        if isinstance(vt, NamespacedType):
            sub_tn = idl_structure_type_to_experimental_c_typename(vt)
            return "{}__ExternalStorage {}[{}U]".format(sub_tn, member_name, type_.size)
    if isinstance(type_, AbstractSequence):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            # Primitive sequence: just the backing buffer region
            return "rosidl_memory_region_t {}".format(member_name)
        # Complex element sequence: need both sequence region and element storage pool
        lines = []
        lines.append("struct")
        lines.append("{")
        lines.append("  rosidl_memory_region_t region;")
        if isinstance(vt, (AbstractString, AbstractWString)):
            lines.append("  rosidl_memory_region_t * element_storage_pool;")
        elif isinstance(vt, NamespacedType):
            sub_tn = idl_structure_type_to_experimental_c_typename(vt)
            lines.append("  {}__ExternalStorage * element_storage_pool;".format(sub_tn))
        lines.append("  size_t element_storage_pool_size;")
        lines.append("}} {}".format(member_name))
        return "\n    ".join(lines)
    assert False, "Unhandled type: " + str(type_)
