#include "core/edit_ops.h"
#include "edit/timeline_document.h"
#include "playback/player.h"
#include "support/synth_media.h"
#include "ui/edit_page.h"
#include "ui/media_pool_model.h"
#include "ui/source_viewer.h"
#include "ui/timeline_view.h"

#include <QAction>
#include <QDropEvent>
#include <QListView>
#include <QMimeData>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest>

namespace {
int centerGray(const QImage& img) {
    return img.pixelColor(img.width() / 2, img.height() / 2).red();
}
} // namespace

class TstEditPage : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir dir_;
    QString video_;
    QString audio_;

  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        video_ = dir_.filePath(QStringLiteral("clip.mov"));
        mcs::test::SynthSpec s;
        s.frames = 30;
        QCOMPARE(mcs::test::writeSynthMov(video_, s), QString());
        audio_ = dir_.filePath(QStringLiteral("so_audio.mov"));
        mcs::test::SynthSpec a;
        a.video = false;
        QCOMPARE(mcs::test::writeSynthMov(audio_, a), QString());
    }

    void selectingClipShowsFirstFrame() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.mediaPoolView()->setCurrentIndex(pool.index(0));
        QCOMPARE(page.sourceViewer()->frameCount(), 30);
        QCOMPARE(page.sourceViewer()->position(), 0);
        QVERIFY(!page.sourceViewer()->currentFrame().isNull());
        QCOMPARE(page.sourceViewer()->timecodeText(), QStringLiteral("00:00:00:00"));
    }

    void scrubShowsExactFrameAndTimecode() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.mediaPoolView()->setCurrentIndex(pool.index(0));
        page.sourceViewer()->setPosition(7);
        QCOMPARE(page.sourceViewer()->position(), 7);
        QVERIFY(qAbs(centerGray(page.sourceViewer()->currentFrame()) -
                     mcs::test::expectedGrayForFrame(7)) <= 4);
        QCOMPARE(page.sourceViewer()->timecodeText(), QStringLiteral("00:00:00:07"));
    }

    void positionIsClamped() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.mediaPoolView()->setCurrentIndex(pool.index(0));
        page.sourceViewer()->setPosition(999);
        QCOMPARE(page.sourceViewer()->position(), 29);
        page.sourceViewer()->setPosition(-5);
        QCOMPARE(page.sourceViewer()->position(), 0);
    }

    void droppingFilesImports() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(video_)});
        // Como num arrasto real: o Qt só entrega o Drop ao widget que aceitou o DragEnter.
        QDragEnterEvent enter(QPoint(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(page.mediaPoolView()->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        QDropEvent drop(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(page.mediaPoolView()->viewport(), &drop);
        QCOMPARE(pool.rowCount(), 1);
    }

    void appendBuildsLinkedClips() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.appendToTimeline(pool.index(0));
        page.appendToTimeline(pool.index(0));
        QCOMPARE(page.timeline().video[0].clips.size(), size_t(2));
        QCOMPARE(page.timeline().audio[0].clips.size(), size_t(2));
        QCOMPARE(page.timeline().duration(), 60);
    }

    void playbackAdvancesPlayhead() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        page.player()->setAudioBackend(mcs::playback::AudioOutput::Backend::Null);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.appendToTimeline(pool.index(0));
        page.togglePlayback();
        QTest::qWait(400);
        page.togglePlayback();
        QVERIFY(page.timelineView()->playhead() > 3);
        QVERIFY(!page.programFrame().isNull());
    }

    void clickingTimelineSeeksAndShowsFrame() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.appendToTimeline(pool.index(0));
        auto* view = page.timelineView();
        QTest::mouseClick(view, Qt::LeftButton, {}, QPoint(view->xForFrame(7), 10));
        QCOMPARE(page.player()->currentFrame(), 7);
        QVERIFY(qAbs(centerGray(page.programFrame()) - mcs::test::expectedGrayForFrame(7)) <= 4);
    }

    void bladeSplitsAtPlayhead() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.appendToTimeline(pool.index(0));
        page.player()->seek(10);
        page.bladeAtPlayhead();
        QCOMPARE(page.timeline().video[0].clips.size(), size_t(2));
        QCOMPARE(page.timeline().audio[0].clips.size(), size_t(2));
    }

    void inOutMarksDriveInsert() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.mediaPoolView()->setCurrentIndex(pool.index(0));
        page.sourceViewer()->setPosition(5);
        page.sourceViewer()->markIn();
        page.sourceViewer()->setPosition(14);
        page.sourceViewer()->markOut();
        page.insertFromSource();
        QCOMPARE(page.timeline().video[0].clips.size(), size_t(1));
        QCOMPARE(page.timeline().video[0].clips[0].duration, 10);
        QCOMPARE(page.timeline().video[0].clips[0].sourceIn, 5);
    }

    void rippleDeleteSelectionAndUndo() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.appendToTimeline(pool.index(0));
        page.appendToTimeline(pool.index(0));
        page.timelineView()->setSelection(
            mcs::core::withLinked(page.timeline(), {page.timeline().video[0].clips[0].id}));
        page.rippleDeleteSelection();
        QCOMPARE(page.timeline().duration(), 30);
        QCOMPARE(page.timeline().video[0].clips[0].start, 0);
        page.document()->undoStack()->undo();
        QCOMPARE(page.timeline().duration(), 60);
    }

    // Pedido do Miguel: retirar só o áudio do vídeo e baixar o volume.
    void unlinkedDeleteRemovesOnlyAudio() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.appendToTimeline(pool.index(0));
        page.findChild<QAction*>(QStringLiteral("action_linkedSelection"))->trigger(); // desliga
        QVERIFY(!page.timelineView()->linkedSelection());
        page.timelineView()->setSelection({page.timeline().audio[0].clips[0].id});
        page.liftSelection();
        QCOMPARE(page.timeline().video[0].clips.size(), size_t(1));
        QVERIFY(page.timeline().audio[0].clips.empty());
    }

    void gainShortcutsChangeVolume() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.appendToTimeline(pool.index(0));
        page.timelineView()->setSelection(
            mcs::core::withLinked(page.timeline(), {page.timeline().video[0].clips[0].id}));
        auto* down = page.findChild<QAction*>(QStringLiteral("action_gainDown"));
        QVERIFY(down);
        QCOMPARE(down->shortcut(), QKeySequence(QStringLiteral("Ctrl+Alt+-")));
        down->trigger();
        down->trigger();
        QCOMPARE(page.timeline().audio[0].clips[0].gainDb, -2.0);
        page.findChild<QAction*>(QStringLiteral("action_gainUp"))->trigger();
        QCOMPARE(page.timeline().audio[0].clips[0].gainDb, -1.0);
    }

    // Review Focus 5
    void impossibleEditIsReported() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QSignalSpy failed(&page, &mcs::ui::EditPage::editFailed);
        page.bladeAtPlayhead();  // timeline vazia
        page.liftSelection();    // nada selecionado
        page.insertFromSource(); // nenhuma mídia selecionada
        QCOMPARE(failed.count(), 3);
        QCOMPARE(page.document()->undoStack()->count(), 0);
    }

    // Review Focus 5
    void playAtEndRestartsFromZero() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        page.player()->setAudioBackend(mcs::playback::AudioOutput::Backend::Null);
        QVERIFY(pool.importFiles({video_}).isEmpty());
        page.appendToTimeline(pool.index(0));
        page.player()->seek(page.timeline().duration());
        page.togglePlayback();
        QVERIFY(page.player()->currentFrame() < 5);
        page.togglePlayback();
    }

    // Review Focus 5
    void audioOnlyClearsViewer() {
        mcs::ui::MediaPoolModel pool;
        mcs::ui::EditPage page(&pool);
        QVERIFY(pool.importFiles({video_, audio_}).isEmpty());
        page.mediaPoolView()->setCurrentIndex(pool.index(0));
        QVERIFY(!page.sourceViewer()->currentFrame().isNull());
        page.mediaPoolView()->setCurrentIndex(pool.index(1));
        QVERIFY(page.sourceViewer()->currentFrame().isNull());
        QCOMPARE(page.sourceViewer()->frameCount(), 0);
    }
};

QTEST_MAIN(TstEditPage)
#include "tst_editpage.moc"
