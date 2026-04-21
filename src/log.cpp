#include "../include/log.hpp"

#include <Windows.h>

#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

std::string ToMessage(HRESULT hr) {
  char* buffer = nullptr;
  DWORD length = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                                static_cast<DWORD>(hr), MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

  std::string message;
  if (length > 0 && buffer) {
    while (length > 0 && (buffer[length - 1] == '\r' || buffer[length - 1] == '\n')) {
      length--;
    }
    message.assign(buffer, length);
    LocalFree(buffer);
  }
  else {
    message = "Unknown error";
  }

  return std::format("{} (hr={:#010x})", message, static_cast<unsigned>(hr));
}

std::string Format(std::string_view message, std::optional<HRESULT> hr) {
  if (hr.has_value()) {
    return std::format("{}. Handle result message: {}", message, ToMessage(*hr));
  }
  return std::string(message);
}

}  // namespace

void logger::enable_virtual_terminal() {
  auto enable = [](DWORD which) {
    HANDLE handle = GetStdHandle(which);
    DWORD mode = 0;

    if (GetConsoleMode(handle, &mode)) {
      SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
  };

  enable(STD_OUTPUT_HANDLE);
  enable(STD_ERROR_HANDLE);
}

void logger::info(std::string_view message, std::optional<HRESULT> hr) {
  std::cout << "\o{33}[32m[INFO]\o{33}[0m " << Format(message, hr) << '\n';
}

void logger::warn(std::string_view message, std::optional<HRESULT> hr) {
  std::cout << "\o{33}[33m[WARN]\o{33}[0m " << Format(message, hr) << '\n';
}

void logger::error(std::string_view message, std::optional<HRESULT> hr) {
  std::cerr << "\o{33}[31m[ERROR]\o{33}[0m " << Format(message, hr) << '\n';
}
