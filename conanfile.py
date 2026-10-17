from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class QtOpenclawConan(ConanFile):
    name = "qt-openclaw"
    version = "0.1.0"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
    }
    default_options = {
        "shared": True,  # LGPL-friendly dynamic link of Qt
        "qt/*:shared": True,
        "qt/*:qtbase": True,
        "qt/*:qtdeclarative": False,
        "qt/*:qttools": False,
        "qt/*:qtwebsockets": True,
        "qt/*:with_freetype": True,
        "qt/*:gui": True,
        "qt/*:widgets": True,
    }

    def requirements(self):
        # Pin a recent Qt 6 from Conan Center; bump when needed.
        self.requires("qt/6.7.3")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()
