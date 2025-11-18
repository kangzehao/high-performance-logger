from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake

import os


class Requirements(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("fmt/11.0.2")
        

    def configure(self):
        self.options["fmt"].shared = False
        

    def build(self):
        cmake = CMake(self)

        cmake.configure()
        cmake.build()
