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

#ifndef XCDR_BUFFERS__LAYOUT__LAYOUT_BUILDER_HPP_
#define XCDR_BUFFERS__LAYOUT__LAYOUT_BUILDER_HPP_

#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <vector>

#include "xcdr_buffers/common/endianness.hpp"
#include "xcdr_buffers/common/types.hpp"
#include "xcdr_buffers/layout/layout.hpp"

namespace xcdr_buffers
{

/**
 * @brief Stateful builder for constructing XCDR layouts incrementally.
 *
 * Tracks current offset and alignment while building, ensuring correct XCDR alignment rules.
 */
class XCdrLayoutBuilder
{
public:
  /**
   * @brief Construct a layout builder.
   *
   * @param endianness Endianness for the layout
   * @param mr Memory resource for internal vectors
   * @param is_top_level If true, reserves space for XCDR header (default true)
   */
  explicit XCdrLayoutBuilder(
    XCdrEndianness endianness = XCdrEndianness::kLittleEndian,
    std::pmr::memory_resource * mr = nullptr,
    bool is_top_level = true);

  /**
   * @brief Allocate a primitive field (struct context).
   *
   * @param name Field name
   * @param kind Primitive type kind
   */
  void allocate_primitive(std::string_view name, XCdrPrimitiveKind kind);

  /**
   * @brief Allocate a primitive element (array/sequence context).
   *
   * Use inside begin/end_allocate_array or begin/end_allocate_sequence.
   *
   * @param kind Primitive type kind
   */
  void allocate_primitive(XCdrPrimitiveKind kind);

  /**
   * @brief Allocate a string field (struct context).
   *
   * @param name Field name
   * @param actual_length String length in characters (excluding null terminator)
   * @param char_kind Character kind (char8 or char16)
   */
  void allocate_string(
    std::string_view name, size_t actual_length,
    XCdrCharKind char_kind = XCdrCharKind::kChar8);

  /**
   * @brief Allocate a string element (array/sequence context).
   *
   * Use inside begin/end_allocate_array or begin/end_allocate_sequence.
   *
   * @param actual_length String length in characters (excluding null terminator)
   * @param char_kind Character kind (char8 or char16)
   */
  void allocate_string(size_t actual_length, XCdrCharKind char_kind = XCdrCharKind::kChar8);

  /**
   * @brief Allocate a primitive array field (one-shot operation).
   *
   * Use this for arrays of primitives. For arrays of strings or structs, use begin/end_allocate_array().
   *
   * @param name Field name
   * @param kind Primitive element kind
   * @param count Number of elements
   */
  void allocate_primitive_array(std::string_view name, XCdrPrimitiveKind kind, size_t count);

  /**
   * @brief Allocate a primitive sequence field (one-shot operation).
   *
   * Use this for sequences of primitives. For sequences of strings or structs, use begin/end_allocate_sequence().
   *
   * @param name Field name
   * @param kind Primitive element kind
   * @param actual_count Actual number of elements
   */
  void allocate_primitive_sequence(
    std::string_view name, XCdrPrimitiveKind kind,
    size_t actual_count);

  /**
   * @brief Same as allocate_primitive_array(name, kind, count) but uses auto-generated field name.
   */
  void allocate_primitive_array(XCdrPrimitiveKind kind, size_t count);

  /**
   * @brief Same as allocate_primitive_sequence(name, kind, actual_count) but uses auto-generated field name.
   */
  void allocate_primitive_sequence(XCdrPrimitiveKind kind, size_t actual_count);

  /**
   * @brief Begin allocating an array field with explicit name.
   *
   * Use this for arrays of non-primitive types (strings, structs).
   * For primitive arrays, use allocate_primitive_array().
   * After this, call allocate_* methods `count` times to define each element,
   * then call end_allocate_array().
   * All elements must be the same type category (all strings with same
   * char_kind, or all structs).
   * Individual element sizes may vary (e.g., different string lengths).
   *
   * @param name Field name (must not be empty)
   * @param count Number of elements
   */
  void begin_allocate_array(std::string_view name, size_t count);

