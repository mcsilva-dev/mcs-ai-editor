#include "ui/main_window.h"
#include "ui/theme.h"

#include <QAction>
#include <QApplication>
#include <QStyle>
#include <QToolButton>
#include <QtTest>

class TstMainWindow : public QObject {
    Q_OBJECT
  private slots:
    void titleHasVersion() {
        mcs::ui::MainWindow w;
        QVERIFY(w.windowTitle().contains(QStringLiteral(MCS_VERSION)));
    }

    void hasEditAndDeliverPagesInOrder() {
        mcs::ui::MainWindow w;
        QCOMPARE(w.pageNames(), (QStringList{QStringLiteral("edit"), QStringLiteral("deliver")}));
    }

    void startsOnEditPage() {
        mcs::ui::MainWindow w;
        QCOMPARE(w.currentPage(), QStringLiteral("edit"));
    }

    void clickingPageButtonSwitchesPage() {
        mcs::ui::MainWindow w;
        w.show();
        auto* btn = w.findChild<QToolButton*>(QStringLiteral("pageButton_deliver"));
        QVERIFY(btn);
        QTest::mouseClick(btn, Qt::LeftButton);
        QCOMPARE(w.currentPage(), QStringLiteral("deliver"));
        QVERIFY(btn->isChecked());
    }

    void showPageIgnoresUnknownName() {
        mcs::ui::MainWindow w;
        w.showPage(QStringLiteral("nao_existe"));
        QCOMPARE(w.currentPage(), QStringLiteral("edit"));
    }

    void importActionHasShortcut() {
        mcs::ui::MainWindow w;
        auto* action = w.findChild<QAction*>(QStringLiteral("action_importMedia"));
        QVERIFY(action);
        QCOMPARE(action->shortcut(), QKeySequence(QStringLiteral("Ctrl+I")));
        QVERIFY(w.mediaPool());
    }

    // Review Focus 4: tema escuro independente do tema do Windows.
    void darkThemeIndependentOfOs() {
        auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
        QVERIFY(app);
        app->setPalette(QPalette(Qt::white)); // simula SO em tema claro
        mcs::ui::applyDarkTheme(*app);
        QVERIFY(QApplication::palette().color(QPalette::Window).lightness() < 64);
        QCOMPARE(QApplication::style()->name(), QStringLiteral("fusion"));
    }
};

QTEST_MAIN(TstMainWindow)
#include "tst_mainwindow.moc"
