from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class QtOpenclawConan(ConanFile):
    name = "qt-openclaw"
    version = "0.1.0"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {
        "shared": True,
        "qt/*:shared": True,
        "qt/*:qtbase": True,
        "qt/*:gui": True,
        "qt/*:widgets": True,
        "qt/*:with_freetype": True,
        "qt/*:openssl": True,
        # qt_quick_enabled = gui && qtshadertools — required for WebEngine Support check
        "qt/*:qtshadertools": True,
        "qt/*:qtdeclarative": True,
        "qt/*:qtwebchannel": True,
        "qt/*:qtpositioning": True,
        "qt/*:qtwebsockets": True,
        "qt/*:qtsvg": True,
        "qt/*:qtwebengine": True,
        "qt/*:qttools": False,
    }

    def requirements(self):
        self.requires("qt/6.7.3")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        CMakeDeps(self).generate()
        CMakeToolchain(self).generate()
