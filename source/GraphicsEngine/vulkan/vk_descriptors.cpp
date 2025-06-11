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
#include <GraphicsEngine/vulkan/vk_descriptors.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    VkDescriptorSet CreateDescriptor(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout, 
                                    VkSampler sampler, VkImageView image_view, VkImageLayout image_layout)
    {
        /// Create Descriptor Set:
        VkDescriptorSet descriptor_set;
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = descriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &layout;
        
        VkResult result = vkAllocateDescriptorSets(device, &alloc_info, &descriptor_set);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to allocate descriptor set!");
            return VK_NULL_HANDLE;
        }

        /// Update the Descriptor Set:
        UpdateDescriptorSet(device, descriptor_set, sampler, image_view, image_layout);
        
        return descriptor_set;
    }
    
    void UpdateDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet, 
                            VkSampler sampler, VkImageView image_view, VkImageLayout image_layout)
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

    VkDescriptorSetLayout CreateBindlessDescriptorSetLayout(VkDevice device)
    {
        ///< Create descriptor set layout with binding for textures (binding 0), storage buffers (binding 1),
        ///< and storage images (binding 2), each with a large array size and appropriate flag for bindless usage
        VkDescriptorSetLayoutBinding bindings[3] = {};
        
        ///< Binding 0: Textures (Sampled Images)
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = BindlessResources::MAX_SAMPLED_IMAGES;
        bindings[0].stageFlags = VK_SHADER_STAGE_ALL;
        
        ///< Binding 1: Storage Buffers
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = BindlessResources::MAX_STORAGE_BUFFERS;
        bindings[1].stageFlags = VK_SHADER_STAGE_ALL;
        
        ///< Binding 2: Storage Images
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[2].descriptorCount = BindlessResources::MAX_STORAGE_IMAGES;
        bindings[2].stageFlags = VK_SHADER_STAGE_ALL;

        /// Create descriptor set layout binding flags for variable descriptor count
        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{};
        bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        
        VkDescriptorBindingFlags flags[3] = {
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
        };
        
        bindingFlags.bindingCount = 3;
        bindingFlags.pBindingFlags = flags;

        ///< Create the descriptor set layout
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 3;
        layoutInfo.pBindings = bindings;
        layoutInfo.pNext = &bindingFlags;
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        
        VkDescriptorSetLayout descriptorSetLayout;

        if (VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout); result != VK_SUCCESS)
		{
            SEDX_CORE_ERROR("Failed to create bindless descriptor set layout!");
            return VK_NULL_HANDLE;
        }
        
        return descriptorSetLayout;
    }

    VkDescriptorPool CreateBindlessDescriptorPool(VkDevice device)
    {
        /// Define the descriptor pool sizes for the different types
        VkDescriptorPoolSize poolSizes[3];
        
        /// Textures (Sampled Images)
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = BindlessResources::MAX_SAMPLED_IMAGES;
        
        /// Storage Buffers
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount = BindlessResources::MAX_STORAGE_BUFFERS;
        
        /// Storage Images
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes[2].descriptorCount = BindlessResources::MAX_STORAGE_IMAGES;

        /// Create the descriptor pool
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 100; /// Allow allocation of multiple descriptor sets if needed
        poolInfo.poolSizeCount = 3;
        poolInfo.pPoolSizes = poolSizes;
        
        VkDescriptorPool descriptorPool;

        if (VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool); result != VK_SUCCESS)
		{
            SEDX_CORE_ERROR("Failed to create bindless descriptor pool!");
            return VK_NULL_HANDLE;
        }
        
        return descriptorPool;
    }

    void InitializeBindlessResources(VkDevice device, BindlessResources& bindlessResources)
    {
        /// Create descriptor set layout for bindless resources
        bindlessResources.bindlessDescriptorSetLayout = CreateBindlessDescriptorSetLayout(device);
        
        /// Create descriptor pool for bindless resources
        bindlessResources.bindlessDescriptorPool = CreateBindlessDescriptorPool(device);
        
        /// Allocate descriptor set from the pool
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = bindlessResources.bindlessDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &bindlessResources.bindlessDescriptorSetLayout;

        if (VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &bindlessResources.bindlessDescriptorSet); result != VK_SUCCESS)
		{
            SEDX_CORE_ERROR("Failed to allocate bindless descriptor set!");
            return;
        }
        
        SEDX_CORE_INFO("Bindless resources initialized successfully");
    }

    void CleanupBindlessResources(VkDevice device, BindlessResources& bindlessResources)
    {
        if (bindlessResources.bindlessDescriptorPool != VK_NULL_HANDLE)
		{
            vkDestroyDescriptorPool(device, bindlessResources.bindlessDescriptorPool, nullptr);
            bindlessResources.bindlessDescriptorPool = VK_NULL_HANDLE;
        }
        
        if (bindlessResources.bindlessDescriptorSetLayout != VK_NULL_HANDLE)
		{
            vkDestroyDescriptorSetLayout(device, bindlessResources.bindlessDescriptorSetLayout, nullptr);
            bindlessResources.bindlessDescriptorSetLayout = VK_NULL_HANDLE;
        }
        
        bindlessResources.bindlessDescriptorSet = VK_NULL_HANDLE;
    }

    void UpdateBindlessTexture(VkDevice device, const BindlessResources& bindlessResources, 
                              uint32_t arrayElement, VkSampler sampler, 
                              VkImageView imageView, VkImageLayout imageLayout)
    {
        /// Early check if we have a valid descriptor set
        if (bindlessResources.bindlessDescriptorSet == VK_NULL_HANDLE)
		{
            SEDX_CORE_ERROR("Attempt to update bindless texture with invalid descriptor set");
            return;
        }
        
        /// Ensure array element index is within range
        if (arrayElement >= BindlessResources::MAX_SAMPLED_IMAGES)
		{
            SEDX_CORE_ERROR("Bindless texture array element out of range: {}", arrayElement);
            return;
        }
        
        /// Set up the descriptor image info
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = sampler;
        imageInfo.imageView = imageView;
        imageInfo.imageLayout = imageLayout;
        
        /// Set up the descriptor write
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = bindlessResources.bindlessDescriptorSet;
        write.dstBinding = 0; /// Texture binding is 0
        write.dstArrayElement = arrayElement;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;
        
        /// Update the descriptor set
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    void UpdateBindlessStorageBuffer(VkDevice device, const BindlessResources& bindlessResources,
                                   uint32_t arrayElement, VkBuffer buffer, 
                                   VkDeviceSize offset, VkDeviceSize range)
    {
        /// Early check if we have a valid descriptor set
        if (bindlessResources.bindlessDescriptorSet == VK_NULL_HANDLE)
		{
            SEDX_CORE_ERROR("Attempt to update bindless storage buffer with invalid descriptor set");
            return;
        }
        
        /// Ensure array element index is within range
        if (arrayElement >= BindlessResources::MAX_STORAGE_BUFFERS)
		{
            SEDX_CORE_ERROR("Bindless storage buffer array element out of range: {}", arrayElement);
            return;
        }
        
        /// Set up the descriptor buffer info
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffer;
        bufferInfo.offset = offset;
        bufferInfo.range = range;
        
        /// Set up the descriptor write
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = bindlessResources.bindlessDescriptorSet;
        write.dstBinding = 1; /// Storage buffer binding is 1
        write.dstArrayElement = arrayElement;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;
        
        /// Update the descriptor set
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    void UpdateBindlessStorageImage(VkDevice device, const BindlessResources& bindlessResources,
                                  uint32_t arrayElement, VkImageView imageView, 
                                  VkImageLayout imageLayout)
    {
        /// Early check if we have a valid descriptor set
        if (bindlessResources.bindlessDescriptorSet == VK_NULL_HANDLE)
		{
            SEDX_CORE_ERROR("Attempt to update bindless storage image with invalid descriptor set");
            return;
        }
        
        /// Ensure array element index is within range
        if (arrayElement >= BindlessResources::MAX_STORAGE_IMAGES)
		{
            SEDX_CORE_ERROR("Bindless storage image array element out of range: {}", arrayElement);
            return;
        }
        
        /// Set up the descriptor image info
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageView = imageView;
        imageInfo.imageLayout = imageLayout;
        
        /// Set up the descriptor write
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = bindlessResources.bindlessDescriptorSet;
        write.dstBinding = 2; /// Storage image binding is 2
        write.dstArrayElement = arrayElement;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;
        
        /// Update the descriptor set
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

} // namespace SceneryEditorX

/// -------------------------------------------------------
