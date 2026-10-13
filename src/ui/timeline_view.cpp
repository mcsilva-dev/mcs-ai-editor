#include "ui/timeline_view.h"

#include <QFileInfo>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>

namespace mcs::ui {

TimelineView::TimelineView(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("timelineView"));
    setMinimumHeight(kRulerHeight + 2 * kTrackHeight);
    setToolTip(tr("Timeline: click or drag to move the playhead"));
}

void TimelineView::setTimeline(const core::Timeline* timeline) {
    timeline_ = timeline;
    update();
}

void TimelineView::setPlayhead(std::int64_t frame) {
    playhead_ = std::max<std::int64_t>(0, frame);
    update();
}

std::int64_t TimelineView::playhead() const {
    return playhead_;
}

void TimelineView::setPixelsPerFrame(double ppf) {
    ppf_ = std::max(0.05, ppf);
    update();
}

double TimelineView::pixelsPerFrame() const {
    return ppf_;
}

std::int64_t TimelineView::frameAtX(int x) const {
    return std::max<std::int64_t>(0, std::llround((x - kHeaderWidth) / ppf_));
}

int TimelineView::xForFrame(std::int64_t frame) const {
    return kHeaderWidth + static_cast<int>(std::lround(static_cast<double>(frame) * ppf_));
}

QSize TimelineView::sizeHint() const {
    return {800, kRulerHeight + 4 * kTrackHeight};
}

void TimelineView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0x18, 0x18, 0x18));

    // Régua: um traço por segundo.
    p.fillRect(QRect(0, 0, width(), kRulerHeight), QColor(0x22, 0x22, 0x22));
    p.setPen(QColor(0x80, 0x80, 0x80));
    const std::int64_t fpsNominal =
        std::max<std::int64_t>(1, std::llround(timeline_ ? timeline_->fps.toDouble() : 25.0));
    for (std::int64_t f = 0; xForFrame(f) < width(); f += fpsNominal) {
        p.drawLine(xForFrame(f), kRulerHeight - 6, xForFrame(f), kRulerHeight);
    }

    // Trilhas: V1.. e depois A1..
    const std::size_t videoTracks = timeline_ ? timeline_->video.size() : 1;
    const std::size_t audioTracks = timeline_ ? timeline_->audio.size() : 1;
    auto drawTrack = [&](int row, const QString& label, const core::Track* track, QColor color) {
        const int y = kRulerHeight + row * kTrackHeight;
        p.setPen(QColor(0x30, 0x30, 0x30));
        p.drawLine(0, y + kTrackHeight - 1, width(), y + kTrackHeight - 1);
        p.setPen(QColor(0xc0, 0xc0, 0xc0));
        p.drawText(QRect(0, y, kHeaderWidth, kTrackHeight), Qt::AlignCenter, label);
        if (!track) {
            return;
        }
        for (const core::Clip& clip : track->clips) {
            const QRect r(xForFrame(clip.start), y + 3,
                          std::max(1, xForFrame(clip.end()) - xForFrame(clip.start)),
                          kTrackHeight - 6);
            p.fillRect(r, color);
            p.setPen(QColor(0x10, 0x10, 0x10));
            p.drawText(r.adjusted(4, 0, -2, 0), Qt::AlignVCenter | Qt::AlignLeft,
                       QFileInfo(clip.path).fileName());
        }
    };
    int row = 0;
    for (std::size_t i = 0; i < videoTracks; ++i) {
        drawTrack(row++, QStringLiteral("V%1").arg(i + 1),
                  timeline_ ? &timeline_->video[i] : nullptr, QColor(0x4a, 0x7d, 0xb8));
    }
    for (std::size_t i = 0; i < audioTracks; ++i) {
        drawTrack(row++, QStringLiteral("A%1").arg(i + 1),
                  timeline_ ? &timeline_->audio[i] : nullptr, QColor(0x4f, 0xa8, 0x6a));
    }

    // Playhead.
    p.setPen(QPen(QColor(0xe0, 0x4a, 0x3a), 2));
    p.drawLine(xForFrame(playhead_), 0, xForFrame(playhead_), height());
}

void TimelineView::moveTo(int x) {
    const std::int64_t frame = frameAtX(x);
    setPlayhead(frame);
    emit playheadMoved(frame);
}

void TimelineView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        moveTo(event->position().toPoint().x());
    }
}

void TimelineView::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        moveTo(event->position().toPoint().x());
    }
}

} // namespace mcs::ui
