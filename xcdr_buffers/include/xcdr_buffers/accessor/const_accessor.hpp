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

#ifndef XCDR_BUFFERS__ACCESSOR__CONST_ACCESSOR_HPP_
#define XCDR_BUFFERS__ACCESSOR__CONST_ACCESSOR_HPP_

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>

#include "tcb_span/span.hpp"
#include "xcdr_buffers/common/endianness.hpp"
#include "xcdr_buffers/common/result.hpp"
#include "xcdr_buffers/common/types.hpp"
#include "xcdr_buffers/layout/layout.hpp"

namespace xcdr_buffers
{

/// Read-only accessor for zero-copy access to XCDR buffers.
///
/// Provides structured access to serialized data without copying.
/// Works with layouts from XCdrLayoutBuilder or XCdrLayoutParser.
///
/// Usage:
/// ```cpp
/// // From a buffer and layout
/// auto accessor_result = XCdrConstAccessor::wrap(buffer, layout);
/// if (!accessor_result) { /* error */ }
/// XCdrConstAccessor accessor = *accessor_result;
///
/// // Access primitive fields
/// uint32_t id = accessor.member("id").as<uint32_t>();
///
/// // Access strings (zero-copy)
/// std::string_view name = accessor.member("name").as<std::string_view>();
///
/// // Access nested fields (path syntax)
/// double x = accessor.member("position.x").as<double>();
///
/// // Access array elements
/// auto tags = accessor.member("tags");
/// for (size_t i = 0; i < tags.size(); ++i) {
///   std::string_view tag = tags.item(i).as<std::string_view>();
/// }
///
/// // Range-based iteration over sequences
/// auto values = accessor.member("values");
/// for (auto elem : values) {
///   uint32_t val = elem.as<uint32_t>();
/// }
/// ```
class XCdrConstAccessor
{
public:
  // Forward declaration of arrow helper (defined after class)
  class ConstAccessorArrowHelper;

  /// Iterator for arrays and sequences.
  /// Note: Does not provide true reference/pointer semantics (returns values instead).
  class Iterator
  {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = XCdrConstAccessor;
    using difference_type = std::ptrdiff_t;
    // Note: No reference/pointer typedefs - operator* returns by value

    Iterator() = default;
    Iterator(const XCdrConstAccessor & accessor, size_t index);

    XCdrConstAccessor operator*() const;
    ConstAccessorArrowHelper operator->();  // Returns arrow helper proxy

    Iterator & operator++();
    Iterator operator++(int);
    Iterator & operator--();
    Iterator operator--(int);

    Iterator & operator+=(difference_type n);
    Iterator & operator-=(difference_type n);

    friend Iterator operator+(const Iterator & it, difference_type n);
    friend Iterator operator+(difference_type n, const Iterator & it);
    friend Iterator operator-(const Iterator & it, difference_type n);
    friend difference_type operator-(const Iterator & lhs, const Iterator & rhs);

    friend bool operator==(const Iterator & lhs, const Iterator & rhs);
    friend bool operator!=(const Iterator & lhs, const Iterator & rhs);
    friend bool operator<(const Iterator & lhs, const Iterator & rhs);
    friend bool operator<=(const Iterator & lhs, const Iterator & rhs);
    friend bool operator>(const Iterator & lhs, const Iterator & rhs);
    friend bool operator>=(const Iterator & lhs, const Iterator & rhs);

private:
    const XCdrConstAccessor * accessor_ = nullptr;  // Pointer to avoid circular dependency
    size_t index_ = 0;
  };

  /// Wraps a buffer with a layout, validating endianness.
  ///
  /// @param buffer The XCDR buffer (must include header)
  /// @param layout The layout describing the buffer structure
  /// @return Accessor on success, error if endianness mismatch or invalid buffer
  static XCdrResult<XCdrConstAccessor> wrap(
    tcb::span<const uint8_t> buffer,
    const XCdrStructLayout & layout);

  /// Template overload to accept any container with data() and size()
  template<typename Container,
    typename = std::enable_if_t<
      std::is_convertible_v<decltype(std::declval<const Container &>().data()), const uint8_t *>&&
      std::is_convertible_v<decltype(std::declval<const Container &>().size()), size_t>>>
  static XCdrResult<XCdrConstAccessor> wrap(
    const Container & buffer,
    const XCdrStructLayout & layout)
  {
    return wrap(tcb::span<const uint8_t>(buffer.data(), buffer.size()), layout);
  }

  /// Constructs an accessor (validates endianness, throws on mismatch).
  ///
  /// Prefer using wrap() for error handling without exceptions.
  ///
  /// @param buffer The XCDR buffer
  /// @param layout The layout describing the buffer
  /// @throws XCdrError if endianness doesn't match system endianness
  XCdrConstAccessor(
    tcb::span<const uint8_t> buffer,
    const XCdrStructLayout & layout);

  /// Accesses a struct field by name or path.
  ///
  /// Supports both simple names ("field") and paths ("outer.inner.field").
  ///
  /// @param name_or_path Field name or dot-separated path
  /// @return Accessor for the field, or error if not found
  XCdrResult<XCdrConstAccessor> member(std::string_view name_or_path) const;

