#include "OcrWorkerPool.h"
#include <iostream>

OcrWorkerPool::OcrWorkerPool(int /*numThreads*/)
{
}

OcrWorkerPool::~OcrWorkerPool() = default;

::grpc::Status OcrWorkerPool::ProcessRequest(const ocr::OcrRequest &req,
                                             ocr::OcrResponse &res)
{
    res.set_batch_id(req.batch_id());
    res.set_image_index(req.image_index());
    res.set_filename(req.filename());

    std::string text;
    long long ms = 0;

    bool ok = false;

    try {
        // Serialize access to Tesseract – it is NOT thread-safe
        std::lock_guard<std::mutex> lock(engineMutex_);
        ok = engine_.recognize(req.image_data(), text, ms);
    } catch (const std::exception &ex) {
        std::cerr << "[OCR] Exception in ProcessRequest: " << ex.what() << "\n";
        ok = false;
    } catch (...) {
        std::cerr << "[OCR] Unknown exception in ProcessRequest\n";
        ok = false;
    }

    res.set_processing_time_ms(ms);

    if (!ok) {
        res.set_success(false);
        res.set_error_message("OCR failed");
        res.set_text("");
    } else {
        res.set_success(true);
        res.set_text(text);
        res.clear_error_message();
    }

    return ::grpc::Status::OK;
}
