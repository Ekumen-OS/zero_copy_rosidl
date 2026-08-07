# Copyright 2026 Ekumen Inc.
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

"""Helper functions for rosidl_typesupport_xcdr_cpython templates."""

from rosidl_parser.definition import (
    AbstractSequence,
    AbstractString,
    AbstractWString,
    Array,
    BoundedSequence,
    BoundedString,
    BoundedWString,
    NamespacedType,
)


def get_xcdr_primitive_kind(basic_type):
    """Map BasicType to XCdr primitive kind."""
    type_map = {
        'boolean': 'kBool',
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
        'char': 'kChar',
        'wchar': 'kWchar',
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
        'char': 'unsigned char',
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


def get_message_type_name(namespaced_type, experimental_context=False):
    """
    Get fully qualified C++ type name for a message.

    If experimental_context is True, append 'experimental' to the namespace
    so that the type resolves to the experimental variant.
    """
    ns = list(namespaced_type.namespaced_name())
    if experimental_context and 'experimental' not in ns:
        # Insert 'experimental' before the type name (last element)
        ns.insert(-1, 'experimental')
    return '::'.join(ns)


def get_python_module_path(namespaced_type, experimental_context=False):
    """
    Get the Python module path for a message class, e.g. 'std_msgs.msg._string'.

    With experimental_context=True the path targets the experimental variant,
    e.g. 'std_msgs.msg.experimental._string'.
    """
    from rosidl_pycommon import convert_camel_case_to_lower_case_underscore
    parts = list(namespaced_type.namespaces)
    if experimental_context and 'experimental' not in parts:
        parts.append('experimental')
    filename = convert_camel_case_to_lower_case_underscore(namespaced_type.name)
    parts.append('_' + filename)
    return '.'.join(parts)


def get_python_class_name(namespaced_type):
    """Get the Python class name of a message, e.g. 'String'."""
    return namespaced_type.name


def get_python_string_class(type_):
    """
    Map an IDL string type to the rosidl_runtime_cpython container class name.

    - BoundedString → 'BoundedString'
    - BoundedWString → 'BoundedWString'
    - AbstractWString (unbounded) → 'WString'
    - AbstractString (unbounded) → 'String'
    """
    if isinstance(type_, BoundedWString):
        return 'BoundedWString'
    if isinstance(type_, BoundedString):
        return 'BoundedString'
    if isinstance(type_, AbstractWString):
        return 'WString'
    if isinstance(type_, AbstractString):
        return 'String'
    assert False, 'not a string type: {}'.format(type_)


def get_nested_typesupport_include(namespaced_type, experimental_context=False):
    """
    Get include path for nested message's XCDR CPython typesupport.

    If experimental_context is True, the include path targets the experimental
    variant of the nested type's typesupport header.
    """
    from rosidl_pycommon import convert_camel_case_to_lower_case_underscore
    parts = list(namespaced_type.namespaces)
    if experimental_context and 'experimental' not in parts:
        parts.append('experimental')
    filename = convert_camel_case_to_lower_case_underscore(namespaced_type.name)
    parts.append('detail')
    parts.append(f'{filename}__rosidl_typesupport_xcdr_cpython')
    return '/'.join(parts) + '.hpp'
