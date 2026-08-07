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

#include <gtest/gtest.h>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>


#include "rcutils/error_handling.h"
#include "rcutils/types/rcutils_ret.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_cpp/experimental/memory.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "rosidl_typesupport_xcdr_c/message_type_support.h"
#include "rosidl_typesupport_xcdr_cpython/message_type_support.hpp"

// Experimental C++ message types (used only as template arguments for the
// generated typesupport handle specializations).
#include "rosidl_typesupport_xcdr_cpython_tests/msg/experimental/detail/primitives__struct.hpp"
#include "rosidl_typesupport_xcdr_cpython_tests/msg/experimental/detail/strings__struct.hpp"
#include "rosidl_typesupport_xcdr_cpython_tests/msg/experimental/detail/containers__struct.hpp"
#include "rosidl_typesupport_xcdr_cpython_tests/msg/experimental/detail/nested__struct.hpp"
#include "rosidl_typesupport_xcdr_cpython_tests/msg/experimental/detail/inner__struct.hpp"
#include "rosidl_typesupport_xcdr_cpython_tests/msg/experimental/detail/empty__struct.hpp"

namespace py = pybind11;

using rosidl_typesupport_xcdr_cpython::get_message_type_support_handle;

namespace
{

// Test fixture: owns a Python interpreter for the duration of the suite.
class EmbeddedPython : public ::testing::Test
{
protected:
  static void SetUpTestCase()
  {
    interpreter_ = new py::scoped_interpreter();
  }

  static void TearDownTestCase()
  {
    delete interpreter_;
    interpreter_ = nullptr;
  }

  static py::scoped_interpreter * interpreter_;
};

py::scoped_interpreter * EmbeddedPython::interpreter_ = nullptr;

/// Evaluate a message-construction snippet in an isolated namespace.
/**
 * The snippet runs with `__main__` as the globals (so imports resolve) but a
 * fresh locals dict, and must leave the message in a variable named `m`.
 * Isolating locals keeps tests independent (no cross-test leakage through a
 * shared namespace).
 */
py::object
build_message(const char * code)
{
  py::dict locals = py::dict();
  py::exec(code, py::globals(), locals);
  return locals["m"];
}

/// Run a serialize → deserialize round trip and assert Python equality.
/**
 * The message operations go through the canonical C trampolines
 * (rosidl_typesupport_xcdr_c_*) — the universal, language-agnostic API that
 * dispatches through the outer rosidl_message_xcdr_type_support_t installed
 * by the CPython typesupport.
 */
void
roundtrip(const rosidl_message_type_support_t * ts, py::object msg)
{
  ASSERT_NE(ts, nullptr);
  ASSERT_TRUE(!msg.is_none());

  // Compute serialized size.
  size_t size = 0;
  if (rosidl_typesupport_xcdr_c_get_message_size(ts, msg.ptr(), &size) !=
    RCUTILS_RET_OK)
  {
    FAIL() << "get_message_size failed: " << rcutils_get_error_string().str;
  }
  ASSERT_GT(size, 0u);

  // Serialize into a buffer.
  std::vector<uint8_t> buffer(size);
  {
    rosidl_runtime_cpp::MemoryRegion<void> storage(buffer.data(), size);
    if (rosidl_typesupport_xcdr_c_serialize_message_into(
        ts, msg.ptr(), storage.c_region()) != RCUTILS_RET_OK)
    {
      FAIL() << "serialize failed: " << rcutils_get_error_string().str;
    }
  }

  // Deserialize into a fresh instance of the same class.
  py::object msg2 = msg.attr("__class__")();
  {
    rosidl_memory_region_t region{{buffer.data(), 0}, size};
    if (rosidl_typesupport_xcdr_c_deserialize_message_from(
        ts, region, msg2.ptr()) != RCUTILS_RET_OK)
    {
      FAIL() << "deserialize failed: " << rcutils_get_error_string().str;
    }
  }

  // Compare with Python equality.
  bool equal = py::cast<bool>(msg.attr("__eq__")(msg2));
      if (!equal) {
      std::string src_repr = py::cast<std::string>(py::repr(msg));
      std::string dst_repr = py::cast<std::string>(py::repr(msg2));
      FAIL() << "roundtrip mismatch\n  src: " << src_repr << "\n  dst: " << dst_repr;
    }
}

}  // namespace

/// Serialize *msg* into *out* via the canonical C trampolines.
void
serialize_message(
  const rosidl_message_type_support_t * ts, py::object msg, std::vector<uint8_t> & out)
{
  size_t size = 0;
  ASSERT_EQ(rosidl_typesupport_xcdr_c_get_message_size(ts, msg.ptr(), &size), RCUTILS_RET_OK);
  out.resize(size);
  rosidl_runtime_cpp::MemoryRegion<void> storage(out.data(), size);
  ASSERT_EQ(
    rosidl_typesupport_xcdr_c_serialize_message_into(ts, msg.ptr(), storage.c_region()),
    RCUTILS_RET_OK);
}

