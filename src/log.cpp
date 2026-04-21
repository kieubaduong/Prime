#include "../include/log.hpp"

#include <Windows.h>

#include <format>
#include <iostream>
#include <string>
#include <string_view>

void Log::Info(std::string_view message) {
  std::cout << message << '\n';
}

void Log::Error(std::string_view message) {
  std::cerr << message << '\n';
}

std::string Log::HRMessage(HRESULT hr) {
  char* buf = nullptr;
  DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                             static_cast<DWORD>(hr), MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&buf), 0, nullptr);

  std::string msg;
  if (len > 0 && buf) {
    // trim trailing \r\n
    while (len > 0 && (buf[len - 1] == '\r' || buf[len - 1] == '\n')) {
      len--;
    }
    msg.assign(buf, len);
    LocalFree(buf);
  }
  else {
    msg = "Unknown error";
  }

  return std::format("{} (hr={:#010x})", msg, static_cast<unsigned>(hr));
}
