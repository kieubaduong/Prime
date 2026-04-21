#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_5.h>

#include <optional>

class ScreenshotService {
 public:
  static std::optional<ScreenshotService> Create();

  ~ScreenshotService();

  ScreenshotService(ScreenshotService&& other) noexcept;
  ScreenshotService& operator=(ScreenshotService&& other) noexcept;

  ScreenshotService(const ScreenshotService&) = delete;
  ScreenshotService& operator=(const ScreenshotService&) = delete;

  ID3D11Texture2D* CaptureScreen();
  void ReleaseFrameTexture();

  ID3D11Device* GetDevice() const { return m_device; }
  ID3D11DeviceContext* GetContext() const { return m_context; }

 private:
  ScreenshotService() = default;

  ID3D11Device* m_device = nullptr;
  ID3D11DeviceContext* m_context = nullptr;
  IDXGIOutput5* m_output5 = nullptr;
  IDXGIOutputDuplication* m_duplication = nullptr;

  IDXGIResource* m_desktopResource = nullptr;
  ID3D11Texture2D* m_frameTexture = nullptr;

  bool CreateDuplication();
  void Cleanup();
};
