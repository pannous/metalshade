# How to Run ShaderToy Shaders on macOS with Vulkan + MoltenVK

## Quick Start

```bash
cd /opt/other/qemu/host-demos/shadertoy/viewer
./run.sh
```

You should see a beautiful animated shader with bump-mapped metallic surface! 🎨

## What You're Seeing

The **Bumped Sinusoidal Warp** shader from ShaderToy, running natively on Metal through:
- **GLSL** fragment shader code
- **SPIR-V** compilation (Vulkan's intermediate format)
- **Vulkan API** calls
- **MoltenVK** translation layer
- **Metal** rendering on your Apple GPU

## Converting Your Own ShaderToy Shaders

### 1. Save the ShaderToy shader

Visit any shader on [ShaderToy](https://www.shadertoy.com/), copy the code.

### 2. Convert to Vulkan GLSL

Replace:
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

Then replace all references:
- `iTime` → `ubo.iTime`
- `iResolution` → `ubo.iResolution` (or `ubo.iResolution.xy`)
- `iMouse` → `ubo.iMouse`

### 3. Compile to SPIR-V

```bash
glslangValidator -V yourshader.frag -o frag.spv
```

### 4. Run the viewer

```bash
./run.sh
```

## Editing the Current Shader

1. Edit `shadertoy.frag`
2. Run `make` to recompile
3. Run `./run.sh` to see your changes

## Adding Textures

The viewer currently has a simple procedural texture in iChannel0. To use actual images:

1. Load your texture in `createTextureImage()` function
2. Replace the procedural pixel generation with image loading
3. Use libraries like stb_image.h for loading PNG/JPG files

## Performance

Currently running at ~60 FPS with VSync on Apple M2 Pro. Performance depends on:
- Shader complexity (number of instructions)
- Texture sampling operations
- Resolution (currently 1280x720)

## Troubleshooting

**"Failed to create Vulkan instance"**
- Check: `export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json`
- Verify MoltenVK is installed: `brew list molten-vk`

**"Window doesn't appear"**
- Check if process is running: `ps aux | grep shadertoy_viewer`
- Try running with verbose: `export MVK_CONFIG_LOG_LEVEL=3`

**Shader looks wrong**
- Verify GLSL conversion (especially uniforms)
- Check SPIR-V compilation: `glslangValidator -V yourshader.frag`
- Compare with original on ShaderToy website

## Advanced: Adding More Channels

ShaderToy supports iChannel0-3 for multiple textures. To add more:

1. Add more sampler bindings in the fragment shader:
```glsl
layout(binding = 2) uniform sampler2D iChannel1;
layout(binding = 3) uniform sampler2D iChannel2;
```

2. Create additional textures in C++ code (similar to `createTextureImage()`)

3. Update descriptor set layout and bindings

## Next Steps

- Try simpler shaders first (just colors, simple patterns)
- Gradually add complexity (textures, raymarching)
- Experiment with resolution and performance
- Port your favorite ShaderToy creations!

## Resources

- [ShaderToy](https://www.shadertoy.com/) - Thousands of shaders to try
- [Vulkan GLSL](https://www.khronos.org/opengl/wiki/OpenGL_Shading_Language) - Language reference
- [MoltenVK](https://github.com/KhronosGroup/MoltenVK) - Vulkan implementation for Metal
- [SPIR-V](https://www.khronos.org/spir/) - Intermediate shader format
