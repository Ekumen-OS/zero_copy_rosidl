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

#ifndef XCDR_BUFFERS__COMMON__ENDIANNESS_HPP_
#define XCDR_BUFFERS__COMMON__ENDIANNESS_HPP_

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>

#include <tcb_span/span.hpp>

#include "xcdr_buffers/common/result.hpp"

namespace xcdr_buffers
{

/**
 * @brief Endianness types.
 */
enum class XCdrEndianness : uint8_t
{
  kBigEndian = 0,
  kLittleEndian
};

/**
 * @brief Get the system's native endianness.
 */
extern const XCdrEndianness kSystemEndianness;

/**
 * @brief XCDR encapsulation header size.
 */
constexpr size_t kXCdrHeaderSize = 4;

/**
 * @brief Byte swap for 16-bit values.
 */
inline uint16_t byteswap(uint16_t value)
{
  return (value >> 8) | (value << 8);
}

/**
 * @brief Byte swap for 32-bit values.
 */
inline uint32_t byteswap(uint32_t value)
{
  return ((value >> 24) & 0x000000FFu) |
         ((value >> 8) & 0x0000FF00u) |
         ((value << 8) & 0x00FF0000u) |
         ((value << 24) & 0xFF000000u);
}

/**
 * @brief Byte swap for 64-bit values.
 */
inline uint64_t byteswap(uint64_t value)
{
  return ((value >> 56) & 0x00000000000000FFull) |
         ((value >> 40) & 0x000000000000FF00ull) |
         ((value >> 24) & 0x0000000000FF0000ull) |
         ((value >> 8) & 0x00000000FF000000ull) |
         ((value << 8) & 0x000000FF00000000ull) |
         ((value << 24) & 0x0000FF0000000000ull) |
         ((value << 40) & 0x00FF000000000000ull) |
         ((value << 56) & 0xFF00000000000000ull);
}

/**
 * @brief Calculate the number of padding bytes needed for alignment.
 *
 * Based on Fast-CDR's alignment calculation.
 *
 * @param current_alignment Current position in the buffer
 * @param data_size Size/alignment requirement of the data (must be power of 2)
 * @return Number of padding bytes needed
 */
inline size_t calculate_alignment(size_t current_alignment, size_t data_size)
{
  return (data_size - (current_alignment % data_size)) & (data_size - 1);
}

/**
 * @brief Align a position to the next boundary.
 *
 * @param position Current position
 * @param alignment Alignment requirement (must be power of 2)
 * @return Aligned position
 */
inline size_t align_to(size_t position, size_t alignment)
{
  return position + calculate_alignment(position, alignment);
}

/**
 * @brief Read a value from bytes with endianness handling.
 *
 * @param dst Destination value
 * @param src Source byte span
 * @param endianness Endianness of the source data
 */
template<typename T>
void read_from_bytes(T * dst, tcb::span<const uint8_t> src, XCdrEndianness endianness)
{
  static_assert(std::is_arithmetic_v<T>, "read_from_bytes only supports arithmetic types");
  assert(sizeof(T) <= src.size() &&
      "Source span must be at least as large as the destination type");

  if constexpr (sizeof(T) > 1) {
    if (endianness != kSystemEndianness) {
      if constexpr (sizeof(T) == sizeof(uint16_t)) {
        uint16_t temp;
        std::memcpy(&temp, src.data(), sizeof(uint16_t));
        temp = byteswap(temp);
        std::memcpy(dst, &temp, sizeof(T));
      } else if constexpr (sizeof(T) == sizeof(uint32_t)) {
        uint32_t temp;
        std::memcpy(&temp, src.data(), sizeof(uint32_t));
        temp = byteswap(temp);
        std::memcpy(dst, &temp, sizeof(T));
      } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
        uint64_t temp;
        std::memcpy(&temp, src.data(), sizeof(uint64_t));
        temp = byteswap(temp);
        std::memcpy(dst, &temp, sizeof(T));
      } else {
        // For non-standard sizes, do a generic byte swap
        uint8_t temp[sizeof(T)];
        std::memcpy(temp, src.data(), sizeof(T));
        std::reverse(std::begin(temp), std::end(temp));
        std::memcpy(dst, temp, sizeof(T));
      }
    } else {
      std::memcpy(dst, src.data(), sizeof(T));
    }
  } else {
    std::memcpy(dst, src.data(), sizeof(T));
  }
}

