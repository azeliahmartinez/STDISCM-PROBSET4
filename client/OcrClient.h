#pragma once

#include <QObject>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "ocr.grpc.pb.h"

class OcrClient : public QObject
{
    Q_OBJECT

public:
    explicit OcrClient(QObject *parent = nullptr);

    void sendImage(qint64 batchId,
                   int index,
                   const QString& filename,
                   const QByteArray& imageData);

signals:
    void ocrResultReady(qint64 batchId,
                        int index,
                        QString filename,
                        QString text,
                        qint64 processingMs,
                        bool success,
                        QString errorMessage);

private:
    std::unique_ptr<ocr::OcrService::Stub> stub_;
};
