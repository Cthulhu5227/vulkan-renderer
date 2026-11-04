// Window, init order, main loop, cleanup.
#include "app.hpp"

// GLFW can't call a member function, so the window's user pointer leads back to App.
void App::framebufferResizeCallback(GLFWwindow* window, int, int) {
    auto app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void App::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // skip the default OpenGL context
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void App::initVulkan() {
    createInstance();
    setupDebugMessenger();

    createSurface();

    pickPhysicalDevices();
    createLogicalDevice();
    createAllocator();

    createSwapChain();
    createImageViews();
    createDepthResources();
    createDescriptorSetLayout();
    createGraphicsPipeline();

    createCommandPool();
    createCommandBuffers();

    createTextureImage();
    createTextureSampler();

    loadModel(MODEL_DIR "spot.obj");
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();

    createSyncObjects();
}

void App::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(device);// cleanup() can't destroy anything the GPU is still using
}

void App::cleanup() { //ORDER IS IMPORTANT: Destory in reverse
    cleanupSwapChain();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vmaDestroyBuffer(allocator, uniformBuffers[i], uniformBufferAllocations[i]);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);// also frees the desc sets
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);
    vmaDestroyImage(allocator, textureImage, textureImageAllocation);
    vmaDestroyBuffer(allocator, indexBuffer, indexBufferAllocation);
    vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);
    vkDestroyCommandPool(device, commandPool, nullptr);// also frees the command buf
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
    if (debugMessenger) {
        auto destroy = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
        destroy(instance, debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}
