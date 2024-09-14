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

  # Path to nanopb_generator.py
  set(NANOPB_GENERATOR_PY ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/generator/nanopb_generator.py)

  # Custom command to generate .c and .h files from .proto using nanopb_generator.py
  function(generate_nanopb PROTO_FILE MESSAGE_NAME)
    set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/nanopb_gen)
    file(MAKE_DIRECTORY ${GENERATED_DIR})

    add_custom_command(
      OUTPUT ${GENERATED_DIR}/${MESSAGE_NAME}.pb.c ${GENERATED_DIR}/${MESSAGE_NAME}.pb.h
      COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_BINARY_DIR}/nanopb/src/nanopb/generator python3 
        ${NANOPB_GENERATOR_PY} --output-dir=${GENERATED_DIR} ${PROTO_FILE}
      DEPENDS nanopb ${PROTO_FILE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Generating ${MESSAGE_NAME}.pb.c and ${MESSAGE_NAME}.pb.h from ${PROTO_FILE}"
    )
    
    add_custom_target(${MESSAGE_NAME}_nanopb ALL DEPENDS ${GENERATED_DIR}/${MESSAGE_NAME}.pb.c ${GENERATED_DIR}/${MESSAGE_NAME}.pb.h)

    install(FILES 
        ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb.h
        ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_common.c
        ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_common.h
        ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_decode.c
        ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_decode.h
        ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_encode.c
        ${CMAKE_BINARY_DIR}/nanopb/src/nanopb/pb_encode.h
      DESTINATION share/${PROJECT_NAME}/nanopb_gen/proto)
    install(
      DIRECTORY ${GENERATED_DIR}
      DESTINATION share/${PROJECT_NAME})

    message(NOTICE
      "Files for nanopb have been generated. 
      Please copy the files from the directory below to the development environment of the microcontroller.
      ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/nanopb_gen/proto")
  endfunction()

  generate_nanopb(${PROTO_FILE} ${MESSAGE_NAME})

  install(TARGETS ${MESSAGE_NAME}
    EXPORT export_${MESSAGE_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include)

endfunction()
