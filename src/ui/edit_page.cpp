#include "ui/edit_page.h"

#include "core/rational.h"
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

namespace mcs::ui {

EditPage::EditPage(MediaPoolModel* pool, QWidget* parent) : QWidget(parent), pool_(pool) {
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
    timelineView_->setTimeline(&timeline_);

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

    auto* append = new QAction(tr("Append to Timeline"), this);
    append->setObjectName(QStringLiteral("action_appendToTimeline"));
    append->setShortcut(QKeySequence(QStringLiteral("Shift+F12")));
    append->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(append, &QAction::triggered, this, [this] { appendToTimeline(list_->currentIndex()); });
    addAction(append);

    auto* playPause = new QAction(tr("Play/Pause"), this);
    playPause->setObjectName(QStringLiteral("action_playPause"));
    playPause->setShortcut(QKeySequence(Qt::Key_Space));
    playPause->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(playPause, &QAction::triggered, this, &EditPage::togglePlayback);
    addAction(playPause);
}

QListView* EditPage::mediaPoolView() const {
    return list_;
}

SourceViewer* EditPage::sourceViewer() const {
    return viewer_;
}

void EditPage::appendToTimeline(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }
    if (core::appendMedia(timeline_, pool_->item(index.row()).info) < 0) {
        return;
    }
    player_->setTimeline(timeline_);
    programVideo_.setTimeline(timeline_);
    timelineView_->update();
    showProgramFrame(player_->currentFrame());
}

void EditPage::togglePlayback() {
    if (player_->isPlaying()) {
        player_->pause();
        return;
    }
    if (timeline_.duration() == 0) {
        return;
    }
    if (player_->currentFrame() >= timeline_.duration()) {
        player_->seek(0);
    }
    player_->play();
}

const core::Timeline& EditPage::timeline() const {
    return timeline_;
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

void EditPage::showProgramFrame(std::int64_t frame) {
    timelineView_->setPlayhead(frame);
    program_->setImage(programVideo_.frameAt(frame));
    programTimecode_->setText(core::timecode(frame, timeline_.fps));
}

void EditPage::refreshPlayback() {
    std::int64_t frame = player_->currentFrame();
    if (frame >= timeline_.duration()) {
        player_->pause();
        frame = std::max<std::int64_t>(0, timeline_.duration() - 1);
    }
    showProgramFrame(frame);
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
