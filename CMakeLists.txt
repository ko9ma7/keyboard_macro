cmake_minimum_required(VERSION 3.10)

project(KeyboardDriver)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)

# libusb 헤더 파일 경로 찾기
find_path(LIBUSB_INCLUDE_DIR
  NAMES libusb.h
  PATHS /usr/include/libusb-1.0
)

# libusb 라이브러리 경로 찾기
find_library(LIBUSB_LIBRARY
  NAMES usb-1.0
  PATHS /usr/lib /usr/local/lib
)

list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local")
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)

# 기존 find_package(gRPC ...) 호출 전에 추가
find_library(PROTOBUF_LIB NAMES protobuf libprotobuf PATHS ${CMAKE_PREFIX_PATH}/lib)
find_library(PROTOC_LIB NAMES protoc libprotoc PATHS ${CMAKE_PREFIX_PATH}/lib)
find_path(PROTOBUF_INCLUDE_DIR google/protobuf/message.h PATHS ${CMAKE_PREFIX_PATH}/include)

add_library(protobuf::libprotobuf STATIC IMPORTED)
set_target_properties(protobuf::libprotobuf PROPERTIES
  IMPORTED_LOCATION "${PROTOBUF_LIB}"
  INTERFACE_INCLUDE_DIRECTORIES "${PROTOBUF_INCLUDE_DIR}")

add_library(protobuf::libprotoc STATIC IMPORTED)
set_target_properties(protobuf::libprotoc PROPERTIES
  IMPORTED_LOCATION "${PROTOC_LIB}"
  INTERFACE_INCLUDE_DIRECTORIES "${PROTOBUF_INCLUDE_DIR}")

# gRPC 라이브러리 찾기
find_package(gRPC CONFIG REQUIRED)

# 헤더 파일 경로 추가
include_directories(${CMAKE_SOURCE_DIR}/include ${LIBUSB_INCLUDE_DIR})

# 소스 파일 추가
set(SOURCE_FILES
    main.cpp
    read_thread/read_thread.cpp
    logger_thread/logger_thread.cpp
    utils/keyboard_util.cpp
    grpc_thread/grpc_thread.cpp
    # global_vars.cpp 이 파일은 이제 필요하지 않으므로 제거합니다.
    # writeThread.cpp 파일 이름이 변경되지 않았다면 이 파일도 포함시킵니다.
)

# Protocol Buffers 파일에 대한 코드를 생성하도록 명령어 추가
set(PROTO_PATH "${CMAKE_SOURCE_DIR}/proto")
set(GENERATED_PROTOBUF_PATH "${CMAKE_BINARY_DIR}/generated/grpc")
file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})
include_directories(${CMAKE_BINARY_DIR}/generated)

# proto 폴더의 모든 .proto 파일을 찾습니다.
file(GLOB PROTO_FILES "${PROTO_PATH}/*.proto")

foreach(PROTO_FILE ${PROTO_FILES})
  # .proto 파일의 이름을 추출 (확장자 제외)
  get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)

  # protoc 명령을 실행
  add_custom_command(
    OUTPUT "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.cc"
           "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.h"
           "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.cc"
           "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.h"
    COMMAND protoc
    ARGS --grpc_out "${GENERATED_PROTOBUF_PATH}"
         --cpp_out "${GENERATED_PROTOBUF_PATH}"
         -I "${PROTO_PATH}"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
         "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
  )

  # 소스 파일에 생성된 Protocol Buffers 파일을 추가
  list(APPEND SOURCE_FILES
      "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.cc"
      "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.cc"
  )
endforeach()

add_executable(KeyboardDriver ${SOURCE_FILES})

# 라이브러리와 링크
target_link_libraries(KeyboardDriver PRIVATE 
    ${LIBUSB_LIBRARY} 
    pthread
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)

# 컴파일 옵션 추가 (예: 최적화, 경고 메시지 등)
target_compile_options(KeyboardDriver PRIVATE -Wall -O2)
