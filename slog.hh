#pragma once

#include <cstdio>
#include <string>
#include <format>

// lightweight logging shim for the gui framework
// replace this with your own logging system when integrating into another project

namespace slog
{
	namespace log
	{
		template <typename... Args>
		inline void info(std::format_string<Args...> fmt, Args&&... args) {
			auto msg = std::format(fmt, std::forward<Args>(args)...);
			printf("[info] %s\n", msg.c_str());
		}

		template <typename... Args>
		inline void error(std::format_string<Args...> fmt, Args&&... args) {
			auto msg = std::format(fmt, std::forward<Args>(args)...);
			printf("[error] %s\n", msg.c_str());
		}

		template <typename... Args>
		inline void success(std::format_string<Args...> fmt, Args&&... args) {
			auto msg = std::format(fmt, std::forward<Args>(args)...);
			printf("[ok] %s\n", msg.c_str());
		}

		template <typename... Args>
		inline void debug(std::format_string<Args...> fmt, Args&&... args) {
			auto msg = std::format(fmt, std::forward<Args>(args)...);
			printf("[dbg] %s\n", msg.c_str());
		}
	}
}
