cmake_minimum_required(VERSION 3.16)

project(ocr_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(TESSERACT REQUIRED tesseract)
pkg_check_modules(LEPTONICA REQUIRED lept)

add_executable(ocr_server
    main.cpp
    OcrServiceImpl.cpp
)

target_include_directories(ocr_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
)

target_link_libraries(ocr_server PRIVATE
    ocr_proto
    gRPC::grpc++
    protobuf::libprotobuf
    ${TESSERACT_LIBRARIES}
    ${LEPTONICA_LIBRARIES}
    Threads::Threads
)
