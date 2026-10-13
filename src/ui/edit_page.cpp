#include "ui/edit_page.h"

#include "ui/media_pool_model.h"
#include "ui/source_viewer.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QListView>
#include <QMimeData>
#include <QSplitter>
#include <QUrl>

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

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(list_);
    splitter->addWidget(viewer_);
    splitter->setStretchFactor(1, 3);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);

    connect(list_->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current) {
                if (!current.isValid()) {
                    viewer_->clear();
                    return;
                }
                viewer_->openMedia(current.data(MediaPoolModel::PathRole).toString());
            });
}

QListView* EditPage::mediaPoolView() const {
    return list_;
}

SourceViewer* EditPage::sourceViewer() const {
    return viewer_;
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
