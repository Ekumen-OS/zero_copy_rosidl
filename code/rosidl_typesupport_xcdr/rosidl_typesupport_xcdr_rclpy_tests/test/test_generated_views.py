# Copyright 2026 Ekumen, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import gc
import os
import subprocess
import time

import pytest
import rclpy
from rosidl_runtime_cpython.constraints import MessageConstraints
from rosidl_typesupport_xcdr_cpp_tests.msg import BoundedVariable
from rosidl_typesupport_xcdr_cpp_tests.msg.views import BasicTypes
from rosidl_typesupport_xcdr_cpp_tests.msg.views import BoundedVariable as RegionBoundedVariable
from rosidl_typesupport_xcdr_cpp_tests.msg.views import (
    NestedBounded, NestedContainers, VariableContainers)
from rosidl_typesupport_xcdr_rclpy_tests.msg.views import CrossPackage


@pytest.fixture
def node():
    rclpy.init()
    node = rclpy.create_node('generated_region_tests')
    yield node
    node.destroy_node()
    rclpy.shutdown()


def wait_until(predicate):
    deadline = time.monotonic() + 4
    while not predicate() and time.monotonic() < deadline:
        time.sleep(0.01)
    assert predicate()


def test_owned_views_and_nested_sequence_lifetime():
    message = RegionBoundedVariable()
    message.name = 'hello'
    message.wide = 'a\0\U0001f600'
    message.values.append(5)
    message.wide_array[0] = 'first'
    assert message.name == 'hello'
    assert message.wide == 'a\0\U0001f600'
    assert list(message.values) == [5]
    assert message.wide_array[0] == 'first'
    message.values = [8, 9]
    message.values = message.values
    assert list(message.values) == [8, 9]
    message.wide_array = ['one', 'two']
    with pytest.raises(ValueError, match='same length'):
        message.wide_array = ['one']
    assert list(message.wide_array) == ['one', 'two']
    with pytest.raises(TypeError, match='fixed array'):
        message.wide_array.append('extra')

    nested = NestedContainers()
    nested.single = {'id': 42}
    nested.char_buf[0] = 4
    assert nested.single.id == 42
    assert nested.char_buf[0] == 4

    containers = VariableContainers()
    containers.items.append({'id': 12, 'value': 1.5})
    item = containers.items[0]
    containers.items.append({'id': 23})
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = item.id
    assert containers.items[0].id == 12
    assert containers.items[1].id == 23
    containers.items = containers.items
    assert [item.id for item in containers.items] == [12, 23]
    other = VariableContainers()
    other.items.append({'id': 99})
    with pytest.raises(TypeError, match='mapping'):
        containers.items = other.items
    assert [item.id for item in containers.items] == [12, 23]
    with pytest.raises(ValueError, match='upper bound'):
        containers.items.append({'id': 24})
    empty = VariableContainers()
    with pytest.raises((TypeError, AttributeError)):
        empty.items.append({'not_a_field': 1})
    assert len(empty.items) == 0
    containers.nested_variable = [{'id': 4, 'name': 'nested', 'data': [7, 8]}]
    assert list(containers.nested_variable[0].data) == [7, 8]
    child = nested.single
    del nested
    gc.collect()
    assert child.id == 42


def test_generated_scalar_and_string_containers():
    basic = BasicTypes()
    basic.bool_value = True
    basic.int64_value = -(2 ** 60)
    basic.uint64_value = 2 ** 63
    basic.float64_value = 1.25
    assert (basic.bool_value, basic.int64_value, basic.uint64_value, basic.float64_value) == (
        True, -(2 ** 60), 2 ** 63, 1.25)
    message = VariableContainers()
    message.labels = ['a', 'b']
    message.names = ['a\0b', 'c']
    message.wide_names = ['\u03bb', 'a\0\U0001f600']
    assert list(message.labels) == ['a', 'b']
    assert list(message.names) == ['a\0b', 'c']
    assert list(message.wide_names) == ['\u03bb', 'a\0\U0001f600']


def test_generated_cross_package_nested_views():
    message = CrossPackage()
    message.child = {'id': 42}
    message.children = [{'id': 12}, {'id': 23}]
    assert message.child.id == 42
    assert [child.id for child in message.children] == [12, 23]
    limits = CrossPackage.Constraints()
    limits.children_length = 2
    assert limits.children_length == 2


@pytest.mark.parametrize('copy', [False, True])
def test_generated_cross_package_transport(node, monkeypatch, copy):
    if copy:
        monkeypatch.setenv('ROS_DISABLE_LOANED_MESSAGES', '1')
    received = []
    constraints = MessageConstraints(type_specific=CrossPackage.Constraints())
    node.create_subscription(
        CrossPackage, 'cross_package',
        lambda view: received.append((view.child.id, view.children[0].id)), 10,
        constraints=constraints)
    publisher = node.create_publisher(CrossPackage, 'cross_package', 10, constraints=constraints)
    wait_until(lambda: publisher.get_subscription_count() > 0)
    loan = publisher.borrow_loaned_message()
    loan.view.child = {'id': 42}
    loan.view.children = [{'id': 12}]
    publisher.publish_loaned_message(loan)
    deadline = time.monotonic() + 4
    while not received and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert received == [(42, 12)]


