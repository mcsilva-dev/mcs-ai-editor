#pragma once

#include "core/rational.h"

#include <QString>
#include <cstdint>
#include <optional>

namespace mcs::core {

struct VideoStreamInfo {
    int width = 0;
    int height = 0;
    Rational fps;
    std::int64_t frameCount = 0;
    QString codec;
    QString pixelFormat;
    // Metadados de cor guardados desde o import (usados pelo Subprojeto Color).
    QString colorPrimaries;
    QString colorTransfer;
    QString colorMatrix;
    QString colorRange;
};

struct AudioStreamInfo {
    int sampleRate = 0;
    int channels = 0;
    QString codec;
};

struct MediaInfo {
    QString path;
    double durationSeconds = 0.0; // só exibição; contas de quadro usam video->frameCount
    std::optional<VideoStreamInfo> video;
    std::optional<AudioStreamInfo> audio;
};

} // namespace mcs::core
