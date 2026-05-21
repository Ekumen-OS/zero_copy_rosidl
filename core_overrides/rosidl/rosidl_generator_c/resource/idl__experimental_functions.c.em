// generated from rosidl_generator_c/resource/idl__experimental_functions.c.em
// with input from @(package_name):@(interface_path)
// generated code does not contain a copyright notice

@#######################################################################
@# EmPy template for generating experimental/detail/<idl>__functions.c files
@#
@# Context:
@#  - package_name (string)
@#  - interface_path (Path relative to the directory named after the package)
@#  - content (IdlContent, list of elements, e.g. Messages or Services)
@#######################################################################
@{
from rosidl_pycommon import convert_camel_case_to_lower_case_underscore
include_parts = [package_name] + list(interface_path.parents[0].parts) + [
    'experimental', 'detail', convert_camel_case_to_lower_case_underscore(interface_path.stem)]
include_base = '/'.join(include_parts)
}@

#include <assert.h>
#include <string.h>

#include "rcutils/allocator.h"
#include "rosidl_runtime_c/experimental/detail/value_helpers.h"
#include "rosidl_runtime_c/experimental/storage.h"

#include "@(include_base)__functions.h"
@#######################################################################
@# Handle message
@#######################################################################
@{
from rosidl_parser.definition import Message
include_directives = {"{}__functions.h".format(include_base)}
}@
@[for message in content.get_elements_of_type(Message)]@
@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=message, include_directives=include_directives)
}@
@[end for]@
@#######################################################################
@# Handle service
@#######################################################################
@{
from rosidl_parser.definition import Service
}@
@[for service in content.get_elements_of_type(Service)]@

@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=service.request_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=service.response_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@
@[end for]@
@#######################################################################
@# Handle action
@#######################################################################
@{
from rosidl_parser.definition import Action
}@
@[for action in content.get_elements_of_type(Action)]@

@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=action.goal, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=action.result, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=action.feedback, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=action.send_goal_service.request_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=action.send_goal_service.response_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=action.get_result_service.request_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=action.get_result_service.response_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_functions.c.em',
    package_name=package_name, interface_path=interface_path,
    message=action.feedback_message, include_directives=include_directives)
}@
@[end for]@
