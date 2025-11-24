set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

cmake_minimum_required(VERSION 3.16)
project(distributed_ocr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------------------------------------------
# Homebrew locations for Protobuf + gRPC
# ----------------------------------------------------------------------
set(Protobuf_DIR "/usr/local/lib/cmake/protobuf")
set(gRPC_DIR "/usr/local/opt/grpc/lib/cmake/grpc")

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# ----------------------------------------------------------------------
# Qt (Widgets)
# ----------------------------------------------------------------------
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# ----------------------------------------------------------------------
# Tesseract (Homebrew, no CMake config)
# ----------------------------------------------------------------------
set(TESSERACT_ROOT "/usr/local/opt/tesseract")
include_directories(${TESSERACT_ROOT}/include)
link_directories(${TESSERACT_ROOT}/lib)

# --- Leptonica (for pixReadMem, pixDestroy) ---
set(LEPTONICA_ROOT "/usr/local/opt/leptonica")
include_directories(${LEPTONICA_ROOT}/include)
link_directories(${LEPTONICA_ROOT}/lib)

# ----------------------------------------------------------------------
# Helper functions to generate protobuf + gRPC C++ sources
# ----------------------------------------------------------------------
# Usage:
#   protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS file1.proto file2.proto)
function(protobuf_generate_cpp SRCS HDRS)
    if (NOT ARGN)
        message(FATAL_ERROR "protobuf_generate_cpp() called with no .proto files")
    endif()

    set(all_srcs "")
    set(all_hdrs "")

    foreach(proto_file ${ARGN})
        get_filename_component(proto_we ${proto_file} NAME_WE)

        set(src "${CMAKE_CURRENT_BINARY_DIR}/${proto_we}.pb.cc")
        set(hdr "${CMAKE_CURRENT_BINARY_DIR}/${proto_we}.pb.h")

        add_custom_command(
            OUTPUT ${src} ${hdr}
            COMMAND protobuf::protoc
            ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
                 -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
                 ${proto_file}
            DEPENDS ${proto_file}
        )

        list(APPEND all_srcs ${src})
        list(APPEND all_hdrs ${hdr})
    endforeach()

    set(${SRCS} ${all_srcs} PARENT_SCOPE)
    set(${HDRS} ${all_hdrs} PARENT_SCOPE)
endfunction()

# Usage:
#   grpc_generate_cpp(GRPC_SRCS GRPC_HDRS file1.proto file2.proto)
function(grpc_generate_cpp SRCS HDRS)
    if (NOT ARGN)
        message(FATAL_ERROR "grpc_generate_cpp() called with no .proto files")
    endif()

    set(all_srcs "")
    set(all_hdrs "")

    foreach(proto_file ${ARGN})
        get_filename_component(proto_we ${proto_file} NAME_WE)

        set(src "${CMAKE_CURRENT_BINARY_DIR}/${proto_we}.grpc.pb.cc")
        set(hdr "${CMAKE_CURRENT_BINARY_DIR}/${proto_we}.grpc.pb.h")

        add_custom_command(
            OUTPUT ${src} ${hdr}
            COMMAND protobuf::protoc
            ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
                 --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                 -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
                 ${proto_file}
            DEPENDS ${proto_file}
        )

        list(APPEND all_srcs ${src})
        list(APPEND all_hdrs ${hdr})
    endforeach()

    set(${SRCS} ${all_srcs} PARENT_SCOPE)
    set(${HDRS} ${all_hdrs} PARENT_SCOPE)
endfunction()

# ----------------------------------------------------------------------
# Generate protobuf / gRPC C++ files from our proto
# ----------------------------------------------------------------------
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_FILE ${PROTO_DIR}/ocr.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_FILE})

add_library(ocr_proto STATIC
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

target_include_directories(ocr_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}   # for generated *.pb.h
    ${PROTO_DIR}                  # for #include "ocr.proto" if ever needed
)

target_link_libraries(ocr_proto PUBLIC
    gRPC::grpc++
    protobuf::libprotobuf
)

# ----------------------------------------------------------------------
# Subdirectories (client + server)
# ----------------------------------------------------------------------
add_subdirectory(server)
add_subdirectory(client)