// ============================================================================
// Phase 4c: zero-copy construct / cast / release
// ============================================================================

TEST_F(EmbeddedPython, ConstructAtStorageZeroCopy)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Primitives>();
  ASSERT_NE(ts, nullptr);

  std::vector<uint8_t> buffer(512);
  void * msg_ptr = nullptr;
  rosidl_memory_region_t region{{buffer.data(), 0}, buffer.size()};
  ASSERT_EQ(rosidl_typesupport_xcdr_c_construct_message_at(ts, region, &msg_ptr), RCUTILS_RET_OK);
  ASSERT_NE(msg_ptr, nullptr);

  py::object m = py::reinterpret_steal<py::object>(static_cast<PyObject *>(msg_ptr));

  // The constructed message is backed by the storage region (zero-copy).
  ASSERT_FALSE(m.attr("_external_storage").is_none());
  ASSERT_EQ(
    py::cast<uintptr_t>(m.attr("_external_storage").attr("block").attr("address")),
    reinterpret_cast<uintptr_t>(buffer.data()));

  // Fill it through the container-aware __setattr__ (pythonic values).
  m.attr("bool_value") = py::cast(true);
  m.attr("int8_value") = py::cast(static_cast<int8_t>(-12));
  m.attr("uint8_value") = py::cast(static_cast<uint8_t>(250));
  m.attr("int16_value") = py::cast(static_cast<int16_t>(-1234));
  m.attr("uint16_value") = py::cast(static_cast<uint16_t>(65000));
  m.attr("int32_value") = py::cast(static_cast<int32_t>(-123456));
  m.attr("uint32_value") = py::cast(static_cast<uint32_t>(4000000000u));
  m.attr("int64_value") = py::cast(static_cast<int64_t>(-1234567890123ll));
  m.attr("uint64_value") = py::cast(static_cast<uint64_t>(12345678901234567890ull));
  m.attr("float32_value") = py::cast(3.14f);
  m.attr("float64_value") = py::cast(2.718281828459045);
  m.attr("char_value") = py::cast(65);

  // Serialized bytes must match a managed message with the same values.
  py::object managed = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._primitives import Primitives
m = Primitives()
m.bool_value = True
m.int8_value = -12
m.uint8_value = 250
m.int16_value = -1234
m.uint16_value = 65000
m.int32_value = -123456
m.uint32_value = 4000000000
m.int64_value = -1234567890123
m.uint64_value = 12345678901234567890
m.float32_value = 3.14
m.float64_value = 2.718281828459045
m.char_value = 65
m
)");
  std::vector<uint8_t> zc, md;
  serialize_message(ts, m, zc);
  serialize_message(ts, managed, md);
  ASSERT_EQ(zc, md);
}

TEST_F(EmbeddedPython, CastReadsWireData)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Primitives>();
  ASSERT_NE(ts, nullptr);

  std::vector<uint8_t> wire;
  py::object src = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._primitives import Primitives
m = Primitives()
m.bool_value = True
m.int8_value = -12
m.uint8_value = 250
m.int16_value = -1234
m.uint16_value = 65000
m.int32_value = -123456
m.uint32_value = 4000000000
m.int64_value = -1234567890123
m.uint64_value = 12345678901234567890
m.float32_value = 3.14
m.float64_value = 2.718281828459045
m.char_value = 65
m
)");
  serialize_message(ts, src, wire);

  void * msg_ptr = nullptr;
  rosidl_memory_region_t region{{wire.data(), 0}, wire.size()};
  ASSERT_EQ(rosidl_typesupport_xcdr_c_cast_message_at(ts, region, &msg_ptr), RCUTILS_RET_OK);
  ASSERT_NE(msg_ptr, nullptr);
  py::object m = py::reinterpret_steal<py::object>(static_cast<PyObject *>(msg_ptr));

  // Backed by the wire buffer; values are readable without copying.
  ASSERT_FALSE(m.attr("_external_storage").is_none());
  ASSERT_EQ(
    py::cast<uintptr_t>(m.attr("_external_storage").attr("block").attr("address")),
    reinterpret_cast<uintptr_t>(wire.data()));
  ASSERT_EQ(py::cast<bool>(m.attr("bool_value").attr("value")), true);
  ASSERT_EQ(py::cast<int32_t>(m.attr("int32_value").attr("value")), -123456);
  ASSERT_EQ(py::cast<uint64_t>(m.attr("uint64_value").attr("value")), 12345678901234567890ull);
  ASSERT_EQ(py::cast<double>(m.attr("float64_value").attr("value")), 2.718281828459045);
  ASSERT_EQ(py::cast<int>(m.attr("char_value").attr("value")), 65);
}

