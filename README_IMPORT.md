# Consolidated ShaderToy Import Workflow

## Usage

The `./import` script consolidates the three-step process into one command:

```bash
./import https://www.shadertoy.com/view/fsVBDm
```

## What It Does

The script handles the complete workflow:

1. **Fetch**: Downloads shader from ShaderToy (requires browser automation)
2. **Convert**: Transforms ShaderToy GLSL → Vulkan GLSL
3. **Activate**: Copies to `shadertoy.frag` and compiles to `frag.spv`

## How It Works

### If Shader Already Fetched
If a `*_raw.glsl` file matching the shader ID exists in `shaders/`, the script:
- Converts it to Vulkan format
- Activates and compiles it
- Ready to run!

### If Shader Needs Fetching
The script exits with code 42, signaling that browser automation is needed.

**With Claude Code + Browser Automation:**
Claude automatically handles the fetch using `/fetch-shader`, then re-runs the import.

**Manual Workflow:**
1. Run `/fetch-shader <url>` in Claude Code, OR
2. Manually copy shader code to `shaders/<name>_raw.glsl`
3. Run `./import <url>` again

## Examples

```bash
# Shader already downloaded
./import https://www.shadertoy.com/view/fsVBDm
# ✓ Found: creation_by_silexars_raw.glsl
# → Converting...
# → Activating...
# → Compiling...
# ✓ Shader ready!

# New shader (needs fetch)
./import https://www.shadertoy.com/view/XsXXDn
# → Shader not found locally
# → Need to fetch from ShaderToy...
# (Claude will handle this via /fetch-shader if available)
```

## Replaced Scripts

This single script replaces:
- `fetch_shader_browser.sh` - Browser automation wrapper
- `import_shader.sh` - Conversion and compilation
- `switch_shader.sh` - Shader activation

## Dependencies

- **Python 3** - For script execution
- **glslangValidator** - For SPIR-V compilation
- **Claude Code** (optional) - For automatic browser automation
- **Playwright MCP** or **Claude in Chrome** (optional) - For fetching

## Manual Shader Fetch

If browser automation isn't available:

1. Go to https://www.shadertoy.com/view/XsXXDn
2. Copy the shader code from the editor
3. Save to `shaders/seascape_raw.glsl`
4. Run `./import https://www.shadertoy.com/view/XsXXDn`

## What Gets Created

```
shaders/
├── seascape_raw.glsl      # Original ShaderToy GLSL
├── seascape.frag          # Converted Vulkan GLSL
└── seascape.json          # Metadata (if fetched via automation)

shadertoy.frag             # Active shader (symlink/copy)
frag.spv                   # Compiled SPIR-V
```

## Next Steps

After successful import:
```bash
./run.sh    # View the shader!
```
