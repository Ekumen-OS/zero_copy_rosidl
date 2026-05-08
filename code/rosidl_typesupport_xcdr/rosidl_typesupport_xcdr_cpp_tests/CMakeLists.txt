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

# Generate interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/experimental/BasicTypes.msg"
  "msg/experimental/BoundedMessage.msg"
  "msg/experimental/UnboundedMessage.msg"
  "msg/InnerMessage.msg"
  "msg/NestedMessage.msg"
  "srv/AddTwoInts.srv"
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  
  ament_lint_auto_find_test_dependencies()
  
  # Test basic typesupport
  ament_add_gtest(test_basic_typesupport
    test/test_basic_typesupport.cpp
  )
  if(TARGET test_basic_typesupport)
    target_link_libraries(test_basic_typesupport
      rosidl_typesupport_xcdr_cpp::rosidl_typesupport_xcdr_cpp
      xcdr_buffers::xcdr_buffers
    )
    rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_xcdr_cpp")
    target_link_libraries(test_basic_typesupport ${cpp_typesupport_target})
  endif()
  
  # Test serialization
  ament_add_gtest(test_serialization
    test/test_serialization.cpp
  )
  if(TARGET test_serialization)
    target_link_libraries(test_serialization
      rosidl_typesupport_xcdr_cpp::rosidl_typesupport_xcdr_cpp
      xcdr_buffers::xcdr_buffers
    )
    rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_xcdr_cpp")
    target_link_libraries(test_serialization ${cpp_typesupport_target})
  endif()
  
  # Test nested message serialization
  ament_add_gtest(test_nested_serialization
    test/test_nested_serialization.cpp
  )
  if(TARGET test_nested_serialization)
    target_link_libraries(test_nested_serialization
      rosidl_typesupport_xcdr_cpp::rosidl_typesupport_xcdr_cpp
      xcdr_buffers::xcdr_buffers
    )
    rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_xcdr_cpp")
    target_link_libraries(test_nested_serialization ${cpp_typesupport_target})
  endif()
  
  # Test service typesupport
  ament_add_gtest(test_service_typesupport
    test/test_service_typesupport.cpp
  )
  if(TARGET test_service_typesupport)
    target_link_libraries(test_service_typesupport
      rosidl_typesupport_xcdr_cpp::rosidl_typesupport_xcdr_cpp
      xcdr_buffers::xcdr_buffers
    )
    rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_xcdr_cpp")
    target_link_libraries(test_service_typesupport ${cpp_typesupport_target})
  endif()
endif()

ament_package()
