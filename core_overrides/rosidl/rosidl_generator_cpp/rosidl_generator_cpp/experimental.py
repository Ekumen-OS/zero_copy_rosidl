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

"""Experimental code generation helpers for rosidl_generator_cpp."""
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


# Mapping from rosidl BasicType typename to the raw C++ scalar type used in Scalar<T> and
# as sequence/array element types in experimental message structs.
BASIC_TYPE_TO_EXPERIMENTAL_CPP = {
    'boolean': 'bool',
    'octet': 'unsigned char',
    'char': 'unsigned char',
    'wchar': 'char16_t',
    'float': 'float',
    'double': 'double',
    'long double': 'long double',
    'uint8': 'uint8_t',
    'int8': 'int8_t',
    'uint16': 'uint16_t',
    'int16': 'int16_t',
    'uint32': 'uint32_t',
    'int32': 'int32_t',
    'uint64': 'uint64_t',
    'int64': 'int64_t',
}

# BasicType typenames that require character_value_to_yaml in YAML traits.
EXPERIMENTAL_CHARACTER_TYPES = ('octet', 'char', 'wchar')


def experimental_namespaced_type_name(type_):
    """Return the C++ experimental qualified name for a NamespacedType."""
    return '::'.join(list(type_.namespaces) + ['experimental', type_.name])


def msg_element_type_to_experimental_cpp(type_):
    """Return the experimental C++ element type for sequence and array members.

    BasicType members use raw scalars (not Scalar<T>) as sequence/array elements,
    preserving the contiguous-storage fast paths in BasicSequence.
    """
    if isinstance(type_, BasicType):
        return BASIC_TYPE_TO_EXPERIMENTAL_CPP[type_.typename]
    elif isinstance(type_, AbstractString):
        if type_.has_maximum_size():
            return 'rosidl_runtime_cpp::BoundedString<{}>'.format(type_.maximum_size)
        return 'rosidl_runtime_cpp::String'
    elif isinstance(type_, AbstractWString):
        if type_.has_maximum_size():
            return 'rosidl_runtime_cpp::BoundedWString<{}>'.format(type_.maximum_size)
        return 'rosidl_runtime_cpp::WString'
    elif isinstance(type_, NamespacedType):
        return experimental_namespaced_type_name(type_)
    else:
        assert False, type_


def msg_type_only_to_experimental_cpp(type_):
    """Convert a top-level message member type to its experimental C++ type.

    BasicType top-level members are wrapped in Scalar<T>.
    """
    if isinstance(type_, AbstractNestedType):
        type_ = type_.value_type
    if isinstance(type_, BasicType):
        return 'rosidl_runtime_cpp::Scalar<{}>'.format(
            BASIC_TYPE_TO_EXPERIMENTAL_CPP[type_.typename])
    elif isinstance(type_, AbstractString):
        if type_.has_maximum_size():
            return 'rosidl_runtime_cpp::BoundedString<{}>'.format(type_.maximum_size)
        return 'rosidl_runtime_cpp::String'
    elif isinstance(type_, AbstractWString):
        if type_.has_maximum_size():
            return 'rosidl_runtime_cpp::BoundedWString<{}>'.format(type_.maximum_size)
        return 'rosidl_runtime_cpp::WString'
    elif isinstance(type_, NamespacedType):
        return experimental_namespaced_type_name(type_)
    else:
        assert False, type_


def msg_type_to_experimental_cpp(type_):
    """Convert a message type to its experimental C++ type, including array/sequence wrapping."""
    if isinstance(type_, Array):
        # rosidl_runtime_cpp::Array<T, N> is used for all element types.
        # For scalars T is the raw primitive; for strings/messages T is the experimental type.
        # This allows the piecewise constructor to be used uniformly for both PMR and
        # storage-based initialization.
        elem = msg_element_type_to_experimental_cpp(type_.value_type)
        return 'rosidl_runtime_cpp::Array<{}, {}>'.format(elem, type_.size)
    elif isinstance(type_, BoundedSequence):
        elem = msg_element_type_to_experimental_cpp(type_.value_type)
        return 'rosidl_runtime_cpp::BoundedSequence<{}, {}>'.format(
            elem, type_.maximum_size)
    elif isinstance(type_, UnboundedSequence):
        elem = msg_element_type_to_experimental_cpp(type_.value_type)
        return 'rosidl_runtime_cpp::Sequence<{}>'.format(elem)
    else:
        return msg_type_only_to_experimental_cpp(type_)


