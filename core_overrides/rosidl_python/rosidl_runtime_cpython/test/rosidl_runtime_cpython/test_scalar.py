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

"""Tests for rosidl_runtime_py.experimental.scalar."""

import numpy as np
import pytest

from rosidl_runtime_cpython._raw_buffer import RawBuffer
from rosidl_runtime_cpython.dtype import Dtype
from rosidl_runtime_cpython.scalar import Scalar


# ---------------------------------------------------------------------------
# RawBuffer — indexing
# ---------------------------------------------------------------------------

def test_rawbuffer_len():
    buf = RawBuffer(4)
    assert len(buf) == 4


def test_rawbuffer_getitem():
    buf = RawBuffer(3)
    buf[0] = 0xAA
    buf[1] = 0xBB
    buf[2] = 0xCC
    assert buf[0] == 0xAA
    assert buf[1] == 0xBB
    assert buf[2] == 0xCC


def test_rawbuffer_negative_index():
    buf = RawBuffer(3)
    buf[2] = 0xFF
    assert buf[-1] == 0xFF


def test_rawbuffer_index_out_of_range_raises():
    buf = RawBuffer(2)
    with pytest.raises(IndexError):
        _ = buf[5]


def test_rawbuffer_setitem_out_of_range_raises():
    buf = RawBuffer(2)
    with pytest.raises(IndexError):
        buf[5] = 0


def test_rawbuffer_setitem_bad_value_raises():
    buf = RawBuffer(1)
    with pytest.raises(ValueError):
        buf[0] = 256
    with pytest.raises(ValueError):
        buf[0] = -1


def test_rawbuffer_setitem_bad_type_raises():
    buf = RawBuffer(1)
    with pytest.raises(TypeError):
        buf[0] = 'x'


# ---------------------------------------------------------------------------
# Construction
# ---------------------------------------------------------------------------

def test_default_value_is_zero():
    s = Scalar(Dtype.INT32)
    assert s.value == 0


@pytest.mark.parametrize('dtype, value', [
    (Dtype.INT32, 42),
    (Dtype.FLOAT64, 3.14),
    (Dtype.BOOL, True),
    (Dtype.UINT8, 255),
    (Dtype.INT64, -(2**62)),
])
def test_initial_value(dtype, value):
    s = Scalar(dtype, value)
    if dtype in (Dtype.FLOAT32, Dtype.FLOAT64, Dtype.LONG_DOUBLE):
        assert abs(s.value - value) < 1e-5
    else:
        assert s.value == value


def test_dtype_property():
    s = Scalar(Dtype.UINT16, 7)
    assert s.dtype is Dtype.UINT16


def test_construction_from_buffer():
    buf = RawBuffer(4)
    s = Scalar(Dtype.INT32, buffer=buf)
    assert s.dtype is Dtype.INT32
    # buffer is re-used, not copied
    assert s._buffer is buf


def test_buffer_ignores_value_argument():
    buf = RawBuffer(4)
    # Write little-endian 99 (0x63000000) directly via indexing.
    buf[0] = 99
    buf[1] = 0
    buf[2] = 0
    buf[3] = 0
    s = Scalar(Dtype.INT32, 0, buffer=buf)
    # The value in the buffer (99) is preserved; the value= arg is ignored.
    assert s.value == 99


# ---------------------------------------------------------------------------
# Value access
# ---------------------------------------------------------------------------

def test_value_setter():
    s = Scalar(Dtype.INT32, 0)
    s.value = 123
    assert s.value == 123


def test_value_setter_clamps_for_integer_types():
    # numpy silently wraps overflow for integer types.
    s = Scalar(Dtype.UINT8, 0)
    s.value = 256   # wraps to 0
    assert s.value == 0


# ---------------------------------------------------------------------------
# Numeric protocol
# ---------------------------------------------------------------------------

def test_int_conversion():
    s = Scalar(Dtype.INT32, 7)
    assert int(s) == 7
    assert isinstance(int(s), int)


def test_float_conversion():
    s = Scalar(Dtype.FLOAT64, 2.5)
    assert float(s) == 2.5
    assert isinstance(float(s), float)


def test_bool_conversion_false():
    s = Scalar(Dtype.BOOL, False)
    assert not bool(s)


def test_bool_conversion_true():
    s = Scalar(Dtype.INT32, 1)
    assert bool(s)


def test_index_for_integer_dtype():
    s = Scalar(Dtype.INT32, 3)
    lst = [0, 1, 2, 3, 4]
    assert lst[s] == 3


def test_index_raises_for_float_dtype():
    s = Scalar(Dtype.FLOAT64, 1.0)
    with pytest.raises(TypeError):
        _ = [0][s]


# ---------------------------------------------------------------------------
# Equality and hashing
# ---------------------------------------------------------------------------

def test_eq_same_dtype_same_value():
    assert Scalar(Dtype.INT32, 5) == Scalar(Dtype.INT32, 5)


def test_eq_same_dtype_different_value():
    assert Scalar(Dtype.INT32, 5) != Scalar(Dtype.INT32, 6)


def test_eq_different_dtype():
    # UINT8 and INT8 with value 1 should compare unequal (dtype differs).
    assert Scalar(Dtype.UINT8, 1) != Scalar(Dtype.INT8, 1)


def test_eq_with_python_scalar():
    s = Scalar(Dtype.INT32, 42)
    assert s == 42
    assert 42 == s


def test_hash_equal_scalars():
    a = Scalar(Dtype.INT32, 10)
    b = Scalar(Dtype.INT32, 10)
    assert hash(a) == hash(b)


def test_hash_in_set():
    s = {Scalar(Dtype.INT32, 1), Scalar(Dtype.INT32, 1), Scalar(Dtype.INT32, 2)}
    assert len(s) == 2


# ---------------------------------------------------------------------------
# numpy / buffer protocols
# ---------------------------------------------------------------------------

def test_numpy_returns_length_1_array():
    s = Scalar(Dtype.INT32, 7)
    arr = s.numpy()
    assert isinstance(arr, np.ndarray)
    assert arr.shape == (1,)
    assert arr[0] == 7


def test_numpy_is_live_view():
    s = Scalar(Dtype.INT32, 0)
    arr = s.numpy()
    s.value = 99
    assert arr[0] == 99


def test_numpy_mutation_reflected_in_scalar():
    s = Scalar(Dtype.INT32, 0)
    s.numpy()[0] = 55
    assert s.value == 55


def test_buffer_protocol():
    s = Scalar(Dtype.INT32, 0)
    mv = memoryview(s)
    assert len(mv) == Dtype.INT32.itemsize


# ---------------------------------------------------------------------------
# Error cases
# ---------------------------------------------------------------------------

def test_non_dtype_raises():
    with pytest.raises(TypeError, match='dtype must be a Dtype'):
        Scalar('int32')


def test_bad_buffer_type_raises():
    with pytest.raises(TypeError, match='buffer must be a RawBuffer'):
        Scalar(Dtype.INT32, buffer=b'\x00\x00\x00\x00')


def test_undersized_buffer_raises():
    with pytest.raises(ValueError, match='buffer size'):
        Scalar(Dtype.INT32, buffer=RawBuffer(2))


# ---------------------------------------------------------------------------
# Representation
# ---------------------------------------------------------------------------

def test_repr():
    s = Scalar(Dtype.INT32, 7)
    assert repr(s) == 'Scalar(Dtype.INT32, 7)'


def test_str():
    s = Scalar(Dtype.FLOAT64, 1.5)
    assert str(s) == '1.5'
