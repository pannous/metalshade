# ShaderToy Viewer - Quick Start Guide

## Running the Viewer

```bash
cd /opt/other/qemu/host-demos/shadertoy/viewer
./run.sh
```

## Keyboard Controls

| Key | Action |
|-----|--------|
| **F** or **F11** | Toggle fullscreen mode |
| **ESC** | Exit viewer |

## Switching Shaders

Try the included examples:

```bash
# Animated color gradient (simple and fast)
./switch_shader.sh simple_gradient
./run.sh

# Classic tunnel effect
./switch_shader.sh tunnel
./run.sh

# Plasma waves
./switch_shader.sh plasma
./run.sh

# Back to the default bump-mapped surface
./switch_shader.sh shadertoy
./run.sh
```

## Available Shaders

List all available shaders:
```bash
./switch_shader.sh
```

Current examples:
- **simple_gradient** - Smooth animated colors (great for testing)
- **tunnel** - Classic demoscene tunnel
- **plasma** - Retro plasma effect
- **shadertoy** - Bumped sinusoidal warp (default)

## Download More Shaders

### Pre-Made Collections

1. **[Geeks3D Shadertoy Demopack](https://www.geeks3d.com/hacklab/20231203/shadertoy-demopack-v23-12-3/)** (December 2023)
   - Curated selection of cool demos
   - Single-pass and multi-pass shaders
   - Includes "Gaussian Splatting", "Enter the Matrix", and more

2. **[Raspberry Pi Collection](https://forums.raspberrypi.com/viewtopic.php?t=247036)**
   - 100+ OpenGL ES 3.0 examples
   - Optimized for real-time performance

3. **[VirtualDJ Mega Pack](https://www.virtualdjskins.co.uk/blog/shaders-for-virtualdj)**
   - 450+ shaders compiled from 20,000+ ShaderToy shaders
   - Tested for VJ performance

4. **[shadertoy-rs Client](https://github.com/fmenozzi/shadertoy-rs)**
   - Desktop app to browse and download directly
   - Rust-based, cross-platform

### From ShaderToy.com

Browse thousands of shaders at [shadertoy.com](https://www.shadertoy.com/).

Popular categories:
- **Procedural** - Algorithmic generation
- **Raymarching** - 3D rendering with distance fields
- **2D** - Classic effects and patterns
- **Abstract** - Artistic visualizations

See `popular_shaders.txt` for a curated list of 40+ recommended shaders organized by difficulty.

## Converting Downloaded Shaders

Downloaded shaders need conversion to Vulkan GLSL. See `HOWTO.md` for detailed instructions:

1. Replace `mainImage(...)` with `main()`
2. Add Vulkan uniform buffer declarations
3. Update `iTime`, `iResolution` to use `ubo.*`
4. Compile to SPIR-V with `glslangValidator`

## Tips

- **Start simple**: Try the included examples first
- **Test in windowed mode**: Easier to debug and iterate
- **Use fullscreen for demos**: Press F for the full experience
- **Check performance**: Complex shaders may reduce FPS
- **Single-pass shaders work best**: Multi-pass requires more setup

## Troubleshooting

**Shader won't compile**
- Check for syntax errors in the fragment shader
- Ensure all `iChannel*` samplers are declared
- Verify uniform buffer structure matches

**Low FPS in fullscreen**
- Try a simpler shader first
- Check GPU usage with Activity Monitor
- Reduce window resolution before going fullscreen

**Shader looks different from ShaderToy**
- Verify texture bindings (iChannel0-3)
- Check if shader uses multi-pass rendering
- Confirm all uniforms are properly updated

## Next Steps

1. Try all included examples to see different effects
2. Download a shader pack and convert a simple shader
3. Experiment with modifying existing shaders
4. Create your own procedural effects!

Happy shader hacking! 🎨✨
