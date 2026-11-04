// GPU images, and loading the texture from a file.
#include "app.hpp"

#define STB_IMAGE_IMPLEMENTATION   // compiles stb_image's code into this .cpp file only
#include <stb/stb_image.h>

#include <stdexcept>

void App::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                      VkImage& image, VmaAllocation& allocation) {
    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { width, height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };

    VmaAllocationCreateInfo allocInfo{ .usage = VMA_MEMORY_USAGE_AUTO };

    if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image");
    }
}

VkImageView App::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = { aspect, 0, 1, 0, 1 } };

    VkImageView view;
    if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view");
    }
    return view;
}

void App::createTextureImage() {
    // STBI_rgb_alpha gives 4 bytes per pixel even if the file has no alpha channel.
    int width, height, channels;
    stbi_uc* pixels = stbi_load(TEXTURE_DIR "spot_texture.png", &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image");
    }

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    createStagingBuffer(pixels, static_cast<VkDeviceSize>(width) * height * 4, stagingBuffer, stagingAllocation);
    stbi_image_free(pixels);

    // SRGB because image files store sRGB colors; the GPU converts them when sampling.
    constexpr VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    createImage(width, height, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                textureImage, textureImageAllocation);

    VkCommandBuffer cmd = beginOneTimeCommands();

    transitionImage(cmd, textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region{
        .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 } };
    vkCmdCopyBufferToImage(cmd, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    transitionImage(cmd, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    endOneTimeCommands(cmd);

    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);

    textureImageView = createImageView(textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
}

void App::createTextureSampler() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,      // blend neighboring texels when enlarged
        .minFilter = VK_FILTER_LINEAR,      // and when shrunk
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,   // tile the image past 0..1
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,        // keeps it sharp on surfaces seen at steep angles
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy };

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler");
    }
}
