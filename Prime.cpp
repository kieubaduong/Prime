#include <fstream>
#include <iostream>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "include/httplib.h"
#include "include/ScreenshotService.h"

int main() {
    ScreenshotService screenshotService;
    std::vector<unsigned char> buffer = screenshotService.ReturnCapturedImage();

    httplib::Server svr;

    svr.Post("/get_image", [&buffer](const httplib::Request& req, httplib::Response& res) {
        try {
            int width = 1920;
            int height = 1080;
            int channels = 4; // Assuming RGBA format

            int png_size;
            unsigned char* png_data = stbi_write_png_to_mem(buffer.data(), 0, width, height, channels, &png_size);

            if (png_data) {
                res.set_content(reinterpret_cast<const char*>(png_data), png_size, "image/png");
            }
            else {
                throw std::runtime_error("Failed to convert image to PNG");
            }
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content(e.what(), "text/plain");
        }
        });

    std::cout << "Server started at http://localhost:8080" << std::endl;
    svr.listen("localhost", 8080);

    return 0;
}
