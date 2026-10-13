#pragma once

#include <QString>
#include <cstdint>
#include <memory>

namespace mcs::media {

// Decodifica o áudio de um arquivo para estéreo float32 intercalado a 48 kHz.
// Leituras sequenciais continuam o decode; saltos fazem seek.
class AudioDecoder {
  public:
    static constexpr int kSampleRate = 48000;
    static constexpr int kChannels = 2;

    AudioDecoder();
    ~AudioDecoder();
    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;

    bool open(const QString& path, QString* error = nullptr);
    bool isOpen() const;
    // Escreve `frames` quadros (L,R) a partir da amostra `position` (48 kHz, desde o início
    // do stream). Fora do stream (ou decoder fechado): zeros.
    void read(std::int64_t position, float* out, int frames);
    std::int64_t length() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};

} // namespace mcs::media
