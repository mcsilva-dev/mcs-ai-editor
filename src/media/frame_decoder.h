#pragma once

#include "core/rational.h"

#include <QImage>
#include <QSize>
#include <QString>
#include <cstdint>
#include <memory>
#include <optional>

namespace mcs::media {

struct DecodedFrame {
    std::int64_t index = 0;
    QImage image; // Format_RGBA8888
};

// Decodifica quadros individuais com seek exato (keyframe anterior + decode até o alvo).
// v0.2: decode por software, síncrono. Cache/prefetch e decode por hardware entram no v0.3.
class FrameDecoder {
  public:
    FrameDecoder();
    ~FrameDecoder();
    FrameDecoder(const FrameDecoder&) = delete;
    FrameDecoder& operator=(const FrameDecoder&) = delete;

    bool open(const QString& path, QString* error = nullptr);
    bool isOpen() const;
    std::optional<DecodedFrame> frameAt(std::int64_t index);
    std::int64_t frameCount() const;
    core::Rational fps() const;
    QSize size() const;
    // Quantos seeks foram feitos (diagnóstico: avançar em sequência não deve fazer seek).
    std::int64_t seekCount() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};

} // namespace mcs::media
