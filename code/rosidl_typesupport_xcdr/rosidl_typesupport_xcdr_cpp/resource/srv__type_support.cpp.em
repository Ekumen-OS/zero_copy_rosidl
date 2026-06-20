@# generated from rosidl_typesupport_xcdr_cpp/resource/srv__type_support.cpp.em
@# with input from @(package_name):@(interface_path)
@# generated code does not contain a copyright notice
@{
from rosidl_cmake import convert_camel_case_to_lower_case_underscore
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_EVENT_MESSAGE_SUFFIX

# Get optional force_experimental flag (set by experimental wrapper templates).
# When True, always generate experimental-message code paths regardless of namespace.
try:
    force_experimental
except NameError:
    force_experimental = False

service_typename = service.namespaced_type.name
standard_full_namespace = package_name + '::srv'
if force_experimental:
    service_full_namespace = package_name + '::srv::experimental'
else:
    service_full_namespace = standard_full_namespace

# For C symbol names, use a single token (e.g., 'srv_experimental') to
# avoid breaking the 4-arg macro.
parent_parts = list(interface_path.parents[0].parts)
if force_experimental:
    parent_parts = [parent_parts[0] + '_experimental']
service_underscore = convert_camel_case_to_lower_case_underscore(service_typename)
}@
@# Generate message typesupport for Request, Response, Event
@{
TEMPLATE(
    'msg__type_support.cpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.request_message, is_service_message=True,
    force_experimental=force_experimental)
}@

@{
TEMPLATE(
    'msg__type_support.cpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.response_message, is_service_message=True,
    force_experimental=force_experimental)
}@

@{
# Generate Event message typesupport only in the standard (non-experimental) file.
# The Event type wraps service_msgs::msg::ServiceEventInfo, which does NOT have
# experimental variants.  The experimental service file reuses the standard Event.
if not force_experimental:
    TEMPLATE(
        'msg__type_support.cpp.em',
        package_name=package_name, interface_path=interface_path,
        message=service.event_message, is_service_message=True,
        force_experimental=False)
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
get_service_type_support_handle<@(service_full_namespace)::@(service_typename)>()
{
  // Thread-safe lazy initialization (C++11 guarantees this for function-local statics)
  static const rosidl_service_type_support_t handle = {
    rosidl_typesupport_xcdr_cpp__identifier,
    nullptr,  // data (not needed for XCDR - we just forward to message typesupports)
    get_service_typesupport_handle_function,
@[if force_experimental]@
    rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(service_full_namespace)::@(service_typename)@(SERVICE_REQUEST_MESSAGE_SUFFIX)>(),
    rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(service_full_namespace)::@(service_typename)@(SERVICE_RESPONSE_MESSAGE_SUFFIX)>(),
    // Event typesupport is always in the standard (non-experimental) namespace
    rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<@(standard_full_namespace)::@(service_typename)@(SERVICE_EVENT_MESSAGE_SUFFIX)>(),
@[else]@
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
@[end if]@
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
  @(', '.join([package_name] + parent_parts)),
  @(service_typename))()
{
  return rosidl_typesupport_xcdr_cpp::get_service_type_support_handle<@(service_full_namespace)::@(service_typename)>();
}

#ifdef __cplusplus
}
#endif
