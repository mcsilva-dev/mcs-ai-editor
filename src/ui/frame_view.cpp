#include "ui/frame_view.h"

#include <QPainter>

namespace mcs::ui {

FrameView::FrameView(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 180);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void FrameView::setImage(const QImage& image) {
    image_ = image;
    update();
}

QImage FrameView::image() const {
    return image_;
}

void FrameView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    if (image_.isNull()) {
        return;
    }
    const QSize fitted = image_.size().scaled(size(), Qt::KeepAspectRatio);
    const QRect target(QPoint((width() - fitted.width()) / 2, (height() - fitted.height()) / 2),
                       fitted);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(target, image_);
}

} // namespace mcs::ui
