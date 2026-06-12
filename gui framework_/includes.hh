#pragma once

// std
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cstdint>
#include <locale>
#include <codecvt>

// windows
#include <Windows.h>

// imgui
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/freetype/imgui_freetype.h"

// d3d11
#include <d3d11.h>

// logging shim - replace with your own logger when integrating
#include "../slog.hh"

// framework internals
#include "math/math.hh"
#include "animations/animations.hh"
#include "render/render.hh"
#include "framework/context/context.hh"
#include "framework/gui/base_element/base_element.hh"
#include "framework/gui/keybind/keybind.hh"
#include "framework/gui/checkbox/checkbox.hh"
#include "framework/gui/slider/slider.hh"
#include "framework/gui/dropdown/dropdown.hh"
#include "framework/gui/multibox/multibox.hh"
#include "framework/gui/button/button.hh"
#include "framework/gui/colorpicker/colorpicker.hh"
#include "framework/gui/textinput/textinput.hh"
#include "framework/gui/listbox/listbox.hh"
#include "framework/gui/popup/popup.hh"
#include "framework/gui/tab/tab.hh"
#include "framework/gui/child/child.hh"
#include "framework/gui/window/window.hh"
#include "framework/menu.hh"
