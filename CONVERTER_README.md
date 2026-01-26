# ShaderToy to Vulkan SPIR-V Converter

A Rust-based tool for converting ShaderToy shaders to Vulkan-compatible GLSL and SPIR-V.

## Features

- **Automatic conversion** from ShaderToy GLSL to Vulkan GLSL
- **SPIR-V compilation** using naga with glslangValidator fallback
- **One-step workflow** - converts and compiles in a single command
- **Fast** - written in Rust, compiles quickly
- **Reliable** - uses industry-standard tools (naga + glslangValidator)

## Installation

Build from source:
```bash
cargo build --release
cp target/release/shadertoy2vulkan .
```

## Usage

### Basic Usage
```bash
./shadertoy2vulkan <input_file> [output_file]
```

### Examples
```bash
# Auto-generate output filename (removes _raw suffix, adds .frag)
./shadertoy2vulkan shaders/creation_by_silexars_raw.glsl

# Specify output filename
./shadertoy2vulkan shaders/bumped_sinusoidal_warp_raw.glsl shaders/bumped.frag
```

## What It Does

The converter performs these transformations:

1. **Adds Vulkan GLSL header** with proper version and layouts
2. **Converts ShaderToy uniforms** to Vulkan uniform buffer:
   - `iTime` → `ubo.iTime`
   - `iResolution` → `ubo.iResolution`
   - `iMouse` → `ubo.iMouse`
3. **Replaces function signature**: `mainImage()` → `main()`
4. **Expands common shortcuts**:
   - `#define t iTime` → direct `ubo.iTime` usage
   - `#define r iResolution.xy` → `ubo.iResolution.xy`
5. **Compiles to SPIR-V** using naga or glslangValidator

## Output Files

The tool creates two files:
- `<name>.frag` - Vulkan GLSL source
- `<name>.frag.spv` - Compiled SPIR-V binary

## Compilation Strategy

1. **First attempt**: Uses naga (pure Rust GLSL → SPIR-V compiler)
2. **Fallback**: If naga fails, uses glslangValidator (reference compiler)
3. **Error reporting**: Clear messages if both fail

## Requirements

- Rust toolchain (for building)
- glslangValidator (for SPIR-V compilation fallback)
  - Install via: `brew install glslang` (macOS)

## Advantages Over Python Script

- **Faster compilation** - Rust is faster than Python
- **Better error handling** - Comprehensive error messages
- **Type safety** - Rust's type system catches bugs at compile time
- **Single binary** - No Python dependencies needed
- **Integrated compilation** - Converts and compiles in one step
- **Smart fallback** - Uses glslangValidator if naga fails

## Limitations

- Designed specifically for ShaderToy shaders
- Does not handle multi-pass shaders (Buffer A, B, C, etc.)
- Some advanced GLSL features may require manual adjustment
- Texture channels beyond iChannel0 need manual uncommenting

## Integration

The `import_shader.sh` script uses this tool automatically:
```bash
./import_shader.sh <shadertoy_url_or_id> [name]
```

## Development

Source code is in `src/main.rs`:
- `convert_shadertoy_to_vulkan()` - Regex-based GLSL transformation
- `compile_glsl_to_spirv()` - naga compilation
- `compile_with_glslang()` - glslangValidator fallback

## License

Same as the parent project.
