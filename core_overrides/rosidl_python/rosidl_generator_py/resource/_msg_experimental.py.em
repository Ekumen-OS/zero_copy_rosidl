@# Included from rosidl_generator_py/resource/_idl_experimental.py.em
@{
from rosidl_pycommon import convert_camel_case_to_lower_case_underscore
from rosidl_generator_py.experimental import BASIC_TYPE_TO_DTYPE
from rosidl_generator_py.experimental import experimental_constraint_type
from rosidl_generator_py.experimental import experimental_default_value_expr
from rosidl_generator_py.experimental import experimental_msg_type
from rosidl_generator_py.experimental import experimental_storage_init_expr
from rosidl_generator_py.experimental import experimental_storage_type
from rosidl_generator_py.experimental import experimental_submsg_members
from rosidl_generator_py.generate_py_impl import constant_value_to_py
from rosidl_parser.definition import AbstractGenericString
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
from rosidl_parser.definition import EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME
from rosidl_parser.definition import NamespacedType
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX
from rosidl_parser.definition import UnboundedSequence
}@
@
@#<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
@# Collect import directives for sub-message member types
@{
from collections import OrderedDict
submsg_imports = OrderedDict()
for member in message.structure.members:
    type_ = member.type
    if isinstance(type_, AbstractNestedType):
        type_ = type_.value_type
    if isinstance(type_, NamespacedType):
        if (
            message.structure.namespaced_type.namespaces[-1] in ['action', 'srv'] and (
                type_.name.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
                type_.name.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX)
            )
        ):
            typename = type_.name.rsplit('_', 1)[0]
            if typename == message.structure.namespaced_type.name.rsplit('_', 1)[0]:
                continue
        if (
            type_.name.endswith(ACTION_GOAL_SUFFIX) or
            type_.name.endswith(ACTION_RESULT_SUFFIX) or
            type_.name.endswith(ACTION_FEEDBACK_SUFFIX)
        ):
            typename = type_.name.rsplit('_', 1)[0]
        else:
            typename = type_.name
        module_path = '.'.join(type_.namespaces) + '.experimental'
        submsg_imports.setdefault((module_path, type_.name), []).append(member.name)
}@
@#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
@
@[if submsg_imports]@
# Import sub-message types
@[  for (module_path, type_name), member_names in submsg_imports.items()]@
@[    for member_name in member_names]@
# Member '@(member_name)'
@[    end for]@
from @(module_path) import @(type_name)
@[  end for]@
@[end if]@

@{
# Check if this is an empty structure
is_empty_struct = (
    len(message.structure.members) == 1 and
    message.structure.members[0].name == EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME
)

# Gather constraint fields
constraint_fields = []
for m in message.structure.members:
    ct = experimental_constraint_type(m.type)
    if ct is not None:
        constraint_fields.append((m.name, ct))
}@

class @(message.structure.namespaced_type.name):
    """Experimental message class '@(message.structure.namespaced_type.name)'.

    Uses experimental container types from rosidl_runtime_py.experimental.
    """

@[if message.constants]@
    # Constants
@[  for constant in message.constants]@
    @(constant.name) = @constant_value_to_py(constant.type, constant.value)
@[  end for]@

@[end if]@
    class Constraints:
        """Per-member constraints for variable-length members."""

        __slots__ = (@
@[if constraint_fields]@
'@("', '".join(name for name, _ in constraint_fields))',@
@[end if]@
)

        def __init__(self@
@[for field_name, field_type in constraint_fields]@
, @(field_name)=None@
@[end for]@
):
@[if constraint_fields]@
@[  for field_name, field_type in constraint_fields]@
            self.@(field_name) = @(field_name) if @(field_name) is not None else @(field_type)()
@[  end for]@
@[else]@
            pass
@[end if]@

        def __eq__(self, other):
            if not isinstance(other, type(self)):
                return NotImplemented
@[if not constraint_fields]@
            return True
@[else]@
@[  for field_name, _ in constraint_fields]@
            if self.@(field_name) != other.@(field_name):
                return False
@[  end for]@
            return True
@[end if]@

        def __ne__(self, other):
            return not self.__eq__(other)

        def __repr__(self):
@[if constraint_fields]@
            typename = type(self).__qualname__
            fields = []
