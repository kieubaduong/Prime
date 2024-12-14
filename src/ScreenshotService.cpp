#include "../include/ScreenshotService.h"
#include "../include/ErrorHandling.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <fstream>

#include <stb_image_write.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

ScreenshotService::ScreenshotService() {
    InitializeD3D();
}

void ScreenshotService::InitializeD3D() {
    try {
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
    catch (...) {
        std::cout << "Unknown error occurred" << std::endl;
        CleanupD3D();
    }
}

std::vector<unsigned char> ScreenshotService::CaptureScreen() {
    HRESULT hr = S_OK;

    DXGI_OUTDUPL_FRAME_INFO frameInfo{};
    frameInfo.AccumulatedFrames = 0;
    IDXGIResource* desktopResource = nullptr;

    while (frameInfo.AccumulatedFrames == 0) {
        std::cout << "No new frames accumulated." << std::endl;
        g_duplication->ReleaseFrame();
        hr = g_duplication->AcquireNextFrame(500, &frameInfo, &desktopResource);
        if (FAILED(hr)) {
            ErrorHandling::PrintError(hr, "Failed to acquire next frame");
            return {};
        }
    }

    ID3D11Texture2D* frameTexture{};
    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&frameTexture);
    if (FAILED(hr)) {
        ErrorHandling::PrintError(hr, "Failed to query interface");
        return {};
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
        return {};
    }

    g_context->CopyResource(stagingTexture, frameTexture);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = g_context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        ErrorHandling::PrintError(hr, "Failed to map staging texture");
        return {};
    }

    auto const* data = reinterpret_cast<unsigned char*>(mappedResource.pData);
    int rowPitch = mappedResource.RowPitch;

    std::vector<unsigned char> buffer(desc.Width * desc.Height * 4);
    for (UINT y = 0; y < desc.Height; ++y) {
        memcpy(buffer.data() + y * desc.Width * 4, data + y * rowPitch, desc.Width * 4);
    }

    g_context->Unmap(stagingTexture, 0);
    g_duplication->ReleaseFrame();

    return buffer;
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
