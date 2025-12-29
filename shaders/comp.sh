#!/bin/sh

glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
glslc nonhardcode.vert -o nonhardcode_vert.spv