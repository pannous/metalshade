CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall
VULKAN_FLAGS = $(shell pkg-config --cflags vulkan glfw3)
VULKAN_LIBS = $(shell pkg-config --libs vulkan glfw3)
LDFLAGS = -framework Cocoa -framework IOKit -framework CoreVideo

# MoltenVK configuration
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json

TARGET = shadertoy_viewer
SRCS = shadertoy_viewer.cpp
SHADERS = vert.spv frag.spv

all: $(TARGET)

$(TARGET): $(SRCS) $(SHADERS)
	$(CXX) $(CXXFLAGS) $(VULKAN_FLAGS) $(SRCS) -o $(TARGET) $(VULKAN_LIBS) $(LDFLAGS)
	@echo "✓ Built ShaderToy Viewer with Vulkan+MoltenVK support"

%.spv: %.vert
	glslangValidator -V $< -o $@

%.spv: %.frag
	glslangValidator -V $< -o $@

clean:
	rm -f $(TARGET) *.spv

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
