#include "core/project_file.h"
#include "support/timeline_text.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using mcs::core::Project;
using mcs::test::formatTimeline;
using mcs::test::parseTimeline;

namespace {
Project sample() {
    Project p;
    p.media = {QStringLiteral("C:/media/a.mov"), QStringLiteral("C:/media/ção b.mov")};
    p.timeline = parseTimeline("V1: A[0..20)@3 B[20..30) | A1: a[0..20)@3");
    p.timeline.fps = {30000, 1001};
    p.timeline.audio[0].clips[0].gainDb = -7.5;
    return p;
}
} // namespace

class TstProjectFile : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir dir_;

  private slots:
    void initTestCase() { QVERIFY(dir_.isValid()); }

    void roundTripPreservesEverything() {
        const Project p = sample();
        const auto loaded = mcs::core::projectFromJson(mcs::core::projectToJson(p));
        QVERIFY2(loaded.project, qPrintable(loaded.error));
        QCOMPARE(loaded.project->media, p.media);
        QCOMPARE(loaded.project->timeline.fps, p.timeline.fps);
        QCOMPARE(formatTimeline(loaded.project->timeline), formatTimeline(p.timeline));
        const auto& before = p.timeline.video[0].clips[0];
        const auto& after = loaded.project->timeline.video[0].clips[0];
        QCOMPARE(after.id, before.id);
        QCOMPARE(after.link, before.link);
        QCOMPARE(after.sourceFps, before.sourceFps);
        QCOMPARE(loaded.project->timeline.audio[0].clips[0].gainDb, -7.5);
    }

    void saveAndLoadFile() {
        const QString path = dir_.filePath(QStringLiteral("projeto ção.mcsproj"));
        QString err;
        QVERIFY2(mcs::core::saveProject(path, sample(), &err), qPrintable(err));
        const auto loaded = mcs::core::loadProject(path);
        QVERIFY2(loaded.project, qPrintable(loaded.error));
        QCOMPARE(formatTimeline(loaded.project->timeline), formatTimeline(sample().timeline));
    }

    void backupsRotate() {
        const QString path = dir_.filePath(QStringLiteral("rot.mcsproj"));
        for (int i = 1; i <= 5; ++i) {
            Project p;
            p.media = {QStringLiteral("v%1").arg(i)};
            QVERIFY(mcs::core::saveProject(path, p, nullptr, 3));
        }
        auto mediaOf = [](const QString& file) {
            return mcs::core::loadProject(file).project->media.value(0);
        };
        QCOMPARE(mediaOf(path), QStringLiteral("v5"));
        QCOMPARE(mediaOf(path + QStringLiteral(".bak1")), QStringLiteral("v4"));
        QCOMPARE(mediaOf(path + QStringLiteral(".bak3")), QStringLiteral("v2"));
        QVERIFY(!QFile::exists(path + QStringLiteral(".bak4")));
    }

    // Review Focus 4
    void corruptFileIsRejected() {
        const auto r = mcs::core::projectFromJson("{ isto não é json");
        QVERIFY(!r.project);
        QVERIFY(!r.error.isEmpty());
        QVERIFY(!mcs::core::projectFromJson("{\"app\":\"x\"}").project); // sem schemaVersion
    }

    // Review Focus 4
    void newerSchemaIsRejected() {
        QByteArray json = mcs::core::projectToJson(sample());
        json.replace("\"schemaVersion\": 1", "\"schemaVersion\": 99");
        const auto r = mcs::core::projectFromJson(json);
        QVERIFY(!r.project);
        QVERIFY(r.error.contains(QStringLiteral("newer")));
    }

    // Revisão final I1: JSON editado à mão com clipe inválido (fps 0 derrubava o playback).
    void invalidClipsAreRejected_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("sourceFps zero") << 0;
        QTest::newRow("duração zero") << 1;
        QTest::newRow("início negativo") << 2;
        QTest::newRow("clipes sobrepostos") << 3;
        QTest::newRow("id repetido") << 4;
    }

    void invalidClipsAreRejected() {
        QFETCH(int, kind);
        Project p = sample();
        auto& clips = p.timeline.video[0].clips;
        switch (kind) {
        case 0:
            clips[0].sourceFps = {0, 1};
            break;
        case 1:
            clips[0].duration = 0;
            break;
        case 2:
            clips[0].start = -5;
            break;
        case 3:
            clips[1].start = 15;
            break;
        default:
            clips[1].id = clips[0].id;
            break;
        }
        const auto r = mcs::core::projectFromJson(mcs::core::projectToJson(p));
        QVERIFY(!r.project);
        QVERIFY(!r.error.isEmpty());
    }

    void gainIsClampedOnLoad() {
        Project p = sample();
        p.timeline.audio[0].clips[0].gainDb = 50.0;
        const auto r = mcs::core::projectFromJson(mcs::core::projectToJson(p));
        QVERIFY2(r.project, qPrintable(r.error));
        QCOMPARE(r.project->timeline.audio[0].clips[0].gainDb, 12.0);
    }

    void missingFileIsAnError() {
        const auto r = mcs::core::loadProject(dir_.filePath(QStringLiteral("nao_existe.mcsproj")));
        QVERIFY(!r.project);
        QVERIFY(r.error.contains(QStringLiteral("nao_existe.mcsproj")));
    }
};

QTEST_GUILESS_MAIN(TstProjectFile)
#include "tst_projectfile.moc"
