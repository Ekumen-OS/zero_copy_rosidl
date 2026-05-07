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
find_package(rcpputils REQUIRED)

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

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC tl::expected tcb_span::tcb_span rcpputils::rcpputils
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

# Export targets
install(EXPORT ${PROJECT_NAME}Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION lib/cmake/${PROJECT_NAME}
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

  # Linting
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_targets(${PROJECT_NAME}Targets HAS_LIBRARY_TARGET)
ament_export_dependencies(tl_expected tcb_span)
ament_package()
