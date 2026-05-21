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

"""Experimental code generation helpers for rosidl_generator_py.

These helpers map rosidl type definitions to the Python experimental
types from ``rosidl_runtime_py.experimental``.
"""

from ast import literal_eval

from rosidl_parser.definition import AbstractGenericString
from rosidl_parser.definition import AbstractNestedType
from rosidl_parser.definition import AbstractSequence
from rosidl_parser.definition import AbstractString
from rosidl_parser.definition import AbstractWString
from rosidl_parser.definition import Array
from rosidl_parser.definition import BasicType
from rosidl_parser.definition import BoundedSequence
from rosidl_parser.definition import NamespacedType
from rosidl_parser.definition import UnboundedSequence


# ---------------------------------------------------------------------------
# BasicType → Dtype member name
# ---------------------------------------------------------------------------
BASIC_TYPE_TO_DTYPE = {
    'boolean': 'Dtype.BOOL',
    'octet': 'Dtype.BYTE',
    'char': 'Dtype.CHAR',
    'wchar': 'Dtype.WCHAR',
    'float': 'Dtype.FLOAT32',
    'double': 'Dtype.FLOAT64',
    'long double': 'Dtype.LONG_DOUBLE',
    'uint8': 'Dtype.UINT8',
    'int8': 'Dtype.INT8',
    'uint16': 'Dtype.UINT16',
    'int16': 'Dtype.INT16',
    'uint32': 'Dtype.UINT32',
    'int32': 'Dtype.INT32',
    'uint64': 'Dtype.UINT64',
    'int64': 'Dtype.INT64',
}


def experimental_msg_type(type_):
    """Return the Python expression for the experimental type of a message member.

    Top-level scalar BasicTypes map to ``Scalar(Dtype.X)`` constructor calls.
    Strings, arrays, and sequences map to the corresponding experimental
    container type.
    """
    if isinstance(type_, Array):
        return _array_type(type_)
    if isinstance(type_, BoundedSequence):
        return _bounded_sequence_type(type_)
    if isinstance(type_, UnboundedSequence):
        return _unbounded_sequence_type(type_)
    # Top-level (non-nested) member types
    if isinstance(type_, BasicType):
        return 'Scalar({})'.format(BASIC_TYPE_TO_DTYPE[type_.typename])
    if isinstance(type_, AbstractString):
        if type_.has_maximum_size():
            return 'BoundedString({})'.format(type_.maximum_size)
        return 'String()'
    if isinstance(type_, AbstractWString):
        if type_.has_maximum_size():
            return 'BoundedWString({})'.format(type_.maximum_size)
        return 'WString()'
    if isinstance(type_, NamespacedType):
        return '{}(_init=MessageInitialization.SKIP)'.format(type_.name)
    assert False, 'unknown type: {}'.format(type_)


def _array_type(type_):
    """Array<T, N> → Array(dtype, N) or Array(SubMsg, N)."""
    vt = type_.value_type
    if isinstance(vt, BasicType):
        return 'Array({}, {})'.format(BASIC_TYPE_TO_DTYPE[vt.typename], type_.size)
    if isinstance(vt, NamespacedType):
        return 'Array({}, {}, data=[{}(_init=MessageInitialization.SKIP) for _ in range({})])'.format(
            vt.name, type_.size, vt.name, type_.size)
    # String/WString arrays use object-typed Array
    if isinstance(vt, AbstractString):
        if vt.has_maximum_size():
            return 'Array(BoundedString, {}, data=[BoundedString({}) for _ in range({})])'.format(
                type_.size, vt.maximum_size, type_.size)
        return 'Array(String, {}, data=[String() for _ in range({})])'.format(
            type_.size, type_.size)
    if isinstance(vt, AbstractWString):
        if vt.has_maximum_size():
            return (
                'Array(BoundedWString, {}, data=[BoundedWString({}) for _ in range({})])'
                .format(type_.size, vt.maximum_size, type_.size))
        return 'Array(WString, {}, data=[WString() for _ in range({})])'.format(
            type_.size, type_.size)
    assert False, 'unknown array element type: {}'.format(vt)


def _bounded_sequence_type(type_):
    """BoundedSequence<T, N>."""
    vt = type_.value_type
    if isinstance(vt, BasicType):
        return 'BoundedSequence({}, {})'.format(
            BASIC_TYPE_TO_DTYPE[vt.typename], type_.maximum_size)
    if isinstance(vt, NamespacedType):
        return 'BoundedSequence({}, {})'.format(vt.name, type_.maximum_size)
    if isinstance(vt, AbstractString):
        if vt.has_maximum_size():
            return 'BoundedSequence(BoundedString, {})'.format(type_.maximum_size)
        return 'BoundedSequence(String, {})'.format(type_.maximum_size)
    if isinstance(vt, AbstractWString):
        if vt.has_maximum_size():
            return 'BoundedSequence(BoundedWString, {})'.format(type_.maximum_size)
        return 'BoundedSequence(WString, {})'.format(type_.maximum_size)
    assert False, 'unknown bounded sequence element type: {}'.format(vt)


