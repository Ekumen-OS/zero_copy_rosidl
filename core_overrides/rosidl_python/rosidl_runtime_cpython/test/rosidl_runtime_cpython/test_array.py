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

"""Tests for rosidl_runtime_py.experimental.array."""

import numpy as np
import pytest

from rosidl_runtime_cpython._raw_buffer import RawBuffer
from rosidl_runtime_cpython.dtype import Dtype
from rosidl_runtime_cpython.array import Array


# ---------------------------------------------------------------------------
# Primitive mode — construction
# ---------------------------------------------------------------------------

def test_primitive_default_zeros():
    a = Array(Dtype.INT32, 4)
    assert len(a) == 4
    assert list(a) == [0, 0, 0, 0]


def test_primitive_length_zero():
    a = Array(Dtype.FLOAT64, 0)
    assert len(a) == 0
    assert list(a) == []


def test_primitive_dtype_property():
    a = Array(Dtype.UINT8, 3)
    assert a.dtype is Dtype.UINT8


def test_primitive_from_buffer():
    # Pack two little-endian int32 values (10, 20) directly via RawBuffer(data).
    import struct
    buf = RawBuffer(struct.pack('<ii', 10, 20))
    a = Array(Dtype.INT32, 2, buffer=buf)
    assert a[0] == 10
    assert a[1] == 20


def test_primitive_buffer_too_small_raises():
    buf = RawBuffer(4)
    with pytest.raises(ValueError, match='buffer size'):
        Array(Dtype.INT32, 2, buffer=buf)  # needs 8 bytes


def test_primitive_bad_buffer_type_raises():
    with pytest.raises(TypeError, match='buffer must be a RawBuffer'):
        Array(Dtype.INT32, 2, buffer=b'\x00' * 8)


def test_negative_length_raises():
    with pytest.raises(ValueError, match='non-negative'):
        Array(Dtype.INT32, -1)


# ---------------------------------------------------------------------------
# Primitive mode — sequence protocol
# ---------------------------------------------------------------------------

def test_primitive_getitem_int():
    a = Array(Dtype.INT32, 3)
    a[0] = 7
    assert a[0] == 7


def test_primitive_getitem_negative_index():
    a = Array(Dtype.INT32, 3)
    a[2] = 99
    assert a[-1] == 99


def test_primitive_getitem_slice():
    a = Array(Dtype.INT32, 5)
    for i in range(5):
        a[i] = i * 10
    result = a[1:4]
    assert isinstance(result, np.ndarray)
    assert list(result) == [10, 20, 30]


def test_primitive_setitem_int():
    a = Array(Dtype.INT32, 3)
    a[1] = 42
    assert a[1] == 42


def test_primitive_setitem_slice():
    a = Array(Dtype.INT32, 4)
    a[1:3] = [100, 200]
    assert a[1] == 100
    assert a[2] == 200


def test_primitive_iter_yields_python_scalars():
    a = Array(Dtype.INT32, 3)
    for i in range(3):
        a[i] = i
    items = list(a)
    assert items == [0, 1, 2]
    assert all(isinstance(x, int) for x in items)


def test_primitive_contains():
    a = Array(Dtype.INT32, 4)
    a[2] = 77
    assert 77 in a
    assert 99 not in a


def test_primitive_reversed():
    a = Array(Dtype.INT32, 3)
    for i in range(3):
        a[i] = i + 1
    assert list(reversed(a)) == [3, 2, 1]


# ---------------------------------------------------------------------------
# Primitive mode — equality
# ---------------------------------------------------------------------------

def test_primitive_eq_same():
    a = Array(Dtype.INT32, 3)
    b = Array(Dtype.INT32, 3)
    for i in range(3):
        a[i] = b[i] = i
    assert a == b


def test_primitive_eq_different_value():
    a = Array(Dtype.INT32, 3)
    b = Array(Dtype.INT32, 3)
    a[0] = 1
    assert a != b


