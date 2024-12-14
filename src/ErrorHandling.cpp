#include "../include/ErrorHandling.h"
#include <format>

void ErrorHandling::PrintError(HRESULT hr, const std::string& message) {
    std::string formattedMessage = std::format("Error: {}, error code: {:#x}", message, hr);
    std::cout << formattedMessage << std::endl;
}

void ErrorHandling::CheckHR(HRESULT hr, const char* errorMessage) {
    if (FAILED(hr)) {
        PrintError(hr, errorMessage);
        throw std::runtime_error(errorMessage);
    }
}
