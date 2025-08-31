/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_descriptors.h
* -------------------------------------------------------
* Created: 14/5/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

#include <vector>
#include <vulkan/vulkan.h>

namespace SceneryEditorX
{
    // Legacy descriptor metadata structures removed. Only minimal helper API retained.

    /**
     * @brief Creates a descriptor set with a combined image sampler
     *
     * @param device The logical device to use for descriptor creation
     * @param descriptorPool The descriptor pool to allocate from.
     * @param layout The descriptor set layout to use.
     * @param sampler The sampler to use.
     * @param image_view The image view to use.
     * @param image_layout The layout of the image.
     * @return VkDescriptorSet The created descriptor set.
     */
    VkDescriptorSet CreateDescriptor(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout, VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);

    /// -------------------------------------------------------

    /**
     * @brief Updates a descriptor set with new image information
     *
     * @param device The logical device
     * @param descriptorSet The descriptor set to update
     * @param sampler The sampler to use
     * @param image_view The image view to use
     * @param image_layout The layout of the image
     */
    void UpdateDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet, VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);

}

/// -------------------------------------------------------
