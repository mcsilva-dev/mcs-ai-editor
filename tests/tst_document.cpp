#include "core/edit_ops.h"
#include "edit/timeline_document.h"
#include "support/timeline_text.h"

#include <QSignalSpy>
#include <QUndoStack>
#include <QtTest>

using mcs::test::formatTimeline;
using mcs::test::parseTimeline;

class TstDocument : public QObject {
    Q_OBJECT
  private slots:
    void applyUndoRedo() {
        mcs::edit::TimelineDocument doc;
        doc.reset(parseTimeline("V1: A[0..20) | A1: a[0..20)"));
        QSignalSpy changed(&doc, &mcs::edit::TimelineDocument::timelineChanged);
        QCOMPARE(doc.apply(QStringLiteral("Blade"), mcs::core::blade(doc.timeline(), 8)),
                 QString());
        QCOMPARE(formatTimeline(doc.timeline()),
                 QStringLiteral("V1: A[0..8) A[8..20)@8 | A1: a[0..8) a[8..20)@8"));
        QCOMPARE(doc.undoStack()->undoText(), QStringLiteral("Blade"));
        doc.undoStack()->undo();
        QCOMPARE(formatTimeline(doc.timeline()), QStringLiteral("V1: A[0..20) | A1: a[0..20)"));
        doc.undoStack()->redo();
        QCOMPARE(doc.timeline().video[0].clips.size(), size_t(2));
        QCOMPARE(changed.count(), 3);
    }

    // Review Focus 5: edição impossível não mexe no histórico.
    void errorDoesNotTouchHistory() {
        mcs::edit::TimelineDocument doc;
        doc.reset(parseTimeline("V1: A[0..10) | A1: -"));
        QSignalSpy changed(&doc, &mcs::edit::TimelineDocument::timelineChanged);
        const QString err =
            doc.apply(QStringLiteral("Blade"), mcs::core::blade(doc.timeline(), 50));
        QVERIFY(!err.isEmpty());
        QCOMPARE(doc.undoStack()->count(), 0);
        QCOMPARE(changed.count(), 0);
        QVERIFY(!doc.isModified());
    }

    void savedStateTracksModifications() {
        mcs::edit::TimelineDocument doc;
        doc.reset(parseTimeline("V1: A[0..20) | A1: -"));
        QVERIFY(!doc.isModified());
        doc.apply(QStringLiteral("Blade"), mcs::core::blade(doc.timeline(), 5));
        QVERIFY(doc.isModified());
        doc.markSaved();
        QVERIFY(!doc.isModified());
        doc.undoStack()->undo();
        QVERIFY(doc.isModified());
    }

    void resetClearsHistory() {
        mcs::edit::TimelineDocument doc;
        doc.apply(QStringLiteral("x"),
                  mcs::core::EditResult{parseTimeline("V1: A[0..5) | A1: -"), {}});
        doc.reset(mcs::core::Timeline{});
        QCOMPARE(doc.undoStack()->count(), 0);
        QCOMPARE(doc.timeline().duration(), 0);
        QVERIFY(!doc.isModified());
    }
};

QTEST_GUILESS_MAIN(TstDocument)
#include "tst_document.moc"
