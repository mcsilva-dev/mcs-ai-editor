#pragma once

#include "core/rational.h"
#include "core/timeline.h"

#include <QSet>
#include <QString>
#include <QUuid>
#include <cstdint>
#include <optional>

namespace mcs::core {

// Resultado de uma operação de edição: nova timeline, ou a mensagem de por que não deu.
struct EditResult {
    std::optional<Timeline> timeline;
    QString error;
};

const Clip* findClip(const Timeline& timeline, const QUuid& id);
// Os ids mais todos os clipes ligados a eles (usado pela seleção com Linked Selection ligada).
QSet<QUuid> withLinked(const Timeline& timeline, const QSet<QUuid>& ids);

// Corta todos os clipes que atravessam `frame` (todas as trilhas).
EditResult blade(const Timeline& timeline, std::int64_t frame);
// Remove exatamente esses clipes, deixando lacuna.
EditResult lift(const Timeline& timeline, const QSet<QUuid>& ids);
// Remove exatamente esses clipes e fecha a lacuna em todas as trilhas. Recusa se outra trilha
// tiver clipe não selecionado no intervalo.
EditResult rippleDelete(const Timeline& timeline, const QSet<QUuid>& ids);

// Trecho de mídia vindo do viewer de fonte: [in, out) em quadros da fonte.
struct SourceRange {
    QString path;
    Rational sourceFps;
    std::int64_t in = 0;
    std::int64_t out = 0;
    bool video = false;
    bool audio = false;
};

// Coloca o trecho em `at` (V1/A1) por cima do que houver.
EditResult overwrite(const Timeline& timeline, const SourceRange& range, std::int64_t at);
// Corta em `at`, empurra todas as trilhas e coloca o trecho no espaço aberto.
EditResult insert(const Timeline& timeline, const SourceRange& range, std::int64_t at);
// Move exatamente esses clipes `delta` quadros, por cima do destino; nunca antes do quadro 0.
EditResult moveClips(const Timeline& timeline, const QSet<QUuid>& ids, std::int64_t delta);
// `delta` ajustado para uma borda dos clipes movidos encostar na borda mais próxima
// (outros clipes, playhead, quadro 0) a até `threshold` quadros; senão o próprio `delta`.
std::int64_t snapDelta(const Timeline& timeline, const QSet<QUuid>& ids, std::int64_t delta,
                       std::int64_t playhead, std::int64_t threshold);

inline constexpr double kMinGainDb = -60.0;
inline constexpr double kMaxGainDb = 12.0;
// Soma `deltaDb` ao volume dos clipes de áudio entre os ids (limitado a kMin..kMaxGainDb).
EditResult adjustGain(const Timeline& timeline, const QSet<QUuid>& ids, double deltaDb);

} // namespace mcs::core
