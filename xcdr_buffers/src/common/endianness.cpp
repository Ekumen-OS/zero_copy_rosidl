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

#include "xcdr_buffers/common/endianness.hpp"

namespace xcdr_buffers
{

const XCdrEndianness kSystemEndianness = []{
  // Detect endianness at runtime
    static const uint32_t test_value = 0x01020304;
    static const uint8_t * test_bytes =
      reinterpret_cast<const uint8_t *>(&test_value);

    if (test_bytes[0] == 0x01) {
      return XCdrEndianness::kBigEndian;
    } else {
      return XCdrEndianness::kLittleEndian;
    }
  }();

}  // namespace xcdr_buffers
