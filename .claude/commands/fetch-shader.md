# Fetch ShaderToy Shader

**Command**: `/fetch-shader`

**Description**: Automatically downloads a ShaderToy shader using browser automation to bypass CloudFlare protection

**Usage**:
```
/fetch-shader https://www.shadertoy.com/view/4l2XWK
/fetch-shader 4l2XWK
/fetch-shader XsXXDn seascape
```

## Instructions

You will fetch a ShaderToy shader using Playwright browser automation. Follow these steps:

1. **Parse input**: Extract shader ID from URL or use ID directly
   - If input contains `/view/`, extract the ID after it
   - Otherwise use the input as-is

2. **Navigate**: Use `mcp__plugin_playwright_playwright__browser_navigate` to:
   ```
   https://www.shadertoy.com/view/{SHADER_ID}
   ```

3. **Wait for CloudFlare**: Use `mcp__plugin_playwright_playwright__browser_wait_for` with 5 seconds to let CloudFlare verification complete

4. **Extract shader data**: Use `mcp__plugin_playwright_playwright__browser_evaluate` with this code:
   ```javascript
   () => {
       // Get title for shader name
       const title = document.title;

       // Extract code from CodeMirror editor
       let code = null;
       if (window.CodeMirror) {
           const cmElements = document.querySelectorAll('.CodeMirror');
           if (cmElements.length > 0) {
               const cm = cmElements[0].CodeMirror;
               if (cm) {
                   code = cm.getValue();
               }
           }
       }

       return { title, code, length: code ? code.length : 0 };
   }
   ```

5. **Sanitize filename**: Convert shader title to a valid filename:
   - Lowercase
   - Replace non-alphanumeric with underscores
   - Remove leading/trailing underscores
   - Use second argument if provided by user

6. **Save files**: Save to `shaders/` directory:
   - `{name}_raw.glsl` - The raw ShaderToy GLSL code
   - `{name}.json` - JSON with metadata (title, id, code)

7. **Close browser**: Use `mcp__plugin_playwright_playwright__browser_close`

8. **Report success**: Tell user:
   - Shader name
   - Output filename
   - File locations
   - Next steps to convert to Vulkan format

## Example Output

```
✓ Fetched shader: Bumped Sinusoidal Warp
✓ Saved GLSL: shaders/bumped_sinusoidal_warp_raw.glsl (7687 chars)
✓ Saved JSON: shaders/bumped_sinusoidal_warp.json

Next steps:
1. Convert to Vulkan GLSL format (replace mainImage, add uniforms)
2. Compile: glslangValidator -V shaders/bumped_sinusoidal_warp.frag -o frag.spv
3. Test: ./switch_shader.sh bumped_sinusoidal_warp && ./run.sh
```

## Error Handling

- If CloudFlare blocks too aggressively, wait longer (up to 10 seconds)
- If CodeMirror not found, report error and suggest manual copy
- If page doesn't load, report the issue

## Notes

- This bypasses CloudFlare by using a real browser session
- Works for any public ShaderToy shader
- Does NOT sign in or access private shaders
- The fetched code still needs Vulkan conversion before use
