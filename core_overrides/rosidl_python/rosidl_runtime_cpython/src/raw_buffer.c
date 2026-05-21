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

/// @file raw_buffer.c
/// @brief CPython extension providing the RawBuffer type.
///
/// RawBuffer is a byte-oriented, PEP 3118 compatible buffer object that holds
/// a rosidl_memory_region_t and operates in two modes:
///
///  Managed   — owns its storage via PyMem_Malloc / PyMem_Free.
///              region.location.address and region.size track the live
///              allocation; size tracks the logical byte count.
///              Created from Python: RawBuffer(capacity=N)
///              reserve(n) replaces the internal block with a freshly
///              allocated one; the old block is freed only after all active
///              exports on it have been released, preventing dangling-pointer
///              access from numpy views captured before the resize.
///
///  External  — holds a view over a caller-owned rosidl_memory_region_t.
///              Created from C only: RawBuffer_FromRegion(&region)
///              Does NOT own the memory; reserve() and resize() raise
///              BufferError.  Writable from Python (the caller controls
///              the lifecycle of the underlying memory).

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdint.h>
#include <string.h>

#include "rosidl_runtime_c/experimental/memory.h"
#include "rosidl_runtime_cpython/raw_buffer.h"

// Forward declaration so _RawBuffer_FromRegion can reference the type.
static PyTypeObject RawBuffer_Type;

// ---------------------------------------------------------------------------
// Internal guard type
// ---------------------------------------------------------------------------
// When a managed RawBuffer is reallocated, the old block must not be freed
// while an active Py_buffer export (e.g. a numpy array view) still points
// into it.  We wrap the old allocation address in a lightweight BufferGuard
// whose sole job is to call PyMem_Free when the last reference to it drops.

typedef struct {
  PyObject_HEAD
  void * data;
} RawBufferGuard;

static void
RawBufferGuard_dealloc(RawBufferGuard * self)
{
  PyMem_Free(self->data);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyTypeObject RawBufferGuard_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "rosidl_runtime_py._raw_buffer.RawBufferGuard",
  .tp_basicsize = sizeof(RawBufferGuard),
  .tp_dealloc = (destructor)RawBufferGuard_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
};

static PyObject *
RawBufferGuard_New(void * data)
{
  RawBufferGuard * g = PyObject_New(RawBufferGuard, &RawBufferGuard_Type);
  if (!g) {
    return NULL;
  }
  g->data = data;
  return (PyObject *)g;
}

// ---------------------------------------------------------------------------
// RawBuffer type
// ---------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD

  // The underlying memory region.
  //   region.location.address — pointer to the first byte.
  //   region.size             — allocated capacity in bytes.
  // For managed buffers, this describes the PyMem allocation.
  // For external buffers, this is a direct copy of the caller's region.
  rosidl_memory_region_t region;

  // Logical byte count: how many bytes are currently in use / meaningful.
  // Always <= region.size.
  Py_ssize_t size;

  // Ownership flag.  1 = managed (owns the allocation), 0 = external view.
  int is_owner;

  // Number of active Py_buffer exports on the *current* allocation.
  // Protected by the GIL.
  Py_ssize_t export_count;

  // Guard holding the *previous* block while it may still be referenced by
  // an active export.  NULL when no reallocation has occurred.
  PyObject * pending_guard;
} RawBuffer;

// Convenience accessors so call sites read cleanly.
#define RB_DATA(self)     ((self)->region.location.address)
#define RB_CAPACITY(self) ((Py_ssize_t)(self)->region.size)

// ---------------------------------------------------------------------------
// Buffer protocol
// ---------------------------------------------------------------------------

static int
RawBuffer_bf_getbuffer(PyObject * exporter, Py_buffer * view, int flags)
{
  RawBuffer * self = (RawBuffer *)exporter;

  if (RB_DATA(self) == NULL && self->size != 0) {
    PyErr_SetString(PyExc_BufferError, "RawBuffer has no data");
    view->obj = NULL;
    return -1;
  }

  // Both managed and external buffers are writable from Python's perspective;
  // the caller controls the lifecycle of external memory.
  if (PyBuffer_FillInfo(view, exporter, RB_DATA(self), self->size, 0, flags) < 0) {
    return -1;
  }

  self->export_count++;
  return 0;
}

