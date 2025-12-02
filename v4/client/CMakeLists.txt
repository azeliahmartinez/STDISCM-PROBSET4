cmake_minimum_required(VERSION 3.16)

project(ocr_client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

qt_add_executable(ocr_client
    main.cpp
    MainWindow.cpp
    MainWindow.h
)

target_include_directories(ocr_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ocr_client PRIVATE
    Qt6::Widgets
    ocr_proto
    gRPC::grpc++
    protobuf::libprotobuf
)
