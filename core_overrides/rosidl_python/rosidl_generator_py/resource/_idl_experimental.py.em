@# Top-level EmPy template for generating experimental _<idl>_experimental.py files
@#
@# Context:
@#  - package_name (string)
@#  - interface_path (Path relative to the directory named after the package)
@#  - content (IdlContent, list of elements, e.g. Messages or Services)
@{
from rosidl_parser.definition import Message
from rosidl_parser.definition import Service
from rosidl_parser.definition import Action
}@
# generated from rosidl_generator_py/resource/_idl_experimental.py.em
# with input from @(package_name):@(interface_path)
# generated code does not contain a copyright notice

from __future__ import annotations

import dataclasses

from rosidl_runtime_cpython import (
    Array,
    BoundedSequence,
    BoundedString,
    BoundedWString,
    Dtype,
    MessageInitialization,
    RawBuffer,
    Scalar,
    Sequence,
    SequenceConstraint,
    String,
    StringConstraint,
    WString,
)
@
@#######################################################################
@# Handle messages
@#######################################################################
@[for message in content.get_elements_of_type(Message)]@
@{
TEMPLATE(
    '_msg_experimental.py.em',
    package_name=package_name, interface_path=interface_path, message=message)
}@
@[end for]@
@
@#######################################################################
@# Handle services
@#######################################################################
@[for service in content.get_elements_of_type(Service)]@
@{
TEMPLATE(
    '_srv_experimental.py.em',
    package_name=package_name, interface_path=interface_path, service=service)
}@
@[end for]@
@
@#######################################################################
@# Handle actions
@#######################################################################
@[for action in content.get_elements_of_type(Action)]@
@{
TEMPLATE(
    '_action_experimental.py.em',
    package_name=package_name, interface_path=interface_path, action=action)
}@
@[end for]@
