#!/bin/bash
# Prepare all Book of Shaders shaders for browsing

SHADER_LIST="shader_list.txt"
rm -f "$SHADER_LIST"

echo "Converting Book of Shaders shaders to Vulkan format..."

find thebookofshaders -name "*.frag" -type f | while read shader; do
    basename="${shader##*/}"
    name="${basename%.frag}"
    output="shaders/bos_${name}.frag"
    
    # Convert using our Book of Shaders converter
    python3 /tmp/convert_book_of_shaders.py "$shader" "$output" 2>/dev/null
    
    if [ -f "$output" ]; then
        # Compile to SPIR-V
        output_spv="shaders/bos_${name}.spv"
        glslangValidator -V "$output" -o "$output_spv" 2>/dev/null
        
        if [ -f "$output_spv" ]; then
            echo "$output_spv" >> "$SHADER_LIST"
        fi
    fi
done

count=$(wc -l < "$SHADER_LIST" 2>/dev/null || echo 0)
echo "✓ Compiled $count shaders"
