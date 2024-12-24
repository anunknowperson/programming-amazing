#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace vkinit {
//> init_cmd
VkCommandPoolCreateInfo command_pool_create_info(
        uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool,
                                                         uint32_t count = 1);
//< init_cmd

VkCommandBufferBeginInfo command_buffer_begin_info(
        VkCommandBufferUsageFlags flags = 0);
VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);

VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

VkSubmitInfo2 submit_info(const VkCommandBufferSubmitInfo* cmd,
                          const VkSemaphoreSubmitInfo* signalSemaphoreInfo,
                          const VkSemaphoreSubmitInfo* waitSemaphoreInfo);
VkPresentInfoKHR present_info();

VkRenderingAttachmentInfo attachment_info(
        VkImageView view, const VkClearValue* clear,
        VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingAttachmentInfo depth_attachment_info(
        VkImageView view,
        VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingInfo rendering_info(
        VkExtent2D renderExtent,
        const VkRenderingAttachmentInfo* colorAttachment,
        const VkRenderingAttachmentInfo* depthAttachment);

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);

VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask,
                                            VkSemaphore semaphore);
VkDescriptorSetLayoutBinding descriptorset_layout_binding(
        VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
VkDescriptorSetLayoutCreateInfo descriptorset_layout_create_info(
        const VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount);
VkWriteDescriptorSet write_descriptor_image(
        VkDescriptorType type, VkDescriptorSet dstSet,
        const VkDescriptorImageInfo* imageInfo, uint32_t binding);
VkWriteDescriptorSet write_descriptor_buffer(
        VkDescriptorType type, VkDescriptorSet dstSet,
        const VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
VkDescriptorBufferInfo buffer_info(VkBuffer buffer, VkDeviceSize offset,
                                   VkDeviceSize range);

VkImageCreateInfo image_create_info(VkFormat format,
                                    VkImageUsageFlags usageFlags,
                                    VkExtent3D extent);
VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image,
                                            VkImageAspectFlags aspectFlags);
VkPipelineLayoutCreateInfo pipeline_layout_create_info();
VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(
        VkShaderStageFlagBits stage, VkShaderModule shaderModule,
        const char* entry = "main");
}  // namespace vkinit
