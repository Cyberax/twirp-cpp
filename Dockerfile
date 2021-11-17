FROM golang:1.17-bullseye as builder

RUN apt-get update; DEBIAN_FRONTEND=noninteractive apt-get install -yq --no-install-recommends \
    vim-nox less sudo curl python3 python3-distutils python3-setuptools python3-pip \
    unzip cmake

RUN pip3 install conan

COPY . /package
WORKDIR /package

######################
#### Build packages
######################

# Conan by default tries to use Ye Olde ABI from C++03, resulting in linker errors
RUN conan profile new default --detect; \
    conan profile update settings.compiler.libcxx=libstdc++11 default; \
    conan profile update settings.compiler.cppstd=20 default

# Native Linux build (whatever the arch) with the tests
RUN conan create . --build=missing

# Run the build matix
RUN conan create . -s os=Linux -s arch=x86_64 --test-folder None
RUN conan create . -s os=Linux -s arch=armv8 --test-folder None
RUN conan create . -s os=Macos -s arch=x86_64 --test-folder None
RUN conan create . -s os=Macos -s arch=armv8 --test-folder None
RUN conan create . -s os=Windows -s arch=x86_64 --test-folder None
