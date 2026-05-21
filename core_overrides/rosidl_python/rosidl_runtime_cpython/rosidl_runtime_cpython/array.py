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

"""Experimental Array type for rosidl_runtime_py."""

from __future__ import annotations

from typing import Any, Iterator

import numpy as np
import numpy.typing as npt

from rosidl_runtime_cpython.dtype import Dtype
from rosidl_runtime_cpython._raw_buffer import RawBuffer


class Array:
    """
    A fixed-length, homogeneous sequence of ROS interface values.

    Two storage modes are selected automatically based on *dtype*:

    **Primitive** (``dtype`` is a :class:`Dtype`)
        Elements are stored contiguously in a :class:`RawBuffer` and
        exposed as a 1-D numpy array.  Zero-copy interoperability with
        numpy is available via :meth:`numpy` and the PEP 3118 buffer
        protocol (``__buffer__``, Python >= 3.12).

    **Object** (``dtype`` is a Python type or ``None``)
        Elements are stored in a numpy object array (``dtype=object``),
        enabling uniform numpy access for both modes.  ``__buffer__`` is
        not supported (numpy object arrays do not implement the buffer
        protocol), but :meth:`numpy` is available.

    Construction
    ------------
    ``Array(dtype, length)``
        Allocate storage for *length* elements of *dtype*.

    ``Array(dtype, length, buffer=buf)``
        Use an existing :class:`RawBuffer` as backing storage (primitive
        mode only).  *buf* must have ``size >= length * dtype.itemsize``.
        The sole mechanism for wrapping an external
        ``rosidl_memory_region_t`` is to pass a ``RawBuffer`` created in
        C/C++ via ``RawBuffer_FromRegion()``.

    Notes
    -----
    Length is fixed at construction; there is no ``resize``.
    """

    __slots__ = ('_dtype', '_is_primitive', '_length', '_buffer', '_view')

    def __init__(
        self,
        dtype: Dtype | type[Any],
        length: int,
        *,
        data: Any = None,
        buffer: RawBuffer | None = None,
    ) -> None:
        if not isinstance(length, int) or length < 0:
            raise ValueError(
                f'length must be a non-negative integer, got {length!r}')
        self._dtype = dtype
        self._length = length
        self._is_primitive = isinstance(dtype, Dtype)

        if self._is_primitive:
            needed = length * dtype.itemsize
            if buffer is not None:
                if not isinstance(buffer, RawBuffer):
                    raise TypeError(
                        f'buffer must be a RawBuffer, got {type(buffer).__name__}')
                if buffer.size < needed:
                    raise ValueError(
                        f'buffer size {buffer.size} is smaller than '
                        f'{needed} bytes needed for {length} × {dtype!r} elements')
                self._buffer: RawBuffer | None = buffer
            else:
                self._buffer = RawBuffer(needed) if needed > 0 else RawBuffer()
            self._view: npt.NDArray[Any] = (
                np.frombuffer(self._buffer, dtype=dtype.numpy_dtype, count=length)
                if length > 0
                else np.empty(0, dtype=dtype.numpy_dtype)
            )
        else:
            if buffer is not None:
                raise TypeError(
                    'buffer= is only supported for Dtype (primitive) arrays')
            self._buffer = None
            # numpy object array: stores PyObject* pointers, supports arbitrary
            # Python objects; initialised to None (numpy zeros object array).
            self._view = np.full(length, None, dtype=object)

        if data is not None:
            items = list(data)
            if len(items) != length:
                raise ValueError(
                    f'data length {len(items)} does not match array length {length}')
            if self._is_primitive:
                self._view[:] = items
            else:
                for i, v in enumerate(items):
                    self._view[i] = v

    # ------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------

    @property
    def dtype(self) -> Dtype | type[Any]:
        """The element type of this array (a :class:`Dtype` or a Python type)."""
        return self._dtype

    # ------------------------------------------------------------------
    # Sequence protocol
    # ------------------------------------------------------------------

    def __len__(self) -> int:
        return self._length

    def __getitem__(self, index: int | slice) -> Any:
        if isinstance(index, int):
            # numpy scalars from primitive arrays need .item() to become Python
            # scalars; object-array elements are already Python objects.
            val = self._view[index]
            return val.item() if self._is_primitive else val
        return self._view[index]

    def __setitem__(self, index: int | slice, value: Any) -> None:
        self._view[index] = value

    def __iter__(self) -> Iterator[Any]:
        if self._is_primitive:
            for i in range(self._length):
                yield self._view[i].item()
        else:
            yield from self._view

    def __contains__(self, value: object) -> bool:
        return bool(np.any(self._view == value))

    def __reversed__(self) -> Iterator[Any]:
        if self._is_primitive:
            for i in range(self._length - 1, -1, -1):
                yield self._view[i].item()
        else:
            for i in range(self._length - 1, -1, -1):
                yield self._view[i]

    # ------------------------------------------------------------------
    # Comparison
    # ------------------------------------------------------------------

    def __eq__(self, other: object) -> bool:
        if isinstance(other, Array):
            if self._dtype != other._dtype or self._length != other._length:
                return False
            return bool(np.array_equal(self._view, other._view))
        try:
            return bool(np.array_equal(self._view, other))
        except (TypeError, ValueError):
            return NotImplemented  # type: ignore[return-value]

    # ------------------------------------------------------------------
    # Buffer / numpy protocols
    # ------------------------------------------------------------------

    def __buffer__(self, flags: int) -> memoryview:
        """PEP 688 buffer protocol (Python >= 3.12, primitive arrays only)."""
        if not self._is_primitive:
            raise TypeError('__buffer__ is not supported for object-typed arrays')
        return memoryview(self._buffer)  # type: ignore[arg-type]

    def numpy(self) -> npt.NDArray[Any]:
        """
        Return a 1-D numpy array backed by this Array's storage.

        For **primitive** arrays the returned array is a live view backed
        by the :class:`RawBuffer` — mutations are reflected in the Array
        and vice versa.

        For **object** arrays the returned array is the internal numpy
        object array itself; elements can be read and replaced through it.
        """
        return self._view

    # ------------------------------------------------------------------
    # Representation
    # ------------------------------------------------------------------

    def __repr__(self) -> str:
        return f'Array({self._dtype!r}, {self._length}, data={self._view.tolist()!r})'
