@# Included from rosidl_generator_cpp/resource/idl__experimental_struct.hpp.em
@{
from rosidl_generator_cpp import escape_string
from rosidl_generator_cpp import escape_wstring
from rosidl_generator_cpp.experimental import BASIC_TYPE_TO_EXPERIMENTAL_CPP
from rosidl_generator_cpp.experimental import EXPERIMENTAL_CHARACTER_TYPES
from rosidl_generator_cpp.experimental import experimental_constraint_type
from rosidl_generator_cpp.experimental import experimental_member_needs_pmr
from rosidl_generator_cpp.experimental import experimental_pmr_init_expr
from rosidl_generator_cpp.experimental import experimental_storage_init_expr
from rosidl_generator_cpp.experimental import experimental_storage_type
from rosidl_generator_cpp.experimental import msg_type_to_experimental_cpp
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
from rosidl_parser.definition import CHARACTER_TYPES
from rosidl_parser.definition import EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME
from rosidl_parser.definition import INTEGER_TYPES
from rosidl_parser.definition import NamespacedType
from rosidl_parser.definition import OCTET_TYPE
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX
from rosidl_parser.definition import UNSIGNED_INTEGER_TYPES

message_typename = '::'.join(
    list(message.structure.namespaced_type.namespaces) +
    ['experimental', message.structure.namespaced_type.name])

msvc_common_macros = ('DELETE', 'ERROR', 'NO_ERROR')
}@
@
@#<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
@# Collect include directives for sub-message member types
@{
from collections import OrderedDict
from rosidl_pycommon import convert_camel_case_to_lower_case_underscore
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
        if (
            type_.name.endswith(ACTION_GOAL_SUFFIX) or
            type_.name.endswith(ACTION_RESULT_SUFFIX) or
            type_.name.endswith(ACTION_FEEDBACK_SUFFIX)
        ):
            typename = type_.name.rsplit('_', 1)[0]
        else:
            typename = type_.name
        member_names = includes.setdefault(
            '/'.join(type_.namespaces + ['experimental', 'detail',
                convert_camel_case_to_lower_case_underscore(typename)]) +
            '__struct.hpp', [])
        member_names.append(member.name)
}@
@#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
@
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

@[for ns in message.structure.namespaced_type.namespaces]@
namespace @(ns)
{

@[end for]@
namespace experimental
{

// experimental message struct
struct @(message.structure.namespaced_type.name)
{
  using Type = @(message.structure.namespaced_type.name);

@{
storage_fields = [
    (m.name, experimental_storage_type(m.type))
    for m in message.structure.members
]
}@
  // storage for external memory initialization
  struct ExternalStorage
  {
    rosidl_runtime_cpp::MemoryRegion<void> block{};

    struct
    {
@[for field_name, field_type in storage_fields]@
      @(field_type) @(field_name){};
@[end for]@
    } members;

    bool operator==(const ExternalStorage & other) const
    {
      if (this->block != other.block) {
        return false;
      }
@[for field_name, _ in storage_fields]@
      if (this->members.@(field_name) != other.members.@(field_name)) {
        return false;
      }
@[end for]@
      return true;
    }
    bool operator!=(const ExternalStorage & other) const
    {
      return !this->operator==(other);
    }
  };  // struct ExternalStorage

@{
from rosidl_generator_cpp.experimental import generate_experimental_default_string
from rosidl_generator_cpp.experimental import create_experimental_member_list
from rosidl_generator_cpp.experimental import generate_experimental_zero_string

init_list, member_list = create_experimental_member_list(message)

default_value_members = [m for m in member_list if m.members[0].default_value]
zero_value_members = [m for m in member_list if m.members[0].zero_value]
non_defaulted_zero_initialized_members = [
    m for m in member_list if m.members[0].zero_value and not m.members[0].default_value
]

pmr_init_list = [
    experimental_pmr_init_expr(m.name, m.type)
    for m in message.structure.members
    if experimental_member_needs_pmr(m.type)
]
storage_init_list = [
    experimental_storage_init_expr(m.name, m.type)
    for m in message.structure.members
]

# Sub-message members (NamespacedType, or an Array thereof) that
# need _reset() called on them to propagate _init recursively.
submsg_members = [
    m for m in message.structure.members
    if isinstance(m.type, NamespacedType) or (
        isinstance(m.type, (Array, AbstractSequence)) and
        isinstance(m.type.value_type, NamespacedType))
]
}@
  explicit @(message.structure.namespaced_type.name)(
    rosidl_runtime_cpp::MessageInitialization _init =
    rosidl_runtime_cpp::MessageInitialization::ALL)
@[if init_list]@
  : @(',\n    '.join(init_list))
@[end if]@
  {
    _reset(_init);
  }

