#include "support/synth_media.h"

#include <QScopeGuard>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

namespace mcs::test {

int lumaForFrame(int k) {
    return 16 + (k * 7) % 200;
}

int expectedGrayForFrame(int k) {
    const double v = (lumaForFrame(k) - 16) * 255.0 / 219.0;
    return static_cast<int>(std::lround(std::clamp(v, 0.0, 255.0)));
}

std::int16_t sampleValueAt(std::int64_t n) {
    return static_cast<std::int16_t>(((n % 2000) - 1000) * 16);
}

float expectedSampleAt(std::int64_t n) {
    return sampleValueAt(n) / 32768.0f;
}

QString writeSynthMov(const QString& path, const SynthSpec& s) {
    const QByteArray file = path.toUtf8();
    AVFormatContext* oc = nullptr;
    if (avformat_alloc_output_context2(&oc, nullptr, s.container, file.constData()) < 0 || !oc) {
        return QStringLiteral("avformat_alloc_output_context2 falhou");
    }
    AVCodecContext* vc = nullptr;
    AVCodecContext* ac = nullptr;
    AVCodecContext* pc = nullptr;
    AVPacket* pkt = av_packet_alloc();
    AVPacket* coverPkt = av_packet_alloc();
    AVFrame* f = av_frame_alloc();
    auto cleanup = qScopeGuard([&] {
        av_frame_free(&f);
        av_packet_free(&pkt);
        av_packet_free(&coverPkt);
        avcodec_free_context(&vc);
        avcodec_free_context(&ac);
        avcodec_free_context(&pc);
        if (oc->pb) {
            avio_closep(&oc->pb);
        }
        avformat_free_context(oc);
    });
    const bool globalHeader = (oc->oformat->flags & AVFMT_GLOBALHEADER) != 0;

    AVStream* vs = nullptr;
    if (s.video) {
        const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
        if (!codec) {
            return QStringLiteral("encoder mpeg4 ausente no FFmpeg");
        }
        vc = avcodec_alloc_context3(codec);
        vc->width = s.width;
        vc->height = s.height;
        vc->pix_fmt = AV_PIX_FMT_YUV420P;
        vc->time_base = AVRational{1, s.fps};
        vc->framerate = AVRational{s.fps, 1};
        vc->gop_size = s.gop;
        vc->max_b_frames = 0;
        vc->bit_rate = 2'000'000;
        if (globalHeader) {
            vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        // Cada quadro tem brilho diferente: sem isto a detecção de corte de cena transforma
        // todos em keyframes e os testes de seek no meio do GOP não testariam nada.
        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "sc_threshold", "1000000000", 0);
        const int opened = avcodec_open2(vc, codec, &opts);
        av_dict_free(&opts);
        if (opened < 0) {
            return QStringLiteral("avcodec_open2(mpeg4) falhou");
        }
        vs = avformat_new_stream(oc, nullptr);
        avcodec_parameters_from_context(vs->codecpar, vc);
        vs->time_base = vc->time_base;
    }

    AVStream* as = nullptr;
    if (s.audio) {
        // MP4 não aceita PCM: usa AAC (nativo), como um .m4a real.
        const bool aac = std::strcmp(s.container, "mp4") == 0;
        const AVCodec* codec = avcodec_find_encoder(aac ? AV_CODEC_ID_AAC : AV_CODEC_ID_PCM_S16LE);
        if (!codec) {
            return QStringLiteral("encoder de áudio ausente no FFmpeg");
        }
        ac = avcodec_alloc_context3(codec);
        ac->sample_fmt = aac ? AV_SAMPLE_FMT_FLTP : AV_SAMPLE_FMT_S16;
        ac->sample_rate = 48000;
        av_channel_layout_default(&ac->ch_layout, 2);
        ac->time_base = AVRational{1, 48000};
        if (globalHeader) {
            ac->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        if (avcodec_open2(ac, codec, nullptr) < 0) {
            return QStringLiteral("avcodec_open2(áudio) falhou");
        }
        as = avformat_new_stream(oc, nullptr);
        avcodec_parameters_from_context(as->codecpar, ac);
        as->time_base = ac->time_base;
    }

    AVStream* cs = nullptr;
    if (s.coverArt) {
        // JPEG como nas capas de MP3 reais (o encoder PNG exige zlib, ausente no nosso FFmpeg).
        const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
        if (!codec) {
            return QStringLiteral("encoder mjpeg ausente no FFmpeg");
        }
        pc = avcodec_alloc_context3(codec);
        pc->width = 16;
        pc->height = 16;
        pc->pix_fmt = AV_PIX_FMT_YUV420P;
        pc->color_range = AVCOL_RANGE_JPEG;
        pc->time_base = AVRational{1, 25};
        if (avcodec_open2(pc, codec, nullptr) < 0) {
            return QStringLiteral("avcodec_open2(mjpeg) falhou");
        }
        f->format = pc->pix_fmt;
        f->width = pc->width;
        f->height = pc->height;
        if (av_frame_get_buffer(f, 0) < 0) {
            return QStringLiteral("av_frame_get_buffer(capa) falhou");
        }
        for (int plane = 0; plane < 3; ++plane) {
            const int rows = plane == 0 ? f->height : f->height / 2;
            const auto cols = static_cast<size_t>(plane == 0 ? f->width : f->width / 2);
            for (int row = 0; row < rows; ++row) {
                std::memset(f->data[plane] + row * f->linesize[plane], plane == 0 ? 200 : 128,
                            cols);
            }
        }
        if (avcodec_send_frame(pc, f) < 0 || avcodec_receive_packet(pc, coverPkt) < 0) {
            return QStringLiteral("encode da capa falhou");
        }
        av_frame_unref(f);
        cs = avformat_new_stream(oc, nullptr);
        avcodec_parameters_from_context(cs->codecpar, pc);
        cs->disposition = AV_DISPOSITION_ATTACHED_PIC;
    }

    if (!(oc->oformat->flags & AVFMT_NOFILE) &&
        avio_open(&oc->pb, file.constData(), AVIO_FLAG_WRITE) < 0) {
        return QStringLiteral("avio_open falhou: %1").arg(path);
    }
    if (avformat_write_header(oc, nullptr) < 0) {
        return QStringLiteral("avformat_write_header falhou");
    }
    if (cs) {
        coverPkt->stream_index = cs->index;
        coverPkt->flags |= AV_PKT_FLAG_KEY;
        if (av_interleaved_write_frame(oc, coverPkt) < 0) {
            return QStringLiteral("gravação da capa falhou");
        }
    }

    auto drain = [&](AVCodecContext* c, AVStream* st) {
        for (;;) {
            const int r = avcodec_receive_packet(c, pkt);
            if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) {
                return true;
            }
            if (r < 0) {
                return false;
            }
            // O encoder não preenche a duração; sem ela o último pacote fica fora da edit list
            // do .mov e o demuxer o marca como DISCARD (o quadro final "some").
            if (pkt->duration <= 0 && !s.omitPacketDuration) {
                pkt->duration = (c == vc) ? 1 : 48000 / s.fps;
            }
            av_packet_rescale_ts(pkt, c->time_base, st->time_base);
            pkt->stream_index = st->index;
            if (av_interleaved_write_frame(oc, pkt) < 0) {
                return false;
            }
        }
    };

    const int samplesPerFrame = 48000 / s.fps;
    // AAC exige blocos de frame_size amostras (1024); PCM aceita qualquer tamanho (frame_size 0).
    const int audioChunk = (ac && ac->frame_size > 0) ? ac->frame_size : samplesPerFrame;
    std::int64_t audioPts = 0;
    for (int k = 0; k < s.frames; ++k) {
        if (vc) {
            av_frame_unref(f);
            f->format = vc->pix_fmt;
            f->width = vc->width;
            f->height = vc->height;
            if (av_frame_get_buffer(f, 0) < 0) {
                return QStringLiteral("av_frame_get_buffer(video) falhou");
            }
            const auto y = static_cast<unsigned char>(lumaForFrame(k));
            for (int row = 0; row < f->height; ++row) {
                std::memset(f->data[0] + row * f->linesize[0], y, static_cast<size_t>(f->width));
            }
            const auto chromaW = static_cast<size_t>((f->width + 1) / 2);
            for (int row = 0; row < (f->height + 1) / 2; ++row) {
                std::memset(f->data[1] + row * f->linesize[1], 128, chromaW);
                std::memset(f->data[2] + row * f->linesize[2], 128, chromaW);
            }
            f->pts = k;
            if (avcodec_send_frame(vc, f) < 0 || !drain(vc, vs)) {
                return QStringLiteral("encode de vídeo falhou no quadro %1").arg(k);
            }
        }
        // Áudio em silêncio acompanhando o vídeo até o fim do quadro k.
        while (ac && audioPts < static_cast<std::int64_t>(k + 1) * samplesPerFrame) {
            av_frame_unref(f);
            f->format = ac->sample_fmt;
            f->sample_rate = ac->sample_rate;
            av_channel_layout_copy(&f->ch_layout, &ac->ch_layout);
            f->nb_samples = audioChunk;
            if (av_frame_get_buffer(f, 0) < 0) {
                return QStringLiteral("av_frame_get_buffer(audio) falhou");
            }
            if (ac->sample_fmt == AV_SAMPLE_FMT_S16) {
                auto* pcm = reinterpret_cast<std::int16_t*>(f->data[0]);
                for (int i = 0; i < audioChunk; ++i) {
                    const std::int16_t v = sampleValueAt(audioPts + i);
                    pcm[2 * i] = v;
                    pcm[2 * i + 1] = static_cast<std::int16_t>(-v);
                }
            } else {
                av_samples_set_silence(f->data, 0, audioChunk, ac->ch_layout.nb_channels,
                                       ac->sample_fmt);
            }
            f->pts = audioPts + s.audioDelaySamples; // conteúdo segue a rampa a partir de 0
            audioPts += audioChunk;
            if (avcodec_send_frame(ac, f) < 0 || !drain(ac, as)) {
                return QStringLiteral("encode de áudio falhou no quadro %1").arg(k);
            }
        }
    }
    if (vc && (avcodec_send_frame(vc, nullptr) < 0 || !drain(vc, vs))) {
        return QStringLiteral("flush de vídeo falhou");
    }
    if (ac && (avcodec_send_frame(ac, nullptr) < 0 || !drain(ac, as))) {
        return QStringLiteral("flush de áudio falhou");
    }
    if (av_write_trailer(oc) < 0) {
        return QStringLiteral("av_write_trailer falhou");
    }
    return {};
}

bool hasAttachedPicture(const QString& path) {
    const QByteArray file = path.toUtf8();
    AVFormatContext* fmt = nullptr;
    if (avformat_open_input(&fmt, file.constData(), nullptr, nullptr) < 0) {
        return false;
    }
    bool found = false;
    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        found = found || (fmt->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0;
    }
    avformat_close_input(&fmt);
    return found;
}

PacketStats videoPacketStats(const QString& path) {
    const QByteArray file = path.toUtf8();
    AVFormatContext* fmt = nullptr;
    if (avformat_open_input(&fmt, file.constData(), nullptr, nullptr) < 0) {
        return {};
    }
    AVPacket* pkt = av_packet_alloc();
    auto cleanup = qScopeGuard([&] {
        av_packet_free(&pkt);
        avformat_close_input(&fmt);
    });
    const int vs = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (avformat_find_stream_info(fmt, nullptr) < 0 || vs < 0) {
        return {};
    }
    PacketStats stats{0, 0, 0};
    while (av_read_frame(fmt, pkt) >= 0) {
        if (pkt->stream_index == vs) {
            ++stats.packets;
            if (pkt->flags & AV_PKT_FLAG_KEY) {
                ++stats.keyframes;
            }
            if (pkt->flags & AV_PKT_FLAG_DISCARD) {
                ++stats.discarded;
            }
        }
        av_packet_unref(pkt);
    }
    return stats;
}

} // namespace mcs::test
