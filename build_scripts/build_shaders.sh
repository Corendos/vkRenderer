#!/bin/bash

for SHADER_SOURCE in ../resources/shaders/*.frag ; do
		glslc $SHADER_SOURCE -o $SHADER_SOURCE.spv || exit 1
done

for SHADER_SOURCE in ../resources/shaders/*.vert ; do
		glslc $SHADER_SOURCE -o $SHADER_SOURCE.spv || exit 1
done