#pragma once

#include <QMainWindow>
#include <QStringList>

class QButtonGroup;
class QStackedWidget;

namespace mcs::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(QWidget* parent = nullptr);

    QStringList pageNames() const;
    QString currentPage() const;
    // Troca para a página; nomes desconhecidos são ignorados.
    void showPage(const QString& name);

  private:
    void addPage(const QString& name, const QString& label);

    QStackedWidget* pages_ = nullptr;
    QButtonGroup* pageButtons_ = nullptr;
    QWidget* pageBar_ = nullptr;
    QStringList names_;
};

} // namespace mcs::ui
