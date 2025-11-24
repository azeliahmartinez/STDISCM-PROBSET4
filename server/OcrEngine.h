#pragma once

#include <string>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

class OcrEngine
{
public:
    OcrEngine();
    ~OcrEngine();

    // imageData: encoded PNG/JPEG bytes
    bool recognize(const std::string &imageData,
                   std::string &outText,
                   long long &processingMs);

private:
    tesseract::TessBaseAPI tess_;
    bool initialized_ = false;
};
