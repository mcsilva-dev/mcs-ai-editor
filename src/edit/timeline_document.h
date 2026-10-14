#pragma once

#include "core/edit_ops.h"
#include "core/timeline.h"

#include <QObject>
#include <QString>
#include <QUndoStack>

namespace mcs::edit {

class SnapshotCommand;

// Timeline editável. TODA mudança entra por apply() como um passo desfazível (snapshot antes/
// depois). É o ponto único de edição: UI, atalhos e, no futuro, o servidor MCP (Subprojeto 7).
class TimelineDocument : public QObject {
    Q_OBJECT
  public:
    explicit TimelineDocument(QObject* parent = nullptr);
    ~TimelineDocument() override;

    const core::Timeline& timeline() const;
    // "" = aplicado; senão a mensagem de erro (nada muda, histórico intacto).
    QString apply(const QString& actionName, const core::EditResult& result);
    // Novo/abrir projeto: troca a timeline, limpa o histórico e marca como salvo.
    void reset(const core::Timeline& timeline);
    QUndoStack* undoStack();
    bool isModified() const;
    void markSaved();

  signals:
    void timelineChanged();

  private:
    friend class SnapshotCommand;
    void setTimeline(const core::Timeline& timeline);

    core::Timeline timeline_;
    QUndoStack stack_;
};

} // namespace mcs::edit
