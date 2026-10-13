#pragma once

#include "core/rational.h"

#include <QString>
#include <cstdint>

struct AVCodec;
struct AVFormatContext;
struct AVStream;

// Utilidades internas de mcs_media (não incluir fora de src/media).
namespace mcs::media::detail {

// Melhor stream de vídeo ignorando capas (AV_DISPOSITION_ATTACHED_PIC, ex.: MP3/M4A com capa).
// -1 se não houver; `decoder` (opcional) recebe o decoder do stream.
int videoStreamOf(AVFormatContext* fmt, const AVCodec** decoder = nullptr);

core::Rational frameRateOf(AVFormatContext* fmt, AVStream* st);

// Menor entre nb_frames e a duração do stream (edit lists cortam amostras); senão o que houver;
// por fim a duração do contêiner.
std::int64_t frameCountOf(AVFormatContext* fmt, AVStream* st, core::Rational fps);

// nullptr → "unknown".
QString nameOr(const char* name);

} // namespace mcs::media::detail
