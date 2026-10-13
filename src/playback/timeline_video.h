#pragma once

#include "core/timeline.h"

#include <QImage>
#include <QString>
#include <cstdint>
#include <map>
#include <memory>

namespace mcs::media {
class FrameDecoder;
}

namespace mcs::playback {

// Converte quadros da timeline em imagens (trilha de vídeo mais alta que tem clipe).
// v0.3: decode síncrono na thread chamadora (GUI); prefetch em worker no v0.8.
class TimelineVideo {
  public:
    TimelineVideo();
    ~TimelineVideo();

    void setTimeline(const core::Timeline& timeline);
    // Imagem do quadro `frame`; nula em lacunas ou fora da timeline.
    QImage frameAt(std::int64_t frame);

  private:
    media::FrameDecoder* decoderFor(const QString& path);

    core::Timeline timeline_;
    std::map<QString, std::unique_ptr<media::FrameDecoder>> decoders_;
    std::int64_t lastFrame_ = -1;
    QImage lastImage_;
};

} // namespace mcs::playback
