#include "../include/ErrorHandling.h"

void ErrorHandling::PrintError(HRESULT hr, const std::string& message) {
    std::stringstream ss;
    ss << "Error: " << message << ", error code: " << std::hex << std::showbase << hr;
    std::cout << ss.str() << std::endl;
}

void ErrorHandling::CheckHR(HRESULT hr, const char* errorMessage) {
    if (FAILED(hr)) {
        PrintError(hr, errorMessage);
        throw std::runtime_error(errorMessage);
    }
}
