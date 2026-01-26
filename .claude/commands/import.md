# Import ShaderToy Shader (Complete Workflow)

**Command**: `/import`

**Description**: One-stop ShaderToy importer that fetches, converts, compiles, and activates a shader in a single command

**Usage**:
```
/import https://www.shadertoy.com/view/XsXXDn
/import https://www.shadertoy.com/view/4l2XWK
/import fsVBDm
```

## Instructions

This skill performs the complete ShaderToy import workflow:
1. Fetch shader from ShaderToy (browser automation)
2. Convert ShaderToy GLSL → Vulkan GLSL
3. Compile to SPIR-V
4. Activate shader (ready to run)

### Step 1: Parse Input & Check Cache

1. **Extract shader ID** from URL or use ID directly:
   - If input contains `/view/`, extract the ID after it
   - Otherwise use the input as-is

2. **Check if already fetched**:
   - Look in `shaders/` directory for files matching pattern `*{shader_id}*_raw.glsl`
   - If found, skip to Step 3 (Convert)
   - If not found, proceed with Step 2 (Fetch)

### Step 2: Fetch Shader (Browser Automation)

Use Playwright browser automation to fetch the shader:

1. **Navigate**: Use `mcp__plugin_playwright_playwright__browser_navigate` to:
   ```
   https://www.shadertoy.com/view/{SHADER_ID}
   ```

2. **Wait for CloudFlare**: Use `mcp__plugin_playwright_playwright__browser_wait_for` with 5 seconds

3. **Extract shader data**: Use `mcp__plugin_playwright_playwright__browser_evaluate`:
   ```javascript
   () => {
       const title = document.title;
       let code = null;
       if (window.CodeMirror) {
           const cmElements = document.querySelectorAll('.CodeMirror');
           if (cmElements.length > 0) {
               const cm = cmElements[0].CodeMirror;
               if (cm) code = cm.getValue();
           }
       }
       return { title, code, length: code ? code.length : 0 };
   }
   ```

4. **Sanitize filename**: Convert shader title to valid filename:
   - Lowercase, replace non-alphanumeric with underscores
   - Remove leading/trailing underscores

5. **Save raw shader**: Write to `shaders/{name}_raw.glsl`

6. **Close browser**: Use `mcp__plugin_playwright_playwright__browser_close`

### Step 3: Convert to Vulkan GLSL

Read the raw shader file and convert using these transformations:

1. **Add Vulkan header**:
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
   ```

2. **Replace uniform references**:
   - `iTime` → `ubo.iTime`
   - `iResolution` → `ubo.iResolution`
   - `iMouse` → `ubo.iMouse`
   - `iTimeDelta` → `0.016`
   - `iFrame` → `int(ubo.iTime * 60.0)`
   - `iFrameRate` → `60.0`

3. **Replace mainImage with main**:
   - `void mainImage(out vec4 fragColor, in vec2 fragCoord)` → `void main()`

4. **Expand shortcuts** (common in code-golf shaders):
   - Standalone `t` → `ubo.iTime`
   - Standalone `r` → `ubo.iResolution.xy`

5. **Save converted shader**: Write to `shaders/{name}.frag`

### Step 4: Compile to SPIR-V

1. **Activate shader**: Copy `shaders/{name}.frag` to `shadertoy.frag`

2. **Compile**: Run `glslangValidator -V shadertoy.frag -o frag.spv`

3. **Check for errors**: If compilation fails, report errors and exit

### Step 5: Report Success

Tell the user:
```
✓ Fetched shader: {Shader Name}
✓ Converted: shaders/{name}.frag
✓ Activated: shadertoy.frag
✓ Compiled: frag.spv

Ready to run!
  ./run.sh
```

## Example Workflow

```
User: /import https://www.shadertoy.com/view/XsXXDn

→ Fetching shader XsXXDn from ShaderToy...
✓ Fetched: Seascape by TDM (2847 chars)
✓ Saved: shaders/seascape_raw.glsl

→ Converting to Vulkan GLSL...
✓ Converted: shaders/seascape.frag

→ Activating shader...
✓ Copied to: shadertoy.frag

→ Compiling to SPIR-V...
✓ Compiled: frag.spv

Ready to run!
  ./run.sh
```

## Error Handling

- **CloudFlare blocking**: Wait longer (up to 10 seconds)
- **CodeMirror not found**: Report error, suggest manual copy
- **Compilation errors**: Display full error output
- **File I/O errors**: Report and exit gracefully

## Notes

- Uses Python's `convert_shader.py` logic inline (no external calls needed)
- All regex transformations happen in-memory
- Automatically detects already-fetched shaders (caching)
- Works for any public ShaderToy shader
- Does NOT sign in or access private shaders
- Browser automation requires Playwright MCP

## Advanced Usage

The skill is smart about caching:
```
/import XsXXDn          # First run: fetch + convert + compile
/import XsXXDn          # Second run: skip fetch, just convert + compile
```

This makes it fast to re-process shaders after editing the raw file.
