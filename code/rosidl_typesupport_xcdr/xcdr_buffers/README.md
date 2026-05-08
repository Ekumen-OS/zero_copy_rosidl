# xcdr_buffers

High-performance XCDRv1 (DDS CDR) serialization library for ROS 2 with zero-copy buffer access.

## Overview

`xcdr_buffers` provides efficient, type-safe serialization compatible with the DDS Common Data Representation (CDR) standard. The library offers two complementary approaches:

1. **Streaming API**: Simple serialization and deserialization with `XCdrWriter` and `XCdrReader`
2. **Zero-Copy API**: Direct buffer access with `XCdrAccessor` for high-performance scenarios

### Key Features

- **FastCDR Compatible**: Full interoperability with eProsima's FastCDR library
- **CDR-Compliant Alignment**: Data-relative alignment matching DDS CDR specification
- **Type-Safe**: Compile-time type checking with explicit error handling
- **Zero-Copy**: Direct buffer access without deserialization overhead
- **Modern C++17**: Uses `std::variant`, `std::optional`, result types, and span
- **Polymorphic Memory Resources**: Customizable allocators via `std::pmr`

### Supported Types

- **Primitives**: All standard CDR types (bool, int8-64, uint8-64, float32/64)
- **Strings**: UTF-8 and UTF-16 with zero-copy views
- **Arrays**: Fixed-size homogeneous collections
- **Sequences**: Dynamic-size collections
- **Structs**: Nested composite types with named members

## Quick Start

### Streaming Serialization

Write and read data with automatic alignment and type safety:

```cpp
#include "xcdr_buffers/serialization/writer.hpp"
#include "xcdr_buffers/serialization/reader.hpp"

// Serialize data
XCdrWriter writer;
writer.write<uint32_t>(42);
writer.write("Hello, CDR!");
writer.write<double>(3.14159);

std::vector<int32_t> numbers = {1, 2, 3, 4, 5};
writer.write_sequence(numbers);

auto buffer = writer.flush();

// Deserialize data (header read automatically on construction)
XCdrReader reader(buffer);  // Throws XCdrError if header invalid

auto id = reader.read<uint32_t>();          // Returns XCdrResult<uint32_t>
auto msg = reader.read<std::string_view>(); // Zero-copy string view
auto pi = reader.read<double>();

// Read sequence as span (zero-copy for matching endianness)
auto nums = reader.read<tcb::span<const int32_t>>();

// Or use wrap() for explicit error handling (no exceptions)
auto reader_result = XCdrReader::wrap(buffer);
if (!reader_result) {
  std::cerr << "Invalid buffer: " << reader_result.error().message() << std::endl;
  return;
}
auto& reader = *reader_result;
```

### Unified read<T>() API

The `read<T>()` method adapts to the requested type:

```cpp
// Primitives - returns by value
auto value = reader.read<uint32_t>();

// Strings - zero-copy view into buffer
auto str = reader.read<std::string_view>();

// Allocating string - copies data
auto owned_str = reader.read<std::string>();

// Sequences - zero-copy span (if endianness matches)
auto span = reader.read<tcb::span<const double>>();

// Allocating vector - copies data
auto vec = reader.read<std::vector<double>>();
```

### Zero-Copy Buffer Access

Access and modify serialized data directly without deserialization:

```cpp
#include "xcdr_buffers/accessor/accessor.hpp"
#include "xcdr_buffers/layout/layout_builder.hpp"

// Define the data structure layout
XCdrLayoutBuilder builder;
builder.allocate_primitive("id", XCdrPrimitiveKind::kUint32);
builder.allocate_string("name", 20);  // Max 20 characters
builder.begin_allocate_struct("position");
  builder.allocate_primitive("x", XCdrPrimitiveKind::kDouble);
  builder.allocate_primitive("y", XCdrPrimitiveKind::kDouble);
builder.end_allocate_struct();
auto layout = builder.finalize();

// Create buffer with XCDR header
std::vector<uint8_t> buffer(layout.total_size());
layout.apply(buffer);  // Writes header and initializes structure

// Zero-copy read/write access
auto accessor_result = XCdrAccessor::wrap(buffer, layout);
ASSERT_TRUE(accessor_result);
auto accessor = *accessor_result;

// Write using member names or indices
accessor["id"] = 12345u;
accessor["name"] = std::string_view("robot");
accessor["position"]["x"] = 10.5;
accessor["position"]["y"] = 20.3;

// Read values (zero-copy for strings)
uint32_t id = accessor["id"].as<uint32_t>();
std::string_view name = accessor["name"].as<std::string_view>();
double x = accessor["position"]["x"].as<double>();
```

