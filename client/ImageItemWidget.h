#pragma once

#include <QWidget>
#include <QImage>

class QLabel;

class ImageItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageItemWidget(QWidget *parent = nullptr);

    void setImage(const QImage &img);
    void setFilename(const QString &name);
    void setResultText(const QString &text);

private:
    QLabel *imageLabel_;
    QLabel *filenameLabel_;
    QLabel *resultLabel_;
};
