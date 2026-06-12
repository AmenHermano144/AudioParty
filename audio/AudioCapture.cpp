#include "AudioCapture.h"
#include <cstdio>

static constexpr REFERENCE_TIME BUFFER_DURATION = 200000; // 20ms in 100ns units

AudioCapture::AudioCapture() {}

AudioCapture::~AudioCapture() {
    stop();
    if (format_) {
        CoTaskMemFree(format_);
        format_ = nullptr;
    }
}

bool AudioCapture::init(const std::wstring& deviceId) {
    ComPtr<IMMDeviceEnumerator> enumerator;
    auto hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        IID_PPV_ARGS(&enumerator)
    );
    if (FAILED(hr)) {
        printf("[!] capture: failed to create enumerator: 0x%08X\n", hr);
        return false;
    }

    if (deviceId.empty()) {
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device_);
    } else {
        hr = enumerator->GetDevice(deviceId.c_str(), &device_);
    }
    if (FAILED(hr)) {
        printf("[!] capture: failed to get device: 0x%08X\n", hr);
        return false;
    }

    hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                           reinterpret_cast<void**>(audioClient_.GetAddressOf()));
    if (FAILED(hr)) {
        printf("[!] capture: failed to activate audio client: 0x%08X\n", hr);
        return false;
    }

    hr = audioClient_->GetMixFormat(&format_);
    if (FAILED(hr)) {
        printf("[!] capture: failed to get mix format: 0x%08X\n", hr);
        return false;
    }

    // AUDCLNT_STREAMFLAGS_LOOPBACK captures the output mix
    hr = audioClient_->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        BUFFER_DURATION, 0, format_, nullptr
    );
    if (FAILED(hr)) {
        printf("[!] capture: failed to initialize audio client: 0x%08X\n", hr);
        return false;
    }

    hr = audioClient_->GetService(IID_PPV_ARGS(&captureClient_));
    if (FAILED(hr)) {
        printf("[!] capture: failed to get capture client: 0x%08X\n", hr);
        return false;
    }

    printf("[+] capture initialized: %dHz, %dch, %dbit\n",
           format_->nSamplesPerSec, format_->nChannels, format_->wBitsPerSample);
    return true;
}

bool AudioCapture::start(RingBuffer* outputBuffer) {
    if (running_) return false;
    if (!captureClient_) return false;

    outputBuffer_ = outputBuffer;

    auto hr = audioClient_->Start();
    if (FAILED(hr)) {
        printf("[!] capture: failed to start: 0x%08X\n", hr);
        return false;
    }

    running_.store(true, std::memory_order_release);
    captureThread_ = std::thread(&AudioCapture::captureLoop, this);
    return true;
}

void AudioCapture::stop() {
    running_.store(false, std::memory_order_release);
    if (captureThread_.joinable())
        captureThread_.join();
    if (audioClient_)
        audioClient_->Stop();
}

void AudioCapture::captureLoop() {
    // each iteration: grab whatever frames WASAPI has buffered and push to ring buffer
    while (running_.load(std::memory_order_acquire)) {
        UINT32 packetLength = 0;
        auto hr = captureClient_->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) break;

        while (packetLength > 0) {
            BYTE* data = nullptr;
            UINT32 numFrames = 0;
            DWORD flags = 0;

            hr = captureClient_->GetBuffer(&data, &numFrames, &flags, nullptr, nullptr);
            if (FAILED(hr)) break;

            auto bytes = numFrames * format_->nBlockAlign;

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                // push silence
                std::vector<uint8_t> silence(bytes, 0);
                outputBuffer_->write(silence.data(), bytes);
            } else {
                outputBuffer_->write(data, bytes);
            }

            captureClient_->ReleaseBuffer(numFrames);

            hr = captureClient_->GetNextPacketSize(&packetLength);
            if (FAILED(hr)) break;
        }

        // ~5ms sleep to avoid busy-waiting while keeping latency low
        Sleep(5);
    }
}
