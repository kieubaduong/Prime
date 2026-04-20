#pragma once

#include <Windows.h>

#include <string>
#include <string_view>

namespace Log {

void Info(std::string_view message);
void Error(std::string_view message);
std::string HRMessage(HRESULT hr);

}  // namespace Log
