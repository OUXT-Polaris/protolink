cmake_minimum_required(VERSION 3.8)
project(protolink)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake_auto REQUIRED)
find_package(PahoMqttCpp REQUIRED)
include(FindProtobuf REQUIRED)
ament_auto_find_build_dependencies()

ament_auto_add_library(protolink SHARED
  src/client.cpp
)
target_link_libraries(protolink )

install(
  DIRECTORY cmake
  DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  include(cmake/add_protolink_message.cmake)
  add_protolink_message_from_ros_message("std_msgs" "String")

  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(unittest test/test.cpp)
  target_link_libraries(unittest protolink 
    ${PROTOBUF_LIBRARY}
    PahoMqttCpp::paho-mqttpp3
    std_msgs__String_proto)
endif()

ament_auto_package(CONFIG_EXTRAS "${PROJECT_NAME}_ament_cmake-extras.cmake")
