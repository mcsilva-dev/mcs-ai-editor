#include "playback/playback_engine.h"

#include "media/audio_decoder.h"

#include <algorithm>

namespace mcs::playback {

namespace {
constexpr int kCh = media::AudioDecoder::kChannels;
constexpr int kRate = media::AudioDecoder::kSampleRate;
constexpr int kChunk = 1024;
} // namespace

PlaybackEngine::PlaybackEngine(int ringFrames)
    : ring_(static_cast<std::size_t>(ringFrames) * kCh), capacity_(ringFrames) {}

void PlaybackEngine::setTimeline(const core::Timeline& timeline) {
    timeline_ = timeline;
    audio_.setTimeline(timeline);
    seek(seekFrame_);
}

void PlaybackEngine::seek(std::int64_t frame) {
    seekFrame_ = std::max<std::int64_t>(0, frame);
    seekSample_ = core::frameToSample(seekFrame_, timeline_.fps, kRate);
    written_.store(0);
    read_.store(0);
}

void PlaybackEngine::setOutputLatency(int frames) {
    latency_.store(std::max(0, frames));
}

void PlaybackEngine::fillAhead() {
    const std::int64_t r = read_.load(std::memory_order_acquire);
    std::int64_t w = written_.load(std::memory_order_relaxed);
    while (w - r < capacity_) {
        const int n = static_cast<int>(std::min<std::int64_t>(kChunk, capacity_ - (w - r)));
        chunk_.resize(static_cast<std::size_t>(n) * kCh);
        audio_.render(seekSample_ + w, chunk_.data(), n);
        for (int i = 0; i < n; ++i) {
            const auto slot = static_cast<std::size_t>((w + i) % capacity_) * kCh;
            ring_[slot] = chunk_[static_cast<std::size_t>(i) * kCh];
            ring_[slot + 1] = chunk_[static_cast<std::size_t>(i) * kCh + 1];
        }
        w += n;
        written_.store(w, std::memory_order_release);
    }
}

void PlaybackEngine::pull(float* out, int frames) {
    const std::int64_t w = written_.load(std::memory_order_acquire);
    const std::int64_t r = read_.load(std::memory_order_relaxed);
    const int n = static_cast<int>(std::min<std::int64_t>(frames, w - r));
    for (int i = 0; i < n; ++i) {
        const auto slot = static_cast<std::size_t>((r + i) % capacity_) * kCh;
        out[i * kCh] = ring_[slot];
        out[i * kCh + 1] = ring_[slot + 1];
    }
    std::fill(out + static_cast<std::ptrdiff_t>(n) * kCh,
              out + static_cast<std::ptrdiff_t>(frames) * kCh, 0.0f);
    if (n < frames) {
        underruns_.fetch_add(1, std::memory_order_relaxed);
    }
    read_.store(r + n, std::memory_order_release);
}

std::int64_t PlaybackEngine::currentFrame() const {
    const std::int64_t heard =
        std::max<std::int64_t>(0, read_.load(std::memory_order_acquire) - latency_.load());
    return std::max(seekFrame_, core::sampleToFrame(seekSample_ + heard, timeline_.fps, kRate));
}

std::int64_t PlaybackEngine::deliveredFrames() const {
    return read_.load();
}

std::int64_t PlaybackEngine::underruns() const {
    return underruns_.load();
}

core::Rational PlaybackEngine::fps() const {
    return timeline_.fps;
}

} // namespace mcs::playback
