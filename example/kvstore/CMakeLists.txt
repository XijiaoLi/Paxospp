# Minimum CMake required
cmake_minimum_required(VERSION 3.5.1)

# Project
project(paxos)

include(../../common.cmake)

# Proto files
foreach(_package
  paxos kvstore)
  get_filename_component(${_package}_proto "../../protos/${_package}.proto" ABSOLUTE)
  get_filename_component(${_package}_proto_path "${paxos_proto}" PATH)
  # Generated sources
  set(${_package}_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_package}.pb.cc")
  set(${_package}_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_package}.pb.h")
  set(${_package}_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_package}.grpc.pb.cc")
  set(${_package}_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_package}.grpc.pb.h")
  add_custom_command(
    OUTPUT "${${_package}_proto_srcs}" "${${_package}_proto_hdrs}" "${${_package}_grpc_srcs}" "${${_package}_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
      --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
      -I "${${_package}_proto_path}"
      --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
      "${${_package}_proto}"
    DEPENDS "${${_package}_proto}")
endforeach()

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("../../include")

# paxos_grpc_proto
add_library(paxos_grpc_proto
  ${paxos_grpc_srcs}
  ${paxos_grpc_hdrs}
  ${paxos_proto_srcs}
  ${paxos_proto_hdrs})
target_link_libraries(paxos_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_library(paxoslib
  "../../src/paxos.cc"
  "../../include/paxos.h")
target_link_libraries(paxoslib
  paxos_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# kvstore_grpc_proto
add_library(kvstore_grpc_proto
  ${kvstore_grpc_srcs}
  ${kvstore_grpc_hdrs}
  ${kvstore_proto_srcs}
  ${kvstore_proto_hdrs})
target_link_libraries(kvstore_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Targets (client|server)
foreach(_target
  kvstore-server kvstore-client)
  add_executable(${_target} "${_target}.cc" "${_target}.h")
  target_link_libraries(${_target}
    paxoslib
    kvstore_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()
