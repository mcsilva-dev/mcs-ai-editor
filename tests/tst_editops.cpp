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

    void withLinkedExpandsSelection() {
        const auto t = parseTimeline("V1: A[0..10) | A1: a[0..10) z[10..20)");
        const auto ids = mcs::core::withLinked(t, idsOf(t, QStringLiteral("A")));
        QCOMPARE(ids.size(), 2);
        QVERIFY(ids.contains(*idsOf(t, QStringLiteral("a")).begin()));
    }
};

QTEST_GUILESS_MAIN(TstEditOps)
#include "tst_editops.moc"
