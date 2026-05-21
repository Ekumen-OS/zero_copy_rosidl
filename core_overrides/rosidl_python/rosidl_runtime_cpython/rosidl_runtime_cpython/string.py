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

"""Experimental String, WString, BoundedString, and BoundedWString types."""

from __future__ import annotations

from typing import Any, Iterator

import numpy as np
import numpy.typing as npt

from rosidl_runtime_cpython.dtype import Dtype
from rosidl_runtime_cpython._raw_buffer import RawBuffer


class String:
    """
    A mutable UTF-8 string backed by a contiguous :class:`RawBuffer`.

    The MutableSequence interface operates on individual **bytes** (uint8),
    exactly mirroring the C ``rosidl_runtime_c__String`` layout.  For bulk
    string operations prefer :meth:`assign` and :meth:`append_str`.

    All byte-level mutation is performed with direct ``memoryview`` slice
    assignment for efficiency; no element-by-element Python dispatch occurs.

    Construction
    ------------
    ``String()``
        Empty string with managed storage.

    ``String(buffer=buf)``
        Use *buf* as backing storage.  Mutations that exceed capacity raise
        :exc:`BufferError` for external (non-owning) buffers.
    """

    __slots__ = ('_buffer', '_size', '_view')

    _DTYPE = Dtype.CHAR    # uint8 — matches rosidl_runtime_c__String data type

    def __init__(
        self,
        data: str | bytes | bytearray | memoryview | None = None,
        *,
        buffer: RawBuffer | None = None,
    ) -> None:
        if buffer is not None:
            if not isinstance(buffer, RawBuffer):
                raise TypeError(
                    f'buffer must be a RawBuffer, got {type(buffer).__name__}')
            self._buffer = buffer
        else:
            self._buffer = RawBuffer()
        self._size = 0   # number of live bytes (NOT counting any NUL terminator)
        self._view: npt.NDArray[Any] = self._make_view()
        if data is not None:
            self.assign(data)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _capacity(self) -> int:
        return self._buffer.size

    def _make_view(self) -> npt.NDArray[Any]:
        if self._size == 0:
            return np.empty(0, dtype=self._DTYPE.numpy_dtype)
        return np.frombuffer(self._buffer, dtype=self._DTYPE.numpy_dtype,
                             count=self._size)

    def _refresh_view(self) -> None:
        self._view = self._make_view()

    def _ensure_capacity(self, needed_bytes: int) -> None:
        if needed_bytes <= self._capacity():
            return
        if not self._buffer.is_owner:
            raise BufferError(
                f'String capacity {self._capacity()} B exceeded for fixed '
                f'external buffer (needed {needed_bytes} B)')
        new_cap = max(8, self._capacity())
        while new_cap < needed_bytes:
            new_cap *= 2
        self._buffer.reserve(new_cap)

    # ------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------

    @property
    def dtype(self) -> Dtype:
        """Dtype: always :attr:`Dtype.CHAR`."""
        return self._DTYPE

    # ------------------------------------------------------------------
    # Bulk operations (preferred API)
    # ------------------------------------------------------------------

    def assign(self, s: str | bytes | bytearray | memoryview) -> None:
        """
        Replace the string contents with *s* (encoded as UTF-8).

        Parameters
        ----------
        s : str or bytes
            Source text.  ``str`` is encoded with ``'utf-8'``; ``bytes``
            are used directly.
        """
        if isinstance(s, str):
            data = s.encode('utf-8')
        elif isinstance(s, (bytes, bytearray, memoryview)):
            data = bytes(s)
        else:
            raise TypeError(f'assign requires str or bytes, got {type(s).__name__}')
        n = len(data)
        self._check_bound(n)
        self._ensure_capacity(n)
        self._buffer.resize(n)
        self._size = n
        self._refresh_view()
        if n:
            self._view[:] = np.frombuffer(data, dtype=self._DTYPE.numpy_dtype)

    def append_str(self, s: str | bytes | bytearray | memoryview) -> None:
        """
        Append *s* (encoded as UTF-8) to the current contents.

        Parameters
        ----------
        s : str or bytes
        """
        if isinstance(s, str):
            data = s.encode('utf-8')
        elif isinstance(s, (bytes, bytearray, memoryview)):
            data = bytes(s)
        else:
            raise TypeError(
                f'append_str requires str or bytes, got {type(s).__name__}')
        n = len(data)
        new_size = self._size + n
        self._check_bound(new_size)
        self._ensure_capacity(new_size)
        self._buffer.resize(new_size)
        prev_size = self._size
        self._size = new_size
        self._refresh_view()
        self._view[prev_size:new_size] = np.frombuffer(data, dtype=self._DTYPE.numpy_dtype)

    def _check_bound(self, needed: int) -> None:
        pass

    # ------------------------------------------------------------------
    # MutableSequence protocol (byte-level)
    # ------------------------------------------------------------------

    def __len__(self) -> int:
        return self._size

    def __getitem__(self, index: int | slice) -> Any:
        if isinstance(index, int):
            if index < 0:
                index += self._size
            if not (0 <= index < self._size):
                raise IndexError('string index out of range')
            return int(self._view[index])
        return list(self._view[index])

    def __setitem__(self, index: int | slice, value: Any) -> None:
        if isinstance(index, int):
            if index < 0:
                index += self._size
            if not (0 <= index < self._size):
                raise IndexError('string index out of range')
        self._view[index] = value

    def __delitem__(self, index: int | slice) -> None:
        arr = np.array(self._view, copy=True)
        if isinstance(index, int):
            if index < 0:
                index += self._size
            if not (0 <= index < self._size):
                raise IndexError('string index out of range')
            new_arr = np.concatenate([arr[:index], arr[index + 1:]])
        else:
            keep = np.ones(self._size, dtype=bool)
            for i in range(*index.indices(self._size)):
                keep[i] = False
            new_arr = arr[keep]
        n = len(new_arr)
        self._buffer.resize(n)
        self._size = n
        self._refresh_view()
        if n:
            self._view[:] = new_arr

    def __iter__(self) -> Iterator[int]:
        for i in range(self._size):
            yield int(self._view[i])

    def __contains__(self, value: object) -> bool:
        if isinstance(value, int):
            return bool(np.any(self._view == value))
        return False

    def __reversed__(self) -> Iterator[int]:
        for i in range(self._size - 1, -1, -1):
            yield int(self._view[i])

    def __eq__(self, other: object) -> bool:
        if isinstance(other, String):
            return bool(np.array_equal(self._view, other._view))
        if isinstance(other, str):
            try:
                return self.__str__() == other
            except UnicodeDecodeError:
                return False
        return NotImplemented  # type: ignore[return-value]

    def append(self, byte_value: int) -> None:
        """Append a single byte value (int 0–255)."""
        self._check_bound(self._size + 1)
        self._ensure_capacity(self._size + 1)
        self._buffer.resize(self._size + 1)
        self._size += 1
        self._refresh_view()
        self._view[self._size - 1] = byte_value

    def insert(self, index: int, byte_value: int) -> None:
        """Insert *byte_value* before *index*."""
        arr = np.array(self._view, copy=True)
        if index < 0:
            index = max(0, self._size + index)
        if index >= self._size:
            self.append(byte_value)
            return
        self._check_bound(self._size + 1)
        self._ensure_capacity(self._size + 1)
        new_arr = np.concatenate([arr[:index], [byte_value], arr[index:]])
        n = len(new_arr)
        self._buffer.resize(n)
        self._size = n
        self._refresh_view()
        self._view[:] = new_arr

    def clear(self) -> None:
        """Remove all bytes (buffer capacity is preserved)."""
        self._buffer.resize(0)
        self._size = 0
        self._refresh_view()

    # ------------------------------------------------------------------
    # String conversion
    # ------------------------------------------------------------------

    def __str__(self) -> str:
        return self._view.tobytes().decode('utf-8')

    def __repr__(self) -> str:
        return f'{type(self).__name__}({self.__str__()!r})'

    # ------------------------------------------------------------------
    # Buffer / numpy
    # ------------------------------------------------------------------

    def __buffer__(self, flags: int) -> memoryview:
        """PEP 688: writable buffer over the live bytes."""
        return memoryview(self._buffer)

    def numpy(self) -> npt.NDArray[np.uint8]:
        """Return a 1-D uint8 numpy array over the live bytes."""
        return self._view


