@# Included from rosidl_generator_cpp/resource/idl__experimental_traits.hpp.em
@{
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX

experimental_service_typename = '::'.join(
    list(service.namespaced_type.namespaces) +
    ['experimental', service.namespaced_type.name])
service_fully_qualified_name = '/'.join(service.namespaced_type.namespaced_name())
}@
@{
TEMPLATE(
    'msg__experimental_traits.hpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.request_message, include_directives=include_directives)
}@

@{
TEMPLATE(
    'msg__experimental_traits.hpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.response_message, include_directives=include_directives)
}@

@{
TEMPLATE(
    'msg__experimental_traits.hpp.em',
    package_name=package_name, interface_path=interface_path,
    message=service.event_message, include_directives=include_directives)
}@

namespace rosidl_generator_traits
{

template<>
constexpr const char * data_type<@(experimental_service_typename)>()
{
  return "@(experimental_service_typename)";
}

template<>
constexpr const char * name<@(experimental_service_typename)>()
{
  return "@(service_fully_qualified_name)";
}

template<>
struct has_fixed_size<@(experimental_service_typename)>
  : std::integral_constant<
    bool,
    has_fixed_size<@(experimental_service_typename)@(SERVICE_REQUEST_MESSAGE_SUFFIX)>::value &&
    has_fixed_size<@(experimental_service_typename)@(SERVICE_RESPONSE_MESSAGE_SUFFIX)>::value
  >
{
};

template<>
struct has_bounded_size<@(experimental_service_typename)>
  : std::integral_constant<
    bool,
    has_bounded_size<@(experimental_service_typename)@(SERVICE_REQUEST_MESSAGE_SUFFIX)>::value &&
    has_bounded_size<@(experimental_service_typename)@(SERVICE_RESPONSE_MESSAGE_SUFFIX)>::value
  >
{
};

template<>
struct is_service<@(experimental_service_typename)>
  : std::true_type
{
};

template<>
struct is_service_request<@(experimental_service_typename)@(SERVICE_REQUEST_MESSAGE_SUFFIX)>
  : std::true_type
{
};

template<>
struct is_service_response<@(experimental_service_typename)@(SERVICE_RESPONSE_MESSAGE_SUFFIX)>
  : std::true_type
{
};

}  // namespace rosidl_generator_traits
