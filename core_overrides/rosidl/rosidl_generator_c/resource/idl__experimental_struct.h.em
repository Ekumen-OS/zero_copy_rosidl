// generated from rosidl_generator_c/resource/idl__experimental_struct.h.em
// with input from @(package_name):@(interface_path)
// generated code does not contain a copyright notice
@{from rosidl_pycommon import convert_camel_case_to_lower_case_underscore}
// IWYU pragma: private, include "@(package_name)/@('/'.join(interface_path.parents[0].parts))/experimental/@(convert_camel_case_to_lower_case_underscore(interface_path.stem)).h"

@
@#######################################################################
@# EmPy template for generating experimental/detail/<idl>__struct.h files
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
header_guard_variable = '__'.join([x.upper() for x in include_parts]) + '__STRUCT_H_'

include_directives = set()
}@

#ifndef @(header_guard_variable)
#define @(header_guard_variable)

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uchar.h>

#include "rcutils/allocator.h"
#include "rosidl_runtime_c/experimental/array.h"
#include "rosidl_runtime_c/experimental/constraints.h"
#include "rosidl_runtime_c/experimental/initialization.h"
#include "rosidl_runtime_c/experimental/memory.h"
#include "rosidl_runtime_c/experimental/scalar.h"
#include "rosidl_runtime_c/experimental/sequence.h"
#include "rosidl_runtime_c/experimental/storage.h"
#include "rosidl_runtime_c/experimental/string.h"

@#######################################################################
@# Handle message
@#######################################################################
@{
from rosidl_parser.definition import Message
}@
@[for message in content.get_elements_of_type(Message)]@
@{
TEMPLATE(
    'msg__experimental_struct.h.em',
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
    'msg__experimental_struct.h.em',
    package_name=package_name, interface_path=interface_path,
    message=service.request_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_struct.h.em',
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
    'msg__experimental_struct.h.em',
    package_name=package_name, interface_path=interface_path,
    message=action.goal, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_struct.h.em',
    package_name=package_name, interface_path=interface_path,
    message=action.result, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_struct.h.em',
    package_name=package_name, interface_path=interface_path,
    message=action.feedback, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_struct.h.em',
    package_name=package_name, interface_path=interface_path,
    message=action.send_goal_service.request_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_struct.h.em',
    package_name=package_name, interface_path=interface_path,
    message=action.send_goal_service.response_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_struct.h.em',
    package_name=package_name, interface_path=interface_path,
    message=action.get_result_service.request_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_struct.h.em',
    package_name=package_name, interface_path=interface_path,
    message=action.get_result_service.response_message, include_directives=include_directives,
    is_service_or_action_member=True)
}@

@{
TEMPLATE(
    'msg__experimental_struct.h.em',
    package_name=package_name, interface_path=interface_path,
    message=action.feedback_message, include_directives=include_directives)
}@

@[end for]@

#ifdef __cplusplus
}
#endif

#endif  // @(header_guard_variable)
