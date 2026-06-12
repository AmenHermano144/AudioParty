#include "ConsoleUI.h"
#include <cstdio>
#include <string>
#include <iostream>

void ConsoleUI::run() {
    showBanner();

    if (!router_.init()) {
        printf("[!] failed to initialize audio router\n");
        printf("    make sure you have at least one audio output device\n");
        printf("\npress enter to exit...");
        std::cin.get();
        return;
    }

    showHelp();

    std::string input;
    while (true) {
        printf("\naudioparty> ");
        std::getline(std::cin, input);

        if (input.empty()) continue;

        auto cmd = input[0];

        switch (cmd) {
            case 'd': case 'D':
                showDevices();
                break;
            case 'c': case 'C':
                showChannels();
                break;
            case 'a': case 'A':
                handleAddChannel();
                break;
            case 'r': case 'R':
                handleRemoveChannel();
                break;
            case 'v': case 'V':
                handleVolume();
                break;
            case 'm': case 'M':
                handleMute();
                break;
            case 'h': case 'H':
                showHelp();
                break;
            case 'q': case 'Q':
                printf("[*] shutting down...\n");
                router_.shutdown();
                return;
            default:
                printf("[?] unknown command '%c', type 'h' for help\n", cmd);
                break;
        }
    }
}

void ConsoleUI::showBanner() {
    printf("\n");
    printf("  ╔═══════════════════════════════════════╗\n");
    printf("  ║         AudioParty v0.1               ║\n");
    printf("  ║   share audio across multiple devices  ║\n");
    printf("  ╚═══════════════════════════════════════╝\n");
    printf("\n");
}

void ConsoleUI::showHelp() {
    printf("\n  commands:\n");
    printf("    d - list available audio devices\n");
    printf("    c - show active channels\n");
    printf("    a - add device as channel\n");
    printf("    r - remove channel\n");
    printf("    v - set channel volume\n");
    printf("    m - mute/unmute channel\n");
    printf("    h - show this help\n");
    printf("    q - quit\n");
}

void ConsoleUI::showDevices() {
    router_.refreshDevices();
    auto& devices = router_.getAvailableDevices();

    if (devices.empty()) {
        printf("[!] no audio output devices found\n");
        return;
    }

    printf("\n  available devices:\n");
    for (int i = 0; i < static_cast<int>(devices.size()); i++) {
        printf("    [%d] %ls%s\n", i, devices[i].name.c_str(),
               devices[i].isDefault ? " (default/source)" : "");
    }
}

void ConsoleUI::showChannels() {
    auto channels = router_.getChannels();

    if (channels.empty()) {
        printf("[*] no active channels. use 'a' to add one\n");
        return;
    }

    printf("\n  active channels:\n");
    for (auto& ch : channels) {
        printf("    [ch%d] %ls | vol: %.0f%% | %s | %s\n",
               ch.index, ch.deviceName.c_str(),
               ch.volume * 100.0f,
               ch.muted ? "MUTED" : "playing",
               ch.active ? "ok" : "error");
    }
}

void ConsoleUI::handleAddChannel() {
    showDevices();
    auto& devices = router_.getAvailableDevices();
    if (devices.empty()) return;

    printf("\n  enter device number: ");
    std::string input;
    std::getline(std::cin, input);

    int idx = -1;
    try { idx = std::stoi(input); } catch (...) { return; }

    auto* dev = router_.getAvailableDevices().data();
    if (idx < 0 || idx >= static_cast<int>(devices.size())) {
        printf("[!] invalid device number\n");
        return;
    }

    auto chIdx = router_.addChannel(devices[idx].id);
    if (chIdx > 0) {
        printf("[+] added channel %d: %ls\n", chIdx, devices[idx].name.c_str());
    } else {
        printf("[!] failed to add channel\n");
    }
}

void ConsoleUI::handleRemoveChannel() {
    showChannels();
    auto channels = router_.getChannels();
    if (channels.empty()) return;

    printf("\n  enter channel number: ");
    std::string input;
    std::getline(std::cin, input);

    int idx = -1;
    try { idx = std::stoi(input); } catch (...) { return; }

    if (router_.removeChannel(idx)) {
        printf("[+] channel %d removed\n", idx);
    } else {
        printf("[!] channel not found\n");
    }
}

void ConsoleUI::handleVolume() {
    showChannels();
    auto channels = router_.getChannels();
    if (channels.empty()) return;

    printf("\n  enter channel number: ");
    std::string input;
    std::getline(std::cin, input);

    int idx = -1;
    try { idx = std::stoi(input); } catch (...) { return; }

    printf("  enter volume (0-100): ");
    std::getline(std::cin, input);

    float vol = -1;
    try { vol = std::stof(input) / 100.0f; } catch (...) { return; }

    if (router_.setChannelVolume(idx, vol)) {
        printf("[+] channel %d volume set to %.0f%%\n", idx, vol * 100.0f);
    } else {
        printf("[!] channel not found\n");
    }
}

void ConsoleUI::handleMute() {
    showChannels();
    auto channels = router_.getChannels();
    if (channels.empty()) return;

    printf("\n  enter channel number: ");
    std::string input;
    std::getline(std::cin, input);

    int idx = -1;
    try { idx = std::stoi(input); } catch (...) { return; }

    if (router_.toggleChannelMute(idx)) {
        printf("[+] channel %d mute toggled\n", idx);
    } else {
        printf("[!] channel not found\n");
    }
}
