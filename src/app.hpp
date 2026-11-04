// App's member functions are defined across the .cpp files, grouped by topic below.
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan's depth range is 0..1, not OpenGL's -1..1
#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

inline const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

inline const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

// Must match the uniform block in shader.vert.
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Must match the inputs in shader.vert.
struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    static VkVertexInputBindingDescription bindingDescription() {
        return { .binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
    }
    static std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions() {
        return {{
            { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos) },
            { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT,    .offset = offsetof(Vertex, texCoord) },
        }};
    }

    glm::vec3 normal;
    
};

class App {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;//owned and destroyed by the swap chain
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;// per frame in flight

    std::vector<VkSemaphore> imageAvailableSemaphores;//per frame in flight
    std::vector<VkFence> inFlightFences;//per frame in flight
    std::vector<VkSemaphore> renderFinishedSemaphores;//per swap-chain image
    uint32_t currentFrame = 0;//indexes the per-frame vectors

    bool framebufferResized = false;//set by GLFW's resize callback

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexBufferAllocation = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VmaAllocation indexBufferAllocation = VK_NULL_HANDLE;

    VkImage depthImage = VK_NULL_HANDLE;
    VmaAllocation depthImageAllocation = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    VkImage textureImage = VK_NULL_HANDLE;
    VmaAllocation textureImageAllocation = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;//frame in flight
    std::vector<VkBuffer> uniformBuffers; //per frame in flight
    std::vector<VmaAllocation> uniformBufferAllocations;//per frame in flight


//FUNCS PER FILE


    // app.cpp
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    // device.cpp
    static bool validationLayersAvailable();
    static VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo();
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevices();
    QueueFamilyIndices findQueueFamilies();
    void createLogicalDevice();

    // swapchain.cpp
    SwapChainSupportDetails querySwapChainSupport();
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps);
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderFinishedSemaphores();
    void cleanupSwapChain();
    void recreateSwapChain();

    // pipeline.cpp
    static std::vector<char> readFile(const std::string& path);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createGraphicsPipeline();

    // frame.cpp
    void createCommandPool();
    void createCommandBuffers();
    void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    void createSyncObjects();
    void drawFrame();

    // buffer.cpp
    void createAllocator();
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags,
                      VkBuffer& buffer, VmaAllocation& allocation);
    void createStagingBuffer(const void* src, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation);
    void createDeviceLocalBuffer(const void* src, VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkBuffer& buffer, VmaAllocation& allocation);
    void createVertexBuffer();
    void createIndexBuffer();
    VkCommandBuffer beginOneTimeCommands();
    void endOneTimeCommands(VkCommandBuffer cmd);

    // descriptors.cpp
    void createDescriptorSetLayout();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void updateUniformBuffer(uint32_t frame);

    // texture.cpp
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                     VkImage& image, VmaAllocation& allocation);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect);
    void createTextureImage();
    void createTextureSampler();

    // model.cpp
    void loadModel(const std::string& path);
};
