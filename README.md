# metalshade

**Run ShaderToy shaders natively on macOS Metal through Vulkan + MoltenVK**

A lightweight, high-performance ShaderToy viewer that renders GLSL fragment shaders using the Vulkan API, translated to Metal via MoltenVK. Perfect for testing shaders, learning graphics programming, or just enjoying beautiful procedural art on your Mac.

<div align="center">

[![Platform](https://img.shields.io/badge/platform-macOS-lightgrey.svg)](https://www.apple.com/macos/)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Vulkan](https://img.shields.io/badge/API-Vulkan-red.svg)](https://www.vulkan.org/)
[![Metal](https://img.shields.io/badge/backend-Metal-orange.svg)](https://developer.apple.com/metal/)

</div>

## ✨ Features

- 🎨 **Full ShaderToy compatibility** - Run shaders from shadertoy.com with minimal conversion
- ⚡ **Native Metal performance** - Hardware-accelerated rendering via MoltenVK
- 🖼️ **Fullscreen support** - Press F to immerse yourself (F11 also works)
- 🎯 **Multiple examples included** - Gradient, tunnel, plasma effects ready to run
- 🔄 **Easy shader switching** - Simple script to try different shaders
- 📦 **Zero dependencies** - Just Vulkan SDK, MoltenVK, and GLFW (via Homebrew)

## 🎬 Quick Start

```bash
# Install dependencies
brew install vulkan-loader molten-vk glfw glslang

# Clone and build
git clone https://github.com/YOUR_USERNAME/metalshade.git
cd metalshade
make

# Run the default shader (bump-mapped sinusoidal warp)
./run.sh
```

**Controls:**
- `F` or `F11` - Toggle fullscreen
- `ESC` - Exit

## 🎨 Try Different Shaders

```bash
# Animated color gradient (simple and fast)
./switch_shader.sh simple_gradient && ./run.sh

# Classic tunnel effect
./switch_shader.sh tunnel && ./run.sh

# Plasma waves
./switch_shader.sh plasma && ./run.sh
```

See all available shaders:
```bash
./switch_shader.sh
```

## 🏗️ Architecture

```
ShaderToy GLSL → SPIR-V → Vulkan API → MoltenVK → Metal → macOS GPU
```

**Why this matters:**
- **Vulkan portability** - Shaders work across platforms (not just macOS)
- **Modern graphics** - Learn Vulkan while enjoying Metal performance
- **No OpenGL** - Uses Apple's native Metal backend for better efficiency
- **SPIR-V intermediate** - Industry-standard shader format

## 📚 Documentation

- **[QUICKSTART.md](QUICKSTART.md)** - Fast reference for common tasks
- **[HOWTO.md](HOWTO.md)** - Convert ShaderToy shaders step-by-step
- **[popular_shaders.txt](popular_shaders.txt)** - 40+ curated shader recommendations

## 🎯 Example Shaders Included

| Shader | Description | Complexity |
|--------|-------------|------------|
| **Bumped Sinusoidal Warp** | Metallic surface with bump mapping (default) | Medium |
| **Simple Gradient** | Animated color waves | Beginner |
| **Tunnel** | Classic demoscene tunnel | Beginner |
| **Plasma** | Retro plasma effect | Beginner |

## 📦 Shader Collections

Download thousands of shaders from these curated packs:

- **[Geeks3D Demopack](https://www.geeks3d.com/hacklab/20231203/shadertoy-demopack-v23-12-3/)** - Hand-picked demos
- **[Raspberry Pi Collection](https://forums.raspberrypi.com/viewtopic.php?t=247036)** - 100+ optimized examples
- **[VirtualDJ Pack](https://www.virtualdjskins.co.uk/blog/shaders-for-virtualdj)** - 450+ VJ shaders
- **[shadertoy-rs](https://github.com/fmenozzi/shadertoy-rs)** - Browse/download client

## 🔧 Requirements

**System:**
- macOS 10.15+ (Catalina or later)
- Apple Silicon (M1/M2/M3) or Intel Mac with Metal support
- 4GB+ RAM recommended

**Dependencies:**
- Vulkan SDK (Homebrew: `vulkan-loader`, `vulkan-headers`)
- MoltenVK (`molten-vk`)
- GLFW 3.x (`glfw`)
- glslangValidator (`glslang`)

All installable via Homebrew:
```bash
brew install vulkan-loader molten-vk glfw glslang
```

## 🛠️ Building from Source

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/metalshade.git
cd metalshade

# Build
make

# Run
./run.sh
```

The build creates:
- `shadertoy_viewer` - Main executable
- `vert.spv` - Compiled vertex shader
- `frag.spv` - Compiled fragment shader

## 🎨 Adding Your Own Shaders

1. **Find a shader** on [ShaderToy.com](https://www.shadertoy.com/)
2. **Convert it** (see HOWTO.md):
   - Replace `mainImage(...)` with `main()`
   - Add Vulkan uniform buffer
   - Update `iTime`, `iResolution` to use `ubo.*`
3. **Save** to `examples/yourshader.frag`
4. **Run** it:
   ```bash
   ./switch_shader.sh yourshader
   ./run.sh
   ```

## 🚀 Performance

Tested on Apple M2 Pro:
- **Simple shaders**: 60 FPS @ 4K (locked to VSync)
- **Complex shaders**: 60 FPS @ 1080p
- **Raymarching**: 30-60 FPS depending on complexity

## 🐛 Troubleshooting

**"Failed to create Vulkan instance"**
```bash
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
./shadertoy_viewer
```

**Shader compilation errors**
- Check GLSL syntax
- Ensure all uniforms are declared
- Verify `ubo.*` references

**Low FPS**
- Try a simpler shader first
- Reduce window resolution
- Check Activity Monitor for GPU usage

## 📝 License

MIT License - see LICENSE file for details

## 🙏 Acknowledgments

- **ShaderToy** - Amazing community and platform
- **MoltenVK** - Vulkan on Metal translation layer
- **Khronos Group** - Vulkan API and SPIR-V
- Original shader authors - Creating beautiful procedural art

## 🔗 Links

- [ShaderToy.com](https://www.shadertoy.com/) - Thousands of shaders
- [Vulkan Tutorial](https://vulkan-tutorial.com/) - Learn Vulkan
- [MoltenVK](https://github.com/KhronosGroup/MoltenVK) - Vulkan on Metal
- [SPIR-V](https://www.khronos.org/spir/) - Shader intermediate format

---

<div align="center">
Made with 🎨 for the shader art community
</div>
