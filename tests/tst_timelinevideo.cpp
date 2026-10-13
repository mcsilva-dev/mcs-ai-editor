#include "core/timeline.h"
#include "media/probe.h"
#include "playback/timeline_video.h"
#include "support/synth_media.h"

#include <QTemporaryDir>
#include <QtTest>

namespace {
int centerGray(const QImage& img) {
    return img.pixelColor(img.width() / 2, img.height() / 2).red();
}
} // namespace

class TstTimelineVideo : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir dir_;
    QString clip_;

  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        clip_ = dir_.filePath(QStringLiteral("clip.mov"));
        mcs::test::SynthSpec s;
        s.frames = 30;
        QCOMPARE(mcs::test::writeSynthMov(clip_, s), QString());
    }

    void framesFollowClipsAcrossBoundary() {
        const auto info = mcs::media::probe(clip_).info;
        QVERIFY(info);
        mcs::core::Timeline t;
        mcs::core::appendMedia(t, *info);
        mcs::core::appendMedia(t, *info);
        mcs::playback::TimelineVideo video;
        video.setTimeline(t);
        QVERIFY(qAbs(centerGray(video.frameAt(5)) - mcs::test::expectedGrayForFrame(5)) <= 4);
        QVERIFY(qAbs(centerGray(video.frameAt(29)) - mcs::test::expectedGrayForFrame(29)) <= 4);
        QVERIFY(qAbs(centerGray(video.frameAt(35)) - mcs::test::expectedGrayForFrame(5)) <= 4);
    }

    // Review Focus 3
    void gapIsNullImage() {
        const auto info = mcs::media::probe(clip_).info;
        QVERIFY(info);
        mcs::core::Timeline t;
        mcs::core::appendMedia(t, *info);
        t.video[0].clips[0].start = 10; // lacuna de 0 a 9
        mcs::playback::TimelineVideo video;
        video.setTimeline(t);
        QVERIFY(video.frameAt(3).isNull());
        QVERIFY(!video.frameAt(10).isNull());
        QVERIFY(video.frameAt(1000).isNull());
    }
};

QTEST_GUILESS_MAIN(TstTimelineVideo)
#include "tst_timelinevideo.moc"
