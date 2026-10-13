#include "media/frame_decoder.h"

#include "media/ffmpeg_util.h"

#include <QCoreApplication>
#include <QFileInfo>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace mcs::media {

struct FrameDecoder::Impl {
    AVFormatContext* fmt = nullptr;
    AVCodecContext* dec = nullptr;
    SwsContext* sws = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    AVStream* st = nullptr;
    int streamIndex = -1;
    core::Rational fps;
    std::int64_t count = 0;
    std::int64_t startPts = 0;
    std::int64_t lastIndex = -1; // último quadro entregue (-1: próximo pedido faz seek)
    bool drained = false;        // fim do arquivo lido: decoder esvaziando (ainda solta quadros)
    bool exhausted = false;      // decoder respondeu EOF: não há mais quadros sem seek
    std::int64_t seeks = 0;

    ~Impl() {
        sws_freeContext(sws);
        av_frame_free(&frame);
        av_packet_free(&pkt);
        avcodec_free_context(&dec);
        avformat_close_input(&fmt);
    }

    AVRational frameTb() const {
        return AVRational{static_cast<int>(fps.den), static_cast<int>(fps.num)};
    }

    QImage toImage(const AVFrame* f) {
        sws = sws_getCachedContext(sws, f->width, f->height, static_cast<AVPixelFormat>(f->format),
                                   f->width, f->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr,
                                   nullptr, nullptr);
        if (!sws) {
            return {};
        }
        QImage img(f->width, f->height, QImage::Format_RGBA8888);
        uint8_t* dst[4] = {img.bits(), nullptr, nullptr, nullptr};
        const int dstStride[4] = {static_cast<int>(img.bytesPerLine()), 0, 0, 0};
        sws_scale(sws, f->data, f->linesize, 0, f->height, dst, dstStride);
        return img;
    }
};

FrameDecoder::FrameDecoder() = default;
FrameDecoder::~FrameDecoder() = default;

bool FrameDecoder::open(const QString& path, QString* error) {
    d_ = std::make_unique<Impl>();
    auto fail = [&](const char* text) {
        if (error) {
            *error =
                QCoreApplication::translate("mcs::media", text).arg(QFileInfo(path).fileName());
        }
        d_.reset();
        return false;
    };
    const QByteArray utf8 = path.toUtf8();
    if (avformat_open_input(&d_->fmt, utf8.constData(), nullptr, nullptr) < 0) {
        return fail("Cannot open \"%1\" as media.");
    }
    if (avformat_find_stream_info(d_->fmt, nullptr) < 0) {
        return fail("\"%1\" has no readable streams.");
    }
    const AVCodec* codec = nullptr;
    d_->streamIndex = detail::videoStreamOf(d_->fmt, &codec);
    if (d_->streamIndex < 0 || !codec) {
        return fail("\"%1\" has no video.");
    }
    d_->st = d_->fmt->streams[d_->streamIndex];
    d_->dec = avcodec_alloc_context3(codec);
    if (!d_->dec || avcodec_parameters_to_context(d_->dec, d_->st->codecpar) < 0) {
        return fail("Cannot prepare the decoder for \"%1\".");
    }
    d_->dec->thread_count = 0; // automático
    if (avcodec_open2(d_->dec, codec, nullptr) < 0) {
        return fail("Cannot open the decoder for \"%1\".");
    }
    d_->fps = detail::frameRateOf(d_->fmt, d_->st);
    if (d_->fps.num <= 0) {
        return fail("\"%1\" has no valid frame rate.");
    }
    d_->count = detail::frameCountOf(d_->fmt, d_->st, d_->fps);
    d_->startPts = d_->st->start_time != AV_NOPTS_VALUE ? d_->st->start_time : 0;
    d_->frame = av_frame_alloc();
    d_->pkt = av_packet_alloc();
    return true;
}

bool FrameDecoder::isOpen() const {
    return d_ != nullptr;
}

std::int64_t FrameDecoder::frameCount() const {
    return d_ ? d_->count : 0;
}

core::Rational FrameDecoder::fps() const {
    return d_ ? d_->fps : core::Rational{};
}

std::int64_t FrameDecoder::seekCount() const {
    return d_ ? d_->seeks : 0;
}

QSize FrameDecoder::size() const {
    return d_ ? QSize(d_->st->codecpar->width, d_->st->codecpar->height) : QSize();
}

std::optional<DecodedFrame> FrameDecoder::frameAt(std::int64_t index) {
    if (!d_ || index < 0 || index >= d_->count) {
        return std::nullopt;
    }
    Impl& s = *d_;
    // Avanço curto a partir do último quadro: continua decodificando, sem seek.
    constexpr std::int64_t kForwardWindow = 30;
    const bool sequential = s.lastIndex >= 0 && !s.exhausted && index > s.lastIndex &&
                            index <= s.lastIndex + kForwardWindow;
    if (!sequential) {
        const std::int64_t target = s.startPts + av_rescale_q(index, s.frameTb(), s.st->time_base);
        if (av_seek_frame(s.fmt, s.streamIndex, target, AVSEEK_FLAG_BACKWARD) < 0) {
            s.lastIndex = -1;
            return std::nullopt;
        }
        avcodec_flush_buffers(s.dec);
        s.drained = false;
        s.exhausted = false;
        ++s.seeks;
    }
    s.lastIndex = -1; // só volta a valer quando um quadro for entregue

    for (;;) {
        const int r = avcodec_receive_frame(s.dec, s.frame);
        if (r == 0) {
            std::int64_t pts = s.frame->best_effort_timestamp;
            if (pts == AV_NOPTS_VALUE) {
                pts = s.frame->pts;
            }
            const std::int64_t got =
                av_rescale_q_rnd(pts - s.startPts, s.st->time_base, s.frameTb(),
                                 static_cast<AVRounding>(AV_ROUND_NEAR_INF));
            if (got >= index) {
                DecodedFrame out{got, s.toImage(s.frame)};
                av_frame_unref(s.frame);
                if (out.image.isNull()) {
                    return std::nullopt;
                }
                s.lastIndex = got;
                return out;
            }
            av_frame_unref(s.frame);
            continue;
        }
        // EOF, erro, ou decoder já esvaziado sem chegar ao quadro: não há quadro.
        if (r == AVERROR_EOF) {
            s.exhausted = true;
        }
        if (r != AVERROR(EAGAIN) || s.drained) {
            return std::nullopt;
        }
        if (av_read_frame(s.fmt, s.pkt) < 0) {
            avcodec_send_packet(s.dec, nullptr); // esvazia o decoder
            s.drained = true;
            continue;
        }
        if (s.pkt->stream_index == s.streamIndex) {
            avcodec_send_packet(s.dec, s.pkt);
        }
        av_packet_unref(s.pkt);
    }
}

} // namespace mcs::media
