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

"""CLI extension for rosidl_typesupport_xcdr_cpp."""

from rosidl_cli.command.generate.extensions import GenerateCommandExtension


class GenerateXcdrCppTypesupport(GenerateCommandExtension):
    """Generate XCDR C++ typesupport code."""

    def generate(
        self,
        package_name,
        interface_files,
        include_paths,
        output_path
    ):
        """Generate typesupport code."""
        from rosidl_typesupport_xcdr_cpp import generate_cpp
        return generate_cpp(
            package_name=package_name,
            interface_files=interface_files,
            include_paths=include_paths,
            output_path=output_path
        )
