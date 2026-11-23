cmake_minimum_required(VERSION 3.20)
project(rosidl_typesupport_xcdr_rclpy_tests)

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_cmake_ros_core REQUIRED)
  find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
  find_package(pybind11 REQUIRED)
  find_package(rcl REQUIRED)
  find_package(rclcpp REQUIRED)
  find_package(rclpy REQUIRED)
  find_package(rosidl_runtime_cpython REQUIRED)
  find_package(rosidl_typesupport_xcdr_cpp_tests REQUIRED)
  ament_lint_auto_find_test_dependencies()

  pybind11_add_module(_region_fixture test/region_fixture.cpp)
  target_include_directories(_region_fixture PRIVATE ${rosidl_runtime_cpython_INCLUDE_DIRS})
  target_link_libraries(_region_fixture PRIVATE
    ament_cmake_ros_core::ament_ros_cxx_standard
    rosidl_typesupport_xcdr_cpp_tests::rosidl_typesupport_xcdr_cpp_tests__rosidl_typesupport_cpp)

  add_executable(region_cpp_peer test/region_cpp_peer.cpp)
  target_compile_features(region_cpp_peer PRIVATE cxx_std_20)
  target_link_libraries(region_cpp_peer PRIVATE
    rclcpp::rclcpp
    rosidl_typesupport_xcdr_cpp_tests::rosidl_typesupport_xcdr_cpp_tests__rosidl_typesupport_cpp)

  ament_add_pytest_test(test_region_publisher test/test_region_publisher.py
    APPEND_ENV PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}
    ENV RMW_IMPLEMENTATION=rmw_fastrtps_cpp REGION_CPP_PEER=$<TARGET_FILE:region_cpp_peer>
    TIMEOUT 60)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(no_endpoint_loans SHARED test/no_endpoint_loans.c)
    target_link_libraries(no_endpoint_loans rcl::rcl)
    ament_add_pytest_test(test_nonloaning_endpoint test/test_region_publisher.py
      APPEND_ENV PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}
      ENV RMW_IMPLEMENTATION=rmw_fastrtps_cpp ROS_DISABLE_LOANED_MESSAGES=0
      REGION_TEST_NO_ENDPOINT_LOANS=1 PYTEST_ADDOPTS=-knonloaning_endpoint
      LD_PRELOAD=$<TARGET_FILE:no_endpoint_loans>
      TIMEOUT 60)
  endif()
  if(ROSIDL_GENERATOR_PY_REGION_VIEWS)
    find_package(rosidl_default_generators REQUIRED)
    rosidl_generate_interfaces(${PROJECT_NAME}
      "msg/experimental/CrossPackage.msg"
      DEPENDENCIES rosidl_typesupport_xcdr_cpp_tests
      SKIP_INSTALL)
    ament_add_pytest_test(test_generated_views test/test_generated_views.py
      APPEND_ENV PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_py
      APPEND_LIBRARY_DIRS ${CMAKE_CURRENT_BINARY_DIR}
      ENV RMW_IMPLEMENTATION=rmw_fastrtps_cpp REGION_CPP_PEER=$<TARGET_FILE:region_cpp_peer>
      TIMEOUT 60)
  endif()
endif()

ament_package()
