#include "ui/main_window.h"

#include "core/project_file.h"
#include "edit/timeline_document.h"
#include "ui/edit_page.h"
#include "ui/media_pool_model.h"

#include <QAction>
#include <QButtonGroup>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QUndoStack>
#include <QVBoxLayout>

namespace mcs::ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
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
    editPage_ = new EditPage(pool_, pages_);
    connect(editPage_, &EditPage::importFailed, this, [this](const QStringList& errors) {
        QMessageBox::warning(this, tr("Import Media"), errors.join(QLatin1Char('\n')));
    });

    addPage(QStringLiteral("edit"), tr("Edit"), editPage_);
    addPage(QStringLiteral("deliver"), tr("Deliver"), new QLabel(tr("Deliver"), pages_));
    barLayout->addStretch();
    createMenus();
    connect(editPage_, &EditPage::editFailed, this,
            [this](const QString& message) { statusBar()->showMessage(message, 6000); });
    connect(editPage_->document()->undoStack(), &QUndoStack::cleanChanged, this,
            &MainWindow::updateTitle);
    autosave_ = new QTimer(this);
    autosave_->setInterval(2 * 60 * 1000);
    connect(autosave_, &QTimer::timeout, this, [this] {
        if (editPage_->document()->isModified()) {
            autosaveNow();
        }
    });
    autosave_->start();
    updateTitle();

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
    auto addAction = [this](QMenu* menu, const QString& name, const QString& text,
                            const QKeySequence& key) {
        QAction* action = menu->addAction(text);
        action->setObjectName(QStringLiteral("action_") + name);
        action->setShortcut(key);
        return action;
    };
    connect(addAction(file, QStringLiteral("newProject"), tr("&New Project"), QKeySequence::New),
            &QAction::triggered, this, [this] {
                if (maybeSave()) {
                    newProject();
                }
            });
    connect(
        addAction(file, QStringLiteral("openProject"), tr("&Open Project..."), QKeySequence::Open),
        &QAction::triggered, this, [this] {
            if (!maybeSave()) {
                return;
            }
            const QString path = QFileDialog::getOpenFileName(
                this, tr("Open Project"), {}, tr("MCS AI Editor project (*.mcsproj)"));
            if (path.isEmpty()) {
                return;
            }
            const QString error = openProject(path);
            if (!error.isEmpty()) {
                QMessageBox::warning(this, tr("Open Project"), error);
            } else if (!warnings_.isEmpty()) {
                QMessageBox::warning(
                    this, tr("Missing media"),
                    tr("These files were not found:\n%1").arg(warnings_.join(QLatin1Char('\n'))));
            }
        });
    connect(addAction(file, QStringLiteral("saveProject"), tr("&Save Project"), QKeySequence::Save),
            &QAction::triggered, this, [this] { saveInteractive(false); });
    connect(addAction(file, QStringLiteral("saveProjectAs"), tr("Save Project &As..."),
                      QKeySequence(QStringLiteral("Ctrl+Shift+S"))),
            &QAction::triggered, this, [this] { saveInteractive(true); });
    file->addSeparator();
    QAction* import = addAction(file, QStringLiteral("importMedia"), tr("&Import Media..."),
                                QKeySequence(QStringLiteral("Ctrl+I")));
    connect(import, &QAction::triggered, this, [this] {
        const QStringList paths = QFileDialog::getOpenFileNames(
            this, tr("Import Media"), {},
            tr("Media (*.mov *.mp4 *.mkv *.avi *.mxf *.webm *.m4v *.wav *.mp3 *.m4a *.aac "
               "*.flac);;All files (*)"));
        importAndReport(paths);
    });

    auto* edit = menuBar()->addMenu(tr("&Edit"));
    QUndoStack* stack = editPage_->document()->undoStack();
    QAction* undo = stack->createUndoAction(this, tr("&Undo"));
    undo->setObjectName(QStringLiteral("action_undo"));
    undo->setShortcut(QKeySequence::Undo);
    QAction* redo = stack->createRedoAction(this, tr("&Redo"));
    redo->setObjectName(QStringLiteral("action_redo"));
    redo->setShortcuts({QKeySequence::Redo, QKeySequence(QStringLiteral("Ctrl+Shift+Z"))});
    edit->addAction(undo);
    edit->addAction(redo);
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

EditPage* MainWindow::editPage() const {
    return editPage_;
}

core::Project MainWindow::currentProject() const {
    return core::Project{pool_->paths(), editPage_->document()->timeline()};
}

QString MainWindow::openProject(const QString& path) {
    const core::ProjectLoad loaded = core::loadProject(path);
    if (!loaded.project) {
        return loaded.error;
    }
    pool_->clear();
    warnings_ = pool_->importFiles(loaded.project->media);
    editPage_->document()->reset(loaded.project->timeline);
    projectPath_ = path;
    updateTitle();
    return {};
}

QString MainWindow::saveProjectTo(const QString& path) {
    QString error;
    if (!core::saveProject(path, currentProject(), &error)) {
        return error;
    }
    projectPath_ = path;
    editPage_->document()->markSaved();
    updateTitle();
    return {};
}

void MainWindow::newProject() {
    pool_->clear();
    editPage_->document()->reset(core::Timeline{});
    projectPath_.clear();
    warnings_.clear();
    updateTitle();
}

QString MainWindow::projectPath() const {
    return projectPath_;
}

QStringList MainWindow::lastWarnings() const {
    return warnings_;
}

QString MainWindow::autosavePath() const {
    if (!projectPath_.isEmpty()) {
        return projectPath_ + QStringLiteral(".autosave");
    }
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                        QStringLiteral("/autosave");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/untitled.mcsproj");
}

QString MainWindow::autosaveNow() {
    QString error;
    core::saveProject(autosavePath(), currentProject(), &error, 0);
    return error;
}

void MainWindow::updateTitle() {
    const QString name =
        projectPath_.isEmpty() ? tr("Untitled") : QFileInfo(projectPath_).completeBaseName();
    const QString dirty = editPage_->document()->isModified() ? QStringLiteral("*") : QString();
    setWindowTitle(tr("%1%2 - MCS AI Editor %3").arg(name, dirty, QStringLiteral(MCS_VERSION)));
}

bool MainWindow::saveInteractive(bool forceDialog) {
    QString path = projectPath_;
    if (forceDialog || path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this, tr("Save Project"), path,
                                            tr("MCS AI Editor project (*.mcsproj)"));
        if (path.isEmpty()) {
            return false;
        }
        if (!path.endsWith(QStringLiteral(".mcsproj"))) {
            path += QStringLiteral(".mcsproj");
        }
    }
    const QString error = saveProjectTo(path);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("Save Project"), error);
        return false;
    }
    return true;
}

bool MainWindow::maybeSave() {
    if (!editPage_->document()->isModified()) {
        return true;
    }
    const auto answer =
        QMessageBox::question(this, tr("Unsaved changes"), tr("Save the changes to this project?"),
                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) {
        return saveInteractive(false);
    }
    return answer == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

} // namespace mcs::ui
