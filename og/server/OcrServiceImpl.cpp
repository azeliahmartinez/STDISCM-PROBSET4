#include "OcrServiceImpl.h"

OcrServiceImpl::OcrServiceImpl(int numThreads)
    : workerPool_(numThreads)
{
}

::grpc::Status OcrServiceImpl::RecognizeImage(::grpc::ServerContext*,
                                              const ocr::OcrRequest* request,
                                              ocr::OcrResponse* response)
{
    return workerPool_.ProcessRequest(*request, *response);
}
