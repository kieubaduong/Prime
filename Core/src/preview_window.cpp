#include "../include/preview_window.hpp"

#include <Windows.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgiformat.h>
#include <string.h>
#include <wrl/client.h>

#include <format>
#include <optional>
#include <utility>

#include "../include/log.hpp"

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3dcompiler.lib")

static const char* const kShaderSource = R"(
Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct VSOut {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

VSOut VS(uint id : SV_VertexID) {
    VSOut o;
    o.uv  = float2((id << 1) & 2, id & 2);
    o.pos = float4(o.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return o;
}

float4 PS(VSOut i) : SV_Target {
    return tex.Sample(samp, i.uv);
}
)";

std::optional<PreviewWindow> PreviewWindow::Create(ID3D11Device* device, ID3D11DeviceContext* context) {
  PreviewWindow window;
  window.m_device = device;
  window.m_context = context;

  if (!window.CreateWindowAndSwapChain()) {
    return std::nullopt;
  }
  if (!window.CreateShaders()) {
    return std::nullopt;
  }
  if (!window.CreateSampler()) {
    return std::nullopt;
  }
  if (!window.CreateRTV()) {
    return std::nullopt;
  }

  return window;
}

PreviewWindow::~PreviewWindow() {
  Cleanup();
}

PreviewWindow::PreviewWindow(PreviewWindow&& other) noexcept
    : m_hwnd(other.m_hwnd),
      m_swapChain(std::move(other.m_swapChain)),
      m_rtv(std::move(other.m_rtv)),
      m_vs(std::move(other.m_vs)),
      m_ps(std::move(other.m_ps)),
      m_sampler(std::move(other.m_sampler)),
      m_device(std::move(other.m_device)),
      m_context(std::move(other.m_context)),
      m_resizeNeeded(other.m_resizeNeeded) {
  other.m_hwnd = nullptr;
  // Update GWLP_USERDATA to point to this instead of other
  if (m_hwnd) {
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  }
}

PreviewWindow& PreviewWindow::operator=(PreviewWindow&& other) noexcept {
  if (this != &other) {
    Cleanup();
    m_hwnd = other.m_hwnd;
    other.m_hwnd = nullptr;
    m_swapChain = std::move(other.m_swapChain);
    m_rtv = std::move(other.m_rtv);
    m_vs = std::move(other.m_vs);
    m_ps = std::move(other.m_ps);
    m_sampler = std::move(other.m_sampler);
    m_device = std::move(other.m_device);
    m_context = std::move(other.m_context);
    m_resizeNeeded = other.m_resizeNeeded;
    if (m_hwnd) {
      SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
  }
  return *this;
}

LRESULT CALLBACK PreviewWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  auto* self = reinterpret_cast<PreviewWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

  switch (msg) {
    case WM_CREATE: {
      auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
      SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
      return 0;
    }
    case WM_SIZE:
      if (self && wParam != SIZE_MINIMIZED) {
        self->m_resizeNeeded = true;
      }
      return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
}

bool PreviewWindow::CreateWindowAndSwapChain() {
  WNDCLASSEX wc{};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.lpszClassName = L"PreviewWindowClass";
  RegisterClassEx(&wc);

  RECT rect = {0, 0, 960, 540};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

  m_hwnd = CreateWindowEx(0, wc.lpszClassName, L"Preview", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
                          rect.bottom - rect.top, nullptr, nullptr, wc.hInstance, this);

  if (!m_hwnd) {
    logger::error("Failed to create preview window");
    return false;
  }

  ComPtr<IDXGIDevice> dxgiDevice;
  m_device.As(&dxgiDevice);
  ComPtr<IDXGIAdapter> adapter;
  dxgiDevice->GetAdapter(adapter.GetAddressOf());
  ComPtr<IDXGIFactory> factory;
  adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf()));

  DXGI_SWAP_CHAIN_DESC sd{};
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = m_hwnd;
  sd.SampleDesc.Count = 1;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  HRESULT hr = factory->CreateSwapChain(m_device.Get(), &sd, m_swapChain.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    logger::error("Failed to create swap chain", hr);
    return false;
  }

  ShowWindow(m_hwnd, SW_SHOW);
  return true;
}

