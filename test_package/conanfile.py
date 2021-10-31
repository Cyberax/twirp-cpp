from conans import ConanFile, CMake
import os


class ProtobufNiceTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    requires = ["gtest/1.10.0", "protobuf/3.12.4"]

    def build(self):
        cmake = CMake(self)
#        cmake.definitions.pop("CONAN_EXPORTED")
        cmake.configure()
        cmake.build()

    # def imports(self):
    #     self.copy("*.dll", dst="bin", src="bin")
    #     self.copy("*.dylib*", dst="bin", src="lib")

    def test(self):
        os.chdir("bin")
        self.run("./gproto")
