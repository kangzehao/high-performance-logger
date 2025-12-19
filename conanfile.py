from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake

import os


class Requirements(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("fmt/11.0.2",override=True)
        self.requires("gtest/1.14.0")
        self.requires("zlib/1.2.13")
        self.requires("zstd/1.5.6")
        self.requires("cryptopp/8.9.0")
        self.requires("protobuf/3.21.12")

        self.requires("benchmark/1.8.3")

        #性能对比库
        self.requires("spdlog/1.12.0")
        

    def configure(self):
        self.options["fmt"].shared = False

        self.options["gtest"].shared = False

        self.options["zlib"].shared = False
        self.options["zstd"].shared = False

        self.options["cryptopp"].shared = False
        
        self.options["protobuf"].shared = False
        self.options["protobuf"].lite = True

        self.options["benchmark"].shared = False
        self.options["spdlog"].shared = False

    def build(self):
        cmake = CMake(self)

        cmake.configure()
        cmake.build()
