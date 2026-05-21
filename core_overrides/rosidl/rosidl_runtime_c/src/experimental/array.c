// Copyright 2026 Ekumen, Inc.
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

#include "rosidl_runtime_c/experimental/array.h"

ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__Float__Array, float);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__Double__Array, double);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__LongDouble__Array, long double);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__Char__Array, char);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__WChar__Array, char16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__Boolean__Array, bool);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__UInt8__Array, uint8_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__Int8__Array, int8_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__UInt16__Array, uint16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__Int16__Array, int16_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__UInt32__Array, uint32_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__Int32__Array, int32_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__UInt64__Array, uint64_t);
ROSIDL_RUNTIME_C__EXPERIMENTAL__PRIMITIVE_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__Int64__Array, int64_t);

ROSIDL_RUNTIME_C__EXPERIMENTAL__ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__String);
ROSIDL_RUNTIME_C__EXPERIMENTAL__ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__WString);

ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__BoundedString);
ROSIDL_RUNTIME_C__EXPERIMENTAL__BOUNDED_ELEMENT_ARRAY_STRUCTURE_DEFINE(
  rosidl_runtime_c__experimental__BoundedWString);