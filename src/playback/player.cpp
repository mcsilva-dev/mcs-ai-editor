#include "playback/player.h"

#include <algorithm>
#include <chrono>

namespace mcs::playback {

Player::Player(QObject* parent) : QObject(parent) {}

Player::~Player() {
    pause();
}

void Player::setAudioBackend(AudioOutput::Backend backend) {
    backend_ = backend;
}

void Player::setTimeline(const core::Timeline& timeline) {
    pause();
    engine_.setTimeline(timeline);
}

void Player::play() {
    if (playing_) {
        return;
    }
    engine_.seek(pausedFrame_);
    engine_.fillAhead(); // buffer cheio antes do primeiro callback
    fallback_ = !output_.start(backend_);
    if (fallback_) {
        fallbackTimer_.start();
    } else {
        feeding_.store(true);
        feeder_ = std::thread([this] {
            while (feeding_.load()) {
                engine_.fillAhead();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
    playing_ = true;
    emit playingChanged(true);
}

void Player::pause() {
    if (!playing_) {
        return;
    }
    pausedFrame_ = currentFrame();
    output_.stop();
    stopFeeder();
    playing_ = false;
    emit playingChanged(false);
}

void Player::stopFeeder() {
    feeding_.store(false);
    if (feeder_.joinable()) {
        feeder_.join();
    }
}

bool Player::isPlaying() const {
    return playing_;
}

void Player::seek(std::int64_t frame) {
    const bool wasPlaying = playing_;
    pause();
    pausedFrame_ = std::max<std::int64_t>(0, frame);
    if (wasPlaying) {
        play();
    }
}

std::int64_t Player::currentFrame() const {
    if (!playing_) {
        return pausedFrame_;
    }
    if (fallback_) {
        const core::Rational fps = engine_.fps();
        return pausedFrame_ + fallbackTimer_.elapsed() * fps.num / (1000 * fps.den);
    }
    return engine_.currentFrame();
}

bool Player::usingFallbackClock() const {
    return playing_ && fallback_;
}

} // namespace mcs::playback
