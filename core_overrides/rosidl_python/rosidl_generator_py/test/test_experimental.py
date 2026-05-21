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

"""Tests for rosidl_generator_py.experimental helper functions."""

import pytest

from rosidl_parser.definition import (
    Array,
    BasicType,
    BoundedSequence,
    BoundedString,
    BoundedWString,
    NamespacedType,
    String,
    UnboundedSequence,
    UnboundedString,
    UnboundedWString,
)

from rosidl_generator_py.experimental import (
    BASIC_TYPE_TO_DTYPE,
    experimental_constraint_type,
    experimental_msg_type,
)


# ---------------------------------------------------------------------------
# BASIC_TYPE_TO_DTYPE mapping
# ---------------------------------------------------------------------------

class TestBasicTypeToDtype:

    def test_all_basic_types_mapped(self):
        expected_keys = {
            'boolean', 'octet', 'char', 'wchar',
            'float', 'double', 'long double',
            'uint8', 'int8', 'uint16', 'int16',
            'uint32', 'int32', 'uint64', 'int64',
        }
        assert set(BASIC_TYPE_TO_DTYPE.keys()) == expected_keys

    def test_values_are_dtype_strings(self):
        for key, value in BASIC_TYPE_TO_DTYPE.items():
            assert value.startswith('Dtype.'), f'{key} -> {value}'


# ---------------------------------------------------------------------------
# experimental_msg_type
# ---------------------------------------------------------------------------

class TestExperimentalMsgType:

    @pytest.mark.parametrize('typename, expected_dtype', [
        ('boolean', 'Dtype.BOOL'),
        ('int32', 'Dtype.INT32'),
        ('float', 'Dtype.FLOAT32'),
        ('double', 'Dtype.FLOAT64'),
        ('uint8', 'Dtype.UINT8'),
        ('char', 'Dtype.CHAR'),
    ])
    def test_basic_type_becomes_scalar(self, typename, expected_dtype):
        bt = BasicType(typename)
        result = experimental_msg_type(bt)
        assert result == f'Scalar({expected_dtype})'

    def test_unbounded_string(self):
        result = experimental_msg_type(UnboundedString())
        assert result == 'String()'

    def test_bounded_string(self):
        result = experimental_msg_type(BoundedString(128))
        assert result == 'BoundedString(128)'

    def test_unbounded_wstring(self):
        result = experimental_msg_type(UnboundedWString())
        assert result == 'WString()'

    def test_bounded_wstring(self):
        result = experimental_msg_type(BoundedWString(64))
        assert result == 'BoundedWString(64)'

    def test_array_of_basic_type(self):
        result = experimental_msg_type(Array(BasicType('int32'), 10))
        assert result == 'Array(Dtype.INT32, 10)'

    def test_array_of_namespaced_type(self):
        nt = NamespacedType(['pkg', 'msg'], 'Sub')
        result = experimental_msg_type(Array(nt, 3))
        assert 'Array(Sub, 3' in result
        assert 'MessageInitialization.SKIP' in result

    def test_bounded_sequence_of_basic(self):
        result = experimental_msg_type(BoundedSequence(BasicType('float'), 50))
        assert result == 'BoundedSequence(Dtype.FLOAT32, 50)'

    def test_unbounded_sequence_of_basic(self):
        result = experimental_msg_type(UnboundedSequence(BasicType('int64')))
        assert result == 'Sequence(Dtype.INT64)'

    def test_unbounded_sequence_of_string(self):
        result = experimental_msg_type(UnboundedSequence(UnboundedString()))
        assert result == 'Sequence(String)'

    def test_unbounded_sequence_of_namespaced(self):
        nt = NamespacedType(['pkg', 'msg'], 'Sub')
        result = experimental_msg_type(UnboundedSequence(nt))
        assert result == 'Sequence(Sub)'

    def test_array_of_unbounded_string(self):
        result = experimental_msg_type(Array(UnboundedString(), 5))
        assert 'Array(String, 5' in result

    def test_array_of_bounded_string(self):
        result = experimental_msg_type(Array(BoundedString(32), 3))
        assert 'Array(BoundedString, 3' in result
        assert 'BoundedString(32)' in result


# ---------------------------------------------------------------------------
# experimental_constraint_type
# ---------------------------------------------------------------------------

class TestExperimentalConstraintType:

    def test_basic_type_no_constraint(self):
        assert experimental_constraint_type(BasicType('int32')) is None

    def test_bounded_string_no_constraint(self):
        assert experimental_constraint_type(BoundedString(128)) is None

    def test_unbounded_string_has_string_constraint(self):
        assert experimental_constraint_type(UnboundedString()) == 'StringConstraint'

    def test_unbounded_wstring_has_string_constraint(self):
        assert experimental_constraint_type(UnboundedWString()) == 'StringConstraint'

    def test_bounded_wstring_no_constraint(self):
        assert experimental_constraint_type(BoundedWString(64)) is None

    def test_namespaced_type_has_constraints(self):
        nt = NamespacedType(['pkg', 'msg'], 'Sub')
        assert experimental_constraint_type(nt) == 'Sub.Constraints'

    def test_array_of_basic_no_constraint(self):
        assert experimental_constraint_type(Array(BasicType('int32'), 10)) is None

    def test_array_of_unbounded_string_has_constraint(self):
        assert experimental_constraint_type(
            Array(UnboundedString(), 5)) == 'StringConstraint'

    def test_array_of_namespaced_has_constraint(self):
        nt = NamespacedType(['pkg', 'msg'], 'Sub')
        assert experimental_constraint_type(Array(nt, 3)) == 'Sub.Constraints'

    def test_bounded_sequence_no_constraint(self):
        assert experimental_constraint_type(
            BoundedSequence(BasicType('int32'), 100)) is None

    def test_unbounded_sequence_has_constraint(self):
        assert experimental_constraint_type(
            UnboundedSequence(BasicType('int32'))) == 'SequenceConstraint'

    def test_unbounded_sequence_of_string_has_constraint(self):
        assert experimental_constraint_type(
            UnboundedSequence(UnboundedString())) == 'SequenceConstraint'
