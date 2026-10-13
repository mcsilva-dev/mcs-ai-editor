#include "core/timeline.h"

#include <algorithm>
#include <cmath>

namespace mcs::core {

namespace {
// round(a / b) para a >= 0, b > 0.
std::int64_t divRound(std::int64_t a, std::int64_t b) {
    return (2 * a + b) / (2 * b);
}
} // namespace

std::int64_t Track::end() const {
    return clips.empty() ? 0 : clips.back().end();
}

std::int64_t Timeline::duration() const {
    std::int64_t d = 0;
    for (const Track& t : video) {
        d = std::max(d, t.end());
    }
    for (const Track& t : audio) {
        d = std::max(d, t.end());
    }
    return d;
}

const Clip* clipAt(const Track& track, std::int64_t frame) {
    for (const Clip& c : track.clips) {
        if (frame >= c.start && frame < c.end()) {
            return &c;
        }
    }
    return nullptr;
}

std::int64_t frameToSample(std::int64_t frame, Rational fps, int sampleRate) {
    const std::int64_t num = frame * sampleRate * fps.den;
    return (num + fps.num - 1) / fps.num;
}

std::int64_t sampleToFrame(std::int64_t sample, Rational fps, int sampleRate) {
    return sample * fps.num / (static_cast<std::int64_t>(sampleRate) * fps.den);
}

std::int64_t sourceFrameAt(const Clip& clip, std::int64_t frame, Rational timelineFps) {
    const std::int64_t offset = std::max<std::int64_t>(0, frame - clip.start);
    return clip.sourceIn + divRound(offset * clip.sourceFps.num * timelineFps.den,
                                    clip.sourceFps.den * timelineFps.num);
}

std::int64_t sourceSampleAt(const Clip& clip, std::int64_t timelineSample, Rational timelineFps,
                            int sampleRate) {
    return frameToSample(clip.sourceIn, clip.sourceFps, sampleRate) +
           (timelineSample - frameToSample(clip.start, timelineFps, sampleRate));
}

std::int64_t appendMedia(Timeline& timeline, const MediaInfo& info) {
    if (!info.video && !info.audio) {
        return -1;
    }
    if (timeline.duration() == 0 && info.video && info.video->fps.num > 0) {
        timeline.fps = info.video->fps;
    }
    const Rational tl = timeline.fps;
    const std::int64_t start = std::max(timeline.video[0].end(), timeline.audio[0].end());

    std::int64_t duration = 0;
    Rational sourceFps = tl;
    if (info.video && info.video->fps.num > 0) {
        sourceFps = info.video->fps;
        duration =
            divRound(info.video->frameCount * tl.num * sourceFps.den, tl.den * sourceFps.num);
    } else {
        duration = std::llround(info.durationSeconds * tl.toDouble());
    }

    if (info.video) {
        timeline.video[0].clips.push_back(
            Clip{QUuid::createUuid(), info.path, sourceFps, start, duration, 0});
    }
    if (info.audio) {
        timeline.audio[0].clips.push_back(
            Clip{QUuid::createUuid(), info.path, sourceFps, start, duration, 0});
    }
    return start;
}

} // namespace mcs::core
