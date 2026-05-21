@# Included from rosidl_generator_py/resource/_idl_experimental.py.em
@{
TEMPLATE(
    '_msg_experimental.py.em',
    package_name=package_name, interface_path=interface_path,
    message=service.request_message)
}@
@{
TEMPLATE(
    '_msg_experimental.py.em',
    package_name=package_name, interface_path=interface_path,
    message=service.response_message)
}@
@{
TEMPLATE(
    '_msg_experimental.py.em',
    package_name=package_name, interface_path=interface_path,
    message=service.event_message)
}@


class @(service.namespaced_type.name):
    """Experimental service class '@(service.namespaced_type.name)'."""

    Request = @(service.request_message.structure.namespaced_type.name)
    Response = @(service.response_message.structure.namespaced_type.name)
    Event = @(service.event_message.structure.namespaced_type.name)
