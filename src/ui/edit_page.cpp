#include "ui/edit_page.h"

#include "core/rational.h"
#include "edit/timeline_document.h"
#include "playback/player.h"
#include "ui/frame_view.h"
#include "ui/media_pool_model.h"
#include "ui/source_viewer.h"
#include "ui/timeline_view.h"

#include <QAction>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QMimeData>
#include <QSplitter>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace mcs::ui {

EditPage::EditPage(MediaPoolModel* pool, QWidget* parent) : QWidget(parent), pool_(pool) {
    document_ = new edit::TimelineDocument(this);
    list_ = new QListView(this);
    list_->setObjectName(QStringLiteral("mediaPoolView"));
    list_->setModel(pool_);
    list_->setIconSize(QSize(80, 45));
    list_->setToolTip(tr("Media Pool: drop files here or use File > Import Media (Ctrl+I)"));
    list_->viewport()->setAcceptDrops(true);
    list_->viewport()->installEventFilter(this);

    viewer_ = new SourceViewer(this);

    program_ = new FrameView(this);
    program_->setObjectName(QStringLiteral("programViewer"));
    programTimecode_ = new QLabel(QStringLiteral("00:00:00:00"), this);
    programTimecode_->setObjectName(QStringLiteral("programTimecode"));
    auto* programBox = new QWidget(this);
    auto* programLayout = new QVBoxLayout(programBox);
    programLayout->setContentsMargins(0, 0, 0, 0);
    programLayout->addWidget(program_, 1);
    programLayout->addWidget(programTimecode_, 0, Qt::AlignRight);

    auto* top = new QSplitter(Qt::Horizontal, this);
    top->addWidget(list_);
    top->addWidget(viewer_);
    top->addWidget(programBox);
    top->setStretchFactor(1, 2);
    top->setStretchFactor(2, 2);

    timelineView_ = new TimelineView(this);
    timelineView_->setTimeline(&document_->timeline());

    auto* vertical = new QSplitter(Qt::Vertical, this);
    vertical->addWidget(top);
    vertical->addWidget(timelineView_);
    vertical->setStretchFactor(0, 3);
    vertical->setStretchFactor(1, 1);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(vertical);

    connect(list_->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current) {
                if (!current.isValid()) {
                    viewer_->clear();
                    return;
                }
                viewer_->openMedia(current.data(MediaPoolModel::PathRole).toString());
            });

    player_ = new playback::Player(this);
    connect(document_, &edit::TimelineDocument::timelineChanged, this,
            &EditPage::onTimelineChanged);
    connect(timelineView_, &TimelineView::clipsMoveRequested, this,
            [this](const QSet<QUuid>& ids, std::int64_t delta) {
                performEdit(tr("Move"), core::moveClips(document_->timeline(), ids, delta));
            });
    refresh_ = new QTimer(this);
    refresh_->setInterval(15);
    connect(refresh_, &QTimer::timeout, this, &EditPage::refreshPlayback);
    connect(player_, &playback::Player::playingChanged, this,
            [this](bool playing) { playing ? refresh_->start() : refresh_->stop(); });
    connect(timelineView_, &TimelineView::playheadMoved, this, [this](std::int64_t frame) {
        player_->seek(frame);
        showProgramFrame(frame);
    });
    connect(list_, &QListView::doubleClicked, this, &EditPage::appendToTimeline);

    connect(addShortcut(QStringLiteral("appendToTimeline"), tr("Append to Timeline"),
                        QKeySequence(QStringLiteral("Shift+F12"))),
            &QAction::triggered, this, [this] { appendToTimeline(list_->currentIndex()); });
    connect(addShortcut(QStringLiteral("playPause"), tr("Play/Pause"), QKeySequence(Qt::Key_Space)),
            &QAction::triggered, this, &EditPage::togglePlayback);
    connect(addShortcut(QStringLiteral("blade"), tr("Blade at Playhead"), QKeySequence(Qt::Key_B)),
            &QAction::triggered, this, &EditPage::bladeAtPlayhead);
    connect(addShortcut(QStringLiteral("delete"), tr("Delete"), QKeySequence(Qt::Key_Delete)),
            &QAction::triggered, this, &EditPage::liftSelection);
    connect(addShortcut(QStringLiteral("rippleDelete"), tr("Ripple Delete"),
                        QKeySequence(QStringLiteral("Shift+Del"))),
            &QAction::triggered, this, &EditPage::rippleDeleteSelection);
    connect(addShortcut(QStringLiteral("markIn"), tr("Mark In"), QKeySequence(Qt::Key_I)),
            &QAction::triggered, viewer_, &SourceViewer::markIn);
    connect(addShortcut(QStringLiteral("markOut"), tr("Mark Out"), QKeySequence(Qt::Key_O)),
            &QAction::triggered, viewer_, &SourceViewer::markOut);
    connect(addShortcut(QStringLiteral("insert"), tr("Insert"), QKeySequence(Qt::Key_F9)),
            &QAction::triggered, this, &EditPage::insertFromSource);
    connect(addShortcut(QStringLiteral("overwrite"), tr("Overwrite"), QKeySequence(Qt::Key_F10)),
            &QAction::triggered, this, &EditPage::overwriteFromSource);
    QAction* snap =
        addShortcut(QStringLiteral("snapping"), tr("Snapping"), QKeySequence(Qt::Key_N));
    snap->setCheckable(true);
    snap->setChecked(timelineView_->snapping());
    connect(snap, &QAction::toggled, timelineView_, &TimelineView::setSnapping);
    QAction* linked = addShortcut(QStringLiteral("linkedSelection"), tr("Linked Selection"),
                                  QKeySequence(QStringLiteral("Ctrl+Shift+L")));
    linked->setCheckable(true);
    linked->setChecked(timelineView_->linkedSelection());
    connect(linked, &QAction::toggled, timelineView_, &TimelineView::setLinkedSelection);
    connect(addShortcut(QStringLiteral("gainUp"), tr("Increase Clip Volume"),
                        QKeySequence(QStringLiteral("Ctrl+Alt+="))),
            &QAction::triggered, this, [this] { adjustSelectedGain(+1.0); });
    connect(addShortcut(QStringLiteral("gainDown"), tr("Decrease Clip Volume"),
                        QKeySequence(QStringLiteral("Ctrl+Alt+-"))),
            &QAction::triggered, this, [this] { adjustSelectedGain(-1.0); });
}

