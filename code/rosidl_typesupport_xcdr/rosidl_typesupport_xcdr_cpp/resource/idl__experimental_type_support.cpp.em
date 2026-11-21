@# generated from rosidl_typesupport_xcdr_cpp/resource/idl__experimental_type_support.cpp.em
@# with input from @(package_name):@(interface_path)
@# generated code does not contain a copyright notice
@{
from rosidl_parser.definition import Message, Service
}@
@[if not content.get_elements_of_type(Message) and (region_views or not content.get_elements_of_type(Service))]@
// generated code does not contain a copyright notice
@[end if]@
@#######################################################################
@# Handle messages
@#######################################################################
@{
for message in content.get_elements_of_type(Message):
    TEMPLATE(
        'msg__type_support.cpp.em',
        package_name=package_name, interface_path=interface_path,
        message=message, force_experimental=True, region_views=region_views)
}@
@
@#######################################################################
@# Handle services
@#######################################################################
@{
if not region_views:
    for service in content.get_elements_of_type(Service):
        TEMPLATE(
            'srv__type_support.cpp.em',
            package_name=package_name, interface_path=interface_path,
            service=service, force_experimental=True)
}@
