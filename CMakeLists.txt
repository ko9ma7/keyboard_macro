cmake_minimum_required(VERSION 3.10)

project(KeyboardDriver C CXX)

include(grpc.cmake)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# libcurl 라이브러리 및 헤더 파일 경로 찾기
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# jsoncpp 라이브러리 및 헤더 파일 경로 찾기
find_path(JSONCPP_INCLUDE_DIR json/json.h PATHS /usr/include/jsoncpp /usr/local/include/jsoncpp)
find_library(JSONCPP_LIBRARY jsoncpp PATHS /usr/lib /usr/local/lib)
include_directories(${JSONCPP_INCLUDE_DIR})

# libusb 헤더 파일과 라이브러리 경로 찾기
find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATHS /usr/include/libusb-1.0)
find_library(LIBUSB_LIBRARY NAMES usb-1.0 PATHS /usr/lib /usr/local/lib)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED libsystemd)
find_package(Boost REQUIRED COMPONENTS system)

# 헤더 파일 경로 추가
include_directories(${CMAKE_SOURCE_DIR}/include ${LIBUSB_INCLUDE_DIR} ${SYSTEMD_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

# 기존의 KeyboardDriver 실행 파일
set(KEYBOARD_DRIVER_SOURCES main.cpp)
file(GLOB MORE_SOURCES "${CMAKE_SOURCE_DIR}/read_thread/*.cpp" "${CMAKE_SOURCE_DIR}/logger_thread/*.cpp"
    "${CMAKE_SOURCE_DIR}/utils/*.cpp" "${CMAKE_SOURCE_DIR}/grpc_thread/*.cpp" "${CMAKE_SOURCE_DIR}/controller/*.cpp" "${CMAKE_SOURCE_DIR}/bluetooth/*.cpp")
list(APPEND KEYBOARD_DRIVER_SOURCES ${MORE_SOURCES})

file(GLOB UTILS_SOURCE "${CMAKE_SOURCE_DIR}/utils/*.cpp")

# 새로운 KeyboardDriverRestarter 실행 파일
set(KEYBOARD_RESTARTER_SOURCES restart.cpp)  # restart.cpp를 포함
list(APPEND KEYBOARD_RESTARTER_SOURCES ${MORE_SOURCES})

set(KEYBOARD_RUN_MACRO_SOURCES macro.cpp)
list(APPEND KEYBOARD_RUN_MACRO_SOURCES ${UTILS_SOURCE})

set(KEYBOARD_BLUETOOTH_SOURCES bluetooth.cpp)  # bluetooth.cpp를 포함
list(APPEND KEYBOARD_BLUETOOTH_SOURCES ${MORE_SOURCES})

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
  list(APPEND KEYBOARD_BLUETOOTH_SOURCES "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.cc" "${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.cc")
endforeach()

# updater.cpp 파일 추가
set(UPDATER_SOURCES updater.cpp)
list(APPEND UPDATER_SOURCES ${UTILS_SOURCE})

# 플랫폼 감지
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(STATUS "Detected ARM 64-bit architecture")
    set(PLATFORM "ARM 64bit")
    set(BLUETOOTH "No Bluetooth")
    add_executable(KeyboardDriver ${UPDATER_SOURCES})
    target_compile_options(KeyboardDriver PRIVATE -Wall -O2)
    target_link_libraries(KeyboardDriver PRIVATE ${LIBUSB_LIBRARY} ${SYSTEMD_LIBRARIES} ${Boost_LIBRARIES} ${CURL_LIBRARIES} ${JSONCPP_LIBRARY} pthread ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})
    install(TARGETS KeyboardDriver DESTINATION ${CMAKE_SOURCE_DIR}/rspi)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    message(STATUS "Detected ARM 32-bit architecture")
    set(PLATFORM "ARM 32bit")
    set(BLUETOOTH "Bluetooth")
    add_executable(KeyboardDriver ${UPDATER_SOURCES})
    target_compile_definitions(KeyboardDriver PRIVATE HAVE_BLUETOOTH)
    target_compile_options(KeyboardDriver PRIVATE -Wall -O2)
    target_link_libraries(KeyboardDriver PRIVATE ${LIBUSB_LIBRARY} ${SYSTEMD_LIBRARIES} ${Boost_LIBRARIES} ${CURL_LIBRARIES} ${JSONCPP_LIBRARY} pthread ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})
    install(TARGETS KeyboardDriver DESTINATION ${CMAKE_SOURCE_DIR}/rspi)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# 기존 실행 파일들 추가
add_executable(KeyboardDriverRestarter ${KEYBOARD_RESTARTER_SOURCES})
add_executable(RunMacro ${KEYBOARD_RUN_MACRO_SOURCES})
add_executable(KeyboardBluetoothDriver ${KEYBOARD_BLUETOOTH_SOURCES})

# 라이브러리와 링크
target_link_libraries(KeyboardDriverRestarter PRIVATE ${LIBUSB_LIBRARY} ${SYSTEMD_LIBRARIES} ${Boost_LIBRARIES} ${CURL_LIBRARIES} ${JSONCPP_LIBRARY} pthread ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})
target_link_libraries(RunMacro PRIVATE ${LIBUSB_LIBRARY} ${SYSTEMD_LIBRARIES} ${Boost_LIBRARIES} ${CURL_LIBRARIES} ${JSONCPP_LIBRARY} pthread ${_REFLECTION})
target_link_libraries(KeyboardBluetoothDriver PRIVATE ${LIBUSB_LIBRARY} ${SYSTEMD_LIBRARIES} ${Boost_LIBRARIES} ${CURL_LIBRARIES} ${JSONCPP_LIBRARY} pthread ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})

# 컴파일 옵션
target_compile_options(KeyboardDriverRestarter PRIVATE -Wall -O2)
target_compile_options(RunMacro PRIVATE -Wall -O2)
target_compile_options(KeyboardBluetoothDriver PRIVATE -Wall -O2)

# 설치 경로
install(TARGETS KeyboardDriverRestarter DESTINATION ${CMAKE_SOURCE_DIR}/rspi)
install(TARGETS RunMacro DESTINATION ${CMAKE_SOURCE_DIR}/rspi)
install(TARGETS KeyboardBluetoothDriver DESTINATION ${CMAKE_SOURCE_DIR}/rspi)
