@# Included from rosidl_generator_cpp/resource/idl__experimental_struct.hpp.em
@{
from rosidl_parser.definition import SERVICE_EVENT_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX

experimental_service_typename = '::'.join(
    list(service.namespaced_type.namespaces) +
    ['experimental', service.namespaced_type.name])
}@
@{
TEMPLATE(
    'msg__experimental_struct.hpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.request_message, include_directives=include_directives)
}@

@{
TEMPLATE(
    'msg__experimental_struct.hpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.response_message, include_directives=include_directives)
}@

@{
TEMPLATE(
    'msg__experimental_struct.hpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.event_message, include_directives=include_directives)
}@

@[for ns in service.namespaced_type.namespaces]@
namespace @(ns)
{

@[end for]@
namespace experimental
{

struct @(service.namespaced_type.name)
{
  using Request = @(experimental_service_typename)@(SERVICE_REQUEST_MESSAGE_SUFFIX);
  using Response = @(experimental_service_typename)@(SERVICE_RESPONSE_MESSAGE_SUFFIX);
  using Event = @(experimental_service_typename)@(SERVICE_EVENT_MESSAGE_SUFFIX);
  using RequestConstraints = Request::Constraints;
  using ResponseConstraints = Response::Constraints;
};

}  // namespace experimental
@[for ns in reversed(service.namespaced_type.namespaces)]@

}  // namespace @(ns)
@[end for]@
