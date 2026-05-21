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

"""Constraint types for experimental messages.

These mirror the ``rosidl_runtime_cpp::StringConstraint`` and
``rosidl_runtime_cpp::SequenceConstraint<T>`` types from the C++ experimental
runtime, providing runtime-queryable bounds for variable-length message
members.
"""

from __future__ import annotations

import dataclasses
from typing import Any


@dataclasses.dataclass
class StringConstraint:
    """Constraint for an unbounded string member.

    Carries the maximum character/code-unit length for the string.
    Bounded strings carry their limit in the type itself, so no
    constraint is generated for them.
    """

    size: int = 0

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, StringConstraint):
            return NotImplemented
        return self.size == other.size

    def __repr__(self) -> str:
        return f'StringConstraint(size={self.size})'


@dataclasses.dataclass
class SequenceConstraint:
    """Constraint for an unbounded sequence member.

    ``size`` is the maximum number of elements in the sequence.
    ``element`` optionally carries per-element constraints:

    - For sequences of unbounded strings: a :class:`StringConstraint`.
    - For sequences of message types: the message's ``Constraints`` class
      instance.
    - For sequences of scalars or bounded types: ``None``.
    """

    size: int = 0
    element: Any = None

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, SequenceConstraint):
            return NotImplemented
        return self.size == other.size and self.element == other.element

    def __repr__(self) -> str:
        if self.element is not None:
            return f'SequenceConstraint(size={self.size}, element={self.element!r})'
        return f'SequenceConstraint(size={self.size})'