TEST_F(EmbeddedPython, ReleaseReturnsBackingStorage)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Primitives>();
  ASSERT_NE(ts, nullptr);

  std::vector<uint8_t> buffer(512);
  void * msg_ptr = nullptr;
  rosidl_memory_region_t region{{buffer.data(), 0}, buffer.size()};
  ASSERT_EQ(rosidl_typesupport_xcdr_c_construct_message_at(ts, region, &msg_ptr), RCUTILS_RET_OK);
  ASSERT_NE(msg_ptr, nullptr);

  py::object m = py::reinterpret_steal<py::object>(static_cast<PyObject *>(msg_ptr));
  // Relinquish the Python reference: release_message now owns the only ref and
  // consumes the message.
  m.release();
  rosidl_memory_region_t released = rosidl_typesupport_xcdr_c_release_message(ts, msg_ptr);
  ASSERT_EQ(released.location.address, buffer.data());
  ASSERT_GT(released.size, 0u);
}

TEST_F(EmbeddedPython, ConstructEmptyZeroCopy)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Empty>();
  ASSERT_NE(ts, nullptr);

  std::vector<uint8_t> buffer(32);
  void * msg_ptr = nullptr;
  rosidl_memory_region_t region{{buffer.data(), 0}, buffer.size()};
  ASSERT_EQ(rosidl_typesupport_xcdr_c_construct_message_at(ts, region, &msg_ptr), RCUTILS_RET_OK);
  ASSERT_NE(msg_ptr, nullptr);
  py::object m = py::reinterpret_steal<py::object>(static_cast<PyObject *>(msg_ptr));
  ASSERT_FALSE(m.attr("_external_storage").is_none());

  // Round-trip the constructed empty message.
  roundtrip(ts, m);
}

// Cast a message whose fixed_array lives at a known offset in the wire buffer,
// then prove the numpy view is a *live* view over that memory: writes into the
// wire buffer appear in the numpy view, and writes through the numpy view land
// in the wire buffer.
TEST_F(EmbeddedPython, CastNumpyViewReadsLiveWireData)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Containers>();
  ASSERT_NE(ts, nullptr);

  std::vector<uint8_t> wire;
  py::object src = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers
from rosidl_runtime_cpython.string import String
m = Containers()
m.fixed_array = [10, 20, 30, 40]
m.dynamic_seq = [1, 2, 3]
m.bounded_seq = [5, 6]
s1 = String(); s1.assign('a')
s2 = String(); s2.assign('bb')
m.string_seq = [s1, s2]
m
)");
  serialize_message(ts, src, wire);

  void * msg_ptr = nullptr;
  rosidl_memory_region_t region{{wire.data(), 0}, wire.size()};
  ASSERT_EQ(rosidl_typesupport_xcdr_c_cast_message_at(ts, region, &msg_ptr), RCUTILS_RET_OK);
  ASSERT_NE(msg_ptr, nullptr);
  py::object m = py::reinterpret_steal<py::object>(static_cast<PyObject *>(msg_ptr));

  // The numpy view of the cast fixed_array points into the wire buffer.
  py::array arr = py::reinterpret_borrow<py::array>(m.attr("fixed_array").attr("numpy")());
  auto info = arr.request();
  ASSERT_EQ(info.ndim, 1);
  ASSERT_EQ(info.shape[0], 4);
  auto * view = static_cast<int32_t *>(info.ptr);
  ASSERT_EQ(view[0], 10);
  ASSERT_EQ(view[3], 40);
  // Same address as the fixed_array region inside the wire buffer.
  auto * view_bytes = static_cast<const uint8_t *>(info.ptr);
  ptrdiff_t fixed_array_offset = view_bytes - wire.data();
  ASSERT_GE(fixed_array_offset, 0);

  // Write into the wire buffer directly; the numpy view must see the change.
  int32_t * wire_slot = reinterpret_cast<int32_t *>(wire.data() + fixed_array_offset);
  wire_slot[1] = -77;
  ASSERT_EQ(view[1], -77);
  // The container itself reflects the live data too.
  ASSERT_EQ(py::cast<int>(m.attr("fixed_array")[py::int_(1)]), -77);

  // Write through the numpy view; the wire buffer must change.
  view[2] = 12345;
  ASSERT_EQ(wire_slot[2], 12345);
  ASSERT_EQ(py::cast<int>(m.attr("fixed_array")[py::int_(2)]), 12345);

  // Serialization still round-trips with the live values.
  std::vector<uint8_t> rewire;
  serialize_message(ts, m, rewire);
  py::object check = m.attr("__class__")();
  rosidl_memory_region_t r2{{rewire.data(), 0}, rewire.size()};
  ASSERT_EQ(
    rosidl_typesupport_xcdr_c_deserialize_message_from(ts, r2, check.ptr()),
    RCUTILS_RET_OK);
  ASSERT_EQ(py::cast<int>(check.attr("fixed_array")[py::int_(1)]), -77);
  ASSERT_EQ(py::cast<int>(check.attr("fixed_array")[py::int_(2)]), 12345);
}

