#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_5.h>
#include <wrl/client.h>

#include <optional>

class ScreenshotService {
 public:
  static std::optional<ScreenshotService> Create(UINT adapterIndex = 0, UINT monitorIndex = 0);

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

  bool CreateDuplication();
};
