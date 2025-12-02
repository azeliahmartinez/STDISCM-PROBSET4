#include "OcrServiceImpl.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <iostream>
#include <chrono>
#include <filesystem>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

// Artificial delay per OCR job (for demo visibility)
static constexpr int kArtificialDelayMs = 300;

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
        const char* env = std::getenv("TESSDATA_PREFIX");
        const char* tessdata = nullptr;

        if (env && std::strlen(env) > 0) {
            tessdata = env;
        } 
        else {
            // Apple Silicon Homebrew
            if (std::filesystem::exists("/opt/homebrew/share/tessdata/eng.traineddata")) {
                tessdata = "/opt/homebrew/share/tessdata";
            }
            // Intel Homebrew 
            else if (std::filesystem::exists("/usr/local/share/tessdata/eng.traineddata")) {
                tessdata = "/usr/local/share/tessdata";
            }
            // Apple Homebrew Cellar 
            else if (std::filesystem::exists("/opt/homebrew/opt/tesseract/share/tessdata/eng.traineddata")) {
                tessdata = "/opt/homebrew/opt/tesseract/share/tessdata";
            }
            // Intel Cellar 
            else if (std::filesystem::exists("/usr/local/Cellar/tesseract/5.5.1_1/share/tessdata/eng.traineddata")) {
                tessdata = "/usr/local/Cellar/tesseract/5.5.1_1/share/tessdata";
            }
            else {
                tessdata = ".";  // as last fallback
            }
        }

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

    bool recognize(const std::string &img, std::string &out, long long &ms) {
        if (!initialized_) return false;

        auto start = std::chrono::steady_clock::now();

        PIX* pix = pixReadMem((l_uint8*)img.data(), img.size());
        if (!pix) return false;

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
    WorkerPool(int n) : running_(true) {
        for (int i = 0; i < n; i++) {
            workers_.emplace_back(&WorkerPool::workerLoop, this);
        }
    }

    ~WorkerPool() {
        running_ = false;
        queue_.stop();

        for (auto &t : workers_) {
            if (t.joinable()) t.join();
        }
    }

    void pushJob(const OcrJob& job) {
        queue_.push(job);
    }

private:
    void workerLoop() {
        OcrEngine engine;

        while (running_) {
            OcrJob job = queue_.pop();

            if (!running_) break;

            std::string text, error;
            long long ms = 0;
            bool ok = engine.recognize(job.imageData, text, ms);

            // Artificial delay to slow down completion for demo visibility
            if (kArtificialDelayMs > 0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(kArtificialDelayMs)
                );
            }

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

grpc::Status OcrServiceImpl::RecognizeImage(
    grpc::ServerContext*,
    const ocr::OcrRequest* req,
    ocr::OcrResponse* res)
{
    static WorkerPool pool(workerCount_);

    std::cout << "[Server] Received image request from client:" << std::endl;
    std::cout << "         Filename: " << req->filename() 
              << " | Index: " << req->image_index()
              << " | Batch: " << req->batch_id() << std::endl;

    std::string text, error;
    long long ms = 0;
    bool success = false;
    bool finished = false;

    // build OCR Job
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

    // push job into worker pool
    pool.pushJob(job);
    std::cout << "[Server] Job pushed to worker pool..." << std::endl;

    // busy wait until job is done
    while (!finished) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (success) {
        std::cout << "[Server] OCR SUCCESS for [" << job.filename << "]" 
                  << " | Time: " << ms << " ms" << std::endl;
    } else {
        std::cout << "[Server] OCR FAILED for [" << job.filename << "]"
                  << " | Error: " << error << std::endl;
    }

    // fill gRPC response
    res->set_batch_id(job.batchId);
    res->set_image_index(job.index);
    res->set_filename(job.filename);
    res->set_text(text);
    res->set_success(success);
    res->set_error_message(error);
    res->set_processing_time_ms(ms);

    std::cout << "[Server] Sending OCR response back to client..." << std::endl;

    return grpc::Status::OK;
}