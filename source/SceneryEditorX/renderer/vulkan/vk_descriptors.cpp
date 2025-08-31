/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_descriptors.cpp
* -------------------------------------------------------
* Created: 14/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_descriptors.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    VkDescriptorSet CreateDescriptor(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout, VkSampler sampler, VkImageView image_view, VkImageLayout image_layout)
    {
        /// Create Descriptor Set:
        VkDescriptorSet descriptor_set;
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = descriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &layout;

        if (VkResult result = vkAllocateDescriptorSets(device, &alloc_info, &descriptor_set); result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to allocate descriptor set!");
            return VK_NULL_HANDLE;
        }

        /// Update the Descriptor Set:
        UpdateDescriptorSet(device, descriptor_set, sampler, image_view, image_layout);

        return descriptor_set;
    }

    /// -------------------------------------------------------

    void UpdateDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet, VkSampler sampler, VkImageView image_view, VkImageLayout image_layout)
    {
        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = sampler;
        desc_image[0].imageView = image_view;
        desc_image[0].imageLayout = image_layout;

        VkWriteDescriptorSet descriptor_write[1] = {};
        descriptor_write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write[0].dstSet = descriptorSet;
        descriptor_write[0].dstBinding = 0;
        descriptor_write[0].dstArrayElement = 0;
        descriptor_write[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_write[0].descriptorCount = 1;
        descriptor_write[0].pImageInfo = desc_image;

        vkUpdateDescriptorSets(device, 1, descriptor_write, 0, nullptr);
    }

    /// -------------------------------------------------------

    // Deprecated per-resource bindless update helpers removed (BindlessDescriptorManager handles updates now).

}

/// -------------------------------------------------------
