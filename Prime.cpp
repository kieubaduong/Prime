#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <uwebsockets/App.h>

#include "include/ScreenshotService.h"
#include "include/Global.h"

struct PerSocketData {};

static bool CaptureAndCheckImage(ScreenshotService& screenshotService, std::vector<unsigned char>& buffer) {
    buffer = screenshotService.CaptureScreen();

    if (buffer.empty()) {
        std::cerr << "Failed to capture image" << std::endl;
        return false;
    }

    if (buffer.size() != static_cast<unsigned long long>(g_width) * g_height * g_channels) {
        std::cerr << "Captured image has wrong size" << std::endl;
        return false;
    }

    return true;
}

static void ConvertAndSendImage(uWS::WebSocket<false, true, PerSocketData>* ws, const std::vector<unsigned char>& buffer) {
    int png_size;
    unsigned char* png_data = stbi_write_png_to_mem(buffer.data(), 0, g_width, g_height, g_channels, &png_size);

    if (png_data) {
        std::string_view png_view(reinterpret_cast<const char*>(png_data), png_size);
        ws->send(png_view, uWS::BINARY);
    }
    else {
        std::cerr << "Failed to convert image to PNG" << std::endl;
    }

    free(png_data);
}

int main() {
    std::atomic<bool> running = false;
    std::atomic<int> frame = 0;

    uWS::App().ws<PerSocketData>("/*", { 
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024 * 1024,
        .idleTimeout = 10,
        .maxBackpressure = 1 * 1024 * 1024,
        .open = [&running, &frame]([[maybe_unused]] auto* ws) {
            std::cout << "Client connected" << std::endl;

            running = true;

            std::thread([&ws, &running, &frame]() {
                ScreenshotService screenshotService;
                std::vector<unsigned char> buffer;

                while (running) {
                    if (!CaptureAndCheckImage(screenshotService, buffer)) {
                        break;
                    }

                    ConvertAndSendImage(ws, buffer);

                    frame++;
                }
            }).detach();

            std::thread([&ws, &running, &frame]() {
                while (running) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    std::cout << "FPS: " << frame << std::endl;
                    frame = 0;
                }
            }).detach();
        },
        .close = []([[maybe_unused]] auto* ws, [[maybe_unused]] int code, [[maybe_unused]] std::string_view message) {
            std::cout << "Client disconnected" << std::endl;
        }
    }).listen(8080, [](auto* token) {
        if (token) {
            std::cout << "Server started at ws://localhost:8080" << std::endl;
        }
        else {
            std::cerr << "Failed to start server" << std::endl;
        }
    }).run();

    return 0;
}
