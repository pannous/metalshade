#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <cstring>
#include <cmath>
#include <climits>  // For PATH_MAX
#include <unistd.h> // For getcwd
#include <dirent.h> // For directory scanning
#include <algorithm> // For std::sort

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const int WIDTH = 1280;
const int HEIGHT = 720;

struct UniformBufferObject {
    alignas(16) float iResolution[3];
    alignas(4) float iTime;
    alignas(16) float iMouse[4];
    alignas(8) float iScroll[2];  // Accumulated scroll offset (x, y)
    // Button states as individual floats (std140 array alignment is complex)
    alignas(4) float iButtonLeft;
    alignas(4) float iButtonRight;
    alignas(4) float iButtonMiddle;
    alignas(4) float iButton4;
    alignas(4) float iButton5;
};

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

class MetalshadeViewer {
public:
    void run(const std::string& initialShader = "") {
        loadShaderList(initialShader);

        // Compile the initial shader before starting Vulkan
        if (!currentShaderPath.empty() && !compileAndLoadShader(currentShaderPath)) {
            std::cerr << "\n✗ Shader compilation failed. Fix errors above and try again." << std::endl;
            exit(1);
        }

        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    size_t currentFrame = 0;
    const int MAX_FRAMES_IN_FLIGHT = 2;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* uniformBufferMapped;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    // Feedback buffers for persistent paint effects (ping-pong)
    VkImage feedbackImages[2];
    VkDeviceMemory feedbackImageMemories[2];
    VkImageView feedbackImageViews[2];
    VkFramebuffer feedbackFramebuffers[2];
    int currentFeedbackBuffer = 0;  // Ping-pong index

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::chrono::steady_clock::time_point startTime;
    bool isFullscreen = false;
    int windowedWidth = WIDTH;
    int windowedHeight = HEIGHT;
    int windowedPosX = 0;
    int windowedPosY = 0;

    // Mouse input state
    double mouseX = 0.0;
    double mouseY = 0.0;
    double mouseClickX = 0.0;
    double mouseClickY = 0.0;
    bool mouseLeftPressed = false;
    bool mouseRightPressed = false;
    bool mouseMiddlePressed = false;
    bool mouseButton4Pressed = false;
    bool mouseButton5Pressed = false;
    float buttonPressDuration[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float scrollX = 0.0f;
    float scrollY = 0.0f;

    // Shader browsing
    std::vector<std::string> shaderList;
    int currentShaderIndex = 0;
    std::string currentShaderPath;
    std::string currentTexturePath;
    bool hasGeometryShader = false;

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            MetalshadeViewer* viewer = static_cast<MetalshadeViewer*>(glfwGetWindowUserPointer(window));

            if (key == GLFW_KEY_ESCAPE) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            } else if (key == GLFW_KEY_F || key == GLFW_KEY_F11) {
                viewer->toggleFullscreen();
            } else if (key == GLFW_KEY_LEFT) {
                viewer->switchShader(-1);
            } else if (key == GLFW_KEY_RIGHT) {
                viewer->switchShader(1);
            } else if (key == GLFW_KEY_R) {
                // Reset scroll offset
                viewer->scrollX = 0.0f;
                viewer->scrollY = 0.0f;
                std::cout << "✓ Scroll reset" << std::endl;
            } else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) {
                // Zoom in with + or = key
                viewer->scrollY += 1.0f;
            } else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) {
                // Zoom out with - key
                viewer->scrollY -= 1.0f;
            }
        }
    }

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        MetalshadeViewer* viewer = static_cast<MetalshadeViewer*>(glfwGetWindowUserPointer(window));

        bool pressed = (action == GLFW_PRESS);

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            viewer->mouseLeftPressed = pressed;
            if (pressed) {
                viewer->mouseClickX = viewer->mouseX;
                viewer->mouseClickY = viewer->mouseY;
                viewer->buttonPressDuration[0] = 0.0f;
            }
            // Duration stays at final value on release, resets on next press
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            viewer->mouseRightPressed = pressed;
            if (pressed) viewer->buttonPressDuration[1] = 0.0f;
        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            viewer->mouseMiddlePressed = pressed;
            if (pressed) viewer->buttonPressDuration[2] = 0.0f;
        } else if (button == 3) {  // GLFW_MOUSE_BUTTON_4
            viewer->mouseButton4Pressed = pressed;
            if (pressed) viewer->buttonPressDuration[3] = 0.0f;
        } else if (button == 4) {  // GLFW_MOUSE_BUTTON_5
            viewer->mouseButton5Pressed = pressed;
            if (pressed) viewer->buttonPressDuration[4] = 0.0f;
        }
    }

    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        MetalshadeViewer* viewer = static_cast<MetalshadeViewer*>(glfwGetWindowUserPointer(window));
        viewer->mouseX = xpos;
        viewer->mouseY = ypos;
    }

    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        MetalshadeViewer* viewer = static_cast<MetalshadeViewer*>(glfwGetWindowUserPointer(window));
        // Accumulate scroll offset for shaders to use as they wish
        viewer->scrollX += static_cast<float>(xoffset);
        viewer->scrollY += static_cast<float>(yoffset);
    }

    void toggleFullscreen() {
        isFullscreen = !isFullscreen;

        if (isFullscreen) {
            // Save windowed position and size
            glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
            glfwGetWindowSize(window, &windowedWidth, &windowedHeight);

            // Get primary monitor and its video mode
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            // Switch to fullscreen
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            std::cout << "✓ Switched to fullscreen: " << mode->width << "x" << mode->height << std::endl;
        } else {
            // Switch back to windowed mode
            glfwSetWindowMonitor(window, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
            std::cout << "✓ Switched to windowed mode: " << windowedWidth << "x" << windowedHeight << std::endl;
        }
    }

    std::string resolveFragmentShader(const std::string& path) {
        // Handle missing extension or trailing dot
        std::string workingPath = path;

        // Remove trailing dot if present
        if (!workingPath.empty() && workingPath.back() == '.') {
            workingPath.pop_back();
        }

        // Check if file exists as-is
        if (fileExists(workingPath)) {
            return workingPath;
        }

        // Check if path has NO extension (no dot in filename)
        size_t lastSlash = workingPath.find_last_of("/");
        size_t lastDot = workingPath.find_last_of(".");
        bool hasExtension = (lastDot != std::string::npos &&
                           (lastSlash == std::string::npos || lastDot > lastSlash));

        if (!hasExtension) {
            // No extension provided, try adding common fragment shader extensions
            std::vector<std::string> fragExts = {".frag", ".fsh", ".glsl"};
            for (const auto& ext : fragExts) {
                std::string testPath = workingPath + ext;
                if (fileExists(testPath)) {
                    std::cout << "✓ Auto-detected extension: " << testPath << std::endl;
                    return testPath;
                }
            }
        }

        // If a vertex/geometry shader is provided, try to find corresponding fragment shader
        std::vector<std::string> vertGeomExts = {".vert", ".vsh", ".geom", ".gsh"};

        for (const auto& ext : vertGeomExts) {
            if (workingPath.size() >= ext.size() &&
                workingPath.substr(workingPath.size() - ext.size()) == ext) {
                // This is a vertex/geometry shader, find the fragment shader
                std::string baseName = getShaderBaseName(workingPath);
                std::string shaderDir = getShaderDirectory(workingPath);

                // Try common fragment shader extensions
                std::vector<std::string> fragExts = {".frag", ".fsh"};
                for (const auto& fragExt : fragExts) {
                    std::string fragPath = shaderDir + "/" + baseName + fragExt;
                    if (fileExists(fragPath)) {
                        std::cout << "✓ Detected vertex/geometry shader, using fragment shader: " << fragPath << std::endl;
                        return fragPath;
                    }
                }

                std::cerr << "⚠ Could not find corresponding fragment shader for: " << workingPath << std::endl;
                std::cerr << "  Tried: " << baseName << ".frag, " << baseName << ".fsh" << std::endl;
                return workingPath; // Fallback to original (will likely fail)
            }
        }

        return workingPath; // Return as-is (may fail if file doesn't exist)
    }

    void loadShaderList(const std::string& initialShader = "") {
        // If a specific shader was provided via command line, use it
        if (!initialShader.empty()) {
            currentShaderPath = resolveFragmentShader(initialShader);
            std::cout << "✓ Loading shader: " << currentShaderPath << std::endl;
        }
        else{
           currentShaderPath = "shaders/example.frag";
        }

        // Load shader list for browsing with arrow keys (optional)
        std::ifstream file("shader_list.txt");
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    shaderList.push_back(line);
                }
            }
            file.close();
            if (!shaderList.empty()) {
                std::cout << "✓ Loaded " << shaderList.size() << " shaders for browsing" << std::endl;
                std::cout << "  Use ← → to browse shaders" << std::endl;
            }
        }
    }

    void scanDirectoryForShaders(const std::string& directory) {
        shaderList.clear();

        DIR* dir = opendir(directory.c_str());
        if (!dir) {
            std::cout << "⚠ Could not scan directory: " << directory << std::endl;
            return;
        }

        struct dirent* entry;
        std::vector<std::string> shaderExts = {".frag", ".glsl", ".fsh", ".gsh", ".vsh"};

        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;

            // Check if file has a shader extension
            for (const auto& ext : shaderExts) {
                if (filename.size() >= ext.size() &&
                    filename.substr(filename.size() - ext.size()) == ext) {
                    std::string fullPath = directory + "/" + filename;
                    shaderList.push_back(fullPath);
                    break;
                }
            }
        }
        closedir(dir);

        // Sort for consistent ordering
        std::sort(shaderList.begin(), shaderList.end());

        // Find current shader index
        for (size_t i = 0; i < shaderList.size(); i++) {
            if (shaderList[i] == currentShaderPath) {
                currentShaderIndex = i;
                break;
            }
        }

        if (!shaderList.empty()) {
            std::cout << "✓ Found " << shaderList.size() << " shader(s) in directory" << std::endl;
        }
    }

    std::string getCompiledSpvPath(const std::string& shaderPath) {
        std::string baseName = getShaderBaseName(shaderPath);
        std::string shaderDir = getShaderDirectory(shaderPath);
        return shaderDir + "/" + baseName + ".frag.spv";
    }

    void switchShader(int delta) {
        if (shaderList.empty()) {
            // Try to scan current directory for shaders
            std::string shaderDir = getShaderDirectory(currentShaderPath);
            scanDirectoryForShaders(shaderDir);

            if (shaderList.empty()) {
                std::cout << "⚠ No shaders found in " << shaderDir << std::endl;
                return;
            }
        }

        // Get the current compiled shader path to avoid duplicates
        std::string currentSpvPath = getCompiledSpvPath(currentShaderPath);

        int attempts = 0;
        const int maxAttempts = shaderList.size();

        while (attempts < maxAttempts) {
            currentShaderIndex = (currentShaderIndex + delta + shaderList.size()) % shaderList.size();
            currentShaderPath = shaderList[currentShaderIndex];

            // Skip if this shader compiles to the same .spv file
            std::string candidateSpvPath = getCompiledSpvPath(currentShaderPath);
            if (candidateSpvPath == currentSpvPath) {
                attempts++;
                continue;
            }

            std::cout << "\n[" << (currentShaderIndex + 1) << "/" << shaderList.size() << "] "
                      << currentShaderPath << std::endl;

            // Recompile and reload shader
            if (compileAndLoadShader(currentShaderPath)) {
                // Wait for device to be idle before recreating pipeline
                vkDeviceWaitIdle(device);

                try {
                    recreatePipeline();
                    std::cout << "✓ Shader loaded" << std::endl;
                    return; // Success!
                } catch (const std::exception& e) {
                    std::cout << "✗ Pipeline error: " << e.what() << std::endl;
                }
            } else {
                std::cout << "✗ Compilation failed, trying next..." << std::endl;
            }

            // Try next shader
            attempts++;
        }

        std::cout << "✗ No working shaders found!" << std::endl;
    }

    std::string getShaderBaseName(const std::string& path) {
        size_t lastSlash = path.find_last_of("/\\");
        size_t lastDot = path.find_last_of(".");
        std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
        return (lastDot == std::string::npos) ? filename : filename.substr(0, lastDot - (lastSlash == std::string::npos ? 0 : lastSlash + 1));
    }

    std::string getShaderDirectory(const std::string& path) {
        size_t lastSlash = path.find_last_of("/\\");
        return (lastSlash == std::string::npos) ? "." : path.substr(0, lastSlash);
    }

    std::string parseTextureFromShader(const std::string& shaderPath) {
        std::ifstream file(shaderPath);
        if (!file.is_open()) {
            return "";
        }

        std::string shaderContent;
        std::string line;
        std::string shaderDir = getShaderDirectory(shaderPath);

        // Read entire file
        while (std::getline(file, line)) {
            shaderContent += line + "\n";
        }
        file.close();

        // 1. Check for ISF (Interactive Shader Format) JSON header
        if (shaderContent.substr(0, 3) == "/*{") {
            size_t jsonEnd = shaderContent.find("}*/");
            if (jsonEnd != std::string::npos) {
                std::string jsonStr = shaderContent.substr(3, jsonEnd - 3);

                // Simple JSON parsing for INPUTS with TYPE "image"
                size_t inputsPos = jsonStr.find("\"INPUTS\"");
                if (inputsPos != std::string::npos) {
                    size_t arrayStart = jsonStr.find("[", inputsPos);
                    size_t arrayEnd = jsonStr.find("]", arrayStart);

                    if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                        std::string inputsArray = jsonStr.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

                        // Look for TYPE: "image" entries
                        size_t typePos = inputsArray.find("\"TYPE\"");
                        while (typePos != std::string::npos) {
                            size_t valueStart = inputsArray.find("\"", typePos + 6);
                            size_t valueEnd = inputsArray.find("\"", valueStart + 1);

                            if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                                std::string typeValue = inputsArray.substr(valueStart + 1, valueEnd - valueStart - 1);

                                if (typeValue == "image") {
                                    // Found an image input, look for NAME
                                    size_t objStart = inputsArray.rfind("{", typePos);
                                    size_t namePos = inputsArray.find("\"NAME\"", objStart);

                                    if (namePos != std::string::npos && namePos < typePos) {
                                        size_t nameValueStart = inputsArray.find("\"", namePos + 6);
                                        size_t nameValueEnd = inputsArray.find("\"", nameValueStart + 1);

                                        if (nameValueStart != std::string::npos && nameValueEnd != std::string::npos) {
                                            std::string imageName = inputsArray.substr(nameValueStart + 1, nameValueEnd - nameValueStart - 1);

                                            // Try common image extensions
                                            for (const auto& ext : {".jpg", ".png", ".jpeg"}) {
                                                std::string texPath = shaderDir + "/" + imageName + ext;
                                                if (fileExists(texPath)) {
                                                    std::cout << "✓ ISF texture: " << imageName << ext << std::endl;
                                                    return texPath;
                                                }
                                            }
                                        }
                                    }
                                    break;
                                }
                            }

                            typePos = inputsArray.find("\"TYPE\"", typePos + 1);
                        }
                    }
                }
            }
        }

        // 2. Look for // @texture <path> directive
        std::istringstream iss(shaderContent);
        while (std::getline(iss, line)) {
            size_t pos = line.find("// @texture");
            if (pos != std::string::npos) {
                size_t start = line.find_first_not_of(" \t", pos + 11);
                if (start != std::string::npos) {
                    size_t end = line.find_last_not_of(" \t\r\n");
                    std::string texPath = line.substr(start, end - start + 1);

                    // If relative path, resolve relative to shader directory
                    if (texPath[0] != '/') {
                        return shaderDir + "/" + texPath;
                    }
                    return texPath;
                }
            }
        }

        // 3. Fallback: look for galaxy.jpg or galaxy.png in shader directory
        std::string defaultTex = shaderDir + "/galaxy.jpg";
        if (fileExists(defaultTex)) {
            return defaultTex;
        }
        defaultTex = shaderDir + "/galaxy.png";
        if (fileExists(defaultTex)) {
            return defaultTex;
        }

        return "";
    }

    bool isVulkanReadyShader(const std::string& path) {
        // Check if file has an extension that indicates it's already in Vulkan format
        const std::vector<std::string> vulkanExts = {".glsl", ".fsh", ".gsh", ".vsh"};
        for (const auto& ext : vulkanExts) {
            if (path.size() >= ext.size() &&
                path.substr(path.size() - ext.size()) == ext) {
                return true;
            }
        }
        return false;
    }

    bool fileExists(const std::string& path) {
        std::ifstream file(path);
        return file.good();
    }

    std::string findMatchingShader(const std::string& basePath, const std::string& shaderDir, const std::vector<std::string>& extensions) {
        for (const auto& ext : extensions) {
            std::string candidatePath = shaderDir + "/" + basePath + ext;
            if (fileExists(candidatePath)) {
                return candidatePath;
            }
        }
        return "";
    }

    std::string getAbsolutePath(const std::string& path) {
        // If already absolute, return as-is
        if (!path.empty() && path[0] == '/') {
            return path;
        }

        // Get current working directory and prepend to relative path
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            return std::string(cwd) + "/" + path;
        }

        return path; // Fallback to original
    }

    bool compileAndLoadShader(const std::string& fragPath) {
        // Convert to absolute path (works when working directory changes)
        std::string absFragPath = getAbsolutePath(fragPath);

        // Parse texture from shader
        currentTexturePath = parseTextureFromShader(absFragPath);
        if (!currentTexturePath.empty()) {
            std::cout << "✓ Texture: " << currentTexturePath << std::endl;
        }

        // Get shader base name and directory
        std::string baseName = getShaderBaseName(absFragPath);
        std::string shaderDir = getShaderDirectory(absFragPath);

        // Check if input is already in Vulkan-ready format (.glsl, .fsh, .gsh, .vsh)
        bool isVulkanReady = isVulkanReadyShader(fragPath);

        std::string tempFrag;
        if (isVulkanReady) {
            // Already in Vulkan format, use it directly
            tempFrag = absFragPath;
            std::cout << "✓ Using Vulkan shader: " << tempFrag << std::endl;
        } else {
            // Need to convert from .frag to .glsl
            tempFrag = shaderDir + "/" + baseName + ".glsl";

            // Check if file is already in Vulkan format (has #version 450)
            std::ifstream checkFile(absFragPath);
            std::string line;
            bool needsConversion = true;
            bool inBlockComment = false;

            // Read first 50 lines to find #version, skipping ISF/comment blocks
            for (int i = 0; i < 50 && checkFile.is_open() && std::getline(checkFile, line); i++) {
                // Skip ISF JSON header
                if (line.find("/*{") != std::string::npos) {
                    inBlockComment = true;
                }
                if (inBlockComment) {
                    if (line.find("}*/") != std::string::npos) {
                        inBlockComment = false;
                    }
                    continue;
                }

                // Check for #version 450
                if (line.find("#version 450") != std::string::npos) {
                    // Already in Vulkan format, just copy it
                    needsConversion = false;
                    std::string copyCmd = "cp \"" + absFragPath + "\" \"" + tempFrag + "\"";
                    if (system(copyCmd.c_str()) != 0) {
                        return false;
                    }
                    break;
                }
            }
            checkFile.close();

            if (needsConversion) {
                // Use absolute path to converter script (works regardless of working directory)
                std::string convertCmd = "python3 /opt/3d/metalshade/convert.py \"" + absFragPath + "\" \"" + tempFrag + "\"";
                int result = system(convertCmd.c_str());
                if (result != 0) {
                    std::cerr << "✗ Shader conversion failed for: " << fragPath << std::endl;
                    return false;
                }
            }
        }

        // Output .spv files in the same directory
        std::string outputFragSpv = shaderDir + "/" + baseName + ".frag.spv";
        std::string outputVertSpv = shaderDir + "/" + baseName + ".vert.spv";

        // Compile to SPIR-V using wrapper script that adds source line context
        std::string compileCmd = "/opt/3d/metalshade/glsl_compile.sh \"" + tempFrag + "\" \"" + outputFragSpv + "\"";
        int result = system(compileCmd.c_str());
        if (result != 0) {
            std::cerr << "✗ Shader compilation failed for: " << fragPath << std::endl;
            std::cerr << "  GLSL shader: " << tempFrag << std::endl;
            return false;
        }

        std::cout << "✓ Compiled: " << outputFragSpv << std::endl;

        // Look for matching vertex shader (.vsh, .vert)
        std::vector<std::string> vertExts = {".vsh", ".vert"};
        std::string vertShaderPath = findMatchingShader(baseName, shaderDir, vertExts);

        if (!vertShaderPath.empty()) {
            // Found matching vertex shader - compile it
            std::cout << "✓ Found vertex shader: " << vertShaderPath << std::endl;
            std::string compileVertCmd = "glslangValidator -S vert -V \"" + vertShaderPath + "\" -o \"" + outputVertSpv + "\" -I\"" + shaderDir + "\"";
            if (system(compileVertCmd.c_str()) != 0) {
                std::cerr << "✗ Vertex shader compilation failed" << std::endl;
                return false;
            }
            std::cout << "✓ Compiled vertex shader: " << outputVertSpv << std::endl;
        }

        // Look for matching geometry shader (.gsh, .geom)
        std::vector<std::string> geomExts = {".gsh", ".geom"};
        std::string geomShaderPath = findMatchingShader(baseName, shaderDir, geomExts);

        if (!geomShaderPath.empty()) {
            // Found matching geometry shader - compile it
            std::cout << "✓ Found geometry shader: " << geomShaderPath << std::endl;
            std::string outputGeomSpv = shaderDir + "/" + baseName + ".geom.spv";
            std::string compileGeomCmd = "glslangValidator -S geom -V \"" + geomShaderPath + "\" -o \"" + outputGeomSpv + "\" -I\"" + shaderDir + "\"";
            if (system(compileGeomCmd.c_str()) != 0) {
                std::cerr << "✗ Geometry shader compilation failed" << std::endl;
                return false;
            }
            std::cout << "✓ Compiled geometry shader: " << outputGeomSpv << std::endl;
            hasGeometryShader = true;
        } else {
            hasGeometryShader = false;
        }

        return true;
    }

    void recreatePipeline() {
        // Destroy old pipeline
        vkDestroyPipeline(device, graphicsPipeline, nullptr);

        // Recreate with new shader
        createGraphicsPipeline();
    }

    void initWindow() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW!");
        }

        // Note: Skip glfwVulkanSupported() check - we know MoltenVK is available
        // The check may fail due to library path issues even when Vulkan works

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Metalshade Viewer (Vulkan/MoltenVK)", nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window!");
        }

        // Set user pointer for callback access
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);
        glfwSetScrollCallback(window, scrollCallback);

        std::cout << "✓ GLFW window created" << std::endl;
        startTime = std::chrono::steady_clock::now();
    }

    void initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createFeedbackBuffers();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Metalshade Viewer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        extensions.push_back("VK_KHR_get_physical_device_properties2");

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = 0;
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance!");
        }
        std::cout << "✓ Vulkan instance created (MoltenVK)" << std::endl;
    }

    void createSurface() {
        VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface! Error code: " + std::to_string(result));
        }
        std::cout << "✓ Window surface created" << std::endl;
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        physicalDevice = devices[0];

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        std::cout << "✓ Using GPU: " << deviceProperties.deviceName << std::endl;
    }

    uint32_t findQueueFamily() {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable queue family!");
    }

    void createLogicalDevice() {
        uint32_t queueFamilyIndex = findQueueFamily();

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{};

        const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 1;
        createInfo.ppEnabledExtensionNames = deviceExtensions;
        createInfo.enabledLayerCount = 0;

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(device, queueFamilyIndex, 0, &graphicsQueue);
    }

    void createSwapchain() {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

        VkSurfaceFormatKHR surfaceFormat;
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
        surfaceFormat = formats[0];
        for (const auto& format : formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surfaceFormat = format;
                break;
            }
        }

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

        swapchainExtent = capabilities.currentExtent;
        if (swapchainExtent.width == 0xFFFFFFFF) {
            swapchainExtent.width = WIDTH;
            swapchainExtent.height = HEIGHT;
        }

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = swapchainExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain!");
        }

        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
        swapchainImageFormat = surfaceFormat.format;
    }

    void createImageViews() {
        swapchainImageViews.resize(swapchainImages.size());
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        if (hasGeometryShader) {
            uboLayoutBinding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
        }

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Feedback texture binding (iChannel1) for paint/persistent effects
        VkDescriptorSetLayoutBinding feedbackLayoutBinding{};
        feedbackLayoutBinding.binding = 2;
        feedbackLayoutBinding.descriptorCount = 1;
        feedbackLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        feedbackLayoutBinding.pImmutableSamplers = nullptr;
        feedbackLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::vector<VkDescriptorSetLayoutBinding> bindings = {uboLayoutBinding, samplerLayoutBinding, feedbackLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module!");
        }
        return shaderModule;
    }

    void createGraphicsPipeline() {
        // Determine which .spv files to use
        std::string vertSpvPath;
        std::string fragSpvPath;
        std::string geomSpvPath;

        // if (currentShaderPath.empty()
        std::string baseName = getShaderBaseName(currentShaderPath);
        std::string shaderDir = getShaderDirectory(currentShaderPath);
        vertSpvPath = shaderDir + "/" + baseName + ".vert.spv";
        fragSpvPath = shaderDir + "/" + baseName + ".frag.spv";
        geomSpvPath = shaderDir + "/" + baseName + ".geom.spv";

        // Use fallback vertex shader if custom one doesn't exist
        if (!fileExists(vertSpvPath)) {
            vertSpvPath = "/opt/3d/metalshade/shaders/example.vert.spv";
            std::cout << "✓ Using default vertex shader" << std::endl;
        }

        auto vertShaderCode = readFile(vertSpvPath);
        auto fragShaderCode = readFile(fragSpvPath);

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

        // Load geometry shader if it exists
        VkShaderModule geomShaderModule = VK_NULL_HANDLE;
        if (hasGeometryShader && fileExists(geomSpvPath)) {
            auto geomShaderCode = readFile(geomSpvPath);
            geomShaderModule = createShaderModule(geomShaderCode);

            VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
            geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
            geomShaderStageInfo.module = geomShaderModule;
            geomShaderStageInfo.pName = "main";

            // Insert geometry shader between vertex and fragment
            shaderStages.insert(shaderStages.begin() + 1, geomShaderStageInfo);
            std::cout << "✓ Using geometry shader in pipeline" << std::endl;
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapchainExtent.width;
        viewport.height = (float)swapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        if (geomShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, geomShaderModule, nullptr);
        }
    }

    void createFramebuffers() {
        swapchainFramebuffers.resize(swapchainImageViews.size());
        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            VkImageView attachments[] = {swapchainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapchainExtent.width;
            framebufferInfo.height = swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        uint32_t queueFamilyIndex = findQueueFamily();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndex;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type!");
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // For feedback buffers: transition directly to shader readable
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            // For feedback buffers: transition to color attachment for rendering
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // After rendering to feedback buffer, transition to shader readable
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            // Before rendering to feedback buffer, transition from shader read
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        } else {
            throw std::invalid_argument("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
    }

    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = nullptr;

        // Try to load texture from currentTexturePath
        if (!currentTexturePath.empty()) {
            pixels = stbi_load(currentTexturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        }

        // If no texture or loading failed, create a default procedural texture
        if (!pixels) {
            std::cout << "✓ Using procedural gradient texture" << std::endl;
            texWidth = 256;
            texHeight = 256;
            VkDeviceSize imageSize = texWidth * texHeight * 4;

            std::vector<uint8_t> proceduralPixels(imageSize);
            for (uint32_t y = 0; y < (uint32_t)texHeight; y++) {
                for (uint32_t x = 0; x < (uint32_t)texWidth; x++) {
                    uint32_t idx = (y * texWidth + x) * 4;
                    float fx = x / (float)texWidth;
                    float fy = y / (float)texHeight;
                    proceduralPixels[idx + 0] = (uint8_t)(fx * 255);
                    proceduralPixels[idx + 1] = (uint8_t)(fy * 255);
                    proceduralPixels[idx + 2] = (uint8_t)((fx + fy) * 128);
                    proceduralPixels[idx + 3] = 255;
                }
            }

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, proceduralPixels.data(), static_cast<size_t>(imageSize));
            vkUnmapMemory(device, stagingBufferMemory);

            createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

            transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
            transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
            return;
        }

        VkDeviceSize imageSize = texWidth * texHeight * 4;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        stbi_image_free(pixels);
    }

    void createTextureImageView() {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture image view!");
        }
    }

    void createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    void createFeedbackBuffers() {
        // Create 2 feedback buffers for ping-pong rendering
        for (int i = 0; i < 2; i++) {
            // Create image for feedback buffer
            createImage(swapchainExtent.width, swapchainExtent.height,
                       VK_FORMAT_R8G8B8A8_UNORM,  // Use UNORM for render target
                       VK_IMAGE_TILING_OPTIMAL,
                       VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       feedbackImages[i], feedbackImageMemories[i]);

            // Create image view
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = feedbackImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &feedbackImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create feedback image view!");
            }

            // Transition to shader read layout initially (starts as "previous frame")
            transitionImageLayout(feedbackImages[i], VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Create framebuffer for rendering to this feedback buffer
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &feedbackImageViews[i];
            framebufferInfo.width = swapchainExtent.width;
            framebufferInfo.height = swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &feedbackFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create feedback framebuffer!");
            }
        }

        std::cout << "✓ Created ping-pong feedback buffers for paint effects" << std::endl;
    }

    void createUniformBuffer() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffer, uniformBufferMemory);
        vkMapMemory(device, uniformBufferMemory, 0, bufferSize, 0, &uniformBufferMapped);
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        // 2 samplers per frame: iChannel0 (static texture) + iChannel1 (feedback)
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            // Feedback texture info (iChannel1) - TODO: Make this dynamic for ping-pong
            VkDescriptorImageInfo feedbackInfo{};
            feedbackInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            feedbackInfo.imageView = feedbackImageViews[0];  // For now, always bind buffer 0
            feedbackInfo.sampler = textureSampler;

            std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pImageInfo = &feedbackInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    void updateFeedbackDescriptor() {
        // Update descriptor to point to the "read" feedback buffer
        int readBuffer = 1 - currentFeedbackBuffer;  // Read from the other buffer

        VkDescriptorImageInfo feedbackInfo{};
        feedbackInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        feedbackInfo.imageView = feedbackImageViews[readBuffer];
        feedbackInfo.sampler = textureSampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[currentFrame];
        descriptorWrite.dstBinding = 2;  // iChannel1
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &feedbackInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        int writeBuffer = currentFeedbackBuffer;
        int readBuffer = 1 - currentFeedbackBuffer;

        // === STEP 1: Transition feedback write buffer to COLOR_ATTACHMENT ===
        VkImageMemoryBarrier barrier1{};
        barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier1.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier1.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier1.image = feedbackImages[writeBuffer];
        barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier1.subresourceRange.baseMipLevel = 0;
        barrier1.subresourceRange.levelCount = 1;
        barrier1.subresourceRange.baseArrayLayer = 0;
        barrier1.subresourceRange.layerCount = 1;
        barrier1.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier1);

        // === STEP 2: Render to feedback buffer ===
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = feedbackFramebuffers[writeBuffer];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        // === STEP 3: Transition feedback buffer back to SHADER_READ ===
        VkImageMemoryBarrier barrier2{};
        barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier2.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier2.image = feedbackImages[writeBuffer];
        barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier2.subresourceRange.baseMipLevel = 0;
        barrier2.subresourceRange.levelCount = 1;
        barrier2.subresourceRange.baseArrayLayer = 0;
        barrier2.subresourceRange.layerCount = 1;
        barrier2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier2);

        // === STEP 4: Transition feedback buffer to TRANSFER_SRC for blit ===
        VkImageMemoryBarrier barrier3{};
        barrier3.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier3.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier3.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier3.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier3.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier3.image = feedbackImages[writeBuffer];
        barrier3.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier3.subresourceRange.baseMipLevel = 0;
        barrier3.subresourceRange.levelCount = 1;
        barrier3.subresourceRange.baseArrayLayer = 0;
        barrier3.subresourceRange.layerCount = 1;
        barrier3.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier3.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        // Transition swapchain image to TRANSFER_DST
        VkImageMemoryBarrier barrier4{};
        barrier4.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier4.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier4.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier4.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier4.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier4.image = swapchainImages[imageIndex];
        barrier4.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier4.subresourceRange.baseMipLevel = 0;
        barrier4.subresourceRange.levelCount = 1;
        barrier4.subresourceRange.baseArrayLayer = 0;
        barrier4.subresourceRange.layerCount = 1;
        barrier4.srcAccessMask = 0;
        barrier4.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        VkImageMemoryBarrier barriers[] = {barrier3, barrier4};
        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 2, barriers);

        // === STEP 5: Blit feedback buffer to swapchain ===
        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {(int32_t)swapchainExtent.width, (int32_t)swapchainExtent.height, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {(int32_t)swapchainExtent.width, (int32_t)swapchainExtent.height, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = 0;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            feedbackImages[writeBuffer], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_NEAREST);

        // === STEP 6: Transition swapchain to PRESENT ===
        VkImageMemoryBarrier barrier5{};
        barrier5.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier5.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier5.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier5.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier5.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier5.image = swapchainImages[imageIndex];
        barrier5.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier5.subresourceRange.baseMipLevel = 0;
        barrier5.subresourceRange.levelCount = 1;
        barrier5.subresourceRange.baseArrayLayer = 0;
        barrier5.subresourceRange.layerCount = 1;
        barrier5.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier5.dstAccessMask = 0;

        // Transition feedback buffer back to SHADER_READ for next frame
        VkImageMemoryBarrier barrier6{};
        barrier6.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier6.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier6.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier6.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier6.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier6.image = feedbackImages[writeBuffer];
        barrier6.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier6.subresourceRange.baseMipLevel = 0;
        barrier6.subresourceRange.levelCount = 1;
        barrier6.subresourceRange.baseArrayLayer = 0;
        barrier6.subresourceRange.layerCount = 1;
        barrier6.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier6.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkImageMemoryBarrier finalBarriers[] = {barrier5, barrier6};
        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, 0, nullptr, 0, nullptr, 2, finalBarriers);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create synchronization objects!");
            }
        }
    }

    void updateUniformBuffer() {
        auto currentTime = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        static float lastTime = 0.0f;
        float deltaTime = time - lastTime;
        lastTime = time;

        UniformBufferObject ubo{};
        ubo.iResolution[0] = static_cast<float>(swapchainExtent.width);
        ubo.iResolution[1] = static_cast<float>(swapchainExtent.height);
        ubo.iResolution[2] = 1.0f;
        ubo.iTime = time;

        // Get window size to calculate framebuffer scale (for Retina displays)
        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        float scaleX = static_cast<float>(swapchainExtent.width) / static_cast<float>(windowWidth);
        float scaleY = static_cast<float>(swapchainExtent.height) / static_cast<float>(windowHeight);

        // ShaderToy mouse convention:
        // xy = current mouse position (or click position)
        // zw = click position (negative if mouse button is up)
        // Scale mouse coordinates to match framebuffer coordinates
        float scaledMouseX = static_cast<float>(mouseX) * scaleX;
        float scaledMouseY = static_cast<float>(mouseY) * scaleY;
        float scaledClickX = static_cast<float>(mouseClickX) * scaleX;
        float scaledClickY = static_cast<float>(mouseClickY) * scaleY;

        if (mouseLeftPressed) {
            ubo.iMouse[0] = scaledMouseX;
            ubo.iMouse[1] = scaledMouseY;
            ubo.iMouse[2] = scaledClickX;
            ubo.iMouse[3] = scaledClickY;
        } else {
            ubo.iMouse[0] = scaledMouseX;
            ubo.iMouse[1] = scaledMouseY;
            ubo.iMouse[2] = -scaledClickX;
            ubo.iMouse[3] = -scaledClickY;
        }

        // Pass scroll offset for shaders to use as they wish
        ubo.iScroll[0] = scrollX;
        ubo.iScroll[1] = scrollY;

        // Update button press durations (accumulate while pressed, keep value after release)
        if (mouseLeftPressed) buttonPressDuration[0] += deltaTime;
        if (mouseRightPressed) buttonPressDuration[1] += deltaTime;
        if (mouseMiddlePressed) buttonPressDuration[2] += deltaTime;
        if (mouseButton4Pressed) buttonPressDuration[3] += deltaTime;
        if (mouseButton5Pressed) buttonPressDuration[4] += deltaTime;

        // Pass button press durations - stays at final value after release until next press
        ubo.iButtonLeft = buttonPressDuration[0];
        ubo.iButtonRight = buttonPressDuration[1];
        ubo.iButtonMiddle = buttonPressDuration[2];
        ubo.iButton4 = buttonPressDuration[3];
        ubo.iButton5 = buttonPressDuration[4];

        memcpy(uniformBufferMapped, &ubo, sizeof(ubo));
    }

    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                                                 imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }

        updateUniformBuffer();
        updateFeedbackDescriptor();  // Update which feedback buffer to read from

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = {swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(graphicsQueue, &presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        currentFeedbackBuffer = 1 - currentFeedbackBuffer;  // Swap ping-pong buffers
    }

    void mainLoop() {
        std::cout << "✓ Running Metalshade shader via Vulkan → MoltenVK → Metal" << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "  Left/Right mouse - Interactive effects" << std::endl;
        std::cout << "  Scroll wheel - Shader-specific (typically zoom)" << std::endl;
        std::cout << "  R - Reset scroll offset" << std::endl;
        std::cout << "  ← → - Switch shaders" << std::endl;
        std::cout << "  F or F11 - Toggle fullscreen" << std::endl;
        std::cout << "  ESC - Exit" << std::endl;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(device);
    }

    void cleanup() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

        for (auto framebuffer : swapchainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto imageView : swapchainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);

        vkDestroyBuffer(device, uniformBuffer, nullptr);
        vkFreeMemory(device, uniformBufferMemory, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        // Cleanup feedback buffers
        for (int i = 0; i < 2; i++) {
            vkDestroyFramebuffer(device, feedbackFramebuffers[i], nullptr);
            vkDestroyImageView(device, feedbackImageViews[i], nullptr);
            vkDestroyImage(device, feedbackImages[i], nullptr);
            vkFreeMemory(device, feedbackImageMemories[i], nullptr);
        }

        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main(int argc, char* argv[]) {
    MetalshadeViewer app;
    try {
        // Check if a shader path was provided as command-line argument
        if (argc > 1) {
            app.run(argv[1]);
        } else {
            app.run();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
