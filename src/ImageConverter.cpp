#include "../include/ImageConverter.hpp"
#include "../include/Global.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <iostream>
#include <stb_image_write.h>

bool ImageConverter::ToJPG(const std::vector<unsigned char>& frameBuffer, std::vector<unsigned char>& jpegBuffer) {
    auto write_to_memory = [](void* context, void* data, int size) {
        auto* buffer = static_cast<std::vector<unsigned char>*>(context);
        auto* bytes = static_cast<unsigned char*>(data);
        buffer->insert(buffer->end(), bytes, bytes + size);
    };

    if (!stbi_write_jpg_to_func(write_to_memory, &jpegBuffer, g_width, g_height, g_channels, frameBuffer.data(), g_jpeg_quality)) {
        std::cerr << "Failed to convert image to JPEG" << std::endl;
        return false;
    }

    return true;
}
