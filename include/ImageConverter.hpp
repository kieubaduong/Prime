#pragma once

#include <vector>

namespace ImageConverter {
    bool ToJPG(const std::vector<unsigned char>& frameBuffer, std::vector<unsigned char>& jpegBuffer);
};
