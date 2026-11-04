// Uniform buffers and the descriptors that connect them to shaders.
#include "app.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <stdexcept>

void App::createDescriptorSetLayout() {
    //Binding numbers in the shaders.
    std::array<VkDescriptorSetLayoutBinding, 2> bindings{{
        { .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
        { .binding = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
    }};

    VkDescriptorSetLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data() };

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

//One per frame in flight, so the CPU never writes a buffer the GPU is still reading.
//MAPPED keeps them mapped, so the copy in updateUniformBuffer is cheap.
void App::createUniformBuffers() {
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                     uniformBuffers[i], uniformBufferAllocations[i]);
    }
}

void App::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{{
        { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = MAX_FRAMES_IN_FLIGHT },
        { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = MAX_FRAMES_IN_FLIGHT },
    }};

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data() };

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool");
    }
}

void App::createDescriptorSets() {

    //Allocating takes one layout per set, even when they're all the same.
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = layouts.data() };

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    //Each set gets its frame's uniform buffer. The texture never changes, so all sets can share the same image info.
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject) };

        VkDescriptorImageInfo imageInfo{
            .sampler = textureSampler,
            .imageView = textureImageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

        std::array<VkWriteDescriptorSet, 2> writes{{
            { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .dstSet = descriptorSets[i],
              .dstBinding = 0,
              .descriptorCount = 1,
              .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
              .pBufferInfo = &bufferInfo },
            { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .dstSet = descriptorSets[i],
              .dstBinding = 1,
              .descriptorCount = 1,
              .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              .pImageInfo = &imageInfo },
        }};

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

//Must run after this frame's fence wait, which guarantees the GPU has finished with the buffer.
void App::updateUniformBuffer(uint32_t frame) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(now - startTime).count();

    UniformBufferObject ubo{};

    //Applied right to left:
    //center Spot on its bounding box, stand it upright (it's modeled Y-up, the scene is Z-up), then spin it at 90 degrees per second.
    const glm::mat4 identity(1.0f);
    const glm::vec3 zAxis(0.0f, 0.0f, 1.0f);
    ubo.model = glm::rotate(identity, time * glm::radians(90.0f), zAxis)
              * glm::rotate(identity, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))
              * glm::translate(identity, glm::vec3(0.0f, -0.108f, -0.19f));

    ubo.view = glm::lookAt(glm::vec3(2.0f), glm::vec3(0.0f), zAxis);

    float aspect = swapChainExtent.width / static_cast<float>(swapChainExtent.height);
    ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;   // GLM assumes OpenGL's Y-up clip space; Vulkan's Y points down

    if (vmaCopyMemoryToAllocation(allocator, &ubo, uniformBufferAllocations[frame], 0, sizeof(ubo)) != VK_SUCCESS) {
        throw std::runtime_error("failed to update uniform buffer");
    }
}