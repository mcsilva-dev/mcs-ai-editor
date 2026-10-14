#pragma once

#include <QCloseEvent>
#include <QMainWindow>
#include <QStringList>

class QButtonGroup;
class QStackedWidget;
class QTimer;

namespace mcs::core {
struct Project;
}

namespace mcs::ui {

class EditPage;
class MediaPoolModel;

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(QWidget* parent = nullptr);

    QStringList pageNames() const;
    QString currentPage() const;
    // Troca para a página; nomes desconhecidos são ignorados.
    void showPage(const QString& name);

    MediaPoolModel* mediaPool() const;
    // Importa e mostra um aviso com os erros, se houver.
    void importAndReport(const QStringList& paths);

    EditPage* editPage() const;
    // "" = ok; senão a mensagem de erro (nada muda).
    QString openProject(const QString& path);
    QString saveProjectTo(const QString& path);
    void newProject();
    QString projectPath() const;
    QString autosavePath() const;
    QString autosaveNow();
    // Avisos do último openProject (ex.: mídias que não foram encontradas).
    QStringList lastWarnings() const;

  protected:
    void closeEvent(QCloseEvent* event) override;

  private:
    void addPage(const QString& name, const QString& label, QWidget* page);
    void createMenus();
    bool maybeSave();
    bool saveInteractive(bool forceDialog);
    void updateTitle();
    core::Project currentProject() const;

    EditPage* editPage_ = nullptr;
    QString projectPath_;
    QStringList warnings_;
    QTimer* autosave_ = nullptr;

    MediaPoolModel* pool_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QButtonGroup* pageButtons_ = nullptr;
    QWidget* pageBar_ = nullptr;
    QStringList names_;
};

} // namespace mcs::ui
