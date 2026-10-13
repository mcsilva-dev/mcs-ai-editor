#include "media/probe.h"
#include "support/synth_media.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class TstProbe : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir dir_;
    QString video_;
    QString audioOnly_;
    QString accented_;
    QString notMedia_;

  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        video_ = dir_.filePath(QStringLiteral("av.mov"));
        QCOMPARE(mcs::test::writeSynthMov(video_), QString());

        audioOnly_ = dir_.filePath(QStringLiteral("audio.mov"));
        mcs::test::SynthSpec audio;
        audio.video = false;
        QCOMPARE(mcs::test::writeSynthMov(audioOnly_, audio), QString());

        // Review Focus 1: pasta e arquivo com espaço e acento.
        const QString sub = dir_.filePath(QStringLiteral("pasta com espaço ção"));
        QVERIFY(QDir().mkpath(sub));
        accented_ = sub + QStringLiteral("/clipe ção.mov");
        QCOMPARE(mcs::test::writeSynthMov(accented_), QString());

        notMedia_ = dir_.filePath(QStringLiteral("texto.mov"));
        QFile txt(notMedia_);
        QVERIFY(txt.open(QIODevice::WriteOnly));
        txt.write("isto nao e um video");
    }

    void readsVideoStream() {
        const auto r = mcs::media::probe(video_);
        QVERIFY2(r.info, qPrintable(r.error));
        QVERIFY(r.info->video);
        const auto& v = *r.info->video;
        QCOMPARE(v.width, 64);
        QCOMPARE(v.height, 48);
        QCOMPARE(v.fps, (mcs::core::Rational{25, 1}));
        QCOMPARE(v.frameCount, 60);
        QCOMPARE(v.codec, QStringLiteral("mpeg4"));
        QCOMPARE(v.pixelFormat, QStringLiteral("yuv420p"));
        QVERIFY(!v.colorRange.isEmpty());
        QVERIFY(qAbs(r.info->durationSeconds - 2.4) < 0.05);
    }

    void readsAudioStream() {
        const auto r = mcs::media::probe(video_);
        QVERIFY(r.info && r.info->audio);
        QCOMPARE(r.info->audio->sampleRate, 48000);
        QCOMPARE(r.info->audio->channels, 2);
        QCOMPARE(r.info->audio->codec, QStringLiteral("pcm_s16le"));
    }

    // Review Focus 5
    void audioOnlyHasNoVideo() {
        const auto r = mcs::media::probe(audioOnly_);
        QVERIFY2(r.info, qPrintable(r.error));
        QVERIFY(!r.info->video);
        QVERIFY(r.info->audio);
    }

    // Review Focus 1
    void accentedPathWorks() {
        const auto r = mcs::media::probe(accented_);
        QVERIFY2(r.info, qPrintable(r.error));
        QCOMPARE(r.info->path, accented_);
    }

    // Review Focus 2
    void notMediaReportsError() {
        const auto r = mcs::media::probe(notMedia_);
        QVERIFY(!r.info);
        QVERIFY(r.error.contains(QStringLiteral("texto.mov")));
    }

    void missingFileReportsError() {
        const auto r = mcs::media::probe(dir_.filePath(QStringLiteral("nao_existe.mov")));
        QVERIFY(!r.info);
        QVERIFY(!r.error.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstProbe)
#include "tst_probe.moc"
