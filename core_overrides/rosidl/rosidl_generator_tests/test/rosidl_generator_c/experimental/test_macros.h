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

// Shared test utilities for the experimental C generator tests.

#ifndef ROSIDL_GENERATOR_C__EXPERIMENTAL__TEST_MACROS_H_
#define ROSIDL_GENERATOR_C__EXPERIMENTAL__TEST_MACROS_H_

#include <stdio.h>

// ---------------------------------------------------------------------------
// Stringification helpers
// ---------------------------------------------------------------------------

#define _STR(x) #x
#define STR(x) _STR(x)

// ---------------------------------------------------------------------------
// Assertion macros
// On failure each macro prints a diagnostic and returns 1 from the caller.
// ---------------------------------------------------------------------------

#define EXPECT_TRUE(expr) \
  do { \
    if (!(expr)) { \
      fprintf(stderr, "FAIL %s:%d  EXPECT_TRUE(" STR(expr) ")\n", \
        __FILE__, __LINE__); \
      return 1; \
    } \
  } while (0)

#define EXPECT_FALSE(expr) \
  do { \
    if ((expr)) { \
      fprintf(stderr, "FAIL %s:%d  EXPECT_FALSE(" STR(expr) ")\n", \
        __FILE__, __LINE__); \
      return 1; \
    } \
  } while (0)

#define EXPECT_EQ(a, b) \
  do { \
    if ((a) != (b)) { \
      fprintf(stderr, "FAIL %s:%d  EXPECT_EQ(" STR(a) ", " STR(b) ")\n", \
        __FILE__, __LINE__); \
      return 1; \
    } \
  } while (0)

#define EXPECT_NE(a, b) \
  do { \
    if ((a) == (b)) { \
      fprintf(stderr, "FAIL %s:%d  EXPECT_NE(" STR(a) ", " STR(b) ")\n", \
        __FILE__, __LINE__); \
      return 1; \
    } \
  } while (0)

// ---------------------------------------------------------------------------
// Package-namespace shorthand
//   As a type:      EXP(Foo) * ptr;
//   As a function:  EXP(Foo__bar)(args)
// ---------------------------------------------------------------------------

#define EXP(T) rosidl_generator_tests__msg__experimental__ ## T

// ---------------------------------------------------------------------------
// Test-runner helper
// ---------------------------------------------------------------------------

#define RUN(fn) \
  do { \
    printf("  " #fn "... "); \
    fflush(stdout); \
    if (fn()) { \
      fprintf(stderr, "FAILED\n"); \
      rc++; \
    } else { \
      printf("OK\n"); \
    } \
  } while (0)

#endif  // ROSIDL_GENERATOR_C__EXPERIMENTAL__TEST_MACROS_H_
