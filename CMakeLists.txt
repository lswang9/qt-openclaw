cmake_minimum_required(VERSION 3.21)

project(qt-openclaw
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network WebSockets)

add_executable(qt-openclaw
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/GatewayClient.h
    src/GatewayClient.cpp
)

target_link_libraries(qt-openclaw PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
)

# Install rules omitted for skeleton; add when packaging.
