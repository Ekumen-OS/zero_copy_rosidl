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

#include <string.h>

#include "rcutils/error_handling.h"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/inner_value.h"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/unbounded_message.h"
#include "rosidl_typesupport_xcdr_cpp_tests/msg/views/variable_containers.h"
#include "rosidl_typesupport_xcdr_cpp_tests/srv/views/view_fields.h"

typedef rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView VCView;
typedef rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__ConstRegionView VCConst;
typedef rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionConstraints VCLimits;

#define REGION_CONSTRAINT(type, name) \
  rosidl_typesupport_xcdr_cpp_tests__msg__ ## type ## __RegionConstraints__ ## name
#define REGION_VIEW(type, name) \
  rosidl_typesupport_xcdr_cpp_tests__msg__ ## type ## __RegionView__ ## name

VCLimits * test_c_region_limits(void)
{
  VCLimits * limits =
    rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionConstraints__create();
  if (!limits) {
    return NULL;
  }
  rosidl_typesupport_xcdr_cpp_tests__msg__UnboundedMessage__RegionConstraints * nested =
    REGION_CONSTRAINT(VariableContainers, nested_variable__get)(limits);
  if (!nested ||
    !REGION_CONSTRAINT(UnboundedMessage, name__set_string_size)(nested, 4) ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__UnboundedMessage__RegionConstraints__data__set_size(
      nested, 2))
  {
    rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionConstraints__destroy(limits);
    return NULL;
  }
  return limits;
}

bool test_c_region_access(void * loan)
{
  VCView * view =
    rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__from_loan(loan);
  const VCConst * read =
    rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__as_const(view);
  const uint16_t wide[] = {0x61, 0x20ac};
  uint16_t copied[2] = {0};
  const char * text = NULL;
  size_t size = 0;
  uint32_t tail = 0;
  if (!view || !read ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__tail__set(view, 42) ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__labels__set(
      view, 0, "label", 5) ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__names__resize(
      view, 1) ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__names__set(
      view, 0, "name", 4) ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__wide__set(
      view, wide, 2) ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__items__resize(
      view, 1) ||
    !REGION_VIEW(VariableContainers, nested_variable__resize)(view, 1))
  {
    return false;
  }
  rosidl_typesupport_xcdr_cpp_tests__msg__InnerValue__RegionView * item =
    rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__items__get(view, 0);
  if (!item || !rosidl_typesupport_xcdr_cpp_tests__msg__InnerValue__RegionView__id__set(item, 17) ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__ConstRegionView__tail__get(
      read, &tail) || tail != 42 ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__ConstRegionView__names__size(
      read, &size) || size != 1 ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__ConstRegionView__names__get(
      read, 0, &text, &size) || size != 4 || memcmp(text, "name", 4) ||
    !rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__ConstRegionView__wide__copy(
      read, copied, 2, &size) || size != 2 || memcmp(copied, wide, sizeof(wide)))
  {
    return false;
  }
  if (rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__names__set(
      view, 2, "bad", 3) || !rcutils_error_is_set())
  {
    return false;
  }
  rcutils_reset_error();
  rosidl_typesupport_xcdr_cpp_tests__msg__UnboundedMessage__RegionView * nested =
    rosidl_typesupport_xcdr_cpp_tests__msg__VariableContainers__RegionView__nested_variable__get(
    view, 0);
  if (!nested ||
    rosidl_typesupport_xcdr_cpp_tests__msg__UnboundedMessage__RegionView__name__set(
      nested, "exceeds", 7) || !rcutils_error_is_set())
  {
    return false;
  }
  rcutils_reset_error();
  if (!rosidl_typesupport_xcdr_cpp_tests__msg__UnboundedMessage__RegionView__name__set(
      nested, "okay", 4))
  {
    return false;
  }
  return rosidl_typesupport_xcdr_cpp_tests__srv__ViewFields__RegionView__get_type_support() != NULL;
}

#undef REGION_CONSTRAINT
#undef REGION_VIEW