  /// Accesses a struct field by name or path (throws on error).
  ///
  /// @param name_or_path Field name or dot-separated path
  /// @return Accessor for the field
  /// @throws XCdrError if field not found
  XCdrConstAccessor operator[](std::string_view name_or_path) const;

  /// Accesses an array/sequence element by index.
  ///
  /// @param index Element index (0-based)
  /// @return Accessor for the element, or error if out of bounds
  XCdrResult<XCdrConstAccessor> item(size_t index) const;

  /// Accesses an array/sequence element by index (throws on error).
  ///
  /// @param index Element index (0-based)
  /// @return Accessor for the element
  /// @throws XCdrError if out of bounds
  XCdrConstAccessor operator[](size_t index) const;

  /// Converts to a primitive type, string_view, or span.
  ///
  /// - Primitives: Returns value by copy
  /// - Strings: Returns std::string_view (zero-copy)
  /// - Primitive arrays/sequences: Returns tcb::span<const T> (zero-copy)
  ///
  /// @tparam T Target type
  /// @return Value of type T
  /// @throws XCdrError if type mismatch or not supported
  template<typename T>
  T as() const;

  /// Returns the number of elements (for arrays/sequences).
  ///
  /// @return Element count, or error if not an array/sequence
  XCdrResult<size_t> size() const;

  /// Returns iterator to first element (for arrays/sequences).
  ///
  /// @return Iterator to first element
  /// @throws XCdrError if not an array/sequence
  Iterator begin() const;

  /// Returns iterator past last element (for arrays/sequences).
  ///
  /// @return Iterator past last element
  /// @throws XCdrError if not an array/sequence
  Iterator end() const;

  /// Returns raw buffer slice for this value.
  ///
  /// @return Span of bytes representing this value
  tcb::span<const uint8_t> slice() const;

  /// Returns the layout for this accessor.
  ///
  /// @return Reference to the layout
  const XCdrLayout & layout() const;

protected:
  /// Internal constructor for creating sub-accessors.
  XCdrConstAccessor(
    tcb::span<const uint8_t> buffer,
    const XCdrLayout * layout,
    size_t base_offset);

  /// Reads a primitive value at the given offset.
  template<typename T>
  T read_primitive(size_t offset) const;

  tcb::span<const uint8_t> buffer_;
  const XCdrStructLayout * struct_layout_;  // For top-level accessor (wrap)
  const XCdrLayout * layout_;  // For member accessor (polymorphic)
  size_t base_offset_;
};

// Template implementation

template<typename T>
T XCdrConstAccessor::as() const
{
  static_assert(
    std::is_arithmetic_v<T>|| std::is_same_v<T, std::string_view>,
    "as<T>() supports primitives and std::string_view");

  if constexpr (std::is_same_v<T, std::string_view>) {
    // String access
    if (!std::holds_alternative<XCdrStringLayout>(*layout_)) {
      throw XCdrError("Type mismatch: expected string layout");
    }

    const auto & str_layout = std::get<XCdrStringLayout>(*layout_);
    size_t string_offset = base_offset_ + kStringLengthPrefixSize;

    if (string_offset + str_layout.actual_length() > buffer_.size()) {
      throw XCdrError("Buffer overflow reading string");
    }

    return std::string_view(
      reinterpret_cast<const char *>(buffer_.data() + string_offset),
      str_layout.actual_length());
  } else {
    // Primitive access
    if (!std::holds_alternative<XCdrPrimitiveLayout>(*layout_)) {
      throw XCdrError("Type mismatch: expected primitive layout");
    }
    const auto & prim_layout = std::get<XCdrPrimitiveLayout>(*layout_);

    // Validate primitive kind matches T using type trait
    constexpr XCdrPrimitiveKind expected_kind = primitive_kind_v<T>;

    if (prim_layout.kind() != expected_kind) {
      throw XCdrError("Primitive type mismatch");
    }

    if (prim_layout.kind() != expected_kind) {
      throw XCdrError("Primitive type mismatch");
    }

    return read_primitive<T>(base_offset_);
  }
}

template<typename T>
T XCdrConstAccessor::read_primitive(size_t offset) const
{
  static_assert(std::is_arithmetic_v<T>, "read_primitive requires arithmetic type");

  if (offset + sizeof(T) > buffer_.size()) {
    throw XCdrError("Buffer overflow reading primitive");
  }

  T result;
  read_from_bytes(&result, buffer_.subspan(offset, sizeof(T)), kSystemEndianness);
  return result;
}

// Arrow helper definition (must come after XCdrConstAccessor is complete)
class XCdrConstAccessor::ConstAccessorArrowHelper
{
public:
  explicit ConstAccessorArrowHelper(XCdrConstAccessor accessor)
  : accessor_(std::move(accessor)) {}

  const XCdrConstAccessor * operator->() const {return &accessor_;}

private:
  XCdrConstAccessor accessor_;
};

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__ACCESSOR__CONST_ACCESSOR_HPP_
