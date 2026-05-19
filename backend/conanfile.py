from conan import ConanFile


class ChessEngineBackend(ConanFile):
    name = "chess_engine_backend"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("fmt/10.2.1")
        self.requires("spdlog/1.13.0")

    def build_requirements(self):
        self.test_requires("gtest/1.14.0")
