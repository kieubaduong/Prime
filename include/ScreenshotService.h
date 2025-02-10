#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <string>
#include <thread>
#include <atomic>
#include <vector>

class ScreenshotService {
public:
    ScreenshotService();
    ~ScreenshotService();

    bool GetNextFrame(std::vector<BYTE>& frameBuffer);

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
    
    bool CaptureScreen(std::vector<BYTE>& frameBuffer);
};
