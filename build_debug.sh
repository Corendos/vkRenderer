#!/bin/bash

cd ./build
cmake .. -DCMAKE_BUILD_TYPE=Debug && make shaders && make
cd ..