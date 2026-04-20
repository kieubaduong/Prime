#include <d3d11.h>

#include <chrono>
#include <thread>

#include "include/log.hpp"
#include "include/preview_window.hpp"
#include "include/screenshot_service.hpp"

int main() {
  auto screenshotService = ScreenshotService::Create();
  if (!screenshotService) {
    return 1;
  }

  auto preview = PreviewWindow::Create(screenshotService->GetDevice(), screenshotService->GetContext());
  if (!preview) {
    return 1;
  }

  Log::Info("Starting capture loop...");

  while (preview->PumpMessages()) {
    ID3D11Texture2D* texture = screenshotService->CaptureScreen();
    if (!texture) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    preview->Present(texture);
    screenshotService->ReleaseFrameTexture();
  }

  return 0;
}
