#pragma once

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

} // namespace mcs::core
