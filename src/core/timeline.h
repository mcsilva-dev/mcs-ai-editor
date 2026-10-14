#pragma once

#include "core/media_info.h"
#include "core/rational.h"

#include <QString>
#include <QUuid>
#include <cstdint>
#include <vector>

namespace mcs::core {

// Clipe na timeline: start/duration em quadros da timeline; sourceIn em quadros da fonte.
struct Clip {
    QUuid id;
    QString path;
    Rational sourceFps; // fps da mídia (ou da timeline para clipe só de áudio)
    std::int64_t start = 0;
    std::int64_t duration = 0;
    std::int64_t sourceIn = 0;
    QUuid link;          // vídeo e áudio criados juntos compartilham o valor (nulo = sem ligação)
    double gainDb = 0.0; // volume do clipe de áudio (−60…+12 dB)

    std::int64_t end() const { return start + duration; }
};

// Clipes ordenados por start, sem sobreposição.
struct Track {
    std::vector<Clip> clips;
    std::int64_t end() const;
};

struct Timeline {
    Rational fps{25, 1};
    std::vector<Track> video{Track{}}; // V1
    std::vector<Track> audio{Track{}}; // A1
    std::int64_t duration() const;     // fim do último clipe em qualquer trilha
};

// Clipe que cobre `frame` ([start, end)), ou nullptr.
const Clip* clipAt(const Track& track, std::int64_t frame);

// Primeira amostra do quadro (teto) e quadro de uma amostra (piso).
std::int64_t frameToSample(std::int64_t frame, Rational fps, int sampleRate);
std::int64_t sampleToFrame(std::int64_t sample, Rational fps, int sampleRate);

// Quadro da fonte exibido no quadro `frame` da timeline (quadro mais próximo).
std::int64_t sourceFrameAt(const Clip& clip, std::int64_t frame, Rational timelineFps);

// Amostra da fonte que soa na amostra `timelineSample` da timeline.
std::int64_t sourceSampleAt(const Clip& clip, std::int64_t timelineSample, Rational timelineFps,
                            int sampleRate);

// Acrescenta a mídia no fim: vídeo em V1, áudio em A1, mesmo início. Timeline vazia adota o
// fps do vídeo. Retorna o quadro inicial, ou -1 se a mídia não tem vídeo nem áudio.
std::int64_t appendMedia(Timeline& timeline, const MediaInfo& info);

} // namespace mcs::core
