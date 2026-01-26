# metalshade - ShaderToy Viewer for macOS

Native ShaderToy shader viewer using **Vulkan + MoltenVK** to render GLSL shaders with Metal on macOS.

```
ShaderToy GLSL → SPIR-V → Vulkan API → MoltenVK → Metal → macOS Display
```

## Quick Start

```bash
./run.sh
```

**Controls**: `F/F11` fullscreen | `ESC` exit

## Features

- Hardware-accelerated Metal rendering on Apple Silicon via MoltenVK
- Real-time animation with iTime, iResolution, iMouse uniforms
- Texture support (iChannel0, expandable to iChannel1-3)
- Bump mapping and lighting effects
- ~60 FPS with VSync at 1280x720

## Requirements

```bash
brew install molten-vk glfw glslang
```

## Building

```bash
make          # Compiles shaders (.vert/.frag → .spv) and C++ viewer
```

## Importing ShaderToy Shaders

### Quick Start (One Command)

In Claude Code:
```bash
/import https://www.shadertoy.com/view/XsXXDn
./run.sh
```

That's it! The `/import` skill handles fetching, converting, compiling, and activating in one step.

### Or use the standalone script:

```bash
./import https://www.shadertoy.com/view/XsXXDn
./run.sh
```

### How It Works

The `/import` skill (or `./import` script) performs these steps automatically:

```bash
1. Fetch shader using browser automation (Playwright)
2. Convert ShaderToy GLSL → Vulkan GLSL
3. Compile to SPIR-V (frag.spv)
4. Activate shader (copy to shadertoy.frag)
```

All done in one command! See `README_IMPORT.md` for details.

### What Gets Converted

The conversion script automatically:
- ✓ Adds Vulkan `#version 450` header
- ✓ Converts `mainImage()` → `main()`
- ✓ Wraps uniforms in UBO: `iTime` → `ubo.iTime`
- ✓ Expands code-golf shortcuts: `t` → `ubo.iTime`, `r` → `ubo.iResolution.xy`
- ✓ Handles texture channels (iChannel0-3)
- ✓ Preserves shader credits and comments

## Switching Shaders

```bash
./switch_shader.sh simple_gradient  # Animated gradient
./switch_shader.sh tunnel           # Classic tunnel
./switch_shader.sh plasma           # Plasma waves
./switch_shader.sh                  # List all available
```

All shaders are stored in `shaders/`. Run `./run.sh` after switching to see the new shader.

## Converting ShaderToy Shaders

**1. Get shader code** from [shadertoy.com](https://www.shadertoy.com/)

**2. Convert to Vulkan GLSL** - Replace:
```glsl
void mainImage( out vec4 fragColor, in vec2 fragCoord )
```
With:
```glsl
#version 450
layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;

void main()
```

**3. Update uniform references**: `iTime` → `ubo.iTime`, `iResolution` → `ubo.iResolution.xy`, `iMouse` → `ubo.iMouse`

**4. Compile**:
```bash
glslangValidator -V yourshader.frag -o frag.spv
./run.sh
```

## Popular Shaders to Try

See `popular_shaders.txt` for 40+ curated shaders organized by difficulty:

**Beginner**: Seascape (XsXXDn), Simple Fire (4dX3Rn), Grid Pattern (XsGfzV)
**Intermediate**: Raymarching Primitives (ldX3W8), Flow Field (XsX3RB), Rain (MdlXz8)
**Advanced**: Elevated Terrain (3lsSzf), Protean Clouds (XlfGRj), Volcanic (4sl3Dr)
**Abstract**: Mandelbrot (4dX3WH), Kaleidoscope (XdX3W8), Apollonian (4ttGDr)

## Shader Packs

Pre-compiled collections (require conversion):
- [Geeks3D Demopack](https://www.geeks3d.com/hacklab/20231203/shadertoy-demopack-v23-12-3/) - Curated single/multi-pass shaders
- [Raspberry Pi Collection](https://forums.raspberrypi.com/viewtopic.php?t=247036) - 100+ OpenGL ES 3.0 examples
- [VirtualDJ Pack](https://www.virtualdjskins.co.uk/blog/shaders-for-virtualdj) - 450+ real-time optimized shaders
- [shadertoy-rs](https://github.com/fmenozzi/shadertoy-rs) - Desktop browser/downloader

## Troubleshooting

**"Failed to create Vulkan instance"**
```bash
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
brew list molten-vk  # Verify installation
```

**Shader compilation errors**
```bash
glslangValidator -V yourshader.frag  # Check for syntax errors
```

**Performance issues**
- Try simpler shaders first (simple_gradient, tunnel)
- Reduce window resolution before fullscreen
- Single-pass shaders work best

## Adding Textures

Current setup has procedural texture in iChannel0. To load images:

1. Modify `createTextureImage()` in C++ code
2. Use stb_image.h for PNG/JPG loading
3. Add multiple channels with additional bindings:
```glsl
layout(binding = 2) uniform sampler2D iChannel1;
layout(binding = 3) uniform sampler2D iChannel2;
```

## Technical Details

- **Resolution**: 1280x720 (configurable in code)
- **Texture**: 256x256 procedural gradient
- **Descriptor Sets**: Double-buffered for smooth frame updates
- **Performance**: Shader-dependent; raymarching shaders more intensive than 2D effects

## References

- [ShaderToy](https://www.shadertoy.com/) - Thousands of shaders to explore
- [MoltenVK](https://github.com/KhronosGroup/MoltenVK) - Vulkan for Metal
- [SPIR-V](https://www.khronos.org/spir/) - Shader intermediate format
- [Vulkan GLSL](https://www.khronos.org/opengl/wiki/OpenGL_Shading_Language) - Language reference

## Additional Resources

- [ISF (Interactive Shader Format)](https://isf.video/) - Alternative shader format
- [shadertoy-browser](https://github.com/repi/shadertoy-browser) - Rust-based browser
- [Metal Shader Converter](https://developer.apple.com/metal/shader-converter/) - Apple's GLSL→MSL tool
