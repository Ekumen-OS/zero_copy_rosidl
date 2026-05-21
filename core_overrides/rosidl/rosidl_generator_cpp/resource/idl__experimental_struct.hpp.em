// generated from rosidl_generator_cpp/resource/idl__experimental_struct.hpp.em
// with input from @(package_name):@(interface_path)
// generated code does not contain a copyright notice
@{from rosidl_pycommon import convert_camel_case_to_lower_case_underscore}
// IWYU pragma: private, include "@(package_name)/experimental/@(convert_camel_case_to_lower_case_underscore(interface_path.stem)).hpp"

@
@#######################################################################
@# EmPy template for generating <idl>__experimental_struct.hpp files
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
header_guard_variable = '__'.join([x.upper() for x in include_parts]) + \
    '__STRUCT_HPP_'

include_directives = set()
}@

#ifndef @(header_guard_variable)
#define @(header_guard_variable)

#include <array>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "rosidl_runtime_cpp/experimental/array.hpp"
#include "rosidl_runtime_cpp/experimental/constraints.hpp"
#include "rosidl_runtime_cpp/experimental/memory.hpp"
#include "rosidl_runtime_cpp/experimental/scalar.hpp"
#include "rosidl_runtime_cpp/experimental/sequence.hpp"
#include "rosidl_runtime_cpp/experimental/string.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"

@#######################################################################
@# Handle message
@#######################################################################
@{
from rosidl_parser.definition import Message
}@
@[for message in content.get_elements_of_type(Message)]@
@{
TEMPLATE(
    'msg__experimental_struct.hpp.em',
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
    'srv__experimental_struct.hpp.em',
    package_name=package_name, interface_path=interface_path,
    service=service, include_directives=include_directives)
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
    'action__experimental_struct.hpp.em',
    package_name=package_name, interface_path=interface_path,
    action=action, include_directives=include_directives)
}@

@[end for]@
#endif  // @(header_guard_variable)
