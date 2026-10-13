#include "media/frame_decoder.h"
#include "support/synth_media.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

namespace {
int centerGray(const QImage& img) {
    return img.pixelColor(img.width() / 2, img.height() / 2).red();
}
} // namespace

class TstDecoder : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir dir_;
    QString video_;

    void checkFrame(mcs::media::FrameDecoder& d, int k) {
        const auto f = d.frameAt(k);
        QVERIFY2(f, qPrintable(QStringLiteral("quadro %1 não decodificado").arg(k)));
        QCOMPARE(f->index, k);
        QCOMPARE(f->image.format(), QImage::Format_RGBA8888);
        QCOMPARE(f->image.size(), QSize(64, 48));
        QVERIFY2(qAbs(centerGray(f->image) - mcs::test::expectedGrayForFrame(k)) <= 4,
                 qPrintable(QStringLiteral("quadro %1: cinza %2, esperado %3")
                                .arg(k)
                                .arg(centerGray(f->image))
                                .arg(mcs::test::expectedGrayForFrame(k))));
    }

  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        video_ = dir_.filePath(QStringLiteral("av.mov"));
        QCOMPARE(mcs::test::writeSynthMov(video_), QString());
    }

    // Sem isto, um arquivo só de keyframes faria os testes de GOP passarem pelo motivo errado.
    void synthHasAllFramesAndRealGop() {
        const auto stats = mcs::test::videoPacketStats(video_);
        QCOMPARE(stats.packets, 60);
        QCOMPARE(stats.keyframes, 5); // quadros 0, 12, 24, 36, 48
        QCOMPARE(stats.discarded, 0);
    }

    void opensAndReportsCount() {
        mcs::media::FrameDecoder d;
        QString err;
        QVERIFY2(d.open(video_, &err), qPrintable(err));
        QVERIFY(d.isOpen());
        QCOMPARE(d.frameCount(), 60);
        QCOMPARE(d.fps(), (mcs::core::Rational{25, 1}));
        QCOMPARE(d.size(), QSize(64, 48));
    }

    void firstFrame() {
        mcs::media::FrameDecoder d;
        QVERIFY(d.open(video_));
        checkFrame(d, 0);
    }

    // Review Focus 3: meio do GOP (GOP = 12) e quadros após o 1º keyframe.
    void midGopFrames() {
        mcs::media::FrameDecoder d;
        QVERIFY(d.open(video_));
        checkFrame(d, 7);
        checkFrame(d, 13);
        checkFrame(d, 59);
    }

    // Review Focus 3: seek para trás.
    void backwardSeek() {
        mcs::media::FrameDecoder d;
        QVERIFY(d.open(video_));
        checkFrame(d, 40);
        checkFrame(d, 5);
    }

    void outOfRangeIsEmpty() {
        mcs::media::FrameDecoder d;
        QVERIFY(d.open(video_));
        QVERIFY(!d.frameAt(60));
        QVERIFY(!d.frameAt(-1));
    }

    // Review Focus 5
    void audioOnlyCannotOpen() {
        const QString path = dir_.filePath(QStringLiteral("audio.mov"));
        mcs::test::SynthSpec s;
        s.video = false;
        QCOMPARE(mcs::test::writeSynthMov(path, s), QString());
        mcs::media::FrameDecoder d;
        QString err;
        QVERIFY(!d.open(path, &err));
        QVERIFY(!err.isEmpty());
        QVERIFY(!d.isOpen());
        QVERIFY(!d.frameAt(0));
    }

    // Review Focus 5: capa de álbum não abre como vídeo.
    void coverArtIsNotVideo() {
        const QString path = dir_.filePath(QStringLiteral("musica_com_capa.m4a"));
        mcs::test::SynthSpec s;
        s.video = false;
        s.coverArt = true;
        s.container = "mp4";
        QCOMPARE(mcs::test::writeSynthMov(path, s), QString());
        QVERIFY(mcs::test::hasAttachedPicture(path)); // o arquivo reproduz o caso
        mcs::media::FrameDecoder d;
        QVERIFY(!d.open(path));
    }

    // Edit list mais curta que as amostras: a contagem segue a edição, não as amostras.
    void editListLimitsFrameCount() {
        const QString path = dir_.filePath(QStringLiteral("cortado.mov"));
        mcs::test::SynthSpec s;
        s.omitPacketDuration = true;
        QCOMPARE(mcs::test::writeSynthMov(path, s), QString());
        QCOMPARE(mcs::test::videoPacketStats(path).discarded, 1); // o arquivo reproduz o caso
        mcs::media::FrameDecoder d;
        QVERIFY(d.open(path));
        QCOMPARE(d.frameCount(), 59);
        checkFrame(d, 58);
    }

    // Review Focus 3: contêiner sem índice (MPEG-TS) também dá o quadro exato.
    void unindexedContainerSeeksExactly() {
        const QString path = dir_.filePath(QStringLiteral("sem_indice.ts"));
        mcs::test::SynthSpec s;
        s.container = "mpegts";
        s.audio = false;
        QCOMPARE(mcs::test::writeSynthMov(path, s), QString());
        mcs::media::FrameDecoder d;
        QVERIFY(d.open(path));
        checkFrame(d, 40);
        checkFrame(d, 5);
        checkFrame(d, 30);
        checkFrame(d, 7);
    }

    // Avançar quadro a quadro não faz seek (essencial para o playback).
    void sequentialReadsDoNotSeek() {
        mcs::media::FrameDecoder d;
        QVERIFY(d.open(video_));
        for (int k = 0; k < 60; ++k) {
            checkFrame(d, k);
        }
        QCOMPARE(d.seekCount(), 1);
        checkFrame(d, 10); // voltar exige seek
        QCOMPARE(d.seekCount(), 2);
        checkFrame(d, 50); // salto grande para frente também
        QCOMPARE(d.seekCount(), 3);
    }

    // Review Focus 1
    void accentedPathDecodes() {
        const QString sub = dir_.filePath(QStringLiteral("pasta com espaço ção"));
        QVERIFY(QDir().mkpath(sub));
        const QString path = sub + QStringLiteral("/clipe ção.mov");
        QCOMPARE(mcs::test::writeSynthMov(path), QString());
        mcs::media::FrameDecoder d;
        QVERIFY(d.open(path));
        checkFrame(d, 3);
    }
};

QTEST_GUILESS_MAIN(TstDecoder)
#include "tst_decoder.moc"
