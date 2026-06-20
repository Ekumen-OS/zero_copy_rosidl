@# generated from rosidl_typesupport_xcdr_cpp/resource/msg__rosidl_typesupport_xcdr_cpp.hpp.em
@# generated code does not contain a copyright notice
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
header_guard_parts = include_parts + ['rosidl_typesupport_xcdr_cpp', 'hpp']
header_guard_variable = '__'.join([x.upper() for x in header_guard_parts]) + '_'
}@

#ifndef @(header_guard_variable)
#define @(header_guard_variable)

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

#endif  // @(header_guard_variable)
