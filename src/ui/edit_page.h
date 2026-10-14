#pragma once

#include "core/edit_ops.h"
#include "core/timeline.h"
#include "playback/timeline_video.h"

#include <QImage>
#include <QStringList>
#include <QWidget>
#include <cstdint>
#include <optional>

class QAction;
class QKeySequence;
class QLabel;
class QListView;
class QModelIndex;
class QTimer;

namespace mcs::edit {
class TimelineDocument;
}

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
    edit::TimelineDocument* document() const;
    void bladeAtPlayhead();
    void liftSelection();
    void rippleDeleteSelection();
    void insertFromSource();
    void overwriteFromSource();
    void adjustSelectedGain(double deltaDb);

  signals:
    void importFailed(const QStringList& errors);
    void editFailed(const QString& message);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    void showProgramFrame(std::int64_t frame);
    void refreshPlayback();
    void performEdit(const QString& name, const core::EditResult& result);
    std::optional<core::SourceRange> sourceRange(QString* error) const;
    void onTimelineChanged();
    QAction* addShortcut(const QString& name, const QString& text, const QKeySequence& key);

    MediaPoolModel* pool_ = nullptr;
    QListView* list_ = nullptr;
    SourceViewer* viewer_ = nullptr;
    edit::TimelineDocument* document_ = nullptr;
    playback::TimelineVideo programVideo_;
    playback::Player* player_ = nullptr;
    TimelineView* timelineView_ = nullptr;
    FrameView* program_ = nullptr;
    QLabel* programTimecode_ = nullptr;
    QTimer* refresh_ = nullptr;
};

} // namespace mcs::ui
