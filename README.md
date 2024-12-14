# Prime

Prime is a C++ project that captures screenshots and serves them over a WebSocket connection. It uses DirectX for screen capturing and uWebSockets for WebSocket communication.

## Features

- Capture screenshots using DirectX
- Serve screenshots over WebSocket
- Convert screenshots to PNG format

## Requirements

- Visual Studio 2022 or later
- Windows 10 or later
- C++17 or later
- DirectX SDK
- uWebSockets library
- stb_image_write library

## Setup

1. Clone the repository:
    ```sh
    git clone https://github.com/kieubaduong/Prime.git
    cd Prime
    ```

2. Open the solution file `Prime.sln` in Visual Studio.

3. Build the solution.

## Usage

1. Run the application:
    ```sh
    ./Prime.exe
    ```

2. Connect to the WebSocket server at `ws://localhost:8080` to receive screenshots.

## Project Structure

- `Prime.cpp`: Main application file.
- `ScreenshotService.h` and `ScreenshotService.cpp`: Handles screen capturing using DirectX.
- `ErrorHandling.h` and `ErrorHandling.cpp`: Utility functions for error handling.
- `httplib.h`: Header-only HTTP library used for additional HTTP functionalities.
- `Prime.vcxproj` and `Prime.vcxproj.filters`: Visual Studio project files.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
