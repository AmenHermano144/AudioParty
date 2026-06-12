#pragma once
#include "AudioCapture.h"
#include "AudioRenderer.h"
#include "DeviceEnumerator.h"
#include "core/RingBuffer.h"
#include <memory>
#include <vector>
#include <string>
#include <mutex>

struct ChannelInfo {
    int index;
    std::wstring deviceId;
    std::wstring deviceName;
    float volume;
    bool muted;
    bool active;
};

class AudioRouter {
public:
    AudioRouter();
    ~AudioRouter();

    bool init();
    void shutdown();

    // device management
    bool refreshDevices();
    const std::vector<AudioDevice>& getAvailableDevices() const;

    // channel management
    int addChannel(const std::wstring& deviceId);
    bool removeChannel(int channelIndex);
    bool setChannelVolume(int channelIndex, float volume);
    bool toggleChannelMute(int channelIndex);

    std::vector<ChannelInfo> getChannels() const;

    bool isRunning() const { return capture_ && capture_->isRunning(); }

    // capture source device (the one we're grabbing audio from)
    bool setCaptureDevice(const std::wstring& deviceId);

private:
    DeviceEnumerator enumerator_;
    std::unique_ptr<AudioCapture> capture_;
    std::unique_ptr<RingBuffer> ringBuffer_;

    struct Channel {
        int index;
        std::wstring deviceId;
        std::wstring deviceName;
        std::unique_ptr<AudioRenderer> renderer;
    };

    std::vector<Channel> channels_;
    mutable std::mutex channelsMutex_;
    int nextChannelIndex_ = 1;
    bool initialized_ = false;
};
