#include "core/project_file.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace mcs::core {

namespace {

QString tr(const char* text) {
    return QCoreApplication::translate("mcs::core", text);
}

QJsonArray rationalToJson(Rational r) {
    return QJsonArray{static_cast<qint64>(r.num), static_cast<qint64>(r.den)};
}

Rational rationalFromJson(const QJsonValue& value) {
    const QJsonArray a = value.toArray();
    return normalized({a.at(0).toInteger(), a.at(1).toInteger(1)});
}

QJsonObject clipToJson(const Clip& c) {
    return QJsonObject{{QStringLiteral("id"), c.id.toString(QUuid::WithoutBraces)},
                       {QStringLiteral("path"), c.path},
                       {QStringLiteral("sourceFps"), rationalToJson(c.sourceFps)},
                       {QStringLiteral("start"), static_cast<qint64>(c.start)},
                       {QStringLiteral("duration"), static_cast<qint64>(c.duration)},
                       {QStringLiteral("sourceIn"), static_cast<qint64>(c.sourceIn)},
                       {QStringLiteral("link"), c.link.toString(QUuid::WithoutBraces)},
                       {QStringLiteral("gainDb"), c.gainDb}};
}

Clip clipFromJson(const QJsonObject& o) {
    return Clip{QUuid::fromString(o.value(QStringLiteral("id")).toString()),
                o.value(QStringLiteral("path")).toString(),
                rationalFromJson(o.value(QStringLiteral("sourceFps"))),
                o.value(QStringLiteral("start")).toInteger(),
                o.value(QStringLiteral("duration")).toInteger(),
                o.value(QStringLiteral("sourceIn")).toInteger(),
                QUuid::fromString(o.value(QStringLiteral("link")).toString()),
                o.value(QStringLiteral("gainDb")).toDouble(0.0)};
}

QJsonArray tracksToJson(const std::vector<Track>& tracks) {
    QJsonArray out;
    for (const Track& t : tracks) {
        QJsonArray clips;
        for (const Clip& c : t.clips) {
            clips.append(clipToJson(c));
        }
        out.append(clips);
    }
    return out;
}

std::vector<Track> tracksFromJson(const QJsonValue& value) {
    std::vector<Track> tracks;
    for (const QJsonValue& track : value.toArray()) {
        Track t;
        for (const QJsonValue& clip : track.toArray()) {
            t.clips.push_back(clipFromJson(clip.toObject()));
        }
        tracks.push_back(std::move(t));
    }
    if (tracks.empty()) {
        tracks.resize(1);
    }
    return tracks;
}

QString backupPath(const QString& path, int n) {
    return path + QStringLiteral(".bak%1").arg(n);
}

} // namespace

QByteArray projectToJson(const Project& project) {
    const QJsonObject timeline{{QStringLiteral("fps"), rationalToJson(project.timeline.fps)},
                               {QStringLiteral("video"), tracksToJson(project.timeline.video)},
                               {QStringLiteral("audio"), tracksToJson(project.timeline.audio)}};
    const QJsonObject root{{QStringLiteral("app"), QStringLiteral("MCS AI Editor")},
                           {QStringLiteral("schemaVersion"), kProjectSchemaVersion},
                           {QStringLiteral("media"), QJsonArray::fromStringList(project.media)},
                           {QStringLiteral("timeline"), timeline}};
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

ProjectLoad projectFromJson(const QByteArray& json) {
    QJsonParseError parse{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parse);
    if (parse.error != QJsonParseError::NoError || !doc.isObject()) {
        return {std::nullopt,
                tr("This is not a valid project file (%1).").arg(parse.errorString())};
    }
    const QJsonObject root = doc.object();
    const int version = root.value(QStringLiteral("schemaVersion")).toInt(0);
    if (version < 1 || !root.value(QStringLiteral("timeline")).isObject()) {
        return {std::nullopt, tr("This is not a valid project file.")};
    }
    if (version > kProjectSchemaVersion) {
        return {std::nullopt, tr("This project was saved by a newer version of MCS AI Editor.")};
    }
    const QJsonObject timeline = root.value(QStringLiteral("timeline")).toObject();
    Project p;
    for (const QJsonValue& m : root.value(QStringLiteral("media")).toArray()) {
        p.media << m.toString();
    }
    p.timeline.fps = rationalFromJson(timeline.value(QStringLiteral("fps")));
    if (p.timeline.fps.num <= 0) {
        return {std::nullopt, tr("This is not a valid project file (frame rate).")};
    }
    p.timeline.video = tracksFromJson(timeline.value(QStringLiteral("video")));
    p.timeline.audio = tracksFromJson(timeline.value(QStringLiteral("audio")));
    return {p, {}};
}

bool saveProject(const QString& path, const Project& project, QString* error, int backups) {
    if (backups > 0 && QFileInfo::exists(path)) {
        QFile::remove(backupPath(path, backups));
        for (int i = backups; i > 1; --i) {
            QFile::rename(backupPath(path, i - 1), backupPath(path, i));
        }
        QFile::copy(path, backupPath(path, 1));
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(projectToJson(project)) < 0 ||
        !file.commit()) {
        if (error) {
            *error =
                tr("Cannot save \"%1\": %2").arg(QFileInfo(path).fileName(), file.errorString());
        }
        return false;
    }
    return true;
}

ProjectLoad loadProject(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {std::nullopt, tr("Cannot open \"%1\".").arg(QFileInfo(path).fileName())};
    }
    return projectFromJson(file.readAll());
}

} // namespace mcs::core
