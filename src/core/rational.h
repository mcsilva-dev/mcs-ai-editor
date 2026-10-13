#pragma once

#include <QString>
#include <cstdint>

namespace mcs::core {

// Fração exata (ex.: fps 30000/1001). Nunca use double para posições de tempo.
struct Rational {
    std::int64_t num = 0;
    std::int64_t den = 1;

    double toDouble() const { return den == 0 ? 0.0 : static_cast<double>(num) / den; }
    friend bool operator==(const Rational&, const Rational&) = default;
};

// Divide pelo MDC e põe o sinal no numerador; den == 0 vira {0, 1}.
Rational normalized(Rational r);

// "HH:MM:SS:FF" non-drop-frame com base nominal round(fps). Entrada inválida: "--:--:--:--".
QString timecode(std::int64_t frame, Rational fps);

} // namespace mcs::core