static void
RawBuffer_bf_releasebuffer(PyObject * exporter, Py_buffer * view)
{
  (void)view;
  RawBuffer * self = (RawBuffer *)exporter;
  if (self->export_count > 0) {
    self->export_count--;
  }
}

static PyBufferProcs RawBuffer_as_buffer = {
  .bf_getbuffer = RawBuffer_bf_getbuffer,
  .bf_releasebuffer = RawBuffer_bf_releasebuffer,
};

// ---------------------------------------------------------------------------
// Allocation / deallocation
// ---------------------------------------------------------------------------

static void
RawBuffer_dealloc(RawBuffer * self)
{
  if (self->is_owner && RB_DATA(self) != NULL) {
    PyMem_Free(RB_DATA(self));
    self->region.location.address = NULL;
    self->region.size = 0;
  }
  Py_XDECREF(self->pending_guard);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

// ---------------------------------------------------------------------------
// Python constructor:  RawBuffer(capacity=0)
// ---------------------------------------------------------------------------

static PyObject *
RawBuffer_new(PyTypeObject * type, PyObject * args, PyObject * kwargs)
{
  (void)args;
  (void)kwargs;
  RawBuffer * self = (RawBuffer *)type->tp_alloc(type, 0);
  if (!self) {
    return NULL;
  }
  memset(&self->region, 0, sizeof(rosidl_memory_region_t));
  self->size = 0;
  self->is_owner = 1;
  self->export_count = 0;
  self->pending_guard = NULL;
  return (PyObject *)self;
}

static int
RawBuffer_init(RawBuffer * self, PyObject * args, PyObject * kwargs)
{
  // Accept either:
  //   RawBuffer()             — empty, zero capacity
  //   RawBuffer(capacity)     — allocate `capacity` zero-filled bytes
  //   RawBuffer(data)         — allocate and copy bytes-like object; size = len(data)
  static char * kwlist[] = {"data", NULL};
  PyObject * data_obj = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &data_obj)) {
    return -1;
  }

  if (data_obj == NULL || data_obj == Py_None) {
    // RawBuffer()  →  empty managed buffer, no allocation.
    return 0;
  }

  // Integer argument: treat as capacity (backwards-compatible).
  if (PyLong_Check(data_obj)) {
    Py_ssize_t capacity = PyLong_AsSsize_t(data_obj);
    if (capacity == -1 && PyErr_Occurred()) {
      return -1;
    }
    if (capacity < 0) {
      PyErr_SetString(PyExc_ValueError, "capacity must be >= 0");
      return -1;
    }
    if (capacity == 0) {
      return 0;
    }
    void * block = PyMem_Malloc((size_t)capacity);
    if (!block) {
      PyErr_NoMemory();
      return -1;
    }
    memset(block, 0, (size_t)capacity);
    self->region.location.address = block;
    self->region.location.attributes = 0;
    self->region.size = (size_t)capacity;
    self->size = capacity;
    return 0;
  }

  // Buffer-protocol argument: allocate and copy.
  Py_buffer view;
  if (PyObject_GetBuffer(data_obj, &view, PyBUF_SIMPLE) < 0) {
    PyErr_SetString(PyExc_TypeError,
      "RawBuffer() argument must be an int, a bytes-like object, or None");
    return -1;
  }
  Py_ssize_t nbytes = view.len;
  if (nbytes == 0) {
    PyBuffer_Release(&view);
    return 0;
  }
  void * block = PyMem_Malloc((size_t)nbytes);
  if (!block) {
    PyBuffer_Release(&view);
    PyErr_NoMemory();
    return -1;
  }
  memcpy(block, view.buf, (size_t)nbytes);
  PyBuffer_Release(&view);
  self->region.location.address = block;
  self->region.location.attributes = 0;
  self->region.size = (size_t)nbytes;
  self->size = nbytes;
  return 0;
}

