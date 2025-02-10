#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>

#include <uwebsockets/App.h>

#include "include/ScreenshotService.h"
#include "include/Global.h"
#include "include/ImageConverter.hpp"

struct PerSocketData {};

static void HandleClientConnection(uWS::WebSocket<false, true, PerSocketData>* ws, std::atomic<bool>& running, std::atomic<int>& frame) {
    std::cout << "Client connected" << std::endl;

    running = true;

    std::jthread([&ws, &running, &frame]() {
        ScreenshotService screenshotService;
        std::vector<unsigned char> frameBuffer;

        while (running) {
            if (!screenshotService.GetNextFrame(frameBuffer)) {
                continue;
            }

            std::vector<unsigned char> jpgBuffer;
            if (!ImageConverter::ToJPG(frameBuffer, jpgBuffer)) {
                continue;
            }

            std::string_view jpeg_view(reinterpret_cast<const char*>(jpgBuffer.data()), jpgBuffer.size());
            ws->send(jpeg_view, uWS::BINARY);

            frame++;
        }
    }).detach();

    std::jthread([&ws, &running, &frame]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "FPS: " << frame << std::endl;
            frame = 0;
        }
    }).detach();
}

int main() {
    std::atomic<bool> running = false;
    std::atomic<int> frame = 0;

    uWS::App().ws<PerSocketData>("/*", {
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024 * 1024, // 16MB
        .idleTimeout = 10, // 10 seconds
        .maxBackpressure = 1 * 1024 * 1024, // 1MB
        .open = [&running, &frame]([[maybe_unused]] auto* ws) {
            HandleClientConnection(ws, running, frame);
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