// get_backing_storage is non-consuming: it reports the region without
// invalidating the message, which remains fully usable afterwards.
TEST_F(EmbeddedPython, GetBackingStorageIsNonConsuming)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Primitives>();
  ASSERT_NE(ts, nullptr);

  std::vector<uint8_t> buffer(512);
  void * msg_ptr = nullptr;
  rosidl_memory_region_t region{{buffer.data(), 0}, buffer.size()};
  ASSERT_EQ(rosidl_typesupport_xcdr_c_construct_message_at(ts, region, &msg_ptr), RCUTILS_RET_OK);
  ASSERT_NE(msg_ptr, nullptr);

  rosidl_memory_region_t backing = rosidl_typesupport_xcdr_c_get_backing_storage(ts, msg_ptr);
  ASSERT_EQ(backing.location.address, buffer.data());
  ASSERT_GT(backing.size, 0u);

  // Message is still alive: fill it and serialize.
  py::object m = py::reinterpret_steal<py::object>(static_cast<PyObject *>(msg_ptr));
  m.attr("int32_value") = py::cast(7);
  std::vector<uint8_t> out;
  serialize_message(ts, m, out);
  ASSERT_GT(out.size(), 0u);

  // Backing query remains stable and non-consuming.
  backing = rosidl_typesupport_xcdr_c_get_backing_storage(ts, msg_ptr);
  ASSERT_EQ(backing.location.address, buffer.data());

  // destroy_message (not release) is the terminal call here.  Relinquish the
  // Python reference first: destroy_message DECREFs the PyObject (consumes the
  // reference construct_message_at returned).  Keeping `m` alive across the
  // call would double-decref (use-after-free).
  m.release();
  rosidl_typesupport_xcdr_c_destroy_message(ts, msg_ptr);
}

// Lifecycle stress under ASAN: exercise the construct→use→release and
// cast→use→destroy handoffs repeatedly.  Refcount errors (double-DECREF,
// using a released message, leaking the PyObject) surface as ASAN
// use-after-free / alloc-dealloc-mismatch / leak reports.
TEST_F(EmbeddedPython, LifecycleStressNoUseAfterFree)
{
  const auto * containers_ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Containers>();
  const auto * primitives_ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Primitives>();
  ASSERT_NE(containers_ts, nullptr);
  ASSERT_NE(primitives_ts, nullptr);

  std::vector<uint8_t> wire;
  py::object src = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers
from rosidl_runtime_cpython.string import String
m = Containers()
m.fixed_array = [1, 2, 3, 4]
m.dynamic_seq = [10, 20, 30]
m.bounded_seq = [7, 8]
s1 = String(); s1.assign('x')
s2 = String(); s2.assign('y')
s3 = String(); s3.assign('z')
m.string_seq = [s1, s2, s3]
m
)");
  serialize_message(containers_ts, src, wire);

  constexpr int kIterations = 50;

  // Construct → write → release (loan handoff).  Primitives is fully bounded,
  // so construct works without a constrained handle.
  for (int i = 0; i < kIterations; ++i) {
    std::vector<uint8_t> buffer(512);
    void * msg_ptr = nullptr;
    rosidl_memory_region_t region{{buffer.data(), 0}, buffer.size()};
    ASSERT_EQ(
      rosidl_typesupport_xcdr_c_construct_message_at(primitives_ts, region, &msg_ptr),
      RCUTILS_RET_OK);
    ASSERT_NE(msg_ptr, nullptr);
    py::object m = py::reinterpret_steal<py::object>(static_cast<PyObject *>(msg_ptr));
    m.attr("int32_value") = py::cast(i);
    // Release path reads the PyObject, extracts the block, DECREFs it.
    m.release();
    rosidl_memory_region_t released = rosidl_typesupport_xcdr_c_release_message(
      primitives_ts, msg_ptr);
    ASSERT_EQ(released.location.address, buffer.data());
    ASSERT_GT(released.size, 0u);
  }

  // Cast → read → destroy (receiver handoff).  Cast parses the layout from
  // the wire, so no constraints are needed.
  for (int i = 0; i < kIterations; ++i) {
    // A fresh copy of the wire each iteration so ASAN redzones guard the
    // exact region the cast message views.
    std::vector<uint8_t> copy = wire;
    void * msg_ptr = nullptr;
    rosidl_memory_region_t region{{copy.data(), 0}, copy.size()};
    ASSERT_EQ(
      rosidl_typesupport_xcdr_c_cast_message_at(containers_ts, region, &msg_ptr),
      RCUTILS_RET_OK);
    ASSERT_NE(msg_ptr, nullptr);
    py::object m = py::reinterpret_steal<py::object>(static_cast<PyObject *>(msg_ptr));
    ASSERT_EQ(py::cast<int>(m.attr("fixed_array")[py::int_(0)]), 1);
    py::array arr = py::reinterpret_borrow<py::array>(m.attr("fixed_array").attr("numpy")());
    auto info = arr.request();
    auto * view = static_cast<const int32_t *>(info.ptr);
    ASSERT_EQ(view[3], 4);
    m.release();
    rosidl_typesupport_xcdr_c_destroy_message(containers_ts, msg_ptr);
  }
}
// ============================================================================
// Phase 4d: constrained handles — construct/cast/validate/compare
// ============================================================================

