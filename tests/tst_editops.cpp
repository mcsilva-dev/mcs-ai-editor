#include "core/edit_ops.h"
#include "support/timeline_text.h"

#include <QtTest>

using mcs::test::formatTimeline;
using mcs::test::idsOf;
using mcs::test::parseTimeline;

namespace {
// "B+b" = ids dos clipes B e b (a seleção chega pronta; as operações não expandem).
QSet<QUuid> select(const mcs::core::Timeline& t, const QString& names) {
    QSet<QUuid> ids;
    for (const QString& name : names.split(QLatin1Char('+'), Qt::SkipEmptyParts)) {
        ids.unite(idsOf(t, name));
    }
    return ids;
}

// expected "" = erro qualquer; "!nome" = erro que menciona `nome`; senão a timeline esperada.
void checkResult(const mcs::core::EditResult& r, const QString& expected) {
    if (expected.isEmpty() || expected.startsWith(QLatin1Char('!'))) {
        QVERIFY(!r.timeline);
        QVERIFY2(r.error.contains(expected.mid(1)), qPrintable(r.error));
        QVERIFY(!r.error.isEmpty());
        return;
    }
    QVERIFY2(r.timeline, qPrintable(r.error));
    QCOMPARE(formatTimeline(*r.timeline), expected);
}
} // namespace

class TstEditOps : public QObject {
    Q_OBJECT
  private slots:
    void blade_data() {
        QTest::addColumn<QString>("before");
        QTest::addColumn<qint64>("frame");
        QTest::addColumn<QString>("expected"); // vazio = erro esperado
        QTest::newRow("meio do clipe ligado") << "V1: A[0..20) | A1: a[0..20)" << qint64(8)
                                              << "V1: A[0..8) A[8..20)@8 | A1: a[0..8) a[8..20)@8";
        QTest::newRow("respeita sourceIn")
            << "V1: A[10..30)@5 | A1: -" << qint64(15) << "V1: A[10..15)@5 A[15..30)@10 | A1: -";
        QTest::newRow("entre clipes") << "V1: A[0..10) B[10..20) | A1: -" << qint64(10) << "";
        QTest::newRow("fora da timeline") << "V1: A[0..10) | A1: -" << qint64(50) << "";
    }

    void blade() {
        QFETCH(QString, before);
        QFETCH(qint64, frame);
        QFETCH(QString, expected);
        checkResult(mcs::core::blade(parseTimeline(before), frame), expected);
    }

    void bladeKeepsRightPartsLinked() {
        const auto r = mcs::core::blade(parseTimeline("V1: A[0..20) | A1: a[0..20)"), 8);
        QVERIFY(r.timeline);
        const auto& v = r.timeline->video[0].clips;
        const auto& a = r.timeline->audio[0].clips;
        QCOMPARE(v[0].link, a[0].link);
        QCOMPARE(v[1].link, a[1].link);
        QVERIFY(v[0].link != v[1].link);
    }

    void lift_data() {
        QTest::addColumn<QString>("before");
        QTest::addColumn<QString>("select");
        QTest::addColumn<QString>("expected");
        QTest::newRow("vídeo e áudio")
            << "V1: A[0..10) B[10..20) C[20..30) | A1: a[0..10) b[10..20) c[20..30)" << "B+b"
            << "V1: A[0..10) C[20..30) | A1: a[0..10) c[20..30)";
        QTest::newRow("só o áudio (Linked Selection desligada)")
            << "V1: A[0..10) B[10..20) | A1: a[0..10) b[10..20)" << "b"
            << "V1: A[0..10) B[10..20) | A1: a[0..10)";
        QTest::newRow("sem seleção") << "V1: A[0..10) | A1: -" << "" << "";
    }

    void lift() {
        QFETCH(QString, before);
        QFETCH(QString, select);
        QFETCH(QString, expected);
        const auto t = parseTimeline(before);
        checkResult(mcs::core::lift(t, ::select(t, select)), expected);
    }

