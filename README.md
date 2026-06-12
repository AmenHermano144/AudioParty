# AudioParty

A Windows utility that lets multiple headphones or speakers play the same system audio at the same time. Think of it as a "master audio device" that captures whatever your PC is playing and fans it out to as many outputs as you want — USB, Bluetooth, 3.5mm, all mixed together.

## Why

Windows only routes audio to one output device at a time. If you want two friends sharing a movie on one PC, or you want sound on both your speakers and your headphones, there is no clean way to do it. AudioParty solves that.

## Features

- Capture system audio via WASAPI loopback (no virtual cable, no driver install)
- Route to up to 8 simultaneous output devices
- Per-channel volume control
- Low-latency, lock-free audio path — no clicks or stutters between outputs
- Single-window GUI: pick a device, set a volume, done

## Tech Stack

- **Language:** C++20
- **Audio:** WASAPI loopback capture, MMDevice API for enumeration
- **Threading:** Lock-free SPSC ring buffer with multi-consumer `peek()` so each renderer reads independently
- **Render:** One dedicated render thread per output device, each with its own read position
- **GUI:** Custom widget framework on top of Dear ImGui (DX11 backend), FreeType for font rendering
- **Build:** CMake + Visual Studio 2022

## Project Layout

```
src/
  main.cpp                  entry point
  core/
    RingBuffer.h            lock-free SPSC ring buffer
  audio/
    DeviceEnumerator.*      MMDevice wrapper, lists input/output devices
    AudioCapture.*          WASAPI loopback capture
    AudioRenderer.*         per-device render thread
    AudioRouter.*           glues capture -> ring buffer -> N renderers
  gui framework_/           custom ImGui-based widget framework
    framework/gui/          window, child, tab, button, dropdown, slider, etc.
    render/                 DX11 render helpers, fonts
    animations/             shared animation globals per widget type
CMakeLists.txt
```

## Build

Requires:
- Visual Studio 2022 with C++ workload
- CMake 3.20+
- `vcpkg install freetype:x64-windows`
- A local copy of Dear ImGui (path configured at the top of `CMakeLists.txt` via `IMGUI_DIR`)

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The executable lands in `build/Release/AudioParty.exe`.

## Usage

1. Launch `AudioParty.exe`.
2. In the **Mixer** tab, press **+ Add Output**.
3. Pick a device from the dropdown and set its volume.
4. Repeat for each headphone/speaker you want included.
5. Play audio on Windows as normal — every selected output gets it.

## Architecture Notes

The audio path is a single producer / many consumers:

```
  Windows audio engine
          |
          v
   AudioCapture (WASAPI loopback)
          |
          v
     RingBuffer  ──┬── AudioRenderer (device A)
                   ├── AudioRenderer (device B)
                   └── AudioRenderer (device N)
```

Each `AudioRenderer` owns its own read cursor into the ring buffer via `peek()`, so renderers never block each other and the capture thread never waits on a slow output. The buffer is sized to absorb scheduling jitter between devices without dropping samples.

## Roadmap

- [x] Console CLI prototype
- [x] GUI (custom ImGui framework)
- [ ] Per-channel EQ / delay compensation
- [ ] Save/restore device profiles
- [ ] Mobile companion (long-term)

## Status

Active personal project — v0.2. Windows-only by design.
"# AudioParty" 
