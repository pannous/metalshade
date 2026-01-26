use anyhow::{Context, Result};
use naga::back::spv;
use naga::front::glsl;
use naga::valid::{Capabilities, ValidationFlags, Validator};
use regex::Regex;
use std::env;
use std::fs;
use std::path::PathBuf;
use std::process::Command;

const VULKAN_HEADER: &str = r#"#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;
// layout(binding = 2) uniform sampler2D iChannel1;
// layout(binding = 3) uniform sampler2D iChannel2;
// layout(binding = 4) uniform sampler2D iChannel3;

"#;

fn convert_shadertoy_to_vulkan(code: &str) -> String {
    let mut code = code.to_string();

    // Remove ShaderToy-specific comments and URLs
    let re = Regex::new(r"// http://www\.pouet\.net.*\n").unwrap();
    code = re.replace_all(&code, "").to_string();

    // Remove common ShaderToy defines
    let re = Regex::new(r"#define\s+t\s+iTime").unwrap();
    code = re.replace_all(&code, "").to_string();
    let re = Regex::new(r"#define\s+r\s+iResolution\.xy").unwrap();
    code = re.replace_all(&code, "").to_string();

    // Replace uniform references with ubo. prefix
    // (ShaderToy code shouldn't have ubo. prefix already)
    let re = Regex::new(r"\biTime\b").unwrap();
    code = re.replace_all(&code, "ubo.iTime").to_string();
    let re = Regex::new(r"\biResolution\b").unwrap();
    code = re.replace_all(&code, "ubo.iResolution").to_string();
    let re = Regex::new(r"\biMouse\b").unwrap();
    code = re.replace_all(&code, "ubo.iMouse").to_string();

    // Replace other common uniforms
    let re = Regex::new(r"\biTimeDelta\b").unwrap();
    code = re.replace_all(&code, "0.016").to_string();
    let re = Regex::new(r"\biFrame\b").unwrap();
    code = re.replace_all(&code, "int(ubo.iTime * 60.0)").to_string();
    let re = Regex::new(r"\biFrameRate\b").unwrap();
    code = re.replace_all(&code, "60.0").to_string();
    let re = Regex::new(r"\biDate\b").unwrap();
    code = re.replace_all(&code, "vec4(2024.0, 1.0, 1.0, 0.0)").to_string();
    let re = Regex::new(r"\biSampleRate\b").unwrap();
    code = re.replace_all(&code, "44100.0").to_string();

    // Replace mainImage signature with main
    let re = Regex::new(r"void\s+mainImage\s*\(\s*out\s+vec4\s+\w+\s*,\s*in\s+vec2\s+\w+\s*\)").unwrap();
    code = re.replace_all(&code, "void main()").to_string();

    // Expand common #define shortcuts used in code-golfed shaders
    // Replace 't' when it's standalone (common iTime shortcut)
    // Note: \b already ensures word boundaries, but may match 'return', 'true', etc.
    // We accept this limitation - users should use #define instead of relying on single-letter vars
    let re = Regex::new(r"\bt\b").unwrap();
    code = re.replace_all(&code, "ubo.iTime").to_string();

    // Replace 'r' when used as iResolution.xy shortcut
    let re = Regex::new(r"\br\b").unwrap();
    code = re.replace_all(&code, "ubo.iResolution.xy").to_string();

    // Clean up multiple blank lines
    let re = Regex::new(r"\n\n\n+").unwrap();
    code = re.replace_all(&code, "\n\n").to_string();

    // Combine header with converted code
    format!("{}{}\n", VULKAN_HEADER, code.trim())
}

fn compile_with_glslang(frag_path: &PathBuf, spv_path: &PathBuf) -> Result<()> {
    let output = Command::new("glslangValidator")
        .arg("-V")
        .arg(frag_path)
        .arg("-o")
        .arg(spv_path)
        .output()
        .context("Failed to execute glslangValidator (is it installed?)")?;

    if !output.status.success() {
        let stderr = String::from_utf8_lossy(&output.stderr);
        anyhow::bail!("glslangValidator failed:\n{}", stderr);
    }

    Ok(())
}

