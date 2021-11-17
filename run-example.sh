#!/usr/bin/env bash
# A simple script to run the example test from the Dockerfile

cd example/cpp || exit
mkdir build
cd build || exit
cmake ..
make -j4

cd bin || exit
./webserver &
./webclient
