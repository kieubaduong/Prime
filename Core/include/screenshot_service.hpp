#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_5.h>
#include <wrl/client.h>

#include <optional>

#include "monitor_picker.hpp"

class ScreenshotService {
 public:
  static std::optional<ScreenshotService> Create(const monitor_picker::MonitorSelection& selection);

  ScreenshotService(ScreenshotService&& other) noexcept = default;
  ScreenshotService& operator=(ScreenshotService&& other) noexcept = default;

  ScreenshotService(const ScreenshotService&) = delete;
  ScreenshotService& operator=(const ScreenshotService&) = delete;

  ID3D11Texture2D* CaptureScreen();
  void ReleaseFrameTexture();

  ID3D11Device* GetDevice() const {
    return m_device.Get();
  }
  ID3D11DeviceContext* GetContext() const {
    return m_context.Get();
  }

 private:
  ScreenshotService() = default;

  Microsoft::WRL::ComPtr<ID3D11Device> m_device;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
  Microsoft::WRL::ComPtr<IDXGIOutput5> m_output5;
  Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_duplication;

  Microsoft::WRL::ComPtr<IDXGIResource> m_desktopResource;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_frameTexture;

  static Microsoft::WRL::ComPtr<IDXGIOutputDuplication> CreateDuplication(IDXGIOutput5* output, ID3D11Device* device);
};