fn compile_glsl_to_spirv(glsl_code: &str) -> Result<Vec<u32>> {
    // Parse GLSL
    let mut parser = glsl::Frontend::default();
    let options = glsl::Options {
        stage: naga::ShaderStage::Fragment,
        defines: Default::default(),
    };

    let module = parser
        .parse(&options, glsl_code)
        .context("Failed to parse GLSL shader")?;

    // Validate the module
    let mut validator = Validator::new(ValidationFlags::all(), Capabilities::all());
    let module_info = validator
        .validate(&module)
        .context("Shader validation failed")?;

    // Generate SPIR-V
    let spv_options = spv::Options::default();

    let spv = spv::write_vec(&module, &module_info, &spv_options, None)
        .context("Failed to generate SPIR-V")?;

    Ok(spv)
}

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        println!("ShaderToy to Vulkan SPIR-V Converter (Rust + naga)");
        println!("==================================================");
        println!();
        println!("Usage: {} <input_file> [output_file]", args[0]);
        println!();
        println!("Examples:");
        println!("  {} shaders/creation_by_silexars_raw.glsl", args[0]);
        println!(
            "  {} shaders/bumped_sinusoidal_warp_raw.glsl shaders/bumped.frag",
            args[0]
        );
        println!();
        println!("Converts ShaderToy GLSL to Vulkan GLSL + SPIR-V with:");
        println!("  - Vulkan #version 450 header");
        println!("  - Uniform buffer object declarations");
        println!("  - main() instead of mainImage()");
        println!("  - ubo.iTime, ubo.iResolution, ubo.iMouse prefixes");
        println!("  - SPIR-V compilation via naga");
        std::process::exit(1);
    }

    let input_path = PathBuf::from(&args[1]);

    if !input_path.exists() {
        anyhow::bail!("Input file not found: {}", input_path.display());
    }

    // Determine output filename
    let output_path = if args.len() > 2 {
        PathBuf::from(&args[2])
    } else {
        // Auto-generate: remove _raw suffix, change to .frag
        let stem = input_path
            .file_stem()
            .unwrap()
            .to_str()
            .unwrap()
            .replace("_raw", "");
        input_path.with_file_name(format!("{}.frag", stem))
    };

    let spv_path = output_path.with_extension("frag.spv");

    println!("Converting: {}", input_path.display());
    println!("Output:     {}", output_path.display());
    println!("SPIR-V:     {}", spv_path.display());

    // Read input
    let shadertoy_code = fs::read_to_string(&input_path)
        .context("Failed to read input file")?;

    // Convert to Vulkan GLSL
    let vulkan_code = convert_shadertoy_to_vulkan(&shadertoy_code);

    // Write Vulkan GLSL
    fs::write(&output_path, &vulkan_code)
        .context("Failed to write output file")?;
    println!("✓ Converted to Vulkan GLSL: {}", output_path.display());

    // Compile to SPIR-V (try naga first, fallback to glslangValidator)
    match compile_glsl_to_spirv(&vulkan_code) {
        Ok(spirv) => {
            // Convert u32 words to bytes
            let spirv_bytes: Vec<u8> = spirv
                .iter()
                .flat_map(|word| word.to_le_bytes())
                .collect();

            fs::write(&spv_path, &spirv_bytes)
                .context("Failed to write SPIR-V file")?;
            println!("✓ Compiled to SPIR-V (via naga): {}", spv_path.display());
        }
        Err(naga_err) => {
            println!("⚠ naga compilation failed, trying glslangValidator...");
            match compile_with_glslang(&output_path, &spv_path) {
                Ok(()) => {
                    println!("✓ Compiled to SPIR-V (via glslangValidator): {}", spv_path.display());
                }
                Err(glslang_err) => {
                    eprintln!("❌ Both naga and glslangValidator failed!");
                    eprintln!();
                    eprintln!("naga error: {}", naga_err);
                    eprintln!("glslangValidator error: {}", glslang_err);
                    eprintln!();
                    eprintln!("Note: The Vulkan GLSL file was still created at: {}", output_path.display());
                    std::process::exit(1);
                }
            }
        }
    }

    println!();
    println!("✓ Conversion complete!");
    println!();
    println!("Next steps:");
    println!("  1. Review: cat {}", output_path.display());
    println!("  2. Test: cp {} shadertoy.frag && make && ./run.sh", output_path.display());

    Ok(())
}
