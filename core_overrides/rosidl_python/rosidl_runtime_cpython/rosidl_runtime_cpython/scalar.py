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

"""Experimental Scalar type for rosidl_runtime_py."""

from __future__ import annotations

from typing import Any

import numpy as np
import numpy.typing as npt

from rosidl_runtime_cpython.dtype import Dtype
from rosidl_runtime_cpython._raw_buffer import RawBuffer


class Scalar:
    """
    A single ROS primitive value backed by a RawBuffer.

    The value is stored as a length-1 numpy array inside a RawBuffer, giving
    zero-copy interoperability with numpy.  Numeric protocol methods
    (__int__, __float__, __bool__, __index__) forward to the underlying value.

    Construction
    ------------
    Scalar(dtype, value=0)
        Allocate managed storage and initialise to *value*.

    Scalar(dtype, buffer=buf)
        Use an existing RawBuffer (managed or external) as backing storage.
        *buf* must have size >= dtype.itemsize.

    The *buffer* keyword argument is the sole mechanism for wrapping an
    external rosidl_memory_region_t — callers in C/C++ create the RawBuffer
    via RawBuffer_FromRegion() and pass it here.
    """

    __slots__ = ('_dtype', '_buffer', '_view')

    def __init__(
        self,
        dtype: Dtype,
        value: Any = 0,
        *,
        buffer: RawBuffer | None = None,
    ) -> None:
        """
        Initialise a Scalar.

        Parameters
        ----------
        dtype : Dtype
            The ROS primitive type of this scalar.
        value : Any, optional
            Initial value when *buffer* is None.  Ignored if *buffer* is given.
        buffer : RawBuffer, optional
            Existing RawBuffer to use as storage.  Must have size >=
            dtype.itemsize.  If None, a managed RawBuffer is allocated.
        """
        if not isinstance(dtype, Dtype):
            raise TypeError(f'dtype must be a Dtype, got {type(dtype).__name__}')
        self._dtype = dtype

        if buffer is not None:
            if not isinstance(buffer, RawBuffer):
                raise TypeError(
                    f'buffer must be a RawBuffer, got {type(buffer).__name__}')
            if buffer.size < dtype.itemsize:
                raise ValueError(
                    f'buffer size {buffer.size} is smaller than '
                    f'itemsize {dtype.itemsize} for {dtype!r}')
            self._buffer = buffer
        else:
            self._buffer = RawBuffer(dtype.itemsize)

        self._view: npt.NDArray[Any] = np.frombuffer(
            self._buffer, dtype=dtype.numpy_dtype)

        if buffer is None:
            self._view[0] = value

    # ------------------------------------------------------------------
    # Value access
    # ------------------------------------------------------------------

    @property
    def dtype(self) -> Dtype:
        """Dtype: the ROS primitive type of this scalar."""
        return self._dtype

    @property
    def value(self) -> Any:
        """Current scalar value as a Python scalar."""
        return self._view[0].item()

    @value.setter
    def value(self, v: Any) -> None:
        self._view[0] = np.array(v).astype(self._dtype.numpy_dtype)

    # ------------------------------------------------------------------
    # Numeric protocol
    # ------------------------------------------------------------------

    def __int__(self) -> int:
        return int(self._view[0])

    def __float__(self) -> float:
        return float(self._view[0])

    def __bool__(self) -> bool:
        return bool(self._view[0])

    def __index__(self) -> int:
        # Only valid for integer-typed scalars; numpy will raise for floats.
        if not np.issubdtype(self._dtype.numpy_dtype, np.integer):
            raise TypeError(f'__index__ is only valid for integer dtypes, got {self._dtype}')
        return self._view[0].__index__()

    # ------------------------------------------------------------------
    # Comparison / hashing
    # ------------------------------------------------------------------

    def __eq__(self, other: object) -> bool:
        if isinstance(other, Scalar):
            return self._dtype == other._dtype and self.value == other.value
        return bool(self.value == other)

    def __hash__(self) -> int:
        return hash((self._dtype, self.value))

    # ------------------------------------------------------------------
    # Buffer / numpy protocols
    # ------------------------------------------------------------------

    def __buffer__(self, flags: int) -> memoryview:
        """PEP 688 buffer protocol (Python >= 3.12)."""
        return memoryview(self._buffer)

    def numpy(self) -> npt.NDArray[Any]:
        """
        Return a length-1 numpy array backed by this scalar's RawBuffer.

        The returned array is a *live view* for managed buffers; it becomes
        stale (but valid) if the backing buffer is replaced.
        """
        return self._view

    # ------------------------------------------------------------------
    # Representation
    # ------------------------------------------------------------------

    def __repr__(self) -> str:
        return f'Scalar({self._dtype!r}, {self.value!r})'

    def __str__(self) -> str:
        return str(self.value)
