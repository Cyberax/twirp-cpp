#!/bin/bash

# The simple version of the build process
conan create .

# Step-by-step build
#conan install . --install-folder=bin --build=missing
#conan source . --source-folder=bin/source
#conan build . --source-folder=bin/source --build-folder=bin
#conan export-pkg . -f --source-folder=bin/source --build-folder=bin
#conan test --build=missing test_package twirp-cpp/<VER>
