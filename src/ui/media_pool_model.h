#pragma once

#include "core/media_info.h"

#include <QAbstractListModel>
#include <QImage>
#include <QList>
#include <QSize>
#include <QStringList>
#include <QUuid>

namespace mcs::ui {

inline constexpr QSize kThumbnailSize{160, 90};

// Lista de mídias importadas no projeto (Media Pool).
class MediaPoolModel : public QAbstractListModel {
    Q_OBJECT
  public:
    enum Role { PathRole = Qt::UserRole + 1 };

    struct Item {
        QUuid id;
        core::MediaInfo info;
        QImage thumbnail;
    };

    explicit MediaPoolModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    // Importa na ordem dada. Retorna as mensagens de erro (vazio = tudo ok).
    // Arquivos já presentes são ignorados sem erro.
    QStringList importFiles(const QStringList& paths);
    const Item& item(int row) const;

  signals:
    // Emitido quando o thumbnail da linha termina (também quando não há vídeo).
    void thumbnailReady(int row);

  private:
    bool contains(const QString& path) const;
    void requestThumbnail(const QUuid& id, const QString& path);

    QList<Item> items_;
};

} // namespace mcs::ui
