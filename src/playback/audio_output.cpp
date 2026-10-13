#include "playback/audio_output.h"

#include "media/audio_decoder.h"
#include "playback/playback_engine.h"

#include <QCoreApplication>
#include <miniaudio.h>

namespace mcs::playback {

struct AudioOutput::Impl {
    ma_context context{};
    ma_device device{};
    bool hasContext = false;
    bool running = false;
    int latency = 0;
};

namespace {
void onAudio(ma_device* device, void* output, const void*, ma_uint32 frames) {
    static_cast<PlaybackEngine*>(device->pUserData)
        ->pull(static_cast<float*>(output), static_cast<int>(frames));
}
} // namespace

AudioOutput::AudioOutput(PlaybackEngine& engine) : engine_(engine), d_(std::make_unique<Impl>()) {}

AudioOutput::~AudioOutput() {
    stop();
}

bool AudioOutput::start(Backend backend, QString* error) {
    stop();
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
    config.pUserData = &engine_;
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
        ma_device_uninit(&d_->device); // para e espera o callback terminar
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

} // namespace mcs::playback
