#pragma once

#include "core/timeline.h"

#include <QWidget>
#include <cstdint>

namespace mcs::ui {

// Timeline desenhada com QPainter direto do modelo (spec 8.2): régua, trilhas V/A, clipes e
// playhead. Clique/arrasto na área move o playhead. Edição de clipes é o v0.4.
class TimelineView : public QWidget {
    Q_OBJECT
  public:
    static constexpr int kHeaderWidth = 40;
    static constexpr int kRulerHeight = 20;
    static constexpr int kTrackHeight = 40;

    explicit TimelineView(QWidget* parent = nullptr);

    // Não toma posse; repinta. nullptr = timeline vazia.
    void setTimeline(const core::Timeline* timeline);
    void setPlayhead(std::int64_t frame);
    std::int64_t playhead() const;
    void setPixelsPerFrame(double ppf);
    double pixelsPerFrame() const;
    std::int64_t frameAtX(int x) const;
    int xForFrame(std::int64_t frame) const;
    QSize sizeHint() const override;

  signals:
    void playheadMoved(std::int64_t frame);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

  private:
    void moveTo(int x);

    const core::Timeline* timeline_ = nullptr;
    std::int64_t playhead_ = 0;
    double ppf_ = 2.0;
};

} // namespace mcs::ui
