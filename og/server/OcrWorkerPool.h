#pragma once

#include <grpcpp/grpcpp.h>
#include <mutex>

#include "ocr.grpc.pb.h"
#include "OcrEngine.h"

class OcrWorkerPool
{
public:
    explicit OcrWorkerPool(int numThreads);
    ~OcrWorkerPool();

    ::grpc::Status ProcessRequest(const ocr::OcrRequest &req,
                                  ocr::OcrResponse &res);

private:
    // Single OCR engine shared by the pool
    OcrEngine engine_;

    // Protects Tesseract (not thread-safe)
    std::mutex engineMutex_;
};
