#include "ui/media_pool_model.h"

#include "core/rational.h"
#include "media/frame_decoder.h"
#include "media/probe.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

namespace mcs::ui {

MediaPoolModel::MediaPoolModel(QObject* parent) : QAbstractListModel(parent) {}

int MediaPoolModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant MediaPoolModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return {};
    }
    const Item& it = items_.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return QFileInfo(it.info.path).fileName();
    case Qt::DecorationRole:
        return it.thumbnail;
    case PathRole:
        return it.info.path;
    case Qt::ToolTipRole: {
        QStringList parts;
        if (it.info.video) {
            const auto& v = *it.info.video;
            parts << QStringLiteral("%1x%2").arg(v.width).arg(v.height)
                  << tr("%1 fps").arg(v.fps.toDouble(), 0, 'f', 3)
                  << core::timecode(v.frameCount, v.fps) << v.codec;
        }
        if (it.info.audio) {
            const auto& a = *it.info.audio;
            parts << tr("%1 Hz, %2 ch").arg(a.sampleRate).arg(a.channels) << a.codec;
        }
        return parts.join(QStringLiteral(" · "));
    }
    default:
        return {};
    }
}

bool MediaPoolModel::contains(const QString& path) const {
    const QString key = QFileInfo(path).absoluteFilePath();
    for (const Item& it : items_) {
        if (QFileInfo(it.info.path).absoluteFilePath().compare(key, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

QStringList MediaPoolModel::importFiles(const QStringList& paths) {
    QStringList errors;
    for (const QString& path : paths) {
        if (contains(path)) {
            continue;
        }
        const media::ProbeResult r = media::probe(path);
        if (!r.info) {
            errors << r.error;
            continue;
        }
        const int row = static_cast<int>(items_.size());
        beginInsertRows({}, row, row);
        items_.append(Item{QUuid::createUuid(), *r.info, {}});
        endInsertRows();
        requestThumbnail(items_.last().id, path);
    }
    return errors;
}

const MediaPoolModel::Item& MediaPoolModel::item(int row) const {
    return items_.at(row);
}

void MediaPoolModel::requestThumbnail(const QUuid& id, const QString& path) {
    auto* watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, id] {
        const QImage thumb = watcher->result();
        watcher->deleteLater();
        for (int row = 0; row < items_.size(); ++row) {
            if (items_[row].id == id) {
                items_[row].thumbnail = thumb;
                const QModelIndex idx = index(row);
                emit dataChanged(idx, idx, {Qt::DecorationRole});
                emit thumbnailReady(row);
                return;
            }
        }
    });
    watcher->setFuture(QtConcurrent::run([path]() -> QImage {
        media::FrameDecoder decoder;
        if (!decoder.open(path)) {
            return {};
        }
        const auto frame = decoder.frameAt(0);
        return frame ? frame->image.scaled(kThumbnailSize, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation)
                     : QImage{};
    }));
}

} // namespace mcs::ui
