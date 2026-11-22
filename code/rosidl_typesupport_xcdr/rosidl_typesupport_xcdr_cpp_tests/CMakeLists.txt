cmake_minimum_required(VERSION 3.5)
project(rosidl_typesupport_xcdr_cpp_tests)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rosidl_typesupport_xcdr_cpp REQUIRED)
find_package(rosidl_typesupport_xcdr_c REQUIRED)

# Generate interfaces
set(view_service_interfaces)
if(_runtime_constraints_header)
  list(APPEND view_service_interfaces "srv/ViewFields.srv")
endif()
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/experimental/BasicTypes.msg"
  "msg/experimental/BoundedMessage.msg"
  "msg/experimental/BoundedVariable.msg"
  "msg/experimental/UnboundedMessage.msg"
  "msg/experimental/InnerValue.msg"
  "msg/experimental/NestedContainers.msg"
  "msg/experimental/NestedBounded.msg"
  "msg/experimental/VariableContainers.msg"
  "msg/InnerMessage.msg"
  "msg/Empty.msg"
  "msg/NestedLast.msg"
  "msg/NestedMessage.msg"
  "msg/Keyed.idl"
  "srv/AddTwoInts.srv"
  ${view_service_interfaces}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)

  ament_lint_auto_find_test_dependencies()

  # Helper macro to reduce repetition
  macro(add_xcdr_test target_name source_file)
    ament_add_gtest(${target_name} ${source_file})
    if(TARGET ${target_name})
      target_link_libraries(${target_name}
        rosidl_typesupport_xcdr_cpp::rosidl_typesupport_xcdr_cpp
        rosidl_typesupport_xcdr_c::rosidl_typesupport_xcdr_c
        xcdr_buffers::xcdr_buffers
      )
      rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME}
        "rosidl_typesupport_xcdr_cpp")
      target_link_libraries(${target_name} ${cpp_typesupport_target})
    endif()
  endmacro()

  if(_runtime_constraints_header)
    message(STATUS "Using Rolling region-view XCDR integration tests")
    add_xcdr_test(test_region_xcdr test/test_region_xcdr.cpp)
    target_sources(test_region_xcdr PRIVATE test/test_region_c.c)
    target_compile_features(test_region_xcdr PRIVATE cxx_std_20)
    rosidl_get_typesupport_target(dispatch_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
    target_link_libraries(test_region_xcdr ${dispatch_target})
  else()
  # --- Typesupport handle lookup ---
  add_xcdr_test(test_typesupport_handle
    test/test_typesupport_handle.cpp
  )

  # --- Serialization roundtrip ---
  add_xcdr_test(test_serialization_roundtrip
    test/test_serialization_roundtrip.cpp
  )

  # --- Serialization overflow / buffer-too-small ---
  add_xcdr_test(test_serialization_overflow
    test/test_serialization_overflow.cpp
  )

  # --- Zero-copy lifecycle (experimental only) ---
  add_xcdr_test(test_zero_copy_lifecycle
    test/test_zero_copy_lifecycle.cpp
  )

  # --- Service typesupport ---
  add_xcdr_test(test_service_typesupport
    test/test_service_typesupport.cpp
  )

  # --- Constrained typesupport lifecycle ---
  add_xcdr_test(test_constrained_typesupport
    test/test_constrained_typesupport.cpp
  )

  # --- Outer callback integration ---
  add_xcdr_test(test_outer_callback_integration
    test/test_outer_callback_integration.cpp
  )

  # --- Typesupport API error contracts ---
  add_xcdr_test(test_typesupport_api_errors
    test/test_typesupport_api_errors.cpp
  )

  # --- Compact traversal (fast path, rewrite path, fail path) ---
  add_xcdr_test(test_compact_traversal
    test/test_compact_traversal.cpp
  )

  # --- ExternalStorage recursion (nested types, containers) ---
  add_xcdr_test(test_external_storage_recursion
    test/test_external_storage_recursion.cpp
  )
  endif()

endif()

ament_package()
