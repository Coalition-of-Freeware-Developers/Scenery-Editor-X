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
#include <GraphicsEngine/vulkan/vk_device.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    ///< Forward declarations
    class VulkanDevice;

    /**
     * @struct BindlessResources
     * @brief Manages bindless resource descriptors for efficient GPU resource access
     * 
     * Bindless resources allow the GPU to access a large number of resources (textures, buffers, etc.)
     * without rebinding descriptor sets between draw calls. This enables more efficient rendering
     * by reducing API overhead and state changes.
     */
    struct BindlessResources
    {
        /**
         * @enum BindlessType
         * @brief Defines the types of resources that can be accessed in a bindless fashion
         */
        enum BindlessType : uint8_t
        {
            TEXTURE,      ///< Regular texture resources (sampled images)
            BUFFER,       ///< Storage or uniform buffer resources
            TLAS,         ///< Top Level Acceleration Structure for ray tracing
            STORAGE_IMAGE ///< Images that support read/write operations in shaders
        };

        /** @brief Descriptor pool used for ImGui interface elements */
        VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;

        /** @brief The descriptor set containing all bindless resources */
        VkDescriptorSet bindlessDescriptorSet = VK_NULL_HANDLE;

        /** @brief Descriptor pool from which the bindless descriptor set is allocated */
        VkDescriptorPool bindlessDescriptorPool = VK_NULL_HANDLE;

        /** @brief Layout defining the organization of descriptors within the bindless set */
        VkDescriptorSetLayout bindlessDescriptorSetLayout = VK_NULL_HANDLE;

        /** @brief Maximum number of storage buffers that can be accessed */
        constexpr static uint32_t MAX_STORAGE_BUFFERS = 8192;

        /** @brief Maximum number of sampled images (textures) that can be accessed */
        constexpr static uint32_t MAX_SAMPLED_IMAGES = 8192;

        /** @brief Maximum number of storage images that can be accessed */
        constexpr static uint32_t MAX_STORAGE_IMAGES = 1024;

        /** @brief Maximum number of uniform buffers that can be accessed */
        constexpr static uint32_t MAX_UNIFORM_BUFFERS = 1024;
    };

    /**
     * @struct Descriptors
     * @brief Manages descriptor sets and resources for rendering
     */
    struct Descriptors : RefCounted
    {
        /** @brief Descriptor set layout for main rendering pipeline */
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        
        /** @brief Descriptor pool for allocating descriptor sets */
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        
        /** @brief Collection of allocated descriptor sets */
        std::vector<VkDescriptorSet> descriptorSets;
    };

	struct ImageDescriptorInfo
    {
        VkDescriptorSet                     TargetDescriptorSet;
        uint32_t                            TargetDescriptorBinding;
        uint32_t                            TargetArrayElement;
        VkDescriptorType                    TargetDescriptorType;
        std::vector<VkDescriptorImageInfo>  ImageInfos;
    };

	struct BufferDescriptorInfo
    {
        VkDescriptorSet                     TargetDescriptorSet;
        uint32_t                            TargetDescriptorBinding;
        uint32_t                            TargetArrayElement;
        VkDescriptorType                    TargetDescriptorType;
        std::vector<VkDescriptorBufferInfo> BufferInfos;
    };

	struct TexelBufferDescriptorInfo
    {
        VkDescriptorSet                     TargetDescriptorSet;
        uint32_t                            TargetDescriptorBinding;
        uint32_t                            TargetArrayElement;
        VkDescriptorType                    TargetDescriptorType;
        std::vector<VkBufferView>           TexelBufferViews;
    };

	struct CopyDescriptorInfo
    {
        VkDescriptorSet     TargetDescriptorSet;
        uint32_t            TargetDescriptorBinding;
        uint32_t            TargetArrayElement;
        VkDescriptorSet     SourceDescriptorSet;
        uint32_t            SourceDescriptorBinding;
        uint32_t            SourceArrayElement;
        uint32_t            DescriptorCount;
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
    VkDescriptorSet CreateDescriptor(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout, 
                                    VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);
    
    /**
     * @brief Updates a descriptor set with new image information
     * 
     * @param device The logical device
     * @param descriptorSet The descriptor set to update
     * @param sampler The sampler to use
     * @param image_view The image view to use
     * @param image_layout The layout of the image
     */
    void UpdateDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet, 
                            VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);

    /**
     * @brief Create a descriptor set layout for bindless resources
     * 
     * @param device The logical device
     * @return VkDescriptorSetLayout The created descriptor set layout
     */
    VkDescriptorSetLayout CreateBindlessDescriptorSetLayout(VkDevice device);

    /**
     * @brief Create a descriptor pool for bindless resources
     * 
     * @param device The logical device
     * @return VkDescriptorPool The created descriptor pool
     */
    VkDescriptorPool CreateBindlessDescriptorPool(VkDevice device);

    /**
     * @brief Initialize the bindless resources structure
     * 
     * @param device The logical device
     * @param bindlessResources The bindless resources structure to initialize
     */
    void InitializeBindlessResources(VkDevice device, BindlessResources& bindlessResources);

    /**
     * @brief Clean up bindless resources
     * 
     * @param device The logical device
     * @param bindlessResources The bindless resources to clean up
     */
    void CleanupBindlessResources(VkDevice device, BindlessResources& bindlessResources);

    /**
     * @brief Update a texture descriptor in the bindless descriptor set
     * 
     * @param device The logical device
     * @param bindlessResources The bindless resources
     * @param arrayElement The array element index to update
     * @param sampler The sampler to use
     * @param imageView The image view to use
     * @param imageLayout The layout of the image
     */
    void UpdateBindlessTexture(VkDevice device, const BindlessResources& bindlessResources, 
                              uint32_t arrayElement, VkSampler sampler, 
                              VkImageView imageView, VkImageLayout imageLayout);

    /**
     * @brief Update a storage buffer descriptor in the bindless descriptor set
     * 
     * @param device The logical device
     * @param bindlessResources The bindless resources
     * @param arrayElement The array element index to update
     * @param buffer The buffer to use
     * @param offset The offset into the buffer
     * @param range The range of the buffer to use
     */
    void UpdateBindlessStorageBuffer(VkDevice device, const BindlessResources& bindlessResources,
                                   uint32_t arrayElement, VkBuffer buffer, 
                                   VkDeviceSize offset, VkDeviceSize range);

    /**
     * @brief Update a storage image descriptor in the bindless descriptor set
     * 
     * @param device The logical device
     * @param bindlessResources The bindless resources
     * @param arrayElement The array element index to update
     * @param imageView The image view to use
     * @param imageLayout The layout of the image
     */
    void UpdateBindlessStorageImage(VkDevice device, const BindlessResources& bindlessResources,
                                  uint32_t arrayElement, VkImageView imageView, 
                                  VkImageLayout imageLayout);

} // namespace SceneryEditorX

/// -------------------------------------------------------
