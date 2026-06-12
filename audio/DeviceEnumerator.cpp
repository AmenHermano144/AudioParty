#include "DeviceEnumerator.h"
#include <cstdio>

DeviceEnumerator::DeviceEnumerator() {
    auto hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        IID_PPV_ARGS(&enumerator_)
    );
    if (FAILED(hr)) {
        printf("[!] failed to create device enumerator: 0x%08X\n", hr);
    }
}

bool DeviceEnumerator::refresh() {
    devices_.clear();
    if (!enumerator_) return false;

    // get default device id for comparison
    std::wstring defaultId;
    {
        ComPtr<IMMDevice> defaultDev;
        if (SUCCEEDED(enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDev))) {
            LPWSTR id = nullptr;
            if (SUCCEEDED(defaultDev->GetId(&id))) {
                defaultId = id;
                CoTaskMemFree(id);
            }
        }
    }

    ComPtr<IMMDeviceCollection> collection;
    auto hr = enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
    if (FAILED(hr)) return false;

    UINT count = 0;
    collection->GetCount(&count);

    for (UINT i = 0; i < count; i++) {
        ComPtr<IMMDevice> device;
        if (FAILED(collection->Item(i, &device))) continue;

        LPWSTR id = nullptr;
        if (FAILED(device->GetId(&id))) continue;

        ComPtr<IPropertyStore> props;
        if (FAILED(device->OpenPropertyStore(STGM_READ, &props))) {
            CoTaskMemFree(id);
            continue;
        }

        PROPVARIANT name;
        PropVariantInit(&name);
        if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &name))) {
            AudioDevice dev;
            dev.id = id;
            dev.name = name.pwszVal ? name.pwszVal : L"Unknown";
            dev.isDefault = (dev.id == defaultId);
            devices_.push_back(std::move(dev));
        }
        PropVariantClear(&name);
        CoTaskMemFree(id);
    }

    return true;
}

const AudioDevice* DeviceEnumerator::getDeviceByIndex(int index) const {
    if (index < 0 || index >= static_cast<int>(devices_.size()))
        return nullptr;
    return &devices_[index];
}

const AudioDevice* DeviceEnumerator::getDefaultDevice() const {
    for (auto& dev : devices_) {
        if (dev.isDefault) return &dev;
    }
    return devices_.empty() ? nullptr : &devices_[0];
}
