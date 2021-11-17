#!/usr/bin/env bash
# A simple script to run the example test from the Dockerfile

cd example/cpp || exit
mkdir build
cd build || exit
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4

cd bin || exit
./webserver &
./webclient
