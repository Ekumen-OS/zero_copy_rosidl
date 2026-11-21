@# generated from rosidl_typesupport_xcdr_cpp/resource/idl__experimental_rosidl_typesupport_xcdr_cpp.hpp.em
@# with input from @(package_name):@(interface_path)
@# generated code does not contain a copyright notice
@{
from rosidl_parser.definition import Message, Service
from rosidl_pycommon import convert_camel_case_to_lower_case_underscore
parts = [package_name] + list(interface_path.parents[0].parts) + [
    'views' if region_views else 'experimental', 'detail',
    convert_camel_case_to_lower_case_underscore(interface_path.stem),
    'rosidl_typesupport_xcdr_cpp']
empty_guard = '__'.join(part.upper() for part in parts) + '_HPP_'
}@
@[if not content.get_elements_of_type(Message) and (region_views or not content.get_elements_of_type(Service))]@
// generated code does not contain a copyright notice
#ifndef @(empty_guard)
#define @(empty_guard)

#endif  // @(empty_guard)
@[end if]@
@#######################################################################
@# Handle messages
@#######################################################################
@{
for message in content.get_elements_of_type(Message):
    TEMPLATE(
        'msg__rosidl_typesupport_xcdr_cpp.hpp.em',
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
            'srv__rosidl_typesupport_xcdr_cpp.hpp.em',
            package_name=package_name, interface_path=interface_path,
            service=service, force_experimental=True)
}@
