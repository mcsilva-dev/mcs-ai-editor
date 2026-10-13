#pragma once

#include "core/rational.h"

#include <QString>
#include <cstdint>

struct AVFormatContext;
struct AVStream;

// Utilidades internas de mcs_media (não incluir fora de src/media).
namespace mcs::media::detail {

core::Rational frameRateOf(AVFormatContext* fmt, AVStream* st);

// nb_frames do stream; senão duração do stream; senão duração do contêiner.
std::int64_t frameCountOf(AVFormatContext* fmt, AVStream* st, core::Rational fps);

// nullptr → "unknown".
QString nameOr(const char* name);

} // namespace mcs::media::detail
