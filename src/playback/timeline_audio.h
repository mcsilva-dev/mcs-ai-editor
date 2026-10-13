#pragma once

#include "core/timeline.h"

#include <QString>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace mcs::media {
class AudioDecoder;
}

namespace mcs::playback {

// Mixa as trilhas de áudio da timeline em estéreo float 48 kHz.
// Não é thread-safe: usado só pela thread de alimentação do PlaybackEngine.
class TimelineAudio {
  public:
    TimelineAudio();
    ~TimelineAudio();

    void setTimeline(const core::Timeline& timeline);
    // Escreve `frames` quadros a partir da amostra `position` da timeline.
    void render(std::int64_t position, float* out, int frames);

  private:
    media::AudioDecoder* decoderFor(const QString& path);

    core::Timeline timeline_;
    std::map<QString, std::unique_ptr<media::AudioDecoder>> decoders_;
    std::vector<float> scratch_;
};

} // namespace mcs::playback
