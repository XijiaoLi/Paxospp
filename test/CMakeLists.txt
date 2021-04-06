# Minimum CMake required
cmake_minimum_required(VERSION 3.5.1)

# Project
project(paxos)

include(../common.cmake)

# # Protobuf
# set(protobuf_MODULE_COMPATIBLE TRUE)
# find_package(Protobuf CONFIG REQUIRED)
# message(STATUS "Using protobuf ${protobuf_VERSION}")
#
# # Protobuf-compiler
# set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
#
# # gRPC
# find_package(gRPC CONFIG REQUIRED)
# message(STATUS "Using gRPC ${gRPC_VERSION}")
# set(_GRPC_GRPCPP gRPC::grpc++)
# set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# Proto file
get_filename_component(hw_proto "../protos/paxos.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/paxos.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/paxos.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/paxos.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/paxos.grpc.pb.h")
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# hw_grpc_proto
add_library(hw_grpc_proto
  ${hw_grpc_srcs}
  ${hw_grpc_hdrs}
  ${hw_proto_srcs}
  ${hw_proto_hdrs})
target_link_libraries(hw_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_library(paxoslib
  "../lib/paxos.cc"
  "../include/paxos.h")
target_link_libraries(paxoslib
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Targets (client|server)
foreach(_target
  test)
  add_executable(${_target} "${_target}.cc")
  target_include_directories(${_target} PRIVATE "../lib")
  target_link_libraries(${_target}
    hw_grpc_proto
    paxoslib
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()
