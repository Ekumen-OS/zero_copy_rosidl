@# generated from rosidl_typesupport_xcdr_cpp/resource/msg__rosidl_typesupport_xcdr_cpp.hpp.em
@{
from rosidl_pycommon import convert_camel_case_to_lower_case_underscore

# Get optional force_experimental flag (set by experimental wrapper templates).
try:
    force_experimental
except NameError:
    force_experimental = False

# For C symbol names, use a single token (e.g., 'msg_experimental') to
# avoid breaking the 4-argument ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME macro.
interface_parent_parts = list(interface_path.parents[0].parts)
if force_experimental:
    interface_parent_parts = [interface_parent_parts[0] + '_experimental']

include_parts = [package_name] + list(interface_path.parents[0].parts) + [
    'detail', convert_camel_case_to_lower_case_underscore(interface_path.stem)]
# For experimental messages, insert 'experimental' before 'detail'
if force_experimental and 'experimental' not in include_parts and 'detail' in include_parts:
    include_parts.insert(include_parts.index('detail'), 'experimental')
header_guard_body = '__'.join([x.upper() for x in include_parts + ['rosidl_typesupport_xcdr_cpp']]) + '_HPP_'
}@
// Copyright 2026 Ekumen Inc.
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

#ifndef @(header_guard_body)
#define @(header_guard_body)

#ifdef __cplusplus
extern "C"
{
#endif

#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "rosidl_typesupport_xcdr_cpp/visibility_control.h"

ROSIDL_TYPESUPPORT_XCDR_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_xcdr_cpp,
  @(', '.join([package_name] + interface_parent_parts)),
  @(message.structure.namespaced_type.name))();

#ifdef __cplusplus
}
#endif

#endif  // @(header_guard_body)
