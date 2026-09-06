// Copyright 2026 xcdr_buffers Contributors
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

#include "xcdr_buffers/layout/layout_builder.hpp"

#include <algorithm>
#include <cassert>

namespace xcdr_buffers
{

XCdrLayoutBuilder::XCdrLayoutBuilder(
  XCdrEndianness endianness, std::pmr::memory_resource * mr, bool is_top_level)
: members_(mr ? mr : std::pmr::new_delete_resource()),
  name_to_index_(mr ? mr : std::pmr::new_delete_resource()),
  current_offset_(0),  // Always start at 0 (relative to struct data start)
  max_alignment_(1),
  endianness_(endianness),
  memory_resource_(mr ? mr : std::pmr::new_delete_resource()),
  is_top_level_(is_top_level),
  context_stack_(mr ? mr : std::pmr::new_delete_resource())
{
}

void XCdrLayoutBuilder::align_current_offset(size_t alignment)
{
  // CDR alignment is always data-relative (relative to position after header for top-level,
  // relative to struct start for nested structs)
  current_offset_ = align_to(current_offset_, alignment);
  max_alignment_ = std::max(max_alignment_, alignment);
}

void XCdrLayoutBuilder::add_field(std::string_view name, size_t offset, XCdrLayout layout)
{
  size_t index = members_.size();
  if (!name.empty()) {
    name_to_index_.try_emplace(std::pmr::string(name, memory_resource_), index);
  }
  members_.emplace_back(
    std::pmr::string(name, memory_resource_), offset,
    std::allocate_shared<XCdrLayout>(
      std::pmr::polymorphic_allocator<XCdrLayout>(memory_resource_), std::move(layout)));
}

void XCdrLayoutBuilder::allocate_primitive(std::string_view name, XCdrPrimitiveKind kind)
{
  if (!context_stack_.empty()) {
    auto & ctx = context_stack_.back();
    if (ctx.type == BuildContext::Type::kStruct) {
      // Nested struct field
      ctx.nested_builder->allocate_primitive(name, kind);
      return;
    } else {
      // Array/sequence element
      align_current_offset(get_primitive_alignment(kind));
      size_t elem_offset = current_offset_ - ctx.start_offset;

      if (ctx.type == BuildContext::Type::kSequence) {
        elem_offset -= kSequenceLengthPrefixSize;  // Sequences have length prefix
      }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
      ctx.element_layouts.push_back(XCdrPrimitiveLayout(kind, memory_resource_));
#pragma GCC diagnostic pop
      ctx.element_offsets.push_back(elem_offset);
      current_offset_ += get_primitive_size(kind);
      return;
    }
  }

  // Top-level field
  align_current_offset(get_primitive_alignment(kind));
  size_t field_offset = current_offset_;
  add_field(name, field_offset, XCdrPrimitiveLayout(kind));
  current_offset_ += get_primitive_size(kind);
}

void XCdrLayoutBuilder::allocate_string(
  std::string_view name, size_t actual_length, XCdrCharKind char_kind)
{
  if (!context_stack_.empty()) {
    auto & ctx = context_stack_.back();
    if (ctx.type == BuildContext::Type::kStruct) {
      // Nested struct field
      ctx.nested_builder->allocate_string(name, actual_length, char_kind);
      return;
    } else {
      // Array/sequence element
      align_current_offset(kStringLengthPrefixSize);  // Strings align to prefix size
      size_t elem_offset = current_offset_ - ctx.start_offset;

      if (ctx.type == BuildContext::Type::kSequence) {
        elem_offset -= kSequenceLengthPrefixSize;  // Sequences have length prefix
      }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
      ctx.element_layouts.push_back(XCdrStringLayout(actual_length, char_kind, memory_resource_));
#pragma GCC diagnostic pop
      ctx.element_offsets.push_back(elem_offset);
      // The layout object we just pushed knows its own byte size.
      current_offset_ += std::get<XCdrStringLayout>(ctx.element_layouts.back()).size();
      return;
    }
  }

  // Top-level field
  align_current_offset(kStringLengthPrefixSize);  // Strings align to prefix size
  size_t field_offset = current_offset_;
  XCdrStringLayout string_layout(actual_length, char_kind, memory_resource_);
  add_field(name, field_offset, string_layout);
  current_offset_ += string_layout.size();
}

void XCdrLayoutBuilder::begin_allocate_array(std::string_view name, size_t count)
{
  BuildContext ctx(memory_resource_);
  ctx.type = BuildContext::Type::kArray;
  ctx.field_name.assign(name);
  ctx.element_count = count;

  // Arrays don't have length prefix, so start offset is current position
  // (will be aligned when first element is added)
  ctx.start_offset = current_offset_;

  context_stack_.push_back(std::move(ctx));
}

void XCdrLayoutBuilder::end_allocate_array()
{
  if (context_stack_.empty() || context_stack_.back().type != BuildContext::Type::kArray) {
    // Error: unmatched end_allocate_array
    return;
  }

  auto ctx = std::move(context_stack_.back());
  context_stack_.pop_back();

  // Determine array type based on elements
  if (ctx.element_layouts.empty()) {
    // Empty array - shouldn't happen
    return;
  }

  const auto & first_elem = ctx.element_layouts[0];

  // Primitive arrays must be built with allocate_primitive_array(): the
  // begin/allocate/end path is only for non-primitive elements (strings,
  // structs, composites), where each element may have a different layout.
  assert(!std::holds_alternative<XCdrPrimitiveLayout>(first_elem) &&
    "Primitive arrays must be built with allocate_primitive_array()");

  // Non-primitive array (strings, structs, nested composites) - use unified XCdrArrayLayout
  std::pmr::vector<XCdrArrayLayout::Element> elements(memory_resource_);

  // Homogeneous arrays: the generator emits a single allocate_* call for the
  // element type; replicate it element_count times.  Each element may sit at a
  // different offset due to alignment padding.
  if (ctx.element_layouts.size() == 1 && ctx.element_count > 1) {
    const auto & elem_layout = ctx.element_layouts[0];
    size_t elem_size = std::visit([](const auto & l) { return l.size(); }, elem_layout);
    size_t elem_align = std::visit([](const auto & l) { return l.alignment(); }, elem_layout);
    size_t offset = ctx.element_offsets[0];
    for (size_t i = 0; i < ctx.element_count; ++i) {
      elements.push_back({
          offset,
          std::allocate_shared<XCdrLayout>(
            std::pmr::polymorphic_allocator<XCdrLayout>(memory_resource_),
            elem_layout)  // copy: the layout is shared by all elements
      });
      offset = align_to(offset + elem_size, elem_align);
    }
  } else {
    for (size_t i = 0; i < ctx.element_layouts.size(); ++i) {
      elements.push_back({
          ctx.element_offsets[i],
          std::allocate_shared<XCdrLayout>(
            std::pmr::polymorphic_allocator<XCdrLayout>(memory_resource_),
            std::move(ctx.element_layouts[i]))
      });
    }
  }

  XCdrArrayLayout array_layout(std::move(elements), memory_resource_);
  if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
    context_stack_.back().nested_builder->add_field(
      ctx.field_name, ctx.start_offset, array_layout);
  } else {
    add_field(ctx.field_name, ctx.start_offset, array_layout);
  }
  // Advance past the array's true span (accounts for inter-element alignment
  // padding that per-element size accumulation under-counts).
  current_offset_ = ctx.start_offset + array_layout.size();
}

