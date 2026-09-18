# metalshade

The metalshade app automatically loads, converts and displays different shader formats:
  1. Consumes ShaderToy-style shaders and those in Vulkan GLSL format (#version 450)                                      
  2. The default shader file is shaders/example.frag which gets compiled to SPIR-V and run                                          
  3. The Book of Shaders examples use old-style GLSL with u_resolution and u_time via convert.py
  4. The app converts .frag to the Vulkan format with ubo.iResolution and ubo.iTime  
  5. Compile to Vulkan glsl_compile.sh
  6. NO need To manually compile shaders 
  
Add new shaders in the ./shaders/ folder 

example interactive mouse: shaders/button_duration_demo.frag

# build
Makefile
metalshade.cpp
metalshade


# Switch shader
Use the arrow keys in the app to iterate over all shaders registered in 
shader_list.txt

They can be registered either in relative or absolute paths:
shaders/clouds_bookofshaders.frag
/opt/3d/metalshade/shaders/plasma.frag
thebookofshaders/03/space.frag

Press F for full screen and ESC to escape. 

# Sublime plugin
/Users/me/Library/Application Support/Sublime Text 3/Packages/User/metalshade.sublime-build
and in our general build configuration:
/Users/me/Library/Application Support/Sublime Text 3/Packages/Build.sublime-build
case "$extension" in
"frag" | "glsl" | "fsh" | "gsh" | "vsh" | "vs" | "vert" | "geom" | "tesc" | "tese" | "comp" )
  export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
  export DYLD_LIBRARY_PATH=/opt/homebrew/lib:$DYLD_LIBRARY_PATH
  export MVK_CONFIG_LOG_LEVEL=1
  /opt/3d/metalshade/metalshade "$file"
  ;;
