#include "support/synth_media.h"
#include "ui/media_pool_model.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class TstMediaPool : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir dir_;
    QString a_;
    QString b_;
    QString audio_;
    QString notMedia_;

  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        a_ = dir_.filePath(QStringLiteral("a.mov"));
        b_ = dir_.filePath(QStringLiteral("b.mov"));
        audio_ = dir_.filePath(QStringLiteral("so_audio.mov"));
        notMedia_ = dir_.filePath(QStringLiteral("texto.mov"));
        QCOMPARE(mcs::test::writeSynthMov(a_), QString());
        QCOMPARE(mcs::test::writeSynthMov(b_), QString());
        mcs::test::SynthSpec s;
        s.video = false;
        QCOMPARE(mcs::test::writeSynthMov(audio_, s), QString());
        QFile txt(notMedia_);
        QVERIFY(txt.open(QIODevice::WriteOnly));
        txt.write("nao e video");
    }

    void importsFilesInOrder() {
        mcs::ui::MediaPoolModel m;
        QVERIFY(m.importFiles({a_, b_}).isEmpty());
        QCOMPARE(m.rowCount(), 2);
        QCOMPARE(m.data(m.index(0), Qt::DisplayRole).toString(), QStringLiteral("a.mov"));
        QCOMPARE(m.data(m.index(1), mcs::ui::MediaPoolModel::PathRole).toString(), b_);
        QVERIFY(m.data(m.index(0), Qt::ToolTipRole).toString().contains(QStringLiteral("64x48")));
    }

    void ignoresDuplicates() {
        mcs::ui::MediaPoolModel m;
        QVERIFY(m.importFiles({a_}).isEmpty());
        QVERIFY(m.importFiles({a_}).isEmpty());
        QCOMPARE(m.rowCount(), 1);
    }

    // Review Focus 2
    void reportsNonMediaAndKeepsPool() {
        mcs::ui::MediaPoolModel m;
        const QStringList errors = m.importFiles({notMedia_, a_});
        QCOMPARE(errors.size(), 1);
        QVERIFY(errors.first().contains(QStringLiteral("texto.mov")));
        QCOMPARE(m.rowCount(), 1);
    }

    void generatesThumbnails() {
        mcs::ui::MediaPoolModel m;
        QSignalSpy spy(&m, &mcs::ui::MediaPoolModel::thumbnailReady);
        QVERIFY(m.importFiles({a_, b_}).isEmpty());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 10000);
        const auto thumb = m.data(m.index(0), Qt::DecorationRole).value<QImage>();
        QVERIFY(!thumb.isNull());
        QVERIFY(thumb.width() <= mcs::ui::kThumbnailSize.width());
        QVERIFY(thumb.height() <= mcs::ui::kThumbnailSize.height());
    }

    // Review Focus 5
    void audioOnlyImportsWithoutThumbnail() {
        mcs::ui::MediaPoolModel m;
        QSignalSpy spy(&m, &mcs::ui::MediaPoolModel::thumbnailReady);
        QVERIFY(m.importFiles({audio_}).isEmpty());
        QCOMPARE(m.rowCount(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 10000);
        QVERIFY(m.data(m.index(0), Qt::DecorationRole).value<QImage>().isNull());
    }
};

QTEST_MAIN(TstMediaPool)
#include "tst_mediapool.moc"
