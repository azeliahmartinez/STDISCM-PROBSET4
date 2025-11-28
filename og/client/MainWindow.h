#pragma once

#include <QMainWindow>
#include <QVector>
#include <QMap>
#include "OcrClient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct ImageJob {
    qint64 batchId;
    int index;
    QString filename;
    QImage thumbnail;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_uploadButton_clicked();

    // called when gRPC result arrives
    void handleOcrResult(qint64 batchId,
                         int index,
                         const QString& filename,
                         const QString& text,
                         qint64 processingMs,
                         bool success,
                         const QString& error);

private:
    void startNewBatchIfNeeded();
    void updateProgressBar();

    Ui::MainWindow *ui;
    OcrClient ocrClient_;

    qint64 currentBatchId_ = 1;
    int nextImageIndex_ = 0;
    int totalImagesInBatch_ = 0;
    int completedInBatch_ = 0;

    // image index → widget pointer (for updating labels)
    QMap<int, QWidget*> itemWidgets_;
};
