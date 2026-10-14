#include "support/timeline_text.h"

#include <QMap>
#include <QRegularExpression>
#include <QStringList>

namespace mcs::test {

core::Timeline parseTimeline(const QString& text) {
    core::Timeline t;
    t.video.clear();
    t.audio.clear();
    QMap<QString, QUuid> links;
    static const QRegularExpression clipRe(
        QStringLiteral(R"(^(\w+)\[(\d+)\.\.(\d+)\)(?:@(\d+))?$)"));
    for (const QString& part : text.split(QLatin1Char('|'))) {
        const QString trimmed = part.trimmed();
        const qsizetype colon = trimmed.indexOf(QLatin1Char(':'));
        const QString header = trimmed.left(colon);
        const bool video = header.startsWith(QLatin1Char('V'));
        const auto index = static_cast<std::size_t>(header.mid(1).toInt() - 1);
        auto& tracks = video ? t.video : t.audio;
        if (tracks.size() <= index) {
            tracks.resize(index + 1);
        }
        const QString body = trimmed.mid(colon + 1).trimmed();
        if (body == QLatin1String("-")) {
            continue;
        }
        for (const QString& token : body.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
            const auto m = clipRe.match(token);
            Q_ASSERT_X(m.hasMatch(), "parseTimeline", qPrintable(token));
            const QString name = m.captured(1);
            QUuid& link = links[name.toLower()];
            if (link.isNull()) {
                link = QUuid::createUuid();
            }
            const std::int64_t start = m.captured(2).toLongLong();
            const std::int64_t end = m.captured(3).toLongLong();
            tracks[index].clips.push_back(core::Clip{QUuid::createUuid(),
                                                     name,
                                                     {25, 1},
                                                     start,
                                                     end - start,
                                                     m.captured(4).toLongLong(),
                                                     link,
                                                     0.0});
        }
    }
    if (t.video.empty()) {
        t.video.resize(1);
    }
    if (t.audio.empty()) {
        t.audio.resize(1);
    }
    return t;
}

QString formatTimeline(const core::Timeline& timeline) {
    QStringList parts;
    auto formatTracks = [&](const std::vector<core::Track>& tracks, QChar kind) {
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            QStringList clips;
            for (const core::Clip& c : tracks[i].clips) {
                QString s = QStringLiteral("%1[%2..%3)").arg(c.path).arg(c.start).arg(c.end());
                if (c.sourceIn != 0) {
                    s += QStringLiteral("@%1").arg(c.sourceIn);
                }
                clips << s;
            }
            parts << QStringLiteral("%1%2: %3")
                         .arg(kind)
                         .arg(i + 1)
                         .arg(clips.isEmpty() ? QStringLiteral("-") : clips.join(QLatin1Char(' ')));
        }
    };
    formatTracks(timeline.video, QLatin1Char('V'));
    formatTracks(timeline.audio, QLatin1Char('A'));
    return parts.join(QStringLiteral(" | "));
}

QSet<QUuid> idsOf(const core::Timeline& timeline, const QString& name) {
    QSet<QUuid> ids;
    for (const auto* tracks : {&timeline.video, &timeline.audio}) {
        for (const core::Track& track : *tracks) {
            for (const core::Clip& c : track.clips) {
                if (c.path == name) {
                    ids.insert(c.id);
                }
            }
        }
    }
    return ids;
}

} // namespace mcs::test
