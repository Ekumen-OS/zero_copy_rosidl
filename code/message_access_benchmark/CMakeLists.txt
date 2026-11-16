cmake_minimum_required(VERSION 3.20)
project(message_access_benchmark)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

option(MESSAGE_ACCESS_EXPERIMENTAL "Benchmark experimental owning and external messages" ON)

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rosidl_cmake REQUIRED)
find_package(rosidl_generator_cpp REQUIRED)
find_package(rosidl_runtime_cpp REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(benchmark REQUIRED)
find_package(pybind11_vendor REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

if(MESSAGE_ACCESS_EXPERIMENTAL)
  find_package(rosidl_typesupport_xcdr_cpp REQUIRED)
  find_package(rosidl_runtime_cpython REQUIRED)
endif()

include(cmake/generate_wide_messages.cmake)
rosidl_generate_interfaces(${PROJECT_NAME} ${wide_interfaces})
rosidl_get_typesupport_target(generator_target ${PROJECT_NAME} rosidl_generator_cpp)

add_library(workloads INTERFACE)
target_include_directories(workloads INTERFACE include "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(workloads INTERFACE
  ${generator_target}
  ${visualization_msgs_TARGETS}
  ${builtin_interfaces_TARGETS}
  rosidl_runtime_cpp::rosidl_runtime_cpp
  ament_index_cpp::ament_index_cpp
  pybind11::embed)
if(MESSAGE_ACCESS_EXPERIMENTAL)
  rosidl_get_typesupport_target(xcdr_target ${PROJECT_NAME} rosidl_typesupport_xcdr_cpp)
  target_compile_definitions(workloads INTERFACE MESSAGE_ACCESS_EXPERIMENTAL)
  target_link_libraries(workloads INTERFACE
    ${xcdr_target}
    rosidl_typesupport_xcdr_cpp::rosidl_typesupport_xcdr_cpp)
  target_include_directories(workloads INTERFACE ${rosidl_runtime_cpython_INCLUDE_DIRS})
endif()

add_executable(message_access
  src/main.cpp src/marker_benchmark.cpp src/python_benchmark.cpp ${wide_sources})
target_link_libraries(message_access PRIVATE workloads benchmark::benchmark)
target_compile_definitions(message_access PRIVATE
  BENCHMARK_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
  BENCHMARK_COMPILER="${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(message_access PRIVATE -Wall -Wextra -Wpedantic)
endif()
install(TARGETS message_access DESTINATION lib/${PROJECT_NAME})
install(FILES scripts/marker_workload.py DESTINATION share/${PROJECT_NAME}/scripts)
install(PROGRAMS scripts/summarize_results.py
  DESTINATION lib/${PROJECT_NAME} RENAME summarize)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_cmake_pytest REQUIRED)
  ament_add_gtest(test_workloads test/test_workloads.cpp SKIP_LINKING_MAIN_LIBRARIES)
  target_link_libraries(test_workloads workloads)
  ament_add_pytest_test(test_summarize_results test/test_summarize_results.py TIMEOUT 30)
endif()

ament_export_dependencies(rosidl_runtime_cpp)
ament_package()