class WString:
    """
    A mutable UTF-16-LE string backed by a contiguous :class:`RawBuffer`.

    The MutableSequence interface operates on individual **code units** (uint16),
    exactly mirroring the C ``rosidl_runtime_c__U16String`` layout.  For bulk
    string operations prefer :meth:`assign` and :meth:`append_str`.

    Construction
    ------------
    ``WString()``
        Empty string with managed storage.

    ``WString(buffer=buf)``
        Use *buf* as backing storage.  Mutations that exceed capacity raise
        :exc:`BufferError` for external (non-owning) buffers.
    """

    __slots__ = ('_buffer', '_size', '_view')

    _DTYPE = Dtype.WCHAR   # uint16 — matches rosidl_runtime_c__U16String data type
    _ITEMSIZE = 2          # bytes per UTF-16 code unit

    def __init__(
        self,
        data: str | bytes | bytearray | memoryview | None = None,
        *,
        buffer: RawBuffer | None = None,
    ) -> None:
        if buffer is not None:
            if not isinstance(buffer, RawBuffer):
                raise TypeError(
                    f'buffer must be a RawBuffer, got {type(buffer).__name__}')
            self._buffer = buffer
        else:
            self._buffer = RawBuffer()
        self._size = 0   # number of live code units
        self._view: npt.NDArray[Any] = self._make_view()
        if data is not None:
            self.assign(data)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _capacity(self) -> int:
        return self._buffer.size // self._ITEMSIZE

    def _ensure_capacity(self, needed_units: int) -> None:
        needed_bytes = needed_units * self._ITEMSIZE
        if needed_bytes <= self._buffer.size:
            return
        if not self._buffer.is_owner:
            raise BufferError(
                f'WString capacity {self._capacity()} code units exceeded for '
                f'fixed external buffer (needed {needed_units})')
        new_cap = max(8, self._capacity())
        while new_cap < needed_units:
            new_cap *= 2
        self._buffer.reserve(new_cap * self._ITEMSIZE)

    def _make_view(self) -> npt.NDArray[Any]:
        if self._size == 0:
            return np.empty(0, dtype=self._DTYPE.numpy_dtype)
        return np.frombuffer(self._buffer, dtype=self._DTYPE.numpy_dtype,
                             count=self._size)

    def _refresh_view(self) -> None:
        self._view = self._make_view()

    # ------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------

    @property
    def dtype(self) -> Dtype:
        """Dtype: always :attr:`Dtype.WCHAR`."""
        return self._DTYPE

    # ------------------------------------------------------------------
    # Bulk operations (preferred API)
    # ------------------------------------------------------------------

    def assign(self, s: str | bytes | bytearray | memoryview) -> None:
        """
        Replace the string contents with *s* (encoded as UTF-16-LE).

        Parameters
        ----------
        s : str or bytes
            Source text.  ``str`` is encoded with ``'utf-16-le'``; ``bytes``
            must be a valid UTF-16-LE byte string (length must be even).
        """
        if isinstance(s, str):
            data = s.encode('utf-16-le')
        elif isinstance(s, (bytes, bytearray, memoryview)):
            data = bytes(s)
            if len(data) % 2 != 0:
                raise ValueError('bytes for WString must have even length')
        else:
            raise TypeError(
                f'assign requires str or bytes, got {type(s).__name__}')
        n_units = len(data) // self._ITEMSIZE
        self._check_bound(n_units)
        n_bytes = len(data)
        self._ensure_capacity(n_units)
        self._buffer.resize(n_bytes)
        self._size = n_units
        self._refresh_view()
        if n_bytes:
            self._view[:] = np.frombuffer(data, dtype=self._DTYPE.numpy_dtype)

    def append_str(self, s: str | bytes | bytearray | memoryview) -> None:
        """
        Append *s* (encoded as UTF-16-LE) to the current contents.

        Parameters
        ----------
        s : str or bytes
        """
        if isinstance(s, str):
            data = s.encode('utf-16-le')
        elif isinstance(s, (bytes, bytearray, memoryview)):
            data = bytes(s)
            if len(data) % 2 != 0:
                raise ValueError('bytes for WString must have even length')
        else:
            raise TypeError(
                f'append_str requires str or bytes, got {type(s).__name__}')
        n_units = len(data) // self._ITEMSIZE
        self._check_bound(self._size + n_units)
        self._ensure_capacity(self._size + n_units)
        old_bytes = self._size * self._ITEMSIZE
        new_bytes = old_bytes + len(data)
        self._buffer.resize(new_bytes)
        old_size = self._size
        self._size += n_units
        self._refresh_view()
        self._view[old_size:] = np.frombuffer(data, dtype=self._DTYPE.numpy_dtype)

    def _check_bound(self, needed: int) -> None:
        pass

    # ------------------------------------------------------------------
    # MutableSequence protocol (code-unit level)
    # ------------------------------------------------------------------

    def __len__(self) -> int:
        return self._size

    def __getitem__(self, index: int | slice) -> Any:
        if isinstance(index, int):
            if index < 0:
                index += self._size
            if not (0 <= index < self._size):
                raise IndexError('wstring index out of range')
            return int(self._view[index])
        return list(self._view[index])

    def __setitem__(self, index: int | slice, value: Any) -> None:
        if isinstance(index, int):
            if index < 0:
                index += self._size
            if not (0 <= index < self._size):
                raise IndexError('wstring index out of range')
        self._view[index] = value

    def __delitem__(self, index: int | slice) -> None:
        arr = np.array(self._view, copy=True)
        if isinstance(index, int):
            if index < 0:
                index += self._size
            if not (0 <= index < self._size):
                raise IndexError('wstring index out of range')
            new_arr = np.concatenate([arr[:index], arr[index + 1:]])
        else:
            keep = np.ones(self._size, dtype=bool)
            for i in range(*index.indices(self._size)):
                keep[i] = False
            new_arr = arr[keep]
        n = len(new_arr)
        self._buffer.resize(n * self._ITEMSIZE)
        self._size = n
        self._refresh_view()
        if n:
            self._view[:] = new_arr

    def __iter__(self) -> Iterator[int]:
        for i in range(self._size):
            yield int(self._view[i])

    def __contains__(self, value: object) -> bool:
        return bool(np.any(self._view == value))

    def __reversed__(self) -> Iterator[int]:
        for i in range(self._size - 1, -1, -1):
            yield int(self._view[i])

    def __eq__(self, other: object) -> bool:
        if isinstance(other, WString):
            if self._size != other._size:
                return False
            return bool(np.array_equal(self._view, other._view))
        if isinstance(other, str):
            try:
                return self.__str__() == other
            except (UnicodeDecodeError, ValueError):
                return False
        return NotImplemented  # type: ignore[return-value]

    def append(self, unit_value: int) -> None:
        """Append a single code unit value (int 0–65535)."""
        self._check_bound(self._size + 1)
        self._ensure_capacity(self._size + 1)
        n_bytes = (self._size + 1) * self._ITEMSIZE
        self._buffer.resize(n_bytes)
        self._size += 1
        self._refresh_view()
        self._view[self._size - 1] = unit_value

    def insert(self, index: int, unit_value: int) -> None:
        """Insert *unit_value* before *index*."""
        arr = np.array(self._view, copy=True)
        if index < 0:
            index = max(0, self._size + index)
        if index >= self._size:
            self.append(unit_value)
            return
        self._check_bound(self._size + 1)
        self._ensure_capacity(self._size + 1)
        new_arr = np.concatenate([arr[:index], [unit_value], arr[index:]])
        self._buffer.resize(len(new_arr) * self._ITEMSIZE)
        self._size = len(new_arr)
        self._refresh_view()
        self._view[:] = new_arr.astype(self._DTYPE.numpy_dtype)

    def clear(self) -> None:
        """Remove all code units (buffer capacity is preserved)."""
        self._buffer.resize(0)
        self._size = 0
        self._refresh_view()

    # ------------------------------------------------------------------
    # String conversion
    # ------------------------------------------------------------------

    def __str__(self) -> str:
        return self._view.tobytes().decode('utf-16-le')

    def __repr__(self) -> str:
        return f'{type(self).__name__}({self.__str__()!r})'

    # ------------------------------------------------------------------
    # Buffer / numpy
    # ------------------------------------------------------------------

    def __buffer__(self, flags: int) -> memoryview:
        """PEP 688: writable buffer over the live bytes."""
        return memoryview(self._buffer)

    def numpy(self) -> npt.NDArray[np.uint16]:
        """Return a 1-D uint16 numpy array over the live code units."""
        return self._view


