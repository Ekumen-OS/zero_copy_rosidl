cmake_minimum_required(VERSION 3.5)
project(rosidl_typesupport_xcdr_c C CXX)

# Default to C11 and C++17
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake_ros REQUIRED)
find_package(rcutils REQUIRED)
find_package(rosidl_runtime_c REQUIRED)

ament_export_dependencies(rcutils)
ament_export_dependencies(rosidl_runtime_c)

add_library(${PROJECT_NAME}
  src/identifier.c
  src/message_type_support.c
)
target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include/${PROJECT_NAME}>")

target_link_libraries(${PROJECT_NAME} PUBLIC
  rcutils::rcutils
  rosidl_runtime_c::rosidl_runtime_c
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE "ROSIDL_TYPESUPPORT_XCDR_C_BUILDING_DLL")

# Export old-style CMake variables
ament_export_include_directories("include/${PROJECT_NAME}")
ament_export_libraries(${PROJECT_NAME})

# Export modern CMake targets
ament_export_targets(${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_xcdr_c_callback_basics
    test/test_xcdr_c_callback_basics.cpp)
  target_link_libraries(test_xcdr_c_callback_basics
    ${PROJECT_NAME})
endif()

install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME})

install(
  TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

ament_package()
