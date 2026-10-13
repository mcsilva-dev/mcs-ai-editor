#pragma once

#include <QString>
#include <memory>

namespace mcs::playback {

class PlaybackEngine;

// Saída de áudio estéreo float 48 kHz (miniaudio) que puxa amostras do PlaybackEngine.
class AudioOutput {
  public:
    // Default: placa de som do sistema. Null: dispositivo simulado em tempo real (testes/CI).
    // None: nenhum dispositivo (start falha; o Player usa o relógio do sistema).
    enum class Backend { Default, Null, None };

    explicit AudioOutput(PlaybackEngine& engine);
    ~AudioOutput();
    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;

    bool start(Backend backend, QString* error = nullptr);
    void stop();
    bool isRunning() const;
    int latencyFrames() const;
    // true se o dispositivo parou sem pedirmos (fone desconectado, backend caiu).
    bool deviceLost() const;
    // Para testes: para o dispositivo "por fora", como um fone desconectado.
    void simulateDeviceLoss();

  private:
    struct Impl;
    PlaybackEngine& engine_;
    std::unique_ptr<Impl> d_;
};

} // namespace mcs::playback