  explicit @(message.structure.namespaced_type.name)(
    std::pmr::memory_resource * mem_res,
    rosidl_runtime_cpp::MessageInitialization _init =
    rosidl_runtime_cpp::MessageInitialization::ALL)
@[if pmr_init_list]@
  : @(',\n    '.join(pmr_init_list))
@[end if]@
  {
@[if not pmr_init_list]@
    (void)mem_res;
@[end if]@
    _reset(_init);
  }

  explicit @(message.structure.namespaced_type.name)(
    const ExternalStorage & storage,
    rosidl_runtime_cpp::MessageInitialization _init =
    rosidl_runtime_cpp::MessageInitialization::ALL)
@[if storage_init_list]@
  : @(',\n    '.join(storage_init_list)),
    _external_storage(storage)
@[end if]@
  {
    _reset(_init);
  }

  @(message.structure.namespaced_type.name)(const @(message.structure.namespaced_type.name) & other)
@[if message.structure.members]@
  : @(',\n    '.join('{name}(other.{name})'.format(name=member.name) for member in message.structure.members))
@[end if]@
  {
  }

  @(message.structure.namespaced_type.name)(@(message.structure.namespaced_type.name) && other) = default;

  @(message.structure.namespaced_type.name) & operator=(const @(message.structure.namespaced_type.name) & other)
  {
@[for member in message.structure.members]@
    this->@(member.name) = other.@(member.name);
@[end for]@
    return *this;
  }

  @(message.structure.namespaced_type.name) & operator=(@(message.structure.namespaced_type.name) && other)
  {
@[for member in message.structure.members]@
    this->@(member.name) = std::move(other.@(member.name));
@[end for]@
    return *this;
  }

  // field types and members
@[for member in message.structure.members]@
  using _@(member.name)_type =
    @(msg_type_to_experimental_cpp(member.type));
  _@(member.name)_type @(member.name);
@[end for]@
  std::optional<ExternalStorage> _external_storage;

  void swap(@(message.structure.namespaced_type.name) & other)
  {
    using std::swap;
@[for member in message.structure.members]@
    swap(this->@(member.name), other.@(member.name));
@[end for]@
    swap(this->_external_storage, other._external_storage);
  }

  friend void swap(@(message.structure.namespaced_type.name) & lhs, @(message.structure.namespaced_type.name) & rhs)
  {
    lhs.swap(rhs);
  }
@[if len(message.structure.members) != 1 or message.structure.members[0].name != EMPTY_STRUCTURE_REQUIRED_MEMBER_NAME]@

  // setters for named parameter idiom
@[    for member in message.structure.members]@
  Type & set__@(member.name)(
    const @(msg_type_to_experimental_cpp(member.type)) & _arg)
  {
    this->@(member.name) = _arg;
    return *this;
  }
@[    end for]@
@[end if]@
@[if message.constants]@

  // constant declarations
@[  for constant in message.constants]@
@[    if constant.name in msvc_common_macros]@
  // guard against '@(constant.name)' being predefined by MSVC by temporarily undefining it
#if defined(_WIN32)
#  if defined(@(constant.name))
#    pragma push_macro("@(constant.name)")
#    undef @(constant.name)
#  endif
#endif
@[    end if]@
@[    if isinstance(constant.type, AbstractString)]@
  inline static const std::string @(constant.name) = "@(escape_string(constant.value))";
@[    elif isinstance(constant.type, AbstractWString)]@
  inline static const std::u16string @(constant.name) = u"@(escape_wstring(constant.value))";
@[    else]@
  static constexpr @(BASIC_TYPE_TO_EXPERIMENTAL_CPP[constant.type.typename]) @(constant.name) =
@[      if constant.type.typename in (*INTEGER_TYPES, *CHARACTER_TYPES, BOOLEAN_TYPE, OCTET_TYPE)]@
    @(int(constant.value))@
@[        if constant.type.typename in UNSIGNED_INTEGER_TYPES]@
u@
@[        end if]@
@[      elif constant.type.typename == 'float']@
    @(constant.value)f@
@[      else]@
    @(constant.value)@
@[      end if];
@[    end if]@
@[    if constant.name in msvc_common_macros]@
#if defined(_WIN32)
#  pragma warning(suppress : 4602)
#  pragma pop_macro("@(constant.name)")
#endif
@[    end if]@
@[  end for]@
@[end if]@

