#include "media/audio_decoder.h"
#include "support/synth_media.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>
#include <vector>

using mcs::media::AudioDecoder;

class TstAudioDecoder : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir dir_;
    QString av_;

    // Confere `frames` amostras lidas a partir de `pos` contra a rampa do arquivo sintético.
    static void checkRamp(const std::vector<float>& buf, std::int64_t pos, int frames,
                          std::int64_t length) {
        for (int i = 0; i < frames; ++i) {
            const std::int64_t n = pos + i;
            const float l = n < length ? mcs::test::expectedSampleAt(n) : 0.0f;
            QVERIFY2(qAbs(buf[2 * i] - l) < 1e-4f && qAbs(buf[2 * i + 1] + l) < 1e-4f,
                     qPrintable(QStringLiteral("amostra %1: %2/%3, esperado %4")
                                    .arg(n)
                                    .arg(buf[2 * i])
                                    .arg(buf[2 * i + 1])
                                    .arg(l)));
        }
    }

  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        av_ = dir_.filePath(QStringLiteral("av.mov"));
        QCOMPARE(mcs::test::writeSynthMov(av_), QString()); // 60 quadros a 25 fps = 115200 amostras
    }

    void opensAndReportsLength() {
        AudioDecoder d;
        QString err;
        QVERIFY2(d.open(av_, &err), qPrintable(err));
        QCOMPARE(d.length(), 115200);
    }

    void readsFromStart() {
        AudioDecoder d;
        QVERIFY(d.open(av_));
        std::vector<float> buf(2 * 500);
        d.read(0, buf.data(), 500);
        checkRamp(buf, 0, 500, d.length());
    }

    void sequentialReadsAcrossPackets() {
        AudioDecoder d;
        QVERIFY(d.open(av_));
        std::vector<float> buf(2 * 777);
        for (int k = 0; k < 20; ++k) {
            d.read(k * 777, buf.data(), 777);
            checkRamp(buf, k * 777, 777, d.length());
        }
    }

    void seeksForwardAndBack() {
        AudioDecoder d;
        QVERIFY(d.open(av_));
        std::vector<float> buf(2 * 1000);
        d.read(90000, buf.data(), 1000);
        checkRamp(buf, 90000, 1000, d.length());
        d.read(5000, buf.data(), 1000);
        checkRamp(buf, 5000, 1000, d.length());
    }

    void pastEndIsSilence() {
        AudioDecoder d;
        QVERIFY(d.open(av_));
        std::vector<float> buf(2 * 400);
        d.read(115000, buf.data(), 400); // 200 com dados, 200 de silêncio
        checkRamp(buf, 115000, 400, d.length());
    }

    void negativePositionIsSilenceThenData() {
        AudioDecoder d;
        QVERIFY(d.open(av_));
        std::vector<float> buf(2 * 300);
        d.read(-100, buf.data(), 300);
        for (int i = 0; i < 100; ++i) {
            QCOMPARE(buf[2 * i], 0.0f);
        }
        for (int i = 100; i < 300; ++i) {
            QVERIFY(qAbs(buf[2 * i] - mcs::test::expectedSampleAt(i - 100)) < 1e-4f);
        }
    }

    void videoOnlyCannotOpen() {
        const QString path = dir_.filePath(QStringLiteral("so_video.mov"));
        mcs::test::SynthSpec s;
        s.audio = false;
        QCOMPARE(mcs::test::writeSynthMov(path, s), QString());
        AudioDecoder d;
        QString err;
        QVERIFY(!d.open(path, &err));
        QVERIFY(!err.isEmpty());
        std::vector<float> buf(2 * 10, 1.0f);
        d.read(0, buf.data(), 10); // fechado: só zeros, sem crash
        QCOMPARE(buf[0], 0.0f);
    }

    // Áudio que começa 100 ms depois do vídeo: a amostra 0 é alinhada ao quadro 0 do vídeo.
    void audioStartingAfterVideoIsDelayed() {
        const QString path = dir_.filePath(QStringLiteral("audio_atrasado.mov"));
        mcs::test::SynthSpec s;
        s.audioDelaySamples = 4800;
        QCOMPARE(mcs::test::writeSynthMov(path, s), QString());
        AudioDecoder d;
        QVERIFY(d.open(path));
        std::vector<float> buf(2 * 6000);
        d.read(0, buf.data(), 6000);
        for (int i = 0; i < 4800; i += 50) {
            QVERIFY2(qAbs(buf[2 * i]) < 1e-4f, qPrintable(QStringLiteral("amostra %1").arg(i)));
        }
        for (int i = 4800; i < 6000; ++i) {
            QVERIFY2(qAbs(buf[2 * i] - mcs::test::expectedSampleAt(i - 4800)) < 1e-4f,
                     qPrintable(QStringLiteral("amostra %1").arg(i)));
        }
    }

    void accentedPath() {
        const QString sub = dir_.filePath(QStringLiteral("pasta com espaço ção"));
        QVERIFY(QDir().mkpath(sub));
        const QString path = sub + QStringLiteral("/áudio ção.mov");
        QCOMPARE(mcs::test::writeSynthMov(path), QString());
        AudioDecoder d;
        QVERIFY(d.open(path));
        std::vector<float> buf(2 * 100);
        d.read(1234, buf.data(), 100);
        checkRamp(buf, 1234, 100, d.length());
    }
};

QTEST_GUILESS_MAIN(TstAudioDecoder)
#include "tst_audiodecoder.moc"
