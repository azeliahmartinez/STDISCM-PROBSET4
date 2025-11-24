#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "ocr.pb.h"
#include <grpcpp/grpcpp.h>
#include "OcrEngine.h"

class OcrWorkerPool
{
public:
    explicit OcrWorkerPool(int numThreads);
    ~OcrWorkerPool();

    ::grpc::Status ProcessRequest(const ocr::OcrRequest& req, ocr::OcrResponse& res);

private:
    OcrEngine engine_;   // simple wrapper around Tesseract
};
