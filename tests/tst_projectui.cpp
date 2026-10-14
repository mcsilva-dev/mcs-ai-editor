#include "core/project_file.h"
#include "edit/timeline_document.h"
#include "support/synth_media.h"
#include "support/timeline_text.h"
#include "ui/edit_page.h"
#include "ui/main_window.h"
#include "ui/media_pool_model.h"

#include <QAction>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest>

using mcs::test::formatTimeline;

class TstProjectUi : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir dir_;
    QString video_;

  private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(dir_.isValid());
        video_ = dir_.filePath(QStringLiteral("clip.mov"));
        mcs::test::SynthSpec s;
        s.frames = 30;
        QCOMPARE(mcs::test::writeSynthMov(video_, s), QString());
    }

    void saveAndReopenRestoresProject() {
        const QString path = dir_.filePath(QStringLiteral("p1.mcsproj"));
        QString before;
        {
            mcs::ui::MainWindow w;
            w.importAndReport({video_});
            w.editPage()->appendToTimeline(w.mediaPool()->index(0));
            before = formatTimeline(w.editPage()->timeline());
            QCOMPARE(w.saveProjectTo(path), QString());
            QCOMPARE(w.projectPath(), path);
        }
        mcs::ui::MainWindow w2;
        QCOMPARE(w2.openProject(path), QString());
        QCOMPARE(w2.mediaPool()->rowCount(), 1);
        QCOMPARE(formatTimeline(w2.editPage()->timeline()), before);
        QVERIFY(w2.lastWarnings().isEmpty());
        QVERIFY(!w2.editPage()->document()->isModified());
    }

    void titleMarksUnsavedChanges() {
        mcs::ui::MainWindow w;
        w.importAndReport({video_});
        w.editPage()->appendToTimeline(w.mediaPool()->index(0));
        QVERIFY(w.windowTitle().contains(QLatin1Char('*')));
        QCOMPARE(w.saveProjectTo(dir_.filePath(QStringLiteral("p2.mcsproj"))), QString());
        QVERIFY(!w.windowTitle().contains(QLatin1Char('*')));
        QVERIFY(w.windowTitle().contains(QStringLiteral("p2")));
    }

    void undoRedoActions() {
        mcs::ui::MainWindow w;
        w.importAndReport({video_});
        w.editPage()->appendToTimeline(w.mediaPool()->index(0));
        auto* undo = w.findChild<QAction*>(QStringLiteral("action_undo"));
        auto* redo = w.findChild<QAction*>(QStringLiteral("action_redo"));
        QVERIFY(undo && redo);
        QCOMPARE(undo->shortcut(), QKeySequence(QKeySequence::Undo));
        undo->trigger();
        QCOMPARE(w.editPage()->timeline().duration(), 0);
        redo->trigger();
        QCOMPARE(w.editPage()->timeline().duration(), 30);
    }

    // Review Focus 3
    void missingMediaLoadsWithWarning() {
        const QString copy = dir_.filePath(QStringLiteral("some.mov"));
        QVERIFY(QFile::copy(video_, copy));
        const QString path = dir_.filePath(QStringLiteral("p3.mcsproj"));
        {
            mcs::ui::MainWindow w;
            w.importAndReport({copy});
            w.editPage()->appendToTimeline(w.mediaPool()->index(0));
            QCOMPARE(w.saveProjectTo(path), QString());
        }
        QVERIFY(QFile::remove(copy));
        mcs::ui::MainWindow w2;
        QCOMPARE(w2.openProject(path), QString());
        QCOMPARE(w2.lastWarnings().size(), 1);
        QVERIFY(w2.lastWarnings().first().contains(QStringLiteral("some.mov")));
        QCOMPARE(w2.editPage()->timeline().video[0].clips.size(), size_t(1));
        // Revisão final I2: salvar não pode apagar a mídia ausente da lista do projeto.
        const QString resaved = dir_.filePath(QStringLiteral("p3b.mcsproj"));
        QCOMPARE(w2.saveProjectTo(resaved), QString());
        QVERIFY(mcs::core::loadProject(resaved).project->media.contains(copy));
    }

    void autosaveWritesLoadableFile() {
        mcs::ui::MainWindow w;
        w.importAndReport({video_});
        w.editPage()->appendToTimeline(w.mediaPool()->index(0));
        QCOMPARE(w.autosaveNow(), QString());
        const auto loaded = mcs::core::loadProject(w.autosavePath());
        QVERIFY2(loaded.project, qPrintable(loaded.error));
        QCOMPARE(formatTimeline(loaded.project->timeline),
                 formatTimeline(w.editPage()->timeline()));
    }

    void newProjectClearsEverything() {
        mcs::ui::MainWindow w;
        w.importAndReport({video_});
        w.editPage()->appendToTimeline(w.mediaPool()->index(0));
        w.newProject();
        QCOMPARE(w.mediaPool()->rowCount(), 0);
        QCOMPARE(w.editPage()->timeline().duration(), 0);
        QVERIFY(w.projectPath().isEmpty());
        QCOMPARE(w.editPage()->document()->undoStack()->count(), 0);
    }

    void openCorruptProjectKeepsCurrent() {
        const QString path = dir_.filePath(QStringLiteral("ruim.mcsproj"));
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{ corrompido");
        f.close();
        mcs::ui::MainWindow w;
        w.importAndReport({video_});
        QVERIFY(!w.openProject(path).isEmpty());
        QCOMPARE(w.mediaPool()->rowCount(), 1);
    }
};

QTEST_MAIN(TstProjectUi)
#include "tst_projectui.moc"
