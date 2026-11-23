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

import ctypes
import gc
import os
import subprocess
import time

import _region_fixture
import pytest
import rclpy
from rclpy.executors import SingleThreadedExecutor
from rclpy.impl.implementation_singleton import rclpy_implementation as _rclpy
from rclpy.task import Future
from rclpy.wait_for_message import wait_for_message
from rosidl_runtime_cpython.constraints import MessageConstraints
from rosidl_typesupport_xcdr_cpp_tests.msg import BoundedVariable


@pytest.fixture
def node():
    rclpy.init()
    node = rclpy.create_node('test_region_python_publisher')
    yield node
    node.destroy_node()
    rclpy.shutdown()


def wait_until(predicate):
    deadline = time.monotonic() + 4
    while not predicate() and time.monotonic() < deadline:
        time.sleep(0.01)
    assert predicate()


def test_old_bridge_capsule_is_rejected(node):
    create_capsule = ctypes.pythonapi.PyCapsule_New
    create_capsule.restype = ctypes.py_object
    create_capsule.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p]
    name = b'rosidl_runtime_cpython.MessageTypeBridge'
    capsule = create_capsule(1, name, None)

    class OldView:
        __region_type_bridge__ = capsule

    with pytest.raises(ValueError, match='incorrect name'):
        _rclpy.Publisher(node.handle, OldView, 'old_bridge', None)


def test_publish_to_ordinary_subscriber_and_invalidate_views(node):
    received = []
    node.create_subscription(BoundedVariable, 'python_region', received.append, 10)
    publisher = node.create_publisher(_region_fixture.BoundedVariable, 'python_region', 10)
    wait_until(lambda: publisher.get_subscription_count() > 0)
    loan = publisher.borrow_loaned_message()
    with loan as view:
        view.name = 'hello'
        view.tail = 42
        member = view.name_view
        publisher.publish_loaned_message(loan)
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = view.name
    with pytest.raises(RuntimeError, match='no longer valid'):
        str(member)

    deadline = time.monotonic() + 4
    while not received and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert len(received) == 1
    assert received[0].name == 'hello'
    assert received[0].tail == 42


def test_return_and_constraints(node):
    typed = _region_fixture.Constraints()
    typed.name_size = 8
    publisher = node.create_publisher(
        _region_fixture.BoundedVariable, 'python_region_limits', 10,
        constraints=MessageConstraints(type_specific=typed, strict=True))
    with publisher.borrow_loaned_message() as view:
        view.name = 'unused'
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = view.name
    tighter = _region_fixture.Constraints()
    tighter.name_size = 2
    loan = publisher.borrow_loaned_message(MessageConstraints(type_specific=tighter))
    loan.view.name = 'abc'
    with pytest.raises(Exception):
        publisher.publish_loaned_message(loan)
    assert not loan.valid
    with publisher.borrow_loaned_message() as view:
        view.name = 'ok'
    looser = _region_fixture.Constraints()
    looser.name_size = 9
    with pytest.raises(Exception):
        publisher.borrow_loaned_message(MessageConstraints(type_specific=looser))


def test_ordinary_to_region_subscription(node):
    received = []

    def callback(view):
        assert view.name == 'hello'
        assert view.tail == 17
        with pytest.raises(TypeError, match='read-only'):
            view.name = 'no'
        received.append((view, view.name_view))

    node.create_subscription(_region_fixture.BoundedVariable, 'ordinary_to_region', callback, 10)
    publisher = node.create_publisher(BoundedVariable, 'ordinary_to_region', 10)
    wait_until(lambda: publisher.get_subscription_count() > 0)
    publisher.publish(BoundedVariable(name='hello', tail=17))
    deadline = time.monotonic() + 4
    while not received and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert len(received) == 1
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = received[0][0].name
    with pytest.raises(RuntimeError, match='no longer valid'):
        str(received[0][1])


def test_region_to_region_subscription_and_callback_error(node):
    retained = []

    def callback(view):
        retained.append(view)
        raise ValueError('callback failure')

    node.create_subscription(_region_fixture.BoundedVariable, 'region_to_region', callback, 10)
    publisher = node.create_publisher(_region_fixture.BoundedVariable, 'region_to_region', 10)
    wait_until(lambda: publisher.get_subscription_count() > 0)
    loan = publisher.borrow_loaned_message()
    loan.view.name = 'hello'
    publisher.publish_loaned_message(loan)
    deadline = time.monotonic() + 4
    with pytest.raises(ValueError, match='callback failure'):
        while time.monotonic() < deadline:
            rclpy.spin_once(node, timeout_sec=0.05)
    assert len(retained) == 1
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = retained[0].name


def test_async_callback_keeps_loan_until_completion(node):
    received = []
    resumed = []
    future = Future(executor=rclpy.get_global_executor())

    async def callback(view):
        received.append(view)
        await future
        resumed.append(view.name)

    node.create_subscription(_region_fixture.BoundedVariable, 'async_region', callback, 10)
    publisher = node.create_publisher(BoundedVariable, 'async_region', 10)
    wait_until(lambda: publisher.get_subscription_count() > 0)
    publisher.publish(BoundedVariable(name='awaited'))
    deadline = time.monotonic() + 4
    while not received and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert len(received) == 1
    assert received[0].name == 'awaited'
    future.set_result(None)
    while not resumed and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert resumed == ['awaited']
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = received[0].name