class BoundedString(String):
    """
    A :class:`String` with an upper bound on its byte length.

    All mutations that would exceed *upper_bound* bytes raise
    :exc:`ValueError` before any storage is modified.
    """

    __slots__ = ('_upper_bound',)

    def __init__(
        self,
        upper_bound: int,
        data: str | bytes | bytearray | memoryview | None = None,
        *,
        buffer: RawBuffer | None = None,
    ) -> None:
        if not isinstance(upper_bound, int) or upper_bound <= 0:
            raise ValueError(
                f'upper_bound must be a positive integer, got {upper_bound!r}')
        self._upper_bound = upper_bound
        super().__init__(data, buffer=buffer)

    @property
    def max_size(self) -> int:
        """int: maximum byte length."""
        return self._upper_bound

    def _check_bound(self, needed: int) -> None:
        if needed > self._upper_bound:
            raise ValueError(
                f'BoundedString upper bound {self._upper_bound} B '
                f'exceeded (needed {needed} B)')

    def __repr__(self) -> str:
        return f'BoundedString({self._upper_bound}, {self.__str__()!r})'


class BoundedWString(WString):
    """
    A :class:`WString` with an upper bound on its code-unit count.

    All mutations that would exceed *upper_bound* code units raise
    :exc:`ValueError` before any storage is modified.
    """

    __slots__ = ('_upper_bound',)

    def __init__(
        self,
        upper_bound: int,
        data: str | bytes | bytearray | memoryview | None = None,
        *,
        buffer: RawBuffer | None = None,
    ) -> None:
        if not isinstance(upper_bound, int) or upper_bound <= 0:
            raise ValueError(
                f'upper_bound must be a positive integer, got {upper_bound!r}')
        self._upper_bound = upper_bound
        super().__init__(data, buffer=buffer)

    @property
    def max_size(self) -> int:
        """int: maximum code-unit length."""
        return self._upper_bound

    def _check_bound(self, needed: int) -> None:
        if needed > self._upper_bound:
            raise ValueError(
                f'BoundedWString upper bound {self._upper_bound} code units '
                f'exceeded (needed {needed})')

    def __repr__(self) -> str:
        return f'BoundedWString({self._upper_bound}, {self.__str__()!r})'