void XCdrLayoutBuilder::begin_allocate_sequence(std::string_view name, size_t actual_count)
{
  BuildContext ctx(memory_resource_);
  ctx.type = BuildContext::Type::kSequence;
  ctx.field_name.assign(name);
  ctx.element_count = actual_count;

  // Sequences have length prefix
  align_current_offset(kSequenceLengthPrefixSize);
  ctx.start_offset = current_offset_;
  current_offset_ += kSequenceLengthPrefixSize;  // Reserve space for length prefix

  context_stack_.push_back(std::move(ctx));
}

void XCdrLayoutBuilder::end_allocate_sequence()
{
  if (context_stack_.empty() || context_stack_.back().type != BuildContext::Type::kSequence) {
    // Error: unmatched end_allocate_sequence
    return;
  }

  auto ctx = std::move(context_stack_.back());
  context_stack_.pop_back();

  // Determine sequence type based on elements
  if (ctx.element_layouts.empty()) {
    // Empty sequence
    if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
      // Can't easily determine type for empty sequence - skip for now
    } else {
      // Empty sequence at top level - skip
    }
    return;
  }

  const auto & first_elem = ctx.element_layouts[0];

  // Primitive sequences must be built with allocate_primitive_sequence():
  // the begin/allocate/end path is only for non-primitive elements (strings,
  // structs, composites), where each element may have a different layout.
  assert(!std::holds_alternative<XCdrPrimitiveLayout>(first_elem) &&
    "Primitive sequences must be built with allocate_primitive_sequence()");

  // Non-primitive sequence (strings, structs, nested composites)
  std::pmr::vector<XCdrSequenceLayout::Element> elements(memory_resource_);
  for (size_t i = 0; i < ctx.element_layouts.size(); ++i) {
    elements.push_back({
        ctx.element_offsets[i],
        std::allocate_shared<XCdrLayout>(
          std::pmr::polymorphic_allocator<XCdrLayout>(memory_resource_),
          std::move(ctx.element_layouts[i]))
    });
  }

  XCdrSequenceLayout sequence_layout(std::move(elements), ctx.element_count, memory_resource_);
  if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
    context_stack_.back().nested_builder->add_field(
      ctx.field_name, ctx.start_offset, sequence_layout);
  } else {
    add_field(ctx.field_name, ctx.start_offset, sequence_layout);
  }
  // Advance past the sequence's true span (includes the length prefix and
  // inter-element alignment padding).
  current_offset_ = ctx.start_offset + sequence_layout.size();
}

