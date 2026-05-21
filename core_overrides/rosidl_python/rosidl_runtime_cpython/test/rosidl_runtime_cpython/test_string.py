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

"""Tests for rosidl_runtime_py.experimental.string."""

import numpy as np
import pytest

from rosidl_runtime_cpython._raw_buffer import RawBuffer
from rosidl_runtime_cpython.dtype import Dtype
from rosidl_runtime_cpython.string import (
    BoundedString,
    BoundedWString,
    String,
    WString,
)


# ===========================================================================
# String
# ===========================================================================

class TestString:

    # -----------------------------------------------------------------------
    # Construction
    # -----------------------------------------------------------------------

    def test_empty(self):
        s = String()
        assert len(s) == 0
        assert str(s) == ''

    def test_dtype(self):
        assert String().dtype is Dtype.CHAR

    def test_from_buffer(self):
        buf = RawBuffer()
        s = String(buffer=buf)
        assert s._buffer is buf

    def test_bad_buffer_type_raises(self):
        with pytest.raises(TypeError, match='RawBuffer'):
            String(buffer=b'hello')

    # -----------------------------------------------------------------------
    # assign / append_str
    # -----------------------------------------------------------------------

    def test_assign_str(self):
        s = String()
        s.assign('hello')
        assert str(s) == 'hello'
        assert len(s) == 5

    def test_assign_bytes(self):
        s = String()
        s.assign(b'world')
        assert str(s) == 'world'

    def test_assign_replaces_content(self):
        s = String()
        s.assign('first')
        s.assign('second')
        assert str(s) == 'second'

    def test_assign_empty_string(self):
        s = String()
        s.assign('hello')
        s.assign('')
        assert len(s) == 0
        assert str(s) == ''

    def test_assign_utf8_multibyte(self):
        # 'café' is 5 UTF-8 bytes (c-a-f-é, where é is 2 bytes).
        s = String()
        s.assign('café')
        assert len(s) == 5
        assert str(s) == 'café'

    def test_append_str(self):
        s = String()
        s.assign('hello')
        s.append_str(' world')
        assert str(s) == 'hello world'
        assert len(s) == 11

    def test_append_str_to_empty(self):
        s = String()
        s.append_str('hi')
        assert str(s) == 'hi'

    def test_assign_bad_type_raises(self):
        with pytest.raises(TypeError, match='assign'):
            String().assign(42)

    # -----------------------------------------------------------------------
    # MutableSequence protocol (byte-level)
    # -----------------------------------------------------------------------

    def test_getitem_byte(self):
        s = String()
        s.assign('ABC')
        assert s[0] == ord('A')
        assert s[1] == ord('B')
        assert s[2] == ord('C')

    def test_getitem_negative(self):
        s = String()
        s.assign('ABC')
        assert s[-1] == ord('C')

    def test_getitem_slice(self):
        s = String()
        s.assign('ABCDE')
        assert list(s[1:4]) == [ord('B'), ord('C'), ord('D')]

    def test_getitem_out_of_range_raises(self):
        s = String()
        s.assign('AB')
        with pytest.raises(IndexError):
            _ = s[5]

    def test_setitem(self):
        s = String()
        s.assign('hello')
        s[0] = ord('H')
        assert str(s) == 'Hello'

    def test_delitem_int(self):
        s = String()
        s.assign('ABC')
        del s[1]
        assert len(s) == 2
        assert s[0] == ord('A')
        assert s[1] == ord('C')

    def test_delitem_slice(self):
        s = String()
        s.assign('ABCDE')
        del s[1:4]
        assert len(s) == 2
        assert s[0] == ord('A')
        assert s[1] == ord('E')

    def test_append_byte(self):
        s = String()
        s.assign('AB')
        s.append(ord('C'))
        assert str(s) == 'ABC'

    def test_insert_byte(self):
        s = String()
        s.assign('AC')
        s.insert(1, ord('B'))
        assert str(s) == 'ABC'

    def test_clear(self):
        s = String()
        s.assign('hello')
        s.clear()
        assert len(s) == 0

    def test_iter(self):
        s = String()
        s.assign('ABC')
        assert list(s) == [ord('A'), ord('B'), ord('C')]

    def test_contains(self):
        s = String()
        s.assign('hello')
        assert ord('e') in s
        assert ord('z') not in s

    def test_reversed(self):
        s = String()
        s.assign('ABC')
        assert list(reversed(s)) == [ord('C'), ord('B'), ord('A')]

    # -----------------------------------------------------------------------
    # Equality
    # -----------------------------------------------------------------------

    def test_eq_string(self):
        a = String()
        b = String()
        a.assign('hello')
        b.assign('hello')
        assert a == b

    def test_eq_str(self):
        s = String()
        s.assign('hello')
        assert s == 'hello'

    def test_neq_str(self):
        s = String()
        s.assign('hello')
        assert s != 'world'

    # -----------------------------------------------------------------------
    # numpy / buffer
    # -----------------------------------------------------------------------

    def test_numpy(self):
        s = String()
        s.assign('ABC')
        arr = s.numpy()
        assert isinstance(arr, np.ndarray)
        assert arr.dtype == np.uint8
        assert list(arr) == [ord('A'), ord('B'), ord('C')]

    def test_buffer_protocol(self):
        s = String()
        s.assign('hello')
        mv = memoryview(s)
        assert len(mv) == s._buffer.size

    # -----------------------------------------------------------------------
    # repr
    # -----------------------------------------------------------------------

    def test_repr(self):
        s = String()
        s.assign('hi')
        assert repr(s) == "String('hi')"


