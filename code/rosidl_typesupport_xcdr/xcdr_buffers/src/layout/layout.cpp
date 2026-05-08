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

#include "xcdr_buffers/layout/layout.hpp"

#include <algorithm>
#include <cstring>

namespace xcdr_buffers
{

// ============================================================================
// XCdrPrimitiveLayout
// ============================================================================

XCdrPrimitiveLayout::XCdrPrimitiveLayout(XCdrPrimitiveKind kind, std::pmr::memory_resource * mr)
: kind_(kind), memory_resource_(mr ? mr : std::pmr::new_delete_resource())
{
}

// ============================================================================
// XCdrStringLayout
// ============================================================================

XCdrStringLayout::XCdrStringLayout(
  size_t actual_length, XCdrCharKind char_kind,
  std::pmr::memory_resource * mr)
: actual_length_(actual_length), char_kind_(char_kind),
  memory_resource_(mr ? mr : std::pmr::new_delete_resource())
{
}

size_t XCdrStringLayout::size() const
{
  const size_t char_size = get_char_size(char_kind_);
  const size_t terminator_size = (char_kind_ == XCdrCharKind::kChar8) ?
    kStringNullTerminatorSize :
    kWStringNullTerminatorSize;
  return kStringLengthPrefixSize + (actual_length_ * char_size) + terminator_size;
}

// ============================================================================
// XCdrPrimitiveArrayLayout
// ============================================================================

XCdrPrimitiveArrayLayout::XCdrPrimitiveArrayLayout(
  XCdrPrimitiveKind element_kind,
  size_t count,
  std::pmr::memory_resource * mr)
: element_kind_(element_kind), count_(count),
  memory_resource_(mr ? mr : std::pmr::new_delete_resource())
{
}

// ============================================================================
// XCdrArrayLayout
// ============================================================================

XCdrArrayLayout::XCdrArrayLayout(std::pmr::vector<Element> elements, std::pmr::memory_resource * mr)
: elements_(std::move(elements)), memory_resource_(mr ? mr : std::pmr::new_delete_resource())
{
}

XCdrResult<size_t> XCdrArrayLayout::element_offset(size_t index) const
{
  if (index >= elements_.size()) {
    return error("Array element index " + std::to_string(index) + " out of range");
  }
  return ok(elements_[index].offset);
}

XCdrResult<std::reference_wrapper<const XCdrLayout>> XCdrArrayLayout::element_layout(
  size_t index) const
{
  if (index >= elements_.size()) {
    return error("Array element index " + std::to_string(index) + " out of range");
  }
  return ok(std::cref(*elements_[index].layout));
}

size_t XCdrArrayLayout::size() const
{
  if (elements_.empty()) {
    return 0;
  }
  // Size = last element offset + last element size
  const auto & last = elements_.back();
  size_t last_size = std::visit([](auto && layout) -> size_t {
        return layout.size();
  }, *last.layout);
  return last.offset + last_size;
}

size_t XCdrArrayLayout::alignment() const
{
  if (elements_.empty()) {
    return 1;
  }
  // Max alignment of all elements
  size_t max_align = 1;
  for (const auto & elem : elements_) {
    size_t elem_align = std::visit([](auto && layout) -> size_t {
          return layout.alignment();
    }, *elem.layout);
    max_align = std::max(max_align, elem_align);
  }
  return max_align;
}

// ============================================================================
// XCdrPrimitiveSequenceLayout
// ============================================================================

XCdrPrimitiveSequenceLayout::XCdrPrimitiveSequenceLayout(
  XCdrPrimitiveKind element_kind,
  size_t actual_count,
  std::pmr::memory_resource * mr)
: element_kind_(element_kind), actual_count_(actual_count),
  memory_resource_(mr ? mr : std::pmr::new_delete_resource())
{
}

// ============================================================================
// XCdrSequenceLayout
// ============================================================================

XCdrSequenceLayout::XCdrSequenceLayout(
  std::pmr::vector<Element> elements,
  std::pmr::memory_resource * mr)
: elements_(std::move(elements)), memory_resource_(mr ? mr : std::pmr::new_delete_resource())
{
}

XCdrResult<size_t> XCdrSequenceLayout::element_offset(size_t index) const
{
  if (index >= elements_.size()) {
    return error("Sequence element index " + std::to_string(index) + " out of range");
  }
  return ok(elements_[index].offset);
}

XCdrResult<std::reference_wrapper<const XCdrLayout>> XCdrSequenceLayout::element_layout(
  size_t index) const
{
  if (index >= elements_.size()) {
    return error("Sequence element index " + std::to_string(index) + " out of range");
  }
  return ok(std::cref(*elements_[index].layout));
}

size_t XCdrSequenceLayout::size() const
{
  if (elements_.empty()) {
    return kSequenceLengthPrefixSize;  // Just the length prefix
  }
  const auto & last = elements_.back();
  size_t last_size = std::visit([](auto && layout) -> size_t {
        return layout.size();
  }, *last.layout);
  return kSequenceLengthPrefixSize + last.offset + last_size;
}

// ============================================================================
// XCdrStructLayout
// ============================================================================

XCdrStructLayout::XCdrStructLayout(
  std::pmr::vector<Member> members,
  std::pmr::map<std::string, size_t> name_to_index,
  size_t total_size,
  size_t max_alignment,
  XCdrEndianness endianness,
  bool is_top_level,
  std::pmr::memory_resource * mr)
: members_(std::move(members)),
  name_to_index_(std::move(name_to_index)),
  total_size_(total_size),
  max_alignment_(max_alignment),
  endianness_(endianness),
  is_top_level_(is_top_level),
  memory_resource_(mr ? mr : std::pmr::new_delete_resource())
{
}

XCdrResult<XCdrStructLayout::MemberConstRef> XCdrStructLayout::get_member(
  std::string_view name) const
{
  auto it = name_to_index_.find(std::string(name));
  if (it == name_to_index_.end()) {
    return error("Member '" + std::string(name) + "' not found in struct");
  }
  return ok(std::cref(members_[it->second]));
}

XCdrResult<XCdrStructLayout::MemberConstRef> XCdrStructLayout::get_member(size_t index) const
{
  if (index >= members_.size()) {
    return error("Member index " + std::to_string(index) + " out of range");
  }
  return ok(std::cref(members_[index]));
}

XCdrStatus XCdrStructLayout::apply(tcb::span<uint8_t> buffer, bool zero_initialize) const
{
  if (buffer.size() < total_size_) {
    return error("Buffer too small for layout (need " + std::to_string(total_size_) +
                 " bytes, got " + std::to_string(buffer.size()) + ")");
  }

  // Zero-initialize if requested
  if (zero_initialize) {
    std::memset(buffer.data(), 0, total_size_);
  }

  // Write XCDR header (only for top-level structs)
  size_t data_offset = 0;
  if (is_top_level_) {
    write_xcdr_header(buffer, endianness_);
    data_offset = kXCdrHeaderSize;
  }

  // Write length prefixes for all variable-length fields
  for (const auto & member : members_) {
    std::visit([&](auto && layout) {
        using T = std::decay_t<decltype(layout)>;

        if constexpr (std::is_same_v<T, XCdrStringLayout>) {
        // Write string length prefix
          // Include null
          uint32_t length = static_cast<uint32_t>(layout.actual_length() + 1);
          size_t prefix_offset = data_offset + member.offset();
          tcb::span<uint8_t> prefix_span(buffer.data() + prefix_offset, 4);
          write_to_bytes(prefix_span, length, endianness_);

        } else if constexpr (std::is_same_v<T, XCdrPrimitiveSequenceLayout>) {
        // Write primitive sequence length prefix
          uint32_t length = static_cast<uint32_t>(layout.actual_count());
          size_t prefix_offset = data_offset + member.offset();
          tcb::span<uint8_t> prefix_span(buffer.data() + prefix_offset, 4);
          write_to_bytes(prefix_span, length, endianness_);

        } else if constexpr (std::is_same_v<T, XCdrSequenceLayout>) {
        // Write non-primitive sequence length prefix
          uint32_t length = static_cast<uint32_t>(layout.actual_count());
          size_t prefix_offset = data_offset + member.offset();
          tcb::span<uint8_t> prefix_span(buffer.data() + prefix_offset, 4);
          write_to_bytes(prefix_span, length, endianness_);

        } else if constexpr (std::is_same_v<T, XCdrStructLayout>) {
        // Recursively apply nested struct layout
          size_t struct_offset = data_offset + member.offset();
          tcb::span<uint8_t> struct_buffer(buffer.data() + struct_offset, layout.total_size());
          auto status = layout.apply(struct_buffer, zero_initialize);
          (void)status;  // Suppress unused variable warning
        }
      // Primitives and arrays don't need initialization
    }, member.layout());
  }

  return ok();
}

}  // namespace xcdr_buffers
