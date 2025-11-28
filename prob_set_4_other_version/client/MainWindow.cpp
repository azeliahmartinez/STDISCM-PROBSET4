#include "MainWindow.h"
#include <QFileDialog>
#include <QBuffer>
#include <QPixmap>
#include <thread>

// ============================
// Server address configuration
// ============================
// YOUR IP (server) = 10.5.146.131
static constexpr const char* kServerAddress = "10.143.25.221:50051";

// ============================
// ImageItemWidget
// ============================

ImageItemWidget::ImageItemWidget(QWidget* parent)
    : QWidget(parent),
      imgLabel(new QLabel(this)),
      resultLabel(new QLabel(this))
{
    setStyleSheet("background-color: transparent;");
    setFixedSize(210, 140);

    imgLabel->setAlignment(Qt::AlignCenter);
    resultLabel->setAlignment(Qt::AlignCenter);

    imgLabel->setStyleSheet(
        "background-color: transparent;"
        "padding: 0px;"
    );

    resultLabel->setStyleSheet(
        "background-color: transparent;"
        "color: #ffffff;"
        "font-size: 13px;"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    layout->addWidget(imgLabel);
    layout->addWidget(resultLabel);
    setLayout(layout);
}

void ImageItemWidget::setImage(const QImage& img) {
    imgLabel->setPixmap(
        QPixmap::fromImage(img).scaled(
            180, 45,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        )
    );
}

void ImageItemWidget::setResult(const QString& text) {
    resultLabel->setText(text);
}

// ============================
// OcrClient
// ============================

OcrClient::OcrClient(QObject* parent)
    : QObject(parent)
{
    auto channel = grpc::CreateChannel(
        kServerAddress,
        grpc::InsecureChannelCredentials()
    );
    stub_ = ocr::OcrService::NewStub(channel);
}

void OcrClient::sendImage(
    qint64 batchId,
    int index,
    const QString& filename,
    const QByteArray& imgData
) {
    std::thread([=]() {
        ocr::OcrRequest req;
        req.set_batch_id(batchId);
        req.set_image_index(index);
        req.set_filename(filename.toStdString());
        req.set_image_data(std::string(imgData.constData(), imgData.size()));

        ocr::OcrResponse res;
        grpc::ClientContext ctx;

        grpc::Status status = stub_->RecognizeImage(&ctx, req, &res);

        if (!status.ok()) {
            emit resultReady(batchId, index, filename, "",
                             false,
                             QString::fromStdString(status.error_message()),
                             0);
        } else {
            emit resultReady(batchId, index,
                             QString::fromStdString(res.filename()),
                             QString::fromStdString(res.text()),
                             res.success(),
                             QString::fromStdString(res.error_message()),
                             res.processing_time_ms());
        }
    }).detach();
}

// ============================
// MainWindow
// ============================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setMinimumSize(1200, 800);
    setStyleSheet("background-color: #303030; color: white;");

    centralWidget_ = new QWidget(this);
    mainLayout_ = new QVBoxLayout(centralWidget_);

    uploadButton_ = new QPushButton("Upload Images", this);
    uploadButton_->setStyleSheet(
        "background-color: #3d3d3d;"
        "padding: 10px 20px;"
        "font-weight: 600;"
        "font-size: 14px;"
        "color: white;"
        "border-radius: 4px;"
    );

    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setStyleSheet(
        "QProgressBar {"
        "   background-color: #3b3b3b;"
        "   border: none;"
        "   height: 5px;"
        "   border-radius: 3px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #007BFF;"
        "   border-radius: 3px;"
        "}"
    );

    scrollArea_ = new QScrollArea(this);
    scrollArea_->setStyleSheet("background-color: #303030;");
    scrollArea_->setWidgetResizable(true);

    scrollContent_ = new QWidget(this);
    gridLayout_ = new QGridLayout(scrollContent_);
    gridLayout_->setContentsMargins(10, 10, 10, 10);
    gridLayout_->setHorizontalSpacing(24);
    gridLayout_->setVerticalSpacing(28);

    scrollContent_->setLayout(gridLayout_);
    scrollArea_->setWidget(scrollContent_);

    mainLayout_->addWidget(uploadButton_);
    mainLayout_->addWidget(progressBar_);
    mainLayout_->addWidget(scrollArea_);

    centralWidget_->setLayout(mainLayout_);
    setCentralWidget(centralWidget_);

    connect(uploadButton_, &QPushButton::clicked,
            this, &MainWindow::onUploadClicked);

    connect(&client_, &OcrClient::resultReady,
            this, &MainWindow::onResult);
}

void MainWindow::clearUI() {
    QLayoutItem* item;
    while ((item = gridLayout_->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    widgets_.clear();
}

void MainWindow::prepareNewBatchIfNeeded() {
    if (batchFinished_) {
        clearUI();
        nextIndex_ = 0;
        totalImages_ = 0;
        completed_ = 0;
        currentBatchId_++;
        batchFinished_ = false;
        progressBar_->setValue(0);
    }
}

void MainWindow::updateProgress() {
    if (totalImages_ == 0) return;

    int percent = (completed_ * 100) / totalImages_;
    progressBar_->setValue(percent);

    if (percent == 100)
        batchFinished_ = true;
}

void MainWindow::onUploadClicked() {
    prepareNewBatchIfNeeded();

    QStringList files = QFileDialog::getOpenFileNames(
        this, "Select images", "",
        "Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)"
    );

    if (files.isEmpty()) return;

    for (const QString& path : files) {
        QImage img(path);
        if (img.isNull()) continue;

        int index = nextIndex_++;
        totalImages_++;

        auto* item = new ImageItemWidget(scrollContent_);
        item->setImage(img);
        item->setResult("In progress...");
        widgets_[index] = item;

        int row = index / columns_;
        int col = index % columns_;
        gridLayout_->addWidget(item, row, col);

        QByteArray data;
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "PNG");

        client_.sendImage(
            currentBatchId_, index,
            QFileInfo(path).fileName(), data
        );
    }

    updateProgress();
}

void MainWindow::onResult(
    qint64 batchId,
    int index,
    const QString& filename,
    const QString& text,
    bool success,
    const QString& error,
    qint64 ms)
{
    if (batchId != currentBatchId_) return;

    completed_++;
    updateProgress();

    auto* item = widgets_.value(index);
    if (!item) return;

    if (!success)
        item->setResult("Error: " + error);
    else
        item->setResult(text);
}