def test_primitive_eq_different_length():
    assert Array(Dtype.INT32, 3) != Array(Dtype.INT32, 4)


def test_primitive_eq_different_dtype():
    assert Array(Dtype.INT32, 2) != Array(Dtype.INT64, 2)


def test_primitive_eq_list():
    a = Array(Dtype.INT32, 3)
    for i in range(3):
        a[i] = i
    assert a == [0, 1, 2]


# ---------------------------------------------------------------------------
# Primitive mode — numpy / buffer
# ---------------------------------------------------------------------------

def test_primitive_numpy_is_live_view():
    a = Array(Dtype.INT32, 3)
    arr = a.numpy()
    a[0] = 55
    assert arr[0] == 55


def test_primitive_numpy_mutation_reflected_in_array():
    a = Array(Dtype.INT32, 3)
    a.numpy()[1] = 88
    assert a[1] == 88


def test_primitive_buffer_protocol():
    a = Array(Dtype.INT32, 4)
    mv = memoryview(a)
    assert len(mv) == 4 * Dtype.INT32.itemsize


def test_primitive_numpy_dtype_matches():
    a = Array(Dtype.FLOAT64, 2)
    assert a.numpy().dtype == Dtype.FLOAT64.numpy_dtype


# ---------------------------------------------------------------------------
# Object mode — construction
# ---------------------------------------------------------------------------

def test_object_default_none():
    a = Array(list, 3)
    assert len(a) == 3
    assert list(a) == [None, None, None]


def test_object_dtype_property():
    a = Array(str, 2)
    assert a.dtype is str


def test_object_buffer_raises():
    with pytest.raises(TypeError, match='buffer='):
        Array(list, 2, buffer=RawBuffer(16))


# ---------------------------------------------------------------------------
# Object mode — sequence protocol
# ---------------------------------------------------------------------------

def test_object_getitem_setitem():
    a = Array(object, 3)
    a[0] = 'hello'
    a[1] = 42
    a[2] = [1, 2, 3]
    assert a[0] == 'hello'
    assert a[1] == 42
    assert a[2] == [1, 2, 3]


def test_object_getitem_slice_returns_ndarray():
    a = Array(object, 4)
    for i in range(4):
        a[i] = i
    result = a[1:3]
    assert list(result) == [1, 2]


def test_object_iter():
    a = Array(object, 3)
    sentinel = object()
    a[0] = sentinel
    a[1] = 'x'
    a[2] = 99
    items = list(a)
    assert items[0] is sentinel
    assert items[1] == 'x'
    assert items[2] == 99


def test_object_contains():
    a = Array(object, 3)
    obj = object()
    a[1] = obj
    assert obj in a
    assert object() not in a


def test_object_reversed():
    a = Array(object, 3)
    for i in range(3):
        a[i] = i
    assert list(reversed(a)) == [2, 1, 0]


# ---------------------------------------------------------------------------
# Object mode — numpy / buffer
# ---------------------------------------------------------------------------

def test_object_numpy_returns_object_array():
    a = Array(object, 3)
    a[0] = 'hello'
    arr = a.numpy()
    assert isinstance(arr, np.ndarray)
    assert arr.dtype == object
    assert arr[0] == 'hello'


def test_object_numpy_is_live_view():
    a = Array(object, 2)
    arr = a.numpy()
    a[0] = 'changed'
    assert arr[0] == 'changed'


def test_object_buffer_protocol_raises():
    a = Array(object, 2)
    with pytest.raises(TypeError, match='__buffer__'):
        memoryview(a)


# ---------------------------------------------------------------------------
# Representation
# ---------------------------------------------------------------------------

def test_repr_primitive():
    a = Array(Dtype.INT32, 2)
    a[0] = 1
    a[1] = 2
    assert repr(a) == 'Array(Dtype.INT32, 2, data=[1, 2])'


def test_repr_object():
    a = Array(list, 2)
    r = repr(a)
    assert 'Array(' in r
    assert 'data=' in r
