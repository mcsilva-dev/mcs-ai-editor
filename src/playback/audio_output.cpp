#include "playback/audio_output.h"

#include "media/audio_decoder.h"
#include "playback/playback_engine.h"

#include <QCoreApplication>
#include <atomic>
#include <miniaudio.h>

namespace mcs::playback {

namespace {
// Estado lido pelas threads do miniaudio (callback de dados e de notificação).
struct DeviceState {
    PlaybackEngine* engine = nullptr;
    std::atomic<bool> stopping{false}; // nós pedimos para parar
    std::atomic<bool> lost{false};     // parou sem pedirmos (fone desconectado, backend caiu)
};

void onAudio(ma_device* device, void* output, const void*, ma_uint32 frames) {
    static_cast<DeviceState*>(device->pUserData)
        ->engine->pull(static_cast<float*>(output), static_cast<int>(frames));
}

void onNotification(const ma_device_notification* notification) {
    auto* state = static_cast<DeviceState*>(notification->pDevice->pUserData);
    if (notification->type == ma_device_notification_type_stopped && !state->stopping.load()) {
        state->lost.store(true);
    }
}
} // namespace

struct AudioOutput::Impl {
    ma_context context{};
    ma_device device{};
    DeviceState state;
    bool hasContext = false;
    bool running = false;
    int latency = 0;
};

AudioOutput::AudioOutput(PlaybackEngine& engine) : engine_(engine), d_(std::make_unique<Impl>()) {
    d_->state.engine = &engine_;
}

AudioOutput::~AudioOutput() {
    stop();
}

bool AudioOutput::start(Backend backend, QString* error) {
    stop();
    d_->state.lost.store(false);
    auto fail = [&](const char* text) {
        if (error) {
            *error = QCoreApplication::translate("mcs::playback", text);
        }
        stop();
        return false;
    };
    if (backend == Backend::None) {
        return fail("No audio device selected.");
    }
    ma_context* context = nullptr;
    if (backend == Backend::Null) {
        const ma_backend backends[] = {ma_backend_null};
        if (ma_context_init(backends, 1, nullptr, &d_->context) != MA_SUCCESS) {
            return fail("Cannot initialize the audio system.");
        }
        d_->hasContext = true;
        context = &d_->context;
    }
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = media::AudioDecoder::kChannels;
    config.sampleRate = media::AudioDecoder::kSampleRate;
    config.dataCallback = onAudio;
    config.notificationCallback = onNotification;
    config.pUserData = &d_->state;
    if (ma_device_init(context, &config, &d_->device) != MA_SUCCESS) {
        return fail("Cannot open the audio device.");
    }
    d_->running = true; // a partir daqui stop() precisa desligar o device
    d_->latency = static_cast<int>(d_->device.playback.internalPeriodSizeInFrames *
                                   d_->device.playback.internalPeriods);
    engine_.setOutputLatency(d_->latency);
    if (ma_device_start(&d_->device) != MA_SUCCESS) {
        return fail("Cannot start the audio device.");
    }
    return true;
}

void AudioOutput::stop() {
    if (d_->running) {
        d_->state.stopping.store(true);
        ma_device_uninit(&d_->device); // para e espera o callback terminar
        d_->state.stopping.store(false);
        d_->running = false;
    }
    if (d_->hasContext) {
        ma_context_uninit(&d_->context);
        d_->hasContext = false;
    }
}

bool AudioOutput::isRunning() const {
    return d_->running;
}

int AudioOutput::latencyFrames() const {
    return d_->latency;
}

bool AudioOutput::deviceLost() const {
    return d_->state.lost.load();
}

void AudioOutput::simulateDeviceLoss() {
    if (d_->running) {
        ma_device_stop(&d_->device);
    }
}

} // namespace mcs::playback
