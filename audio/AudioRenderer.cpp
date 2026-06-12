#include "AudioRenderer.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <cmath>

static constexpr REFERENCE_TIME BUFFER_DURATION = 200000; // 20ms

AudioRenderer::AudioRenderer() {}

AudioRenderer::~AudioRenderer() {
    stop();
    if (format_) {
        CoTaskMemFree(format_);
        format_ = nullptr;
    }
}

bool AudioRenderer::init(const std::wstring& deviceId, WAVEFORMATEX* sourceFormat) {
    deviceId_ = deviceId;

    ComPtr<IMMDeviceEnumerator> enumerator;
    auto hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        IID_PPV_ARGS(&enumerator)
    );
    if (FAILED(hr)) return false;

    hr = enumerator->GetDevice(deviceId.c_str(), &device_);
    if (FAILED(hr)) {
        printf("[!] renderer: failed to get device: 0x%08X\n", hr);
        return false;
    }

    hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                           reinterpret_cast<void**>(audioClient_.GetAddressOf()));
    if (FAILED(hr)) {
        printf("[!] renderer: failed to activate audio client: 0x%08X\n", hr);
        return false;
    }

    // check if the device supports the source format directly
    WAVEFORMATEX* closestMatch = nullptr;
    hr = audioClient_->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, sourceFormat, &closestMatch);

    WAVEFORMATEX* useFormat = sourceFormat;
    if (hr == S_FALSE && closestMatch) {
        useFormat = closestMatch;
        printf("[*] renderer: using closest matching format\n");
    } else if (FAILED(hr)) {
        // fall back to the device's mix format
        hr = audioClient_->GetMixFormat(&format_);
        if (FAILED(hr)) return false;
        useFormat = format_;
        printf("[*] renderer: using device mix format\n");
    }

    hr = audioClient_->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0, BUFFER_DURATION, 0, useFormat, nullptr
    );
    if (FAILED(hr)) {
        printf("[!] renderer: failed to initialize: 0x%08X\n", hr);
        if (closestMatch) CoTaskMemFree(closestMatch);
        return false;
    }

    // store the format we're actually using
    if (!format_) {
        auto size = sizeof(WAVEFORMATEX) + useFormat->cbSize;
        format_ = static_cast<WAVEFORMATEX*>(CoTaskMemAlloc(size));
        std::memcpy(format_, useFormat, size);
    }
    if (closestMatch) CoTaskMemFree(closestMatch);

    hr = audioClient_->GetBufferSize(&bufferFrameCount_);
    if (FAILED(hr)) return false;

    hr = audioClient_->GetService(IID_PPV_ARGS(&renderClient_));
    if (FAILED(hr)) {
        printf("[!] renderer: failed to get render client: 0x%08X\n", hr);
        return false;
    }

    return true;
}

bool AudioRenderer::start(RingBuffer* inputBuffer) {
    if (running_) return false;
    if (!renderClient_) return false;

    inputBuffer_ = inputBuffer;
    readPos_ = inputBuffer->getWritePos(); // start from current position

    auto hr = audioClient_->Start();
    if (FAILED(hr)) {
        printf("[!] renderer: failed to start: 0x%08X\n", hr);
        return false;
    }

    running_.store(true, std::memory_order_release);
    renderThread_ = std::thread(&AudioRenderer::renderLoop, this);
    return true;
}

void AudioRenderer::stop() {
    running_.store(false, std::memory_order_release);
    if (renderThread_.joinable())
        renderThread_.join();
    if (audioClient_)
        audioClient_->Stop();
}

void AudioRenderer::renderLoop() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    while (running_.load(std::memory_order_acquire)) {
        // how many frames can we write right now?
        UINT32 padding = 0;
        auto hr = audioClient_->GetCurrentPadding(&padding);
        if (FAILED(hr)) break;

        auto framesAvailable = bufferFrameCount_ - padding;
        if (framesAvailable == 0) {
            Sleep(1);
            continue;
        }

        auto bytesNeeded = framesAvailable * format_->nBlockAlign;

        BYTE* renderBuffer = nullptr;
        hr = renderClient_->GetBuffer(framesAvailable, &renderBuffer);
        if (FAILED(hr)) {
            Sleep(1);
            continue;
        }

        // read from ring buffer using our independent read position
        auto bytesRead = inputBuffer_->peek(renderBuffer, bytesNeeded, readPos_);

        if (bytesRead > 0) {
            readPos_ += bytesRead;
            auto framesRead = static_cast<UINT32>(bytesRead / format_->nBlockAlign);

            // apply volume/mute
            if (muted_.load(std::memory_order_acquire)) {
                std::memset(renderBuffer, 0, bytesRead);
            } else {
                applyVolume(renderBuffer, bytesRead, format_);
            }

            // pad remaining frames with silence if we didn't fill the buffer
            if (framesRead < framesAvailable) {
                std::memset(renderBuffer + bytesRead, 0,
                           (framesAvailable - framesRead) * format_->nBlockAlign);
            }

            renderClient_->ReleaseBuffer(framesAvailable, 0);
        } else {
            // no data available, write silence
            renderClient_->ReleaseBuffer(framesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
        }

        Sleep(2);
    }

    CoUninitialize();
}

void AudioRenderer::applyVolume(uint8_t* data, size_t bytes, WAVEFORMATEX* fmt) {
    auto vol = volume_.load(std::memory_order_acquire);
    if (vol >= 0.99f) return; // no adjustment needed

    // handle IEEE float format (most common for WASAPI shared mode)
    if (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
        (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE && fmt->wBitsPerSample == 32)) {
        auto samples = reinterpret_cast<float*>(data);
        auto count = bytes / sizeof(float);
        for (size_t i = 0; i < count; i++) {
            samples[i] *= vol;
        }
    }
    // handle 16-bit PCM
    else if (fmt->wBitsPerSample == 16) {
        auto samples = reinterpret_cast<int16_t*>(data);
        auto count = bytes / sizeof(int16_t);
        for (size_t i = 0; i < count; i++) {
            samples[i] = static_cast<int16_t>(samples[i] * vol);
        }
    }
}
