#pragma once

#include "core/timeline.h"
#include "playback/timeline_video.h"

#include <QImage>
#include <QStringList>
#include <QWidget>
#include <cstdint>

class QLabel;
class QListView;
class QModelIndex;
class QTimer;

namespace mcs::playback {
class Player;
}

namespace mcs::ui {

class FrameView;
class MediaPoolModel;
class SourceViewer;
class TimelineView;

// Página Edit: Media Pool, viewer de fonte e viewer de programa em cima; timeline embaixo.
class EditPage : public QWidget {
    Q_OBJECT
  public:
    explicit EditPage(MediaPoolModel* pool, QWidget* parent = nullptr);

    QListView* mediaPoolView() const;
    SourceViewer* sourceViewer() const;

    // Acrescenta a mídia da linha no fim da timeline (V1 + A1). Duplo clique / Shift+F12.
    void appendToTimeline(const QModelIndex& index);
    // Espaço: play/pause; no fim da timeline recomeça do início.
    void togglePlayback();
    const core::Timeline& timeline() const;
    playback::Player* player() const;
    TimelineView* timelineView() const;
    QImage programFrame() const;

  signals:
    void importFailed(const QStringList& errors);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    void showProgramFrame(std::int64_t frame);
    void refreshPlayback();

    MediaPoolModel* pool_ = nullptr;
    QListView* list_ = nullptr;
    SourceViewer* viewer_ = nullptr;
    core::Timeline timeline_;
    playback::TimelineVideo programVideo_;
    playback::Player* player_ = nullptr;
    TimelineView* timelineView_ = nullptr;
    FrameView* program_ = nullptr;
    QLabel* programTimecode_ = nullptr;
    QTimer* refresh_ = nullptr;
};

} // namespace mcs::ui
