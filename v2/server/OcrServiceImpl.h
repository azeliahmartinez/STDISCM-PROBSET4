#pragma once

#include <grpcpp/grpcpp.h>
#include "ocr.grpc.pb.h"

class OcrServiceImpl : public ocr::OcrService::Service {
public:
    explicit OcrServiceImpl(int workerCount);

    grpc::Status RecognizeImage(
        grpc::ServerContext* context,
        const ocr::OcrRequest* request,
        ocr::OcrResponse* response
    ) override;

private:
    int workerCount_;
};
