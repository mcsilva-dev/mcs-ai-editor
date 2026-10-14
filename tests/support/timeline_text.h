#pragma once

#include "core/timeline.h"

#include <QSet>
#include <QString>
#include <QUuid>

namespace mcs::test {

// "V1: A[0..10) B[10..20)@5 | A1: a[0..10)" — nome = path; @N = sourceIn; fps 25.
// Nomes iguais sem diferenciar maiúsculas compartilham link. "V1: -" = trilha vazia.
core::Timeline parseTimeline(const QString& text);
QString formatTimeline(const core::Timeline& timeline);
// Ids de todos os clipes cujo path é exatamente `name`.
QSet<QUuid> idsOf(const core::Timeline& timeline, const QString& name);

} // namespace mcs::test
