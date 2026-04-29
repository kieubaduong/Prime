#include <d3d11.h>

#include <chrono>
#include <optional>
#include <thread>

#include "include/log.hpp"
#include "include/monitor_picker.hpp"
#include "include/preview_window.hpp"
#include "include/screenshot_service.hpp"

int main() {
  logger::enable_virtual_terminal();

  monitor_picker::MonitorSelection selection = monitor_picker::choose();
  std::optional<ScreenshotService> screenshotService = ScreenshotService::Create(selection);
  if (!screenshotService) {
    return 1;
  }

  std::optional<PreviewWindow> preview = PreviewWindow::Create(screenshotService->GetDevice(), screenshotService->GetContext());
  if (!preview) {
    return 1;
  }

  logger::info("Starting capture loop...");

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
