set(CLIENT_SOURCES
    main.cpp
    MainWindow.cpp
    ImageItemWidget.cpp
    OcrClient.cpp
)

set(CLIENT_HEADERS
    MainWindow.h
    ImageItemWidget.h
    OcrClient.h
)

set(CLIENT_UI
    MainWindow.ui
)

qt_wrap_ui(CLIENT_UI_HDRS ${CLIENT_UI})   # Qt5 style; for Qt6 use qt_add_executable

add_executable(ocr_client
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}
    ${CLIENT_UI_HDRS}
)

target_include_directories(ocr_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ocr_client
    ocr_proto
    Qt6::Widgets      # or Qt5::Widgets
    gRPC::grpc++
    protobuf::libprotobuf
)

target_include_directories(ocr_client PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}   # this is where ui_MainWindow.h is generated
)