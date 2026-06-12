#include "AudioRouter.h"
#include <cstdio>
#include <algorithm>

// 2MB ring buffer - enough for ~2.8 seconds at 48kHz/32bit/stereo
static constexpr size_t RING_BUFFER_SIZE = 2 * 1024 * 1024;

AudioRouter::AudioRouter() {}

AudioRouter::~AudioRouter() {
    shutdown();
}

bool AudioRouter::init() {
    if (initialized_) return true;

    enumerator_.refresh();

    capture_ = std::make_unique<AudioCapture>();
    if (!capture_->init()) {
        printf("[!] router: failed to initialize capture\n");
        return false;
    }

    ringBuffer_ = std::make_unique<RingBuffer>(RING_BUFFER_SIZE);

    if (!capture_->start(ringBuffer_.get())) {
        printf("[!] router: failed to start capture\n");
        return false;
    }

    initialized_ = true;
    printf("[+] router initialized and capturing\n");
    return true;
}

void AudioRouter::shutdown() {
    {
        std::lock_guard lock(channelsMutex_);
        for (auto& ch : channels_) {
            if (ch.renderer) ch.renderer->stop();
        }
        channels_.clear();
    }

    if (capture_) capture_->stop();
    capture_.reset();
    ringBuffer_.reset();
    initialized_ = false;
}

bool AudioRouter::refreshDevices() {
    return enumerator_.refresh();
}

const std::vector<AudioDevice>& AudioRouter::getAvailableDevices() const {
    return enumerator_.getOutputDevices();
}

int AudioRouter::addChannel(const std::wstring& deviceId) {
    if (!initialized_ || !capture_->getFormat()) return -1;

    // check if device is already added
    {
        std::lock_guard lock(channelsMutex_);
        for (auto& ch : channels_) {
            if (ch.deviceId == deviceId) {
                printf("[!] device already has a channel\n");
                return -1;
            }
        }
    }

    // find device name
    std::wstring name = L"Unknown";
    for (auto& dev : enumerator_.getOutputDevices()) {
        if (dev.id == deviceId) {
            name = dev.name;
            break;
        }
    }

    auto renderer = std::make_unique<AudioRenderer>();
    if (!renderer->init(deviceId, capture_->getFormat())) {
        printf("[!] router: failed to init renderer for channel\n");
        return -1;
    }

    if (!renderer->start(ringBuffer_.get())) {
        printf("[!] router: failed to start renderer\n");
        return -1;
    }

    std::lock_guard lock(channelsMutex_);
    auto idx = nextChannelIndex_++;
    channels_.push_back({idx, deviceId, name, std::move(renderer)});
    return idx;
}

bool AudioRouter::removeChannel(int channelIndex) {
    std::lock_guard lock(channelsMutex_);
    auto it = std::find_if(channels_.begin(), channels_.end(),
        [channelIndex](const Channel& ch) { return ch.index == channelIndex; });

    if (it == channels_.end()) return false;

    if (it->renderer) it->renderer->stop();
    channels_.erase(it);
    return true;
}

bool AudioRouter::setChannelVolume(int channelIndex, float volume) {
    std::lock_guard lock(channelsMutex_);
    for (auto& ch : channels_) {
        if (ch.index == channelIndex) {
            ch.renderer->setVolume(volume);
            return true;
        }
    }
    return false;
}

bool AudioRouter::toggleChannelMute(int channelIndex) {
    std::lock_guard lock(channelsMutex_);
    for (auto& ch : channels_) {
        if (ch.index == channelIndex) {
            ch.renderer->setMuted(!ch.renderer->isMuted());
            return true;
        }
    }
    return false;
}

std::vector<ChannelInfo> AudioRouter::getChannels() const {
    std::lock_guard lock(channelsMutex_);
    std::vector<ChannelInfo> result;
    for (auto& ch : channels_) {
        result.push_back({
            ch.index,
            ch.deviceId,
            ch.deviceName,
            ch.renderer->getVolume(),
            ch.renderer->isMuted(),
            ch.renderer->isRunning()
        });
    }
    return result;
}

bool AudioRouter::setCaptureDevice(const std::wstring& deviceId) {
    // stop everything, reinit capture on new device, restart channels
    std::vector<std::pair<std::wstring, float>> savedChannels;
    {
        std::lock_guard lock(channelsMutex_);
        for (auto& ch : channels_) {
            savedChannels.push_back({ch.deviceId, ch.renderer->getVolume()});
            ch.renderer->stop();
        }
        channels_.clear();
    }

    capture_->stop();
    ringBuffer_->reset();

    capture_ = std::make_unique<AudioCapture>();
    if (!capture_->init(deviceId)) return false;
    if (!capture_->start(ringBuffer_.get())) return false;

    // restore channels
    for (auto& [devId, vol] : savedChannels) {
        auto idx = addChannel(devId);
        if (idx > 0) setChannelVolume(idx, vol);
    }

    return true;
}
