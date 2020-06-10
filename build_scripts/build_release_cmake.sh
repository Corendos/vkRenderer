#!/bin/bash

cd ../build
cmake .. -DCMAKE_BUILD_TYPE=Release -Wno-dev && make shaders && make
cd ..