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

#include "xcdr_buffers/accessor/const_accessor.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include "xcdr_buffers/common/utilities.hpp"

namespace xcdr_buffers
{

// Helper function to get static primitive layouts for array elements
static const XCdrLayout & get_primitive_layout(XCdrPrimitiveKind kind)
{
  static const std::array<XCdrLayout, 14> layouts = {{
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kBool),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kInt8),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kUint8),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kChar),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kInt16),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kUint16),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kWchar),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kInt32),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kUint32),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kFloat),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kInt64),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kUint64),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kDouble),
    XCdrPrimitiveLayout(XCdrPrimitiveKind::kLongDouble)
  }};
  return layouts[static_cast<size_t>(kind)];
}

// Iterator implementation

XCdrConstAccessor::Iterator::Iterator(const XCdrConstAccessor & accessor, size_t index)
: accessor_(&accessor), index_(index)
{
}

XCdrConstAccessor XCdrConstAccessor::Iterator::operator*() const
{
  auto result = accessor_->item(index_);
  if (!result) {
    throw result.error();
  }
  return *result;
}

XCdrConstAccessor::ConstAccessorArrowHelper XCdrConstAccessor::Iterator::operator->()
{
  auto result = accessor_->item(index_);
  if (!result) {
    throw result.error();
  }
  return ConstAccessorArrowHelper(*result);
}

XCdrConstAccessor::Iterator & XCdrConstAccessor::Iterator::operator++()
{
  ++index_;
  return *this;
}

XCdrConstAccessor::Iterator XCdrConstAccessor::Iterator::operator++(int)  // NOLINT
{
  Iterator temp = *this;
  ++(*this);
  return temp;
}

XCdrConstAccessor::Iterator & XCdrConstAccessor::Iterator::operator--()
{
  --index_;
  return *this;
}

XCdrConstAccessor::Iterator XCdrConstAccessor::Iterator::operator--(int)  // NOLINT
{
  Iterator temp = *this;
  --(*this);
  return temp;
}

XCdrConstAccessor::Iterator & XCdrConstAccessor::Iterator::operator+=(difference_type n)
{
  index_ += n;
  return *this;
}

XCdrConstAccessor::Iterator & XCdrConstAccessor::Iterator::operator-=(difference_type n)
{
  index_ -= n;
  return *this;
}

XCdrConstAccessor::Iterator operator+(
  const XCdrConstAccessor::Iterator & it,
  XCdrConstAccessor::Iterator::difference_type n)
{
  XCdrConstAccessor::Iterator result = it;
  result += n;
  return result;
}

XCdrConstAccessor::Iterator operator+(
  XCdrConstAccessor::Iterator::difference_type n,
  const XCdrConstAccessor::Iterator & it)
{
  return it + n;
}

XCdrConstAccessor::Iterator operator-(
  const XCdrConstAccessor::Iterator & it,
  XCdrConstAccessor::Iterator::difference_type n)
{
  XCdrConstAccessor::Iterator result = it;
  result -= n;
  return result;
}

XCdrConstAccessor::Iterator::difference_type operator-(
  const XCdrConstAccessor::Iterator & lhs,
  const XCdrConstAccessor::Iterator & rhs)
{
  return static_cast<XCdrConstAccessor::Iterator::difference_type>(lhs.index_) -
         static_cast<XCdrConstAccessor::Iterator::difference_type>(rhs.index_);
}

bool operator==(const XCdrConstAccessor::Iterator & lhs, const XCdrConstAccessor::Iterator & rhs)
{
  return lhs.index_ == rhs.index_;
}

bool operator!=(const XCdrConstAccessor::Iterator & lhs, const XCdrConstAccessor::Iterator & rhs)
{
  return !(lhs == rhs);
}

bool operator<(const XCdrConstAccessor::Iterator & lhs, const XCdrConstAccessor::Iterator & rhs)
{
  return lhs.index_ < rhs.index_;
}