def create_experimental_member_list(message):
    """Return (init_list, member_list) for experimental message constructors.

    Like create_init_alloc_and_member_lists but for experimental types:
    - No allocator constructor; alloc_list is not produced.
    - NamespacedType (sub-message) members appear in init_list with SKIP so
      that reset() can propagate the actual MessageInitialization afterwards
      without double-initializing.
    - member_list carries the same zero/default annotation tracking as the
      standard version, used to generate the body of reset().
    """
    from rosidl_generator_cpp import default_value_from_type
    from rosidl_generator_cpp import primitive_value_to_cpp

    class Member:
        def __init__(self, name):
            self.name = name
            self.default_value = None
            self.zero_value = None
            self.type = None

        def same_default_and_zero_value(self, other):
            return (self.default_value == other.default_value and
                    self.zero_value == other.zero_value)

    class CommonMemberSet:
        def __init__(self):
            self.members = []

        def add_member(self, member):
            if not self.members or self.members[-1].same_default_and_zero_value(member):
                self.members.append(member)
                return True
            return False

    init_list = []
    member_list = []
    for field in message.structure.members:
        member = Member(field.name)
        member.type = field.type
        if isinstance(field.type, Array):
            if isinstance(field.type.value_type, (BasicType, AbstractGenericString)):
                default = default_value_from_type(field.type.value_type)
                single = primitive_value_to_cpp(field.type.value_type, default)
                member.zero_value = [single]
                if field.has_annotation('default'):
                    default_value = literal_eval(
                        field.get_annotation_value('default')['value'])
                    member.default_value = [
                        primitive_value_to_cpp(field.type.value_type, v)
                        for v in default_value]
            else:
                assert isinstance(field.type.value_type, NamespacedType)
                # Piecewise-construct each element with SKIP to avoid double init.
                tuples = ', '.join(
                    ['std::make_tuple(rosidl_runtime_cpp::MessageInitialization::SKIP)']
                    * field.type.size)
                init_list.append(
                    '{}(std::piecewise_construct, {})'.format(field.name, tuples))
        elif isinstance(field.type, AbstractSequence):
            if field.has_annotation('default'):
                default_value = literal_eval(
                    field.get_annotation_value('default')['value'])
                member.default_value = [
                    primitive_value_to_cpp(field.type.value_type, v)
                    for v in default_value]
            else:
                member.zero_value = '{}'  # clear when no default
        elif isinstance(field.type, (BasicType, AbstractGenericString)):
            default = default_value_from_type(field.type)
            member.zero_value = primitive_value_to_cpp(field.type, default)
            if field.has_annotation('default'):
                member.default_value = primitive_value_to_cpp(
                    field.type,
                    field.get_annotation_value('default')['value'])
        else:
            # NamespacedType: initialize with SKIP; reset() propagates _init.
            init_list.append(
                '{}(rosidl_runtime_cpp::MessageInitialization::SKIP)'.format(field.name))

        if field.has_annotation('default') or member.zero_value is not None:
            if not member_list or not member_list[-1].add_member(member):
                commonset = CommonMemberSet()
                commonset.add_member(member)
                member_list.append(commonset)

    return init_list, member_list


def experimental_member_needs_pmr(type_):
    """Return True if this member type needs PMR resource propagation in the PMR constructor."""
    if isinstance(type_, BasicType):
        return False  # Scalar<T> uses inline storage
    if isinstance(type_, Array):
        return not isinstance(type_.value_type, BasicType)  # Array<scalar,N> is inline
    return True  # strings, sequences, and sub-messages all need PMR propagation


def experimental_pmr_init_expr(member_name, type_):
    """Return the member-initializer list expression for the PMR constructor.

    Sub-messages (NamespacedType) are constructed with SKIP so that _initialize()
    can propagate the actual MessageInitialization value afterwards.
    """
    if isinstance(type_, Array) and not isinstance(type_.value_type, BasicType):
        if isinstance(type_.value_type, NamespacedType):
            tuples = ', '.join(
                ['std::make_tuple(mem_res, rosidl_runtime_cpp::MessageInitialization::SKIP)'
                 ] * type_.size)
        else:
            tuples = ', '.join(['std::make_tuple(mem_res)'] * type_.size)
        return '{}(std::piecewise_construct, {})'.format(member_name, tuples)
    if isinstance(type_, NamespacedType):
        return '{}(mem_res, rosidl_runtime_cpp::MessageInitialization::SKIP)'.format(member_name)
    return '{}(mem_res)'.format(member_name)


