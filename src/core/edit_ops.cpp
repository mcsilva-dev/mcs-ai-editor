#include "core/edit_ops.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <algorithm>
#include <cstdlib>
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

void sortTrack(Track& track) {
    std::sort(track.clips.begin(), track.clips.end(),
              [](const Clip& a, const Clip& b) { return a.start < b.start; });
}

// Libera [s, e) na trilha: remove o que está dentro e apara/divide o que cruza as bordas.
void clearRange(Track& track, std::int64_t s, std::int64_t e, Rational fps,
                std::map<QUuid, QUuid>& rightLinks) {
    std::vector<Clip> out;
    for (const Clip& c : track.clips) {
        if (c.end() <= s || c.start >= e) {
            out.push_back(c);
            continue;
        }
        if (c.start < s) {
            Clip left = c;
            left.duration = s - c.start;
            out.push_back(left);
        }
        if (c.end() > e) {
            out.push_back(rightPart(c, e, fps, rightLinks));
        }
    }
    track.clips = std::move(out);
    sortTrack(track);
}

void place(Track& track, const Clip& clip, Rational fps, std::map<QUuid, QUuid>& rightLinks) {
    clearRange(track, clip.start, clip.end(), fps, rightLinks);
    track.clips.push_back(clip);
    sortTrack(track);
}

bool isValid(const SourceRange& r) {
    return !r.path.isEmpty() && r.sourceFps.num > 0 && r.in >= 0 && r.out > r.in &&
           (r.video || r.audio);
}

struct NewClips {
    std::optional<Clip> video;
    std::optional<Clip> audio;
    std::int64_t duration = 0;
};

NewClips makeClips(const SourceRange& r, std::int64_t at, Rational tl) {
    NewClips n;
    const std::int64_t num = (r.out - r.in) * tl.num * r.sourceFps.den;
    const std::int64_t den = tl.den * r.sourceFps.num;
    n.duration = std::max<std::int64_t>(1, (2 * num + den) / (2 * den));
    const QUuid link = QUuid::createUuid();
    if (r.video) {
        n.video = Clip{QUuid::createUuid(), r.path, r.sourceFps, at, n.duration, r.in, link, 0.0};
    }
    if (r.audio) {
        n.audio = Clip{QUuid::createUuid(), r.path, r.sourceFps, at, n.duration, r.in, link, 0.0};
    }
    return n;
}

QString invalidRangeMessage() {
    return tr("Mark a valid In and Out point on a clip in the source viewer first.");
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

EditResult overwrite(const Timeline& timeline, const SourceRange& range, std::int64_t at) {
    if (!isValid(range)) {
        return {std::nullopt, invalidRangeMessage()};
    }
    Timeline r = timeline;
    std::map<QUuid, QUuid> rightLinks;
    const NewClips n = makeClips(range, std::max<std::int64_t>(0, at), r.fps);
    if (n.video) {
        place(r.video[0], *n.video, r.fps, rightLinks);
    }
    if (n.audio) {
        place(r.audio[0], *n.audio, r.fps, rightLinks);
    }
    return {r, {}};
}

EditResult insert(const Timeline& timeline, const SourceRange& range, std::int64_t at) {
    if (!isValid(range)) {
        return {std::nullopt, invalidRangeMessage()};
    }
    at = std::max<std::int64_t>(0, at);
    Timeline r = timeline;
    std::map<QUuid, QUuid> rightLinks;
    const NewClips n = makeClips(range, at, r.fps);
    for (Track* track : tracksOf(r)) {
        std::vector<Clip> out;
        for (const Clip& c : track->clips) {
            if (c.start < at && at < c.end()) {
                Clip left = c;
                left.duration = at - c.start;
                Clip right = rightPart(c, at, r.fps, rightLinks);
                right.start += n.duration;
                out.push_back(left);
                out.push_back(right);
            } else {
                Clip moved = c;
                if (c.start >= at) {
                    moved.start += n.duration;
                }
                out.push_back(moved);
            }
        }
        track->clips = std::move(out);
    }
    if (n.video) {
        r.video[0].clips.push_back(*n.video);
        sortTrack(r.video[0]);
    }
    if (n.audio) {
        r.audio[0].clips.push_back(*n.audio);
        sortTrack(r.audio[0]);
    }
    return {r, {}};
}

EditResult moveClips(const Timeline& timeline, const QSet<QUuid>& ids, std::int64_t delta) {
    std::int64_t minStart = -1;
    for (const Track* track : tracksOf(timeline)) {
        for (const Clip& c : track->clips) {
            if (ids.contains(c.id)) {
                minStart = minStart < 0 ? c.start : std::min(minStart, c.start);
            }
        }
    }
    if (minStart < 0) {
        return {std::nullopt, tr("Select one or more clips first.")};
    }
    delta = std::max(delta, -minStart);
    Timeline r = timeline;
    std::map<QUuid, QUuid> rightLinks;
    for (Track* track : tracksOf(r)) {
        std::vector<Clip> moved;
        for (const Clip& c : track->clips) {
            if (ids.contains(c.id)) {
                moved.push_back(c);
            }
        }
        std::erase_if(track->clips, [&](const Clip& c) { return ids.contains(c.id); });
        for (Clip c : moved) {
            c.start += delta;
            place(*track, c, r.fps, rightLinks);
        }
    }
    return {r, {}};
}

std::int64_t snapDelta(const Timeline& timeline, const QSet<QUuid>& ids, std::int64_t delta,
                       std::int64_t playhead, std::int64_t threshold) {
    std::vector<std::int64_t> targets{0, playhead};
    std::vector<std::int64_t> edges;
    for (const Track* track : tracksOf(timeline)) {
        for (const Clip& c : track->clips) {
            auto& list = ids.contains(c.id) ? edges : targets;
            list.push_back(c.start);
            list.push_back(c.end());
        }
    }
    std::int64_t best = delta;
    std::int64_t bestDistance = threshold + 1;
    for (const std::int64_t edge : edges) {
        for (const std::int64_t target : targets) {
            const std::int64_t d = target - (edge + delta);
            if (std::abs(d) < bestDistance) {
                bestDistance = std::abs(d);
                best = delta + d;
            }
        }
    }
    return best;
}

EditResult adjustGain(const Timeline& timeline, const QSet<QUuid>& ids, double deltaDb) {
    Timeline r = timeline;
    bool changed = false;
    for (Track& track : r.audio) {
        for (Clip& c : track.clips) {
            if (ids.contains(c.id)) {
                c.gainDb = std::clamp(c.gainDb + deltaDb, kMinGainDb, kMaxGainDb);
                changed = true;
            }
        }
    }
    if (!changed) {
        return {std::nullopt, tr("Select an audio clip to change its volume.")};
    }
    return {r, {}};
}

} // namespace mcs::core
