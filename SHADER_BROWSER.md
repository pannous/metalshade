# Shader Browser

Browse through 155+ Book of Shaders examples using arrow keys!

## Usage

```bash
./shadertoy_viewer
```

### Controls

- **← Left Arrow**: Previous shader
- **→ Right Arrow**: Next shader  
- **F / F11**: Toggle fullscreen
- **ESC**: Quit

## How It Works

1. Loads shader list from `shader_list.txt` (155 shaders)
2. On arrow key press:
   - Converts Book of Shaders format → Vulkan GLSL
   - Compiles to SPIR-V on-the-fly
   - Recreates graphics pipeline
3. Displays shader name and index in terminal

## Shader Sources

- **thebookofshaders/** - 155 GLSL fragment shaders
- All shaders automatically converted from Book of Shaders format

## Technical Details

- Hot-reloading: Shaders compile in ~100ms
- No pre-compilation needed
- Python converter handles format differences
- Vulkan pipeline recreation on shader switch

## Example Session

```
✓ Loaded 155 shaders
  Use ← → to browse shaders

[1/155] thebookofshaders/00/cmyk-halftone.frag
✓ Shader loaded

[2/155] thebookofshaders/00/halftone.frag
✓ Shader loaded
```
