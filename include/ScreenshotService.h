#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <string>

class ScreenshotService {
public:
    ScreenshotService();
    ~ScreenshotService();

    void CaptureScreen();

private:
    ID3D11Device* g_device = nullptr;
    ID3D11DeviceContext* g_context = nullptr;
    IDXGIDevice* g_dxgiDevice = nullptr;
    IDXGIAdapter* g_adapter = nullptr;
    IDXGIOutput* g_output = nullptr;
    IDXGIOutput1* g_output1 = nullptr;
    IDXGIOutputDuplication* g_duplication = nullptr;

    void InitializeD3D();
    void CleanupD3D();
    void SaveTextureToFile(ID3D11Texture2D* texture, const char* filename);
};
