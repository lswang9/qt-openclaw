cmake_minimum_required(VERSION 3.21)
project(qt-openclaw VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network WebSockets)

set(QT_OPENCLAW_SOURCES
    src/main.cpp
    src/MainWindow.h src/MainWindow.cpp
    src/GatewayClient.h src/GatewayClient.cpp
    src/GatewayChatClient.h src/GatewayChatClient.cpp
    src/GatewayConfig.h src/GatewayConfig.cpp
    src/LocalGatewayProcess.h src/LocalGatewayProcess.cpp
    src/RoleLoader.h src/RoleLoader.cpp
    src/CharacterSelectDialog.h src/CharacterSelectDialog.cpp
    src/SettingsDialog.h src/SettingsDialog.cpp
    src/SplashPage.h src/SplashPage.cpp
    src/RoleMaterializer.h src/RoleMaterializer.cpp
    src/Persona.h
    resources/characters.qrc
)

if(APPLE)
    add_executable(qt-openclaw MACOSX_BUNDLE ${QT_OPENCLAW_SOURCES})
    set_target_properties(qt-openclaw PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "器灵 Agent"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.qiling.agent"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_INFO_STRING "器灵 Agent"
        OUTPUT_NAME "器灵 Agent"
    )
else()
    add_executable(qt-openclaw ${QT_OPENCLAW_SOURCES})
endif()

target_link_libraries(qt-openclaw PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::WebSockets)

if(APPLE)
    # Prefer CMakeCache / toolchain macdeployqt (Conan Qt); fall back to PATH.
    if(NOT MACDEPLOYQT_EXECUTABLE)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt)
    endif()
    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET qt-openclaw POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}"
                    "$<TARGET_BUNDLE_DIR:qt-openclaw>"
                    -always-overwrite
            COMMENT "Deploy Qt frameworks into 器灵 Agent.app"
            VERBATIM)
        message(STATUS "macdeployqt: ${MACDEPLOYQT_EXECUTABLE}")
    else()
        message(WARNING "macdeployqt not found; .app may miss Qt frameworks")
    endif()
endif()
