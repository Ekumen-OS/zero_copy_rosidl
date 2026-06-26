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

#ifndef XCDR_BUFFERS__COMMON__RESULT_HPP_
#define XCDR_BUFFERS__COMMON__RESULT_HPP_

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <tl/expected.hpp>

namespace xcdr_buffers
{

/**
 * @brief Error type for xcdr_buffers operations.
 *
 * Contains an error message with detailed context.
 */
class XCdrError : public std::exception
{
public:
  explicit XCdrError(std::string_view message)
  : message_(message) {}

  std::string_view message() const noexcept {return message_;}

  const char * what() const noexcept override {return message_.c_str();}

private:
  std::string message_;
};

/**
 * @brief Result type for operations that don't return a value.
 *
 * Uses tl::expected<void, XCdrError> for exception-free error handling.
 */
using XCdrStatus = tl::expected<void, XCdrError>;

/**
 * @brief Result type for operations that return a value.
 *
 * Uses tl::expected<T, XCdrError> for exception-free error handling.
 *
 * @tparam T The type of the value on success
 */
template<typename T>
using XCdrResult = tl::expected<T, XCdrError>;

/**
 * @brief Helper to create a success status.
 */
inline XCdrStatus ok()
{
  return {};  // default-constructed tl::expected<void, E> is success
}

/**
 * @brief Helper to create a success result with a value.
 */
template<typename T>
inline XCdrResult<T> ok(T value)
{
  return XCdrResult<T>{std::move(value)};
}

/**
 * @brief Helper to create an error status.
 */
inline tl::unexpected<XCdrError> error(std::string_view message)
{
  return tl::unexpected<XCdrError>{XCdrError{message}};
}

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__COMMON__RESULT_HPP_