bool operator<=(const XCdrConstAccessor::Iterator & lhs, const XCdrConstAccessor::Iterator & rhs)
{
  return !(rhs < lhs);
}

bool operator>(const XCdrConstAccessor::Iterator & lhs, const XCdrConstAccessor::Iterator & rhs)
{
  return rhs < lhs;
}

bool operator>=(const XCdrConstAccessor::Iterator & lhs, const XCdrConstAccessor::Iterator & rhs)
{
  return !(lhs < rhs);
}

// XCdrConstAccessor implementation

XCdrResult<XCdrConstAccessor> XCdrConstAccessor::wrap(
  tcb::span<const uint8_t> buffer,
  const XCdrStructLayout & layout)
{
  // Validate buffer size
  if (buffer.size() < kXCdrHeaderSize) {
    return error("Buffer too small for XCDR header");
  }

  // Read and validate header
  auto endianness_result = read_xcdr_header(buffer);
  if (!endianness_result) {
    return error(endianness_result.error().message());
  }

  // Validate endianness matches system
  if (*endianness_result != kSystemEndianness) {
    return error("Buffer endianness does not match system endianness");
  }

  // Validate buffer size matches layout
  if (buffer.size() < layout.total_size()) {
    return error(
      "Buffer size (" + std::to_string(buffer.size()) +
      ") smaller than layout total size (" + std::to_string(layout.total_size()) + ")");
  }

  return XCdrConstAccessor(buffer, layout);
}

XCdrConstAccessor::XCdrConstAccessor(
  tcb::span<const uint8_t> buffer,
  const XCdrStructLayout & layout)
: buffer_(buffer),
  struct_layout_(&layout),  // Store pointer to externally-owned layout
  layout_(nullptr),
  base_offset_(kXCdrHeaderSize)
{
  // Validate endianness (throws on mismatch)
  auto endianness_result = read_xcdr_header(buffer);
  if (!endianness_result) {
    throw XCdrError(endianness_result.error().message());
  }

  if (*endianness_result != kSystemEndianness) {
    throw XCdrError("Buffer endianness does not match system endianness");
  }
}

XCdrConstAccessor::XCdrConstAccessor(
  tcb::span<const uint8_t> buffer,
  const XCdrLayout * layout,
  size_t base_offset)
: buffer_(buffer), struct_layout_(nullptr), layout_(layout), base_offset_(base_offset)
{
}

XCdrResult<XCdrConstAccessor> XCdrConstAccessor::member(std::string_view name_or_path) const
{
  // Get the struct layout (either from top-level or from polymorphic layout)
  const XCdrStructLayout * struct_layout_ptr = nullptr;

  if (struct_layout_) {
    // Top-level accessor
    struct_layout_ptr = struct_layout_;
  } else if (layout_ && std::holds_alternative<XCdrStructLayout>(*layout_)) {
    // Member accessor pointing to a nested struct
    struct_layout_ptr = &std::get<XCdrStructLayout>(*layout_);
  } else {
    return error("member() called on non-struct type");
  }

  const auto & struct_layout = *struct_layout_ptr;

  // Partition path at first dot to handle nested access
  auto [first_component, remainder] = partition(name_or_path, '.');

  // Find the member by name
  const XCdrStructLayout::Member * member_ptr = nullptr;
  for (size_t i = 0; i < struct_layout.member_count(); ++i) {
    auto member_result = struct_layout.get_member(i);
    if (!member_result) {
      return error("Failed to get member at index " + std::to_string(i));
    }
    const XCdrStructLayout::Member & member_ref = member_result->get();  // Unwrap reference_wrapper
    if (member_ref.name() == first_component) {
      member_ptr = &member_ref;
      break;
    }
  }

  if (!member_ptr) {
    return error("Member '" + std::string(first_component) + "' not found");
  }

  const auto & member = *member_ptr;

  // Create accessor for this member
  XCdrConstAccessor member_accessor(
    buffer_,
    &member.layout(),
    base_offset_ + member.offset());

  // If there's a remainder, recursively access nested members
  if (!remainder.empty()) {
    return member_accessor.member(remainder);
  }

  return member_accessor;
}

