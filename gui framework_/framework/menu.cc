#include "../includes.hh"
#include "../../audio/AudioRouter.h"

extern AudioRouter* getRouter();

static constexpr int MAX_CHANNELS = 16;

// per-slot state
static int slot_volume[MAX_CHANNELS]{ 0 };
static int slot_device_sel[MAX_CHANNELS]{ 0 };
static int slot_device_sel_prev[MAX_CHANNELS]{ -1 };
static bool slot_visible[MAX_CHANNELS]{ false };
static int slot_router_idx[MAX_CHANNELS]{ 0 };
static int active_count = 0;

// per-slot widgets
static std::shared_ptr<framework::c_dropdown> slot_dropdown[MAX_CHANNELS];
static std::shared_ptr<framework::c_slider_int> slot_slider[MAX_CHANNELS];
static std::shared_ptr<framework::c_button> slot_remove_btn[MAX_CHANNELS];

// device list
static std::vector<std::string> device_names;
static std::vector<std::wstring> device_ids;

static std::string wstr_to_str(const std::wstring& ws) {
    if (ws.empty()) return {};
    int sz = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()), nullptr, 0, nullptr, nullptr);
    std::string out(sz, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()), out.data(), sz, nullptr, nullptr);
    return out;
}

static void refresh_device_list() {
    auto* router = getRouter();
    if (!router) return;

    router->refreshDevices();
    auto& devs = router->getAvailableDevices();

    device_names.clear();
    device_ids.clear();

    for (auto& d : devs) {
        auto name = wstr_to_str(d.name);
        if (d.isDefault)
            name += " (source)";
        device_names.push_back(name);
        device_ids.push_back(d.id);
    }

    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (slot_dropdown[i])
            slot_dropdown[i]->items() = device_names;
    }
}

static int find_device_index(const std::wstring& deviceId) {
    for (int d = 0; d < static_cast<int>(device_ids.size()); d++) {
        if (device_ids[d] == deviceId)
            return d;
    }
    return -1;
}

static void add_channel() {
    auto* router = getRouter();
    if (!router || device_ids.empty() || active_count >= MAX_CHANNELS) return;

    // pick first non-source device
    int pick = 0;
    for (int i = 0; i < static_cast<int>(device_ids.size()); i++) {
        if (device_names[i].find("(source)") == std::string::npos) {
            pick = i;
            break;
        }
    }

    int idx = router->addChannel(device_ids[pick]);
    if (idx > 0) {
        // pre-init the new slot so it's ready on the next frame
        int slot = active_count;
        slot_router_idx[slot] = idx;
        slot_device_sel[slot] = pick;
        slot_device_sel_prev[slot] = pick;
        slot_volume[slot] = 100;
        slot_visible[slot] = true;
        active_count++;

        router->setChannelVolume(idx, 1.0f);
        slog::log::success("added channel {}: {}", idx, device_names[pick]);
    } else {
        slog::log::error("failed to add channel for: {}", device_names[pick]);
    }
}

static void remove_channel(int slot) {
    auto* router = getRouter();
    if (!router || slot < 0 || slot >= active_count) return;

    router->removeChannel(slot_router_idx[slot]);

    // shift remaining slots down
    for (int i = slot; i < active_count - 1; i++) {
        slot_router_idx[i] = slot_router_idx[i + 1];
        slot_device_sel[i] = slot_device_sel[i + 1];
        slot_device_sel_prev[i] = slot_device_sel_prev[i + 1];
        slot_volume[i] = slot_volume[i + 1];
    }

    active_count--;
    slot_visible[active_count] = false;

    slog::log::success("removed channel from slot {}", slot);
}

namespace framework
{
    void c_menu::initialize()
    {
        slog::log::info("initializing AudioParty menu");

        refresh_device_list();

        auto window = std::make_shared<c_window>("AudioParty", math::c_vector_2d(0, 0), math::c_vector_2d(680, 520));
        {
            window->prebuild_tabs([](framework::c_tab* controller) {
                controller->create_tab("Mixer", {});
            });
            window->finish_tab_prebuild();

            window->build_child("Controls", framework::child_width::full, 75, [](framework::c_child* controller) {
                controller->attach_child("Mixer", "");

                controller->add_button("+  Add Output", []() {
                    add_channel();
                });
            });

            window->build_child("Channels", framework::child_width::full, 345, [](framework::c_child* controller) {
                controller->attach_child("Mixer", "");

                for (int i = 0; i < MAX_CHANNELS; i++) {
                    std::string label_dd = "Output " + std::to_string(i + 1);
                    std::string label_vol = "Volume " + std::to_string(i + 1);
                    std::string label_rm = "Remove " + std::to_string(i + 1);

                    auto dd = controller->add_dropdown(label_dd, &slot_device_sel[i], device_names);
                    dd->set_callback_visiblity(&slot_visible[i]);
                    slot_dropdown[i] = dd;

                    auto sl = controller->add_slider_int(label_vol, &slot_volume[i], 0, 100);
                    sl->set_callback_visiblity(&slot_visible[i]);
                    slot_slider[i] = sl;

                    int slot = i;
                    auto btn = controller->add_button(label_rm, [slot]() {
                        remove_channel(slot);
                    });
                    btn->set_callback_visiblity(&slot_visible[i]);
                    slot_remove_btn[i] = btn;
                }
            });

            this->m_windows.push_back(window);
        }
    }

    void c_menu::runtime()
    {
        g_ctx->m_click_consumed = false;

        auto* router = getRouter();
        if (router) {
            // sync from router to detect externally removed channels
            auto channels = router->getChannels();
            int router_count = static_cast<int>(channels.size());

            // rebuild slot state if router count differs from our tracked count
            if (router_count != active_count) {
                active_count = std::min(router_count, MAX_CHANNELS);
                for (int i = 0; i < active_count; i++) {
                    slot_router_idx[i] = channels[i].index;
                    slot_visible[i] = true;

                    int dev_idx = find_device_index(channels[i].deviceId);
                    if (dev_idx >= 0) {
                        slot_device_sel[i] = dev_idx;
                        slot_device_sel_prev[i] = dev_idx;
                    }

                    slot_volume[i] = static_cast<int>(channels[i].volume * 100.0f);
                }
                for (int i = active_count; i < MAX_CHANNELS; i++)
                    slot_visible[i] = false;
            }

            // per-slot: detect user changes and push to router
            for (int i = 0; i < active_count; i++) {
                // user changed device via dropdown
                if (slot_device_sel[i] != slot_device_sel_prev[i]) {
                    if (slot_device_sel[i] >= 0 && slot_device_sel[i] < static_cast<int>(device_ids.size())) {
                        auto new_id = device_ids[slot_device_sel[i]];
                        router->removeChannel(slot_router_idx[i]);
                        int new_idx = router->addChannel(new_id);
                        if (new_idx > 0) {
                            slot_router_idx[i] = new_idx;
                            slot_device_sel_prev[i] = slot_device_sel[i];
                            router->setChannelVolume(new_idx, static_cast<float>(slot_volume[i]) / 100.0f);
                            slog::log::success("swapped channel to: {}", device_names[slot_device_sel[i]]);
                        } else {
                            // swap failed, revert dropdown
                            slot_device_sel[i] = slot_device_sel_prev[i];
                            slog::log::error("failed to swap device");
                        }
                    }
                    continue;
                }

                // push volume changes
                float ui_vol = static_cast<float>(slot_volume[i]) / 100.0f;
                router->setChannelVolume(slot_router_idx[i], ui_vol);
            }
        }

        for (auto& w : this->m_windows)
        {
            w->input();
            w->paint();
        }
    }
}