// ---------------------------------------------------------------------------
// C-only factory:  _RawBuffer_FromRegion
// Exposed via PyCapsule; NOT a Python-callable method.
// ---------------------------------------------------------------------------

static PyObject *
_RawBuffer_FromRegion(const rosidl_memory_region_t * region)
{
  if (!region) {
    PyErr_SetString(PyExc_ValueError, "region pointer is NULL");
    return NULL;
  }
  RawBuffer * buf = (RawBuffer *)RawBuffer_new(&RawBuffer_Type, NULL, NULL);
  if (!buf) {
    return NULL;
  }
  // Copy the entire region struct so the caller's layout (address, attributes,
  // size) is preserved exactly.
  buf->region = *region;
  buf->size = (Py_ssize_t)region->size;
  buf->is_owner = 0;
  return (PyObject *)buf;
}

// ---------------------------------------------------------------------------
// C API struct instance.
// Stored by pointer in the PyCapsule so PyCapsule_New receives a data pointer
// (void * from struct *), not a function pointer — the cast is valid in C.
// ---------------------------------------------------------------------------
static const RawBuffer_CAPI_t _RawBuffer_CAPI = {
  _RawBuffer_FromRegion,
};

// ---------------------------------------------------------------------------
// Methods
// ---------------------------------------------------------------------------

// reserve(n): ensure at least n bytes of capacity.
// Always allocates a fresh block so existing exports remain valid.
// Raises BufferError for external (non-owning) buffers.
static PyObject *
RawBuffer_reserve(RawBuffer * self, PyObject * args)
{
  Py_ssize_t new_capacity;
  if (!PyArg_ParseTuple(args, "n", &new_capacity)) {
    return NULL;
  }

  if (new_capacity < 0) {
    PyErr_SetString(PyExc_ValueError, "capacity must be >= 0");
    return NULL;
  }

  if (new_capacity <= RB_CAPACITY(self)) {
    Py_RETURN_NONE;
  }

  if (!self->is_owner) {
    PyErr_SetString(
      PyExc_BufferError,
      "cannot reserve on an external (non-owning) RawBuffer");
    return NULL;
  }

  void * new_block = PyMem_Malloc((size_t)new_capacity);
  if (!new_block) {
    return PyErr_NoMemory();
  }
  memset(new_block, 0, (size_t)new_capacity);

  if (RB_DATA(self) && self->size > 0) {
    memcpy(new_block, RB_DATA(self), (size_t)self->size);
  }

  // If there are active exports on the current block, hand it off to a guard
  // so its lifetime is tied to the last export that references it.
  if (self->export_count > 0 && RB_DATA(self) != NULL) {
    Py_XDECREF(self->pending_guard);
    self->pending_guard = RawBufferGuard_New(RB_DATA(self));
    if (!self->pending_guard) {
      PyMem_Free(new_block);
      return NULL;
    }
    // Guard now owns the old allocation; do NOT free it here.
    self->export_count = 0;
  } else {
    PyMem_Free(RB_DATA(self));
    Py_XDECREF(self->pending_guard);
    self->pending_guard = NULL;
  }

  self->region.location.address = new_block;
  self->region.location.attributes = 0;
  self->region.size = (size_t)new_capacity;
  // Note: self->size (logical) is intentionally unchanged; callers update it.
  Py_RETURN_NONE;
}

