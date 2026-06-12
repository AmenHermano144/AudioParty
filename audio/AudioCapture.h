#pragma once
#include "core/RingBuffer.h"
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <wrl/client.h>
#include <atomic>
#include <thread>
#include <functional>
#include <string>

using Microsoft::WRL::ComPtr;

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    // initialize capture on the default output device (loopback)
    bool init(const std::wstring& deviceId = L"");

    // start/stop the capture thread
    bool start(RingBuffer* outputBuffer);
    void stop();

    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    // audio format info (available after init)
    WAVEFORMATEX* getFormat() const { return format_; }
    uint32_t getSampleRate() const { return format_ ? format_->nSamplesPerSec : 0; }
    uint16_t getChannels() const { return format_ ? format_->nChannels : 0; }
    uint16_t getBitsPerSample() const { return format_ ? format_->wBitsPerSample : 0; }

private:
    void captureLoop();

    ComPtr<IMMDevice> device_;
    ComPtr<IAudioClient> audioClient_;
    ComPtr<IAudioCaptureClient> captureClient_;
    WAVEFORMATEX* format_ = nullptr;

    RingBuffer* outputBuffer_ = nullptr;
    std::thread captureThread_;
    std::atomic<bool> running_{false};
};
