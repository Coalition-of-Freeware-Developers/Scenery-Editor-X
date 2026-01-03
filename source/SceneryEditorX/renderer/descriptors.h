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
#include "SceneryEditorX/renderer/enums.h"
#include "SceneryEditorX/renderer/image_data.h"
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class Descriptors
	 *
	 * @brief A descriptor class that represents a Vulkan descriptor binding configuration.
	 *
	 * This class encapsulates all the necessary information for creating and managing
	 * Vulkan descriptors, including both static properties (reflected from shaders)
	 * and dynamic properties (set by the renderer at runtime).
	 *
	 * The class organizes properties into three categories:
	 * - Static properties that affect descriptor hash (slot, stage)
	 * - Dynamic properties that affect descriptor set hash (mip, mipRange, data)
	 * - Properties that don't affect hashing (type, layout, range, etc.)
	 *
	 * @note The constructor takes parameters to initialize core descriptor properties,
	 *       while other properties can be set individually as needed.
	 *
	 * @param name The name identifier for the descriptor (for debugging purposes)
	 * @param type The type of descriptor (texture, buffer, etc.)
	 * @param layout The image layout for texture descriptors
	 * @param slot The binding slot in the descriptor set
	 * @param stage The shader stage flags where this descriptor is used
	 * @param structSize The size of the structure for buffer descriptors
	 * @param asArray Whether this descriptor represents an array
	 * @param arrayLength The length of the array if asArray is true
	 */
	class Descriptors
	{
	public:
		Descriptors() = default;
		Descriptors(const std::string& name, const DescriptorType type, const Layout::ImageLayout layout, const uint32_t slot, const uint32_t stage, const uint32_t structSize, const bool asArray, const uint32_t arrayLength)
		{
			this->name        = name;
			this->type        = type;
			this->layout      = layout;
			this->slot        = slot;
			this->stage       = stage;
			this->structSize  = structSize;
			this->asArray     = asArray;
			this->arrayLength = arrayLength;
		}

		~Descriptors() = default;

        bool IsStorage() const { return type == DescriptorType::TextureStorage; }

        // Properties that affect the descriptor hash (static - reflected)
        uint32_t slot = 0;
        uint32_t stage = 0;

        // Properties that affect the descriptor set hash (dynamic - renderer)
        uint32_t mip = 0;
        uint32_t mipRange = 0;
        void *data = nullptr;

        // Properties that don't affect any hash
        DescriptorType type = DescriptorType::MaxEnum;
        Layout::ImageLayout layout = Layout::ImageLayout::Max;
        uint64_t range = 0;
        uint32_t dynamicOffset = 0;
        uint32_t structSize = 0;
        uint32_t arrayLength = 0;
        bool asArray = false;

        // Debugging
        std::string name;

    private:
        uint64_t m_Hash = 0;
    };

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
    // VkDescriptorSet CreateDescriptor(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout, VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);

    /**
     * @brief Updates a descriptor set with new image information
     *
     * @param device The logical device
     * @param descriptorSet The descriptor set to update
     * @param sampler The sampler to use
     * @param image_view The image view to use
     * @param image_layout The layout of the image
     */
    // void UpdateDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet, VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);

}

// -------------------------------------------------------
