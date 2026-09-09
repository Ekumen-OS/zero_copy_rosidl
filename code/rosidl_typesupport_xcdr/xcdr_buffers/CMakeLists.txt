cmake_minimum_required(VERSION 3.14)
project(xcdr_buffers VERSION 0.2.0 LANGUAGES CXX)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(tcb_span REQUIRED)
find_package(tl-expected REQUIRED)

# Library
add_library(${PROJECT_NAME}
  src/common/endianness.cpp
  src/serialization/writer.cpp
  src/serialization/reader.cpp
  src/layout/layout.cpp
  src/layout/layout_builder.cpp
  src/layout/layout_parser.cpp
  src/accessor/const_accessor.cpp
  src/accessor/accessor.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC tl::expected tcb_span::tcb_span
)

# Install library
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY include/
  DESTINATION include
)

# Testing
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(fastcdr REQUIRED)

  # Unit tests
  ament_add_gtest(test_writer test/unit/test_writer.cpp)
  target_link_libraries(test_writer ${PROJECT_NAME})

  ament_add_gtest(test_reader test/unit/test_reader.cpp)
  target_link_libraries(test_reader ${PROJECT_NAME})

  ament_add_gtest(test_layout_builder test/unit/test_layout_builder.cpp)
  target_link_libraries(test_layout_builder ${PROJECT_NAME})

  ament_add_gtest(test_layout_parser test/unit/test_layout_parser.cpp)
  target_link_libraries(test_layout_parser ${PROJECT_NAME})

  ament_add_gtest(test_const_accessor test/unit/test_const_accessor.cpp)
  target_link_libraries(test_const_accessor ${PROJECT_NAME})

  ament_add_gtest(test_accessor test/unit/test_accessor.cpp)
  target_link_libraries(test_accessor ${PROJECT_NAME})

  # Integration tests
  ament_add_gtest(test_fastcdr_interop test/integration/test_fastcdr_interop.cpp)
  target_link_libraries(test_fastcdr_interop ${PROJECT_NAME} fastcdr)

  ament_add_gtest(test_writer_parser test/integration/test_writer_parser.cpp)
  target_link_libraries(test_writer_parser ${PROJECT_NAME})

  ament_add_gtest(test_accessor_integration test/integration/test_accessor_integration.cpp)
  target_link_libraries(test_accessor_integration ${PROJECT_NAME})

  # Microbenchmarks (Google Benchmark). These are skipped by default unless
  # AMENT_RUN_PERFORMANCE_TESTS is ON at configure time.
  find_package(ament_cmake_google_benchmark REQUIRED)

  ament_add_google_benchmark(benchmark_layout_build test/benchmark/benchmark_layout_build.cpp TIMEOUT 120)
  if(TARGET benchmark_layout_build)
    target_link_libraries(benchmark_layout_build ${PROJECT_NAME})
  endif()

  ament_add_google_benchmark(benchmark_layout_parse test/benchmark/benchmark_layout_parse.cpp TIMEOUT 120)
  if(TARGET benchmark_layout_parse)
    target_link_libraries(benchmark_layout_parse ${PROJECT_NAME})
  endif()

  ament_add_google_benchmark(benchmark_accessor test/benchmark/benchmark_accessor.cpp TIMEOUT 120)
  if(TARGET benchmark_accessor)
    target_link_libraries(benchmark_accessor ${PROJECT_NAME})
  endif()

  ament_add_google_benchmark(benchmark_reader test/benchmark/benchmark_reader.cpp TIMEOUT 120)
  if(TARGET benchmark_reader)
    target_link_libraries(benchmark_reader ${PROJECT_NAME})
  endif()

  ament_add_google_benchmark(benchmark_writer test/benchmark/benchmark_writer.cpp TIMEOUT 120)
  if(TARGET benchmark_writer)
    target_link_libraries(benchmark_writer ${PROJECT_NAME})
  endif()

  # Linting
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_targets(${PROJECT_NAME}Targets HAS_LIBRARY_TARGET)
ament_export_dependencies(tl-expected tcb_span)
ament_package()