def test_cancelled_callback_returns_loan_and_releases_group(node):
    received = []
    executor = SingleThreadedExecutor()
    executor.add_node(node)
    future = Future(executor=executor)

    async def callback(view):
        received.append(view)
        await future

    subscription = node.create_subscription(
        _region_fixture.BoundedVariable, 'cancelled_region', callback, 10)
    publisher = node.create_publisher(BoundedVariable, 'cancelled_region', 10)
    try:
        wait_until(lambda: publisher.get_subscription_count() > 0)
        publisher.publish(BoundedVariable(name='first'))
        deadline = time.monotonic() + 4
        while not received and time.monotonic() < deadline:
            executor.spin_once(timeout_sec=0.05)
        assert len(received) == 1
        with executor._tasks_lock:
            tasks = [
                task for task, data in executor._pending_tasks.items()
                if data.source_entity is subscription]
        assert len(tasks) == 1
        tasks[0].cancel()
        with pytest.raises(RuntimeError, match='no longer valid'):
            _ = received[0].name
        future.set_result(None)
        publisher.publish(BoundedVariable(name='second'))
        while len(received) < 2 and time.monotonic() < deadline:
            executor.spin_once(timeout_sec=0.05)
        assert len(received) == 2
        with pytest.raises(RuntimeError, match='no longer valid'):
            _ = received[1].name
    finally:
        executor.shutdown()


def test_reject_unsupported_region_paths_and_wrong_publisher(node):
    with pytest.raises(TypeError, match='raw'):
        node.create_subscription(
            _region_fixture.BoundedVariable, 'raw', lambda _: None, 10, raw=True)
    with pytest.raises(TypeError, match='does not support'):
        wait_for_message(_region_fixture.BoundedVariable, node, 'wait')
    first = node.create_publisher(_region_fixture.BoundedVariable, 'first', 10)
    second = node.create_publisher(_region_fixture.BoundedVariable, 'second', 10)
    loan = first.borrow_loaned_message()
    with pytest.raises(ValueError, match='does not belong'):
        second.publish_loaned_message(loan)
    assert loan.valid
    loan.return_loan()
    with pytest.raises(RuntimeError, match='no longer valid'):
        loan.return_loan()


def test_loan_destruction_and_invalid_constraints(node):
    publisher = node.create_publisher(_region_fixture.BoundedVariable, 'destruction', 10)
    loan = publisher.borrow_loaned_message()
    view = loan.view
    member = view.name_view
    del loan
    gc.collect()
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = view.name
    with pytest.raises(RuntimeError, match='no longer valid'):
        str(member)
    with pytest.raises(TypeError):
        publisher.borrow_loaned_message(object())
    with pytest.raises(TypeError):
        publisher.borrow_loaned_message(MessageConstraints(type_specific=object()))
    with pytest.raises(Exception):
        publisher.borrow_loaned_message(MessageConstraints(max_string_length=-1))


def test_loan_outlives_publisher_handle(node):
    publisher = node.create_publisher(_region_fixture.BoundedVariable, 'destroyed_publisher', 10)
    loan = publisher.borrow_loaned_message()
    view = loan.view
    node.destroy_publisher(publisher)
    view.name = 'still'
    loan.return_loan()
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = view.name


def test_loan_return_after_context_shutdown():
    context = rclpy.context.Context()
    rclpy.init(context=context)
    node = rclpy.create_node('test_region_shutdown', context=context)
    try:
        publisher = node.create_publisher(_region_fixture.BoundedVariable, 'shutdown', 10)
        loan = publisher.borrow_loaned_message()
        view = loan.view
        context.shutdown()
        loan.return_loan()
        with pytest.raises(RuntimeError, match='no longer valid'):
            _ = view.name
    finally:
        node.destroy_node()
        if context.ok():
            context.shutdown()


def test_explicitly_disabled_loans_copy_to_and_from_ordinary_peers(node, monkeypatch):
    monkeypatch.setenv('ROS_DISABLE_LOANED_MESSAGES', '1')
    received_plain = []
    received_view = []
    node.create_subscription(BoundedVariable, 'copy_to_plain', received_plain.append, 10)
    publisher = node.create_publisher(_region_fixture.BoundedVariable, 'copy_to_plain', 10)
    assert not publisher.can_loan_messages
    wait_until(lambda: publisher.get_subscription_count() > 0)
    loan = publisher.borrow_loaned_message()
    view = loan.view
    view.name = 'hello'
    publisher.publish_loaned_message(loan)
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = view.name

    sub = node.create_subscription(
        _region_fixture.BoundedVariable, 'plain_to_copy', received_view.append, 10)
    assert not sub.can_loan_messages
    plain_pub = node.create_publisher(BoundedVariable, 'plain_to_copy', 10)
    wait_until(lambda: plain_pub.get_subscription_count() > 0)
    plain_pub.publish(BoundedVariable(name='peer'))
    deadline = time.monotonic() + 4
    while (not received_plain or not received_view) and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert received_plain[0].name == 'hello'
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = received_view[0].name
    monkeypatch.setenv('ROS_DISABLE_LOANED_MESSAGES', '2')
    subscription = node.create_subscription(
        _region_fixture.BoundedVariable, 'disabled_subscription', lambda _: None, 10)
    assert not subscription.can_loan_messages