/**
 * @brief Read multiple values from bytes with endianness handling.
 *
 * @param dst Destination span
 * @param src Source byte span
 * @param endianness Endianness of the source data
 */
template<typename T>
void read_from_bytes(tcb::span<T> dst, tcb::span<const uint8_t> src, XCdrEndianness endianness)
{
  static_assert(std::is_arithmetic_v<T>, "read_from_bytes only supports arithmetic types");
  assert(sizeof(T) * dst.size() <= src.size() &&
      "Source span must be large enough to fill the destination span");

  if constexpr (sizeof(T) > 1) {
    if (endianness != kSystemEndianness) {
      for (size_t i = 0; i < dst.size(); ++i) {
        const uint8_t *ptr = src.data() + i * sizeof(T);
        if constexpr (sizeof(T) == sizeof(uint16_t)) {
          uint16_t temp;
          std::memcpy(&temp, ptr, sizeof(uint16_t));
          temp = byteswap(temp);
          std::memcpy(&dst[i], &temp, sizeof(T));
        } else if constexpr (sizeof(T) == sizeof(uint32_t)) {
          uint32_t temp;
          std::memcpy(&temp, ptr, sizeof(uint32_t));
          temp = byteswap(temp);
          std::memcpy(&dst[i], &temp, sizeof(T));
        } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
          uint64_t temp;
          std::memcpy(&temp, ptr, sizeof(uint64_t));
          temp = byteswap(temp);
          std::memcpy(&dst[i], &temp, sizeof(T));
        } else {
          // For non-standard sizes, do a generic byte swap
          uint8_t temp[sizeof(T)];
          std::memcpy(temp, ptr, sizeof(T));
          std::reverse(std::begin(temp), std::end(temp));
          std::memcpy(&dst[i], temp, sizeof(T));
        }
      }
    } else {
      std::memcpy(dst.data(), src.data(), sizeof(T) * dst.size());
    }
  } else {
    std::memcpy(dst.data(), src.data(), dst.size());
  }
}

/**
 * @brief Write a value to bytes with endianness handling.
 *
 * @param dst Destination byte span
 * @param src Source value
 * @param endianness Endianness for the output data
 */
template<typename T>
void write_to_bytes(tcb::span<uint8_t> dst, T src, XCdrEndianness endianness)
{
  static_assert(std::is_arithmetic_v<T>, "write_to_bytes only supports arithmetic types");
  assert(dst.size() >= sizeof(T) &&
      "Destination span must be at least as large as the source type");

  if constexpr (sizeof(T) > 1) {
    if (endianness != kSystemEndianness) {
      if constexpr (sizeof(T) == sizeof(uint16_t)) {
        uint16_t temp;
        std::memcpy(&temp, &src, sizeof(uint16_t));
        temp = byteswap(temp);
        std::memcpy(dst.data(), &temp, sizeof(uint16_t));
      } else if constexpr (sizeof(T) == sizeof(uint32_t)) {
        uint32_t temp;
        std::memcpy(&temp, &src, sizeof(uint32_t));
        temp = byteswap(temp);
        std::memcpy(dst.data(), &temp, sizeof(uint32_t));
      } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
        uint64_t temp;
        std::memcpy(&temp, &src, sizeof(uint64_t));
        temp = byteswap(temp);
        std::memcpy(dst.data(), &temp, sizeof(uint64_t));
      } else {
        // For non-standard sizes, do a generic byte swap
        uint8_t temp[sizeof(T)];
        std::memcpy(temp, &src, sizeof(T));
        std::reverse(std::begin(temp), std::end(temp));
        std::memcpy(dst.data(), temp, sizeof(T));
      }
    } else {
      std::memcpy(dst.data(), &src, sizeof(T));
    }
  } else {
    std::memcpy(dst.data(), &src, sizeof(T));
  }
}

