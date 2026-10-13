#include "media/probe.h"

#include "media/ffmpeg_util.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QScopeGuard>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
}

namespace mcs::media {

namespace {
QString msg(const char* text, const QString& path) {
    return QCoreApplication::translate("mcs::media", text).arg(QFileInfo(path).fileName());
}
} // namespace

ProbeResult probe(const QString& path) {
    const QByteArray utf8 = path.toUtf8();
    AVFormatContext* fmt = nullptr;
    if (avformat_open_input(&fmt, utf8.constData(), nullptr, nullptr) < 0) {
        return {std::nullopt, msg("Cannot open \"%1\" as media.", path)};
    }
    auto close = qScopeGuard([&] { avformat_close_input(&fmt); });
    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        return {std::nullopt, msg("\"%1\" has no readable streams.", path)};
    }

    core::MediaInfo info;
    info.path = path;
    if (fmt->duration != AV_NOPTS_VALUE) {
        info.durationSeconds = static_cast<double>(fmt->duration) / AV_TIME_BASE;
    }

    const int v = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (v >= 0) {
        AVStream* st = fmt->streams[v];
        const AVCodecParameters* p = st->codecpar;
        core::VideoStreamInfo vi;
        vi.width = p->width;
        vi.height = p->height;
        vi.fps = detail::frameRateOf(fmt, st);
        vi.frameCount = detail::frameCountOf(fmt, st, vi.fps);
        vi.codec = detail::nameOr(avcodec_get_name(p->codec_id));
        vi.pixelFormat = detail::nameOr(av_get_pix_fmt_name(static_cast<AVPixelFormat>(p->format)));
        vi.colorPrimaries = detail::nameOr(av_color_primaries_name(p->color_primaries));
        vi.colorTransfer = detail::nameOr(av_color_transfer_name(p->color_trc));
        vi.colorMatrix = detail::nameOr(av_color_space_name(p->color_space));
        vi.colorRange = detail::nameOr(av_color_range_name(p->color_range));
        info.video = vi;
    }

    const int a = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (a >= 0) {
        const AVCodecParameters* p = fmt->streams[a]->codecpar;
        core::AudioStreamInfo ai;
        ai.sampleRate = p->sample_rate;
        ai.channels = p->ch_layout.nb_channels;
        ai.codec = detail::nameOr(avcodec_get_name(p->codec_id));
        info.audio = ai;
    }

    if (!info.video && !info.audio) {
        return {std::nullopt, msg("\"%1\" has no audio or video.", path)};
    }
    return {info, {}};
}

} // namespace mcs::media
