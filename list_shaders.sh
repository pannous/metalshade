#!/bin/bash
# Create a list of all shader files for browsing
find thebookofshaders -name "*.frag" -type f | sort > shader_list.txt
echo "Found $(wc -l < shader_list.txt) shaders"
head -10 shader_list.txt
