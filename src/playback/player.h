#pragma once

#include "core/timeline.h"
#include "playback/audio_output.h"
#include "playback/playback_engine.h"

#include <QElapsedTimer>
#include <QObject>
#include <atomic>
#include <cstdint>
#include <thread>

namespace mcs::playback {

// Junta motor, saída de áudio e thread de alimentação. Sem dispositivo de áudio, o relógio
// passa a ser o do sistema (QElapsedTimer).
class Player : public QObject {
    Q_OBJECT
  public:
    explicit Player(QObject* parent = nullptr);
    ~Player() override;

    void setAudioBackend(AudioOutput::Backend backend);
    void setTimeline(const core::Timeline& timeline);
    void play();
    void pause();
    bool isPlaying() const;
    void seek(std::int64_t frame);
    std::int64_t currentFrame() const;
    bool usingFallbackClock() const;

  signals:
    void playingChanged(bool playing);

  private:
    void stopFeeder();

    PlaybackEngine engine_;
    AudioOutput output_{engine_};
    AudioOutput::Backend backend_ = AudioOutput::Backend::Default;
    std::thread feeder_;
    std::atomic<bool> feeding_{false};
    bool playing_ = false;
    bool fallback_ = false;
    QElapsedTimer fallbackTimer_;
    std::int64_t pausedFrame_ = 0;
};

} // namespace mcs::playback
