#pragma once

#include "core/timeline.h"
#include "playback/timeline_audio.h"

#include <atomic>
#include <cstdint>
#include <vector>

namespace mcs::playback {

// Coração do playback. O áudio é o relógio mestre:
//   fillAhead() — thread de alimentação: mixa a timeline num buffer circular (SPSC);
//   pull()      — thread de áudio: entrega amostras ao dispositivo (silêncio se faltar);
//   currentFrame() — qualquer thread: quadro do áudio que está soando (entregue − latência).
// Underrun não avança o relógio: o vídeo espera o áudio. setTimeline/seek só com a saída parada.
class PlaybackEngine {
  public:
    explicit PlaybackEngine(int ringFrames = 24000); // 0,5 s a 48 kHz

    void setTimeline(const core::Timeline& timeline);
    void seek(std::int64_t frame);
    void fillAhead();
    void pull(float* out, int frames);
    void setOutputLatency(int frames);

    std::int64_t currentFrame() const;
    std::int64_t deliveredFrames() const;
    std::int64_t underruns() const;
    core::Rational fps() const;

  private:
    core::Timeline timeline_;
    TimelineAudio audio_;
    std::vector<float> ring_;
    std::vector<float> chunk_;
    std::int64_t capacity_;
    std::int64_t seekFrame_ = 0;
    std::int64_t seekSample_ = 0;
    std::atomic<std::int64_t> written_{0};
    std::atomic<std::int64_t> read_{0};
    std::atomic<std::int64_t> underruns_{0};
    std::atomic<int> latency_{0};
};

} // namespace mcs::playback
