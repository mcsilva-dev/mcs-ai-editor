#pragma once

#include "core/timeline.h"

#include <QSet>
#include <QUuid>
#include <QWidget>
#include <cstdint>

namespace mcs::ui {

// Timeline desenhada com QPainter direto do modelo (spec 8.2). Régua/área vazia: playhead.
// Clipe: seleciona com os ligados (Ctrl alterna) e arrasta (ímã opcional). A view só PEDE a
// mudança (clipsMoveRequested); quem edita é o TimelineDocument.
class TimelineView : public QWidget {
    Q_OBJECT
  public:
    static constexpr int kHeaderWidth = 40;
    static constexpr int kRulerHeight = 20;
    static constexpr int kTrackHeight = 40;
    static constexpr int kSnapPixels = 8;

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

    void setSelection(const QSet<QUuid>& ids);
    QSet<QUuid> selection() const;
    void setSnapping(bool on);
    bool snapping() const;
    // Ligada: clicar seleciona também os clipes ligados (vídeo + áudio). Desligada: só o clicado.
    void setLinkedSelection(bool on);
    bool linkedSelection() const;
    QUuid clipAt(QPoint pos) const;

  signals:
    void playheadMoved(std::int64_t frame);
    void selectionChanged();
    void clipsMoveRequested(const QSet<QUuid>& ids, std::int64_t delta);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private:
    enum class Drag { None, Playhead, Clips };

    void moveTo(int x);
    std::int64_t dragDeltaFor(int x) const;
    const core::Track* trackAtY(int y) const;

    const core::Timeline* timeline_ = nullptr;
    std::int64_t playhead_ = 0;
    double ppf_ = 2.0;
    QSet<QUuid> selection_;
    bool snapping_ = true;
    bool linkedSelection_ = true;
    Drag drag_ = Drag::None;
    int dragOriginX_ = 0;
    std::int64_t dragDelta_ = 0;
};

} // namespace mcs::ui
