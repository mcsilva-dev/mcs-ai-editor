#pragma once

#include "core/timeline.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <optional>

namespace mcs::core {

inline constexpr int kProjectSchemaVersion = 1;

// Projeto salvo em .mcsproj (JSON): mídias importadas (caminhos absolutos) e a timeline.
struct Project {
    QStringList media;
    Timeline timeline;
};

struct ProjectLoad {
    std::optional<Project> project;
    QString error;
};

QByteArray projectToJson(const Project& project);
ProjectLoad projectFromJson(const QByteArray& json);
// Grava de forma atômica (QSaveFile: ou o arquivo novo inteiro, ou o antigo intacto) e guarda
// até `backups` versões anteriores (.bak1 = a mais recente).
bool saveProject(const QString& path, const Project& project, QString* error = nullptr,
                 int backups = 3);
ProjectLoad loadProject(const QString& path);

} // namespace mcs::core