    void rippleDelete_data() {
        QTest::addColumn<QString>("before");
        QTest::addColumn<QString>("select");
        QTest::addColumn<QString>("expected");
        QTest::newRow("fecha a lacuna nas duas trilhas")
            << "V1: A[0..10) B[10..20) C[20..30) | A1: a[0..10) b[10..20) c[20..30)" << "B+b"
            << "V1: A[0..10) C[10..20) | A1: a[0..10) c[10..20)";
        QTest::newRow("lacuna antes também anda")
            << "V1: A[0..10) B[10..20) C[25..30) | A1: -" << "B"
            << "V1: A[0..10) C[15..20) | A1: -";
        // Review Focus 1: outra trilha ocupada no intervalo.
        QTest::newRow("bloqueado") << "V1: A[0..10) B[10..20) | A1: x[5..25)" << "B" << "!x";
        QTest::newRow("só o vídeo com o áudio ligado no lugar")
            << "V1: A[0..10) B[10..20) | A1: a[0..10) b[10..20)" << "B" << "!b";
        QTest::newRow("sem seleção") << "V1: A[0..10) | A1: -" << "" << "";
    }

    void rippleDelete() {
        QFETCH(QString, before);
        QFETCH(QString, select);
        QFETCH(QString, expected);
        const auto t = parseTimeline(before);
        checkResult(mcs::core::rippleDelete(t, ::select(t, select)), expected);
    }

    void overwrite_data() {
        QTest::addColumn<QString>("before");
        QTest::addColumn<qint64>("at");
        QTest::addColumn<QString>("expected");
        QTest::newRow("divide o clipe de baixo")
            << "V1: A[0..20) | A1: a[0..20)" << qint64(7)
            << "V1: A[0..7) S[7..13) A[13..20)@13 | A1: a[0..7) S[7..13) a[13..20)@13";
        QTest::newRow("cobre clipe inteiro") << "V1: A[5..8) B[8..30) | A1: -" << qint64(4)
                                             << "V1: S[4..10) B[10..30)@2 | A1: S[4..10)";
    }

    void overwrite() {
        QFETCH(QString, before);
        QFETCH(qint64, at);
        QFETCH(QString, expected);
        const mcs::core::SourceRange src{QStringLiteral("S"), {25, 1}, 0, 6, true, true};
        const auto r = mcs::core::overwrite(parseTimeline(before), src, at);
        QVERIFY2(r.timeline, qPrintable(r.error));
        QCOMPARE(formatTimeline(*r.timeline), expected);
    }

    void insert_data() {
        QTest::addColumn<QString>("before");
        QTest::addColumn<qint64>("at");
        QTest::addColumn<QString>("expected");
        QTest::newRow("empurra todas as trilhas")
            << "V1: A[0..20) | A1: a[0..20)" << qint64(7)
            << "V1: A[0..7) S[7..13) A[13..26)@7 | A1: a[0..7) S[7..13) a[13..26)@7";
        QTest::newRow("no fim") << "V1: A[0..10) | A1: -" << qint64(10)
                                << "V1: A[0..10) S[10..16) | A1: S[10..16)";
    }

    void insert() {
        QFETCH(QString, before);
        QFETCH(qint64, at);
        QFETCH(QString, expected);
        const mcs::core::SourceRange src{QStringLiteral("S"), {25, 1}, 0, 6, true, true};
        const auto r = mcs::core::insert(parseTimeline(before), src, at);
        QVERIFY2(r.timeline, qPrintable(r.error));
        QCOMPARE(formatTimeline(*r.timeline), expected);
    }

    void sourceRangeUsesInPointAndFps() {
        // 10 quadros a 50 fps (in 4..14) = 5 quadros na timeline de 25; sourceIn = 4.
        const mcs::core::SourceRange src{QStringLiteral("S"), {50, 1}, 4, 14, true, false};
        const auto r = mcs::core::overwrite(parseTimeline("V1: - | A1: -"), src, 0);
        QVERIFY(r.timeline);
        QCOMPARE(formatTimeline(*r.timeline), QStringLiteral("V1: S[0..5)@4 | A1: -"));
    }

    void invalidSourceRangeIsRejected() {
        const mcs::core::SourceRange empty{QStringLiteral("S"), {25, 1}, 6, 6, true, true};
        QVERIFY(!mcs::core::insert(parseTimeline("V1: - | A1: -"), empty, 0).timeline);
        QVERIFY(!mcs::core::overwrite(parseTimeline("V1: - | A1: -"), empty, 0).timeline);
    }

