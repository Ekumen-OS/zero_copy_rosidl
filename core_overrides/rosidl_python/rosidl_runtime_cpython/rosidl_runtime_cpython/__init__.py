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

"""
Experimental CPython-native container types for rosidl_runtime_py.

These types mirror the ``rosidl_runtime_cpp::experimental`` quartet —
``Scalar``, ``Array``, ``Sequence``/``BoundedSequence``, and
``String``/``WString``/``BoundedString``/``BoundedWString`` — as pure Python
classes backed by a thin C extension (:mod:`_rosidl_runtime_py`) that provides
the :class:`RawBuffer` type.

Key design properties
---------------------
* **Zero-copy numpy interop**: all containers implement both PEP 3118
  (``__buffer__``, Python >= 3.12) and a ``.numpy()`` method returning a
  live ``numpy.ndarray`` view over the backing :class:`RawBuffer`.

* **External memory from C/C++**: containers accept a :class:`RawBuffer`
  keyword argument.  C/C++ code creates external ``RawBuffer`` objects via
  ``RawBuffer_FromRegion()`` (see ``include/rosidl_runtime_py/raw_buffer.h``).

* **Safe resize**: ``Sequence.append()`` / ``resize()`` replace the backing
  :class:`RawBuffer` with a fresh block when growth is needed; earlier
  ``numpy`` views remain valid (pointing to the old block) until GC-collected.

* **Strict external capacity**: sequences backed by external buffers raise
  :exc:`BufferError` rather than silently promoting to managed memory.

Types
-----
.. autosummary::
   :nosignatures:

   Dtype
   Scalar
   Array
   Sequence
   BoundedSequence
   String
   WString
   BoundedString
   BoundedWString
   RawBuffer
"""

import collections.abc

from rosidl_runtime_cpython._raw_buffer import RawBuffer

from rosidl_runtime_cpython.constraints import (
    SequenceConstraint,
    StringConstraint,
)
from rosidl_runtime_cpython.dtype import Dtype
from rosidl_runtime_cpython.message_initialization import MessageInitialization
from rosidl_runtime_cpython.scalar import Scalar
from rosidl_runtime_cpython.array import Array
from rosidl_runtime_cpython.sequence import Sequence, BoundedSequence
from rosidl_runtime_cpython.string import (
    String,
    WString,
    BoundedString,
    BoundedWString,
)

# ---------------------------------------------------------------------------
# Register abstract base classes so isinstance checks work throughout the
# Python ecosystem without re-implementing mixin methods in each class.
# ---------------------------------------------------------------------------

# Array: fixed-length, immutable-length sequence (read-write elements).
collections.abc.Sequence.register(Array)

# Variable-length sequences: MutableSequence gives free implementations of
# __contains__, __iter__, __reversed__, index(), count(), and the rest of
# the MutableSequence interface on top of the primitives defined in each class.
collections.abc.MutableSequence.register(Sequence)
collections.abc.MutableSequence.register(BoundedSequence)
collections.abc.MutableSequence.register(String)
collections.abc.MutableSequence.register(WString)
collections.abc.MutableSequence.register(BoundedString)
collections.abc.MutableSequence.register(BoundedWString)

__all__ = [
    'Dtype',
    'MessageInitialization',
    'RawBuffer',
    'Scalar',
    'Array',
    'Sequence',
    'BoundedSequence',
    'SequenceConstraint',
    'String',
    'StringConstraint',
    'WString',
    'BoundedString',
    'BoundedWString',
]
