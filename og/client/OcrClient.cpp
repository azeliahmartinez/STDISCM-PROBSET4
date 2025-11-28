#include "OcrClient.h"

#include <QByteArray>
#include <QDebug>
#include <thread>

OcrClient::OcrClient(QObject *parent)
    : QObject(parent)
{
    // TODO: change "localhost" to server IP if server is on another machine
    auto channel = grpc::CreateChannel("localhost:50051",
                                       grpc::InsecureChannelCredentials());
    stub_ = ocr::OcrService::NewStub(channel);
}

void OcrClient::sendImage(qint64 batchId,
                          int index,
                          const QString& filename,
                          const QByteArray& imageData)
{
    // Run RPC in a background thread so we don't block the Qt UI thread
    std::thread([this, batchId, index, filename, imageData]() {
        ocr::OcrRequest request;
        request.set_batch_id(batchId);
        request.set_image_index(index);
        request.set_filename(filename.toStdString());
        request.set_image_data(
            std::string(imageData.constData(), imageData.size()));

        ocr::OcrResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->RecognizeImage(&context, request, &response);

        if (!status.ok()) {
            emit ocrResultReady(batchId,
                                index,
                                filename,
                                QString(),
                                0,
                                false,
                                QString::fromStdString(status.error_message()));
        } else {
            emit ocrResultReady(batchId,
                                index,
                                QString::fromStdString(response.filename()),
                                QString::fromStdString(response.text()),
                                response.processing_time_ms(),
                                response.success(),
                                QString::fromStdString(response.error_message()));
        }
    }).detach();
}
