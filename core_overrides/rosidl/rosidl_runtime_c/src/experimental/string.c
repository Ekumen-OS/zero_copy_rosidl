// Copyright 2026 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rosidl_runtime_c/experimental/string.h"

// Unbounded strings
ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_STRING_DEFINE(
  rosidl_runtime_c__experimental__String, char);
ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_STRING_DEFINE(
  rosidl_runtime_c__experimental__WString, char16_t);

// Bounded strings
ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_BOUNDED_STRING_DEFINE(
  rosidl_runtime_c__experimental__BoundedString, char);
ROSIDL_RUNTIME_C__EXPERIMENTAL__BASIC_BOUNDED_STRING_DEFINE(
  rosidl_runtime_c__experimental__BoundedWString, char16_t);