void XCdrLayoutBuilder::begin_allocate_struct()
{
  // If we're inside a struct context, delegate to the nested builder
  if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
    context_stack_.back().nested_builder->begin_allocate_struct();
    return;
  }

  // Create new struct context (unnamed — used inside array/sequence)
  BuildContext ctx(memory_resource_);
  ctx.type = BuildContext::Type::kStruct;
  ctx.element_count = 0;
  ctx.nested_builder = std::allocate_shared<XCdrLayoutBuilder>(
    std::pmr::polymorphic_allocator<XCdrLayoutBuilder>(memory_resource_),
    endianness_, memory_resource_, false);

  ctx.start_offset = current_offset_;

  context_stack_.push_back(std::move(ctx));
}

void XCdrLayoutBuilder::begin_allocate_struct(std::string_view name)
{
  // If we're inside a struct context, delegate to the nested builder
  if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
    context_stack_.back().nested_builder->begin_allocate_struct(name);
    return;
  }

  // Create new struct context
  BuildContext ctx(memory_resource_);
  ctx.type = BuildContext::Type::kStruct;
  ctx.field_name.assign(name);
  ctx.element_count = 0;
  ctx.nested_builder = std::allocate_shared<XCdrLayoutBuilder>(
    std::pmr::polymorphic_allocator<XCdrLayoutBuilder>(memory_resource_),
    endianness_, memory_resource_, false);

  // Struct alignment will be determined when finalized
  ctx.start_offset = current_offset_;

  context_stack_.push_back(std::move(ctx));
}

