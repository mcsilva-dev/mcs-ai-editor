#include "playback/timeline_audio.h"

#include "media/audio_decoder.h"

#include <algorithm>

namespace mcs::playback {

using media::AudioDecoder;

TimelineAudio::TimelineAudio() = default;
TimelineAudio::~TimelineAudio() = default;

void TimelineAudio::setTimeline(const core::Timeline& timeline) {
    timeline_ = timeline;
}

AudioDecoder* TimelineAudio::decoderFor(const QString& path) {
    auto it = decoders_.find(path);
    if (it == decoders_.end()) {
        auto decoder = std::make_unique<AudioDecoder>();
        if (!decoder->open(path)) {
            decoder.reset();
        }
        it = decoders_.emplace(path, std::move(decoder)).first;
    }
    return it->second.get();
}

void TimelineAudio::render(std::int64_t position, float* out, int frames) {
    constexpr int ch = AudioDecoder::kChannels;
    constexpr int rate = AudioDecoder::kSampleRate;
    std::fill(out, out + static_cast<std::ptrdiff_t>(frames) * ch, 0.0f);
    scratch_.resize(static_cast<std::size_t>(frames) * ch);
    const std::int64_t end = position + frames;
    // ponytail: varre todos os clipes a cada bloco; índice por tempo se timelines grandes pesarem.
    for (const core::Track& track : timeline_.audio) {
        for (const core::Clip& clip : track.clips) {
            const std::int64_t from =
                std::max(core::frameToSample(clip.start, timeline_.fps, rate), position);
            const std::int64_t to =
                std::min(core::frameToSample(clip.end(), timeline_.fps, rate), end);
            if (from >= to) {
                continue;
            }
            AudioDecoder* decoder = decoderFor(clip.path);
            if (!decoder) {
                continue;
            }
            const int n = static_cast<int>(to - from);
            decoder->read(core::sourceSampleAt(clip, from, timeline_.fps, rate), scratch_.data(),
                          n);
            float* dst = out + (from - position) * ch;
            for (int i = 0; i < n * ch; ++i) {
                dst[i] += scratch_[static_cast<std::size_t>(i)];
            }
        }
    }
}

} // namespace mcs::playback