TEST_F(EmbeddedPython, ConstrainedHandleEnablesConstructCast)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Strings>();
  ASSERT_NE(ts, nullptr);

  // Python Constraints with bounds for the unbounded string members.
  py::object cs = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._strings import Strings
from rosidl_runtime_cpython import StringConstraint
m = Strings.Constraints(text=StringConstraint(size=32), wide=StringConstraint(size=16))
m
)");
  rosidl_message_type_constraints_t constraints{};
  constraints.type_specific = cs.ptr();
  constraints.max_string_length = 64;
  constraints.max_total_size = 0;
  constraints.strict = true;

  auto * constrained = rosidl_typesupport_xcdr_c_create_constrained_message_type_support(
    ts, &constraints);
  ASSERT_NE(constrained, nullptr);

  // Construct at storage with the constrained handle (layout now available).
  std::vector<uint8_t> buffer(512);
  void * msg_ptr = nullptr;
  rosidl_memory_region_t region{{buffer.data(), 0}, buffer.size()};
  ASSERT_EQ(rosidl_typesupport_xcdr_c_construct_message_at(
    constrained, region, &msg_ptr), RCUTILS_RET_OK);
  ASSERT_NE(msg_ptr, nullptr);
  py::object m = py::reinterpret_steal<py::object>(static_cast<PyObject *>(msg_ptr));
  ASSERT_FALSE(m.attr("_external_storage").is_none());

  m.attr("text") = py::cast("hello world");
  m.attr("bounded_text") = py::cast("short");
  m.attr("wide") = py::cast("wider text");
  m.attr("bounded_wide") = py::cast("abcd");

  // Bytes must match a managed message with the same values.
  py::object managed = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._strings import Strings
m = Strings()
m.text = 'hello world'
m.bounded_text = 'short'
m.wide = 'wider text'
m.bounded_wide = 'abcd'
m
)");
  std::vector<uint8_t> zc, md;
  serialize_message(constrained, m, zc);
  serialize_message(constrained, managed, md);
  // The constructed message serializes its actual payload into a max-sized
  // loan buffer (get_message_size fast path returns the block size), while
  // the managed message's size is exactly the payload length.  The actual
  // bytes must be identical.
  ASSERT_GE(zc.size(), md.size());
  ASSERT_TRUE(std::equal(md.begin(), md.end(), zc.begin()));

  // Cast the wire bytes back and read the string (prepopulated size).
  void * cast_ptr = nullptr;
  rosidl_memory_region_t cast_region{{zc.data(), 0}, zc.size()};
  ASSERT_EQ(rosidl_typesupport_xcdr_c_cast_message_at(
    constrained, cast_region, &cast_ptr), RCUTILS_RET_OK);
  ASSERT_NE(cast_ptr, nullptr);
  py::object cast_msg = py::reinterpret_steal<py::object>(static_cast<PyObject *>(cast_ptr));
  ASSERT_EQ(py::cast<std::string>(cast_msg.attr("text").attr("__str__")()), "hello world");
  ASSERT_EQ(py::cast<std::string>(cast_msg.attr("wide").attr("__str__")()), "wider text");

  rosidl_typesupport_xcdr_c_destroy_constrained_message_type_support(constrained);
}

