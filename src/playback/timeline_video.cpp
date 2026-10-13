#include "playback/timeline_video.h"

#include "media/frame_decoder.h"

namespace mcs::playback {

TimelineVideo::TimelineVideo() = default;
TimelineVideo::~TimelineVideo() = default;

void TimelineVideo::setTimeline(const core::Timeline& timeline) {
    timeline_ = timeline;
    lastFrame_ = -1;
    lastImage_ = {};
}

media::FrameDecoder* TimelineVideo::decoderFor(const QString& path) {
    auto it = decoders_.find(path);
    if (it == decoders_.end()) {
        auto decoder = std::make_unique<media::FrameDecoder>();
        if (!decoder->open(path)) {
            decoder.reset(); // guarda a falha para não tentar abrir a cada quadro
        }
        it = decoders_.emplace(path, std::move(decoder)).first;
    }
    return it->second.get();
}

QImage TimelineVideo::frameAt(std::int64_t frame) {
    if (frame == lastFrame_) {
        return lastImage_;
    }
    QImage image;
    for (auto track = timeline_.video.rbegin(); track != timeline_.video.rend(); ++track) {
        const core::Clip* clip = core::clipAt(*track, frame);
        if (!clip) {
            continue;
        }
        if (media::FrameDecoder* decoder = decoderFor(clip->path)) {
            const auto decoded = decoder->frameAt(core::sourceFrameAt(*clip, frame, timeline_.fps));
            if (decoded) {
                image = decoded->image;
            }
        }
        break;
    }
    lastFrame_ = frame;
    lastImage_ = image;
    return image;
}

} // namespace mcs::playback
