#pragma once
#include "core/RingBuffer.h"
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <wrl/client.h>
#include <atomic>
#include <thread>
#include <string>

using Microsoft::WRL::ComPtr;

class AudioRenderer {
public:
    AudioRenderer();
    ~AudioRenderer();

    // initialize renderer for a specific output device
    bool init(const std::wstring& deviceId, WAVEFORMATEX* sourceFormat);

    bool start(RingBuffer* inputBuffer);
    void stop();

    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    void setVolume(float vol) { volume_.store(std::clamp(vol, 0.0f, 1.0f), std::memory_order_release); }
    float getVolume() const { return volume_.load(std::memory_order_acquire); }

    void setMuted(bool muted) { muted_.store(muted, std::memory_order_release); }
    bool isMuted() const { return muted_.load(std::memory_order_acquire); }

    const std::wstring& getDeviceId() const { return deviceId_; }

private:
    void renderLoop();
    void applyVolume(uint8_t* data, size_t bytes, WAVEFORMATEX* fmt);

    std::wstring deviceId_;
    ComPtr<IMMDevice> device_;
    ComPtr<IAudioClient> audioClient_;
    ComPtr<IAudioRenderClient> renderClient_;
    WAVEFORMATEX* format_ = nullptr;
    UINT32 bufferFrameCount_ = 0;

    // each renderer tracks its own read position in the shared ring buffer
    // this allows multiple renderers to read from the same capture buffer
    RingBuffer* inputBuffer_ = nullptr;
    size_t readPos_ = 0;

    std::thread renderThread_;
    std::atomic<bool> running_{false};
    std::atomic<float> volume_{1.0f};
    std::atomic<bool> muted_{false};
};
