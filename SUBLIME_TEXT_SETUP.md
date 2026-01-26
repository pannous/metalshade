# Sublime Text Setup for Metalshade

This guide shows how to integrate metalshade with Sublime Text for seamless shader development with clickable error messages.

## What You Get

- **Click errors to jump to the line** in your `.glsl` file
- **Build with Cmd+B** (Mac) or Ctrl+B (Windows/Linux)
- **Syntax highlighting** for GLSL shaders
- **Instant error feedback** in the build panel

## Step 1: Build System (Already Done!)

A build system file has been created at:
```
~/Library/Application Support/Sublime Text 3/Packages/User/metalshade.sublime-build
```

This file contains:
```json
{
    "shell_cmd": "./metalshade \"$file\"",
    "file_regex": "^ERROR: (.+?):(\\d+):(\\d+)?: (.*)$",
    "working_dir": "/opt/3d/metalshade",
    "selector": "source.glsl, source.c.glsl"
}
```

### What This Does

- **`file_regex`**: Parses error messages from glslangValidator
  - Capture group 1: filename
  - Capture group 2: line number  
  - Capture group 3: column number (optional)
  - Capture group 4: error message
  
- **`selector`**: Automatically activates for `.glsl` files

## Step 2: How to Use

1. **Open a shader file** in Sublime Text:
   ```bash
   subl /opt/3d/metalshade/thebookofshaders/09/dots1.frag
   ```

2. **Select the build system**:
   - Go to: `Tools` → `Build System` → `metalshade`
   - Or just press `Cmd+B` and it will auto-select

3. **Build the shader**:
   - Press `Cmd+B` (Mac) or `Ctrl+B` (Windows/Linux)
   - Or go to `Tools` → `Build`

4. **If there are errors**:
   - They appear in the build panel at the bottom
   - **Click any error line** to jump directly to that line in the `.glsl` file
   - Fix the error and press `Cmd+B` again

## Step 3: GLSL Syntax Highlighting (Optional)

If you don't have GLSL syntax highlighting yet:

1. Press `Cmd+Shift+P` (Mac) or `Ctrl+Shift+P` (Windows/Linux)
2. Type: `Install Package`
3. Search for: `GLSL` or `OpenGL Shading Language`
4. Install it

**Or** simply use C++ syntax (good enough):
- View → Syntax → C++

## Example Workflow

```bash
# 1. Open a shader
subl thebookofshaders/18/hatch.frag

# 2. Press Cmd+B to build
# Output appears:
#   ✓ Loading shader: thebookofshaders/18/hatch.frag
#   ✓ Converted: .../hatch.glsl
#   ERROR: .../hatch.glsl:30: 'a' : vector swizzle selection out of range
#   ✗ Shader compilation failed

# 3. Click the error line → jumps to hatch.glsl:30

# 4. Fix the error

# 5. Press Cmd+B again → Success!
#   ✓ Compiled: .../hatch.frag.spv
#   ✓ Vertex shader: .../hatch.vert.spv
```

## Error Pattern Format

Sublime Text recognizes errors in this format:
```
ERROR: /path/to/file.glsl:30: error message
```

The build system uses this regex to parse them:
```regex
^ERROR: (.+?):(\d+):(\d+)?: (.*)$
```

This captures:
- Line 30: `(\d+)` - line number
- Column (optional): `(\d+)?`
- File path: `(.+?)`
- Error message: `(.*)$`

## Troubleshooting

### Errors Not Clickable?

Check that the build system is selected:
- `Tools` → `Build System` → `metalshade`

### Build System Not Showing Up?

Restart Sublime Text after creating the build file.

### Wrong Directory?

The build system runs from `/opt/3d/metalshade`. If metalshade is elsewhere, edit:
```bash
subl ~/Library/Application\ Support/Sublime\ Text\ 3/Packages/User/metalshade.sublime-build
```

And change `"working_dir": "/opt/3d/metalshade"` to your path.

## Advanced: Custom Key Bindings

Add to your key bindings for even faster builds:

1. `Preferences` → `Key Bindings`
2. Add to the user file:

```json
[
    {
        "keys": ["f5"],
        "command": "build",
        "context": [{"key": "selector", "operator": "equal", "operand": "source.glsl"}]
    }
]
```

Now press **F5** to build any `.glsl` file!

## Files Created by Metalshade

For `shader.frag`, metalshade creates:
- `shader.glsl` - Converted to Vulkan GLSL (this is what you edit when fixing errors)
- `shader.frag.spv` - Compiled fragment shader
- `shader.vert.spv` - Compiled vertex shader

All files stay in the same directory as the original shader.

## Summary

✓ Build system installed
✓ Error regex configured  
✓ Errors are clickable
✓ Ready to use!

Press `Cmd+B` and start developing shaders!
