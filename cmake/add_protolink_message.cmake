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

  install(TARGETS ${MESSAGE_NAME}
    EXPORT export_${MESSAGE_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include)

endfunction()
