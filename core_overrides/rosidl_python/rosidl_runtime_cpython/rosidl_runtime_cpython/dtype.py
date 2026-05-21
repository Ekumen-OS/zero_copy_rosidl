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

"""ROS primitive type descriptors used by experimental container types."""

from __future__ import annotations

import enum
from typing import Any

import numpy as np
import numpy.typing as npt


class Dtype(enum.Enum):
    """
    Descriptor for the 15 ROS IDL primitive scalar types.

    Each member carries:
      numpy_dtype  -- a numpy.dtype object for use with np.frombuffer()
      format_char  -- struct module format character (native-endian)
      itemsize     -- byte width of one element (== numpy_dtype.itemsize)

    String / WString are treated as byte-element sequences (1 byte per
    element for String, 2 bytes per element for WString) rather than
    fixed-width character arrays, matching the rosidl_runtime_cpp semantics.
    """

    # Unsigned integers
    UINT8 = (0, 'uint8', 'B')
    UINT16 = (1, '<u2', 'H')
    UINT32 = (2, '<u4', 'I')
    UINT64 = (3, '<u8', 'Q')

    # Signed integers
    INT8 = (4, 'int8', 'b')
    INT16 = (5, '<i2', 'h')
    INT32 = (6, '<i4', 'i')
    INT64 = (7, '<i8', 'q')

    # Floating point
    FLOAT32 = (8, 'float32', 'f')
    FLOAT64 = (9, 'float64', 'd')
    LONG_DOUBLE = (10, 'longdouble', None)

    # Character code units (treated as small integers for numpy purposes)
    CHAR = (11, 'uint8', 'B')              # ROS char  — unsigned 8-bit
    WCHAR = (12, '<u2', 'H')               # ROS wchar — UTF-16 code unit (LE)
    # Boolean / byte
    BOOL = (13, 'bool', '?')
    BYTE = (14, 'uint8', 'B')              # ROS byte (octet)

    # Runtime-set attributes (assigned in __new__, declared here for type checkers)
    numpy_dtype: npt.DTypeLike
    format_char: str | None
    itemsize: int

    def __new__(cls, value: int, numpy_dtype_str: str, format_char: str) -> Dtype:
        obj = object.__new__(cls)
        obj._value_ = value
        obj.numpy_dtype = np.dtype(numpy_dtype_str)
        obj.format_char = format_char
        obj.itemsize = obj.numpy_dtype.itemsize
        return obj

    def __repr__(self) -> str:
        return f'Dtype.{self.name}'

    def pack(self, value: Any) -> bytes:
        """Pack a single Python value into its raw byte representation."""
        return np.array(value, dtype=self.numpy_dtype).tobytes()

    def unpack(self, data: bytes | bytearray) -> Any:
        """Unpack a single Python value from its raw byte representation."""
        return np.frombuffer(data, dtype=self.numpy_dtype, count=1)[0].item()
