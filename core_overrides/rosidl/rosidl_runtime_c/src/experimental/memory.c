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

#include "rosidl_runtime_c/experimental/memory.h"

bool rosidl_memory_is_valid(const rosidl_memory_t * memory)
{
  return memory != NULL && memory->address != NULL;
}

bool rosidl_memory_region_is_valid(const rosidl_memory_region_t * region)
{
  return region != NULL && rosidl_memory_is_valid(&region->location) && region->size > 0U;
}
