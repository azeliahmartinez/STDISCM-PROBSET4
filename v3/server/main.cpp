#include <grpcpp/grpcpp.h>
#include "OcrServiceImpl.h"
#include <iostream>

int main() {
    const std::string address = "0.0.0.0:50051";

    OcrServiceImpl service(8);   // 8 worker threads

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "[Server] Running at " << address << std::endl;

    server->Wait();
    return 0;
}
