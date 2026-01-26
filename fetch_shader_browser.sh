#!/bin/bash
# Fetch ShaderToy shader using browser automation
# This is a wrapper that calls Claude with the /fetch-shader skill
# Usage: ./fetch_shader_browser.sh <url_or_id> [output_name]

INPUT="$1"
OUTPUT_NAME="$2"

if [ -z "$INPUT" ]; then
    echo "Fetch ShaderToy shaders using browser automation"
    echo ""
    echo "Usage: $0 <url_or_id> [output_name]"
    echo ""
    echo "Examples:"
    echo "  $0 https://www.shadertoy.com/view/4l2XWK"
    echo "  $0 4l2XWK"
    echo "  $0 XsXXDn seascape"
    echo ""
    echo "This uses Playwright browser automation to bypass CloudFlare."
    echo "Run this in a Claude Code session or use: /fetch-shader $INPUT"
    exit 1
fi

echo "Note: This script requires Claude Code with Playwright MCP enabled."
echo "To fetch shader '$INPUT', run in Claude Code:"
echo ""
echo "  /fetch-shader $INPUT${OUTPUT_NAME:+ $OUTPUT_NAME}"
echo ""
echo "Or ask Claude: 'Fetch shader $INPUT'"
