#pragma once

#include "ocr.grpc.pb.h"
#include "OcrWorkerPool.h"

class OcrServiceImpl final : public ocr::OcrService::Service
{
public:
    explicit OcrServiceImpl(int numThreads);

    ::grpc::Status RecognizeImage(::grpc::ServerContext* context,
                                  const ocr::OcrRequest* request,
                                  ocr::OcrResponse* response) override;

private:
    OcrWorkerPool workerPool_;
};
