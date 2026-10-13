#include "media/ffmpeg_util.h"

#include <algorithm>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace mcs::media::detail {

int videoStreamOf(AVFormatContext* fmt, const AVCodec** decoder) {
    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        const AVStream* st = fmt->streams[i];
        if (st->codecpar->codec_type != AVMEDIA_TYPE_VIDEO ||
            (st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            continue;
        }
        if (decoder) {
            *decoder = avcodec_find_decoder(st->codecpar->codec_id);
        }
        return static_cast<int>(i);
    }
    return -1;
}

core::Rational frameRateOf(AVFormatContext* fmt, AVStream* st) {
    const AVRational r = av_guess_frame_rate(fmt, st, nullptr);
    return core::normalized({r.num, r.den});
}

std::int64_t frameCountOf(AVFormatContext* fmt, AVStream* st, core::Rational fps) {
    std::int64_t byDuration = 0;
    if (fps.num > 0) {
        const AVRational frameTb{static_cast<int>(fps.den), static_cast<int>(fps.num)};
        if (st->duration != AV_NOPTS_VALUE) {
            byDuration = av_rescale_q(st->duration, st->time_base, frameTb);
        } else if (fmt->duration != AV_NOPTS_VALUE) {
            byDuration = av_rescale_q(fmt->duration, AVRational{1, AV_TIME_BASE}, frameTb);
        }
    }
    if (st->nb_frames > 0 && byDuration > 0) {
        return std::min(st->nb_frames, byDuration);
    }
    return st->nb_frames > 0 ? st->nb_frames : byDuration;
}

QString nameOr(const char* name) {
    return name ? QString::fromUtf8(name) : QStringLiteral("unknown");
}

} // namespace mcs::media::detail