def _unbounded_sequence_type(type_):
    """Sequence<T>."""
    vt = type_.value_type
    if isinstance(vt, BasicType):
        return 'Sequence({})'.format(BASIC_TYPE_TO_DTYPE[vt.typename])
    if isinstance(vt, NamespacedType):
        return 'Sequence({})'.format(vt.name)
    if isinstance(vt, AbstractString):
        if vt.has_maximum_size():
            return 'Sequence(BoundedString)'
        return 'Sequence(String)'
    if isinstance(vt, AbstractWString):
        if vt.has_maximum_size():
            return 'Sequence(BoundedWString)'
        return 'Sequence(WString)'
    assert False, 'unknown sequence element type: {}'.format(vt)


# ---------------------------------------------------------------------------
# Constraint type for a member
# ---------------------------------------------------------------------------

def experimental_constraint_type(type_):
    """Return the Python constraint type string for a member, or None if none is needed.

    - BasicType (Scalar): no constraint
    - Bounded string/wstring: no constraint (bound is part of type)
    - Unbounded string/wstring: StringConstraint
    - NamespacedType: SubMsg.Constraints
    - Array<BasicType, N>: no constraint
    - Array<BoundedString, N>: no constraint
    - Array<UnboundedString, N>: StringConstraint
    - Array<NamespacedType, N>: SubMsg.Constraints
    - BoundedSequence: no constraint (bound is part of type)
    - UnboundedSequence: SequenceConstraint
    """
    if isinstance(type_, BasicType):
        return None
    if isinstance(type_, (AbstractString, AbstractWString)):
        return None if type_.has_maximum_size() else 'StringConstraint'
    if isinstance(type_, NamespacedType):
        return '{}.Constraints'.format(type_.name)
    if isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            return None
        if isinstance(vt, (AbstractString, AbstractWString)):
            return None if vt.has_maximum_size() else 'StringConstraint'
        if isinstance(vt, NamespacedType):
            return '{}.Constraints'.format(vt.name)
        return None
    if isinstance(type_, AbstractSequence):
        if isinstance(type_, BoundedSequence):
            return None
        return 'SequenceConstraint'
    return None


# ---------------------------------------------------------------------------
# Default / zero value generation
# ---------------------------------------------------------------------------

def _experimental_primitive_value_to_py(type_, value):
    """Convert a rosidl value to a Python literal suitable for experimental types.

    For experimental Scalars, bytes/octet and char types are stored as
    integers (via numpy), not as Python bytes/str objects.  This function
    returns integer literals for those types.
    """
    if isinstance(type_, BasicType):
        if type_.typename == 'boolean':
            return 'True' if value else 'False'
        if type_.typename in ('octet', 'char'):
            # Experimental Scalar stores these as integer values
            return str(int(value))
        if type_.typename == 'wchar':
            return str(int(value))
    # Fall through to the standard converter for everything else
    from rosidl_generator_py.generate_py_impl import primitive_value_to_py
    return primitive_value_to_py(type_, value)


def experimental_default_value_expr(member):
    """Return Python expression to set the default value for *member*, or None."""
    if not member.has_annotation('default'):
        return None
    default = member.get_annotation_value('default')['value']
    type_ = member.type

    if isinstance(type_, AbstractNestedType):
        values = literal_eval(default)
        vt = type_.value_type
        items = [_experimental_primitive_value_to_py(vt, v) for v in values]
        if isinstance(type_, Array):
            # For arrays, set element by element
            lines = []
            for i, v in enumerate(items):
                lines.append('self.{name}[{i}] = {v}'.format(name=member.name, i=i, v=v))
            return lines
        else:
            # For sequences, extend with values
            return ['self.{name}.extend([{vals}])'.format(
                name=member.name, vals=', '.join(items))]
    elif isinstance(type_, BasicType):
        py_val = _experimental_primitive_value_to_py(type_, default)
        return ['self.{name}.value = {val}'.format(name=member.name, val=py_val)]
    elif isinstance(type_, AbstractGenericString):
        py_val = _experimental_primitive_value_to_py(type_, default)
        return ['self.{name}.assign({val})'.format(name=member.name, val=py_val)]
    else:
        return None


def experimental_zero_value_expr(member):
    """Return Python expression to zero-init *member*, or None."""
    type_ = member.type

    if isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            return None  # Arrays of primitives are zero-init by default
        if isinstance(vt, AbstractGenericString):
            return None  # String arrays already have empty strings
        if isinstance(vt, NamespacedType):
            return None  # Sub-message arrays are initialized recursively
        return None
    if isinstance(type_, AbstractSequence):
        return None  # Sequences start empty
    if isinstance(type_, BasicType):
        return None  # Scalar default is 0
    if isinstance(type_, AbstractGenericString):
        return None  # Strings default to empty
    if isinstance(type_, NamespacedType):
        return None  # Sub-messages are initialized recursively
    return None


