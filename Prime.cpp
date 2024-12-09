#include <d3d11.h>
#include <dxgi1_2.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;

void PrintError(HRESULT hr, const std::string& message) {
    std::stringstream ss;
    ss << "Error: " << message << ", error code: " << std::hex << std::showbase << hr;
    std::cout << ss.str() << std::endl;
}

void SaveTextureToFile(ID3D11DeviceContext* context, ID3D11Texture2D* texture, const char* filename) {
    auto hr = S_OK;

    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;

    ID3D11Texture2D* stagingTexture = nullptr;
    hr = g_device->CreateTexture2D(&desc, nullptr, &stagingTexture);
    if (FAILED(hr)) {
        PrintError(hr, "Failed to create staging texture");
        return;
    }

    context->CopyResource(stagingTexture, texture);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        PrintError(hr, "Failed to map staging texture");
        return;
    }

    auto const* data = reinterpret_cast<unsigned char*>(mappedResource.pData);
    int rowPitch = mappedResource.RowPitch;

    std::vector<unsigned char> buffer(desc.Width * desc.Height * 4);
    for (UINT y = 0; y < desc.Height; ++y) {
        memcpy(buffer.data() + y * desc.Width * 4, data + y * rowPitch, desc.Width * 4);
    }

    if (auto error = stbi_write_png(filename, desc.Width, desc.Height, 4, buffer.data(), desc.Width * 4); error == 0) {
        PrintError(hr, "Failed to write image file");
    }
    else {
        std::cout << "Image saved to " << filename << std::endl;
    }

    context->Unmap(stagingTexture, 0);
    stagingTexture->Release();
}

void CaptureScreen() {
    auto hr = S_OK;

    IDXGIDevice* dxgiDevice = nullptr;
    hr = g_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) {
        PrintError(hr, "Failed to query interface");
        return;
    }

    IDXGIAdapter* adapter = nullptr;
    hr = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr)) {
        PrintError(hr, "Failed to get adapter");
        return;
    }

    IDXGIOutput* output = nullptr;
    hr = adapter->EnumOutputs(0, &output);
    if (FAILED(hr)) {
        PrintError(hr, "Failed to get output");
        return;
    }

    IDXGIOutput1* output1 = nullptr;
    hr = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
    if (FAILED(hr)) {
        PrintError(hr, "Failed to query interface");
        return;
    }

    IDXGIOutputDuplication* duplication = nullptr;
    hr = output1->DuplicateOutput(g_device, &duplication);
    if (FAILED(hr)) {
        PrintError(hr, "Failed to duplicate output");
        return;
    }

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* desktopResource = nullptr;
    hr = duplication->AcquireNextFrame(500, &frameInfo, &desktopResource);
    if (FAILED(hr)) {
        PrintError(hr, "Failed to acquire next frame");
        return;
    }

    while (frameInfo.AccumulatedFrames == 0) {
        std::cout << "No new frames accumulated." << std::endl;
        duplication->ReleaseFrame();
        hr = duplication->AcquireNextFrame(500, &frameInfo, &desktopResource);
        if (FAILED(hr)) {
            PrintError(hr, "Failed to acquire next frame");
            return;
        }
    }

    ID3D11Texture2D* frameTexture = nullptr;
    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&frameTexture);
    desktopResource->Release();

    if (FAILED(hr)) {
        PrintError(hr, "Failed to query interface");
        return;
    }

    SaveTextureToFile(g_context, frameTexture, "screenshot.png");

    duplication->ReleaseFrame();
    frameTexture->Release();
    duplication->Release();
    output1->Release();
    output->Release();
    adapter->Release();
    dxgiDevice->Release();
}

void InitializeD3D() {
    auto hr = S_OK;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &g_device, nullptr, &g_context);
    if (FAILED(hr)) {
        PrintError(hr, "Failed to create device and context");
        exit(1);
    }
}

void CleanupD3D() {
    if (g_context) {
        g_context->Release();
    }
    if (g_device) {
        g_device->Release();
    }
}

int main() {
    InitializeD3D();
    CaptureScreen();
    CleanupD3D();

    return 0;
}
