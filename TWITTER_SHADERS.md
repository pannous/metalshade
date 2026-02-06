# Converting Twitter / Code Golf Shaders

Twitter shaders (also called "code golf" shaders) are extremely compact GLSL fragments often shared on Twitter/X, Pouet.net, and demo scene communities. They use implicit variables and minified syntax.

## Common Extensions

These shaders typically use these extensions:
- `.raw` - Raw shader code (no wrapper)
- `.glsl` - GLSL fragment
- `.txt` - Plain text
- No extension - Just the code

## Conversion Tool

Use `convert_golf.py` to automatically convert:

```bash
./convert_golf.py shaders/demo.raw shaders/demo.frag
```

## Common Code Golf Variables

These ultra-compact shaders use single-letter variables:

| Variable | Meaning | Expansion |
|----------|---------|-----------|
| `FC` | fragCoord | `fragCoord` or `gl_FragCoord` |
| `r` | iResolution | `ubo.iResolution` |
| `t` | iTime | `ubo.iTime` |
| `o` | fragColor | Output color (vec4) |
| `m` | iMouse | `ubo.iMouse` |
| `p` | position | Usually normalized coords |
| `v` | vector | Temporary calculation |
| `i` | index | Loop counter |
| `f` | float | Loop/calculation variable |

## Example Conversion

**Original (Twitter format):**
```glsl
vec2 p=(FC.xy*2.-r)/r.y/.3,v;
for(float i,f;i++<1e1;o+=(cos(i+vec4(0,1,2,3))+1.)/6./length(v))
  for(v=p,f=0.;f++<9.;v+=sin(v.yx*f+i+t)/f);
o=tanh(o*o);
```

**Converted (Vulkan GLSL):**
```glsl
#version 450
layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;
// ... uniforms ...

void main() {
    vec3 r = ubo.iResolution;
    float t = ubo.iTime;
    vec4 o = vec4(0.0);

    vec2 p = (fragCoord.xy * 2.0 - r.xy) / r.y / 0.3;
    vec2 v;

    for (float i = 0.0, f; i < 10.0; i++) {
        for (v = p, f = 0.0; f < 9.0; f++) {
            v += sin(v.yx * f + i + t) / f;
        }
        o += (cos(i + vec4(0,1,2,3)) + 1.0) / 6.0 / length(v);
    }

    o = tanh(o * o);
    fragColor = o;
}
```

## Common Issues & Fixes

### 1. Missing Semicolons
Code golf often omits semicolons where possible.
```glsl
// Original: for(;;)o+=1
// Fixed:    for(;;) o += 1.0;
```

### 2. Unusual Operators
```glsl
i++<10    →    i < 10.0; i++
f=0.;f++  →    f = 0.0; f < ...; f++
```

### 3. Implicit Type Conversions
```glsl
1e1    →    10.0
9.     →    9.0
.3     →    0.3
```

### 4. Comma Operators in For Loops
```glsl
for(float i,f;  // Declares i and f
    i++<10;     // Condition
    o+=...)     // Multiple statements in "increment"
```

## Manual Cleanup Steps

After running `convert_golf.py`:

1. **Check syntax** - Run through glslangValidator
2. **Fix loops** - Rewrite unusual for-loop structures
3. **Add comments** - Document what the shader does
4. **Test** - Run in metalshade viewer
5. **Optimize** - Sometimes the minified version has bugs!

## Where to Find Code Golf Shaders

- **Twitter/X**: Search for `#shadertoy #codegolf`
- **Pouet.net**: Demo scene shader competitions
- **Shadertoy**: Filter by "< 280 chars" or "code golf"
- **Dwitter**: JavaScript/WebGL tiny demos (similar concept)

## Testing

```bash
# Convert
./convert_golf.py shaders/twitter_shader.raw shaders/twitter_shader.frag

# View (may need manual fixes first)
./metalshade shaders/twitter_shader

# If it fails to compile, check the .glsl file for errors
```

## Example Shaders

See `shaders/demo_fixed.frag` for a working example of a converted Twitter shader.

## Tips

1. **Start simple** - Test with well-formed golf shaders first
2. **Manual fixes** - Most Twitter shaders need some cleanup
3. **Understand the pattern** - Learn the common code golf idioms
4. **Ask the author** - Many shader artists are happy to explain!
5. **Save originals** - Keep `.raw` files as reference

## Advanced: Writing Your Own

To write compact shaders:

```glsl
// Verbose
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    fragColor = vec4(uv, 0.5, 1.0);
}

// Code golf
vec2 p=FC.xy/r.xy;o=vec4(p,.5,1);
```

But remember: **readability > brevity** for production shaders!