@[  for field_name, _ in constraint_fields]@
            fields.append('@(field_name)={!r}'.format(self.@(field_name)))
@[  end for]@
            return '@(message.structure.namespaced_type.name).Constraints({})'.format(', '.join(fields))
@[else]@
            return '@(message.structure.namespaced_type.name).Constraints()'
@[end if]@

@{
# Gather ExternalStorage fields
storage_fields = []
if not is_empty_struct:
    for m in message.structure.members:
        storage_type = experimental_storage_type(m.type)
        storage_fields.append((m.name, storage_type))
}@
    @@dataclasses.dataclass(slots=True)
    class ExternalStorage:
        """External memory descriptors for zero-copy initialization.

        Attributes:
            block: Optional contiguous buffer for entire message.
            members: Per-field storage descriptors.

        Each field descriptor specifies the external memory region(s) to use
        when constructing a message with _storage= parameter.  All fields use
        RawBuffer as the descriptor type:

        - Scalars, strings, primitive arrays/sequences: single RawBuffer
        - Arrays/sequences of strings/messages: list of RawBuffers/ExternalStorages
        - Nested messages: recursive SubMsg.ExternalStorage
        """

        @@dataclasses.dataclass(slots=True)
        class Members:
            """Per-field storage descriptors."""
@[if storage_fields]@
@[  for name, storage_type in storage_fields]@
            @(name): @(storage_type) | None = None
@[  end for]@
@[else]@
            pass
@[end if]@

        block: RawBuffer | None = None
        members: Members = dataclasses.field(default_factory=Members)

        def __post_init__(self):
            # Fast type assertion for debug builds (no-op with -O)
            assert self.block is None or isinstance(self.block, RawBuffer), \
                'ExternalStorage.block must be RawBuffer or None'

    __slots__ = (
@[if not is_empty_struct]@
@[  for member in message.structure.members]@
        '@(member.name)',
@[  end for]@
@[end if]@
        '_external_storage',
    )

    def __init__(
        self,
        _init=MessageInitialization.ALL,
        *,
        _storage=None,
        **kwargs,
    ):
        if _storage is not None:
            # Store reference to keep storage descriptors alive
            self._external_storage = _storage
            # Initialize from external storage
@[if not is_empty_struct]@
@[  for member in message.structure.members]@
@{
init_expr = experimental_storage_init_expr(member.name, member.type)
# Determine if this is a complex type needing custom initialization
is_array = isinstance(member.type, Array)
is_sequence = isinstance(member.type, AbstractSequence)
vt = member.type.value_type if (is_array or is_sequence) else None
is_string = isinstance(vt, (AbstractString, AbstractWString)) if vt else False
is_msg = isinstance(vt, NamespacedType) if vt else False
is_bounded_string = (vt.has_maximum_size() if is_string else False)
is_wstring = isinstance(vt, AbstractWString) if is_string else False
# Combined flags for simpler template logic
is_bounded_wstring = is_wstring and is_bounded_string
is_unbounded_wstring = is_wstring and not is_bounded_string
is_bounded_regular_string = is_bounded_string and not is_wstring
is_unbounded_regular_string = is_string and not is_bounded_string and not is_wstring
# Compound conditions for top-level branching
is_array_of_strings = is_array and is_string
is_sequence_of_strings = is_sequence and is_string
is_array_of_msgs = is_array and is_msg
is_sequence_of_msgs = is_sequence and is_msg
}@
@[  if init_expr]@
            self.@(member.name) = @(init_expr)
@[  elif is_array_of_strings]@
            # Array of strings from list[RawBuffer]
            self.@(member.name) = @(experimental_msg_type(member.type))
            storage_bufs = _storage.members.@(member.name)
            if storage_bufs is not None:
                assert isinstance(storage_bufs, list) and len(storage_bufs) == @(member.type.size)
                for i, buf in enumerate(storage_bufs):
@[    if is_bounded_wstring]@
                    self.@(member.name)[i] = BoundedWString(@(vt.maximum_size), buffer=buf)
@[    elif is_unbounded_wstring]@
                    self.@(member.name)[i] = WString(buffer=buf)
@[    elif is_bounded_regular_string]@
                    self.@(member.name)[i] = BoundedString(@(vt.maximum_size), buffer=buf)
@[    else]@
                    self.@(member.name)[i] = String(buffer=buf)