XCdrConstAccessor XCdrConstAccessor::operator[](std::string_view name_or_path) const
{
  auto result = member(name_or_path);
  if (!result) {
    throw result.error();
  }
  return *result;
}

XCdrResult<XCdrConstAccessor> XCdrConstAccessor::item(size_t index) const
{
  // Get the struct layout (either from top-level or from polymorphic layout)
  const XCdrStructLayout * struct_layout_ptr = nullptr;

  if (struct_layout_) {
    // Top-level struct accessor - access member by index
    struct_layout_ptr = struct_layout_;
  } else if (layout_ && std::holds_alternative<XCdrStructLayout>(*layout_)) {
    // Member accessor pointing to a nested struct - access member by index
    struct_layout_ptr = &std::get<XCdrStructLayout>(*layout_);
  }

  // If this is a struct accessor, treat index as member index
  if (struct_layout_ptr) {
    const auto & struct_layout = *struct_layout_ptr;
    if (index >= struct_layout.member_count()) {
      return error("Struct member index out of bounds");
    }

    auto member_result = struct_layout.get_member(index);
    if (!member_result) {
      return error("Failed to get member at index " + std::to_string(index));
    }
    const XCdrStructLayout::Member & member = member_result->get();

    // Create accessor for this member
    return ok(XCdrConstAccessor(
      buffer_,
      &member.layout(),
      base_offset_ + member.offset()));
  }

  // Not a struct, check if it's an array or sequence
  if (!layout_) {
    return error("item() called on invalid accessor");
  }

  // Handle primitive arrays
  if (std::holds_alternative<XCdrPrimitiveArrayLayout>(*layout_)) {
    const auto & prim_array = std::get<XCdrPrimitiveArrayLayout>(*layout_);

    if (index >= prim_array.count()) {
      return error("Array index out of bounds");
    }

    const size_t elem_offset = base_offset_ + prim_array.element_offset(index);
    return XCdrConstAccessor(buffer_, &get_primitive_layout(prim_array.element_kind()),
        elem_offset);
  }

  // Handle non-primitive arrays (unified)
  if (std::holds_alternative<XCdrArrayLayout>(*layout_)) {
    const auto & array_layout = std::get<XCdrArrayLayout>(*layout_);

    if (index >= array_layout.count()) {
      return error("Array index out of bounds");
    }

    auto offset_result = array_layout.element_offset(index);
    if (!offset_result) {
      return error(offset_result.error().message());
    }

    auto layout_result = array_layout.element_layout(index);
    if (!layout_result) {
      return error(layout_result.error().message());
    }

    const size_t elem_offset = base_offset_ + *offset_result;
    return XCdrConstAccessor(buffer_, &layout_result->get(), elem_offset);
  }

  // Handle primitive sequences
  if (std::holds_alternative<XCdrPrimitiveSequenceLayout>(*layout_)) {
    const auto & prim_seq = std::get<XCdrPrimitiveSequenceLayout>(*layout_);

    if (index >= prim_seq.actual_count()) {
      return error("Sequence index out of bounds");
    }

    const size_t elem_offset = base_offset_ + prim_seq.element_offset(index);
    return XCdrConstAccessor(buffer_, &get_primitive_layout(prim_seq.element_kind()), elem_offset);
  }

  // Handle non-primitive sequences (unified)
  if (std::holds_alternative<XCdrSequenceLayout>(*layout_)) {
    const auto & seq_layout = std::get<XCdrSequenceLayout>(*layout_);

    if (index >= seq_layout.actual_count()) {
      return error("Sequence index out of bounds");
    }

    auto offset_result = seq_layout.element_offset(index);
    if (!offset_result) {
      return error(offset_result.error().message());
    }

    auto layout_result = seq_layout.element_layout(index);
    if (!layout_result) {
      return error(layout_result.error().message());
    }

    // Element offset is already relative to sequence data start (after length prefix)
    const size_t elem_offset = base_offset_ + kSequenceLengthPrefixSize + *offset_result;
    return XCdrConstAccessor(buffer_, &layout_result->get(), elem_offset);
  }

  return error("item() called on non-array/sequence type");
}

