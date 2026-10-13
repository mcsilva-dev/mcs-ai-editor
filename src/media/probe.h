#pragma once

#include "core/media_info.h"

#include <QString>
#include <optional>

namespace mcs::media {

struct ProbeResult {
    std::optional<core::MediaInfo> info; // vazio em erro
    QString error;                       // mensagem com o nome do arquivo
};

// Lê só cabeçalhos/metadados (rápido). Caminho vai ao FFmpeg em UTF-8.
ProbeResult probe(const QString& path);

} // namespace mcs::media
