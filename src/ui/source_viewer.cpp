#include "ui/source_viewer.h"

#include "core/rational.h"
#include "media/frame_decoder.h"
#include "ui/frame_view.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <algorithm>

namespace mcs::ui {

SourceViewer::SourceViewer(QWidget* parent) : QWidget(parent) {
    view_ = new FrameView(this);
    slider_ = new QSlider(Qt::Horizontal, this);
    slider_->setObjectName(QStringLiteral("sourceScrubber"));
    slider_->setToolTip(tr("Scrub the source clip (arrow keys step one frame)"));
    timecode_ = new QLabel(this);
    timecode_->setObjectName(QStringLiteral("sourceTimecode"));

    auto* bar = new QHBoxLayout;
    bar->addWidget(slider_, 1);
    bar->addWidget(timecode_);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view_, 1);
    layout->addLayout(bar);

    connect(slider_, &QSlider::valueChanged, this, [this](int v) { showFrame(v); });
    clear();
}

SourceViewer::~SourceViewer() = default;

bool SourceViewer::openMedia(const QString& path) {
    auto decoder = std::make_unique<media::FrameDecoder>();
    if (!decoder->open(path) || decoder->frameCount() <= 0) {
        clear();
        return false;
    }
    decoder_ = std::move(decoder);
    const QSignalBlocker block(slider_);
    slider_->setEnabled(true);
    slider_->setRange(0, static_cast<int>(decoder_->frameCount() - 1));
    slider_->setValue(0);
    showFrame(0);
    return true;
}

void SourceViewer::clear() {
    decoder_.reset();
    position_ = 0;
    const QSignalBlocker block(slider_);
    slider_->setRange(0, 0);
    slider_->setEnabled(false);
    view_->setImage({});
    timecode_->setText(QStringLiteral("--:--:--:--"));
}

std::int64_t SourceViewer::position() const {
    return position_;
}

std::int64_t SourceViewer::frameCount() const {
    return decoder_ ? decoder_->frameCount() : 0;
}

void SourceViewer::setPosition(std::int64_t frame) {
    if (!decoder_) {
        return;
    }
    const std::int64_t clamped = std::clamp<std::int64_t>(frame, 0, decoder_->frameCount() - 1);
    const QSignalBlocker block(slider_);
    slider_->setValue(static_cast<int>(clamped));
    showFrame(clamped);
}

QImage SourceViewer::currentFrame() const {
    return view_->image();
}

QString SourceViewer::timecodeText() const {
    return timecode_->text();
}

void SourceViewer::showFrame(std::int64_t frame) {
    if (!decoder_) {
        return;
    }
    const auto decoded = decoder_->frameAt(frame);
    position_ = frame;
    view_->setImage(decoded ? decoded->image : QImage{});
    timecode_->setText(core::timecode(frame, decoder_->fps()));
}

} // namespace mcs::ui