// resize(n): set the logical size to n bytes, growing capacity as needed.
// Raises BufferError for external (non-owning) buffers.
static PyObject *
RawBuffer_resize(RawBuffer * self, PyObject * args)
{
  Py_ssize_t new_size;
  if (!PyArg_ParseTuple(args, "n", &new_size)) {
    return NULL;
  }

  if (!self->is_owner) {
    PyErr_SetString(
      PyExc_BufferError,
      "cannot resize an external (non-owning) RawBuffer");
    return NULL;
  }

  if (new_size < 0) {
    PyErr_SetString(PyExc_ValueError, "size must be >= 0");
    return NULL;
  }

  if (new_size > RB_CAPACITY(self)) {
    // Geometric growth: double until sufficient.
    Py_ssize_t new_capacity = RB_CAPACITY(self) == 0 ? new_size : RB_CAPACITY(self);
    while (new_capacity < new_size) {
      new_capacity *= 2;
      if (new_capacity <= 0) {  // overflow guard
        new_capacity = new_size;
        break;
      }
    }
    PyObject * reserve_args = Py_BuildValue("(n)", new_capacity);
    if (!reserve_args) {
      return NULL;
    }
    PyObject * result = RawBuffer_reserve(self, reserve_args);
    Py_DECREF(reserve_args);
    if (!result) {
      return NULL;
    }
    Py_DECREF(result);
  }

  // Zero-fill any newly exposed bytes.
  if (new_size > self->size && RB_DATA(self)) {
    memset((char *)RB_DATA(self) + self->size, 0, (size_t)(new_size - self->size));
  }

  self->size = new_size;
  Py_RETURN_NONE;
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

static PyObject *
RawBuffer_get_address(RawBuffer * self, void * closure)
{
  (void)closure;
  return PyLong_FromVoidPtr(RB_DATA(self));
}

static PyObject *
RawBuffer_get_size(RawBuffer * self, void * closure)
{
  (void)closure;
  return PyLong_FromSsize_t(self->size);
}

static PyObject *
RawBuffer_get_capacity(RawBuffer * self, void * closure)
{
  (void)closure;
  return PyLong_FromSsize_t(RB_CAPACITY(self));
}

static PyObject *
RawBuffer_get_is_owner(RawBuffer * self, void * closure)
{
  (void)closure;
  return PyBool_FromLong(self->is_owner);
}

// ---------------------------------------------------------------------------
// Indexing: buf[i], buf[i] = v
// ---------------------------------------------------------------------------
//
// buf[i]         — return the byte at position i as a Python int.
// buf[i] = v     — write a byte value (int 0-255) at position i.
// len(buf)       — return the logical size.
//
// These work on the *logical* size (self->size), not capacity.

static Py_ssize_t
RawBuffer_sq_length(RawBuffer * self)
{
  return self->size;
}

static PyObject *
RawBuffer_mp_subscript(RawBuffer * self, PyObject * key)
{
  if (!PyLong_Check(key)) {
    PyErr_SetString(PyExc_TypeError, "RawBuffer indices must be integers");
    return NULL;
  }
  Py_ssize_t idx = PyLong_AsSsize_t(key);
  if (idx == -1 && PyErr_Occurred()) {
    return NULL;
  }
  if (idx < 0) {
    idx += self->size;
  }
  if (idx < 0 || idx >= self->size) {
    PyErr_SetString(PyExc_IndexError, "RawBuffer index out of range");
    return NULL;
  }
  return PyLong_FromLong(((unsigned char *)RB_DATA(self))[idx]);
}

static int
RawBuffer_mp_ass_subscript(RawBuffer * self, PyObject * key, PyObject * value)
{
  if (!PyLong_Check(key)) {
    PyErr_SetString(PyExc_TypeError, "RawBuffer indices must be integers");
    return -1;
  }
  Py_ssize_t idx = PyLong_AsSsize_t(key);
  if (idx == -1 && PyErr_Occurred()) {
    return -1;
  }
  if (idx < 0) {
    idx += self->size;
  }
  if (idx < 0 || idx >= self->size) {
    PyErr_SetString(PyExc_IndexError, "RawBuffer index out of range");
    return -1;
  }
  if (!PyLong_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "RawBuffer values must be integers (0-255)");
    return -1;
  }
  long v = PyLong_AsLong(value);
  if (v == -1 && PyErr_Occurred()) {
    return -1;
  }
  if (v < 0 || v > 255) {
    PyErr_SetString(PyExc_ValueError, "byte value must be in range 0-255");
    return -1;
  }
  ((unsigned char *)RB_DATA(self))[idx] = (unsigned char)v;
  return 0;
}

