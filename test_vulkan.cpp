#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    std::cout << "GLFW initialized" << std::endl;
    std::cout << "Vulkan supported: " << (glfwVulkanSupported() ? "YES" : "NO") << std::endl;

    if (glfwVulkanSupported()) {
        uint32_t count;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        std::cout << "Required extensions (" << count << "):" << std::endl;
        for (uint32_t i = 0; i < count; i++) {
            std::cout << "  - " << extensions[i] << std::endl;
        }
    }

    glfwTerminate();
    return 0;
}
