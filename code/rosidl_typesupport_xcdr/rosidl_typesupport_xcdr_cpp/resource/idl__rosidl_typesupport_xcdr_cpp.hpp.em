@# generated from rosidl_typesupport_xcdr_cpp/resource/idl__rosidl_typesupport_xcdr_cpp.hpp.em
@# with input from @(package_name):@(interface_path)
@# generated code does not contain a copyright notice

@{
from rosidl_pycommon import convert_camel_case_to_lower_case_underscore
from rosidl_parser.definition import Message, Service

include_parts = [package_name] + list(interface_path.parents[0].parts) + [
    'detail', convert_camel_case_to_lower_case_underscore(interface_path.stem)]
include_base = '/'.join(include_parts)
}@

@#######################################################################
@# Handle messages
@#######################################################################
@{
for message in content.get_elements_of_type(Message):
    TEMPLATE(
        'msg__rosidl_typesupport_xcdr_cpp.hpp.em',
        package_name=package_name, interface_path=interface_path, message=message)
}@

@#######################################################################
@# Handle services
@#######################################################################
@{
for service in content.get_elements_of_type(Service):
    TEMPLATE(
        'srv__rosidl_typesupport_xcdr_cpp.hpp.em',
        package_name=package_name, interface_path=interface_path, service=service)
}@
