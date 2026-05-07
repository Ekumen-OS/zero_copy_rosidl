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

#ifndef XCDR_BUFFERS__ACCESSOR__ACCESSOR_HPP_
#define XCDR_BUFFERS__ACCESSOR__ACCESSOR_HPP_

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>

#include "tcb_span/span.hpp"
#include "xcdr_buffers/accessor/const_accessor.hpp"
#include "xcdr_buffers/common/endianness.hpp"
#include "xcdr_buffers/common/result.hpp"
#include "xcdr_buffers/common/types.hpp"
#include "xcdr_buffers/layout/layout.hpp"

namespace xcdr_buffers
{

/// Read-write accessor for zero-copy access to XCDR buffers.
///
/// Extends XCdrConstAccessor with write capabilities.
///
/// Usage:
/// ```cpp
/// // From a buffer and layout
/// auto accessor_result = XCdrAccessor::wrap(buffer, layout);
/// if (!accessor_result) { /* error */ }
/// XCdrAccessor accessor = *accessor_result;
///
/// // Read primitive fields (like const accessor)
/// uint32_t id = accessor.member("id").as<uint32_t>();
///
/// // Write primitive fields
/// accessor.member("id") = 12345u;
///
/// // Write strings (validates length matches layout)
/// accessor.member("name") = std::string_view("robot");
///
/// // Modify nested fields
/// accessor.member("position.x") = 10.5;
///
/// // Modify array elements
/// accessor.member("tags").item(0) = std::string_view("new_tag");
/// accessor.member("tags")[1] = std::string_view("another");
/// ```
class XCdrAccessor
{
public:
  // Forward declaration of arrow helper (defined after class)
  class AccessorArrowHelper;

  /// Iterator for arrays and sequences.
  /// Note: Does not provide true reference/pointer semantics (returns values instead).
  class Iterator
  {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = XCdrAccessor;
    using difference_type = std::ptrdiff_t;
    // Note: No reference/pointer typedefs - operator* returns by value

    Iterator() = default;
    Iterator(XCdrAccessor & accessor, size_t index);

    XCdrAccessor operator*() const;
    AccessorArrowHelper operator->();  // Returns arrow helper proxy

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
    XCdrAccessor * accessor_ = nullptr;  // Pointer to avoid circular dependency
    size_t index_ = 0;
  };

  /// Wraps a buffer with a layout, validating endianness.
  ///
  /// @param buffer The XCDR buffer (must include header, non-const)
  /// @param layout The layout describing the buffer structure
  /// @return Accessor on success, error if endianness mismatch or invalid buffer
  static XCdrResult<XCdrAccessor> wrap(
    tcb::span<uint8_t> buffer,
    const XCdrStructLayout & layout);

  /// Template overload to accept any container with data() and size()
  template<typename Container,
    typename = std::enable_if_t<
      std::is_convertible_v<decltype(std::declval<Container &>().data()), uint8_t *>&&
      std::is_convertible_v<decltype(std::declval<Container &>().size()), size_t>>>
  static XCdrResult<XCdrAccessor> wrap(
    Container & buffer,
    const XCdrStructLayout & layout)
  {
    return wrap(tcb::span<uint8_t>(buffer.data(), buffer.size()), layout);
  }

  /// Constructs an accessor (validates endianness, throws on mismatch).
  ///
  /// Prefer using wrap() for error handling without exceptions.
  ///
  /// @param buffer The XCDR buffer (non-const)
  /// @param layout The layout describing the buffer
  /// @throws XCdrError if endianness doesn't match system endianness
  XCdrAccessor(
    tcb::span<uint8_t> buffer,
    const XCdrStructLayout & layout);

  /// Accesses a struct field by name or path.
  ///
  /// Supports both simple names ("field") and paths ("outer.inner.field").
  ///
  /// @param name_or_path Field name or dot-separated path
  /// @return Accessor for the field, or error if not found
  XCdrResult<XCdrAccessor> member(std::string_view name_or_path);

  /// Accesses a struct field by name or path (throws on error).
  ///
  /// @param name_or_path Field name or dot-separated path
  /// @return Accessor for the field
  /// @throws XCdrError if field not found
  XCdrAccessor operator[](std::string_view name_or_path);

  /// Accesses an array/sequence element by index.
  ///
  /// @param index Element index (0-based)
  /// @return Accessor for the element, or error if out of bounds
  XCdrResult<XCdrAccessor> item(size_t index);

  /// Accesses an array/sequence element by index (throws on error).
  ///
  /// @param index Element index (0-based)
  /// @return Accessor for the element
  /// @throws XCdrError if out of bounds
  XCdrAccessor operator[](size_t index);

  /// Converts to a primitive type, string_view, or span.
  ///
  /// - Primitives: Returns value by copy
  /// - Strings: Returns std::string_view (zero-copy)
  /// - Primitive arrays/sequences: Returns tcb::span<const T> (zero-copy for read)
  ///
  /// @tparam T Target type
  /// @return Value of type T
  /// @throws XCdrError if type mismatch or not supported
  template<typename T>
  T as() const;

