#pragma once

#include <Windows.h>

#include <iostream>
#include <string>
#include <sstream>

namespace ErrorHandling {

    void PrintError(HRESULT hr, const std::string& message);
    void CheckHR(HRESULT hr, const char* errorMessage);

}
