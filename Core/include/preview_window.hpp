#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <optional>

class PreviewWindow {
 public:
  static std::optional<PreviewWindow> Create(ID3D11Device* device, ID3D11DeviceContext* context);

  ~PreviewWindow();

  PreviewWindow(PreviewWindow&& other) noexcept;
  PreviewWindow& operator=(PreviewWindow&& other) noexcept;

  PreviewWindow(const PreviewWindow&) = delete;
  PreviewWindow& operator=(const PreviewWindow&) = delete;

  void Present(ID3D11Texture2D* capturedTexture);
  bool PumpMessages() const;

 private:
  PreviewWindow() = default;

  HWND m_hwnd = nullptr;
  Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
  Microsoft::WRL::ComPtr<ID3D11Device> m_device;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
  bool m_resizeNeeded = false;

  bool CreateWindowAndSwapChain();
  bool CreateShaders();
  bool CreateSampler();
  bool CreateRTV();
  void ResizeSwapChain();
  void Cleanup();

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
