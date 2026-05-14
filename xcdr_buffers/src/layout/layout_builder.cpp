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
  context_stack_(mr ? mr : std::pmr::new_delete_resource()),
  field_counter_(0)
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
  name_to_index_[std::string(name)] = index;
  members_.push_back({std::string(name), offset, std::make_shared<XCdrLayout>(std::move(layout))});
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

      ctx.element_layouts.push_back(XCdrPrimitiveLayout(kind));
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

      ctx.element_layouts.push_back(XCdrStringLayout(actual_length, char_kind, memory_resource_));
      ctx.element_offsets.push_back(elem_offset);
      current_offset_ += kStringLengthPrefixSize + actual_length + kStringNullTerminatorSize;
      return;
    }
  }

  // Top-level field
  align_current_offset(kStringLengthPrefixSize);  // Strings align to prefix size
  size_t field_offset = current_offset_;
  add_field(name, field_offset, XCdrStringLayout(actual_length, char_kind, memory_resource_));
  current_offset_ += kStringLengthPrefixSize + actual_length + kStringNullTerminatorSize;
}

void XCdrLayoutBuilder::begin_allocate_array(std::string_view name, size_t count)
{
  BuildContext ctx;
  ctx.type = BuildContext::Type::kArray;
  ctx.field_name = std::string(name);
  ctx.element_count = count;
  ctx.element_layouts = std::pmr::vector<XCdrLayout>(memory_resource_);
  ctx.element_offsets = std::pmr::vector<size_t>(memory_resource_);

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

  if (std::holds_alternative<XCdrPrimitiveLayout>(first_elem)) {
    // Primitive array
    auto prim = std::get<XCdrPrimitiveLayout>(first_elem);

    if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
      context_stack_.back().nested_builder->add_field(
        ctx.field_name,
        ctx.element_offsets[0] + ctx.start_offset,
        XCdrPrimitiveArrayLayout(prim.kind(), ctx.element_count));
    } else {
      add_field(ctx.field_name, ctx.start_offset,
          XCdrPrimitiveArrayLayout(prim.kind(), ctx.element_count));
    }

  } else {
    // Non-primitive array (strings, structs, nested composites) - use unified XCdrArrayLayout
    std::pmr::vector<XCdrArrayLayout::Element> elements(memory_resource_);
    for (size_t i = 0; i < ctx.element_layouts.size(); ++i) {
      elements.push_back({
          ctx.element_offsets[i],
          std::make_shared<XCdrLayout>(std::move(ctx.element_layouts[i]))
      });
    }

    if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
      context_stack_.back().nested_builder->add_field(
        ctx.field_name, ctx.start_offset, XCdrArrayLayout(std::move(elements), memory_resource_));
    } else {
      add_field(ctx.field_name, ctx.start_offset,
          XCdrArrayLayout(std::move(elements), memory_resource_));
    }
  }
}

void XCdrLayoutBuilder::begin_allocate_sequence(std::string_view name, size_t actual_count)
{
  BuildContext ctx;
  ctx.type = BuildContext::Type::kSequence;
  ctx.field_name = std::string(name);
  ctx.element_count = actual_count;
  ctx.element_layouts = std::pmr::vector<XCdrLayout>(memory_resource_);
  ctx.element_offsets = std::pmr::vector<size_t>(memory_resource_);

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

  if (std::holds_alternative<XCdrPrimitiveLayout>(first_elem)) {
    // Primitive sequence
    auto prim = std::get<XCdrPrimitiveLayout>(first_elem);

    if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
      context_stack_.back().nested_builder->add_field(
        ctx.field_name, ctx.start_offset,
          XCdrPrimitiveSequenceLayout(prim.kind(), ctx.element_count));
    } else {
      add_field(ctx.field_name, ctx.start_offset,
          XCdrPrimitiveSequenceLayout(prim.kind(), ctx.element_count));
    }

  } else {
    // Non-primitive sequence (strings, structs, nested composites) - use unified XCdrSequenceLayout
    std::pmr::vector<XCdrSequenceLayout::Element> elements(memory_resource_);
    for (size_t i = 0; i < ctx.element_layouts.size(); ++i) {
      elements.push_back({
          ctx.element_offsets[i],
          std::make_shared<XCdrLayout>(std::move(ctx.element_layouts[i]))
      });
    }

    if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
      context_stack_.back().nested_builder->add_field(
        ctx.field_name, ctx.start_offset,
          XCdrSequenceLayout(std::move(elements), memory_resource_));
    } else {
      add_field(ctx.field_name, ctx.start_offset,
          XCdrSequenceLayout(std::move(elements), memory_resource_));
    }
  }
}

void XCdrLayoutBuilder::begin_allocate_struct(std::string_view name)
{
  // If we're inside a struct context, delegate to the nested builder
  if (!context_stack_.empty() && context_stack_.back().type == BuildContext::Type::kStruct) {
    context_stack_.back().nested_builder->begin_allocate_struct(name);
    return;
  }

  // Create new struct context
  BuildContext ctx;
  ctx.type = BuildContext::Type::kStruct;
  ctx.field_name = std::string(name);
  ctx.element_count = 0;
  ctx.nested_builder = std::make_unique<XCdrLayoutBuilder>(endianness_, memory_resource_, false);
  ctx.element_layouts = std::pmr::vector<XCdrLayout>(memory_resource_);
  ctx.element_offsets = std::pmr::vector<size_t>(memory_resource_);

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

std::string XCdrLayoutBuilder::generate_field_name()
{
  return "field_" + std::to_string(members_.size());
}

// No-name overloads (for array/sequence elements)
void XCdrLayoutBuilder::allocate_primitive(XCdrPrimitiveKind kind)
{
  allocate_primitive(generate_field_name(), kind);
}

void XCdrLayoutBuilder::allocate_string(size_t actual_length, XCdrCharKind char_kind)
{
  allocate_string(generate_field_name(), actual_length, char_kind);
}

void XCdrLayoutBuilder::allocate_primitive_array(XCdrPrimitiveKind kind, size_t count)
{
  allocate_primitive_array(generate_field_name(), kind, count);
}

void XCdrLayoutBuilder::allocate_primitive_sequence(XCdrPrimitiveKind kind, size_t actual_count)
{
  allocate_primitive_sequence(generate_field_name(), kind, actual_count);
}

void XCdrLayoutBuilder::begin_allocate_array(size_t count)
{
  begin_allocate_array(generate_field_name(), count);
}

void XCdrLayoutBuilder::begin_allocate_sequence(size_t actual_count)
{
  begin_allocate_sequence(generate_field_name(), actual_count);
}

// Shortcut methods for primitive arrays/sequences
void XCdrLayoutBuilder::allocate_primitive_array(
  std::string_view name, XCdrPrimitiveKind kind, size_t count)
{
  begin_allocate_array(name, count);
  for (size_t i = 0; i < count; ++i) {
    allocate_primitive(kind);  // Use no-name version for elements
  }
  end_allocate_array();
}

void XCdrLayoutBuilder::allocate_primitive_sequence(
  std::string_view name, XCdrPrimitiveKind kind, size_t actual_count)
{
  begin_allocate_sequence(name, actual_count);
  for (size_t i = 0; i < actual_count; ++i) {
    allocate_primitive(kind);  // Use no-name version for elements
  }
  end_allocate_sequence();
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
  field_counter_ = 0;
}

}  // namespace xcdr_buffers
