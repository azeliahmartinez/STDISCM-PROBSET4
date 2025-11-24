#include "ImageItemWidget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>

ImageItemWidget::ImageItemWidget(QWidget *parent)
    : QWidget(parent),
      imageLabel_(new QLabel(this)),
      filenameLabel_(new QLabel(this)),
      resultLabel_(new QLabel(this))
{
    imageLabel_->setAlignment(Qt::AlignCenter);
    filenameLabel_->setAlignment(Qt::AlignCenter);
    resultLabel_->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(imageLabel_);
    layout->addWidget(filenameLabel_);
    layout->addWidget(resultLabel_);
    setLayout(layout);
}

void ImageItemWidget::setImage(const QImage& img)
{
    imageLabel_->setPixmap(
        QPixmap::fromImage(img).scaled(
            160, 80,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
}

void ImageItemWidget::setFilename(const QString& name)
{
    filenameLabel_->setText(name);
}

void ImageItemWidget::setResultText(const QString& text)
{
    resultLabel_->setText(text);
}
