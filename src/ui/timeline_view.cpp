#include "ui/timeline_view.h"

#include "core/edit_ops.h"

#include <QFileInfo>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>

namespace mcs::ui {

TimelineView::TimelineView(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("timelineView"));
    setMinimumHeight(kRulerHeight + 2 * kTrackHeight);
    setToolTip(tr("Timeline: click a clip to select it, drag to move; click the ruler to move the "
                  "playhead"));
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

void TimelineView::setSelection(const QSet<QUuid>& ids) {
    if (ids == selection_) {
        return;
    }
    selection_ = ids;
    emit selectionChanged();
    update();
}

QSet<QUuid> TimelineView::selection() const {
    return selection_;
}

void TimelineView::setSnapping(bool on) {
    snapping_ = on;
}

bool TimelineView::snapping() const {
    return snapping_;
}

void TimelineView::setLinkedSelection(bool on) {
    linkedSelection_ = on;
}

bool TimelineView::linkedSelection() const {
    return linkedSelection_;
}

const core::Track* TimelineView::trackAtY(int y) const {
    if (!timeline_ || y < kRulerHeight) {
        return nullptr;
    }
    const auto row = static_cast<std::size_t>((y - kRulerHeight) / kTrackHeight);
    if (row < timeline_->video.size()) {
        return &timeline_->video[row];
    }
    const std::size_t audioRow = row - timeline_->video.size();
    return audioRow < timeline_->audio.size() ? &timeline_->audio[audioRow] : nullptr;
}

QUuid TimelineView::clipAt(QPoint pos) const {
    const core::Track* track = trackAtY(pos.y());
    if (!track) {
        return {};
    }
    const auto frame = static_cast<std::int64_t>(std::floor((pos.x() - kHeaderWidth) / ppf_));
    const core::Clip* clip = core::clipAt(*track, frame);
    return clip ? clip->id : QUuid();
}

std::int64_t TimelineView::dragDeltaFor(int x) const {
    const std::int64_t raw = std::llround((x - dragOriginX_) / ppf_);
    if (!snapping_ || !timeline_) {
        return raw;
    }
    const auto threshold = std::max<std::int64_t>(1, std::llround(kSnapPixels / ppf_));
    return core::snapDelta(*timeline_, selection_, raw, playhead_, threshold);
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
            const bool selected = selection_.contains(clip.id);
            const std::int64_t offset = (selected && drag_ == Drag::Clips) ? dragDelta_ : 0;
            const QRect r(xForFrame(clip.start + offset), y + 3,
                          std::max(1, xForFrame(clip.end()) - xForFrame(clip.start)),
                          kTrackHeight - 6);
            p.fillRect(r, selected ? color.lighter(140) : color);
            if (selected) {
                p.setPen(QPen(QColor(0xe0, 0x7a, 0x2e), 2));
                p.drawRect(r.adjusted(1, 1, -1, -1));
            }
            p.setPen(QColor(0x10, 0x10, 0x10));
            p.drawText(r.adjusted(4, 0, -2, 0), Qt::AlignVCenter | Qt::AlignLeft,
                       QFileInfo(clip.path).fileName());
            if (clip.gainDb != 0.0) {
                p.drawText(r.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignRight,
                           tr("%1 dB").arg(clip.gainDb, 0, 'f', 1));
            }
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

    p.setPen(QPen(QColor(0xe0, 0x4a, 0x3a), 2));
    p.drawLine(xForFrame(playhead_), 0, xForFrame(playhead_), height());
}

void TimelineView::moveTo(int x) {
    const std::int64_t frame = frameAtX(x);
    setPlayhead(frame);
    emit playheadMoved(frame);
}

void TimelineView::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const QPoint pos = event->position().toPoint();
    const QUuid id = pos.y() >= kRulerHeight ? clipAt(pos) : QUuid();
    if (id.isNull()) {
        if (pos.y() >= kRulerHeight) {
            setSelection({});
        }
        drag_ = Drag::Playhead;
        moveTo(pos.x());
        return;
    }
    const QSet<QUuid> linked =
        linkedSelection_ ? core::withLinked(*timeline_, {id}) : QSet<QUuid>{id};
    if (event->modifiers() & Qt::ControlModifier) {
        QSet<QUuid> next = selection_;
        if (selection_.contains(id)) {
            next.subtract(linked);
        } else {
            next.unite(linked);
        }
        setSelection(next);
        drag_ = Drag::None;
        return;
    }
    if (!selection_.contains(id)) {
        setSelection(linked);
    }
    drag_ = Drag::Clips;
    dragOriginX_ = pos.x();
    dragDelta_ = 0;
}

void TimelineView::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    const int x = event->position().toPoint().x();
    if (drag_ == Drag::Playhead) {
        moveTo(x);
    } else if (drag_ == Drag::Clips) {
        dragDelta_ = dragDeltaFor(x);
        update();
    }
}

void TimelineView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }
    if (drag_ == Drag::Clips && dragDelta_ != 0) {
        emit clipsMoveRequested(selection_, dragDelta_);
    }
    drag_ = Drag::None;
    dragDelta_ = 0;
    update();
}

} // namespace mcs::ui
