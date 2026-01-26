#!/bin/bash
# Test and filter working shaders

WORKING="working_shaders.txt"
rm -f "$WORKING"

echo "Testing shaders..."
count=0
working=0

find thebookofshaders -name "*.frag" -type f | sort | while read shader; do
    count=$((count + 1))
    
    # Check if shader has main()
    if ! grep -q "void main" "$shader" 2>/dev/null; then
        continue
    fi
    
    # Try to convert
    if ! python3 convert_book_of_shaders.py "$shader" /tmp/test.frag 2>/dev/null; then
        continue
    fi
    
    # Try to compile
    if glslangValidator -V /tmp/test.frag -o /tmp/test.spv >/dev/null 2>&1; then
        echo "$shader" >> "$WORKING"
        working=$((working + 1))
    fi
done

echo "Tested shaders, found $working working"
mv "$WORKING" shader_list.txt 2>/dev/null
wc -l shader_list.txt
