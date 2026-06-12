#pragma once
#include <string>
#include <vector>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct AudioDevice {
    std::wstring id;
    std::wstring name;
    bool isDefault = false;
};

class DeviceEnumerator {
public:
    DeviceEnumerator();
    ~DeviceEnumerator() = default;

    // refresh the list of available output devices
    bool refresh();

    const std::vector<AudioDevice>& getOutputDevices() const { return devices_; }
    const AudioDevice* getDeviceByIndex(int index) const;
    const AudioDevice* getDefaultDevice() const;

private:
    ComPtr<IMMDeviceEnumerator> enumerator_;
    std::vector<AudioDevice> devices_;
};
