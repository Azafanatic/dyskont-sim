#!/usr/bin/bash

if [ -d "build" ]; then
  rm -rf build
fi

mkdir build
cd build
cmake ..
make

./sim 7200 600
