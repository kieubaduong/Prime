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

std::atomic<bool> running = false;
std::atomic<int> frame = 0;
std::jthread frameCaptureThread;
std::jthread fpsThread;

static void FrameCaptureThread(uWS::WebSocket<false, true, PerSocketData>* ws) {
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
}

static void FPSThread() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "FPS: " << frame << std::endl;
        frame = 0;
    }
}

static void HandleClientConnection(uWS::WebSocket<false, true, PerSocketData>* ws) {
    std::cout << "Client connected" << std::endl;

    running = true;

    if (!frameCaptureThread.joinable()) {
        frameCaptureThread = std::jthread(FrameCaptureThread, ws);
    }

    if (!fpsThread.joinable()) {
        fpsThread = std::jthread(FPSThread);
    }
}

static void HandleClientDisconnection(uWS::App* app) {
    std::cout << "Client disconnected" << std::endl;
    running = false;

    if (frameCaptureThread.joinable()) {
        frameCaptureThread.join();
    }
    if (fpsThread.joinable()) {
        fpsThread.join();
    }

    app->close();
}

int main() {
    uWS::App app;

    app.ws<PerSocketData>("/*", {
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024 * 1024, // 16MB
        .idleTimeout = 10, // 10 seconds
        .maxBackpressure = 1 * 1024 * 1024, // 1MB
        .open = []([[maybe_unused]] auto* ws) {
            HandleClientConnection(ws);
        },
        .close = [&app]([[maybe_unused]] auto* ws, [[maybe_unused]] int code, [[maybe_unused]] std::string_view message) {
            HandleClientDisconnection(&app);
        },
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
