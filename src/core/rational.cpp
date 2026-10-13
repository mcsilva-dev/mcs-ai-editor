#include "core/rational.h"

#include <cmath>
#include <numeric>

namespace mcs::core {

Rational normalized(Rational r) {
    if (r.den == 0) {
        return {0, 1};
    }
    if (r.den < 0) {
        r.num = -r.num;
        r.den = -r.den;
    }
    const std::int64_t g = std::gcd(r.num, r.den);
    return g > 1 ? Rational{r.num / g, r.den / g} : r;
}

QString timecode(std::int64_t frame, Rational fps) {
    const auto base = static_cast<std::int64_t>(std::llround(fps.toDouble()));
    if (base <= 0 || frame < 0) {
        return QStringLiteral("--:--:--:--");
    }
    const std::int64_t ff = frame % base;
    const std::int64_t totalSeconds = frame / base;
    const std::int64_t ss = totalSeconds % 60;
    const std::int64_t mm = (totalSeconds / 60) % 60;
    const std::int64_t hh = totalSeconds / 3600;
    return QStringLiteral("%1:%2:%3:%4")
        .arg(hh, 2, 10, QLatin1Char('0'))
        .arg(mm, 2, 10, QLatin1Char('0'))
        .arg(ss, 2, 10, QLatin1Char('0'))
        .arg(ff, 2, 10, QLatin1Char('0'));
}

} // namespace mcs::core