TEST_F(EmbeddedPython, ConstructFailsWithoutConstraints)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Strings>();
  ASSERT_NE(ts, nullptr);

  // The unconstrained handle has no cached layout for a message with
  // variable-length members — construct must fail (as in the C++ reference).
  std::vector<uint8_t> buffer(512);
  void * msg_ptr = nullptr;
  rosidl_memory_region_t region{{buffer.data(), 0}, buffer.size()};
  ASSERT_EQ(rosidl_typesupport_xcdr_c_construct_message_at(ts, region, &msg_ptr),
    RCUTILS_RET_ERROR);
}

TEST_F(EmbeddedPython, ValidatePassesWithinBounds)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Containers>();
  ASSERT_NE(ts, nullptr);

  py::object cs = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers
from rosidl_runtime_cpython import SequenceConstraint, StringConstraint
m = Containers.Constraints(
    dynamic_seq=SequenceConstraint(size=8),
    string_seq=SequenceConstraint(size=4, element=StringConstraint(size=16)))
m
)");
  rosidl_message_type_constraints_t constraints{};
  constraints.type_specific = cs.ptr();
  constraints.max_string_length = 64;
  constraints.max_total_size = 0;
  constraints.strict = true;

  py::object msg = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers
m = Containers()
m.fixed_array = [1, 2, 3, 4]
m.dynamic_seq = [10, 20, 30]
m.bounded_seq = [1, 2, 3]
m.string_seq = ['alpha', 'beta']
m
)");

  std::vector<std::string> reported;
  auto report_cb = [](void * ud, const char * path, int /*code*/) {
      static_cast<std::vector<std::string> *>(ud)->emplace_back(path);
    };
  ASSERT_EQ(rosidl_typesupport_xcdr_c_validate_message(
    ts, &constraints, msg.ptr(), report_cb, &reported), RCUTILS_RET_OK);
  ASSERT_TRUE(reported.empty());
}

TEST_F(EmbeddedPython, ValidateFailsBeyondBounds)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Containers>();
  ASSERT_NE(ts, nullptr);

  py::object cs = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers
from rosidl_runtime_cpython import SequenceConstraint, StringConstraint
m = Containers.Constraints(
    dynamic_seq=SequenceConstraint(size=2),
    string_seq=SequenceConstraint(size=4, element=StringConstraint(size=4)))
m
)");
  rosidl_message_type_constraints_t constraints{};
  constraints.type_specific = cs.ptr();
  constraints.max_string_length = 64;
  constraints.max_total_size = 0;
  constraints.strict = true;

  py::object msg = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers
m = Containers()
m.dynamic_seq = [1, 2, 3]            # 3 > bound 2
m.string_seq = ['toolongstr']        # 10 > element bound 4
m
)");

  std::vector<std::string> reported;
  auto report_cb = [](void * ud, const char * path, int /*code*/) {
      static_cast<std::vector<std::string> *>(ud)->emplace_back(path);
    };
  ASSERT_EQ(rosidl_typesupport_xcdr_c_validate_message(
    ts, &constraints, msg.ptr(), report_cb, &reported), RCUTILS_RET_ERROR);
  ASSERT_FALSE(reported.empty());
  // The first failing field is reported with a dot-separated path.
  ASSERT_EQ(reported[0], "dynamic_seq");
}

TEST_F(EmbeddedPython, CompareConstraintsDetectsLooser)
{
  const auto * ts = get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Containers>();
  ASSERT_NE(ts, nullptr);

  py::object baseline_obj = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers
from rosidl_runtime_cpython import SequenceConstraint, StringConstraint
m = Containers.Constraints(
    dynamic_seq=SequenceConstraint(size=4),
    string_seq=SequenceConstraint(size=4, element=StringConstraint(size=8)))
m
)");
  py::object looser_obj = build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers
from rosidl_runtime_cpython import SequenceConstraint, StringConstraint
m = Containers.Constraints(
    dynamic_seq=SequenceConstraint(size=16),
    string_seq=SequenceConstraint(size=4, element=StringConstraint(size=8)))
m
)");

  rosidl_message_type_constraints_t baseline{};
  baseline.type_specific = baseline_obj.ptr();
  baseline.max_string_length = 64;
  baseline.max_total_size = 0;
  baseline.strict = true;

  rosidl_message_type_constraints_t looser{};
  looser.type_specific = looser_obj.ptr();
  looser.max_string_length = 64;
  looser.max_total_size = 0;
  looser.strict = true;

  // A looser candidate (bigger dynamic_seq bound) is incompatible with the
  // baseline; an identical candidate is compatible.
  ASSERT_FALSE(rosidl_typesupport_xcdr_c_compare_constraints(
    ts, &looser, &baseline, nullptr, nullptr));
  ASSERT_TRUE(rosidl_typesupport_xcdr_c_compare_constraints(
    ts, &baseline, &baseline, nullptr, nullptr));
}


