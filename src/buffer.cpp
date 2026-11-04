// GPU memory through VMA, buffers, and one-off command buffers.
#define VMA_IMPLEMENTATION//compiles VMA's code into this .cpp file only
#include "app.hpp"

#include <stdexcept>

void App::createAllocator() {
    VmaAllocatorCreateInfo allocatorInfo{
        .physicalDevice = physicalDevice,
        .device = device,
        .instance = instance,
        .vulkanApiVersion = VK_API_VERSION_1_3 };

    if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
        throw std::runtime_error("failed to create VMA allocator");
    }
}

//flags of 0 gets GPU-only DEVICE_LOCAL memory, the fastest for the GPU to read.
void App::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags,
                       VkBuffer& buffer, VmaAllocation& allocation) {
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE };

    VmaAllocationCreateInfo allocInfo{ .flags = flags, .usage = VMA_MEMORY_USAGE_AUTO };

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer");
    }
}

//temporary CPU-writable buffer holding src, to copy from into GPU-only memory.
void App::createStagingBuffer(const void* src, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation) {
    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, buffer, allocation);

    if (vmaCopyMemoryToAllocation(allocator, src, allocation, 0, size) != VK_SUCCESS) {
        throw std::runtime_error("failed to write staging buffer");
    }
}

//Uploads src into a new GPU-only buffer. The CPU can't write to that memory so the data has to arrive through a GPU copy.
void App::createDeviceLocalBuffer(const void* src, VkDeviceSize size, VkBufferUsageFlags usage,
                                  VkBuffer& buffer, VmaAllocation& allocation) {
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    createStagingBuffer(src, size, stagingBuffer, stagingAllocation);

    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, 0, buffer, allocation);

    VkCommandBuffer cmd = beginOneTimeCommands();
    VkBufferCopy region{ .size = size };
    vkCmdCopyBuffer(cmd, stagingBuffer, buffer, 1, &region);
    endOneTimeCommands(cmd);

    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

void App::createVertexBuffer() {
    createDeviceLocalBuffer(vertices.data(), sizeof(vertices[0]) * vertices.size(),
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferAllocation);
}

void App::createIndexBuffer() {
    createDeviceLocalBuffer(indices.data(), sizeof(indices[0]) * indices.size(),
                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferAllocation);
}

//Starts a throwaway command buffer for one-off GPU work at startup.
VkCommandBuffer App::beginOneTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1 };

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    vkBeginCommandBuffer(cmd, &beginInfo);

    return cmd;
}

//Submits the command buffer, waits for the GPU to finish it, and frees it.
void App::endOneTimeCommands(VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);

    VkCommandBufferSubmitInfo commandBufferInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cmd };
    VkSubmitInfo2 submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferInfo };

    vkQueueSubmit2(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);   // only used at startup, so blocking here is fine

    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}
