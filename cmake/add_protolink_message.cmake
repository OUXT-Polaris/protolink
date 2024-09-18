# Copyright 2024 OUXT Polaris.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

function(add_protolink_message PROTO_FILE MESSAGE_NAME)
  find_package(Boost REQUIRED thread)
  include(FindProtobuf REQUIRED)
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})
  add_library(${MESSAGE_NAME} SHARED
    ${PROTO_SRCS}
    ${PROTO_HDRS}
  )
  target_link_libraries(${MESSAGE_NAME} ${PROTOBUF_LIBRARY})

  include_directories(
    include
    ${CMAKE_BINARY_DIR}
  )

  include(ExternalProject)

  if(TARGET nanopb)
  else()
    ExternalProject_Add(
      nanopb
      GIT_REPOSITORY https://github.com/nanopb/nanopb.git
      GIT_TAG master
      PREFIX ${CMAKE_BINARY_DIR}/nanopb
      CONFIGURE_COMMAND ""  # no configuration required
      BUILD_COMMAND ""      # no build required
      INSTALL_COMMAND ""    # no install required
      UPDATE_DISCONNECTED 1
    )
  endif()

  # Path to nanopb_generator.py
  set(NANOPB_GENERATOR_PY ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/generator/nanopb_generator.py)

  # Custom command to generate .c and .h files from .proto using nanopb_generator.py
  function(generate_nanopb_for_stm32cubeide PROTO_FILE MESSAGE_NAME)
    set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/nanopb_gen/STM32CubeIDE)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/nanopb_gen)
    file(MAKE_DIRECTORY ${GENERATED_DIR})

    file(COPY ${PROTO_FILE} DESTINATION ${GENERATED_DIR}/proto)
    get_filename_component(PROTO_FILENAME ${PROTO_FILE} NAME)

    add_custom_command(
      OUTPUT ${GENERATED_DIR}/${MESSAGE_NAME}.pb.c ${GENERATED_DIR}/${MESSAGE_NAME}.pb.h
      COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_BINARY_DIR}/nanopb/src/nanopb/generator python3 
        ${NANOPB_GENERATOR_PY} --output-dir=${GENERATED_DIR} proto/${PROTO_FILENAME}
      DEPENDS nanopb ${PROTO_FILE}
      WORKING_DIRECTORY ${GENERATED_DIR}
    )

    message(NOTICE "Files for nanopb have been generated. 
      Message name : ${MESSAGE_NAME}
      Please copy the files from the directory below to the development environment of the microcontroller. (STM32 CubeIDE)
      ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/nanopb_gen/STM32CubeIDE")
    
    add_custom_target(${MESSAGE_NAME}_nanopb_stm32cubeide ALL DEPENDS ${GENERATED_DIR}/${MESSAGE_NAME}.pb.c ${GENERATED_DIR}/${MESSAGE_NAME}.pb.h)
    if(TARGET ${MESSAGE_NAME}__from_ros)
      add_dependencies(${MESSAGE_NAME}_nanopb_stm32cubeide ${MESSAGE_NAME}__from_ros)
    else()
    endif()

    install(FILES 
      ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb.h
      ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_common.h
      ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_decode.h
      ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_encode.h
      DESTINATION share/${PROJECT_NAME}/nanopb_gen/STM32CubeIDE/Inc)
    install(FILES
      ${GENERATED_DIR}/proto/${MESSAGE_NAME}.pb.h
      DESTINATION share/${PROJECT_NAME}/nanopb_gen/STM32CubeIDE/Inc/proto)
    install(FILES 
      ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_common.c
      ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_decode.c
      ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_encode.c
      DESTINATION share/${PROJECT_NAME}/nanopb_gen/STM32CubeIDE/Src)
    install(FILES
      ${GENERATED_DIR}/proto/${MESSAGE_NAME}.pb.c
      DESTINATION share/${PROJECT_NAME}/nanopb_gen/STM32CubeIDE/Src/proto)
  endfunction()

  generate_nanopb_for_stm32cubeide(${PROTO_FILE} ${MESSAGE_NAME})

  install(TARGETS ${MESSAGE_NAME}
    EXPORT export_${MESSAGE_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include)

endfunction()

function(add_protolink_message_from_ros_message MESSAGE_PACKAGE MESSAGE_TYPE)
  set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_files)
  file(MAKE_DIRECTORY ${GENERATED_DIR})
  set(CONVERSION_HEADER_FILE conversion_${MESSAGE_PACKAGE}__${MESSAGE_TYPE}.hpp)
  set(CONVERSION_SOURCE_FILE conversion_${MESSAGE_PACKAGE}__${MESSAGE_TYPE}.cpp)

  set(PROTO_FILE ${GENERATED_DIR}/${MESSAGE_PACKAGE}__${MESSAGE_TYPE}.proto)

  if(${PROJECT_NAME} STREQUAL "protolink")
    set(GENERATE_PROTO_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_proto.py)
    set(JINJA_TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/template_converter.hpp.jinja)
  else()
    find_package(protolink REQUIRED)
    set(GENERATE_PROTO_SCRIPT ${protolink_DIR}/generate_proto.py)
    set(JINJA_TEMPLATE ${protolink_DIR}/template_converter.hpp.jinja)
  endif()

  add_custom_command(
    OUTPUT ${PROTO_FILE} ${CONVERSION_HEADER_FILE} ${CONVERSION_SOURCE_FILE}
    COMMAND python3 ${GENERATE_PROTO_SCRIPT} ${MESSAGE_PACKAGE}/${MESSAGE_TYPE} ${PROTO_FILE} ${CONVERSION_HEADER_FILE} ${CONVERSION_SOURCE_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${GENERATE_PROTO_SCRIPT} ${JINJA_TEMPLATE}
  )

  add_custom_target(${MESSAGE_PACKAGE}__${MESSAGE_TYPE}_from_ros ALL DEPENDS ${PROTO_FILE} ${CONVERSION_HEADER_FILE} ${CONVERSION_SOURCE_FILE})

  message("Generated protobuf message => ${PROTO_FILE}")

  include(FindProtobuf REQUIRED)

  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})
  include_directories(
    include
    ${CMAKE_BINARY_DIR}
  )
  add_library(${MESSAGE_PACKAGE}__${MESSAGE_TYPE}_proto SHARED ${PROTO_SRCS})
  target_link_libraries(${MESSAGE_PACKAGE}__${MESSAGE_TYPE}_proto ${PROTOBUF_LIBRARY})

  install(TARGETS ${MESSAGE_PACKAGE}__${MESSAGE_TYPE}_proto
    EXPORT export_${MESSAGE_PACKAGE}__${MESSAGE_TYPE}_proto
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include)

  # add_protolink_message(${PROTO_FILE} ${MESSAGE_PACKAGE}__${MESSAGE_TYPE})
endfunction()