static PySequenceMethods RawBuffer_as_sequence = {
  .sq_length = (lenfunc)RawBuffer_sq_length,
};

static PyMappingMethods RawBuffer_as_mapping = {
  .mp_length = (lenfunc)RawBuffer_sq_length,
  .mp_subscript = (binaryfunc)RawBuffer_mp_subscript,
  .mp_ass_subscript = (objobjargproc)RawBuffer_mp_ass_subscript,
};

// ---------------------------------------------------------------------------
// __repr__
// ---------------------------------------------------------------------------

static PyObject *
RawBuffer_repr(RawBuffer * self)
{
  return PyUnicode_FromFormat(
    "RawBuffer(address=%p, size=%zd, capacity=%zd, is_owner=%s)",
    RB_DATA(self),
    self->size,
    RB_CAPACITY(self),
    self->is_owner ? "True" : "False");
}

// ---------------------------------------------------------------------------
// PEP 688 __buffer__ / __release_buffer__ (Python >= 3.12)
// ---------------------------------------------------------------------------

#if PY_VERSION_HEX >= 0x030c0000
static PyObject *
RawBuffer_py_buffer(RawBuffer * self, PyObject * flags_obj)
{
  int flags = (int)PyLong_AsLong(flags_obj);
  if (flags == -1 && PyErr_Occurred()) {
    return NULL;
  }
  Py_buffer view;
  if (RawBuffer_bf_getbuffer((PyObject *)self, &view, flags) < 0) {
    return NULL;
  }
  // bf_getbuffer incremented export_count; the memoryview's own release will
  // call bf_releasebuffer which decrements it.
  return PyMemoryView_FromBuffer(&view);
}

static PyObject *
RawBuffer_py_release_buffer(RawBuffer * self, PyObject * mv)
{
  (void)self;
  (void)mv;
  Py_RETURN_NONE;
}
#endif

// ---------------------------------------------------------------------------
// Type tables
// ---------------------------------------------------------------------------

static PyMethodDef RawBuffer_methods[] = {
  {"reserve", (PyCFunction)RawBuffer_reserve, METH_VARARGS,
    "reserve(n)\n--\n\n"
    "Ensure at least *n* bytes of capacity.\n\n"
    "Always allocates a new block so numpy views captured before this call\n"
    "remain valid (pointing to the old block) until they are GC-collected.\n"
    "Raises BufferError on external (non-owning) buffers."},
  {"resize", (PyCFunction)RawBuffer_resize, METH_VARARGS,
    "resize(n)\n--\n\n"
    "Set the logical size to *n* bytes, growing capacity as needed.\n"
    "Raises BufferError on external (non-owning) buffers."},
#if PY_VERSION_HEX >= 0x030c0000
  {"__buffer__", (PyCFunction)RawBuffer_py_buffer, METH_O, NULL},
  {"__release_buffer__", (PyCFunction)RawBuffer_py_release_buffer, METH_O, NULL},
#endif
  {NULL, NULL, 0, NULL}
};

