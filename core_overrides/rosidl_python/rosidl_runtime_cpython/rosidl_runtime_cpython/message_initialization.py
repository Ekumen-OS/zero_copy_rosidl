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

"""MessageInitialization enum for experimental messages.

Mirrors ``rosidl_runtime_cpp::MessageInitialization`` to control which
fields are set in the ``_reset()`` method of generated experimental
messages.
"""

from __future__ import annotations

import enum


class MessageInitialization(enum.Enum):
    """Control which fields are initialised by ``_reset()``.

    Members
    -------
    ALL
        Apply both default values and zero-initialisation for fields
        that do not have an explicit default.
    DEFAULTS_ONLY
        Only apply fields that carry an explicit ``@default`` annotation.
    ZERO
        Zero-initialise all fields that carry a zero value (including
        fields that also have an explicit default).
    SKIP
        Skip all initialisation.  Useful for sub-messages that will be
        initialised separately.
    """

    ALL = 'ALL'
    DEFAULTS_ONLY = 'DEFAULTS_ONLY'
    ZERO = 'ZERO'
    SKIP = 'SKIP'
