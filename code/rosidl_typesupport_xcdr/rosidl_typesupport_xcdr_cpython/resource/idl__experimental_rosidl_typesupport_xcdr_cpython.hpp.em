@# generated from rosidl_typesupport_xcdr_cpython/resource/idl__experimental_rosidl_typesupport_xcdr_cpython.hpp.em
@{
from rosidl_parser.definition import Message, Service
}@
@#######################################################################
@# Handle messages
@#######################################################################
@{
for message in content.get_elements_of_type(Message):
    TEMPLATE(
        'msg__rosidl_typesupport_xcdr_cpython.hpp.em',
        package_name=package_name, interface_path=interface_path,
        message=message, force_experimental=True)
}@
@#######################################################################
@# Handle services
@#######################################################################
@{
for service in content.get_elements_of_type(Service):
    TEMPLATE(
        'srv__rosidl_typesupport_xcdr_cpython.hpp.em',
        package_name=package_name, interface_path=interface_path,
        service=service, force_experimental=True)
}@