static PyGetSetDef RawBuffer_getset[] = {
  {"address", (getter)RawBuffer_get_address, NULL,
    "int: address of the first byte (region.location.address).", NULL},
  {"size", (getter)RawBuffer_get_size, NULL,
    "int: number of bytes currently in use (logical size).", NULL},
  {"capacity", (getter)RawBuffer_get_capacity, NULL,
    "int: allocated byte capacity (region.size).", NULL},
  {"is_owner", (getter)RawBuffer_get_is_owner, NULL,
    "bool: True if this buffer owns and manages the underlying memory,\n"
    "False if it holds a non-owning view over an external "
    "rosidl_memory_region_t.", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};

static PyTypeObject RawBuffer_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "rosidl_runtime_py._raw_buffer.RawBuffer",
  .tp_doc =
    "RawBuffer(data=None)\n--\n\n"
    "Byte buffer that holds a rosidl_memory_region_t and implements the\n"
    "PEP 3118 buffer protocol plus integer indexing.\n\n"
    "Construction:\n\n"
    "  RawBuffer()           — empty managed buffer, no allocation.\n"
    "  RawBuffer(n)          — allocate n zero-filled bytes.\n"
    "  RawBuffer(data)       — allocate and copy a bytes-like object;\n"
    "                          size is set to len(data).\n\n"
    "Indexing:\n\n"
    "  buf[i]                — return byte at index i as int.\n"
    "  buf[i] = v            — write int v (0-255) at index i.\n"
    "  len(buf)              — logical byte count.\n\n"
    "Two modes:\n\n"
    "  Managed  — owns PyMem-allocated storage; constructed from Python.\n"
    "  External — holds a non-owning view over a caller-supplied\n"
    "             rosidl_memory_region_t; constructed from C/C++ via\n"
    "             RawBuffer_FromRegion() (see raw_buffer.h).\n\n"
    "Both modes are writable from Python.\n\n"
    "reserve() always allocates a *new* block, so numpy arrays produced via\n"
    "numpy.frombuffer() before a reserve remain valid (but stale) until GC.",
  .tp_basicsize = sizeof(RawBuffer),
  .tp_dealloc = (destructor)RawBuffer_dealloc,
  .tp_repr = (reprfunc)RawBuffer_repr,
  .tp_as_sequence = &RawBuffer_as_sequence,
  .tp_as_mapping = &RawBuffer_as_mapping,
  .tp_as_buffer = &RawBuffer_as_buffer,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_methods = RawBuffer_methods,
  .tp_getset = RawBuffer_getset,
  .tp_new = RawBuffer_new,
  .tp_init = (initproc)RawBuffer_init,
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

static PyModuleDef _raw_buffer_module = {
  PyModuleDef_HEAD_INIT,
  .m_name = "rosidl_runtime_py._raw_buffer",
  .m_doc =
    "Internal C extension for rosidl_runtime_py.\n\n"
    "Exports:\n"
    "  RawBuffer — managed or non-owning-view byte buffer over a\n"
    "              rosidl_memory_region_t.\n"
    "  _C_API    — PyCapsule for downstream C/C++ extensions.",
  .m_size = -1,
};

PyMODINIT_FUNC
PyInit__raw_buffer(void)
{
  if (PyType_Ready(&RawBufferGuard_Type) < 0) {
    return NULL;
  }
  if (PyType_Ready(&RawBuffer_Type) < 0) {
    return NULL;
  }

  PyObject * m = PyModule_Create(&_raw_buffer_module);
  if (!m) {
    return NULL;
  }

  Py_INCREF(&RawBuffer_Type);
  if (PyModule_AddObject(m, "RawBuffer", (PyObject *)&RawBuffer_Type) < 0) {
    Py_DECREF(&RawBuffer_Type);
    Py_DECREF(m);
    return NULL;
  }

  // Export C API via PyCapsule so downstream extensions can wrap
  // rosidl_memory_region_t objects without linking against this module.
  // &_RawBuffer_CAPI is a struct pointer (data pointer), so the implicit
  // conversion to void * in PyCapsule_New is valid in both C and C++.
  PyObject * capi_capsule = PyCapsule_New(
    (void *)&_RawBuffer_CAPI,
    ROSIDL_RUNTIME_PY_RAW_BUFFER_CAPSULE_NAME,
    NULL);
  if (!capi_capsule) {
    Py_DECREF(m);
    return NULL;
  }

  if (PyModule_AddObject(m, "_C_API", capi_capsule) < 0) {
    Py_DECREF(capi_capsule);
    Py_DECREF(m);
    return NULL;
  }

  return m;
}
