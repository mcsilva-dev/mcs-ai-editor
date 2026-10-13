#pragma once

#include <QString>

namespace mcs::media {

// Versão do FFmpeg linkado (av_version_info), ex.: "9.0.2".
QString ffmpegVersion();

// true se o FFmpeg linkado tem um decoder com esse nome (ex.: "h264").
bool hasDecoder(const char* name);

} // namespace mcs::media