# ---------------------------------------------------------------------------
# Sub-message member detection
# ---------------------------------------------------------------------------

def experimental_has_submsg_members(message):
    """Return True if the message has sub-message members that need _reset propagation."""
    for m in message.structure.members:
        if isinstance(m.type, NamespacedType):
            return True
        if isinstance(m.type, (Array, AbstractSequence)):
            if isinstance(m.type.value_type, NamespacedType):
                return True
    return False


def experimental_submsg_members(message):
    """Yield (member, is_nested) for sub-message members that need _reset propagation."""
    for m in message.structure.members:
        if isinstance(m.type, NamespacedType):
            yield m, False
        elif isinstance(m.type, (Array, AbstractSequence)):
            if isinstance(m.type.value_type, NamespacedType):
                yield m, True


# ---------------------------------------------------------------------------
# External storage support
# ---------------------------------------------------------------------------

def experimental_storage_type(type_):
    """Return the Python type annotation for an ExternalStorage member field.

    All fields use RawBuffer as the base descriptor type, with lists for
    arrays/sequences of complex element types:

    - BasicType (Scalar) → RawBuffer
    - String/WString → RawBuffer
    - NamespacedType → SubMsg.ExternalStorage (recursive)
    - Array<BasicType, N> → RawBuffer (contiguous)
    - Array<String, N> → list[RawBuffer] (one per element)
    - Array<NamespacedType, N> → list[SubMsg.ExternalStorage]
    - Sequence<BasicType> → RawBuffer (contiguous)
    - Sequence<String> → list[RawBuffer] (variable-length)
    - Sequence<NamespacedType> → list[SubMsg.ExternalStorage]
    """
    if isinstance(type_, BasicType):
        return 'RawBuffer'
    if isinstance(type_, (AbstractString, AbstractWString)):
        return 'RawBuffer'
    if isinstance(type_, NamespacedType):
        return '{}.ExternalStorage'.format(type_.name)
    if isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            return 'RawBuffer'  # Contiguous primitive array
        if isinstance(vt, (AbstractString, AbstractWString)):
            return 'list[RawBuffer]'  # One buffer per string element
        if isinstance(vt, NamespacedType):
            return 'list[{}.ExternalStorage]'.format(vt.name)
    if isinstance(type_, AbstractSequence):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            return 'RawBuffer'  # Contiguous primitive sequence
        if isinstance(vt, (AbstractString, AbstractWString)):
            return 'list[RawBuffer]'  # One buffer per string element
        if isinstance(vt, NamespacedType):
            return 'list[{}.ExternalStorage]'.format(vt.name)
    raise ValueError('Unknown type: {}'.format(type_))


def experimental_storage_init_expr(member_name, type_):
    """Return a Python expression to initialize a field from external storage.

    For simple cases (scalars, strings, primitive arrays/sequences), returns
    a single-line constructor expression with buffer= parameter.

    For complex cases (arrays/sequences of strings/messages), returns None
    to signal that the template should generate custom initialization code.

    Args:
        member_name: The field name (e.g., 'my_int')
        type_: The rosidl type definition

    Returns:
        str or None: Python constructor expression, or None for complex cases
    """
    storage_path = '_storage.members.{}'.format(member_name)

    if isinstance(type_, BasicType):
        dtype = BASIC_TYPE_TO_DTYPE[type_.typename]
        return 'Scalar({dtype}, buffer={storage})'.format(
            dtype=dtype, storage=storage_path)

    if isinstance(type_, AbstractString):
        if type_.has_maximum_size():
            return 'BoundedString({bound}, buffer={storage})'.format(
                bound=type_.maximum_size, storage=storage_path)
        return 'String(buffer={storage})'.format(storage=storage_path)

    if isinstance(type_, AbstractWString):
        if type_.has_maximum_size():
            return 'BoundedWString({bound}, buffer={storage})'.format(
                bound=type_.maximum_size, storage=storage_path)
        return 'WString(buffer={storage})'.format(storage=storage_path)

    if isinstance(type_, NamespacedType):
        return '{type_name}(_storage={storage})'.format(
            type_name=type_.name, storage=storage_path)

    if isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            # Primitive array: single contiguous buffer
            dtype = BASIC_TYPE_TO_DTYPE[vt.typename]
            return 'Array({dtype}, {size}, buffer={storage})'.format(
                dtype=dtype, size=type_.size, storage=storage_path)
        # Complex arrays need per-element initialization - template handles it
        return None

    if isinstance(type_, AbstractSequence):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            # Primitive sequence: single contiguous buffer
            dtype = BASIC_TYPE_TO_DTYPE[vt.typename]
            if isinstance(type_, BoundedSequence):
                return 'BoundedSequence({dtype}, {bound}, buffer={storage})'.format(
                    dtype=dtype, bound=type_.maximum_size, storage=storage_path)
            return 'Sequence({dtype}, buffer={storage})'.format(
                dtype=dtype, storage=storage_path)
        # Complex sequences need per-element initialization - template handles it
        return None

    return None
