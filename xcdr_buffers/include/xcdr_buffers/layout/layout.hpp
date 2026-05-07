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

#ifndef XCDR_BUFFERS__LAYOUT__LAYOUT_HPP_
#define XCDR_BUFFERS__LAYOUT__LAYOUT_HPP_

#include <cstddef>
#include <map>
#include <memory>
#include <memory_resource>
#include <string>
#include <variant>
#include <vector>
#include <utility>

#include <tcb_span/span.hpp>

#include "xcdr_buffers/common/endianness.hpp"
#include "xcdr_buffers/common/result.hpp"
#include "xcdr_buffers/common/types.hpp"

namespace xcdr_buffers
{

// Forward declarations
class XCdrPrimitiveLayout;
class XCdrStringLayout;
class XCdrPrimitiveArrayLayout;
class XCdrArrayLayout;
class XCdrPrimitiveSequenceLayout;
class XCdrSequenceLayout;
class XCdrStructLayout;

// Variant holding any layout type (7 types total)
using XCdrLayout = std::variant<
  XCdrPrimitiveLayout,
  XCdrStringLayout,
  XCdrPrimitiveArrayLayout,
  XCdrArrayLayout,
  XCdrPrimitiveSequenceLayout,
  XCdrSequenceLayout,
  XCdrStructLayout
>;

/**
 * @brief Layout for primitive types (fixed-size, no internal structure).
 */
class XCdrPrimitiveLayout
{
public:
  explicit XCdrPrimitiveLayout(
    XCdrPrimitiveKind kind,
    std::pmr::memory_resource * mr = nullptr);

  XCdrPrimitiveKind kind() const {return kind_;}
  size_t size() const {return get_primitive_size(kind_);}
  size_t alignment() const {return get_primitive_alignment(kind_);}

private:
  XCdrPrimitiveKind kind_;
  std::pmr::memory_resource * memory_resource_;
};

/**
 * @brief Layout for strings (variable-length).
 *
 * Supports both regular strings (char8) and wide strings (char16).
 */
class XCdrStringLayout
{
public:
  /**
   * @param actual_length String length in characters (excluding null terminator)
   * @param char_kind Character kind (char8 or char16)
   * @param mr Memory resource for internal allocations
   */
  explicit XCdrStringLayout(
    size_t actual_length,
    XCdrCharKind char_kind = XCdrCharKind::kChar8,
    std::pmr::memory_resource * mr = nullptr);

  XCdrCharKind char_kind() const {return char_kind_;}
  size_t actual_length() const {return actual_length_;}    // In characters
  size_t size() const;  // Total bytes including prefix and terminator
  size_t alignment() const {return kStringLengthPrefixSize;}

private:
  size_t actual_length_;  // In characters, excludes null terminator
  XCdrCharKind char_kind_;
  std::pmr::memory_resource * memory_resource_;
};

/**
 * @brief Layout for arrays of primitives (homogeneous, contiguous).
 */
class XCdrPrimitiveArrayLayout
{
public:
  XCdrPrimitiveArrayLayout(
    XCdrPrimitiveKind element_kind,
    size_t count,
    std::pmr::memory_resource * mr = nullptr);

  XCdrPrimitiveKind element_kind() const {return element_kind_;}
  size_t count() const {return count_;}
  size_t size() const {return count_ * get_primitive_size(element_kind_);}
  size_t alignment() const {return get_primitive_alignment(element_kind_);}

  size_t element_offset(size_t index) const
  {
    return index * get_primitive_size(element_kind_);
  }

private:
  XCdrPrimitiveKind element_kind_;
  size_t count_;
  std::pmr::memory_resource * memory_resource_;
};

/**
 * @brief Layout for arrays of non-primitives (strings, structs, nested composites).
 *
 * Each element may have different size/internal layout. Stores per-element offsets
 * and layouts for O(1) access.
 */
class XCdrArrayLayout
{
public:
  struct Element
  {
    size_t offset;  // Relative to array start
    std::shared_ptr<XCdrLayout> layout;
  };

  explicit XCdrArrayLayout(
    std::pmr::vector<Element> elements,
    std::pmr::memory_resource * mr = nullptr);

  size_t count() const {return elements_.size();}
  XCdrResult<size_t> element_offset(size_t index) const;
  XCdrResult<std::reference_wrapper<const XCdrLayout>> element_layout(size_t index) const;
  size_t size() const;
  size_t alignment() const;

private:
  std::pmr::vector<Element> elements_;
  std::pmr::memory_resource * memory_resource_;
};

/**
 * @brief Layout for sequences of primitives (homogeneous, contiguous, with length prefix).
 */
class XCdrPrimitiveSequenceLayout
{
public:
  XCdrPrimitiveSequenceLayout(
    XCdrPrimitiveKind element_kind,
    size_t actual_count,
    std::pmr::memory_resource * mr = nullptr);

