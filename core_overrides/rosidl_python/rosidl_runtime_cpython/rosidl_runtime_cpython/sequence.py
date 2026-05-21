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

"""Experimental Sequence and BoundedSequence types for rosidl_runtime_py."""

from __future__ import annotations

from collections.abc import Iterable, Iterator
from typing import Any

import numpy as np
import numpy.typing as npt

from rosidl_runtime_cpython.dtype import Dtype
from rosidl_runtime_cpython._raw_buffer import RawBuffer


class Sequence:
    """
    A variable-length, homogeneous sequence of ROS interface values.

    Two storage modes are selected automatically based on *dtype*:

    **Primitive** (``dtype`` is a :class:`Dtype`)
        Elements are stored contiguously in a :class:`RawBuffer` with
        geometric growth.  Structural mutations may replace the internal
        buffer; prior numpy views remain valid (though stale) until GC.
        Zero-copy interoperability is available via :meth:`numpy` and
        ``__buffer__``.

    **Object** (``dtype`` is a Python type or ``None``)
        Elements are stored in a numpy object array (``dtype=object``).
        A capacity-sized backing array ``_store`` (analogous to the
        primitive ``_buffer``) is managed with geometric growth; ``_view``
        always holds the logical slice ``_store[:_size]``.  :meth:`numpy`
        returns the current logical view.

    Construction
    ------------
    ``Sequence(dtype)``
        Empty sequence with managed storage.

    ``Sequence(dtype, buffer=buf)``
        Use *buf* as backing storage (primitive mode only).  Capacity is
        ``buf.size // dtype.itemsize``.  Mutations that exceed it raise
        :exc:`BufferError` for external (non-owning) buffers.

    Notes
    -----
    The sequence does NOT silently promote from external to managed storage.
    Overflow on an external buffer raises :exc:`BufferError`.
    """

    __slots__ = ('_dtype', '_is_primitive', '_buffer', '_store', '_view', '_size')

    def __init__(
        self,
        dtype: Dtype | type[Any],
        *,
        data: Any = None,
        buffer: RawBuffer | None = None,
    ) -> None:
        self._dtype = dtype
        self._is_primitive = isinstance(dtype, Dtype)
        self._size = 0

        if self._is_primitive:
            if buffer is not None:
                if not isinstance(buffer, RawBuffer):
                    raise TypeError(
                        f'buffer must be a RawBuffer, got {type(buffer).__name__}')
                self._buffer: RawBuffer | None = buffer
            else:
                self._buffer = RawBuffer()
            self._store: npt.NDArray[Any] | None = None
            self._view: npt.NDArray[Any] = self._make_view()
        else:
            if buffer is not None:
                raise TypeError(
                    'buffer= is only supported for Dtype (primitive) sequences')
            self._buffer = None
            # Capacity-sized numpy object array: stores PyObject* pointers.
            self._store = np.empty(0, dtype=object)
            self._view = self._make_view()

        if data is not None:
            self.extend(data)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _make_view(self) -> npt.NDArray[Any]:
        if self._is_primitive:
            if self._size == 0:
                return np.empty(0, dtype=self._dtype.numpy_dtype)
            return np.frombuffer(
                self._buffer, dtype=self._dtype.numpy_dtype, count=self._size)
        return self._store[:self._size]  # view of the object store

    def _capacity(self) -> int:
        if self._is_primitive:
            return self._buffer.size // self._dtype.itemsize
        return len(self._store)

    def _ensure_capacity(self, needed: int) -> None:
        """Grow backing storage to hold at least *needed* elements."""
        if needed <= self._capacity():
            return
        if self._is_primitive:
            if not self._buffer.is_owner:
                raise BufferError(
                    f'Sequence capacity {self._capacity()} exceeded for fixed '
                    f'external buffer (needed {needed})')
            new_cap = max(1, self._capacity())
            while new_cap < needed:
                new_cap *= 2
            self._buffer.reserve(new_cap * self._dtype.itemsize)
        else:
            new_cap = max(4, len(self._store))
            while new_cap < needed:
                new_cap *= 2
            new_store: npt.NDArray[Any] = np.empty(new_cap, dtype=object)
            new_store[:self._size] = self._store[:self._size]
            self._store = new_store

    def _refresh_view(self) -> None:
        self._view = self._make_view()

    # ------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------

    @property
    def dtype(self) -> Dtype | type[Any]:
        """The element type (a :class:`Dtype` or a Python type)."""
        return self._dtype

    @property
    def capacity(self) -> int:
        """int: element capacity without reallocation."""
        return self._capacity()

    # ------------------------------------------------------------------
    # MutableSequence protocol
    # ------------------------------------------------------------------

    def __len__(self) -> int:
        return self._size

    def __getitem__(self, index: int | slice) -> Any:
        if isinstance(index, int):
            if index < 0:
                index += self._size
            if not (0 <= index < self._size):
                raise IndexError('sequence index out of range')
            if self._is_primitive:
                return self._view[index].item()
            return self._store[index]
        if self._is_primitive:
            return list(self._view[index])
        return list(self._view[index])  # view[slice] → object ndarray → list

    def __setitem__(self, index: int | slice, value: Any) -> None:
        if isinstance(index, int):
            if index < 0:
                index += self._size
            if not (0 <= index < self._size):
                raise IndexError('sequence index out of range')
        if self._is_primitive:
            self._view[index] = value
        else:
            if isinstance(index, int):
                self._store[index] = value
            else:
                self._view[index] = value

    def __delitem__(self, index: int | slice) -> None:
        if isinstance(index, int):
            if index < 0:
                index += self._size
            if not (0 <= index < self._size):
                raise IndexError('sequence index out of range')
            if self._is_primitive:
                self._view[index:-1] = self._view[index + 1:]
                self._size -= 1
                self._buffer.resize(self._size * self._dtype.itemsize)
            else:
                self._store[index:self._size - 1] = self._store[index + 1:self._size]
                self._store[self._size - 1] = None  # release the dangling ref
                self._size -= 1
            self._refresh_view()
        else:
            if self._is_primitive:
                for i in sorted(range(*index.indices(self._size)), reverse=True):
                    del self[i]
            else:
                keep = np.ones(self._size, dtype=bool)
                for i in range(*index.indices(self._size)):
                    keep[i] = False
                surviving = np.array(self._store[:self._size][keep], copy=True)
                n = len(surviving)
                self._store[:n] = surviving
                self._store[n:self._size] = None  # release dangling refs
                self._size = n
                self._refresh_view()

    def __iter__(self) -> Iterator[Any]:
        if self._is_primitive:
            for i in range(self._size):
                yield self._view[i].item()
        else:
            for i in range(self._size):
                yield self._store[i]

    def __contains__(self, value: object) -> bool:
        if self._is_primitive:
            return bool(np.any(self._view == value))
        # Object mode: use identity-then-equality for correct Python semantics.
        for i in range(self._size):
            elem = self._store[i]
            if elem is value or elem == value:
                return True
        return False

    def __reversed__(self) -> Iterator[Any]:
        if self._is_primitive:
            for i in range(self._size - 1, -1, -1):
                yield self._view[i].item()
        else:
            for i in range(self._size - 1, -1, -1):
                yield self._store[i]

    def __eq__(self, other: object) -> bool:
        if isinstance(other, Sequence):
            if self._dtype != other._dtype or self._size != other._size:
                return False
            return bool(np.array_equal(self._view, other._view))
        try:
            return bool(np.array_equal(self._view, other))
        except (TypeError, ValueError):
            return NotImplemented  # type: ignore[return-value]

    # ------------------------------------------------------------------
    # Bound hook (overridden by BoundedSequence)
    # ------------------------------------------------------------------

    def _check_bound(self, needed: int) -> None:
        pass

    # ------------------------------------------------------------------
    # Mutation
    # ------------------------------------------------------------------

    def append(self, value: Any) -> None:
        """Append a single element."""
        self._check_bound(self._size + 1)
        if self._is_primitive:
            self._ensure_capacity(self._size + 1)
            self._buffer.resize((self._size + 1) * self._dtype.itemsize)
            self._size += 1
            self._refresh_view()
            self._view[self._size - 1] = value
        else:
            self._ensure_capacity(self._size + 1)
            self._store[self._size] = value
            self._size += 1
            self._refresh_view()

    def extend(self, values: Iterable[Any]) -> None:
        """Append all elements from *values*."""
        items = list(values)
        self._check_bound(self._size + len(items))
        if self._is_primitive:
            self._ensure_capacity(self._size + len(items))
            new_size = self._size + len(items)
            self._buffer.resize(new_size * self._dtype.itemsize)
            old_size = self._size
            self._size = new_size
            self._refresh_view()
            for i, v in enumerate(items):
                self._view[old_size + i] = v
        else:
            self._ensure_capacity(self._size + len(items))
            for i, v in enumerate(items):
                self._store[self._size + i] = v
            self._size += len(items)
            self._refresh_view()

    def insert(self, index: int, value: Any) -> None:
        """Insert *value* before *index*."""
        if index < 0:
            index = max(0, self._size + index)
        if index >= self._size:
            self.append(value)
            return
        self._check_bound(self._size + 1)
        if self._is_primitive:
            self._ensure_capacity(self._size + 1)
            self._buffer.resize((self._size + 1) * self._dtype.itemsize)
            self._size += 1
            self._refresh_view()
            self._view[index + 1:] = self._view[index:-1].copy()
            self._view[index] = value
        else:
            self._ensure_capacity(self._size + 1)
            # Shift elements right from the end to avoid overwriting.
            self._store[index + 1:self._size + 1] = self._store[index:self._size]
            self._store[index] = value
            self._size += 1
            self._refresh_view()

    def pop(self, index: int = -1) -> Any:
        """Remove and return element at *index*."""
        if self._size == 0:
            raise IndexError('pop from empty sequence')
        if index < 0:
            index += self._size
        if not (0 <= index < self._size):
            raise IndexError('pop index out of range')
        value = self._view[index].item() if self._is_primitive else self._store[index]
        del self[index]
        return value

    def remove(self, value: Any) -> None:
        """Remove the first occurrence of *value*."""
        if self._is_primitive:
            for i in range(self._size):
                if self._view[i].item() == value:
                    del self[i]
                    return
        else:
            for i in range(self._size):
                elem = self._store[i]
                if elem is value or elem == value:
                    del self[i]
                    return
        raise ValueError(f'{value!r} is not in sequence')

    def clear(self) -> None:
        """Remove all elements (capacity is preserved)."""
        if self._is_primitive:
            self._size = 0
            self._buffer.resize(0)
        else:
            self._store[:self._size] = None  # release object references
            self._size = 0
        self._refresh_view()

    def resize(self, new_size: int, fill: Any = 0) -> None:
        """
        Set the logical length to *new_size*.

        New elements are initialised to *fill*.  Raises :exc:`BufferError`
        if the backing buffer is external and *new_size* exceeds its capacity.
        """
        self._check_bound(new_size)
        if self._is_primitive:
            self._ensure_capacity(new_size)
            old_size = self._size
            self._buffer.resize(new_size * self._dtype.itemsize)
            self._size = new_size
            self._refresh_view()
            if new_size > old_size and fill != 0:
                self._view[old_size:] = fill
        else:
            self._ensure_capacity(new_size)
            if new_size > self._size:
                self._store[self._size:new_size] = fill
            elif new_size < self._size:
                self._store[new_size:self._size] = None  # release refs
            self._size = new_size
            self._refresh_view()

    # ------------------------------------------------------------------
    # Buffer / numpy protocols
    # ------------------------------------------------------------------

    def __buffer__(self, flags: int) -> memoryview:
        """PEP 688 buffer protocol (Python >= 3.12, primitive sequences only)."""
        if not self._is_primitive:
            raise TypeError('__buffer__ is not supported for object-typed sequences')
        return memoryview(self._buffer)  # type: ignore[arg-type]

    def numpy(self) -> npt.NDArray[Any]:
        """
        Return a numpy array over the live elements.

        For **primitive** sequences the returned array is a snapshot of the
        current buffer; it becomes stale after any mutation that replaces the
        buffer.

        For **object** sequences the returned array is a view of the internal
        object store slice; it also becomes stale after structural mutations
        that grow the store.
        """
        return self._view

    # ------------------------------------------------------------------
    # Representation
    # ------------------------------------------------------------------

    def __repr__(self) -> str:
        return f'{type(self).__name__}({self._dtype!r}, data={self._view.tolist()!r})'


