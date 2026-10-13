#pragma once

#include <QMainWindow>
#include <QStringList>

class QButtonGroup;
class QStackedWidget;

namespace mcs::ui {

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

  private:
    void addPage(const QString& name, const QString& label, QWidget* page);
    void createMenus();

    MediaPoolModel* pool_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QButtonGroup* pageButtons_ = nullptr;
    QWidget* pageBar_ = nullptr;
    QStringList names_;
};

} // namespace mcs::ui
