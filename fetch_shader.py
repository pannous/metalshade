#!/usr/bin/env python3
"""
Fetch ShaderToy shader using browser automation to bypass CloudFlare
Usage: ./fetch_shader.py <url_or_id> [output_name]
"""

import sys
import json
import re
from pathlib import Path
from urllib.parse import urlparse

SHADERS_DIR = Path("shaders")


def extract_shader_id(url_or_id):
    """Extract shader ID from URL or return ID directly"""
    if "/" in url_or_id or "shadertoy.com" in url_or_id:
        match = re.search(r'/view/([a-zA-Z0-9]+)', url_or_id)
        if match:
            return match.group(1)
        raise ValueError(f"Could not extract shader ID from URL: {url_or_id}")
    return url_or_id


def sanitize_name(name):
    """Convert shader name to valid filename"""
    name = re.sub(r'[^\w\s-]', '', name.lower())
    name = re.sub(r'[-\s]+', '_', name)
    return name.strip('_')


def fetch_with_browser(shader_id):
    """
    This function prepares the request for browser automation.
    The actual browser work will be done by Claude's MCP tools.
    """
    url = f"https://www.shadertoy.com/view/{shader_id}"

    # Return the request info that Claude will use with browser tools
    return {
        "url": url,
        "shader_id": shader_id,
        "instructions": [
            f"Navigate to {url}",
            "Wait for page to load",
            "Extract shader data from page JavaScript/DOM",
            "Look for gShaderToy or shader code in page source",
            "Return shader name and code"
        ]
    }


def save_shader(shader_data, output_name):
    """Save shader JSON and GLSL code"""
    SHADERS_DIR.mkdir(exist_ok=True)

    # Save JSON
    json_path = SHADERS_DIR / f"{output_name}.json"
    with open(json_path, 'w') as f:
        json.dump(shader_data, f, indent=2)

    # Save raw GLSL
    shader_code = shader_data.get("code", "")
    if shader_code:
        raw_path = SHADERS_DIR / f"{output_name}_raw.glsl"
        with open(raw_path, 'w') as f:
            f.write(shader_code)
        return json_path, raw_path

    return json_path, None


def main():
    if len(sys.argv) < 2:
        print("Usage: ./fetch_shader.py <url_or_id> [output_name]")
        print()
        print("Examples:")
        print("  ./fetch_shader.py https://www.shadertoy.com/view/4l2XWK")
        print("  ./fetch_shader.py 4l2XWK")
        print("  ./fetch_shader.py XsXXDn seascape")
        sys.exit(1)

    url_or_id = sys.argv[1]
    output_name = sys.argv[2] if len(sys.argv) > 2 else None

    try:
        shader_id = extract_shader_id(url_or_id)
        request_info = fetch_with_browser(shader_id)

        # Output the request for Claude to handle with browser tools
        print(json.dumps(request_info, indent=2))

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