TEST_F(EmbeddedPython, PrimitivesRoundtrip)
{
  // Uses the container-aware __setattr__: plain values assigned to fields.
  py::object msg =
    build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._primitives import Primitives
m = Primitives()
m.bool_value = True
m.int8_value = -12
m.uint8_value = 250
m.int16_value = -1234
m.uint16_value = 65000
m.int32_value = -123456
m.uint32_value = 4000000000
m.int64_value = -1234567890123
m.uint64_value = 12345678901234567890
m.float32_value = 3.14
m.float64_value = 2.718281828459045
m.char_value = 65
m
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Primitives>();
  roundtrip(ts, msg);
}

TEST_F(EmbeddedPython, StringsRoundtrip)
{
  py::object msg =
    build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._strings import Strings
m = Strings()
m.text = 'hello world'
m.bounded_text = 'short'
m.wide = 'wider text'
m.bounded_wide = 'abcd'
m
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Strings>();
  roundtrip(ts, msg);
}

TEST_F(EmbeddedPython, ContainersRoundtrip)
{
  py::object msg =
    build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers
from rosidl_runtime_cpython.string import String
m = Containers()
m.fixed_array = [1, 2, 3, 4]
m.dynamic_seq = [10, 20, 30]
m.bounded_seq = [1, 2, 3, 4, 5]
s1 = String()
s1.assign('alpha')
s2 = String()
s2.assign('beta')
m.string_seq = [s1, s2]
m
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Containers>();
  roundtrip(ts, msg);
}

TEST_F(EmbeddedPython, NestedRoundtrip)
{
  py::object msg =
    build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._nested import Nested
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._inner import Inner
m = Nested()
m.name = 'outer'
m.inner.value = 7
m.inner.label = 'inner label'
e1 = Inner()
e1.value = 1
e1.label = 'one'
e2 = Inner()
e2.value = 2
e2.label = 'two'
m.inner_seq = [e1, e2]
m
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Nested>();
  roundtrip(ts, msg);
}

TEST_F(EmbeddedPython, EmptyRoundtrip)
{
  py::object msg =
    build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._empty import Empty
m = Empty()
m
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Empty>();
  roundtrip(ts, msg);
}

TEST_F(EmbeddedPython, ManagedAssignmentIsShallow)
{
  py::object msg =
    build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._nested import Nested
m = Nested()
m.inner.value = 7
b = Nested()
b.inner = m.inner          # managed receiver -> shallow reference assignment
assert b.inner is m.inner
assert b.inner.value == 7
# mutating through the alias is visible on both (reference semantics)
b.inner.value = 99
assert m.inner.value == 99
m
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Nested>();
  roundtrip(ts, msg);
}