  /**
   * @brief Begin allocating an array field with auto-generated name.
   *
   * Same as begin_allocate_array(name, count) but uses auto-generated field
   * name (field_0, field_1, etc.).
   *
   * @param count Number of elements
   */
  void begin_allocate_array(size_t count);

  /**
   * @brief End allocating an array field.
   *
   * Validates that exactly `count` elements were allocated.
   */
  void end_allocate_array();

  /**
   * @brief Begin allocating a sequence field with explicit name.
   *
   * Use this for sequences of non-primitive types (strings, structs).
   * For primitive sequences, use allocate_primitive_sequence().
   * After this, call allocate_* methods `actual_count` times to define each
   * element, then call end_allocate_sequence().
   * All elements must be the same type category (all strings with same
   * char_kind, or all structs).
   * Individual element sizes may vary (e.g., different string lengths).
   *
   * @param name Field name (must not be empty)
   * @param actual_count Number of elements
   */
  void begin_allocate_sequence(std::string_view name, size_t actual_count);

  /**
   * @brief Begin allocating a sequence field with auto-generated name.
   *
   * Same as begin_allocate_sequence(name, actual_count) but uses
   * auto-generated field name (field_0, field_1, etc.).
   *
   * @param actual_count Number of elements
   */
  void begin_allocate_sequence(size_t actual_count);

  /**
   * @brief End allocating a sequence field.
   *
   * Validates that exactly `actual_count` elements were allocated.
   */
  void end_allocate_sequence();

  /**
   * @brief Begin allocating a struct field (struct context).
   *
   * After this, call allocate_* methods to define struct fields, then call end_allocate_struct().
   *
   * @param name Field name
   */
  void begin_allocate_struct(std::string_view name);

  /**
   * @brief Begin allocating a struct element (array/sequence context).
   *
   * Use inside begin/end_allocate_array or begin/end_allocate_sequence.
   * After this, call allocate_* methods to define struct fields, then call end_allocate_struct().
   */
  void begin_allocate_struct();

  /**
   * @brief End allocating a struct field.
   */
  void end_allocate_struct();

  /**
   * @brief Finalize and return the constructed layout.
   *
   * Resets builder state after returning layout.
   *
   * @return Constructed struct layout
   */
  XCdrStructLayout finalize();

  /**
   * @brief Reset builder state.
   *
   * @param endianness Endianness for next layout
   */
  void reset();

  /**
   * @brief Check if there are open contexts (for delegation).
   */
  bool has_open_contexts() const {return !context_stack_.empty();}

private:
  struct BuildContext
  {
    enum class Type { kStruct, kArray, kSequence };
    enum class ElementType { kNone, kPrimitive, kString, kStruct };

    Type type;
    std::string field_name;
    size_t start_offset;
    size_t element_count;  // For arrays/sequences
    ElementType element_type = ElementType::kNone;  // Track element type category
    XCdrPrimitiveKind prim_kind;  // For primitive elements
    XCdrCharKind char_kind;       // For string elements

    // Nested builder for structs
    std::unique_ptr<XCdrLayoutBuilder> nested_builder;

    // Element layouts for arrays/sequences (one per element)
    std::pmr::vector<XCdrLayout> element_layouts;
    std::pmr::vector<size_t> element_offsets;
  };

  std::pmr::vector<XCdrStructLayout::Member> members_;
  std::pmr::map<std::string, size_t> name_to_index_;
  size_t current_offset_;
  size_t max_alignment_;
  XCdrEndianness endianness_;
  std::pmr::memory_resource * memory_resource_;
  bool is_top_level_;  // Whether this is a top-level struct (needs header in buffer)

  std::pmr::vector<BuildContext> context_stack_;
  size_t field_counter_;  // For auto-generating field names

  void add_field(std::string_view name, size_t offset, XCdrLayout layout);
  void align_current_offset(size_t alignment);
  std::string generate_field_name();  // Generate field_N names
};

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__LAYOUT__LAYOUT_BUILDER_HPP_