QListView* EditPage::mediaPoolView() const {
    return list_;
}

SourceViewer* EditPage::sourceViewer() const {
    return viewer_;
}

QAction* EditPage::addShortcut(const QString& name, const QString& text, const QKeySequence& key) {
    auto* action = new QAction(text, this);
    action->setObjectName(QStringLiteral("action_") + name);
    action->setShortcut(key);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(action);
    return action;
}

edit::TimelineDocument* EditPage::document() const {
    return document_;
}

void EditPage::performEdit(const QString& name, const core::EditResult& result) {
    const QString error = document_->apply(name, result);
    if (!error.isEmpty()) {
        emit editFailed(error);
    }
}

void EditPage::onTimelineChanged() {
    const core::Timeline& t = document_->timeline();
    player_->setTimeline(t);
    programVideo_.setTimeline(t);
    QSet<QUuid> alive;
    for (const QUuid& id : timelineView_->selection()) {
        if (core::findClip(t, id)) {
            alive.insert(id);
        }
    }
    timelineView_->setSelection(alive);
    timelineView_->update();
    showProgramFrame(player_->currentFrame());
}

void EditPage::appendToTimeline(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }
    core::Timeline next = document_->timeline();
    if (core::appendMedia(next, pool_->item(index.row()).info) < 0) {
        return;
    }
    performEdit(tr("Append to Timeline"), core::EditResult{next, {}});
}

void EditPage::bladeAtPlayhead() {
    performEdit(tr("Blade"), core::blade(document_->timeline(), player_->currentFrame()));
}

