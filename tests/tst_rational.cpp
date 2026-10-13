#include "core/rational.h"

#include <QtTest>

using mcs::core::Rational;

class TstRational : public QObject {
    Q_OBJECT
  private slots:
    void normalizesByGcd() { QCOMPARE(mcs::core::normalized({50, 2}), (Rational{25, 1})); }

    void normalizesSignToNumerator() {
        QCOMPARE(mcs::core::normalized({30, -1}), (Rational{-30, 1}));
    }

    void zeroDenominatorBecomesZero() { QCOMPARE(mcs::core::normalized({5, 0}), (Rational{0, 1})); }

    void ntscToDouble() { QVERIFY(qAbs(Rational{30000, 1001}.toDouble() - 29.97003) < 1e-4); }

    void timecodeAt25() {
        QCOMPARE(mcs::core::timecode(0, {25, 1}), QStringLiteral("00:00:00:00"));
        QCOMPARE(mcs::core::timecode(7, {25, 1}), QStringLiteral("00:00:00:07"));
        QCOMPARE(mcs::core::timecode(25 * 3661 + 3, {25, 1}), QStringLiteral("01:01:01:03"));
    }

    // Review Focus 4: 29,97 usa base nominal 30 (non-drop).
    void timecodeAtNtsc() {
        QCOMPARE(mcs::core::timecode(30, {30000, 1001}), QStringLiteral("00:00:01:00"));
        QCOMPARE(mcs::core::timecode(1800, {30000, 1001}), QStringLiteral("00:01:00:00"));
        QCOMPARE(mcs::core::timecode(29, {30000, 1001}), QStringLiteral("00:00:00:29"));
    }

    void timecodeInvalidInputs() {
        QCOMPARE(mcs::core::timecode(5, {0, 1}), QStringLiteral("--:--:--:--"));
        QCOMPARE(mcs::core::timecode(-1, {25, 1}), QStringLiteral("--:--:--:--"));
    }
};

QTEST_GUILESS_MAIN(TstRational)
#include "tst_rational.moc"
