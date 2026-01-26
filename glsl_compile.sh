#!/bin/bash
# Wrapper for glslangValidator that adds source line context to errors

SHADER_FILE="$1"
OUTPUT_FILE="$2"

# Run glslangValidator and capture output
OUTPUT=$(glslangValidator -S frag -V "$SHADER_FILE" -o "$OUTPUT_FILE" 2>&1)
RESULT=$?

# Process output line by line
echo "$OUTPUT" | while IFS= read -r line; do
    # Check if this is an error line
    if [[ "$line" =~ ^ERROR:\ (.+):([0-9]+):\ (.+)$ ]]; then
        FILE="${BASH_REMATCH[1]}"
        LINE_NUM="${BASH_REMATCH[2]}"
        ERROR_MSG="${BASH_REMATCH[3]}"

        # Print formatted error header
        echo "GLSL ERROR:"

        # Extract and print the offending line
        if [ -f "$FILE" ]; then
            OFFENDING_LINE=$(sed -n "${LINE_NUM}p" "$FILE")
            echo "$OFFENDING_LINE"
        fi

        # Print the error in standard format
        echo "$FILE:$LINE_NUM: error: $ERROR_MSG"
    else
        # Pass through non-error lines
        echo "$line"
    fi
done

exit $RESULT
