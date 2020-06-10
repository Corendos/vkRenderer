#!/bin/bash

cd ../build
cmake .. -DCMAKE_BUILD_TYPE=Debug -Wno-dev && make shaders && make