@pytest.mark.parametrize('copy', [False, True])
def test_generated_views_publish_and_subscribe(node, monkeypatch, copy):
    if copy:
        monkeypatch.setenv('ROS_DISABLE_LOANED_MESSAGES', '1')
    received = []
    retained = []

    def callback(view):
        assert view.name == 'python'
        assert view.wide == 'a\0\U0001f600'
        assert list(view.values) == [1, 2]
        with pytest.raises(TypeError, match='read-only'):
            view.values.append(3)
        retained.append(view.values)
        received.append(view.tail)

    limits = RegionBoundedVariable.Constraints()
    limits.name_size = 8
    constraints = MessageConstraints(type_specific=limits, strict=True)
    node.create_subscription(
        RegionBoundedVariable, 'generated_views', callback, 10, constraints=constraints)
    publisher = node.create_publisher(RegionBoundedVariable, 'generated_views', 10)
    wait_until(lambda: publisher.get_subscription_count() > 0)
    loan = publisher.borrow_loaned_message()
    loan.view.name = 'python'
    loan.view.wide = 'a\0\U0001f600'
    loan.view.values.append(1)
    loan.view.values.append(2)
    loan.view.tail = 31
    publisher.publish_loaned_message(loan)
    deadline = time.monotonic() + 4
    while not received and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert received == [31]
    with pytest.raises(RuntimeError, match='no longer valid'):
        len(retained[0])


def test_generated_nested_constraints_and_invalid_types(node):
    limits = NestedBounded.Constraints()
    limits.nested.name_size = 3
    limits.items_length = 2
    limits.items_element.name_size = 4
    assert limits.nested.name_size == 3
    assert limits.items_length == 2
    assert limits.items_element.name_size == 4
    with pytest.raises(TypeError, match='constraints'):
        node.create_publisher(
            RegionBoundedVariable, 'wrong_constraints', 10,
            constraints=MessageConstraints(type_specific=limits))


def test_generated_publisher_constraints(node):
    baseline = RegionBoundedVariable.Constraints()
    baseline.name_size = 8
    publisher = node.create_publisher(
        RegionBoundedVariable, 'generated_limits', 10,
        constraints=MessageConstraints(type_specific=baseline, strict=True))
    tighter = RegionBoundedVariable.Constraints()
    tighter.name_size = 2
    loan = publisher.borrow_loaned_message(MessageConstraints(type_specific=tighter))
    loan.view.name = 'abc'
    with pytest.raises(Exception):
        publisher.publish_loaned_message(loan)
    assert not loan.valid
    looser = RegionBoundedVariable.Constraints()
    looser.name_size = 9
    with pytest.raises(Exception):
        publisher.borrow_loaned_message(MessageConstraints(type_specific=looser))


@pytest.mark.parametrize('copy', [False, True])
def test_generated_publisher_to_ordinary_subscriber(node, monkeypatch, copy):
    if copy:
        monkeypatch.setenv('ROS_DISABLE_LOANED_MESSAGES', '1')
    received = []
    node.create_subscription(BoundedVariable, 'generated_to_plain', received.append, 10)
    publisher = node.create_publisher(RegionBoundedVariable, 'generated_to_plain', 10)
    wait_until(lambda: publisher.get_subscription_count() > 0)
    loan = publisher.borrow_loaned_message()
    loan.view.name = 'hello'
    loan.view.wide = 'wide'
    loan.view.tail = 7
    publisher.publish_loaned_message(loan)
    deadline = time.monotonic() + 4
    while not received and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert len(received) == 1
    assert received[0].name == 'hello'
    assert received[0].wide == 'wide'
    assert received[0].tail == 7


def test_ordinary_publisher_to_generated_subscriber(node):
    received = []
    node.create_subscription(
        RegionBoundedVariable, 'plain_to_generated',
        lambda view: received.append((view.name, view.tail)), 10)
    publisher = node.create_publisher(BoundedVariable, 'plain_to_generated', 10)
    wait_until(lambda: publisher.get_subscription_count() > 0)
    publisher.publish(BoundedVariable(name='plain', tail=13))
    deadline = time.monotonic() + 4
    while not received and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert received == [('plain', 13)]


def test_cpp_to_generated_python(node):
    received = []
    node.create_subscription(
        RegionBoundedVariable, 'cpp_to_python',
        lambda view: received.append((view.name, view.tail)), 10)
    peer = subprocess.Popen([os.environ['REGION_CPP_PEER'], 'publisher'])
    try:
        deadline = time.monotonic() + 8
        while not received and time.monotonic() < deadline:
            rclpy.spin_once(node, timeout_sec=0.05)
        assert received == [('cpp', 25)]
        assert peer.wait(timeout=5) == 0
    finally:
        if peer.poll() is None:
            peer.kill()
            peer.wait()


def test_generated_python_to_cpp(node):
    publisher = node.create_publisher(RegionBoundedVariable, 'python_to_cpp', 10)
    peer = subprocess.Popen([os.environ['REGION_CPP_PEER'], 'subscriber'])
    try:
        wait_until(lambda: publisher.get_subscription_count() > 0)
        loan = publisher.borrow_loaned_message()
        loan.view.name = 'python'
        loan.view.tail = 31
        publisher.publish_loaned_message(loan)
        assert peer.wait(timeout=8) == 0
    finally:
        if peer.poll() is None:
            peer.kill()
            peer.wait()
