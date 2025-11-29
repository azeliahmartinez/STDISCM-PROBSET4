
// cd /Users/zae/Desktop/STDISCM-PROBSET4
// rm -rf build
// mkdir build
// cd build
// cmake ..
// make -j8

// # terminal 1
// cd server
// ./ocr_server


// # terminal 2
// cd client
// ./ocr_client


#include <grpcpp/grpcpp.h>
#include "OcrServiceImpl.h"

int main(int argc, char** argv)
{
    const std::string server_address = "0.0.0.0:50051";

    OcrServiceImpl service(/*numThreads=*/8);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "OCR server listening on " << server_address << std::endl;

    server->Wait();
    return 0;
}
