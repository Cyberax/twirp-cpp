cmake_minimum_required(VERSION 3.13)
project(cppws)

set(CMAKE_CXX_STANDARD 20)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

include(${CMAKE_SOURCE_DIR}/conan.cmake)
conan_cmake_run(CONANFILE ${CMAKE_SOURCE_DIR}/conanfile.py
        BUILD_TYPE ${CMAKE_BUILD_TYPE}
        BASIC_SETUP CMAKE_TARGETS
        BUILD missing)

### ASAN support
if(ENABLE_ASAN)
    message("Enabling ASAN")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

############################################################
add_library(protos
    proto/service.proto
)

target_link_libraries(protos
    CONAN_PKG::twirp-cpp
    CONAN_PKG::abseil
    CONAN_PKG::protobuf
    CONAN_PKG::cpp-httplib
    CONAN_PKG::jsoncpp
    CONAN_PKG::openssl
)

protobuf_generate(LANGUAGE cpp
    PROTO_PATH "${CMAKE_SOURCE_DIR}/proto"
    TARGET protos
    IMPORT_DIRS "${CMAKE_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen"
)

protobuf_generate(LANGUAGE twirpcpp
    GENERATE_EXTENSIONS _client.cpp _client.hpp _server.cpp _server.hpp
    PLUGIN "${CONAN_BIN_DIRS_TWIRP-CPP}/protoc-gen-twirpcpp"
    TARGET protos
    IMPORT_DIRS "${CMAKE_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen"
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/gen")
include_directories(${CMAKE_CURRENT_BINARY_DIR}/gen)

############################################################
add_executable(webserver
    tw-server.cpp
)
target_link_libraries(webserver protos)

############################################################
add_executable(webclient
    tw-client.cpp
)
target_link_libraries(webclient protos)
