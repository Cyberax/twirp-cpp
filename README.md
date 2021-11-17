# Twirp-Cpp

`twirp-cpp` is a C++ implementation of Twirp v7 protocol: https://twitchtv.github.io/twirp/docs/spec_v7.html As a brief
overview, this protocol can be described in a few words as "using HTTP POST to send protobuf-serialized requests and 
getting protobuf as the response".

The protocol is incredibly simple and can be implemented in a few hundreds lines of code. Theis repository structure
contains the following subprojects:

1. A Golang-based code generator that creates stubs for Twirp services (`src` directory).
2. Header files for the C++ clients and server (`include`).
3. `test_package` with comprehensive tests that are run inside CI/CD.
4. An example of a complete C++ project (`example`) that implements server and client side for a simple Twirp service.

## Building Twirp-Cpp

`twirp-cpp` uses Conan for package management and should also be consumed as a Conan package. The build process is 
thus pretty simple: 

```
conan create .
```

It does the usual Conan steps: creates a temporary directory, copies the source 
code there, installs dependencies, builds the package and runs the tests from the `test_package`. The build requires
`Go` compiler to be available (version 1.17 or later), the produced binary artifacts don't require `Go`.

If you want to do these steps yourself, you should do the following:
1. `conan install . --install-folder=bin --build=missing` - creates a subfolder `bin`, and installs the 
required dependencies.
2. `conan source . --source-folder=bin/source` - copies the source code of `twirp-cpp` into the install directory.
3. `conan build . --source-folder=bin/source --build-folder=bin` - builds the `twirp-cpp` binary artifacts for 
the target platform (which in this case just includes compiling the Go-based code generator).
4. `conan export-pkg . -f --source-folder=bin/source --build-folder=bin` - exports the compiled artifacts and the
auxiliary files into the local Conan cache, after which the project can be consumed by other projects.
5. `conan test --build=missing test_package twirp-cpp/<VER>` - runs the tests in `test_package`

It's also possible to cross-compile `twirp-cpp` for other platforms by specifying it for `conan create .` method. 
Here is the list of tested and supported platforms:
```bash
conan create . -s os=Linux -s arch=x86_64 --test-folder None
conan create . -s os=Linux -s arch=armv8 --test-folder None
conan create . -s os=Macos -s arch=x86_64 --test-folder None
conan create . -s os=Macos -s arch=armv8 --test-folder None
conan create . -s os=Windows -s arch=x86_64 --test-folder None
```

Tests are needed to be disabled, since they can't be run on non-host architectures. The CI/CD step does test them
using `qemu` CPU emulation in Docker.

## Using Twirp-Cpp

`twirp-cpp` can be consumed as a regular Conan package. It provides C++ headers and installs a Go-based code generator
into your binary path. 

**However!** It does NOT automatically add required dependencies on packages like `Abseil` and `Protobuf`  
because it makes it very complicated to cross-build `twirp-cpp` for multiple architectures. Make sure to add the
required dependencies manually.

The minimal Conan dependency file for a consumer of Twirp would thus look like:
```
[requires]
twirp-cpp/<VERSION>
cpp-httplib/0.9.7
jsoncpp/1.9.3
protobuf/3.12.4
abseil/20210324.2

[generators]
cmake
```

If you want to enable SSL for client or the server, you'll need to add `openssl/1.1.1l` to the list of dependencies and
add preprocessor definition `CPPHTTPLIB_OPENSSL_SUPPORT` in your CMake file and/or source code.

## CMake and Protobuf

CMake has support for protobuf that is pretty easy to use. Let's start with a typical preamble that initializes the
Conan system and declares the dependency source:

```bash
cmake_minimum_required(VERSION 3.13)
project(example)

set(CMAKE_CXX_STANDARD 20)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

include(${CMAKE_SOURCE_DIR}/conan.cmake)
conan_cmake_run(CONANFILE ${CMAKE_SOURCE_DIR}/conanfile.txt
        BUILD_TYPE ${CMAKE_BUILD_TYPE}
        BASIC_SETUP CMAKE_TARGETS
        BUILD missing)        
```

Next you need to add your C++ binary (`webserver`), and declare all the required protobuf files in the source code list.
Then you need to attach the dependent packages so that CMake would know about them:
```bash
add_executable(webserver
    my-server.cpp
    proto/service.proto
)
  
target_link_libraries(webserver
    CONAN_PKG::twirp-cpp
    CONAN_PKG::abseil
    CONAN_PKG::protobuf
    CONAN_PKG::cpp-httplib
    CONAN_PKG::jsoncpp
    CONAN_PKG::openssl
)  
```

The above steps are standard for most C++ projects that use Conan. However, there's no way for the standard CMake 
tooling to know what to do with `.proto` files. So we need to add a bit more:

```bash
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/gen")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/gen")

protobuf_generate(LANGUAGE cpp
    PROTO_PATH "${CMAKE_SOURCE_DIR}/proto"
    TARGET webserver
    IMPORT_DIRS "${CMAKE_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen"
)

protobuf_generate(LANGUAGE twirpcpp
    GENERATE_EXTENSIONS _client.cpp _client.hpp _server.cpp _server.hpp
    PLUGIN "${CONAN_BIN_DIRS_TWIRP-CPP}/protoc-gen-twirpcpp"
    TARGET webserver
    IMPORT_DIRS "${CMAKE_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen"
)
```

First, this code creates a `${CMAKE_BINARY_DIR}/gen` folder where the generated files would be placed.
This folder is unfortunately not created by the `protobuf_generate` target, so we must do it manually. We also need
to add it to the includes search path.

Then the first `protobuf_generate` stanza invokes the regular `protoc` compiler to generate standard protobuf 
bindings for C++. In this case it would produce two file: `gen/service.pb.cc` and `gen/service.pb.h`. These files
are then added as dependencies to the `webserver` target.

The second `protobuf_generate` stanza creates the Twirp service stubs named: `gen/service_client.hpp`, 
`gen/service_client.cpp`, `gen/service_server.hpp`, `gen/service_server.cpp`. 

And that's it! CMake does all the required heavy lifting. 


## Creating a server

See SERVER.md for detailed instructions and the discussion of generated code for the server side.

## Creating a client

See CLIENT.md for detailed instructions and the discussion of generated code for the client side.

## Developing Twirp-Cpp

See DEVELOPMENT.md for detailed instruction on developing/fixing the Twirp-Cpp.