void XCdrLayoutBuilder::end_allocate_struct()
{
  if (context_stack_.empty() || context_stack_.back().type != BuildContext::Type::kStruct) {
    // Error: unmatched end_allocate_struct
    return;
  }

  auto & ctx = context_stack_.back();

  // Check if the nested builder has any open struct contexts
  // If so, delegate to it first
  if (ctx.nested_builder && !ctx.nested_builder->context_stack_.empty() &&
    ctx.nested_builder->context_stack_.back().type == BuildContext::Type::kStruct)
  {
    ctx.nested_builder->end_allocate_struct();
    return;
  }

  // Now we can actually end this struct context
  auto ctx_moved = std::move(context_stack_.back());
  context_stack_.pop_back();

  // Finalize nested struct
  XCdrStructLayout nested_layout = ctx_moved.nested_builder->finalize();

  // Align to nested struct's max alignment
  align_current_offset(nested_layout.max_alignment());
  size_t field_offset = current_offset_;

  if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
    // Adding struct field to parent struct
    context_stack_.back().nested_builder->add_field(ctx_moved.field_name, field_offset,
        std::move(nested_layout));
  } else {
    // Top-level struct field
    add_field(ctx_moved.field_name, field_offset, std::move(nested_layout));
  }

  current_offset_ += nested_layout.total_size();
}

// No-name overloads (for array/sequence elements or unnamed struct fields)
void XCdrLayoutBuilder::allocate_primitive(XCdrPrimitiveKind kind)
{
  allocate_primitive("", kind);
}

void XCdrLayoutBuilder::allocate_string(size_t actual_length, XCdrCharKind char_kind)
{
  allocate_string("", actual_length, char_kind);
}

void XCdrLayoutBuilder::allocate_primitive_array(XCdrPrimitiveKind kind, size_t count)
{
  allocate_primitive_array("", kind, count);
}

void XCdrLayoutBuilder::allocate_primitive_sequence(XCdrPrimitiveKind kind, size_t actual_count)
{
  allocate_primitive_sequence("", kind, actual_count);
}

void XCdrLayoutBuilder::begin_allocate_array(size_t count)
{
  begin_allocate_array("", count);
}

void XCdrLayoutBuilder::begin_allocate_sequence(size_t actual_count)
{
  begin_allocate_sequence("", actual_count);
}

// Shortcut methods for primitive arrays/sequences
void XCdrLayoutBuilder::allocate_primitive_array(
  std::string_view name, XCdrPrimitiveKind kind, size_t count)
{
  // Build the homogeneous primitive array layout in one shot: align to the
  // element alignment, record the (aligned) field offset, and advance past
  // all elements.  The begin/allocate/end path is for non-primitive arrays.
  align_current_offset(get_primitive_alignment(kind));
  size_t field_offset = current_offset_;
  XCdrPrimitiveArrayLayout arr(kind, count, memory_resource_);
  current_offset_ += arr.size();

  if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
    context_stack_.back().nested_builder->add_field(name, field_offset, std::move(arr));
  } else {
    add_field(name, field_offset, std::move(arr));
  }
}

void XCdrLayoutBuilder::allocate_primitive_sequence(
  std::string_view name, XCdrPrimitiveKind kind, size_t actual_count)
{
  // Build the homogeneous primitive sequence layout in one shot: align to the
  // length prefix, record the prefix-start field offset, and advance past the
  // prefix + padding + all element bytes.  The begin/allocate/end path is for
  // non-primitive sequences.
  align_current_offset(kSequenceLengthPrefixSize);
  size_t field_offset = current_offset_;
  XCdrPrimitiveSequenceLayout seq(kind, actual_count, memory_resource_);
  current_offset_ += seq.size();

  if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
    context_stack_.back().nested_builder->add_field(name, field_offset, std::move(seq));
  } else {
    add_field(name, field_offset, std::move(seq));
  }
}

XCdrStructLayout XCdrLayoutBuilder::finalize()
{
  // For top-level structs, add header size to total_size
  size_t total_size = is_top_level_ ? (current_offset_ + kXCdrHeaderSize) : current_offset_;

  XCdrStructLayout layout(
    std::move(members_),
    std::move(name_to_index_),
    total_size,
    max_alignment_,
    endianness_,
    is_top_level_,
    memory_resource_
  );

  reset();
  return layout;
}

void XCdrLayoutBuilder::reset()
{
  members_.clear();
  name_to_index_.clear();
  current_offset_ = 0;  // Always reset to 0 (relative offset)
  max_alignment_ = 1;
  context_stack_.clear();
}

}  // namespace xcdr_buffers
