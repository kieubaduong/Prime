#pragma once

#include <Windows.h>

#include <optional>
#include <string_view>

namespace logger {

void enable_virtual_terminal();
void info(std::string_view message, std::optional<HRESULT> hr = std::nullopt);
void warn(std::string_view message, std::optional<HRESULT> hr = std::nullopt);
void error(std::string_view message, std::optional<HRESULT> hr = std::nullopt);

}  // namespace logger
