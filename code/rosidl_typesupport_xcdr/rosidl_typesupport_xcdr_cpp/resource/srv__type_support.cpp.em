@# generated from rosidl_typesupport_xcdr_cpp/resource/srv__type_support.cpp.em
@# with input from @(package_name):@(interface_path)
@# generated code does not contain a copyright notice
@{
from rosidl_cmake import convert_camel_case_to_lower_case_underscore
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_EVENT_MESSAGE_SUFFIX

service_typename = service.namespaced_type.name
package_parts = package_name.split('_')
service_underscore = convert_camel_case_to_lower_case_underscore(service_typename)
}@
@# Generate message typesupport for Request, Response, Event
@{
TEMPLATE(
    'msg__type_support.cpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.request_message, is_service_message=True)
}@

@{
TEMPLATE(
    'msg__type_support.cpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.response_message, is_service_message=True)
}@

@{
TEMPLATE(
    'msg__type_support.cpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.event_message, is_service_message=True)
}@

#include <cstddef>

#include "rosidl_runtime_c/service_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "rosidl_typesupport_xcdr_cpp/identifier.hpp"
#include "rosidl_typesupport_xcdr_cpp/service_type_support.hpp"
#include "@(package_name)/srv/detail/@(service_underscore)__struct.hpp"

namespace rosidl_typesupport_xcdr_cpp
{

template<>
const rosidl_service_type_support_t *
get_service_type_support_handle<@(package_name)::srv::@(service_typename)>()
{
  // Thread-safe lazy initialization (C++11 guarantees this for function-local statics)
  static const rosidl_service_type_support_t handle = {
    rosidl_typesupport_xcdr_cpp__identifier,
    nullptr,  // data (not needed for XCDR - we just forward to message typesupports)
    get_service_typesupport_handle_function,
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
      rosidl_typesupport_xcdr_cpp,
      @(package_name), srv,
      @(service_typename)@(SERVICE_REQUEST_MESSAGE_SUFFIX))(),
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
      rosidl_typesupport_xcdr_cpp,
      @(package_name), srv,
      @(service_typename)@(SERVICE_RESPONSE_MESSAGE_SUFFIX))(),
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
      rosidl_typesupport_xcdr_cpp,
      @(package_name), srv,
      @(service_typename)@(SERVICE_EVENT_MESSAGE_SUFFIX))(),
    nullptr,  // event_message_create_handle_function (TODO: implement if needed)
    nullptr,  // event_message_destroy_handle_function (TODO: implement if needed)
    nullptr,  // get_type_hash_func (TODO: implement if needed)
    nullptr,  // get_type_description_func (TODO: implement if needed)
    nullptr,  // get_type_description_sources_func (TODO: implement if needed)
  };
  
  return &handle;
}

}  // namespace rosidl_typesupport_xcdr_cpp

// C symbol export
#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC_@(package_name.upper())
const rosidl_service_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpp,
  @(', '.join([package_name] + list(interface_path.parents[0].parts))),
  @(service_typename))()
{
  return rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<@(package_name)::srv::@(service_typename)>();
}

#ifdef __cplusplus
}
#endif
