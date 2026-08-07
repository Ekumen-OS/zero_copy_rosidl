cmake_minimum_required(VERSION 3.20)
project(rosidl_typesupport_xcdr_cpython_tests)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rosidl_typesupport_xcdr_cpython REQUIRED)
find_package(rosidl_typesupport_xcdr_c REQUIRED)
find_package(rosidl_runtime_cpython REQUIRED)

# Find python before pybind11 (rclpy pattern)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11_vendor REQUIRED)
find_package(pybind11 REQUIRED)

# Generate interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/experimental/Primitives.msg"
  "msg/experimental/Strings.msg"
  "msg/experimental/Containers.msg"
  "msg/experimental/Inner.msg"
  "msg/experimental/Nested.msg"
  "msg/experimental/Empty.msg"
  "srv/AddTwoInts.srv"
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)

  # Exclude cpplint: the workspace's mandated uncrustify style (Allman braces
  # for function definitions, ament_code_style_0_78) conflicts with cpplint's
  # whitespace/braces rule, and cpplint is not enforced by sibling packages
  # (e.g. rosidl_typesupport_xcdr_cpp_tests).  uncrustify remains the
  # formatting authority.
  list(APPEND AMENT_LINT_AUTO_EXCLUDE "ament_cmake_cpplint")

  ament_lint_auto_find_test_dependencies()

    # PYTHONPATH must expose the generated experimental message modules (build
    # space) on top of the workspace environment (which already provides the
    # installed site-packages for rosidl_runtime_cpython, rosidl_parser, etc.).
    # APPEND_ENV preserves the inherited PYTHONPATH instead of replacing it.
    ament_add_gtest(test_serialization_roundtrip
      test/test_serialization_roundtrip.cpp
      APPEND_ENV
        "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_py"
    )
    if(TARGET test_serialization_roundtrip)
      target_link_libraries(test_serialization_roundtrip
        rosidl_typesupport_xcdr_cpython::rosidl_typesupport_xcdr_cpython
        rosidl_typesupport_xcdr_c::rosidl_typesupport_xcdr_c
        xcdr_buffers::xcdr_buffers
        pybind11::pybind11
        pybind11::embed
      )
      rosidl_get_typesupport_target(cpython_typesupport_target ${PROJECT_NAME}
        "rosidl_typesupport_xcdr_cpython")
      target_link_libraries(test_serialization_roundtrip ${cpython_typesupport_target})

      # rosidl_runtime_cpython has no exported CMake target: add its include dir
      # via ament_target_dependencies (provides raw_buffer.h).
      ament_target_dependencies(test_serialization_roundtrip
        "rosidl_runtime_cpython"
      )
    endif()
endif()

ament_package()
