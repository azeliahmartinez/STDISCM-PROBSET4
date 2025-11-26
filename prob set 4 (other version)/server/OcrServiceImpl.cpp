#include "OcrServiceImpl.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <iostream>
#include <chrono>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

// holds all data needed for processing one image
struct OcrJob {
    int batchId;
    int index;
    std::string filename;
    std::string imageData;

    std::string* outText;
    bool* outSuccess;
    long long* outMs;
    std::string* outError;
    bool* finishedFlag;
};


class JobQueue {
public:
    void push(const OcrJob& job) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push(job);
        }
        cv_.notify_one();
    }

    OcrJob pop() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&]{ return !queue_.empty() || !running_; });

        if (!running_ && queue_.empty()) {
            return {}; // return empty job
        }

        OcrJob job = queue_.front();
        queue_.pop();
        return job;
    }

    // signal all workers to stop waiting
    void stop() {
        running_ = false;
        cv_.notify_all();
    }

private:
    std::queue<OcrJob> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};
};


class OcrEngine {
public:
    OcrEngine() {
        const char* tessdata = std::getenv("TESSDATA_PREFIX");
        if (!tessdata) tessdata = "/opt/homebrew/share/tessdata";

        // initialize Tesseract engine
        int rc = tess_.Init(tessdata, "eng");
        if (rc != 0) {
            std::cerr << "[OCR] Failed to initialize Tesseract.\n";
            initialized_ = false;
            return;
        }

        tess_.SetPageSegMode(tesseract::PSM_AUTO);
        initialized_ = true;
    }

    ~OcrEngine() {
        if (initialized_) tess_.End();
    }

    // performs OCR on one image
    bool recognize(const std::string &img, std::string &out, long long &ms) {
        if (!initialized_) return false;

        auto start = std::chrono::steady_clock::now();

        PIX* pix = pixReadMem((l_uint8*)img.data(), img.size());
        if (!pix) return false;

        // run Tesseract
        tess_.SetImage(pix);
        char* raw = tess_.GetUTF8Text();
        pixDestroy(&pix);

        auto end = std::chrono::steady_clock::now();
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        if (raw) {
            out = raw;
            delete[] raw;
            return true;
        }
        return false;
    }

private:
    tesseract::TessBaseAPI tess_;
    bool initialized_ = false;
};


class WorkerPool {
public:
    // start n worker threads
    WorkerPool(int n) : running_(true) {
        for (int i = 0; i < n; i++) {
            workers_.emplace_back(&WorkerPool::workerLoop, this);
        }
    }

    // stop queue + join all threads
    ~WorkerPool() {
        running_ = false;
        queue_.stop();

        for (auto &t : workers_) {
            if (t.joinable()) t.join();
        }
    }

    // add a new OCR job to the queue
    void pushJob(const OcrJob& job) {
        queue_.push(job);
    }

private:
     // worker thread loop: keeps taking jobs until shutdown
    void workerLoop() {
        OcrEngine engine;

        while (running_) {
            OcrJob job = queue_.pop();

            if (!running_) break;

            std::string text, error;
            long long ms = 0;
            bool ok = engine.recognize(job.imageData, text, ms);

            *(job.outText) = ok ? text : "";
            *(job.outSuccess) = ok;
            *(job.outMs) = ms;
            *(job.outError) = ok ? "" : "OCR failed";
            *(job.finishedFlag) = true;
        }
    }

    std::vector<std::thread> workers_;
    std::atomic<bool> running_;
    JobQueue queue_;
};


OcrServiceImpl::OcrServiceImpl(int workerCount)
    : workerCount_(workerCount)
{}

// gRPC method: RecognizeImage
grpc::Status OcrServiceImpl::RecognizeImage(
    grpc::ServerContext*,
    const ocr::OcrRequest* req,
    ocr::OcrResponse* res)
{
    // static worker pool across all RPC calls
    static WorkerPool pool(workerCount_);

    std::string text, error;
    long long ms = 0;
    bool success = false;
    bool finished = false;

    // prepare job
    OcrJob job;
    job.batchId = req->batch_id();
    job.index = req->image_index();
    job.filename = req->filename();
    job.imageData = req->image_data();

    job.outText = &text;
    job.outSuccess = &success;
    job.outMs = &ms;
    job.outError = &error;
    job.finishedFlag = &finished;

    // send job to worker threads
    pool.pushJob(job);

    // wait until worker thread sets finished = true
    while (!finished) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // fill response
    res->set_batch_id(job.batchId);
    res->set_image_index(job.index);
    res->set_filename(job.filename);
    res->set_text(text);
    res->set_success(success);
    res->set_error_message(error);
    res->set_processing_time_ms(ms);

    return grpc::Status::OK;
}