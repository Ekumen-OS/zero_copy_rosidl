# Copyright 2026 Ekumen Inc.
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

"""Generator for XCDR-based CPython typesupport."""

from rosidl_pycommon import generate_files


def generate_cpython(generator_arguments_file):
    """Generate XCDR CPython typesupport code.

    Only experimental message entries are emitted: the XCDR CPython
    typesupport describes the experimental Python message classes (containers
    from rosidl_runtime_cpython).  Standard messages keep resolving through
    the C typesupport, whose dispatch is unchanged.
    """
    mapping = {
        'idl__experimental_rosidl_typesupport_xcdr_cpython.hpp.em':
        'experimental/detail/%s__rosidl_typesupport_xcdr_cpython.hpp',
        'idl__experimental_type_support.cpp.em':
        'experimental/detail/xcdr/%s__type_support.cpp',
    }
    return generate_files(generator_arguments_file, mapping)
