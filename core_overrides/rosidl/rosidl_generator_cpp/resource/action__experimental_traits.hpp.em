@# Included from rosidl_generator_cpp/resource/idl__experimental_traits.hpp.em
@{
from rosidl_parser.definition import ACTION_FEEDBACK_MESSAGE_SUFFIX
from rosidl_parser.definition import ACTION_FEEDBACK_SUFFIX
from rosidl_parser.definition import ACTION_GOAL_SERVICE_SUFFIX
from rosidl_parser.definition import ACTION_GOAL_SUFFIX
from rosidl_parser.definition import ACTION_RESULT_SERVICE_SUFFIX
from rosidl_parser.definition import ACTION_RESULT_SUFFIX

experimental_action_typename = '::'.join(
    list(action.namespaced_type.namespaces) +
    ['experimental', action.namespaced_type.name])
action_fully_qualified_name = '/'.join(action.namespaced_type.namespaced_name())
}@
@{
TEMPLATE(
    'msg__experimental_traits.hpp.em',
    package_name=package_name, interface_path=interface_path,
    message=action.goal, include_directives=include_directives)
}@

@{
TEMPLATE(
    'msg__experimental_traits.hpp.em',
    package_name=package_name, interface_path=interface_path,
    message=action.result, include_directives=include_directives)
}@

@{
TEMPLATE(
    'msg__experimental_traits.hpp.em',
    package_name=package_name, interface_path=interface_path,
    message=action.feedback, include_directives=include_directives)
}@

@{
TEMPLATE(
    'srv__experimental_traits.hpp.em',
    package_name=package_name, interface_path=interface_path,
    service=action.send_goal_service, include_directives=include_directives)
}@

@{
TEMPLATE(
    'srv__experimental_traits.hpp.em',
    package_name=package_name, interface_path=interface_path,
    service=action.get_result_service, include_directives=include_directives)
}@

@{
TEMPLATE(
    'msg__experimental_traits.hpp.em',
    package_name=package_name, interface_path=interface_path,
    message=action.feedback_message, include_directives=include_directives)
}@

namespace rosidl_generator_traits
{

template<>
constexpr const char * data_type<@(experimental_action_typename)>()
{
  return "@(experimental_action_typename)";
}

template<>
constexpr const char * name<@(experimental_action_typename)>()
{
  return "@(action_fully_qualified_name)";
}

template<>
struct is_action<@(experimental_action_typename)>
  : std::true_type
{
};

template<>
struct is_action_goal<@(experimental_action_typename)@(ACTION_GOAL_SUFFIX)>
  : std::true_type
{
};

template<>
struct is_action_result<@(experimental_action_typename)@(ACTION_RESULT_SUFFIX)>
  : std::true_type
{
};

template<>
struct is_action_feedback<@(experimental_action_typename)@(ACTION_FEEDBACK_SUFFIX)>
  : std::true_type
{
};

}  // namespace rosidl_generator_traits