void EditPage::liftSelection() {
    performEdit(tr("Delete"), core::lift(document_->timeline(), timelineView_->selection()));
}

void EditPage::adjustSelectedGain(double deltaDb) {
    performEdit(tr("Clip Volume"),
                core::adjustGain(document_->timeline(), timelineView_->selection(), deltaDb));
}

void EditPage::rippleDeleteSelection() {
    performEdit(tr("Ripple Delete"),
                core::rippleDelete(document_->timeline(), timelineView_->selection()));
}

std::optional<core::SourceRange> EditPage::sourceRange(QString* error) const {
    const QModelIndex index = list_->currentIndex();
    if (!index.isValid()) {
        *error = tr("Select a clip in the Media Pool first.");
        return std::nullopt;
    }
    const core::MediaInfo& info = pool_->item(index.row()).info;
    core::SourceRange r;
    r.path = info.path;
    r.video = info.video.has_value();
    r.audio = info.audio.has_value();
    if (info.video && info.video->fps.num > 0) {
        r.sourceFps = info.video->fps;
        r.in = viewer_->inPoint().value_or(0);
        r.out = viewer_->outPoint() ? *viewer_->outPoint() + 1 : info.video->frameCount;
    } else {
        r.sourceFps = document_->timeline().fps;
        r.out = std::llround(info.durationSeconds * r.sourceFps.toDouble());
    }
    return r;
}

void EditPage::insertFromSource() {
    QString error;
    const auto range = sourceRange(&error);
    if (!range) {
        emit editFailed(error);
        return;
    }
    performEdit(tr("Insert"), core::insert(document_->timeline(), *range, player_->currentFrame()));
}

void EditPage::overwriteFromSource() {
    QString error;
    const auto range = sourceRange(&error);
    if (!range) {
        emit editFailed(error);
        return;
    }
    performEdit(tr("Overwrite"),
                core::overwrite(document_->timeline(), *range, player_->currentFrame()));
}

void EditPage::togglePlayback() {
    if (player_->isPlaying()) {
        player_->pause();
        return;
    }
    const std::int64_t duration = document_->timeline().duration();
    if (duration == 0) {
        return;
    }
    if (player_->currentFrame() >= duration) {
        player_->seek(0);
    }
    player_->play();
}

const core::Timeline& EditPage::timeline() const {
    return document_->timeline();
}

void EditPage::showProgramFrame(std::int64_t frame) {
    timelineView_->setPlayhead(frame);
    program_->setImage(programVideo_.frameAt(frame));
    programTimecode_->setText(core::timecode(frame, document_->timeline().fps));
}

void EditPage::refreshPlayback() {
    std::int64_t frame = player_->currentFrame();
    const std::int64_t duration = document_->timeline().duration();
    if (frame >= duration) {
        player_->pause();
        frame = std::max<std::int64_t>(0, duration - 1);
    }
    showProgramFrame(frame);
}

playback::Player* EditPage::player() const {
    return player_;
}

TimelineView* EditPage::timelineView() const {
    return timelineView_;
}

QImage EditPage::programFrame() const {
    return program_->image();
}

bool EditPage::eventFilter(QObject* watched, QEvent* event) {
    if (watched == list_->viewport()) {
        if (event->type() == QEvent::DragEnter) {
            auto* e = static_cast<QDragEnterEvent*>(event);
            if (e->mimeData()->hasUrls()) {
                e->acceptProposedAction();
                return true;
            }
        } else if (event->type() == QEvent::Drop) {
            auto* e = static_cast<QDropEvent*>(event);
            QStringList paths;
            for (const QUrl& url : e->mimeData()->urls()) {
                if (url.isLocalFile()) {
                    paths << url.toLocalFile();
                }
            }
            const QStringList errors = pool_->importFiles(paths);
            if (!errors.isEmpty()) {
                emit importFailed(errors);
            }
            e->acceptProposedAction();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace mcs::ui
