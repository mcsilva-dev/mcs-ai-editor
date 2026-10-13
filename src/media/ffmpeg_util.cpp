#include "media/ffmpeg_util.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace mcs::media::detail {

core::Rational frameRateOf(AVFormatContext* fmt, AVStream* st) {
    const AVRational r = av_guess_frame_rate(fmt, st, nullptr);
    return core::normalized({r.num, r.den});
}

std::int64_t frameCountOf(AVFormatContext* fmt, AVStream* st, core::Rational fps) {
    if (st->nb_frames > 0) {
        return st->nb_frames;
    }
    if (fps.num <= 0) {
        return 0;
    }
    const AVRational frameTb{static_cast<int>(fps.den), static_cast<int>(fps.num)};
    if (st->duration != AV_NOPTS_VALUE) {
        return av_rescale_q(st->duration, st->time_base, frameTb);
    }
    if (fmt->duration != AV_NOPTS_VALUE) {
        return av_rescale_q(fmt->duration, AVRational{1, AV_TIME_BASE}, frameTb);
    }
    return 0;
}

QString nameOr(const char* name) {
    return name ? QString::fromUtf8(name) : QStringLiteral("unknown");
}

} // namespace mcs::media::detail