class BoundedSequence(Sequence):
    """
    A variable-length sequence with an upper bound on its element count.

    All mutations that would exceed *upper_bound* raise :exc:`ValueError`
    before any storage is modified.

    Construction
    ------------
    ``BoundedSequence(dtype, upper_bound)``
        Empty sequence with managed storage, bounded to *upper_bound* elements.

    ``BoundedSequence(dtype, upper_bound, buffer=buf)``
        Use *buf* as backing storage.
    """

    __slots__ = ('_upper_bound',)

    def __init__(
        self,
        dtype: Dtype | type[Any],
        upper_bound: int,
        *,
        data: Any = None,
        buffer: RawBuffer | None = None,
    ) -> None:
        if not isinstance(upper_bound, int) or upper_bound <= 0:
            raise ValueError(
                f'upper_bound must be a positive integer, got {upper_bound!r}')
        self._upper_bound = upper_bound
        super().__init__(dtype, data=data, buffer=buffer)

    @property
    def max_size(self) -> int:
        """int: maximum number of elements this sequence can hold."""
        return self._upper_bound

    def _check_bound(self, needed: int) -> None:
        if needed > self._upper_bound:
            raise ValueError(
                f'BoundedSequence upper bound {self._upper_bound} '
                f'exceeded (needed {needed})')

    def __repr__(self) -> str:
        return (
            f'BoundedSequence({self._dtype!r}, {self._upper_bound}, '
            f'data={self._view.tolist()!r})'
        )
