#include "core/timeline.h"
#include "support/timeline_text.h"
#include "ui/timeline_view.h"

#include <QMouseEvent>
#include <QSignalSpy>
#include <QtTest>

class TstTimelineView : public QObject {
    Q_OBJECT
  private slots:
    void clickMovesPlayhead() {
        mcs::core::Timeline t;
        t.video[0].clips.push_back(mcs::core::Clip{
            QUuid::createUuid(), QStringLiteral("a.mov"), {25, 1}, 0, 60, 0, {}, 0.0});
        mcs::ui::TimelineView view;
        view.resize(800, 200);
        view.setTimeline(&t);
        QSignalSpy spy(&view, &mcs::ui::TimelineView::playheadMoved);
        QTest::mouseClick(&view, Qt::LeftButton, {}, QPoint(view.xForFrame(30), 10));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().value<std::int64_t>(), 30);
        QCOMPARE(view.playhead(), 30);
    }

    void clickOnHeaderIsFrameZero() {
        mcs::ui::TimelineView view;
        QCOMPARE(view.frameAtX(5), 0);
    }

    void paintsWithAndWithoutTimeline() {
        mcs::ui::TimelineView view;
        view.resize(600, 150);
        QVERIFY(!view.grab().isNull());
        mcs::core::Timeline t;
        t.audio[0].clips.push_back(mcs::core::Clip{
            QUuid::createUuid(), QStringLiteral("b.wav"), {25, 1}, 10, 40, 0, {}, 0.0});
        view.setTimeline(&t);
        view.setPlayhead(20);
        QVERIFY(!view.grab().isNull());
    }

    // Eventos de mouse com estado dos botões explícito (QTest::mouseMove não carrega botões).
    static void mouse(QWidget* w, QEvent::Type type, QPoint pos, Qt::MouseButtons buttons,
                      Qt::KeyboardModifiers mods = {}) {
        QMouseEvent e(type, QPointF(pos), w->mapToGlobal(QPointF(pos)),
                      type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton, buttons, mods);
        QCoreApplication::sendEvent(w, &e);
    }

    static int rowY(int row) {
        return mcs::ui::TimelineView::kRulerHeight + row * mcs::ui::TimelineView::kTrackHeight +
               mcs::ui::TimelineView::kTrackHeight / 2;
    }

    void clickOnClipSelectsLinkedAndKeepsPlayhead() {
        const auto t = mcs::test::parseTimeline("V1: A[0..60) | A1: a[0..60)");
        mcs::ui::TimelineView view;
        view.resize(800, 200);
        view.setTimeline(&t);
        QSignalSpy playhead(&view, &mcs::ui::TimelineView::playheadMoved);
        const QPoint p(view.xForFrame(30), rowY(0));
        mouse(&view, QEvent::MouseButtonPress, p, Qt::LeftButton);
        mouse(&view, QEvent::MouseButtonRelease, p, Qt::NoButton);
        QCOMPARE(view.selection().size(), 2);
        QCOMPARE(playhead.count(), 0);
    }

    void linkedSelectionOffSelectsOnlyClicked() {
        const auto t = mcs::test::parseTimeline("V1: A[0..60) | A1: a[0..60)");
        mcs::ui::TimelineView view;
        view.resize(800, 200);
        view.setTimeline(&t);
        view.setLinkedSelection(false);
        const QPoint audio(view.xForFrame(30), rowY(1));
        mouse(&view, QEvent::MouseButtonPress, audio, Qt::LeftButton);
        mouse(&view, QEvent::MouseButtonRelease, audio, Qt::NoButton);
        QCOMPARE(view.selection(), mcs::test::idsOf(t, QStringLiteral("a")));
    }

    void dragMovesWithSnap() {
        const auto t = mcs::test::parseTimeline("V1: A[0..10) B[40..50) | A1: -");
        mcs::ui::TimelineView view;
        view.resize(800, 200);
        view.setTimeline(&t);
        view.setPlayhead(300);
        QSignalSpy moved(&view, &mcs::ui::TimelineView::clipsMoveRequested);
        mouse(&view, QEvent::MouseButtonPress, QPoint(view.xForFrame(5), rowY(0)), Qt::LeftButton);
        mouse(&view, QEvent::MouseMove, QPoint(view.xForFrame(33), rowY(0)), Qt::LeftButton);
        mouse(&view, QEvent::MouseButtonRelease, QPoint(view.xForFrame(33), rowY(0)), Qt::NoButton);
        QCOMPARE(moved.count(), 1);
        QCOMPARE(moved.first().at(1).value<std::int64_t>(), 30); // fim de A (38) encosta em B (40)
    }

    void dragWithoutSnapping() {
        const auto t = mcs::test::parseTimeline("V1: A[0..10) B[40..50) | A1: -");
        mcs::ui::TimelineView view;
        view.resize(800, 200);
        view.setTimeline(&t);
        view.setPlayhead(300);
        view.setSnapping(false);
        QSignalSpy moved(&view, &mcs::ui::TimelineView::clipsMoveRequested);
        mouse(&view, QEvent::MouseButtonPress, QPoint(view.xForFrame(5), rowY(0)), Qt::LeftButton);
        mouse(&view, QEvent::MouseMove, QPoint(view.xForFrame(33), rowY(0)), Qt::LeftButton);
        mouse(&view, QEvent::MouseButtonRelease, QPoint(view.xForFrame(33), rowY(0)), Qt::NoButton);
        QCOMPARE(moved.first().at(1).value<std::int64_t>(), 28);
    }

    void clickEmptyAreaClearsSelectionAndMovesPlayhead() {
        const auto t = mcs::test::parseTimeline("V1: A[0..10) | A1: -");
        mcs::ui::TimelineView view;
        view.resize(800, 200);
        view.setTimeline(&t);
        view.setSelection(mcs::test::idsOf(t, QStringLiteral("A")));
        QSignalSpy playhead(&view, &mcs::ui::TimelineView::playheadMoved);
        const QPoint p(view.xForFrame(100), rowY(0));
        mouse(&view, QEvent::MouseButtonPress, p, Qt::LeftButton);
        mouse(&view, QEvent::MouseButtonRelease, p, Qt::NoButton);
        QVERIFY(view.selection().isEmpty());
        QCOMPARE(playhead.count(), 1);
    }

    void ctrlClickTogglesSelection() {
        const auto t = mcs::test::parseTimeline("V1: A[0..10) B[10..20) | A1: -");
        mcs::ui::TimelineView view;
        view.resize(800, 200);
        view.setTimeline(&t);
        const QPoint a(view.xForFrame(5), rowY(0));
        const QPoint b(view.xForFrame(15), rowY(0));
        mouse(&view, QEvent::MouseButtonPress, a, Qt::LeftButton);
        mouse(&view, QEvent::MouseButtonRelease, a, Qt::NoButton);
        mouse(&view, QEvent::MouseButtonPress, b, Qt::LeftButton, Qt::ControlModifier);
        mouse(&view, QEvent::MouseButtonRelease, b, Qt::NoButton, Qt::ControlModifier);
        QCOMPARE(view.selection().size(), 2);
        mouse(&view, QEvent::MouseButtonPress, a, Qt::LeftButton, Qt::ControlModifier);
        mouse(&view, QEvent::MouseButtonRelease, a, Qt::NoButton, Qt::ControlModifier);
        QCOMPARE(view.selection(), mcs::test::idsOf(t, QStringLiteral("B")));
    }
};

QTEST_MAIN(TstTimelineView)
#include "tst_timelineview.moc"
