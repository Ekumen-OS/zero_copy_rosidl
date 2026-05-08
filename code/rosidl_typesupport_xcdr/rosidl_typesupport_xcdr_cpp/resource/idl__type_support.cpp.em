@# generated from rosidl_typesupport_xcdr_cpp/resource/idl__type_support.cpp.em
@# with input from @(package_name):@(interface_path)
@# generated code does not contain a copyright notice

@#######################################################################
@# Handle messages
@#######################################################################
@{
from rosidl_parser.definition import Message

for message in content.get_elements_of_type(Message):
    TEMPLATE(
        'msg__type_support.cpp.em',
        package_name=package_name, interface_path=interface_path, message=message)
}@

@#######################################################################
@# Handle services
@#######################################################################
@{
from rosidl_parser.definition import Service

for service in content.get_elements_of_type(Service):
    TEMPLATE(
        'srv__type_support.cpp.em',
        package_name=package_name, interface_path=interface_path, service=service)
}@
