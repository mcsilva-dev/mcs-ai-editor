#include "core/timeline.h"

#include <QtTest>

using namespace mcs::core;

namespace {
MediaInfo videoInfo(std::int64_t frames, Rational fps, bool audio = true) {
    MediaInfo info;
    info.path = QStringLiteral("clip.mov");
    VideoStreamInfo v;
    v.width = 64;
    v.height = 48;
    v.fps = fps;
    v.frameCount = frames;
    info.video = v;
    info.durationSeconds = static_cast<double>(frames) * fps.den / fps.num;
    if (audio) {
        info.audio = AudioStreamInfo{48000, 2, QStringLiteral("pcm_s16le")};
    }
    return info;
}
} // namespace

class TstTimeline : public QObject {
    Q_OBJECT
  private slots:
    void appendMediaPlacesLinkedClips() {
        Timeline t;
        QCOMPARE(appendMedia(t, videoInfo(60, {25, 1})), 0);
        QCOMPARE(appendMedia(t, videoInfo(60, {25, 1})), 60);
        QCOMPARE(t.video[0].clips.size(), size_t(2));
        QCOMPARE(t.audio[0].clips.size(), size_t(2));
        QCOMPARE(t.video[0].clips[1].start, 60);
        QCOMPARE(t.audio[0].clips[1].start, 60);
        QCOMPARE(t.duration(), 120);
    }

    void appendMediaLinksVideoAndAudio() {
        Timeline t;
        appendMedia(t, videoInfo(60, {25, 1}));
        QVERIFY(!t.video[0].clips[0].link.isNull());
        QCOMPARE(t.video[0].clips[0].link, t.audio[0].clips[0].link);
        QVERIFY(t.video[0].clips[0].id != t.audio[0].clips[0].id);
    }

    void emptyTimelineAdoptsVideoFps() {
        Timeline t;
        appendMedia(t, videoInfo(30, {30000, 1001}));
        QCOMPARE(t.fps, (Rational{30000, 1001}));
    }

    // Review Focus 1
    void appendMediaRescalesDuration() {
        Timeline t;
        appendMedia(t, videoInfo(50, {25, 1}));
        appendMedia(t, videoInfo(100, {50, 1})); // 2 s a 50 fps = 50 quadros a 25 fps
        QCOMPARE(t.video[0].clips[1].duration, 50);
        QCOMPARE(t.duration(), 100);
    }

    void audioOnlyUsesDurationSeconds() {
        Timeline t;
        MediaInfo a;
        a.path = QStringLiteral("musica.wav");
        a.durationSeconds = 2.0;
        a.audio = AudioStreamInfo{48000, 2, QStringLiteral("pcm_s16le")};
        QCOMPARE(appendMedia(t, a), 0);
        QVERIFY(t.video[0].clips.empty());
        QCOMPARE(t.audio[0].clips[0].duration, 50);
    }

    void clipAtUsesHalfOpenRange() {
        Timeline t;
        appendMedia(t, videoInfo(60, {25, 1}));
        appendMedia(t, videoInfo(60, {25, 1}));
        QCOMPARE(clipAt(t.video[0], 0), &t.video[0].clips[0]);
        QCOMPARE(clipAt(t.video[0], 59), &t.video[0].clips[0]);
        QCOMPARE(clipAt(t.video[0], 60), &t.video[0].clips[1]);
        QVERIFY(!clipAt(t.video[0], 120));
        QVERIFY(!clipAt(t.video[0], -1));
    }

    void sourceFrameAtSameFps() {
        Clip c{{}, {}, {25, 1}, 100, 50, 10, {}, 0.0};
        QCOMPARE(sourceFrameAt(c, 100, {25, 1}), 10);
        QCOMPARE(sourceFrameAt(c, 120, {25, 1}), 30);
    }

    // Review Focus 1
    void sourceFrameAtDifferentFps() {
        Clip c{{}, {}, {50, 1}, 0, 50, 0, {}, 0.0};
        QCOMPARE(sourceFrameAt(c, 7, {25, 1}), 14);
    }

    void frameSampleRoundTripNtsc() {
        const Rational ntsc{30000, 1001};
        for (std::int64_t f = 0; f < 2000; ++f) {
            QCOMPARE(sampleToFrame(frameToSample(f, ntsc, 48000), ntsc, 48000), f);
        }
        QCOMPARE(frameToSample(25, {25, 1}, 48000), 48000);
    }

    void sourceSampleAtFollowsClipStart() {
        Clip c{{}, {}, {25, 1}, 60, 60, 25, {}, 0.0}; // começa no quadro 60 (115200), sourceIn 1 s
        QCOMPARE(sourceSampleAt(c, 115200, {25, 1}, 48000), 48000);
        QCOMPARE(sourceSampleAt(c, 115200 + 10, {25, 1}, 48000), 48010);
    }
};

QTEST_GUILESS_MAIN(TstTimeline)
#include "tst_timeline.moc"
