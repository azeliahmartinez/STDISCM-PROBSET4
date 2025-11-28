#pragma once

#include <QMainWindow>
#include <QGridLayout>
#include <QScrollArea>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QImage>
#include <QFileInfo>
#include <QVBoxLayout>

#include <grpcpp/grpcpp.h>
#include "ocr.grpc.pb.h"


class ImageItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit ImageItemWidget(QWidget* parent = nullptr);

    void setImage(const QImage& img);       // set the preview image
    void setResult(const QString& text);    // set the OCR text

private:
    QLabel* imgLabel;
    QLabel* resultLabel;
};


class OcrClient : public QObject {
    Q_OBJECT

public:
    explicit OcrClient(QObject* parent = nullptr);

    // sends one image to the server for processing
    void sendImage(
        qint64 batchId,
        int index,
        const QString& filename,
        const QByteArray& imgData
    );

signals:
    void resultReady(
        qint64 batchId,
        int index,
        QString filename,
        QString text,
        bool success,
        QString error,
        qint64 ms
    );

private:
    std::unique_ptr<ocr::OcrService::Stub> stub_;
};


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    // when the user clicks "Upload Images"
    void onUploadClicked();

    // when an OCR result is returned from server
    void onResult(
        qint64 batchId,
        int index,
        const QString& filename,
        const QString& text,
        bool success,
        const QString& error,
        qint64 ms
    );

private:
    QWidget* centralWidget_;
    QVBoxLayout* mainLayout_;
    QPushButton* uploadButton_;
    QProgressBar* progressBar_;

    QScrollArea* scrollArea_;
    QWidget* scrollContent_;
    QGridLayout* gridLayout_;

    OcrClient client_;

    qint64 currentBatchId_ = 1;
    int nextIndex_ = 0;
    int totalImages_ = 0;
    int completed_ = 0;
    bool batchFinished_ = false;

    QMap<int, ImageItemWidget*> widgets_;

    const int columns_ = 4;

    void updateProgress();
    void clearUI();
    void prepareNewBatchIfNeeded();
};
