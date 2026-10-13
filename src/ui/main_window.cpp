#include "ui/main_window.h"

#include "ui/edit_page.h"
#include "ui/media_pool_model.h"

#include <QAction>
#include <QButtonGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace mcs::ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("MCS AI Editor %1").arg(QStringLiteral(MCS_VERSION)));
    setMinimumSize(1280, 720);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    pages_ = new QStackedWidget(central);
    pageBar_ = new QWidget(central);
    // Folha de estilo só na barra: no QApplication ela embrulharia o estilo de todo o app.
    pageBar_->setStyleSheet(QStringLiteral("QToolButton { padding: 6px 18px; border: none; }"
                                           "QToolButton:checked { color: #e07a2e; }"));
    auto* barLayout = new QHBoxLayout(pageBar_);
    barLayout->setContentsMargins(0, 4, 0, 4);
    barLayout->addStretch();
    pageButtons_ = new QButtonGroup(this);
    pageButtons_->setExclusive(true);

    layout->addWidget(pages_, 1);
    layout->addWidget(pageBar_);
    setCentralWidget(central);

    pool_ = new MediaPoolModel(this);
    auto* editPage = new EditPage(pool_, pages_);
    connect(editPage, &EditPage::importFailed, this, [this](const QStringList& errors) {
        QMessageBox::warning(this, tr("Import Media"), errors.join(QLatin1Char('\n')));
    });

    addPage(QStringLiteral("edit"), tr("Edit"), editPage);
    addPage(QStringLiteral("deliver"), tr("Deliver"), new QLabel(tr("Deliver"), pages_));
    barLayout->addStretch();
    createMenus();

    showPage(QStringLiteral("edit"));
}

void MainWindow::addPage(const QString& name, const QString& label, QWidget* page) {
    page->setObjectName(QStringLiteral("page_") + name);
    if (auto* text = qobject_cast<QLabel*>(page)) {
        text->setAlignment(Qt::AlignCenter);
    }
    pages_->addWidget(page);

    auto* button = new QToolButton(pageBar_);
    button->setObjectName(QStringLiteral("pageButton_") + name);
    button->setText(label);
    button->setCheckable(true);
    button->setToolTip(tr("Open the %1 page").arg(label));
    pageButtons_->addButton(button, static_cast<int>(names_.size()));
    pageBar_->layout()->addWidget(button);
    connect(button, &QToolButton::clicked, this, [this, name] { showPage(name); });

    names_.append(name);
}

QStringList MainWindow::pageNames() const {
    return names_;
}

QString MainWindow::currentPage() const {
    return names_.value(pages_->currentIndex());
}

void MainWindow::showPage(const QString& name) {
    const auto index = names_.indexOf(name);
    if (index < 0) {
        return;
    }
    pages_->setCurrentIndex(static_cast<int>(index));
    pageButtons_->button(static_cast<int>(index))->setChecked(true);
}

void MainWindow::createMenus() {
    auto* file = menuBar()->addMenu(tr("&File"));
    auto* import = file->addAction(tr("&Import Media..."));
    import->setObjectName(QStringLiteral("action_importMedia"));
    import->setShortcut(QKeySequence(QStringLiteral("Ctrl+I")));
    connect(import, &QAction::triggered, this, [this] {
        const QStringList paths = QFileDialog::getOpenFileNames(
            this, tr("Import Media"), {},
            tr("Media (*.mov *.mp4 *.mkv *.avi *.mxf *.webm *.m4v *.wav *.mp3 *.m4a *.aac "
               "*.flac);;All files (*)"));
        importAndReport(paths);
    });
}

MediaPoolModel* MainWindow::mediaPool() const {
    return pool_;
}

void MainWindow::importAndReport(const QStringList& paths) {
    const QStringList errors = pool_->importFiles(paths);
    if (!errors.isEmpty()) {
        QMessageBox::warning(this, tr("Import Media"), errors.join(QLatin1Char('\n')));
    }
}

} // namespace mcs::ui
