#include "core/timeline.h"
#include "media/probe.h"
#include "playback/player.h"
#include "support/synth_media.h"

#include <QTemporaryDir>
#include <QtTest>

using mcs::playback::AudioOutput;
using mcs::playback::Player;

class TstPlayer : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir dir_;
    mcs::core::Timeline timeline_;

  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        const QString clip = dir_.filePath(QStringLiteral("clip.mov"));
        QCOMPARE(mcs::test::writeSynthMov(clip), QString());
        const auto info = mcs::media::probe(clip).info;
        QVERIFY(info);
        mcs::core::appendMedia(timeline_, *info); // 60 quadros = 2,4 s
    }

    // Dispositivo simulado do miniaudio, em tempo real: o relógio anda como o áudio.
    void nullDeviceAdvancesInRealTime() {
        Player p;
        p.setAudioBackend(AudioOutput::Backend::Null);
        p.setTimeline(timeline_);
        p.play();
        QVERIFY(p.isPlaying());
        QVERIFY(!p.usingFallbackClock());
        QTest::qWait(600);
        const std::int64_t f = p.currentFrame();
        QVERIFY2(f >= 8 && f <= 18, qPrintable(QString::number(f))); // ~15 quadros em 0,6 s
        p.pause();
        const std::int64_t paused = p.currentFrame();
        QTest::qWait(150);
        QCOMPARE(p.currentFrame(), paused);
    }

    // Review Focus 2
    void noDeviceUsesFallbackClock() {
        Player p;
        p.setAudioBackend(AudioOutput::Backend::None);
        p.setTimeline(timeline_);
        p.play();
        QVERIFY(p.usingFallbackClock());
        QTest::qWait(400);
        const std::int64_t f = p.currentFrame();
        QVERIFY2(f >= 6 && f <= 14, qPrintable(QString::number(f))); // ~10 quadros
        p.pause();
    }

    // Review Focus 2: fone desconectado no meio do playback — a imagem não pode congelar.
    void deviceLossFallsBackToSystemClock() {
        Player p;
        p.setAudioBackend(AudioOutput::Backend::Null);
        p.setTimeline(timeline_);
        p.play();
        QTest::qWait(200);
        p.simulateAudioDeviceLoss();
        const std::int64_t before = p.currentFrame();
        QTest::qWait(400);
        const std::int64_t after = p.currentFrame();
        QVERIFY2(after >= before + 5,
                 qPrintable(QStringLiteral("%1 -> %2").arg(before).arg(after)));
        QVERIFY(p.usingFallbackClock());
        p.pause();
    }

    // Review Focus 5
    void seekWhilePlayingRepositions() {
        Player p;
        p.setAudioBackend(AudioOutput::Backend::Null);
        p.setTimeline(timeline_);
        p.play();
        QTest::qWait(100);
        p.seek(40);
        QVERIFY(p.isPlaying());
        QVERIFY(p.currentFrame() >= 40);
        QVERIFY(p.currentFrame() < 46);
        p.pause();
        p.seek(10);
        QCOMPARE(p.currentFrame(), 10);
    }
};

QTEST_GUILESS_MAIN(TstPlayer)
#include "tst_player.moc"