  void _reset(
    rosidl_runtime_cpp::MessageInitialization _init =
    rosidl_runtime_cpp::MessageInitialization::ALL)
  {
@[if not member_list and not submsg_members]@
    (void)_init;
@[end if]@
    switch(_init) {
@[if default_value_members or non_defaulted_zero_initialized_members]@
      case rosidl_runtime_cpp::MessageInitialization::ALL:
@[  if default_value_members]@
@[    for membset in default_value_members]@
@[      for line in generate_experimental_default_string(membset)]@
        @(line)
@[      end for]@
@[    end for]@
@[  end if]@
@[  if non_defaulted_zero_initialized_members]@
@[    for membset in non_defaulted_zero_initialized_members]@
@[      for line in generate_experimental_zero_string(membset)]@
@[        if line]@
        @(line)
@[        end if]@
@[      end for]@
@[    end for]@
@[  end if]@
        break;
@[end if]@
@[if default_value_members]@
      case rosidl_runtime_cpp::MessageInitialization::DEFAULTS_ONLY:
@[  for membset in default_value_members]@
@[    for line in generate_experimental_default_string(membset)]@
        @(line)
@[    end for]@
@[  end for]@
        break;
@[end if]@
@[if zero_value_members]@
      case rosidl_runtime_cpp::MessageInitialization::ZERO:
@[  for membset in zero_value_members]@
@[    for line in generate_experimental_zero_string(membset)]@
        @(line)
@[    end for]@
@[  end for]@
        break;
@[end if]@
      default:
        break;
    }
@[if submsg_members]@
    // propagate _init recursively to sub-message members
@[  for member in submsg_members]@
@[    if isinstance(member.type, (Array, AbstractSequence))]@
    for (auto & _elem : this->@(member.name)) {
      _elem._reset(_init);
    }
@[    else]@
    this->@(member.name)._reset(_init);
@[    end if]@
@[  end for]@
@[end if]@
  }

  // pointer types
  using RawPtr = @(message.structure.namespaced_type.name) *;
  using ConstRawPtr = const @(message.structure.namespaced_type.name) *;
  using SharedPtr = std::shared_ptr<@(message.structure.namespaced_type.name)>;
  using ConstSharedPtr =
    std::shared_ptr<const @(message.structure.namespaced_type.name)>;
  using UniquePtr = std::unique_ptr<@(message.structure.namespaced_type.name)>;
  using ConstUniquePtr =
    std::unique_ptr<const @(message.structure.namespaced_type.name)>;
  using WeakPtr = std::weak_ptr<@(message.structure.namespaced_type.name)>;
  using ConstWeakPtr =
    std::weak_ptr<const @(message.structure.namespaced_type.name)>;

  // comparison operators
  bool operator==(const @(message.structure.namespaced_type.name) & other) const
  {
@[if not message.structure.members]@
    (void)other;
@[end if]@
@[for member in message.structure.members]@
    if (this->@(member.name) != other.@(member.name)) {
      return false;
    }
@[end for]@
    return true;
  }
  bool operator!=(const @(message.structure.namespaced_type.name) & other) const
  {
    return !this->operator==(other);
  }

@{
constraint_fields = [
    (m.name, experimental_constraint_type(m.type))
    for m in message.structure.members
    if experimental_constraint_type(m.type) is not None
]
}@
  // constraints for variable-length members
  struct Constraints
  {
@[if constraint_fields]@
@[  for field_name, field_type in constraint_fields]@
    @(field_type) @(field_name){};
@[  end for]@

@[end if]@
    bool operator==(const Constraints & other) const
    {
@[if not constraint_fields]@
      (void)other;
@[end if]@
@[for field_name, _ in constraint_fields]@
      if (this->@(field_name) != other.@(field_name)) {
        return false;
      }
@[end for]@
      return true;
    }
    bool operator!=(const Constraints & other) const
    {
      return !this->operator==(other);
    }
  };  // struct Constraints
};  // struct @(message.structure.namespaced_type.name)

}  // namespace experimental
@[for ns in reversed(message.structure.namespaced_type.namespaces)]@

}  // namespace @(ns)
@[end for]@

namespace rosidl_runtime_cpp
{

// Constraints for a sequence of @(message_typename) messages.
template<>
struct SequenceConstraint<@(message_typename)>
{
  // Maximum number of elements in the sequence.
  std::size_t size{0};
  // Constraints applied to each element in the sequence.
  @(message_typename)::Constraints element{};

  bool operator==(const SequenceConstraint & other) const
  {
    if (size != other.size) {
      return false;
    }
    if (element != other.element) {
      return false;
    }
    return true;
  }

  bool operator!=(const SequenceConstraint & other) const
  {
    return !(*this == other);
  }
};

}  // namespace rosidl_runtime_cpp
