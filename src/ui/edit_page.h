#pragma once

#include <QStringList>
#include <QWidget>

class QListView;

namespace mcs::ui {

class MediaPoolModel;
class SourceViewer;

// Página Edit (v0.2): Media Pool à esquerda, viewer de fonte à direita.
class EditPage : public QWidget {
    Q_OBJECT
  public:
    explicit EditPage(MediaPoolModel* pool, QWidget* parent = nullptr);

    QListView* mediaPoolView() const;
    SourceViewer* sourceViewer() const;

  signals:
    void importFailed(const QStringList& errors);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    MediaPoolModel* pool_ = nullptr;
    QListView* list_ = nullptr;
    SourceViewer* viewer_ = nullptr;
};

} // namespace mcs::ui