TEST_F(EmbeddedPython, ExternalAssignmentIsDeepInPlace)
{
  py::object msg =
    build_message(
    R"(
from rosidl_runtime_cpython import MessageInitialization
from rosidl_runtime_cpython._raw_buffer import RawBuffer
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._inner import Inner

# External-backed receiver: containers are views into fixed RawBuffers.
ext = Inner.ExternalStorage()
ext.block = RawBuffer(64, growing=False)
ext.members.value = RawBuffer(4, growing=False)
ext.members.label = RawBuffer(16, growing=False)
r = Inner(_storage=ext, _init=MessageInitialization.SKIP)
orig_scalar = r.value
orig_label = r.label
assert r._external_storage is not None

src = Inner()
src.value = 42
src.label = 'copied'

r.value = src.value        # Scalar -> deep in-place
r.label = src.label        # String -> deep in-place (via __buffer__)

assert r.value is orig_scalar      # container identity preserved
assert r.label is orig_label
assert r.value.value == 42
assert r.label == 'copied'
assert src.value == 42             # source untouched
m = r
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Inner>();
  roundtrip(ts, msg);
}

TEST_F(EmbeddedPython, ExternalMessageSequencePoolPreservesBacking)
{
  py::object msg =
    build_message(
    R"(
from rosidl_runtime_cpython import MessageInitialization
from rosidl_runtime_cpython._raw_buffer import RawBuffer
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._nested import Nested
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._inner import Inner

ext = Nested.ExternalStorage()
ext.block = RawBuffer(512, growing=False)
slots = []
for _ in range(2):
    s = Inner.ExternalStorage()
    s.block = RawBuffer(64, growing=False)
    s.members.value = RawBuffer(4, growing=False)
    s.members.label = RawBuffer(16, growing=False)
    slots.append(s)
ext.members.inner_seq = slots
m = Nested(_storage=ext, _init=MessageInitialization.SKIP)
seq = m.inner_seq
assert seq.capacity == 2
assert len(seq) == 0

e1 = Inner()
e1.value = 1
e1.label = 'one'
e2 = Inner()
e2.value = 2
e2.label = 'two'
m.inner_seq = [e1, e2]                # deep in-place into pool elements
first = seq[0]
second = seq[1]
assert first.value.value == 1
assert second.label == 'two'
assert seq[0] is first               # pool identity preserved
assert seq[1] is second

# Shrink clears the excess pool element via the generated message clear().
m.inner_seq = [e1]
assert len(seq) == 1
assert len(second.label) == 0

# Growing must not resurrect stale content.
seq.resize(2)
assert seq[1] is second
assert second.value.value == 0
assert second.label == ''
m
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Nested>();
  roundtrip(ts, msg);
}

TEST_F(EmbeddedPython, TypeMismatchRejected)
{
  py::exec(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._primitives import Primitives
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._inner import Inner
m = Primitives()
try:
    m.int32_value = Inner()      # message into a Scalar field
    raised = False
except TypeError:
    raised = True
assert raised, 'expected TypeError for scalar <- message'

try:
    m.int32_value = 'abc'        # non-numeric str into a Scalar field
    raised = False
except (TypeError, ValueError):
    raised = True
assert raised, 'expected error for scalar <- non-numeric str'
)");
}

TEST_F(EmbeddedPython, SequenceAssignReplacesContents)
{
  py::object msg =
    build_message(
    R"(
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers
m = Containers()
m.dynamic_seq = [1, 2, 3]
assert list(m.dynamic_seq) == [1, 2, 3]
m.dynamic_seq = [9]                # assign replaces, does not append
assert list(m.dynamic_seq) == [9]
m
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Containers>();
  roundtrip(ts, msg);
}

TEST_F(EmbeddedPython, ExternalSequencePoolPreservesBacking)
{
  py::object msg =
    build_message(
    R"(
from rosidl_runtime_cpython import MessageInitialization
from rosidl_runtime_cpython._raw_buffer import RawBuffer
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers

# External-backed receiver: string_seq has a fixed pool of 2 slots.
ext = Containers.ExternalStorage()
ext.block = RawBuffer(256, growing=False)
ext.members.string_seq = [RawBuffer(16, growing=False), RawBuffer(16, growing=False)]
m = Containers(_storage=ext, _init=MessageInitialization.SKIP)
seq = m.string_seq
assert seq.capacity == 2
assert len(seq) == 0              # empty by default; the pool provides capacity

# Pythonic list assignment -> deep in-place into the pool elements.
m.string_seq = ['alpha', 'beta']
assert len(seq) == 2
first = seq[0]
second = seq[1]

# Re-assignment preserves pool element identity (external backing).
m.string_seq = ['x', 'y']
assert seq[0] is first
assert seq[1] is second
assert str(first) == 'x'

# Appending beyond the pool raises BufferError.
try:
    seq.append('third')
    raised = False
except BufferError:
    raised = True
assert raised, 'expected BufferError appending beyond pool capacity'
m
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Containers>();
  roundtrip(ts, msg);
}

TEST_F(EmbeddedPython, ExternalSequenceDeepcopyPreservesBacking)
{
  py::object msg =
    build_message(
    R"(
from rosidl_runtime_cpython import MessageInitialization
from rosidl_runtime_cpython._raw_buffer import RawBuffer
from rosidl_typesupport_xcdr_cpython_tests.msg.experimental._containers import Containers

ext = Containers.ExternalStorage()
ext.block = RawBuffer(256, growing=False)
ext.members.string_seq = [RawBuffer(16, growing=False), RawBuffer(16, growing=False)]
r = Containers(_storage=ext, _init=MessageInitialization.SKIP)
seq = r.string_seq

src = Containers()
src.string_seq = ['deep', 'copy']

# Assigning a same-type sequence deep-copies into the receiver's pool.
r.string_seq = src.string_seq
assert len(seq) == 2
assert str(seq[0]) == 'deep'
assert str(seq[1]) == 'copy'
assert seq[0] is not src.string_seq[0]     # deep, not aliased

# External backing survives a shorter re-assignment.
e0 = seq[0]
r.string_seq = ['again']
assert seq[0] is e0
assert len(seq) == 1
assert str(seq[0]) == 'again'
m = r
)");
  const auto * ts =
    get_message_type_support_handle<
    rosidl_typesupport_xcdr_cpython_tests::msg::experimental::Containers>();
  roundtrip(ts, msg);
}