@[    end if]@
@[  elif is_sequence_of_strings]@
            # Sequence of strings from list[RawBuffer]
            self.@(member.name) = @(experimental_msg_type(member.type))
            storage_bufs = _storage.members.@(member.name)
            if storage_bufs is not None:
                assert isinstance(storage_bufs, list)
                for buf in storage_bufs:
@[    if is_bounded_wstring]@
                    self.@(member.name).append(BoundedWString(@(vt.maximum_size), buffer=buf))
@[    elif is_unbounded_wstring]@
                    self.@(member.name).append(WString(buffer=buf))
@[    elif is_bounded_regular_string]@
                    self.@(member.name).append(BoundedString(@(vt.maximum_size), buffer=buf))
@[    else]@
                    self.@(member.name).append(String(buffer=buf))
@[    end if]@
@[  elif is_array_of_msgs]@
            # Array of messages from list[SubMsg.ExternalStorage]
            self.@(member.name) = @(experimental_msg_type(member.type))
            storage_msgs = _storage.members.@(member.name)
            if storage_msgs is not None:
                assert isinstance(storage_msgs, list) and len(storage_msgs) == @(member.type.size)
                for i, msg_storage in enumerate(storage_msgs):
                    self.@(member.name)[i] = @(vt.name)(_storage=msg_storage, _init=MessageInitialization.SKIP)
@[  elif is_sequence_of_msgs]@
            # Sequence of messages from list[SubMsg.ExternalStorage]
            self.@(member.name) = @(experimental_msg_type(member.type))
            storage_msgs = _storage.members.@(member.name)
            if storage_msgs is not None:
                assert isinstance(storage_msgs, list)
                for msg_storage in storage_msgs:
                    self.@(member.name).append(@(vt.name)(_storage=msg_storage, _init=MessageInitialization.SKIP))
@[  else]@
            # Fallback - shouldn't reach here for known types
            self.@(member.name) = @(experimental_msg_type(member.type))
@[  end if]@
@[  end for]@
@[end if]@
        else:
            self._external_storage = None
            # Managed storage (default behavior)
@[if not is_empty_struct]@
@[  for member in message.structure.members]@
            self.@(member.name) = @(experimental_msg_type(member.type))
@[  end for]@
@[end if]@
        self._reset(_init)
        # Apply keyword overrides
        for key, value in kwargs.items():
            if not hasattr(self, key):
                raise TypeError(
                    "@(message.structure.namespaced_type.name)() "
                    f"got an unexpected keyword argument {key!r}")
            setattr(self, key, value)

    def _reset(self, _init=MessageInitialization.ALL):
        """Initialise fields based on *_init* policy."""
@{
default_members = []
zero_members = []
for member in message.structure.members:
    dval = experimental_default_value_expr(member)
    if dval is not None:
        default_members.append((member, dval))
submsg_list = list(experimental_submsg_members(message))
}@
@[if not default_members and not submsg_list]@
        pass
@[else]@
@[  if default_members]@
        if _init == MessageInitialization.ALL:
@[    for member, lines in default_members]@
@[      for line in lines]@
            @(line)
@[      end for]@
@[    end for]@
        elif _init == MessageInitialization.DEFAULTS_ONLY:
@[    for member, lines in default_members]@
@[      for line in lines]@
            @(line)
@[      end for]@
@[    end for]@
@[  end if]@
@[  if submsg_list]@
        # Propagate _init to sub-message members
@[    for member, is_nested in submsg_list]@
@[      if is_nested]@
        for _elem in self.@(member.name):
            _elem._reset(_init)
@[      else]@
        self.@(member.name)._reset(_init)
@[      end if]@
@[    end for]@
@[  end if]@
@[end if]@

    def __eq__(self, other):
        if not isinstance(other, @(message.structure.namespaced_type.name)):
            return False
@[if not is_empty_struct]@
@[  for member in message.structure.members]@
        if self.@(member.name) != other.@(member.name):
            return False
@[  end for]@
@[end if]@
        return True

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
@[if not is_empty_struct]@
        fields = []
@[  for member in message.structure.members]@
        fields.append('@(member.name)={!r}'.format(self.@(member.name)))
@[  end for]@
        return '@(message.structure.namespaced_type.name)({})'.format(', '.join(fields))
@[else]@
        return '@(message.structure.namespaced_type.name)()'
@[end if]
