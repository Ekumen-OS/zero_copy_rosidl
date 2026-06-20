@# generated from rosidl_typesupport_xcdr_cpp/resource/srv__rosidl_typesupport_xcdr_cpp.hpp.em
@# with input from @(package_name):@(interface_path)
@# generated code does not contain a copyright notice
@{
from rosidl_cmake import convert_camel_case_to_lower_case_underscore
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_EVENT_MESSAGE_SUFFIX

# Get optional force_experimental flag (set by experimental wrapper templates).
try:
    force_experimental
except NameError:
    force_experimental = False

service_typename = service.namespaced_type.name
if force_experimental:
    service_full_namespace = package_name + '::srv::experimental'
else:
    service_full_namespace = package_name + '::srv'

# For C symbol names, use a single token (e.g., 'srv_experimental') to
# avoid breaking the 4-argument ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME macro.
symbol_parent_parts = list(interface_path.parents[0].parts)
if force_experimental:
    symbol_parent_parts = [symbol_parent_parts[0] + '_experimental']

header_guard_parts = [
    package_name] + list(interface_path.parents[0].parts) + [
    'detail', convert_camel_case_to_lower_case_underscore(service_typename) + '__rosidl_typesupport_xcdr_cpp_hpp']
header_guard_variable = '__'.join([x.upper() for x in header_guard_parts]) + '_'
}@
#ifndef @(header_guard_variable)
#define @(header_guard_variable)

#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/service_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "rosidl_typesupport_xcdr_cpp/service_type_support.hpp"
#include "rosidl_typesupport_xcdr_cpp/visibility_control.h"
#include "@(package_name)/msg/rosidl_typesupport_xcdr_cpp__visibility_control.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Declare message typesupport for Request, Response, and Event
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpp,
  @(', '.join([package_name] + symbol_parent_parts)),
  @(service_typename)@(SERVICE_REQUEST_MESSAGE_SUFFIX))();

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpp,
  @(', '.join([package_name] + symbol_parent_parts)),
  @(service_typename)@(SERVICE_RESPONSE_MESSAGE_SUFFIX))();

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpp,
  @(', '.join([package_name] + symbol_parent_parts)),
  @(service_typename)@(SERVICE_EVENT_MESSAGE_SUFFIX))();

// Declare service typesupport
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_service_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpp,
  @(', '.join([package_name] + symbol_parent_parts)),
  @(service_typename))();

#ifdef __cplusplus
}
#endif

namespace rosidl_typesupport_xcdr_cpp
{

template<>
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_service_type_support_t *
get_service_type_support_handle<@(service_full_namespace)::@(service_typename)>();

}  // namespace rosidl_typesupport_xcdr_cpp

#endif  // @(header_guard_variable)