def test_copy_constraints_and_return(node, monkeypatch):
    monkeypatch.setenv('ROS_DISABLE_LOANED_MESSAGES', '1')
    baseline = _region_fixture.Constraints()
    baseline.name_size = 4
    publisher = node.create_publisher(
        _region_fixture.BoundedVariable, 'copy_constraints', 10,
        constraints=MessageConstraints(type_specific=baseline, strict=True))
    baseline.name_size = 8
    tighter = _region_fixture.Constraints()
    tighter.name_size = 2
    loan = publisher.borrow_loaned_message(MessageConstraints(type_specific=tighter))
    view = loan.view
    view.name = 'bad'
    with pytest.raises(ValueError, match='constraints'):
        publisher.publish_loaned_message(loan)
    assert not loan.valid
    with pytest.raises(RuntimeError, match='no longer valid'):
        _ = view.name
    partial = _region_fixture.Constraints()
    loan = publisher.borrow_loaned_message(MessageConstraints(type_specific=partial))
    loan.view.name = 'too long'
    with pytest.raises(ValueError, match='constraints'):
        publisher.publish_loaned_message(loan)
    looser = _region_fixture.Constraints()
    looser.name_size = 5
    with pytest.raises(RuntimeError, match='looser'):
        publisher.borrow_loaned_message(MessageConstraints(type_specific=looser))
    with pytest.raises(RuntimeError, match='blanket'):
        node.create_publisher(
            _region_fixture.BoundedVariable, 'invalid_copy', 10,
            constraints=MessageConstraints(max_string_length=3))


@pytest.mark.skipif(
    not os.environ.get('REGION_TEST_NO_ENDPOINT_LOANS'),
    reason='Requires a middleware endpoint reporting no loan support')
def test_nonloaning_endpoint_uses_copy(node):
    received = []
    subscription = node.create_subscription(
        _region_fixture.BoundedVariable, 'nonloaning_endpoint', received.append, 10)
    publisher = node.create_publisher(
        _region_fixture.BoundedVariable, 'nonloaning_endpoint', 10)
    assert not publisher.can_loan_messages
    assert not subscription.can_loan_messages
    wait_until(lambda: publisher.get_subscription_count() > 0)
    loan = publisher.borrow_loaned_message()
    loan.view.name = 'fallback'
    publisher.publish_loaned_message(loan)
    deadline = time.monotonic() + 4
    while not received and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert len(received) == 1


def test_native_endpoints_report_loan_usage(node, monkeypatch):
    if rclpy.get_rmw_implementation_identifier() != 'rmw_fastrtps_cpp':
        pytest.skip('Fast DDS region loans are required')
    monkeypatch.setenv('ROS_DISABLE_LOANED_MESSAGES', '0')
    publisher = node.create_publisher(_region_fixture.BoundedVariable, 'loan_capability', 10)
    subscription = node.create_subscription(
        _region_fixture.BoundedVariable, 'loan_capability', lambda _: None, 10)
    assert publisher.can_loan_messages
    assert subscription.can_loan_messages


@pytest.mark.parametrize('copy', [False, True])
def test_subscription_baseline(node, monkeypatch, copy):
    if copy:
        monkeypatch.setenv('ROS_DISABLE_LOANED_MESSAGES', '1')
    limits = _region_fixture.Constraints()
    limits.name_size = 2
    received = []
    node.create_subscription(
        _region_fixture.BoundedVariable, 'sub_limits',
        lambda view: received.append(view.name), 10,
        constraints=MessageConstraints(type_specific=limits, strict=True))
    publisher = node.create_publisher(BoundedVariable, 'sub_limits', 10)
    wait_until(lambda: publisher.get_subscription_count() > 0)
    publisher.publish(BoundedVariable(name='long'))
    publisher.publish(BoundedVariable(name='ok'))
    deadline = time.monotonic() + 4
    expected_error = ValueError if copy else _rclpy.RCLError
    expected_message = 'exceeds constraints' if copy else 'failed to cast'
    with pytest.raises(expected_error, match=expected_message):
        while time.monotonic() < deadline:
            rclpy.spin_once(node, timeout_sec=0.05)
    while not received and time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
    assert received == ['ok']


def test_cpp_to_python(node):
    received = []
    node.create_subscription(
        _region_fixture.BoundedVariable, 'cpp_to_python',
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


def test_python_to_cpp(node):
    publisher = node.create_publisher(_region_fixture.BoundedVariable, 'python_to_cpp', 10)
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
