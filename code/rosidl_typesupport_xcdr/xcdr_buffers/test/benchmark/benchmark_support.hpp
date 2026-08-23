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

#ifndef BENCHMARK__BENCHMARK_SUPPORT_HPP_
#define BENCHMARK__BENCHMARK_SUPPORT_HPP_

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <string>
#include <string_view>
#include <vector>

#include <tcb_span/span.hpp>

#include "xcdr_buffers/common/endianness.hpp"
#include "xcdr_buffers/layout/layout.hpp"
#include "xcdr_buffers/layout/layout_builder.hpp"
#include "xcdr_buffers/layout/layout_parser.hpp"
#include "xcdr_buffers/serialization/writer.hpp"

namespace xcdr_buffers
{
namespace benchmark_support
{

/// Fixed arena size for PMR-backed layout build/parse benchmarks.
///
/// The schema below has a constant number of fields (5), so arena usage is
/// bounded by the number of fields, not by element counts: one-shot
/// allocate_primitive_sequence()/allocate_primitive_array() create a single
/// layout object regardless of count. 64 KiB is far more than the few KiB
/// the schema needs, while still fitting comfortably in L1/L2 so the
/// monotonic resource itself does not dominate the measurement.
constexpr size_t kArenaBytes = 64 * 1024;

/// Skips the benchmark on big-endian hosts.
///
/// Payloads are generated little-endian, and the accessor wrap() and the
/// reader's span reads reject (or mis-handle) buffers whose endianness does
/// not match the host. Rather than silently measuring error paths, fail
/// loudly.
inline bool skip_on_big_endian(benchmark::State & state)
{
  if (kSystemEndianness != XCdrEndianness::kLittleEndian) {
    state.SkipWithError("benchmarks assume little-endian payloads");
    return true;
  }
  return false;
}

/// Schema exercised by the benchmarks:
///   struct {
///     uint32 id;
///     string name;                       // name_len chars, excluding null
///     struct { double x; double y; } pos;
///     sequence<uint32> values;           // seq_count elements
///     array<uint8> data;                 // array_count elements
///   }
struct SchemaSpec
{
  size_t name_len = 8;       // chars, excluding null terminator
  size_t seq_count = 64;     // uint32 sequence elements
  size_t array_count = 256;  // uint8 array elements
};

/// Builds the schema layout with the given memory resource.
inline XCdrStructLayout build_layout(
  const SchemaSpec & spec,
  std::pmr::memory_resource * mr)
{
  XCdrLayoutBuilder builder(XCdrEndianness::kLittleEndian, mr);
  builder.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
  builder.allocate_string("name", spec.name_len);
  builder.begin_allocate_struct("pos");
  builder.allocate_primitive("x", XCdrPrimitiveKind::kDouble);
  builder.allocate_primitive("y", XCdrPrimitiveKind::kDouble);
  builder.end_allocate_struct();
  builder.allocate_primitive_sequence(
    "values", XCdrPrimitiveKind::kUint32, spec.seq_count);
  builder.allocate_primitive_array("data", XCdrPrimitiveKind::kUint8, spec.array_count);
  return builder.finalize();
}

/// Writes a payload matching the schema with the given memory resource.
inline std::pmr::vector<uint8_t> write_payload(
  const SchemaSpec & spec,
  std::pmr::memory_resource * mr)
{
  XCdrWriter writer(XCdrEndianness::kLittleEndian, mr);

  writer.write(static_cast<uint32_t>(42));
  writer.write(std::string_view(std::string(spec.name_len, 'n')));
  writer.begin_write_struct();
  writer.write(static_cast<double>(1.5));
  writer.write(static_cast<double>(2.5));
  writer.end_write_struct();

  std::vector<uint32_t> values(spec.seq_count, 7u);
  writer.write_sequence(tcb::span<const uint32_t>(values));

  std::vector<uint8_t> data(spec.array_count, static_cast<uint8_t>(0xAB));
  writer.write_array(tcb::span<const uint8_t>(data));

  return writer.flush();
}

/// Parses a payload matching the schema, producing a layout.
///
/// The walk mirrors build_layout()/write_payload() field-for-field so the
/// parser sees exactly the bytes the writer produced.
inline XCdrResult<XCdrStructLayout> parse_payload(
  tcb::span<const uint8_t> buffer,
  const SchemaSpec & spec,
  std::pmr::memory_resource * mr)
{
  XCdrLayoutParser parser(buffer, mr);

  auto status = parser.parse_primitive(XCdrPrimitiveKind::kUint32);
  if (!status) {
    return tl::unexpected(status.error());
  }
  status = parser.parse_string(XCdrCharKind::kChar8);
  if (!status) {
    return tl::unexpected(status.error());
  }
  status = parser.begin_parse_struct("pos");
  if (!status) {
    return tl::unexpected(status.error());
  }
  status = parser.parse_primitive(XCdrPrimitiveKind::kDouble);
  if (!status) {
    return tl::unexpected(status.error());
  }
  status = parser.parse_primitive(XCdrPrimitiveKind::kDouble);
  if (!status) {
    return tl::unexpected(status.error());
  }
  status = parser.end_parse_struct();
  if (!status) {
    return tl::unexpected(status.error());
  }
  status = parser.parse_primitive_sequence(
    XCdrPrimitiveKind::kUint32, spec.seq_count);
  if (!status) {
    return tl::unexpected(status.error());
  }
  status = parser.parse_primitive_array(
    XCdrPrimitiveKind::kUint8, spec.array_count);
  if (!status) {
    return tl::unexpected(status.error());
  }
  return parser.finalize();
}

}  // namespace benchmark_support
}  // namespace xcdr_buffers

#endif  // BENCHMARK__BENCHMARK_SUPPORT_HPP_
