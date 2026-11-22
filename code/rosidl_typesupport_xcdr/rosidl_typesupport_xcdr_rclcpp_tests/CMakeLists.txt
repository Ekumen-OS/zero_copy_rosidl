cmake_minimum_required(VERSION 3.8)
project(rosidl_typesupport_xcdr_rclcpp_tests)

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  find_package(rosidl_runtime_c REQUIRED)

  ament_lint_auto_find_test_dependencies()
  get_target_property(runtime_c_includes rosidl_runtime_c::rosidl_runtime_c
    INTERFACE_INCLUDE_DIRECTORIES)
  find_file(runtime_constraints_header rosidl_runtime_c/constraints.h
    HINTS ${runtime_c_includes} NO_DEFAULT_PATH)
  if(runtime_constraints_header)
    find_package(rclcpp REQUIRED)
    find_package(rosidl_typesupport_xcdr_cpp_tests REQUIRED)
    ament_add_gtest(test_region_rclcpp test/test_region_rclcpp.cpp)
    target_compile_features(test_region_rclcpp PRIVATE cxx_std_20)
    target_link_libraries(test_region_rclcpp
      rclcpp::rclcpp
      rosidl_typesupport_xcdr_cpp_tests::rosidl_typesupport_xcdr_cpp_tests__rosidl_typesupport_cpp)
  endif()
endif()

ament_package()
