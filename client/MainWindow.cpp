#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QImageReader>
#include <QBuffer>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDebug>

#include "ImageItemWidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // progress bar setup
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(100);
    ui->progressBar->setValue(0);

    // connect OCR client signal
    connect(&ocrClient_, &OcrClient::ocrResultReady,
            this, &MainWindow::handleOcrResult);

    // connect upload button
    connect(ui->uploadButton, &QPushButton::clicked,
            this, &MainWindow::on_uploadButton_clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startNewBatchIfNeeded()
{
    if (totalImagesInBatch_ > 0 && completedInBatch_ == totalImagesInBatch_) {
        currentBatchId_++;
        nextImageIndex_ = 0;
        totalImagesInBatch_ = 0;
        completedInBatch_ = 0;
        itemWidgets_.clear();

        QWidget *container = ui->scrollAreaWidgetContents;
        if (auto *layout = container->layout()) {
            QLayoutItem *item;
            while ((item = layout->takeAt(0)) != nullptr) {
                delete item->widget();
                delete item;
            }
        }
        ui->progressBar->setValue(0);
    }
}

void MainWindow::updateProgressBar()
{
    if (totalImagesInBatch_ == 0) {
        ui->progressBar->setValue(0);
        return;
    }
    int percent = static_cast<int>(100.0 * completedInBatch_ / totalImagesInBatch_);
    ui->progressBar->setValue(percent);
}

void MainWindow::on_uploadButton_clicked()
{
    startNewBatchIfNeeded();

    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Select images"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)")
    );

    if (files.isEmpty())
        return;

    QWidget *container = ui->scrollAreaWidgetContents;
    QLayout *layout = container->layout();
    if (!layout) {
        layout = new QVBoxLayout(container);
        container->setLayout(layout);
    }

    for (const QString &path : files) {
        QImageReader reader(path);
        QImage image = reader.read();
        if (image.isNull())
            continue;

        int idx = nextImageIndex_++;
        totalImagesInBatch_++;

        auto *item = new ImageItemWidget(container);
        item->setImage(image);
        item->setFilename(QFileInfo(path).fileName());
        item->setResultText("In progress");
        layout->addWidget(item);
        itemWidgets_.insert(idx, item);

        // convert QImage to PNG bytes
        QByteArray data;
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");

        ocrClient_.sendImage(
            currentBatchId_,
            idx,
            QFileInfo(path).fileName(),
            data
        );
    }

    updateProgressBar();
}

void MainWindow::handleOcrResult(qint64 batchId,
                                 int index,
                                 const QString& filename,
                                 const QString& text,
                                 qint64 processingMs,
                                 bool success,
                                 const QString& error)
{
    Q_UNUSED(filename)
    Q_UNUSED(processingMs)

    if (batchId != currentBatchId_)
        return; // old batch, ignore

    completedInBatch_++;
    updateProgressBar();

    auto *w = itemWidgets_.value(index, nullptr);
    auto *item = qobject_cast<ImageItemWidget*>(w);
    if (!item)
        return;

    if (!success) {
        item->setResultText(QString("Error: %1").arg(error));
    } else {
        item->setResultText(text);
    }
}
