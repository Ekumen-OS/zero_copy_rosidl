// Copyright 2026 Ekumen, Inc.
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

/// @file raw_buffer.h
/// @brief C API for creating external-memory RawBuffer objects from C/C++.
///
/// Other C extensions (e.g. rosidl_generator_py) that need to wrap a
/// rosidl_memory_region_t as a Python RawBuffer should:
///
///   #include "rosidl_runtime_py/raw_buffer.h"
///
///   // Once, at module init time:
///   if (RawBuffer_ImportCAPI() < 0) { return NULL; }
///
///   // Then, when wrapping a region:
///   PyObject * buf = RawBuffer_FromRegion(&my_region);
///
/// The macro imports the PyCapsule from _rosidl_runtime_py and caches the
/// function pointer in a static variable — the same pattern numpy uses for
/// its C API.

#ifndef ROSIDL_RUNTIME_PY__RAW_BUFFER_H_
#define ROSIDL_RUNTIME_PY__RAW_BUFFER_H_

#include <Python.h>
#include "rosidl_runtime_c/experimental/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Capsule name exported by rosidl_runtime_py._raw_buffer.
#define ROSIDL_RUNTIME_PY_RAW_BUFFER_CAPSULE_NAME \
  "rosidl_runtime_py._raw_buffer._C_API"

/// @brief Function pointer type for RawBuffer_FromRegion.
typedef PyObject * (* RawBuffer_FromRegion_t)(const rosidl_memory_region_t *);

/// @brief C API struct exported via PyCapsule.
///
/// A pointer to a static instance of this struct is stored in the capsule.
/// Callers retrieve it with PyCapsule_Import and cast to RawBuffer_CAPI_t * —
/// a data-pointer cast, valid in both C and C++.
typedef struct {
  RawBuffer_FromRegion_t FromRegion;
} RawBuffer_CAPI_t;

/// @brief Cached pointer populated by RawBuffer_ImportCAPI().
static const RawBuffer_CAPI_t * _rosidl_runtime_py_CAPI = NULL;

/// @brief Import the C API from _rosidl_runtime_py.  Call once at init.
/// @return 0 on success, -1 on failure (Python exception is set).
static inline int
RawBuffer_ImportCAPI(void)
{
  if (_rosidl_runtime_py_CAPI != NULL) {
    return 0;
  }
  // PyCapsule_Import returns void * directly — not a PyObject *.
  _rosidl_runtime_py_CAPI = (const RawBuffer_CAPI_t *)PyCapsule_Import(
    ROSIDL_RUNTIME_PY_RAW_BUFFER_CAPSULE_NAME, 0);
  return (_rosidl_runtime_py_CAPI != NULL) ? 0 : -1;
}

/// @brief Wrap a rosidl_memory_region_t as a non-owning external RawBuffer.
///
/// The returned RawBuffer holds a copy of *region and does NOT own the
/// underlying memory.  The region must outlive the returned Python object.
/// Both managed and external RawBuffers are writable from Python.
/// @param region Pointer to the memory region; must outlive the returned object.
/// @return New reference to a RawBuffer Python object, or NULL on failure.
static inline PyObject *
RawBuffer_FromRegion(const rosidl_memory_region_t * region)
{
  if (_rosidl_runtime_py_CAPI == NULL) {
    PyErr_SetString(
      PyExc_RuntimeError,
      "RawBuffer C API not imported — call RawBuffer_ImportCAPI() first");
    return NULL;
  }
  return _rosidl_runtime_py_CAPI->FromRegion(region);
}

#ifdef __cplusplus
}
#endif

#endif  // ROSIDL_RUNTIME_PY__RAW_BUFFER_H_