/**
 * @brief Write multiple values to bytes with endianness handling.
 *
 * @param dst Destination byte span
 * @param src Source span
 * @param endianness Endianness for the output data
 */
template<typename T>
void write_to_bytes(tcb::span<uint8_t> dst, tcb::span<const T> src, XCdrEndianness endianness)
{
  static_assert(std::is_arithmetic_v<T>, "write_to_bytes only supports arithmetic types");
  assert(dst.size() >= sizeof(T) * src.size() &&
      "Destination span must be large enough to hold all source elements");

  if constexpr (sizeof(T) > 1) {
    if (endianness != kSystemEndianness) {
      for (size_t i = 0; i < src.size(); ++i) {
        uint8_t *dst_ptr = dst.data() + i * sizeof(T);
        if constexpr (sizeof(T) == sizeof(uint16_t)) {
          uint16_t temp;
          std::memcpy(&temp, &src[i], sizeof(uint16_t));
          temp = byteswap(temp);
          std::memcpy(dst_ptr, &temp, sizeof(uint16_t));
        } else if constexpr (sizeof(T) == sizeof(uint32_t)) {
          uint32_t temp;
          std::memcpy(&temp, &src[i], sizeof(uint32_t));
          temp = byteswap(temp);
          std::memcpy(dst_ptr, &temp, sizeof(uint32_t));
        } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
          uint64_t temp;
          std::memcpy(&temp, &src[i], sizeof(uint64_t));
          temp = byteswap(temp);
          std::memcpy(dst_ptr, &temp, sizeof(uint64_t));
        } else {
          // For non-standard sizes, do a generic byte swap
          uint8_t temp[sizeof(T)];
          std::memcpy(temp, &src[i], sizeof(T));
          std::reverse(std::begin(temp), std::end(temp));
          std::memcpy(dst_ptr, temp, sizeof(T));
        }
      }
    } else {
      std::memcpy(dst.data(), src.data(), sizeof(T) * src.size());
    }
  } else {
    std::memcpy(dst.data(), src.data(), src.size());
  }
}

/**
 * @brief Write XCDR encapsulation header to buffer.
 *
 * Format: [0x00, encoding_flag, 0x00, 0x00]
 * where encoding_flag = 0x01 for little endian, 0x00 for big endian
 *
 * @param buffer Buffer to write to (must be at least 4 bytes)
 * @param endianness Endianness to encode in header
 */
inline void write_xcdr_header(tcb::span<uint8_t> buffer, XCdrEndianness endianness)
{
  assert(buffer.size() >= kXCdrHeaderSize && "Buffer must be at least 4 bytes");
  buffer[0] = 0x00;  // Representation options
  buffer[1] = static_cast<uint8_t>(endianness);  // Encoding flag
  buffer[2] = 0x00;  // Options
  buffer[3] = 0x00;  // Options
}

/**
 * @brief Read and validate XCDR encapsulation header from buffer.
 *
 * @param buffer Buffer to read from (must be at least 4 bytes)
 * @return Result containing endianness or error
 */
inline XCdrResult<XCdrEndianness> read_xcdr_header(tcb::span<const uint8_t> buffer)
{
  if (buffer.size() < kXCdrHeaderSize) {
    return error("Buffer too small for XCDR header");
  }

  // Check representation options (byte 0)
  if (buffer[0] != 0x00) {
    return error("Invalid XCDR header: unexpected representation options");
  }

  // Read endianness from byte 1
  uint8_t encoding_flag = buffer[1];
  if (encoding_flag != 0x00 && encoding_flag != 0x01) {
    return error("Invalid XCDR header: unexpected encoding flag");
  }

  return ok(static_cast<XCdrEndianness>(encoding_flag));
}

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__COMMON__ENDIANNESS_HPP_
