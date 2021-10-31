#!/bin/bash

conan install . --install-folder=bin --build=missing
conan source . --source-folder=bin/source
conan build . --source-folder=bin/source --build-folder=bin
conan export-pkg . -f --source-folder=bin/source --build-folder=bin
#conan test --build=missing test_package twirp-cpp/development_bf680c26
