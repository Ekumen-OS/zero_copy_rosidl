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

#ifndef XCDR_BUFFERS__COMMON__UTILITIES_HPP_
#define XCDR_BUFFERS__COMMON__UTILITIES_HPP_

#include <string_view>
#include <utility>

namespace xcdr_buffers
{

/**
 * @brief Partitions a string at the first occurrence of a delimiter.
 *
 * Returns the portion before the delimiter and the portion after.
 * If the delimiter is not found, returns the entire string and an empty string.
 *
 * @param str The string to partition
 * @param delimiter The delimiter character
 * @return Pair of (before_delimiter, after_delimiter)
 *
 * @example
 *   partition("foo.bar.baz", '.') -> {"foo", "bar.baz"}
 *   partition("no_delimiter", '.') -> {"no_delimiter", ""}
 */
inline std::pair<std::string_view, std::string_view> partition(
  std::string_view str, char delimiter)
{
  size_t pos = str.find(delimiter);
  if (pos == std::string_view::npos) {
    return {str, ""};
  }
  return {str.substr(0, pos), str.substr(pos + 1)};
}

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__COMMON__UTILITIES_HPP_