XCdrConstAccessor XCdrConstAccessor::operator[](size_t index) const
{
  auto result = item(index);
  if (!result) {
    throw result.error();
  }
  return *result;
}

XCdrResult<size_t> XCdrConstAccessor::size() const
{
  // Handle primitive arrays
  if (std::holds_alternative<XCdrPrimitiveArrayLayout>(*layout_)) {
    return std::get<XCdrPrimitiveArrayLayout>(*layout_).count();
  }

  // Handle non-primitive arrays (unified)
  if (std::holds_alternative<XCdrArrayLayout>(*layout_)) {
    return std::get<XCdrArrayLayout>(*layout_).count();
  }

  // Handle primitive sequences
  if (std::holds_alternative<XCdrPrimitiveSequenceLayout>(*layout_)) {
    return std::get<XCdrPrimitiveSequenceLayout>(*layout_).actual_count();
  }

  // Handle non-primitive sequences (unified)
  if (std::holds_alternative<XCdrSequenceLayout>(*layout_)) {
    return std::get<XCdrSequenceLayout>(*layout_).actual_count();
  }

  return error("size() called on non-array/sequence type");
}

XCdrConstAccessor::Iterator XCdrConstAccessor::begin() const
{
  if (!std::holds_alternative<XCdrPrimitiveArrayLayout>(*layout_) &&
    !std::holds_alternative<XCdrArrayLayout>(*layout_) &&
    !std::holds_alternative<XCdrPrimitiveSequenceLayout>(*layout_) &&
    !std::holds_alternative<XCdrSequenceLayout>(*layout_))
  {
    throw XCdrError("begin() called on non-array/sequence type");
  }
  return Iterator(*this, 0);
}

XCdrConstAccessor::Iterator XCdrConstAccessor::end() const
{
  auto size_result = size();
  if (!size_result) {
    throw size_result.error();
  }
  return Iterator(*this, *size_result);
}

tcb::span<const uint8_t> XCdrConstAccessor::slice() const
{
  // Calculate the size of this value
  size_t value_size = 0;

  if (std::holds_alternative<XCdrPrimitiveLayout>(*layout_)) {
    const auto & prim = std::get<XCdrPrimitiveLayout>(*layout_);
    value_size = get_primitive_size(prim.kind());
  } else if (std::holds_alternative<XCdrStringLayout>(*layout_)) {
    const auto & str = std::get<XCdrStringLayout>(*layout_);
    value_size = kStringLengthPrefixSize + str.actual_length() + kStringNullTerminatorSize;
  } else if (std::holds_alternative<XCdrStructLayout>(*layout_)) {
    const auto & st = std::get<XCdrStructLayout>(*layout_);
    // For struct, use the total size minus the header (since base_offset already accounts for it)
    value_size = st.total_size() - kXCdrHeaderSize;
  } else {
    // For arrays and sequences, we need to compute the total size
    // This is complex, so for now just return from base_offset to end of buffer
    return buffer_.subspan(base_offset_);
  }

  if (base_offset_ + value_size > buffer_.size()) {
    throw XCdrError("Buffer overflow in slice()");
  }

  return buffer_.subspan(base_offset_, value_size);
}

const XCdrLayout & XCdrConstAccessor::layout() const
{
  return *layout_;
}

}  // namespace xcdr_buffers
