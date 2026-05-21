cmake_minimum_required(VERSION 3.20)

project(rosidl_runtime_cpython)

# Default to C11
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rosidl_runtime_c REQUIRED)

# Prefer the unversioned system Python (matches rclpy's approach).
cmake_policy(SET CMP0094 NEW)
set(Python3_FIND_UNVERSIONED_NAMES FIRST)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# ---------------------------------------------------------------------------
# Pure Python package
# ---------------------------------------------------------------------------
ament_python_install_package(${PROJECT_NAME})

# ---------------------------------------------------------------------------
# C extension: rosidl_runtime_cpython._raw_buffer  (provides RawBuffer)
# ---------------------------------------------------------------------------
Python3_add_library(_raw_buffer MODULE src/raw_buffer.c)
target_include_directories(_raw_buffer PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  ${rosidl_runtime_c_INCLUDE_DIRS}
)
target_link_libraries(_raw_buffer PRIVATE
  rosidl_runtime_c::rosidl_runtime_c
)
set_target_properties(_raw_buffer PROPERTIES
  # Output into the rosidl_runtime_cpython/ subdirectory of the build space so that
  # PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} resolves the submodule import
  # `from rosidl_runtime_cpython._raw_buffer import RawBuffer` correctly.
  LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/rosidl_runtime_cpython>
)

install(TARGETS _raw_buffer
  DESTINATION "${PYTHON_INSTALL_DIR}/rosidl_runtime_cpython"
)

# ---------------------------------------------------------------------------
# Testing
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_pytest REQUIRED)
  set(_pytest_tests
    test/rosidl_runtime_cpython/test_dtype.py
    test/rosidl_runtime_cpython/test_scalar.py
    test/rosidl_runtime_cpython/test_array.py
    test/rosidl_runtime_cpython/test_sequence.py
    test/rosidl_runtime_cpython/test_string.py
  )
  foreach(_test_path ${_pytest_tests})
    get_filename_component(_test_name ${_test_path} NAME_WE)
    ament_add_pytest_test(${_test_name} ${_test_path}
      APPEND_ENV PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}
      TIMEOUT 60
    )
  endforeach()
endif()

ament_package()
