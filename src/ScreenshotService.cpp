#include "../include/ScreenshotService.h"
#include "../include/ErrorHandling.h"
#include "../include/Global.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <fstream>

#include <stb_image_write.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

bool ScreenshotService::GetNextFrame(std::vector<BYTE>& frameBuffer) {
    if (!this->CaptureScreen(frameBuffer)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return false;
    }

    if (frameBuffer.empty()) {
        std::cerr << "Failed to capture image" << std::endl;
        return false;
    }

    if (frameBuffer.size() != static_cast<unsigned long long>(g_width) * g_height * g_channels) {
        std::cerr << "Captured image has wrong size" << std::endl;
        return false;
    }

    return true;
}

ScreenshotService::ScreenshotService() {
    InitializeD3D();
}

void ScreenshotService::InitializeD3D() {
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &g_device, nullptr, &g_context);
    ErrorHandling::CheckHR(hr, "Failed to create device and context");

    if (!g_device || !g_context) {
        throw std::runtime_error("Failed to create device and context");
    }

    hr = g_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&g_dxgiDevice);
    ErrorHandling::CheckHR(hr, "Failed to query interface");

    hr = g_dxgiDevice->GetAdapter(&g_adapter);
    ErrorHandling::CheckHR(hr, "Failed to get adapter");

    hr = g_adapter->EnumOutputs(0, &g_output);
    ErrorHandling::CheckHR(hr, "Failed to get output");

    hr = g_output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&g_output1);
    ErrorHandling::CheckHR(hr, "Failed to query interface");

    hr = g_output1->DuplicateOutput(g_device, &g_duplication);
    ErrorHandling::CheckHR(hr, "Failed to duplicate output");
}

bool ScreenshotService::CaptureScreen(std::vector<BYTE>& frameBuffer) {
    DXGI_OUTDUPL_FRAME_INFO frameInfo{};
    IDXGIResource* desktopResource = nullptr;
    ID3D11Texture2D* frameTexture{};

    HRESULT hr = g_duplication->AcquireNextFrame(500, &frameInfo, &desktopResource);
    if (FAILED(hr) || hr == DXGI_ERROR_WAIT_TIMEOUT) {
        ErrorHandling::PrintError(hr, "Failed to acquire next frame");
        return false;
    }

    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&frameTexture);
    if (FAILED(hr)) {
        ErrorHandling::PrintError(hr, "Failed to query interface");
        return false;
    }

    D3D11_TEXTURE2D_DESC desc = {};
    frameTexture->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;

    ID3D11Texture2D* stagingTexture = nullptr;
    hr = g_device->CreateTexture2D(&desc, nullptr, &stagingTexture);
    if (FAILED(hr)) {
        ErrorHandling::PrintError(hr, "Failed to create staging texture");
        return false;
    }

    g_context->CopyResource(stagingTexture, frameTexture);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = g_context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        ErrorHandling::PrintError(hr, "Failed to map staging texture");
        return false;
    }

    frameBuffer.resize(static_cast<size_t>(desc.Width) * desc.Height * 4);
    auto const* src = reinterpret_cast<BYTE*>(mappedResource.pData);
    auto* dst = frameBuffer.data();

    for (UINT y = 0; y < desc.Height; ++y) {
        auto* srcRow = src + y * mappedResource.RowPitch;
        auto* dstRow = dst + y * desc.Width * 4;

        for (UINT x = 0; x < desc.Width; ++x) {
            dstRow[x * 4 + 0] = srcRow[x * 4 + 2]; // R
            dstRow[x * 4 + 1] = srcRow[x * 4 + 1]; // G
            dstRow[x * 4 + 2] = srcRow[x * 4 + 0]; // B
            dstRow[x * 4 + 3] = srcRow[x * 4 + 3]; // A
        }
    }

    g_context->Unmap(stagingTexture, 0);
    stagingTexture->Release();
    frameTexture->Release();
    desktopResource->Release();
    g_duplication->ReleaseFrame();

    return true;
}

void ScreenshotService::CleanupD3D() {
    if (g_context) {
        g_context->Release();
    }
    if (g_device) {
        g_device->Release();
    }
    if (g_duplication) {
        g_duplication->Release();
    }
    if (g_output1) {
        g_output1->Release();
    }
    if (g_output) {
        g_output->Release();
    }
    if (g_adapter) {
        g_adapter->Release();
    }
    if (g_dxgiDevice) {
        g_dxgiDevice->Release();
    }
}

ScreenshotService::~ScreenshotService() {
    CleanupD3D();
}