def generate_experimental_default_string(membset: list) -> list[str]:
    strlist: list[str] = []
    for member in membset.members:
        if member.default_value is not None:
            if isinstance(member.default_value, list):
                if len(member.default_value) > 1 and \
                        all(v == member.default_value[0] for v in member.default_value):
                    if isinstance(member.type, AbstractSequence):
                        strlist.append('this->%s.resize(%d);' % (
                            member.name, len(member.default_value)))
                    strlist.append(
                        'std::fill(this->%s.begin(), this->%s.end(), %s);' % (
                            member.name, member.name, member.default_value[0]))
                else:
                    strlist.append('this->%s = {%s};' % (
                        member.name, ', '.join(member.default_value)))
            else:
                strlist.append('this->%s = %s;' % (member.name, member.default_value))
    return strlist


def generate_experimental_zero_string(membset):
    """Like generate_zero_string but uses experimental C++ type names for array fill.

    For Array<NamespacedType, N> members the fill element type is the experimental
    message name (e.g. pkg::ns::experimental::Sub) rather than the standard
    allocator-based type (e.g. pkg::ns::Sub_<std::allocator<void>>).
    """
    strlist = []
    for member in membset.members:
        if isinstance(member.zero_value, list):
            if isinstance(member.type, AbstractSequence):
                strlist.append('this->%s.resize(%d);' % (
                    member.name, len(member.zero_value)))
            strlist.append('std::fill(this->%s.begin(), this->%s.end(), %s);' % (
                member.name, member.name, member.zero_value[0]))
        else:
            strlist.append('this->%s = %s;' % (member.name, member.zero_value))
    return strlist


def experimental_storage_type(type_):
    """Return the C++ type for a member's field in the ExternalStorage nested struct.

    Each field holds the memory descriptor(s) needed to initialise the
    corresponding message member to point to externally managed memory.

    Rules:
    - BasicType → rosidl_runtime_cpp::Memory<T>     (for Scalar<T>)
    - {Bounded}String
                → rosidl_runtime_cpp::MemoryRegion<char>  (for {Bounded}String)
    - {Bounded}WString
                → rosidl_runtime_cpp::MemoryRegion<char16_t>  (for {Bounded}WString)
    - NamespacedType → SubMsg::ExternalStorage      (recursive)
    - Array<BasicType, N>
                → rosidl_runtime_cpp::MemoryRegion<T>  (contiguous scalar block)
    - Array<{Bounded}String, N>
                → std::array<rosidl_runtime_cpp::MemoryRegion<char>, N>
    - Array<{Bounded}WString, N>
                → std::array<rosidl_runtime_cpp::MemoryRegion<char16_t>, N>
    - Array<NamespacedType, N>
                → std::array<SubMsg::ExternalStorage, N>
      (Storage arrays are always std::array — no external memory management needed
       for descriptor fields; the piecewise constructor on Array<T,N> is used to
       initialise the message member from this descriptor array.)
    - {Bounded}Sequence<BasicType>
                → rosidl_runtime_cpp::MemoryRegion<T>  (contiguous scalar buffer)
    - {Bounded}Sequence<{Bounded}String>
                → std::vector<rosidl_runtime_cpp::MemoryRegion<char>>
    - {Bounded}Sequence<{Bounded}WString>
                → std::vector<rosidl_runtime_cpp::MemoryRegion<char16_t>>
    - {Bounded}Sequence<NamespacedType>
                → std::vector<SubMsg::ExternalStorage>
    """
    if isinstance(type_, BasicType):
        cpp_type = BASIC_TYPE_TO_EXPERIMENTAL_CPP[type_.typename]
        return 'rosidl_runtime_cpp::Memory<{}>'.format(cpp_type)
    if isinstance(type_, AbstractString):
        return 'rosidl_runtime_cpp::MemoryRegion<char>'
    if isinstance(type_, AbstractWString):
        return 'rosidl_runtime_cpp::MemoryRegion<char16_t>'
    if isinstance(type_, NamespacedType):
        return '{}::ExternalStorage'.format(experimental_namespaced_type_name(type_))
    if isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            cpp_type = BASIC_TYPE_TO_EXPERIMENTAL_CPP[vt.typename]
            return 'rosidl_runtime_cpp::MemoryRegion<{}>'.format(cpp_type)
        if isinstance(vt, AbstractString):
            return 'std::array<rosidl_runtime_cpp::MemoryRegion<char>, {}>'.format(type_.size)
        if isinstance(vt, AbstractWString):
            return 'std::array<rosidl_runtime_cpp::MemoryRegion<char16_t>, {}>'.format(type_.size)
        if isinstance(vt, NamespacedType):
            return 'std::array<{}::ExternalStorage, {}>'.format(
                experimental_namespaced_type_name(vt), type_.size)
    if isinstance(type_, AbstractSequence):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            # Contiguous scalar buffer — a single region suffices.
            cpp_type = BASIC_TYPE_TO_EXPERIMENTAL_CPP[vt.typename]
            return 'rosidl_runtime_cpp::MemoryRegion<{}>'.format(cpp_type)
        if isinstance(vt, AbstractString):
            # One region per string element.
            return 'std::vector<rosidl_runtime_cpp::MemoryRegion<char>>'
        if isinstance(vt, AbstractWString):
            # One region per wstring element.
            return 'std::vector<rosidl_runtime_cpp::MemoryRegion<char16_t>>'
        if isinstance(vt, NamespacedType):
            # One ExternalStorage struct per message element.
            return 'std::vector<{}::ExternalStorage>'.format(
                experimental_namespaced_type_name(vt))
    assert False, type_