  XCdrPrimitiveKind element_kind() const {return element_kind_;}
  size_t actual_count() const {return actual_count_;}
  size_t size() const
  {
    // Account for alignment padding after length prefix
    size_t element_size = get_primitive_size(element_kind_);
    size_t element_alignment = get_primitive_alignment(element_kind_);
    size_t first_elem_offset = align_to(kSequenceLengthPrefixSize, element_alignment);
    size_t padding = first_elem_offset - kSequenceLengthPrefixSize;
    return kSequenceLengthPrefixSize + padding + actual_count_ * element_size;
  }
  size_t alignment() const {return kSequenceLengthPrefixSize;}
  size_t data_offset() const {return kSequenceLengthPrefixSize;}

  size_t element_offset(size_t index) const
  {
    // First element is aligned after the length prefix
    size_t element_size = get_primitive_size(element_kind_);
    size_t element_alignment = get_primitive_alignment(element_kind_);
    size_t first_elem_offset = align_to(kSequenceLengthPrefixSize, element_alignment);
    return first_elem_offset + index * element_size;
  }

private:
  XCdrPrimitiveKind element_kind_;
  size_t actual_count_;
  std::pmr::memory_resource * memory_resource_;
};

/**
 * @brief Layout for sequences of non-primitives (strings, structs, nested composites).
 *
 * Each element may have different size/internal layout. Stores per-element offsets
 * and layouts for O(1) access. Includes length prefix.
 */
class XCdrSequenceLayout
{
public:
  struct Element
  {
    size_t offset;  // Relative to sequence data start (after length prefix)
    std::shared_ptr<XCdrLayout> layout;
  };

  explicit XCdrSequenceLayout(
    std::pmr::vector<Element> elements,
    std::pmr::memory_resource * mr = nullptr);

  size_t actual_count() const {return elements_.size();}
  XCdrResult<size_t> element_offset(size_t index) const;  // Relative to data start
  XCdrResult<std::reference_wrapper<const XCdrLayout>> element_layout(size_t index) const;
  size_t size() const;  // Includes length prefix
  size_t alignment() const {return kSequenceLengthPrefixSize;}
  size_t data_offset() const {return kSequenceLengthPrefixSize;}

private:
  std::pmr::vector<Element> elements_;
  std::pmr::memory_resource * memory_resource_;
};

/**
 * @brief Layout for structs (heterogeneous composite with named members).
 */
class XCdrStructLayout
{
public:
  class Member
  {
public:
    Member(std::string name, size_t offset, std::shared_ptr<XCdrLayout> layout)
    : name_(std::move(name)), offset_(offset), layout_(std::move(layout)) {}

    const std::string & name() const {return name_;}
    size_t offset() const {return offset_;}
    const XCdrLayout & layout() const {return *layout_;}

private:
    std::string name_;
    size_t offset_;
    std::shared_ptr<XCdrLayout> layout_;
  };

  // Type alias for const reference to Member
  using MemberConstRef = std::reference_wrapper<const Member>;

  XCdrStructLayout(
    std::pmr::vector<Member> members,
    std::pmr::map<std::string, size_t> name_to_index,
    size_t total_size,
    size_t max_alignment,
    XCdrEndianness endianness,
    bool is_top_level,
    std::pmr::memory_resource * mr = nullptr);

  const std::pmr::vector<Member> & members() const {return members_;}
  XCdrResult<MemberConstRef> get_member(std::string_view name) const;
  XCdrResult<MemberConstRef> get_member(size_t index) const;
  size_t member_count() const {return members_.size();}
  size_t total_size() const {return total_size_;}
  size_t max_alignment() const {return max_alignment_;}
  XCdrEndianness endianness() const {return endianness_;}

  // Convenience wrappers for compatibility with std::visit on XCdrLayout variant
  size_t size() const {return total_size_;}
  size_t alignment() const {return max_alignment_;}

  /**
   * @brief Apply layout to buffer (write XCDR header + length prefixes for variable-length fields).
   *
   * @param buffer Buffer to initialize (must be at least total_size() bytes)
   * @param zero_initialize If true, zero-fill the entire buffer
   * @return Status indicating success or error
   */
  XCdrStatus apply(tcb::span<uint8_t> buffer, bool zero_initialize = false) const;

private:
  std::pmr::vector<Member> members_;
  std::pmr::map<std::string, size_t> name_to_index_;
  size_t total_size_;
  size_t max_alignment_;
  XCdrEndianness endianness_;
  bool is_top_level_;  // Whether this is a top-level struct (has XCDR header in buffer)
  std::pmr::memory_resource * memory_resource_;
};

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__LAYOUT__LAYOUT_HPP_
