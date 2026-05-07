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

#ifndef XCDR_BUFFERS__COMMON__TYPES_HPP_
#define XCDR_BUFFERS__COMMON__TYPES_HPP_

#include <cstddef>
#include <cstdint>

namespace xcdr_buffers
{

/**
 * @brief Enumeration of CDR primitive type kinds.
 *
 * These are fixed-size types that can be directly serialized.
 */
enum class XCdrPrimitiveKind : uint8_t
{
  // 1 byte
  kBool = 0,
  kInt8,
  kUint8,
  kChar,

  // 2 bytes
  kInt16,
  kUint16,
  kWchar,

  // 4 bytes
  kInt32,
  kUint32,
  kFloat,

  // 8 bytes
  kInt64,
  kUint64,
  kDouble,

  // 16 bytes
  kLongDouble
};

/**
 * @brief Get size in bytes for a primitive type.
 *
 * @param kind The primitive type kind
 * @return Size in bytes
 */
inline size_t get_primitive_size(XCdrPrimitiveKind kind)
{
  switch (kind) {
    case XCdrPrimitiveKind::kBool:
    case XCdrPrimitiveKind::kInt8:
    case XCdrPrimitiveKind::kUint8:
    case XCdrPrimitiveKind::kChar:
      return 1;

    case XCdrPrimitiveKind::kInt16:
    case XCdrPrimitiveKind::kUint16:
    case XCdrPrimitiveKind::kWchar:
      return 2;

    case XCdrPrimitiveKind::kInt32:
    case XCdrPrimitiveKind::kUint32:
    case XCdrPrimitiveKind::kFloat:
      return 4;

    case XCdrPrimitiveKind::kInt64:
    case XCdrPrimitiveKind::kUint64:
    case XCdrPrimitiveKind::kDouble:
      return 8;

    case XCdrPrimitiveKind::kLongDouble:
      return 16;
  }
  return 0;  // Should never reach here
}

/**
 * @brief Get alignment requirement for a primitive type.
 *
 * @param kind The primitive type kind
 * @return Alignment in bytes
 */
inline size_t get_primitive_alignment(XCdrPrimitiveKind kind)
{
  // For XCDR, alignment equals size
  return get_primitive_size(kind);
}

/**
 * @brief Type trait to map C++ types to XCdrPrimitiveKind.
 *
 * Usage: primitive_kind_v<uint32_t> == XCdrPrimitiveKind::kUint32
 */
template<typename T>
struct primitive_kind;

template<>
struct primitive_kind<bool>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kBool;
};
template<>
struct primitive_kind<int8_t>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kInt8;
};
template<>
struct primitive_kind<uint8_t>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kUint8;
};
template<>
struct primitive_kind<char>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kChar;
};
template<>
struct primitive_kind<int16_t>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kInt16;
};
template<>
struct primitive_kind<uint16_t>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kUint16;
};
template<>
struct primitive_kind<wchar_t>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kWchar;
};
template<>
struct primitive_kind<int32_t>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kInt32;
};
template<>
struct primitive_kind<uint32_t>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kUint32;
};
template<>
struct primitive_kind<float>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kFloat;
};
template<>
struct primitive_kind<int64_t>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kInt64;
};
template<>
struct primitive_kind<uint64_t>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kUint64;
};
template<>
struct primitive_kind<double>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kDouble;
};
template<>
struct primitive_kind<long double>
{
  static constexpr XCdrPrimitiveKind value = XCdrPrimitiveKind::kLongDouble;
};

/// Helper variable template
template<typename T>
inline constexpr XCdrPrimitiveKind primitive_kind_v = primitive_kind<T>::value;

/**
 * @brief Enumeration of character kinds for string types.
 *
 * Distinguishes between regular strings (char/char8_t) and wide strings (char16_t).
 */
enum class XCdrCharKind : uint8_t
{
  kChar8 = 0,   // Regular string (char/char8_t), 1 byte per character
  kChar16,      // Wide string (char16_t), 2 bytes per character
};

/**
 * @brief Get size in bytes for a character type.
 *
 * @param kind The character kind
 * @return Size in bytes per character
 */
inline size_t get_char_size(XCdrCharKind kind)
{
  switch (kind) {
    case XCdrCharKind::kChar8:
      return 1;
    case XCdrCharKind::kChar16:
      return 2;
  }
  return 0;  // Should never reach here
}

// XCDR format constants
///< Size of sequence length prefix in bytes
inline constexpr size_t kSequenceLengthPrefixSize = 4;
///< Size of string length prefix in bytes
inline constexpr size_t kStringLengthPrefixSize = 4;
///< Size of string null terminator in bytes (char8)
inline constexpr size_t kStringNullTerminatorSize = 1;
///< Size of wstring null terminator in bytes (char16)
inline constexpr size_t kWStringNullTerminatorSize = 2;

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__COMMON__TYPES_HPP_
