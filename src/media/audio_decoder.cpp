#include "media/audio_decoder.h"

#include "media/ffmpeg_util.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <algorithm>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

namespace mcs::media {

struct AudioDecoder::Impl {
    AVFormatContext* fmt = nullptr;
    AVCodecContext* dec = nullptr;
    SwrContext* swr = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    AVStream* st = nullptr;
    int streamIndex = -1;
    std::int64_t startPts = 0;
    std::int64_t length = 0;
    std::vector<float> buf;     // amostras decodificadas, intercaladas
    std::int64_t bufStart = -1; // amostra de buf[0]; -1 = desconhecida (após seek)
    bool draining = false;

    ~Impl() {
        swr_free(&swr);
        av_frame_free(&frame);
        av_packet_free(&pkt);
        avcodec_free_context(&dec);
        avformat_close_input(&fmt);
    }

    std::int64_t bufFrames() const { return static_cast<std::int64_t>(buf.size()) / kChannels; }

    void seekTo(std::int64_t pos) {
        const std::int64_t ts =
            startPts + av_rescale_q(pos, AVRational{1, kSampleRate}, st->time_base);
        av_seek_frame(fmt, streamIndex, ts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(dec);
        swr_close(swr);
        swr_init(swr);
        buf.clear();
        bufStart = -1;
        draining = false;
    }

    void append(const AVFrame* f) {
        if (bufStart < 0) {
            std::int64_t pts = f->best_effort_timestamp;
            if (pts == AV_NOPTS_VALUE) {
                pts = f->pts;
            }
            bufStart = pts == AV_NOPTS_VALUE ? 0
                                             : av_rescale_q(pts - startPts, st->time_base,
                                                            AVRational{1, kSampleRate});
        }
        const int capacity = swr_get_out_samples(swr, f->nb_samples);
        if (capacity <= 0) {
            return;
        }
        const std::size_t old = buf.size();
        buf.resize(old + static_cast<std::size_t>(capacity) * kChannels);
        auto* out = reinterpret_cast<uint8_t*>(buf.data() + old);
        const int got = swr_convert(swr, &out, capacity, f->extended_data, f->nb_samples);
        buf.resize(old + static_cast<std::size_t>(std::max(got, 0)) * kChannels);
    }

    // Decodifica mais um bloco em buf. false no fim do stream.
    bool decodeMore() {
        for (;;) {
            const int r = avcodec_receive_frame(dec, frame);
            if (r == 0) {
                append(frame);
                av_frame_unref(frame);
                return true;
            }
            if (r != AVERROR(EAGAIN) || draining) {
                return false;
            }
            if (av_read_frame(fmt, pkt) < 0) {
                avcodec_send_packet(dec, nullptr);
                draining = true;
                continue;
            }
            if (pkt->stream_index == streamIndex) {
                avcodec_send_packet(dec, pkt);
            }
            av_packet_unref(pkt);
        }
    }
};

AudioDecoder::AudioDecoder() = default;
AudioDecoder::~AudioDecoder() = default;

bool AudioDecoder::open(const QString& path, QString* error) {
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
    d_->streamIndex = av_find_best_stream(d_->fmt, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (d_->streamIndex < 0 || !codec) {
        return fail("\"%1\" has no audio.");
    }
    d_->st = d_->fmt->streams[d_->streamIndex];
    d_->dec = avcodec_alloc_context3(codec);
    if (!d_->dec || avcodec_parameters_to_context(d_->dec, d_->st->codecpar) < 0 ||
        avcodec_open2(d_->dec, codec, nullptr) < 0) {
        return fail("Cannot open the audio decoder for \"%1\".");
    }
    if (d_->dec->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
        av_channel_layout_default(&d_->dec->ch_layout, d_->dec->ch_layout.nb_channels);
    }
    AVChannelLayout stereo;
    av_channel_layout_default(&stereo, kChannels);
    if (swr_alloc_set_opts2(&d_->swr, &stereo, AV_SAMPLE_FMT_FLT, kSampleRate, &d_->dec->ch_layout,
                            d_->dec->sample_fmt, d_->dec->sample_rate, 0, nullptr) < 0 ||
        swr_init(d_->swr) < 0) {
        return fail("Cannot convert the audio of \"%1\".");
    }
    d_->startPts = d_->st->start_time != AV_NOPTS_VALUE ? d_->st->start_time : 0;
    // Com vídeo no arquivo, a amostra 0 é o início do vídeo: áudio que começa depois (ou antes)
    // mantém o deslocamento e o lip-sync fica certo.
    const int video = detail::videoStreamOf(d_->fmt);
    if (video >= 0 && d_->fmt->streams[video]->start_time != AV_NOPTS_VALUE) {
        const AVStream* vs = d_->fmt->streams[video];
        d_->startPts = av_rescale_q(vs->start_time, vs->time_base, d_->st->time_base);
    }
    if (d_->st->duration != AV_NOPTS_VALUE) {
        d_->length = av_rescale_q(d_->st->duration, d_->st->time_base, AVRational{1, kSampleRate});
    } else if (d_->fmt->duration != AV_NOPTS_VALUE) {
        d_->length = av_rescale_q(d_->fmt->duration, AVRational{1, AV_TIME_BASE},
                                  AVRational{1, kSampleRate});
    }
    d_->frame = av_frame_alloc();
    d_->pkt = av_packet_alloc();
    return true;
}

bool AudioDecoder::isOpen() const {
    return d_ != nullptr;
}

std::int64_t AudioDecoder::length() const {
    return d_ ? d_->length : 0;
}

void AudioDecoder::read(std::int64_t position, float* out, int frames) {
    std::fill(out, out + static_cast<std::ptrdiff_t>(frames) * kChannels, 0.0f);
    if (!d_ || frames <= 0) {
        return;
    }
    Impl& s = *d_;
    // Parte negativa: silêncio.
    int written = static_cast<int>(std::clamp<std::int64_t>(-position, 0, frames));
    const std::int64_t first = position + written;
    // Até 1 s à frente do que já está decodificado: continua em sequência; senão, seek.
    const bool reachable =
        s.bufStart >= 0 && first >= s.bufStart && first <= s.bufStart + s.bufFrames() + kSampleRate;
    if (!reachable && written < frames) {
        s.seekTo(first);
    }
    while (written < frames) {
        const std::int64_t pos = position + written;
        if (s.bufStart >= 0) {
            const std::int64_t bufEnd = s.bufStart + s.bufFrames();
            if (pos < s.bufStart) { // seek caiu depois do alvo: silêncio até os dados
                written +=
                    static_cast<int>(std::min<std::int64_t>(s.bufStart - pos, frames - written));
                continue;
            }
            if (pos < bufEnd) {
                const int n =
                    static_cast<int>(std::min<std::int64_t>(bufEnd - pos, frames - written));
                const auto offset = static_cast<std::ptrdiff_t>((pos - s.bufStart) * kChannels);
                std::copy_n(s.buf.begin() + offset, n * kChannels,
                            out + static_cast<std::ptrdiff_t>(written) * kChannels);
                written += n;
                continue;
            }
            // Já consumido: descarta mantendo a posição contínua.
            s.buf.clear();
            s.bufStart = bufEnd;
        }
        if (!s.decodeMore()) {
            break; // fim do stream: o resto fica em silêncio
        }
    }
}

} // namespace mcs::media
