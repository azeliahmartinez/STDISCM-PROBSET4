set(SERVER_SOURCES
    main.cpp
    OcrServiceImpl.cpp
    OcrWorkerPool.cpp
    OcrEngine.cpp
)

add_executable(ocr_server ${SERVER_SOURCES})

target_include_directories(ocr_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ocr_server
    ocr_proto
    gRPC::grpc++
    protobuf::libprotobuf
    tesseract
    leptonica
    pthread
)


