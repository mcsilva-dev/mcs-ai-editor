#include "core/timeline.h"
#include "media/audio_decoder.h"
#include "media/probe.h"
#include "playback/playback_engine.h"
#include "playback/timeline_audio.h"
#include "playback/timeline_video.h"
#include "support/synth_media.h"

#include <QTemporaryDir>
#include <QtTest>
#include <vector>

namespace {
constexpr std::int64_t kClipSamples = 115200; // 60 quadros a 25 fps
int centerGray(const QImage& img) {
    return img.pixelColor(img.width() / 2, img.height() / 2).red();
}
} // namespace

class TstPlayback : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir dir_;
    QString clip_;

    mcs::core::Timeline repeatedClip(int copies) {
        const auto info = mcs::media::probe(clip_).info;
        mcs::core::Timeline t;
        for (int i = 0; i < copies; ++i) {
            mcs::core::appendMedia(t, *info);
        }
        return t;
    }

  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        clip_ = dir_.filePath(QStringLiteral("clip.mov"));
        QCOMPARE(mcs::test::writeSynthMov(clip_), QString());
    }

    void mixerFollowsClipsAcrossBoundary() {
        mcs::playback::TimelineAudio audio;
        audio.setTimeline(repeatedClip(2));
        std::vector<float> buf(2 * 400);
        audio.render(kClipSamples - 200, buf.data(), 400); // 200 do 1º clipe + 200 do 2º
        for (int i = 0; i < 400; ++i) {
            const std::int64_t local = (kClipSamples - 200 + i) % kClipSamples;
            QVERIFY(qAbs(buf[2 * i] - mcs::test::expectedSampleAt(local)) < 1e-4f);
        }
    }

    void clipGainScalesSamples() {
        auto t = repeatedClip(1);
        t.audio[0].clips[0].gainDb = -6.0206; // ×0,5
        mcs::playback::TimelineAudio audio;
        audio.setTimeline(t);
        std::vector<float> buf(2 * 100);
        audio.render(1000, buf.data(), 100);
        QVERIFY(qAbs(buf[2 * 10] - 0.5f * mcs::test::expectedSampleAt(1010)) < 1e-4f);
    }

    // Review Focus 3
    void gapIsSilence() {
        auto t = repeatedClip(1);
        t.audio[0].clips[0].start = 25; // 1 s de lacuna
        mcs::playback::TimelineAudio audio;
        audio.setTimeline(t);
        std::vector<float> buf(2 * 100, 1.0f);
        audio.render(0, buf.data(), 100);
        for (float v : buf) {
            QCOMPARE(v, 0.0f);
        }
        audio.render(48000, buf.data(), 100);
        QVERIFY(qAbs(buf[2 * 10] - mcs::test::expectedSampleAt(10)) < 1e-4f);
    }

    void clockFollowsDeliveredAudioMinusLatency() {
        mcs::playback::PlaybackEngine engine;
        engine.setTimeline(repeatedClip(2));
        engine.setOutputLatency(1024);
        engine.seek(0);
        QCOMPARE(engine.currentFrame(), 0);
        std::vector<float> buf(2 * 4800);
        for (int i = 0; i < 10; ++i) { // 1 s
            engine.fillAhead();
            engine.pull(buf.data(), 4800);
        }
        QCOMPARE(engine.deliveredFrames(), 48000);
        QCOMPARE(engine.currentFrame(), (48000 - 1024) / 1920); // 24
    }

    void seekStartsAudioAtFrame() {
        mcs::playback::PlaybackEngine engine;
        engine.setTimeline(repeatedClip(2));
        engine.seek(70); // 2º clipe, quadro local 10
        QCOMPARE(engine.currentFrame(), 70);
        engine.fillAhead();
        std::vector<float> buf(2 * 100);
        engine.pull(buf.data(), 100);
        QVERIFY(qAbs(buf[0] - mcs::test::expectedSampleAt(10 * 1920)) < 1e-4f);
    }

    // Review Focus 4
    void underrunStallsClock() {
        mcs::playback::PlaybackEngine engine;
        engine.setTimeline(repeatedClip(1));
        engine.seek(0);
        std::vector<float> buf(2 * 1000, 1.0f);
        engine.pull(buf.data(), 1000); // nada preenchido
        QCOMPARE(engine.underruns(), 1);
        QCOMPARE(buf[0], 0.0f);
        QCOMPARE(engine.deliveredFrames(), 0);
        QCOMPARE(engine.currentFrame(), 0);
    }

    // Spec v0.3: drift A/V < 1 quadro em 10 minutos (tempo virtual, 60 ticks por segundo).
    void tenMinutesStayInSync() {
        const auto timeline = repeatedClip(250); // 250 × 2,4 s = 600 s
        mcs::playback::PlaybackEngine engine;
        engine.setTimeline(timeline);
        engine.setOutputLatency(1024);
        engine.seek(0);
        mcs::playback::TimelineVideo video;
        video.setTimeline(timeline);

        constexpr int kTick = 800; // 1/60 s a 48 kHz
        std::vector<float> buf(2 * kTick);
        std::int64_t played = 0;
        std::int64_t shown = -1;
        while (played < 250 * kClipSamples) {
            engine.fillAhead();
            engine.pull(buf.data(), kTick);
            // Áudio: cada amostra entregue é a da posição certa da timeline (sem buraco nem pulo).
            for (int i = 0; i < kTick; i += 97) {
                const float expected = mcs::test::expectedSampleAt((played + i) % kClipSamples);
                if (qAbs(buf[2 * i] - expected) >= 1e-4f) {
                    QFAIL(qPrintable(QStringLiteral("áudio errado na amostra %1").arg(played + i)));
                }
            }
            played += kTick;
            // Vídeo: o quadro exibido é o do áudio que está soando (entregue − latência).
            const std::int64_t frame = engine.currentFrame();
            const std::int64_t expectedFrame = std::max<std::int64_t>(0, played - 1024) / 1920;
            if (frame != expectedFrame) {
                QFAIL(qPrintable(
                    QStringLiteral("relógio em %1, esperado %2").arg(frame).arg(expectedFrame)));
            }
            if (frame != shown && frame < timeline.duration()) {
                const int gray = centerGray(video.frameAt(frame));
                if (qAbs(gray - mcs::test::expectedGrayForFrame(static_cast<int>(frame % 60))) >
                    4) {
                    QFAIL(
                        qPrintable(QStringLiteral("quadro %1 com cinza %2").arg(frame).arg(gray)));
                }
                shown = frame;
            }
        }
        QCOMPARE(engine.underruns(), 0);
    }
};

QTEST_GUILESS_MAIN(TstPlayback)
#include "tst_playback.moc"
