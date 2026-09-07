cmake_minimum_required(VERSION 3.5)
project(constrained_pubsub_benchmark)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rmw_fastrtps_cpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rosidl_runtime_cpp REQUIRED)
find_package(rosidl_typesupport_cpp REQUIRED)
find_package(rosidl_typesupport_interface REQUIRED)

# ── Cross-process benchmark executables (C++ side) ──────────────────────
# Publisher and subscriber halves of the two-process constrained pub/sub
# benchmark, orchestrated by scripts/inter_proc_benchmark.py.  Each half can pair
# with the Python half (scripts/inter_proc_py_publisher.py / inter_proc_py_subscriber.py).

set(INTER_PROC_CPP_LINK_LIBS
  rclcpp::rclcpp
  ${rmw_fastrtps_cpp_LIBRARIES}
  ${sensor_msgs_TARGETS}
  ${builtin_interfaces_TARGETS}
  rosidl_runtime_cpp::rosidl_runtime_cpp
  rosidl_typesupport_cpp::rosidl_typesupport_cpp
  rosidl_typesupport_interface::rosidl_typesupport_interface
)

add_executable(inter_proc_cpp_publisher
  src/inter_proc_cpp_publisher.cpp
  src/config.cpp
)

target_include_directories(inter_proc_cpp_publisher
  PUBLIC
    include
    ${rmw_fastrtps_cpp_INCLUDE_DIRS}
)

target_link_libraries(inter_proc_cpp_publisher
  ${INTER_PROC_CPP_LINK_LIBS}
)

add_executable(inter_proc_cpp_subscriber
  src/inter_proc_cpp_subscriber.cpp
  src/config.cpp
)

target_include_directories(inter_proc_cpp_subscriber
  PUBLIC
    include
    ${rmw_fastrtps_cpp_INCLUDE_DIRS}
)

target_link_libraries(inter_proc_cpp_subscriber
  ${INTER_PROC_CPP_LINK_LIBS}
)

install(TARGETS
  inter_proc_cpp_publisher
  inter_proc_cpp_subscriber
  DESTINATION lib/${PROJECT_NAME}
)

# ── Single-process benchmark executable (C++ side) ───────────────────────
# Publisher + subscriber on one node, all five configurations.  Intra-process
# comms are disabled in-process so the DDS transport is exercised.

add_executable(intra_cpp_benchmark
  src/intra_cpp_benchmark.cpp
  src/config.cpp
)

target_include_directories(intra_cpp_benchmark
  PUBLIC
    include
    ${rmw_fastrtps_cpp_INCLUDE_DIRS}
)

target_link_libraries(intra_cpp_benchmark
  ${INTER_PROC_CPP_LINK_LIBS}
)

install(TARGETS
  intra_cpp_benchmark
  DESTINATION lib/${PROJECT_NAME}
)

# ── Cross-process benchmark scripts (Python side + orchestrator) ────────

install(PROGRAMS
  scripts/inter_proc_benchmark.py
  scripts/intra_proc_benchmark.py
  scripts/inter_proc_py_publisher.py
  scripts/inter_proc_py_subscriber.py
  DESTINATION lib/${PROJECT_NAME}
)

install(FILES
  scripts/benchmark_common.py
  scripts/transport_config.py
  DESTINATION lib/${PROJECT_NAME}
)

# ── Fast DDS transport profiles (UDP-only, SHM-only, SHM + data sharing) ─

install(DIRECTORY
  profiles
  DESTINATION share/${PROJECT_NAME}
)

# ── Tests ─────────────────────────────────────────────────────────────────

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_cmake_pytest REQUIRED)

  ament_add_pytest_test(test_benchmark_common
    test/test_benchmark_common.py
    TIMEOUT 60
  )

  ament_add_gtest(test_sample_record
    test/test_sample_record.cpp
    src/config.cpp
  )
  target_include_directories(test_sample_record
    PUBLIC
      include
      ${rmw_fastrtps_cpp_INCLUDE_DIRS}
  )
  target_link_libraries(test_sample_record
    rclcpp::rclcpp
    ${rmw_fastrtps_cpp_LIBRARIES}
    ${sensor_msgs_TARGETS}
    ${builtin_interfaces_TARGETS}
    rosidl_runtime_cpp::rosidl_runtime_cpp
    rosidl_typesupport_cpp::rosidl_typesupport_cpp
    rosidl_typesupport_interface::rosidl_typesupport_interface
  )

  ament_add_gtest(test_config
    test/test_config.cpp
    src/config.cpp
  )
  target_include_directories(test_config
    PUBLIC
      include
      ${rmw_fastrtps_cpp_INCLUDE_DIRS}
  )
  target_link_libraries(test_config
    rclcpp::rclcpp
    rosidl_runtime_cpp::rosidl_runtime_cpp
  )

  ament_add_gtest(test_smoke
    test/test_smoke.cpp
    src/config.cpp
  )
  target_include_directories(test_smoke
    PUBLIC
      include
      ${rmw_fastrtps_cpp_INCLUDE_DIRS}
  )
  target_link_libraries(test_smoke
    rclcpp::rclcpp
    ${rmw_fastrtps_cpp_LIBRARIES}
    ${sensor_msgs_TARGETS}
    ${builtin_interfaces_TARGETS}
    rosidl_runtime_cpp::rosidl_runtime_cpp
    rosidl_typesupport_cpp::rosidl_typesupport_cpp
    rosidl_typesupport_interface::rosidl_typesupport_interface
  )

  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
