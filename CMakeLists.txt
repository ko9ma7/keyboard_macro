cmake_minimum_required(VERSION 3.10)

project(KeyboardDriver C CXX)

include(grpc.cmake)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)

# libusb 헤더 파일과 라이브러리 경로 찾기
find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATHS /usr/include/libusb-1.0)
find_library(LIBUSB_LIBRARY NAMES usb-1.0 PATHS /usr/lib /usr/local/lib)

# 헤더 파일 경로 추가
include_directories(${CMAKE_SOURCE_DIR}/include ${LIBUSB_INCLUDE_DIR})

# 기존의 KeyboardDriver 실행 파일
set(KEYBOARD_DRIVER_SOURCES main.cpp)
file(GLOB MORE_SOURCES "${CMAKE_SOURCE_DIR}/read_thread/*.cpp" "${CMAKE_SOURCE_DIR}/logger_thread/*.cpp"
    "${CMAKE_SOURCE_DIR}/utils/*.cpp" "${CMAKE_SOURCE_DIR}/grpc_thread/*.cpp" "${CMAKE_SOURCE_DIR}/controller/*.cpp")
list(APPEND KEYBOARD_DRIVER_SOURCES ${MORE_SOURCES})


# 새로운 KeyboardDriverRestarter 실행 파일
set(KEYBOARD_RESTARTER_SOURCES restart.cpp)  # restart.cpp를 포함
list(APPEND KEYBOARD_RESTARTER_SOURCES ${MORE_SOURCES})

# Protocol Buffers 파일 생성
set(PROTO_PATH "${CMAKE_SOURCE_DIR}/proto")
set(GENERATED_PROTOBUF_PATH "${CMAKE_BINARY_DIR}/generated/grpc")
file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})
include_directories(${CMAKE_BINARY_DIR}/generated)
file(GLOB PROTO_FILES "${PROTO_PATH}/*.proto")
foreach(PROTO_FILE ${PROTO_FILES})
  get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
  add_custom_command(
    OUTPUT "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.cc"
           "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.h"
           "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.cc"
           "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.h"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${GENERATED_PROTOBUF_PATH}"
         --cpp_out "${GENERATED_PROTOBUF_PATH}"
         -I "${PROTO_PATH}"
         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
         "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
  )
  list(APPEND KEYBOARD_DRIVER_SOURCES "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.cc" "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.cc")
  list(APPEND KEYBOARD_RESTARTER_SOURCES "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.cc" "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.cc")
endforeach()
add_executable(KeyboardDriver ${KEYBOARD_DRIVER_SOURCES})
add_executable(KeyboardDriverRestarter ${KEYBOARD_RESTARTER_SOURCES})

# 라이브러리와 링크
target_link_libraries(KeyboardDriver PRIVATE ${LIBUSB_LIBRARY} pthread ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})
target_link_libraries(KeyboardDriverRestarter PRIVATE ${LIBUSB_LIBRARY} pthread ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})

# 컴파일 옵션
target_compile_options(KeyboardDriver PRIVATE -Wall -O2)
target_compile_options(KeyboardDriverRestarter PRIVATE -Wall -O2)

# 설치 경로
install(TARGETS KeyboardDriver DESTINATION ${CMAKE_SOURCE_DIR}/rspi)
install(TARGETS KeyboardDriverRestarter DESTINATION ${CMAKE_SOURCE_DIR}/rspi)
