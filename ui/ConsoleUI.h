#pragma once
#include "audio/AudioRouter.h"

class ConsoleUI {
public:
    ConsoleUI() = default;
    ~ConsoleUI() = default;

    void run();

private:
    void showBanner();
    void showHelp();
    void showDevices();
    void showChannels();
    void handleAddChannel();
    void handleRemoveChannel();
    void handleVolume();
    void handleMute();

    AudioRouter router_;
};
