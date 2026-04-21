#include "../include/screenshot_service.hpp"

#include <Windows.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_5.h>
#include <dxgiformat.h>

#include <array>
#include <optional>

#include "../include/log.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

static constexpr std::array kSupportedFormats = {
    DXGI_FORMAT_B8G8R8A8_UNORM,
};

std::optional<ScreenshotService> ScreenshotService::Create() {
  ScreenshotService service;

  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                                 &service.m_device, nullptr, &service.m_context);
  if (FAILED(hr)) {
    Log::Error("Failed to create device and context: " + Log::HRMessage(hr));
    return std::nullopt;
  }

  IDXGIDevice* dxgiDevice = nullptr;
  hr = service.m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
  if (FAILED(hr)) {
    Log::Error("Failed to query IDXGIDevice: " + Log::HRMessage(hr));
    return std::nullopt;
  }

  IDXGIAdapter* adapter = nullptr;
  hr = dxgiDevice->GetAdapter(&adapter);
  dxgiDevice->Release();
  if (FAILED(hr)) {
    Log::Error("Failed to get adapter: " + Log::HRMessage(hr));
    return std::nullopt;
  }

  IDXGIOutput* output = nullptr;
  hr = adapter->EnumOutputs(0, &output);
  adapter->Release();
  if (FAILED(hr)) {
    Log::Error("Failed to get output: " + Log::HRMessage(hr));
    return std::nullopt;
  }

  hr = output->QueryInterface(__uuidof(IDXGIOutput5), (void**)&service.m_output5);
  output->Release();
  if (FAILED(hr)) {
    Log::Error("Failed to query IDXGIOutput5: " + Log::HRMessage(hr));
    return std::nullopt;
  }

  if (!service.CreateDuplication()) {
    return std::nullopt;
  }

  return service;
}

bool ScreenshotService::CreateDuplication() {
  HRESULT hr = m_output5->DuplicateOutput1(m_device, 0, static_cast<UINT>(kSupportedFormats.size()), kSupportedFormats.data(), &m_duplication);
  if (SUCCEEDED(hr)) {
    return true;
  }

  Log::Info("DuplicateOutput1 not supported, falling back to DuplicateOutput: " + Log::HRMessage(hr));
  hr = m_output5->DuplicateOutput(m_device, &m_duplication);
  if (FAILED(hr)) {
    Log::Error("Failed to duplicate output: " + Log::HRMessage(hr));
    return false;
  }

  return true;
}

ID3D11Texture2D* ScreenshotService::CaptureScreen() {
  DXGI_OUTDUPL_FRAME_INFO frameInfo{};

  HRESULT hr = m_duplication->AcquireNextFrame(0, &frameInfo, &m_desktopResource);
  if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
    return nullptr;
  }

  if (hr == DXGI_ERROR_ACCESS_LOST) {
    Log::Info("Desktop duplication access lost, re-creating...");
    if (m_duplication) {
      m_duplication->Release();
      m_duplication = nullptr;
    }

    CreateDuplication();
    return nullptr;
  }

  if (FAILED(hr)) {
    Log::Error("Failed to acquire next frame: " + Log::HRMessage(hr));
    return nullptr;
  }

  hr = m_desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&m_frameTexture);
  if (FAILED(hr)) {
    Log::Error("Failed to query interface: " + Log::HRMessage(hr));
    m_desktopResource->Release();
    m_desktopResource = nullptr;
    m_duplication->ReleaseFrame();
    return nullptr;
  }

  return m_frameTexture;
}

void ScreenshotService::ReleaseFrameTexture() {
  if (m_frameTexture) {
    m_frameTexture->Release();
    m_frameTexture = nullptr;
  }

  if (m_desktopResource) {
    m_desktopResource->Release();
    m_desktopResource = nullptr;
  }

  m_duplication->ReleaseFrame();
}

void ScreenshotService::Cleanup() {
  if (m_duplication) {
    m_duplication->Release();
  }
  if (m_output5) {
    m_output5->Release();
  }
  if (m_context) {
    m_context->Release();
  }
  if (m_device) {
    m_device->Release();
  }
}

ScreenshotService::ScreenshotService(ScreenshotService&& other) noexcept
    : m_device(other.m_device),
      m_context(other.m_context),
      m_output5(other.m_output5),
      m_duplication(other.m_duplication),
      m_desktopResource(other.m_desktopResource),
      m_frameTexture(other.m_frameTexture) {
  other.m_device = nullptr;
  other.m_context = nullptr;
  other.m_output5 = nullptr;
  other.m_duplication = nullptr;
  other.m_desktopResource = nullptr;
  other.m_frameTexture = nullptr;
}

ScreenshotService& ScreenshotService::operator=(ScreenshotService&& other) noexcept {
  if (this != &other) {
    Cleanup();
    m_device = other.m_device;
    m_context = other.m_context;
    m_output5 = other.m_output5;
    m_duplication = other.m_duplication;
    m_desktopResource = other.m_desktopResource;
    m_frameTexture = other.m_frameTexture;
    other.m_device = nullptr;
    other.m_context = nullptr;
    other.m_output5 = nullptr;
    other.m_duplication = nullptr;
    other.m_desktopResource = nullptr;
    other.m_frameTexture = nullptr;
  }
  return *this;
}

ScreenshotService::~ScreenshotService() {
  Cleanup();
}