# ===========================================================================
# WString
# ===========================================================================

class TestWString:

    # -----------------------------------------------------------------------
    # Construction
    # -----------------------------------------------------------------------

    def test_empty(self):
        s = WString()
        assert len(s) == 0
        assert str(s) == ''

    def test_dtype(self):
        assert WString().dtype is Dtype.WCHAR

    def test_from_buffer(self):
        buf = RawBuffer()
        s = WString(buffer=buf)
        assert s._buffer is buf

    # -----------------------------------------------------------------------
    # assign / append_str
    # -----------------------------------------------------------------------

    def test_assign_str(self):
        s = WString()
        s.assign('hello')
        assert str(s) == 'hello'
        assert len(s) == 5   # 5 UTF-16 code units

    def test_assign_multibyte(self):
        # 'café' — 4 code units in UTF-16-LE.
        s = WString()
        s.assign('café')
        assert len(s) == 4
        assert str(s) == 'café'

    def test_assign_surrogates(self):
        # U+1F600 (😀) encodes as 2 code units in UTF-16.
        s = WString()
        s.assign('\U0001F600')
        assert len(s) == 2
        assert str(s) == '\U0001F600'

    def test_assign_replaces_content(self):
        s = WString()
        s.assign('first')
        s.assign('second')
        assert str(s) == 'second'

    def test_append_str(self):
        s = WString()
        s.assign('hello')
        s.append_str(' world')
        assert str(s) == 'hello world'
        assert len(s) == 11

    def test_assign_bytes(self):
        data = 'AB'.encode('utf-16-le')
        s = WString()
        s.assign(data)
        assert len(s) == 2
        assert str(s) == 'AB'

    def test_assign_odd_bytes_raises(self):
        with pytest.raises(ValueError, match='even length'):
            WString().assign(b'\x00\x00\x00')

    # -----------------------------------------------------------------------
    # MutableSequence protocol (code-unit level)
    # -----------------------------------------------------------------------

    def test_getitem_unit(self):
        s = WString()
        s.assign('ABC')
        assert s[0] == ord('A')
        assert s[-1] == ord('C')

    def test_getitem_slice(self):
        s = WString()
        s.assign('ABCDE')
        assert list(s[1:4]) == [ord('B'), ord('C'), ord('D')]

    def test_getitem_out_of_range_raises(self):
        s = WString()
        s.assign('AB')
        with pytest.raises(IndexError):
            _ = s[5]

    def test_setitem(self):
        s = WString()
        s.assign('hello')
        s[0] = ord('H')
        assert str(s) == 'Hello'

    def test_delitem(self):
        s = WString()
        s.assign('ABC')
        del s[1]
        assert len(s) == 2
        assert s[0] == ord('A')
        assert s[1] == ord('C')

    def test_append_unit(self):
        s = WString()
        s.assign('AB')
        s.append(ord('C'))
        assert str(s) == 'ABC'

    def test_insert_unit(self):
        s = WString()
        s.assign('AC')
        s.insert(1, ord('B'))
        assert str(s) == 'ABC'

    def test_clear(self):
        s = WString()
        s.assign('hello')
        s.clear()
        assert len(s) == 0

    def test_iter(self):
        s = WString()
        s.assign('ABC')
        assert list(s) == [ord('A'), ord('B'), ord('C')]

    def test_contains(self):
        s = WString()
        s.assign('hello')
        assert ord('e') in s
        assert ord('z') not in s

    def test_reversed(self):
        s = WString()
        s.assign('ABC')
        assert list(reversed(s)) == [ord('C'), ord('B'), ord('A')]

    # -----------------------------------------------------------------------
    # Equality
    # -----------------------------------------------------------------------

    def test_eq_wstring(self):
        a = WString()
        b = WString()
        a.assign('hello')
        b.assign('hello')
        assert a == b

    def test_eq_str(self):
        s = WString()
        s.assign('hello')
        assert s == 'hello'

    # -----------------------------------------------------------------------
    # numpy / buffer
    # -----------------------------------------------------------------------

    def test_numpy(self):
        s = WString()
        s.assign('ABC')
        arr = s.numpy()
        assert isinstance(arr, np.ndarray)
        assert arr.dtype == np.dtype('<u2')
        assert list(arr) == [ord('A'), ord('B'), ord('C')]

    def test_buffer_protocol(self):
        s = WString()
        s.assign('hello')
        mv = memoryview(s)
        assert len(mv) == s._buffer.size

    # -----------------------------------------------------------------------
    # repr
    # -----------------------------------------------------------------------

    def test_repr(self):
        s = WString()
        s.assign('hi')
        assert repr(s) == "WString('hi')"


