#include "OcrEngine.h"

#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>

OcrEngine::OcrEngine()
{
    // Try TESSDATA_PREFIX first, else Homebrew default
    const char *tessdata = std::getenv("TESSDATA_PREFIX");
    if (!tessdata) {
        tessdata = "/opt/homebrew/share/tessdata";
    }

    int rc = tess_.Init(tessdata, "eng");
    if (rc != 0) {
        std::cerr << "[OCR] Failed to initialize Tesseract with tessdata path: "
                  << tessdata << " (rc=" << rc << ")\n";
        initialized_ = false;
        return;
    }

    tess_.SetPageSegMode(tesseract::PSM_SINGLE_WORD);
    initialized_ = true;

    std::cerr << "[OCR] Tesseract initialized OK, tessdata=" << tessdata << "\n";
}

OcrEngine::~OcrEngine()
{
    if (initialized_) {
        tess_.End();
    }
}

bool OcrEngine::recognize(const std::string &imageData,
                          std::string &outText,
                          long long &processingMs)
{
    processingMs = 0;
    outText.clear();

    if (!initialized_) {
        std::cerr << "[OCR] recognize() called but Tesseract is not initialized\n";
        return false;
    }

    auto start = std::chrono::steady_clock::now();

    try {
        // Decode image from memory
        PIX *pix = pixReadMem(
            reinterpret_cast<const unsigned char *>(imageData.data()),
            imageData.size());

        if (!pix) {
            std::cerr << "[OCR] pixReadMem() failed – invalid image data (" 
                      << imageData.size() << " bytes)\n";
            return false;
        }

        tess_.SetImage(pix);

        char *raw = tess_.GetUTF8Text();

        pixDestroy(&pix);

        auto end = std::chrono::steady_clock::now();
        processingMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           end - start)
                           .count();

        if (!raw) {
            std::cerr << "[OCR] GetUTF8Text() returned nullptr\n";
            return false;
        }

        outText.assign(raw);
        delete[] raw;

        return true;
    }
    catch (const std::exception &ex) {
        std::cerr << "[OCR] Exception in recognize(): " << ex.what() << "\n";
        return false;
    }
    catch (...) {
        std::cerr << "[OCR] Unknown exception in recognize()\n";
        return false;
    }
}
