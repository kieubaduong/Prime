#include "../include/monitor_picker.hpp"

#include <Windows.h>
#include <conio.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wchar.h>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "../include/log.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace {

struct MonitorInfo {
  UINT adapterIndex;
  UINT monitorIndex;
  std::string deviceName;
  int width;
  int height;
  int x;
  int y;
};

std::vector<MonitorInfo> EnumerateMonitors() {
  IDXGIFactory1* factory = nullptr;
  HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
  if (FAILED(hr)) {
    logger::error("monitor_picker: failed to create DXGI factory", hr);
    return {};
  }

  std::vector<MonitorInfo> monitors;
  for (UINT adapterIndex = 0;; ++adapterIndex) {
    IDXGIAdapter1* adapter = nullptr;
    if (factory->EnumAdapters1(adapterIndex, &adapter) != S_OK) {
      break;
    }

    for (UINT monitorIndex = 0;; ++monitorIndex) {
      IDXGIOutput* output = nullptr;
      if (adapter->EnumOutputs(monitorIndex, &output) != S_OK) {
        break;
      }

      DXGI_OUTPUT_DESC desc{};
      output->GetDesc(&desc);
      output->Release();

      MonitorInfo info;
      info.adapterIndex = adapterIndex;
      info.monitorIndex = monitorIndex;
      info.deviceName.assign(desc.DeviceName, desc.DeviceName + wcslen(desc.DeviceName));
      info.x = desc.DesktopCoordinates.left;
      info.y = desc.DesktopCoordinates.top;
      info.width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
      info.height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
      monitors.push_back(std::move(info));
    }

    adapter->Release();
  }

  factory->Release();
  return monitors;
}

void PrintList(const std::vector<MonitorInfo>& monitors, UINT selected) {
  for (UINT i = 0; i < monitors.size(); ++i) {
    const MonitorInfo& monitor = monitors[i];
    std::cout << "\o{33}[K";
    if (i == selected) {
      std::cout << "\o{33}[36m> " << monitor.deviceName << "  " << monitor.width << "x" << monitor.height << " @ (" << monitor.x << ","
                << monitor.y << ")\o{33}[0m\n";
    }
    else {
      std::cout << "  " << monitor.deviceName << "  " << monitor.width << "x" << monitor.height << " @ (" << monitor.x << "," << monitor.y
                << ")\n";
    }
  }
}

}  // namespace

monitor_picker::MonitorSelection monitor_picker::choose() {
  // _getch() returns one of these first when an extended key is pressed, then a second call returns the scan code.
  constexpr int kExtendedKeyPrefix = 0xE0;
  constexpr int kExtendedKeyPrefixAlt = 0x00;
  constexpr int kScanCodeArrowUp = 0x48;
  constexpr int kScanCodeArrowDown = 0x50;
  constexpr int kKeyEnter = '\r';

  std::vector<MonitorInfo> monitors = EnumerateMonitors();
  if (monitors.empty()) {
    logger::warn("No monitors enumerated, defaulting to adapter 0 / output 0");
    return {0, 0};
  }

  if (monitors.size() == 1) {
    return {monitors[0].adapterIndex, monitors[0].monitorIndex};
  }

  UINT selected = 0;
  std::cout << "Select a monitor (Up/Down, Enter):\n";
  PrintList(monitors, selected);

  while (true) {
    int key = _getch();
    if (key == kExtendedKeyPrefix || key == kExtendedKeyPrefixAlt) {
      int scanCode = _getch();
      if (scanCode == kScanCodeArrowUp && selected > 0) {
        --selected;
      }
      else if (scanCode == kScanCodeArrowDown && selected + 1 < monitors.size()) {
        ++selected;
      }
      else {
        continue;
      }
    }
    else if (key == kKeyEnter) {
      break;
    }
    else {
      continue;
    }

    std::cout << "\o{33}[" << monitors.size() << "F";
    PrintList(monitors, selected);
  }

  return {monitors[selected].adapterIndex, monitors[selected].monitorIndex};
}
