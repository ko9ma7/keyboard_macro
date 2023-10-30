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

# 헤더 파일 경로 추가
include_directories(${CMAKE_SOURCE_DIR}/include ${LIBUSB_INCLUDE_DIR})

# 소스 파일 추가
set(SOURCE_FILES
    main.cpp
    global_vars.cpp
    keyboard_util.cpp
    readThread.cpp
    writeThread.cpp
)

add_executable(KeyboardDriver ${SOURCE_FILES})

# 라이브러리와 링크
target_link_libraries(KeyboardDriver PRIVATE ${LIBUSB_LIBRARY} pthread)

# 컴파일 옵션 추가 (예: 최적화, 경고 메시지 등)
target_compile_options(KeyboardDriver PRIVATE -Wall -O2)
