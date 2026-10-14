#include "core/timeline.h"
#include "ui/timeline_view.h"

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
};

QTEST_MAIN(TstTimelineView)
#include "tst_timelineview.moc"
