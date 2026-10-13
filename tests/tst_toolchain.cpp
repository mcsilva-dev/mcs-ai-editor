#include "media/ffmpeg_info.h"

#include <QtTest>

class TstToolchain : public QObject {
    Q_OBJECT
  private slots:
    void qtVersionIsPinned() {
        QCOMPARE(QString::fromLatin1(qVersion()), QStringLiteral("6.11.3"));
    }

    void ffmpegIsLinked() { QVERIFY(!mcs::media::ffmpegVersion().isEmpty()); }

    void h264DecoderAvailable() { QVERIFY(mcs::media::hasDecoder("h264")); }

    void unknownDecoderIsFalse() { QVERIFY(!mcs::media::hasDecoder("decoder_que_nao_existe")); }

    // Review Focus 1: sem /utf-8 o MSVC corromperia esta string.
    void utf8SourceEncoding() {
        const QString s = QStringLiteral("Edição");
        QCOMPARE(s.size(), 6);
        QCOMPARE(s.at(4), QChar(0x00E3));
    }
};

QTEST_MAIN(TstToolchain)
#include "tst_toolchain.moc"
