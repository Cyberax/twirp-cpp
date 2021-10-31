cmake_minimum_required(VERSION 3.13)
project(twirp-cpp-test)

set(CMAKE_CXX_STANDARD 20)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

include(${CMAKE_SOURCE_DIR}/conan.cmake)
conan_cmake_run(CONANFILE ${CMAKE_SOURCE_DIR}/conanfile.py
        BUILD_TYPE ${CMAKE_BUILD_TYPE}
        BASIC_SETUP CMAKE_TARGETS
        BUILD missing)

### Testing
enable_testing()

add_executable(gproto
    tests/rpc-tests.cpp
    proto/validate/validate.proto
    proto/service1.proto
)
target_link_libraries(gproto
    CONAN_PKG::gtest
    CONAN_PKG::protobuf
    CONAN_PKG::abseil
    CONAN_PKG::twirp-cpp
)

add_test(AllTestsInFoo gproto)

include_directories(include)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/gen")
include_directories(${CMAKE_BINARY_DIR}/gen)

protobuf_generate(LANGUAGE cpp
    PROTO_PATH "${CMAKE_SOURCE_DIR}/proto"
    TARGET gproto
    IMPORT_DIRS "${CMAKE_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${CMAKE_BINARY_DIR}/gen"
)

protobuf_generate(LANGUAGE twirpcpp
    GENERATE_EXTENSIONS _client.cpp _client.hpp _server.cpp _server.hpp
    PLUGIN "${CONAN_BIN_DIRS_TWIRP-CPP}/protoc-gen-twirpcpp"
    TARGET gproto
    IMPORT_DIRS "${CMAKE_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${CMAKE_BINARY_DIR}/gen"
)