  /// Assigns a primitive value.
  ///
  /// @tparam T Primitive type
  /// @param value Value to assign
  /// @return Reference to this accessor for chaining
  /// @throws XCdrError if type mismatch
  template<typename T>
  XCdrAccessor & operator=(T value);

  /// Assigns a string value.
  ///
  /// Validates that the string length matches the layout.
  ///
  /// @param value String to assign
  /// @return Reference to this accessor for chaining
  /// @throws XCdrError if length mismatch or type mismatch
  XCdrAccessor & operator=(std::string_view value);

  /// Returns the number of elements (for arrays/sequences).
  ///
  /// @return Element count, or error if not an array/sequence
  XCdrResult<size_t> size() const;

  /// Returns iterator to first element (for arrays/sequences).
  ///
  /// @return Iterator to first element
  /// @throws XCdrError if not an array/sequence
  Iterator begin();

  /// Returns iterator past last element (for arrays/sequences).
  ///
  /// @return Iterator past last element
  /// @throws XCdrError if not an array/sequence
  Iterator end();

  /// Returns raw buffer slice for this value (mutable).
  ///
  /// @return Span of bytes representing this value
  tcb::span<uint8_t> slice();

  /// Returns raw buffer slice for this value (const).
  ///
  /// @return Span of bytes representing this value
  tcb::span<const uint8_t> slice() const;

  /// Returns the layout for this accessor.
  ///
  /// @return Reference to the layout
  const XCdrLayout & layout() const;

private:
  /// Internal constructor for creating sub-accessors.
  XCdrAccessor(
    tcb::span<uint8_t> buffer,
    const XCdrLayout * layout,
    size_t base_offset);

  /// Writes a primitive value at the given offset.
  template<typename T>
  void write_primitive(size_t offset, T value);

  /// Reads a primitive value at the given offset.
  template<typename T>
  T read_primitive(size_t offset) const;

  tcb::span<uint8_t> buffer_;
  const XCdrStructLayout * struct_layout_;  // For top-level accessor (wrap)
  const XCdrLayout * layout_;  // For member accessor (polymorphic)
  size_t base_offset_;
};

// Template implementation

template<typename T>
T XCdrAccessor::as() const
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
    // Primitive access (same as const accessor)
    if (!std::holds_alternative<XCdrPrimitiveLayout>(*layout_)) {
      throw XCdrError("Type mismatch: expected primitive layout");
    }
    const auto & prim_layout = std::get<XCdrPrimitiveLayout>(*layout_);

    // Validate primitive kind matches T using type trait
    constexpr XCdrPrimitiveKind expected_kind = primitive_kind_v<T>;

    if (prim_layout.kind() != expected_kind) {
      throw XCdrError("Primitive type mismatch");
    }

    return read_primitive<T>(base_offset_);
  }
}

template<typename T>
XCdrAccessor & XCdrAccessor::operator=(T value)
{
  static_assert(std::is_arithmetic_v<T>, "Assignment supports arithmetic types");

  if (!std::holds_alternative<XCdrPrimitiveLayout>(*layout_)) {
    throw XCdrError("Type mismatch: expected primitive layout for assignment");
  }

  const auto & prim_layout = std::get<XCdrPrimitiveLayout>(*layout_);

  // Validate primitive kind matches T using type trait
  constexpr XCdrPrimitiveKind expected_kind = primitive_kind_v<T>;

  if (prim_layout.kind() != expected_kind) {
    throw XCdrError("Primitive type mismatch in assignment");
  }

  write_primitive(base_offset_, value);
  return *this;
}

template<typename T>
void XCdrAccessor::write_primitive(size_t offset, T value)
{
  static_assert(std::is_arithmetic_v<T>, "write_primitive requires arithmetic type");

  if (offset + sizeof(T) > buffer_.size()) {
    throw XCdrError("Buffer overflow writing primitive");
  }

  write_to_bytes(buffer_.subspan(offset, sizeof(T)), value, kSystemEndianness);
}

template<typename T>
T XCdrAccessor::read_primitive(size_t offset) const
{
  static_assert(std::is_arithmetic_v<T>, "read_primitive requires arithmetic type");

  if (offset + sizeof(T) > buffer_.size()) {
    throw XCdrError("Buffer overflow reading primitive");
  }

  T result;
  read_from_bytes(&result, buffer_.subspan(offset, sizeof(T)), kSystemEndianness);
  return result;
}

// Arrow helper definition (must come after XCdrAccessor is complete)
class XCdrAccessor::AccessorArrowHelper
{
public:
  explicit AccessorArrowHelper(XCdrAccessor accessor)
  : accessor_(std::move(accessor)) {}

  XCdrAccessor * operator->() {return &accessor_;}

private:
  XCdrAccessor accessor_;
};

}  // namespace xcdr_buffers

#endif  // XCDR_BUFFERS__ACCESSOR__ACCESSOR_HPP_