def experimental_storage_init_expr(member_name, type_):
    """Return the initializer-list expression for the ExternalStorage constructor.

    For scalar arrays and all non-array types the member is directly constructible
    from its storage field via a single-argument constructor.  For Array<non-scalar, N>
    the piecewise constructor is used, passing each storage element as its own
    single-element tuple — identical in shape to the PMR constructor.

    Sub-messages (NamespacedType) are constructed with SKIP so that _initialize()
    can propagate the actual MessageInitialization value afterwards.
    """
    if isinstance(type_, Array) and not isinstance(type_.value_type, BasicType):
        if isinstance(type_.value_type, NamespacedType):
            tuples = ', '.join(
                ['std::make_tuple(storage.members.{}[{}], rosidl_runtime_cpp::MessageInitialization::SKIP)'
                 .format(member_name, i) for i in range(type_.size)])
        else:
            tuples = ', '.join(
                ['std::make_tuple(storage.members.{}[{}])'.format(member_name, i)
                 for i in range(type_.size)])
        return '{}(std::piecewise_construct, {})'.format(member_name, tuples)
    if isinstance(type_, NamespacedType):
        return '{}(storage.members.{}, rosidl_runtime_cpp::MessageInitialization::SKIP)'.format(
            member_name, member_name)
    return '{}(storage.members.{})'.format(member_name, member_name)


def experimental_constraint_type(type_):
    """Return the C++ constraint type string for a member, or None if none is needed.

    Bounded types carry their limit in the type itself; no constraint is generated for them.

    Rules:
    - BasicType (Scalar<T>): fixed, no constraint.
    - Bounded string/wstring: bounded, no constraint.
    - Unbounded string/wstring: rosidl_runtime_cpp::StringConstraint (max characters via .size).
    - NamespacedType (sub-message): SubMsg::Constraints.
    - Array<BasicType, N>: fixed, no constraint.
    - Array<BoundedString, N>: bounded elements, no constraint.
    - Array<UnboundedString, N>: rosidl_runtime_cpp::StringConstraint (shared max chars via .size).
    - Array<NamespacedType, N>: SubMsg::Constraints (shared per-element constraint).
    - BoundedSequence<any>: bounded, no constraint.
    - UnboundedSequence<T>: rosidl_runtime_cpp::SequenceConstraint<elem_type>.
      Primary template (just 'size') covers scalars and bounded-string elements.
      SequenceConstraint<String/WString> adds 'element_size' for unbounded strings.
      Generated SequenceConstraint<Msg> adds 'element' for message elements.
    """
    if isinstance(type_, BasicType):
        return None
    if isinstance(type_, (AbstractString, AbstractWString)):
        return None if type_.has_maximum_size() else 'rosidl_runtime_cpp::StringConstraint'
    if isinstance(type_, NamespacedType):
        return '{}::Constraints'.format(experimental_namespaced_type_name(type_))
    if isinstance(type_, Array):
        vt = type_.value_type
        if isinstance(vt, BasicType):
            return None
        if isinstance(vt, (AbstractString, AbstractWString)):
            return None if vt.has_maximum_size() else 'rosidl_runtime_cpp::StringConstraint'
        if isinstance(vt, NamespacedType):
            return '{}::Constraints'.format(experimental_namespaced_type_name(vt))
        return None
    if isinstance(type_, AbstractSequence):
        if isinstance(type_, BoundedSequence):
            return None  # sequence bound is implied by the type, no constraint needed
        elem = msg_element_type_to_experimental_cpp(type_.value_type)
        return 'rosidl_runtime_cpp::SequenceConstraint<{}>'.format(elem)
    return None
