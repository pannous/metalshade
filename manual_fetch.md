# Manual Shader Fetch Instructions

Since browser automation isn't available yet, you can manually fetch the shader:

## Steps:

1. **Open in browser**: https://www.shadertoy.com/view/fsVBDm

2. **Copy the shader code** from the editor

3. **Save to file**:
   ```bash
   # Paste the code and press Ctrl+D
   cat > shaders/fsVBDm_raw.glsl
   ```

4. **Run import again**:
   ```bash
   ./import fsVBDm
   ```

## OR: Restart Chromium

If you prefer automated fetching:

1. **Close Chromium completely**
2. **Reopen Chromium**
3. **Verify extension is loaded**: Go to `chrome://extensions/`
4. **Come back** and I'll retry the browser automation

Which would you prefer?
