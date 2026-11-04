// The swap chain and everything rebuilt with it when the window changes size.
#include "app.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

SwapChainSupportDetails App::querySwapChainSupport() {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());

    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, nullptr);
    details.presentModes.resize(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, details.presentModes.data());

    return details;
}

// Prefers 8-bit BGRA in sRGB, so colors display correctly.
VkSurfaceFormatKHR App::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }
    return formats[0];
}

// Prefers MAILBOX (no tearing, low latency). The spec guarantees FIFO is always available.
VkPresentModeKHR App::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (auto m : modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

// The surface usually dictates the size. A width of UINT32_MAX means we choose,
// so use the window's size in pixels, within the allowed range.
VkExtent2D App::chooseExtent(const VkSurfaceCapabilitiesKHR& caps) {
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return caps.currentExtent;
    }
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    VkExtent2D extent = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
    extent.width  = std::clamp(extent.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
    extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return extent;
}

void App::createSwapChain() {
    auto details = querySwapChainSupport();
    auto surfaceFormat = chooseSurfaceFormat(details.formats);
    auto presentMode = choosePresentMode(details.presentModes);
    auto extent = chooseExtent(details.capabilities);

    // One above the minimum, so we never wait on the driver for a free image.
    // A maxImageCount of 0 means there's no maximum.
    uint32_t imageCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
        imageCount = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // CONCURRENT avoids ownership transfers when two families share the images.
    // The default, EXCLUSIVE, is faster when one family does both jobs.
    QueueFamilyIndices indices = findQueueFamilies();
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    createInfo.preTransform = details.capabilities.currentTransform;   // no extra rotation or flip
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;     // window isn't see-through
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;                                      // skip pixels hidden by other windows

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain");
    }
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
}

void App::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void App::createDepthResources() {
    createImage(swapChainExtent.width, swapChainExtent.height, DEPTH_FORMAT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImage, depthImageAllocation);
    depthImageView = createImageView(depthImage, DEPTH_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// One per swap-chain image, because the presentation engine consumes these: an image can't
// be acquired again until its last present is done, so its semaphore is safe to reuse.
// Rebuilt here because the image count can change with the swap chain.
void App::createRenderFinishedSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    renderFinishedSemaphores.resize(swapChainImages.size());
    for (auto& sem : renderFinishedSemaphores) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &sem) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects");
        }
    }
}

void App::cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(allocator, depthImage, depthImageAllocation);
    for (auto view : swapChainImageViews) {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    for (auto sem : renderFinishedSemaphores) {
        vkDestroySemaphore(device, sem, nullptr);
    }
}

// The pipeline isn't rebuilt: it depends only on the image format, and the
// viewport and scissor are set every frame.
void App::recreateSwapChain() {
    // A minimized window is 0x0, which isn't a valid swap chain size. Sleep until it's restored.
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
    }

    vkDeviceWaitIdle(device);   // the GPU may still be using the old images

    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createDepthResources();
    createRenderFinishedSemaphores();
}