### Layout Discovery from Buffers

Parse layouts from existing serialized buffers:

```cpp
#include "xcdr_buffers/layout/layout_parser.hpp"

// Given a serialized buffer from an external source
XCdrLayoutParser parser(buffer);

// Describe the expected structure
parser.parse_primitive(XCdrPrimitiveKind::kUint32);   // id field
parser.parse_string();                                // name field
parser.begin_parse_struct("position");
  parser.parse_primitive(XCdrPrimitiveKind::kDouble); // x
  parser.parse_primitive(XCdrPrimitiveKind::kDouble); // y
parser.end_parse_struct();

auto layout = parser.finalize();  // Now you have a reusable layout

// Use layout for zero-copy access
auto accessor = XCdrConstAccessor::wrap(buffer, *layout);
```

## Architecture

### Design Principles

1. **Type Safety First**
   - Separate layout types for primitives, strings, arrays, sequences, and structs
   - Compile-time type checking prevents misuse
   - No "can be anything" generic types

2. **Explicit Error Handling**
   - Methods return `XCdrResult<T>` or `XCdrStatus` for explicit error checking
   - Operators and implicit conversions throw exceptions for convenience
   - User chooses safety vs. ergonomics per use case

3. **Zero-Copy Philosophy**
   - Readers return `std::string_view` and `tcb::span<const T>` when possible
   - Accessors provide direct buffer access without intermediate copies
   - Layouts store relative offsets for reusability and composition

4. **Two Complementary APIs**
   - **Streaming**: When you need to serialize/deserialize data sequentially
   - **Zero-Copy**: When you need random access to fields without copying

5. **Implicit Top-Level Struct & Header**
   - Writers, readers, parsers, and builders start in an implicit root struct context
   - `XCdrReader` automatically validates XCDR header on construction
   - Eliminates boilerplate for simple use cases
   - Nested structs still use explicit delimiters

### FastCDR Interoperability

The library is fully compatible with eProsima's FastCDR (DDS CDR implementation):

```cpp
// Write with xcdr_buffers, read with FastCDR
XCdrWriter writer;
writer.write<uint32_t>(42);
writer.write<double>(3.14);
auto buffer = writer.flush();

// Skip 4-byte XCDR header for FastCDR (DDS_CDR mode doesn't include header)
const uint8_t* data = buffer.data() + kXCdrHeaderSize;
size_t size = buffer.size() - kXCdrHeaderSize;

eprosima::fastcdr::FastBuffer fb(const_cast<char*>(reinterpret_cast<const char*>(data)), size);
eprosima::fastcdr::Cdr cdr(fb, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::DDS_CDR);

uint32_t value1;
double value2;
cdr >> value1 >> value2;  // Reads correctly
```

**Key Compatibility Notes:**
- Both use CDR data-relative alignment (offsets relative to data start, not buffer start)
- XCdr includes 4-byte XCDR header; FastCDR's DDS_CDR mode does not
- Call `cdr.serialize_encapsulation()` when writing with FastCDR for XCdrReader compatibility

## Implementation Details

### XCDR Header Format

All buffers start with a 4-byte header:
```
[0x00, encoding_flag, 0x00, 0x00]
```
- Byte 0: Representation options (0x00 = PLAIN_CDR)
- Byte 1: Encoding flag (0x01 = little endian, 0x00 = big endian)
- Bytes 2-3: Reserved (always 0x00)

### CDR Alignment Rules

Alignment is **data-relative** (relative to position after the 4-byte header for top-level, relative to struct start for nested structs):

- **Primitives**: Align to their size (1, 2, 4, 8 bytes)
- **Strings**: Align to 4 bytes (for length prefix)
- **Sequences**: Align to 4 bytes (for length prefix), then elements align individually
- **Arrays**: Align to element alignment
- **Structs**: Align to maximum member alignment

