# Shader Browser

Browse through 132 working Book of Shaders examples using arrow keys!

**Features:**
- ✓ Auto-skip broken shaders
- ✓ Pre-filtered working shaders only
- ✓ No crashes or errors
- ✓ Hot-reloading (~100ms)

## Usage

```bash
./metalshade
```

### Controls

- **← Left Arrow**: Previous shader
- **→ Right Arrow**: Next shader  
- **F / F11**: Toggle fullscreen
- **ESC**: Quit

## How It Works

1. **Pre-filtering**: `test_shaders.sh` tests all shaders (132/155 pass)
2. **Startup**: Loads `shader_list.txt` with working shaders only
3. **Arrow key press**:
   - Converts Book of Shaders format → Vulkan GLSL
   - Compiles to SPIR-V (~100ms)
   - Recreates Vulkan graphics pipeline
   - If compilation fails, auto-skips to next shader
4. **Display**: Shows shader name and index in terminal

## Shader Sources

- **thebookofshaders/** - 155 total GLSL fragment shaders
- **shader_list.txt** - 132 pre-tested working shaders
- All shaders automatically converted from Book of Shaders format

### Why 132/155?

23 shaders filtered out due to:
- Missing `void main()` function (incomplete examples)
- Incompatible uniform types
- Shader-specific extensions not supported

## Technical Details

- Hot-reloading: Shaders compile in ~100ms
- No pre-compilation needed
- Python converter handles format differences
- Vulkan pipeline recreation on shader switch

## Example Session

```
✓ Loaded 132 shaders
  Use ← → to browse shaders

[1/132] thebookofshaders/02/hello_world.frag
✓ Shader loaded

[5/132] thebookofshaders/05/easing.frag
✓ Shader loaded

[50/132] thebookofshaders/11/lava-lamp.frag
✓ Shader loaded
```

## Updating Shader List

To regenerate the tested shader list:

```bash
./test_shaders.sh
```

This will:
1. Test all 155 shaders in `thebookofshaders/`
2. Filter out non-compiling shaders
3. Update `shader_list.txt` with working shaders only
