
// rm -rf build
// mkdir build
// cd build
// cmake ..
// cmake --build .

// # terminal 1
// cd server
// ./ocr_server


// # terminal 2
// cd client
// ./ocr_client

#include <grpcpp/grpcpp.h>
#include "OcrServiceImpl.h"
#include <iostream>

int main() {
    const std::string lanIP = "192.168.1.12";
    const std::string address = "0.0.0.0:50051";

    std::cout << "[Server] Initializing OCR Service..." << std::endl;
    OcrServiceImpl service(8);   // 8 worker threads

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::cout << "[Server] Starting server with 8 worker threads..." << std::endl;

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout << "[Server] Server is now running." << std::endl;
    std::cout << "[Server] Server reachable at: " 
          << lanIP << ":50051" << std::endl;
    std::cout << "[Server] Waiting for client connections..." << std::endl;

    server->Wait();
    return 0;
}

