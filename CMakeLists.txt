cmake_minimum_required(VERSION 3.10)

project(KeyboardDriver C CXX)

include(grpc.cmake)

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

# 헤더 파일 경로 추가
include_directories(${CMAKE_SOURCE_DIR}/include ${LIBUSB_INCLUDE_DIR})

# 소스 파일 추가
set(SOURCE_FILES
    main.cpp
)

# 각 폴더에 있는 .cpp 파일을 자동으로 찾아서 SOURCE_FILES에 추가
file(GLOB READ_THREAD_SOURCES "${CMAKE_SOURCE_DIR}/read_thread/*.cpp")
file(GLOB LOGGER_THREAD_SOURCES "${CMAKE_SOURCE_DIR}/logger_thread/*.cpp")
file(GLOB UTILS_SOURCES "${CMAKE_SOURCE_DIR}/utils/*.cpp")
file(GLOB GRPC_THREAD_SOURCES "${CMAKE_SOURCE_DIR}/grpc_thread/*.cpp")

# 찾아진 소스 파일을 SOURCE_FILES에 추가
list(APPEND SOURCE_FILES ${READ_THREAD_SOURCES} ${LOGGER_THREAD_SOURCES} ${UTILS_SOURCES} ${GRPC_THREAD_SOURCES})

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
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${GENERATED_PROTOBUF_PATH}"
         --cpp_out "${GENERATED_PROTOBUF_PATH}"
         -I "${PROTO_PATH}"
         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
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
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)

# 컴파일 옵션 추가 (예: 최적화, 경고 메시지 등)
target_compile_options(KeyboardDriver PRIVATE -Wall -O2)
