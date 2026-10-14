#include "core/edit_ops.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <algorithm>
#include <map>
#include <utility>
#include <vector>

namespace mcs::core {

namespace {

QString tr(const char* text) {
    return QCoreApplication::translate("mcs::core", text);
}

template <typename T> auto tracksOf(T& timeline) {
    std::vector<decltype(&timeline.video[0])> out;
    for (auto& t : timeline.video) {
        out.push_back(&t);
    }
    for (auto& t : timeline.audio) {
        out.push_back(&t);
    }
    return out;
}

// Parte direita de um clipe cortado em `frame` (start < frame < end). Clipes ligados cortados
// juntos recebem o mesmo link novo nas partes direitas (via `rightLinks`).
Clip rightPart(const Clip& c, std::int64_t frame, Rational fps,
               std::map<QUuid, QUuid>& rightLinks) {
    Clip r = c;
    r.id = QUuid::createUuid();
    if (!c.link.isNull()) {
        auto [it, inserted] = rightLinks.try_emplace(c.link);
        if (inserted) {
            it->second = QUuid::createUuid();
        }
        r.link = it->second;
    }
    r.sourceIn = sourceFrameAt(c, frame, fps);
    r.start = frame;
    r.duration = c.end() - frame;
    return r;
}

} // namespace

const Clip* findClip(const Timeline& timeline, const QUuid& id) {
    for (const Track* track : tracksOf(timeline)) {
        for (const Clip& c : track->clips) {
            if (c.id == id) {
                return &c;
            }
        }
    }
    return nullptr;
}

QSet<QUuid> withLinked(const Timeline& timeline, const QSet<QUuid>& ids) {
    QSet<QUuid> links;
    for (const Track* track : tracksOf(timeline)) {
        for (const Clip& c : track->clips) {
            if (ids.contains(c.id) && !c.link.isNull()) {
                links.insert(c.link);
            }
        }
    }
    QSet<QUuid> out = ids;
    for (const Track* track : tracksOf(timeline)) {
        for (const Clip& c : track->clips) {
            if (!c.link.isNull() && links.contains(c.link)) {
                out.insert(c.id);
            }
        }
    }
    return out;
}

EditResult blade(const Timeline& timeline, std::int64_t frame) {
    Timeline r = timeline;
    std::map<QUuid, QUuid> rightLinks;
    bool cut = false;
    for (Track* track : tracksOf(r)) {
        std::vector<Clip> out;
        for (const Clip& c : track->clips) {
            if (c.start < frame && frame < c.end()) {
                Clip left = c;
                left.duration = frame - c.start;
                out.push_back(left);
                out.push_back(rightPart(c, frame, r.fps, rightLinks));
                cut = true;
            } else {
                out.push_back(c);
            }
        }
        track->clips = std::move(out);
    }
    if (!cut) {
        return {std::nullopt, tr("There is no clip under the playhead to cut.")};
    }
    return {r, {}};
}

EditResult lift(const Timeline& timeline, const QSet<QUuid>& ids) {
    if (ids.isEmpty()) {
        return {std::nullopt, tr("Select one or more clips first.")};
    }
    Timeline r = timeline;
    for (Track* track : tracksOf(r)) {
        std::erase_if(track->clips, [&](const Clip& c) { return ids.contains(c.id); });
    }
    return {r, {}};
}

EditResult rippleDelete(const Timeline& timeline, const QSet<QUuid>& ids) {
    std::vector<std::pair<std::int64_t, std::int64_t>> ranges;
    for (const Track* track : tracksOf(timeline)) {
        for (const Clip& c : track->clips) {
            if (ids.contains(c.id)) {
                ranges.emplace_back(c.start, c.end());
            }
        }
    }
    if (ranges.empty()) {
        return {std::nullopt, tr("Select one or more clips first.")};
    }
    std::sort(ranges.begin(), ranges.end());
    std::vector<std::pair<std::int64_t, std::int64_t>> merged;
    for (const auto& range : ranges) {
        if (!merged.empty() && range.first <= merged.back().second) {
            merged.back().second = std::max(merged.back().second, range.second);
        } else {
            merged.push_back(range);
        }
    }
    // Outra trilha com clipe no intervalo: fechar a lacuna sobreporia esse clipe.
    for (const Track* track : tracksOf(timeline)) {
        for (const Clip& c : track->clips) {
            if (ids.contains(c.id)) {
                continue;
            }
            for (const auto& [s, e] : merged) {
                if (c.start < e && c.end() > s) {
                    return {std::nullopt,
                            tr("Ripple delete would move clips over \"%1\". Select it too or use "
                               "Delete.")
                                .arg(QFileInfo(c.path).fileName())};
                }
            }
        }
    }
    Timeline r = timeline;
    for (Track* track : tracksOf(r)) {
        std::erase_if(track->clips, [&](const Clip& c) { return ids.contains(c.id); });
        for (Clip& c : track->clips) {
            std::int64_t shift = 0;
            for (const auto& [s, e] : merged) {
                if (e <= c.start) {
                    shift += e - s;
                }
            }
            c.start -= shift;
        }
    }
    return {r, {}};
}

} // namespace mcs::core
