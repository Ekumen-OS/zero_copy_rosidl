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

"""Tests for rosidl_runtime_py.experimental.dtype."""

import struct

import numpy as np
import pytest

from rosidl_runtime_cpython.dtype import Dtype


# All 15 ROS primitive Dtype members.
ALL_DTYPES = list(Dtype)


def test_all_members_present():
    names = {d.name for d in Dtype}
    expected = {
        'UINT8', 'UINT16', 'UINT32', 'UINT64',
        'INT8', 'INT16', 'INT32', 'INT64',
        'FLOAT32', 'FLOAT64', 'LONG_DOUBLE',
        'CHAR', 'WCHAR',
        'BOOL', 'BYTE',
    }
    assert names == expected, names


@pytest.mark.parametrize('dtype', ALL_DTYPES)
def test_numpy_dtype_is_np_dtype(dtype):
    assert isinstance(dtype.numpy_dtype, np.dtype)


@pytest.mark.parametrize('dtype', ALL_DTYPES)
def test_itemsize_matches_numpy(dtype):
    assert dtype.itemsize == dtype.numpy_dtype.itemsize
    assert dtype.itemsize >= 1


@pytest.mark.parametrize('dtype', [d for d in ALL_DTYPES if d is not Dtype.LONG_DOUBLE])
def test_format_char_struct_size_matches_itemsize(dtype):
    # struct.calcsize gives the byte size of the format character.
    assert struct.calcsize(dtype.format_char) == dtype.itemsize


@pytest.mark.parametrize('dtype, value', [
    (Dtype.FLOAT32, 1.5),
    (Dtype.FLOAT64, -3.14),
    (Dtype.LONG_DOUBLE, 1.23456789012345),
    (Dtype.CHAR, 65),        # 'A'
    (Dtype.WCHAR, 0x4E16),   # '世'
    (Dtype.BOOL, 1),
    (Dtype.BYTE, 255),
    (Dtype.UINT8, 200),
    (Dtype.UINT16, 60000),
    (Dtype.UINT32, 2**31),
    (Dtype.UINT64, 2**63),
    (Dtype.INT8, -1),
    (Dtype.INT16, -32768),
    (Dtype.INT32, -1_000_000),
    (Dtype.INT64, -(2**62)),
])
def test_pack_unpack_round_trip(dtype, value):
    data = dtype.pack(value)
    assert len(data) == dtype.itemsize
    result = dtype.unpack(data)
    # Use approximate comparison for floats.
    if dtype in (Dtype.FLOAT32, Dtype.FLOAT64, Dtype.LONG_DOUBLE):
        assert abs(result - value) < abs(value) * 1e-5 + 1e-10
    else:
        assert result == value


def test_repr():
    assert repr(Dtype.INT32) == 'Dtype.INT32'
    assert repr(Dtype.WCHAR) == 'Dtype.WCHAR'


def test_specific_itemsizes():
    assert Dtype.FLOAT32.itemsize == 4
    assert Dtype.FLOAT64.itemsize == 8
    assert Dtype.CHAR.itemsize == 1
    assert Dtype.WCHAR.itemsize == 2
    assert Dtype.BOOL.itemsize == 1
    assert Dtype.BYTE.itemsize == 1
    assert Dtype.UINT8.itemsize == 1
    assert Dtype.UINT16.itemsize == 2
    assert Dtype.UINT32.itemsize == 4
    assert Dtype.UINT64.itemsize == 8
    assert Dtype.INT8.itemsize == 1
    assert Dtype.INT16.itemsize == 2
    assert Dtype.INT32.itemsize == 4
    assert Dtype.INT64.itemsize == 8


def test_char_byte_uint8_share_numpy_dtype():
    # All three map to uint8 — intentional semantic aliasing.
    assert Dtype.CHAR.numpy_dtype == Dtype.BYTE.numpy_dtype
    assert Dtype.BYTE.numpy_dtype == Dtype.UINT8.numpy_dtype


def test_wchar_uint16_share_numpy_dtype():
    assert Dtype.WCHAR.numpy_dtype == Dtype.UINT16.numpy_dtype
