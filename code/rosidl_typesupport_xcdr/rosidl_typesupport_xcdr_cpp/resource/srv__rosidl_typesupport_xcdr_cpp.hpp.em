@# generated from rosidl_typesupport_xcdr_cpp/resource/srv__rosidl_typesupport_xcdr_cpp.hpp.em
@# with input from @(package_name):@(interface_path)
@# generated code does not contain a copyright notice
@{
from rosidl_cmake import convert_camel_case_to_lower_case_underscore
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_EVENT_MESSAGE_SUFFIX

service_typename = service.namespaced_type.name
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
  @(', '.join([package_name] + list(interface_path.parents[0].parts))),
  @(service_typename)@(SERVICE_REQUEST_MESSAGE_SUFFIX))();

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpp,
  @(', '.join([package_name] + list(interface_path.parents[0].parts))),
  @(service_typename)@(SERVICE_RESPONSE_MESSAGE_SUFFIX))();

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpp,
  @(', '.join([package_name] + list(interface_path.parents[0].parts))),
  @(service_typename)@(SERVICE_EVENT_MESSAGE_SUFFIX))();

// Declare service typesupport
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_service_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpp,
  @(', '.join([package_name] + list(interface_path.parents[0].parts))),
  @(service_typename))();

#ifdef __cplusplus
}
#endif

namespace rosidl_typesupport_xcdr_cpp
{

template<>
ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_service_type_support_t *
get_service_type_support_handle<@(package_name)::srv::@(service_typename)>();

}  // namespace rosidl_typesupport_xcdr_cpp

#endif  // @(header_guard_variable)
