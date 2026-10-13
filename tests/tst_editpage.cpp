#include "support/synth_media.h"
#include "ui/edit_page.h"
#include "ui/media_pool_model.h"
#include "ui/source_viewer.h"

#include <QDropEvent>
#include <QListView>
#include <QMimeData>
#include <QTemporaryDir>
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
