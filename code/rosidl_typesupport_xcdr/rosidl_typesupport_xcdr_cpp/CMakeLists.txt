cmake_minimum_required(VERSION 3.5)
project(rosidl_typesupport_xcdr_cpp)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rosidl_cmake REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_runtime_cpp REQUIRED)
find_package(rosidl_typesupport_interface REQUIRED)
find_package(xcdr_buffers REQUIRED)

# Build the base library
add_library(${PROJECT_NAME}
  src/identifier.cpp
  src/message_type_support.cpp
  src/service_type_support.cpp
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
    xcdr_buffers::xcdr_buffers
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE "ROSIDL_TYPESUPPORT_XCDR_CPP_BUILDING_DLL"
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

# Install Python package
ament_python_install_package(${PROJECT_NAME})

# Set Python install directory for extras file  
# ament_python_install_package sets this automatically, but we need to ensure it's available
if(NOT DEFINED PYTHON_INSTALL_DIR)
  set(PYTHON_INSTALL_DIR "lib/python3/dist-packages")
endif()

# Install generator script
install(
  PROGRAMS bin/rosidl_typesupport_xcdr_cpp
  DESTINATION lib/rosidl_typesupport_xcdr_cpp
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
ament_export_dependencies(
  rosidl_runtime_c
  rosidl_runtime_cpp
  rosidl_typesupport_interface
  xcdr_buffers
)

ament_export_include_directories(include/${PROJECT_NAME})
ament_export_libraries(${PROJECT_NAME})
ament_export_targets(${PROJECT_NAME})

# Register this typesupport implementation
ament_index_register_resource("rosidl_typesupport_cpp")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package(
  CONFIG_EXTRAS "rosidl_typesupport_xcdr_cpp-extras.cmake.in"
)
