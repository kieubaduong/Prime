#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <uwebsockets/App.h>

#include "include/ScreenshotService.h"

struct PerSocketData {};

int main() {
    ScreenshotService screenshotService;
    std::vector<unsigned char> buffer = screenshotService.CaptureScreen();

    uWS::App()
        .ws<PerSocketData>(
            "/*",
            {
                /* Settings */
                 .compression = uWS::SHARED_COMPRESSOR,
                 .maxPayloadLength = 16 * 1024 * 1024,
                 .idleTimeout = 10,
                 .maxBackpressure = 1 * 1024 * 1024,
                 /* Handlers */
                 .open =
                     [&buffer](auto* ws) {
                         std::cout << "Client connected" << std::endl;
                     },
                 .message =
                     [&buffer, &screenshotService](
                         auto* ws, std::string_view message, uWS::OpCode opCode) {
                             // Capture and send image continuously
                             while (true) {
                                 buffer = screenshotService.CaptureScreen();
                                 int width = 1920;
                                 int height = 1080;
                                 int channels = 4;  // Assuming RGBA format

                                 if (buffer.empty()) {
                                     std::cerr << "Failed to capture image" << std::endl;
                                     break;
                                 }

                                 if (buffer.size() != width * height * channels) {
                                     std::cerr << "Captured image has wrong size"
                                         << std::endl;
                                     break;
                                 }

                                 int png_size;
                                 unsigned char* png_data =
                                     stbi_write_png_to_mem(buffer.data(), 0, width,
                                                           height, channels, &png_size);

                                 if (png_data) {
                                     std::string_view png_view(reinterpret_cast<const char*>(png_data), png_size);
                                     ws->send(png_view, uWS::BINARY);
                                     free(png_data);
                                 } 
                                 else {
                                    std::cerr << "Failed to convert image to PNG" << std::endl;
                                 }

                                 std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // Send image every second
                             }
},
.close =
    [](auto* ws, int code, std::string_view message) {
        std::cout << "Client disconnected" << std::endl;
    } })
        .listen(8080,
            [](auto* token) {
                if (token) {
                    std::cout << "Server started at ws://localhost:8080"
                        << std::endl;
                }
                else {
                    std::cerr << "Failed to start server" << std::endl;
                }
            })
        .run();

    return 0;
}