### String Representation

- **Wire format**: Length prefix includes null terminator
- **API semantics**: `actual_length` parameter excludes null terminator (matches C++ `std::string`)
- **Zero-copy**: `XCdrReader::read<std::string_view>()` returns view into buffer without copying

### Memory Management

- **PMR Support**: All layout classes accept `std::pmr::memory_resource*` with `nullptr` defaulting to `std::pmr::new_delete_resource()`
- **Accessor Lifetime**: Accessors are lightweight views; buffer and layout must outlive accessor
- **Buffer Ownership**: `XCdrWriter::flush()` moves buffer ownership to caller and resets writer state

## Dependencies

- **ROS 2**: Rolling, Jazzy, or newer
- **tl_expected**: Result/error handling (`XCdrResult<T>`)
- **tcb_span**: Lightweight array views (`tcb::span<T>`)
- **FastCDR** (test only): Interoperability validation

All dependencies are available as ROS 2 packages via rosdep.

## Building

This package uses `ament_cmake` and integrates with standard ROS 2 workflows:

```bash
# In your ROS 2 workspace
cd /path/to/ros2_ws
colcon build --packages-select xcdr_buffers
```

## Testing

The library includes a comprehensive test suite (117 tests):

```bash
# Run all tests
colcon test --packages-select xcdr_buffers

# View detailed results
colcon test-result --verbose --test-result-base build/xcdr_buffers
```

**Test Coverage:**
- **Unit tests** (96 tests): Writer, Reader, Layout Builder, Layout Parser, Accessors
- **Integration tests** (14 tests): End-to-end workflows, Writer→Parser→Accessor
- **Interoperability tests** (7 tests): Full compatibility with FastCDR

All tests pass with 100% success rate.

## Choosing the Right API

### Use Streaming API When:
- Serializing/deserializing entire messages sequentially
- Structure is known at compile time
- Simple workflow: write once, read once
- Integration with existing serialization pipelines

### Use Zero-Copy API When:
- Need random access to specific fields
- Modifying only parts of large messages
- High-performance scenarios where copying is expensive
- Working with shared memory or memory-mapped files
- Structure may vary or is discovered at runtime

### Use Layout Parser When:
- Receiving buffers from external sources
- Need to inspect unknown CDR data
- Building debugging/inspection tools
- Creating reusable layouts from example data

## API Reference

### Core Classes

- **`XCdrWriter`**: Streaming serialization with automatic alignment
- **`XCdrReader`**: Streaming deserialization with automatic header validation
  - Constructor reads header automatically and throws `XCdrError` on failure
  - Static `wrap()` factory for explicit error handling without exceptions
- **`XCdrLayoutBuilder`**: Construct type-safe layouts programmatically
- **`XCdrLayoutParser`**: Discover layouts from serialized buffers
- **`XCdrAccessor`**: Read-write zero-copy buffer access
- **`XCdrConstAccessor`**: Read-only zero-copy buffer access

### Result Types

- **`XCdrResult<T>`**: Success with value `T` or error
- **`XCdrStatus`**: Success/failure without value
- **`XCdrError`**: Error with message

Both support standard result/optional patterns:
```cpp
auto result = reader.read<uint32_t>();
if (result) {
  uint32_t value = *result;  // Dereference to get value
} else {
  std::cerr << result.error().message() << std::endl;
}
```

## Contributing

Contributions are welcome! Please ensure:
- All tests pass (`colcon test --packages-select xcdr_buffers`)
- Code follows existing style (checked by `ament_cpplint`)
- New features include unit tests
- Public APIs are documented

## Resources

- [DDS XTypes Specification](https://www.omg.org/spec/DDS-XTypes/) - Official OMG DDS XTypes standard
- [eProsima FastCDR](https://github.com/eProsima/Fast-CDR) - Reference CDR implementation
- [ROS 2 Documentation](https://docs.ros.org/) - ROS 2 resources

## License

Apache License 2.0 - See LICENSE file for details.

## Maintainer

Michel Hidalgo - [michel@ekumenlabs.com](mailto:michel@ekumenlabs.com)