bool PreviewWindow::CreateShaders() {
  ComPtr<ID3DBlob> vsBlob;
  ComPtr<ID3DBlob> psBlob;
  ComPtr<ID3DBlob> errorBlob;

  HRESULT hr = D3DCompile(kShaderSource, strlen(kShaderSource), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, vsBlob.ReleaseAndGetAddressOf(),
                          errorBlob.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    if (errorBlob) {
      logger::error(std::format("VS compile: {}", static_cast<const char*>(errorBlob->GetBufferPointer())));
    }
    else {
      logger::error("VS compile failed", hr);
    }
    return false;
  }

  hr = D3DCompile(kShaderSource, strlen(kShaderSource), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, psBlob.ReleaseAndGetAddressOf(),
                  errorBlob.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    if (errorBlob) {
      logger::error(std::format("PS compile: {}", static_cast<const char*>(errorBlob->GetBufferPointer())));
    }
    else {
      logger::error("PS compile failed", hr);
    }
    return false;
  }

  hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vs.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    logger::error("Failed to create vertex shader", hr);
    return false;
  }

  hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_ps.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    logger::error("Failed to create pixel shader", hr);
    return false;
  }

  return true;
}

bool PreviewWindow::CreateSampler() {
  D3D11_SAMPLER_DESC sd{};
  sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

  HRESULT hr = m_device->CreateSamplerState(&sd, m_sampler.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    logger::error("Failed to create sampler", hr);
    return false;
  }
  return true;
}

bool PreviewWindow::CreateRTV() {
  ComPtr<ID3D11Texture2D> backBuffer;
  HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
  if (FAILED(hr)) {
    logger::error("Failed to get back buffer", hr);
    return false;
  }

  hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_rtv.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    logger::error("Failed to create RTV", hr);
    return false;
  }
  return true;
}

void PreviewWindow::ResizeSwapChain() {
  m_rtv.Reset();
  m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
  CreateRTV();
  m_resizeNeeded = false;
}

void PreviewWindow::Present(ID3D11Texture2D* capturedTexture) {
  if (m_resizeNeeded) {
    ResizeSwapChain();
  }

  ComPtr<ID3D11ShaderResourceView> srv;
  D3D11_TEXTURE2D_DESC texDesc{};
  capturedTexture->GetDesc(&texDesc);

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = texDesc.Format;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;

  HRESULT hr = m_device->CreateShaderResourceView(capturedTexture, &srvDesc, srv.GetAddressOf());
  if (FAILED(hr)) {
    logger::error("Failed to create SRV", hr);
    return;
  }

  DXGI_SWAP_CHAIN_DESC scDesc{};
  m_swapChain->GetDesc(&scDesc);

  D3D11_VIEWPORT vp{};
  vp.Width = static_cast<float>(scDesc.BufferDesc.Width);
  vp.Height = static_cast<float>(scDesc.BufferDesc.Height);
  vp.MaxDepth = 1.0f;

  m_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), nullptr);
  m_context->RSSetViewports(1, &vp);
  m_context->VSSetShader(m_vs.Get(), nullptr, 0);
  m_context->PSSetShader(m_ps.Get(), nullptr, 0);
  m_context->PSSetShaderResources(0, 1, srv.GetAddressOf());
  m_context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
  m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_context->Draw(3, 0);

  m_swapChain->Present(1, 0);
}

bool PreviewWindow::PumpMessages() const {
  MSG msg{};
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      return false;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return true;
}

void PreviewWindow::Cleanup() {
  m_sampler.Reset();
  m_ps.Reset();
  m_vs.Reset();
  m_rtv.Reset();
  m_swapChain.Reset();
  if (m_hwnd) {
    DestroyWindow(m_hwnd);
    m_hwnd = nullptr;
  }
}
