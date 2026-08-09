cmake_minimum_required(VERSION 3.20)
project(rosidl_typesupport_xcdr_cpython)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rosidl_cmake REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_runtime_cpp REQUIRED)
find_package(rosidl_runtime_cpython REQUIRED)
find_package(rosidl_typesupport_interface REQUIRED)
find_package(rosidl_typesupport_xcdr_c REQUIRED)
find_package(xcdr_buffers REQUIRED)

# Find python before pybind11 (rclpy pattern)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11_vendor REQUIRED)
find_package(pybind11 REQUIRED)

# ---------------------------------------------------------------------------
# Base runtime library
# ---------------------------------------------------------------------------
add_library(${PROJECT_NAME}
  src/identifier.cpp
  src/message_type_support.cpp
  src/python_helpers.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    rosidl_runtime_c::rosidl_runtime_c
    rosidl_runtime_cpp::rosidl_runtime_cpp
    rosidl_typesupport_interface::rosidl_typesupport_interface
    rosidl_typesupport_xcdr_c::rosidl_typesupport_xcdr_c
    xcdr_buffers::xcdr_buffers)
# pybind11 is an implementation detail of this library: python_helpers.cpp uses
# it, but consumers link the compiled ABI and do not need pybind11 headers or
# symbols.  Gate both the pybind11 and Python3 links behind BUILD_INTERFACE so
# the exported target carries no pybind11/Python3 reference that consumers
# would need to find_package.
target_link_libraries(${PROJECT_NAME} PRIVATE
  $<BUILD_INTERFACE:pybind11::pybind11>
  $<BUILD_INTERFACE:Python3::Python>
)

# rosidl_runtime_cpython has no exported CMake target (pure Python + module),
# so pull in its exported include directory for raw_buffer.h via ament.
ament_target_dependencies(${PROJECT_NAME} PUBLIC
  "rosidl_runtime_cpython"
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE "ROSIDL_TYPESUPPORT_XCDR_CPYTHON_BUILDING_DLL"
)

# Install library
install(
  TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# Install headers
install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

# Install Python package (the code generator)
ament_python_install_package(${PROJECT_NAME})

# Set Python install directory for extras file
if(NOT DEFINED PYTHON_INSTALL_DIR)
  set(PYTHON_INSTALL_DIR "lib/python3/dist-packages")
endif()

# Install generator script
install(
  PROGRAMS bin/rosidl_typesupport_xcdr_cpython
  DESTINATION lib/rosidl_typesupport_xcdr_cpython
)

# Install resource files (templates)
install(
  DIRECTORY resource
  DESTINATION share/${PROJECT_NAME}
)

# Install CMake files
install(
  DIRECTORY cmake
  DESTINATION share/${PROJECT_NAME}
)

# Register as rosidl typesupport
# pybind11/Python3 are NOT exported: they are implementation details (linked
# PRIVATE), so no consumer of this package's exported targets needs them.
ament_export_dependencies(
  rosidl_runtime_c
  rosidl_runtime_cpp
  rosidl_typesupport_interface
  rosidl_typesupport_xcdr_c
  xcdr_buffers
)

ament_export_include_directories(include/${PROJECT_NAME})
ament_export_libraries(${PROJECT_NAME})
ament_export_targets(${PROJECT_NAME})

# Register this typesupport implementation with the CPython dispatch layer.
# NOTE: NOT registered as "rosidl_typesupport_cpp" — the CPython typesupport is
# a language-specific implementation of the CPython dispatch mechanism and must
# not appear in the C++ dispatch list.  Middleware resolves it from the
# CPython dispatch handle's type_support_map via the language-agnostic XCDR
# lookup helper.
ament_index_register_resource("rosidl_typesupport_cpython")

# Register as experimental-capable (supports experimental message namespaces)
ament_index_register_resource("available_experimental_typesupports")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)

  # Exclude cpplint: the workspace's mandated uncrustify style (Allman braces
  # for function definitions, ament_code_style_0_78) conflicts with cpplint's
  # whitespace/braces rule, and cpplint is not enforced by the sibling
  # rosidl_typesupport_xcdr_cpp package.  uncrustify remains the formatting
  # authority.
  list(APPEND AMENT_LINT_AUTO_EXCLUDE "ament_cmake_cpplint")

  ament_lint_auto_find_test_dependencies()
endif()

ament_package(
  CONFIG_EXTRAS "rosidl_typesupport_xcdr_cpython-extras.cmake.in"
)
