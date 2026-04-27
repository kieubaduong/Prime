#include "../include/screenshot_service.hpp"

#include <Windows.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_5.h>
#include <dxgiformat.h>
#include <wrl/client.h>

#include <array>
#include <optional>

#include "../include/log.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

static constexpr std::array kSupportedFormats = {
    DXGI_FORMAT_B8G8R8A8_UNORM,
};

std::optional<ScreenshotService> ScreenshotService::Create(UINT adapterIndex, UINT monitorIndex) {
  ComPtr<IDXGIFactory1> factory;
  HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()));
  if (FAILED(hr)) {
    logger::error("Failed to create DXGI factory", hr);
    return std::nullopt;
  }

  ComPtr<IDXGIAdapter1> adapter;
  hr = factory->EnumAdapters1(adapterIndex, adapter.GetAddressOf());
  if (FAILED(hr)) {
    logger::error("Failed to get adapter", hr);
    return std::nullopt;
  }

  ScreenshotService service;
  // Driver type must be UNKNOWN when an explicit adapter is passed.
  hr = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                         service.m_device.GetAddressOf(), nullptr, service.m_context.GetAddressOf());
  if (FAILED(hr)) {
    logger::error("Failed to create device and context", hr);
    return std::nullopt;
  }

  ComPtr<IDXGIOutput> output;
  hr = adapter->EnumOutputs(monitorIndex, output.GetAddressOf());
  if (FAILED(hr)) {
    logger::error("Failed to get output", hr);
    return std::nullopt;
  }

  hr = output.As(&service.m_output5);
  if (FAILED(hr)) {
    logger::error("Failed to query IDXGIOutput5", hr);
    return std::nullopt;
  }

  if (!service.CreateDuplication()) {
    return std::nullopt;
  }

  return service;
}

bool ScreenshotService::CreateDuplication() {
  HRESULT hr = m_output5->DuplicateOutput1(m_device.Get(), 0, static_cast<UINT>(kSupportedFormats.size()), kSupportedFormats.data(),
                                           m_duplication.ReleaseAndGetAddressOf());
  if (SUCCEEDED(hr)) {
    return true;
  }

  logger::warn("DuplicateOutput1 not supported, falling back to DuplicateOutput", hr);
  hr = m_output5->DuplicateOutput(m_device.Get(), m_duplication.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    logger::error("Failed to duplicate output", hr);
    return false;
  }

  return true;
}

ID3D11Texture2D* ScreenshotService::CaptureScreen() {
  DXGI_OUTDUPL_FRAME_INFO frameInfo{};

  HRESULT hr = m_duplication->AcquireNextFrame(0, &frameInfo, m_desktopResource.ReleaseAndGetAddressOf());
  if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
    return nullptr;
  }

  if (hr == DXGI_ERROR_ACCESS_LOST) {
    logger::info("Desktop duplication access lost, re-creating...");
    m_duplication.Reset();
    CreateDuplication();
    return nullptr;
  }

  if (FAILED(hr)) {
    logger::error("Failed to acquire next frame", hr);
    return nullptr;
  }

  hr = m_desktopResource.As(&m_frameTexture);
  if (FAILED(hr)) {
    logger::error("Failed to query interface", hr);
    m_desktopResource.Reset();
    m_duplication->ReleaseFrame();
    return nullptr;
  }

  return m_frameTexture.Get();
}

void ScreenshotService::ReleaseFrameTexture() {
  m_frameTexture.Reset();
  m_desktopResource.Reset();
  m_duplication->ReleaseFrame();
}