# ===========================================================================
# BoundedString
# ===========================================================================

class TestBoundedString:

    def test_max_size(self):
        s = BoundedString(10)
        assert s.max_size == 10

    def test_assign_within_bound(self):
        s = BoundedString(5)
        s.assign('hello')
        assert str(s) == 'hello'

    def test_assign_over_bound_raises(self):
        s = BoundedString(4)
        with pytest.raises(ValueError, match='upper bound'):
            s.assign('hello')  # 5 bytes > 4

    def test_append_str_over_bound_raises(self):
        s = BoundedString(5)
        s.assign('hello')
        with pytest.raises(ValueError, match='upper bound'):
            s.append_str('!')

    def test_append_byte_over_bound_raises(self):
        s = BoundedString(3)
        s.assign('ABC')
        with pytest.raises(ValueError, match='upper bound'):
            s.append(ord('D'))

    def test_insert_over_bound_raises(self):
        s = BoundedString(3)
        s.assign('ABC')
        with pytest.raises(ValueError, match='upper bound'):
            s.insert(0, ord('X'))

    def test_zero_upper_bound_raises(self):
        with pytest.raises(ValueError, match='positive integer'):
            BoundedString(0)

    def test_repr(self):
        s = BoundedString(10)
        s.assign('hi')
        assert repr(s) == "BoundedString(10, 'hi')"

    def test_utf8_multibyte_bound_enforced_on_bytes(self):
        # 'café' = 5 UTF-8 bytes; bound of 4 should reject it.
        s = BoundedString(4)
        with pytest.raises(ValueError, match='upper bound'):
            s.assign('café')

    def test_inherits_string_interface(self):
        s = BoundedString(20)
        s.assign('hello')
        s.append_str(' world')
        assert str(s) == 'hello world'


# ===========================================================================
# BoundedWString
# ===========================================================================

class TestBoundedWString:

    def test_max_size(self):
        s = BoundedWString(10)
        assert s.max_size == 10

    def test_assign_within_bound(self):
        s = BoundedWString(5)
        s.assign('hello')
        assert str(s) == 'hello'

    def test_assign_over_bound_raises(self):
        s = BoundedWString(4)
        with pytest.raises(ValueError, match='upper bound'):
            s.assign('hello')  # 5 code units > 4

    def test_append_str_over_bound_raises(self):
        s = BoundedWString(5)
        s.assign('hello')
        with pytest.raises(ValueError, match='upper bound'):
            s.append_str('!')

    def test_append_unit_over_bound_raises(self):
        s = BoundedWString(3)
        s.assign('ABC')
        with pytest.raises(ValueError, match='upper bound'):
            s.append(ord('D'))

    def test_zero_upper_bound_raises(self):
        with pytest.raises(ValueError, match='positive integer'):
            BoundedWString(0)

    def test_repr(self):
        s = BoundedWString(10)
        s.assign('hi')
        assert repr(s) == "BoundedWString(10, 'hi')"

    def test_surrogate_pair_bound_counted_as_two_units(self):
        # U+1F600 = 2 code units; bound of 1 should reject it.
        s = BoundedWString(1)
        with pytest.raises(ValueError, match='upper bound'):
            s.assign('\U0001F600')

    def test_inherits_wstring_interface(self):
        s = BoundedWString(20)
        s.assign('hello')
        s.append_str(' world')
        assert str(s) == 'hello world'