    void move_data() {
        QTest::addColumn<QString>("before");
        QTest::addColumn<QString>("select");
        QTest::addColumn<qint64>("delta");
        QTest::addColumn<QString>("expected");
        QTest::newRow("por cima do vizinho")
            << "V1: A[0..10) B[10..20) C[20..30) | A1: a[0..10) b[10..20) c[20..30)" << "A+a"
            << qint64(25)
            << "V1: B[10..20) C[20..25) A[25..35) | A1: b[10..20) c[20..25) a[25..35)";
        // Review Focus 2: nunca antes do quadro 0.
        QTest::newRow("limitado ao zero") << "V1: A[0..10) B[10..20) C[20..30) | A1: -" << "B"
                                          << qint64(-50) << "V1: B[0..10) C[20..30) | A1: -";
    }

    void move() {
        QFETCH(QString, before);
        QFETCH(QString, select);
        QFETCH(qint64, delta);
        QFETCH(QString, expected);
        const auto t = parseTimeline(before);
        const auto r = mcs::core::moveClips(t, ::select(t, select), delta);
        QVERIFY2(r.timeline, qPrintable(r.error));
        QCOMPARE(formatTimeline(*r.timeline), expected);
    }

    void snapToNeighbourEdge() {
        const auto t = parseTimeline("V1: A[0..10) B[20..30) | A1: -");
        const auto a = idsOf(t, QStringLiteral("A"));
        QCOMPARE(mcs::core::snapDelta(t, a, 8, 200, 3), 10); // fim de A encosta em B (20)
        QCOMPARE(mcs::core::snapDelta(t, a, 8, 17, 3), 7);   // playhead (17) está mais perto
        QCOMPARE(mcs::core::snapDelta(t, a, 5, 200, 3), 5);  // nada perto: fica como está
    }

    // Pedido do Miguel: trocar o áudio do vídeo por outro (arquivo só de áudio, F10).
    void audioOnlyOverwriteReplacesOnlyA1() {
        const mcs::core::SourceRange music{QStringLiteral("M"), {25, 1}, 0, 10, false, true};
        const auto r = mcs::core::overwrite(parseTimeline("V1: A[0..20) | A1: a[0..20)"), music, 5);
        QVERIFY2(r.timeline, qPrintable(r.error));
        QCOMPARE(formatTimeline(*r.timeline),
                 QStringLiteral("V1: A[0..20) | A1: a[0..5) M[5..15) a[15..20)@15"));
    }

    void adjustGainOnlyTouchesAudioWithinRange() {
        const auto t = parseTimeline("V1: A[0..10) | A1: a[0..10)");
        const QSet<QUuid> both = select(t, QStringLiteral("A+a"));
        auto r = mcs::core::adjustGain(t, both, -6.0);
        QVERIFY2(r.timeline, qPrintable(r.error));
        QCOMPARE(r.timeline->audio[0].clips[0].gainDb, -6.0);
        QCOMPARE(r.timeline->video[0].clips[0].gainDb, 0.0);
        r = mcs::core::adjustGain(*r.timeline, both, -100.0);
        QCOMPARE(r.timeline->audio[0].clips[0].gainDb, mcs::core::kMinGainDb);
        r = mcs::core::adjustGain(*r.timeline, both, +500.0);
        QCOMPARE(r.timeline->audio[0].clips[0].gainDb, mcs::core::kMaxGainDb);
        QVERIFY(!mcs::core::adjustGain(t, select(t, QStringLiteral("A")), -1.0).timeline);
    }

    void withLinkedExpandsSelection() {
        const auto t = parseTimeline("V1: A[0..10) | A1: a[0..10) z[10..20)");
        const auto ids = mcs::core::withLinked(t, idsOf(t, QStringLiteral("A")));
        QCOMPARE(ids.size(), 2);
        QVERIFY(ids.contains(*idsOf(t, QStringLiteral("a")).begin()));
    }
};

QTEST_GUILESS_MAIN(TstEditOps)
#include "tst_editops.moc"
