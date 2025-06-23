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
#include <SceneryEditorX/renderer/buffers/storage_buffer.h>
#include <SceneryEditorX/renderer/buffers/uniform_buffer.h>
#include <SceneryEditorX/renderer/shaders/shader.h>
#include <SceneryEditorX/renderer/vulkan/vk_enums.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
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
        VkDescriptorSetLayout bindlessDescriptorLayout = VK_NULL_HANDLE;

        /** @brief Maximum number of storage buffers that can be accessed */
        constexpr static uint32_t MAX_STORAGE_BUFFERS = 8192;

        /** @brief Maximum number of sampled images (textures) that can be accessed */
        constexpr static uint32_t MAX_SAMPLED_IMAGES = 8192;

        /** @brief Maximum number of storage images that can be accessed */
        constexpr static uint32_t MAX_STORAGE_IMAGES = 1024;

        /** @brief Maximum number of uniform buffers that can be accessed */
        constexpr static uint32_t MAX_UNIFORM_BUFFERS = 1024;
    };

    /// -------------------------------------------------------

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
        VkDescriptorSet                     targetDescriptorSet;
        uint32_t                            targetDescriptorBinding;
        uint32_t                            targetArrayElement;
        VkDescriptorType                    targetDescriptorType;
        std::vector<VkDescriptorImageInfo>  imageInfos;
    };

	struct BufferDescriptorInfo
    {
        VkDescriptorSet                     targetDescriptorSet;
        uint32_t                            targetDescriptorBinding;
        uint32_t                            targetArrayElement;
        VkDescriptorType                    targetDescriptorType;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
    };

	struct TexelBufferDescriptorInfo
    {
        VkDescriptorSet                     targetDescriptorSet;
        uint32_t                            targetDescriptorBinding;
        uint32_t                            targetArrayElement;
        VkDescriptorType                    targetDescriptorType;
        std::vector<VkBufferView>           texelBufferViews;
    };

	struct CopyDescriptorInfo
    {
        VkDescriptorSet     targetDescriptorSet;
        uint32_t            targetDescriptorBinding;
        uint32_t            targetArrayElement;
	    /* -----------------------------------*/
        VkDescriptorSet     sourceDescriptorSet;
        uint32_t            sourceDescriptorBinding;
        uint32_t            sourceArrayElement;
        uint32_t            descriptorCount;
    };

    /// -------------------------------------------------------

	/*
	struct RenderPassInput
	{
        ResourceType type = ResourceType::None;
        std::vector<Ref<RefCounted>> Input;

		RenderPassInput() = default;

		RenderPassInput(Ref<UniformBuffer> &uniformBuffer) : type(ResourceType::UniformBuffer), Input(std::vector<Ref<RefCounted>>{ 1, uniformBuffer }) {}
        RenderPassInput(Ref<UniformBufferSet> &uniformBufferSet) : type(ResourceType::UniformBuffer), Input(std::vector<Ref<RefCounted>>{ 1,  uniformBufferSet }) {}
		RenderPassInput(Ref<StorageBuffer> storageBuffer) : type(ResourceType::StorageBuffer), Input(std::vector<Ref<RefCounted>>(1, storageBuffer)) {}
		RenderPassInput(Ref<StorageBufferSet> storageBufferSet) : type(ResourceType::StorageBufferSet), Input(std::vector<Ref<RefCounted>>(1, storageBufferSet)) {}
		RenderPassInput(Ref<Texture2D> texture) : type(ResourceType::Texture2D), Input(std::vector<Ref<RefCounted>>(1, texture)) {}
		RenderPassInput(Ref<TextureCube> texture) : type(ResourceType::TextureCube), Input(std::vector<Ref<RefCounted>>(1, texture)) {}
		RenderPassInput(Ref<Image2D> image) : type(ResourceType::Image2D), Input(std::vector<Ref<RefCounted>>(1, image)) {}

		void Set(Ref<UniformBuffer> &uniformBuffer, uint32_t index = 0)
		{
			type = ResourceType::UniformBuffer;
			Input[index] = uniformBuffer;
		}

		void Set(Ref<UniformBufferSet> uniformBufferSet, uint32_t index = 0)
		{
			type = ResourceType::UniformBufferSet;
			Input[index] = uniformBufferSet;
		}

		void Set(Ref<StorageBuffer> storageBuffer, uint32_t index = 0)
		{
			type = ResourceType::StorageBuffer;
			Input[index] = storageBuffer;
		}

		void Set(Ref<StorageBufferSet> storageBufferSet, uint32_t index = 0)
		{
			type = ResourceType::StorageBufferSet;
			Input[index] = storageBufferSet;
		}

		void Set(Ref<Texture2D> texture, uint32_t index = 0)
		{
			type = ResourceType::Texture2D;
			Input[index] = texture;
		}

		void Set(Ref<TextureCube> texture, uint32_t index = 0)
		{
			type = ResourceType::TextureCube;
			Input[index] = texture;
		}

		void Set(Ref<Image2D> image, uint32_t index = 0)
		{
			type = ResourceType::Image2D;
			Input[index] = image;
		}

		void Set(Ref<ImageView> image, uint32_t index = 0)
		{
			type = ResourceType::Image2D;
			Input[index] = image;
		}
	};
	*/


    /// -------------------------------------------------------

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

    /// -------------------------------------------------------

    /**
     * @brief Create a descriptor set layout for bindless resources
     * 
     * @param device The logical device
     * @return VkDescriptorSetLayout The created descriptor set layout
     */
    VkDescriptorSetLayout CreateBindlessDescriptorSetLayout(VkDevice device);

    /// -------------------------------------------------------

    /**
     * @brief Create a descriptor pool for bindless resources
     * 
     * @param device The logical device
     * @return VkDescriptorPool The created descriptor pool
     */
    VkDescriptorPool CreateBindlessDescriptorPool(VkDevice device);

    /// -------------------------------------------------------

    /**
     * @brief Initialize the bindless resources structure
     * 
     * @param device The logical device
     * @param bindlessResources The bindless resources structure to initialize
     */
    void InitializeBindlessResources(VkDevice device, BindlessResources& bindlessResources);

    /// -------------------------------------------------------

    /**
     * @brief Clean up bindless resources
     * 
     * @param device The logical device
     * @param bindlessResources The bindless resources to clean up
     */
    void CleanupBindlessResources(VkDevice device, BindlessResources& bindlessResources);

    /// -------------------------------------------------------

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
    void UpdateBindlessTexture(VkDevice device, const BindlessResources& bindlessResources, uint32_t arrayElement, VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout);

    /// -------------------------------------------------------

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
    void UpdateBindlessStorageBuffer(VkDevice device, const BindlessResources& bindlessResources, uint32_t arrayElement, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

    /// -------------------------------------------------------

    /**
     * @brief Update a storage image descriptor in the bindless descriptor set
     * 
     * @param device The logical device
     * @param bindlessResources The bindless resources
     * @param arrayElement The array element index to update
     * @param imageView The image view to use
     * @param imageLayout The layout of the image
     */
    void UpdateBindlessStorageImage(VkDevice device, const BindlessResources& bindlessResources, uint32_t arrayElement, VkImageView imageView, VkImageLayout imageLayout);

    /// -------------------------------------------------------



    struct RenderPassInput
    {
        ResourceType Type = ResourceType::None;
        std::vector<Ref<RefCounted>> Input;

        RenderPassInput() = default;

        RenderPassInput(Ref<UniformBuffer> uniformBuffer) : Type(ResourceType::UniformBuffer), Input(std::vector<Ref<RefCounted>>(1, uniformBuffer)) {}

        RenderPassInput(Ref<UniformBufferSet> uniformBufferSet) : Type(ResourceType::UniformBufferSet), Input(std::vector<Ref<RefCounted>>(1, uniformBufferSet)) {}

        RenderPassInput(Ref<StorageBuffer> storageBuffer) : Type(ResourceType::StorageBuffer), Input(std::vector<Ref<RefCounted>>(1, storageBuffer)) {}

        RenderPassInput(Ref<StorageBufferSet> storageBufferSet) : Type(ResourceType::StorageBufferSet), Input(std::vector<Ref<RefCounted>>(1, storageBufferSet)) {}

        RenderPassInput(Ref<Texture2D> texture) : Type(ResourceType::Texture2D), Input(std::vector<Ref<RefCounted>>(1, texture)) {}

        RenderPassInput(Ref<TextureCube> texture) : Type(ResourceType::TextureCube), Input(std::vector<Ref<RefCounted>>(1, texture)) {}

        RenderPassInput(Ref<Image2D> image) : Type(ResourceType::Image2D), Input(std::vector<Ref<RefCounted>>(1, image)) {}

        void Set(Ref<UniformBuffer> uniformBuffer, uint32_t index = 0)
        {
            Type = ResourceType::UniformBuffer;
            Input[index] = uniformBuffer;
        }

        void Set(Ref<UniformBufferSet> uniformBufferSet, uint32_t index = 0)
        {
            Type = ResourceType::UniformBufferSet;
            Input[index] = uniformBufferSet;
        }

        void Set(Ref<StorageBuffer> storageBuffer, uint32_t index = 0)
        {
            Type = ResourceType::StorageBuffer;
            Input[index] = storageBuffer;
        }

        void Set(Ref<StorageBufferSet> storageBufferSet, uint32_t index = 0)
        {
            Type = ResourceType::StorageBufferSet;
            Input[index] = storageBufferSet;
        }

        void Set(Ref<Texture2D> texture, uint32_t index = 0)
        {
            Type = ResourceType::Texture2D;
            Input[index] = texture;
        }

        void Set(Ref<TextureCube> texture, uint32_t index = 0)
        {
            Type = ResourceType::TextureCube;
            Input[index] = texture;
        }

        void Set(Ref<Image2D> image, uint32_t index = 0)
        {
            Type = ResourceType::Image2D;
            Input[index] = image;
        }

        void Set(Ref<ImageView> image, uint32_t index = 0)
        {
            Type = ResourceType::Image2D;
            Input[index] = image;
        }
    };

    /// -------------------------------------------------------

    struct RenderPassInputDeclaration
    {
        RenderPassInputType Type = RenderPassInputType::None;
        uint32_t Set = 0;
        uint32_t Binding = 0;
        uint32_t Count = 0;
        std::string Name;
    };

    struct DescriptorSetManagerSpecification
    {
        Ref<Shader> Shader;
        std::string DebugName;

        // Which descriptor sets should be managed
        uint32_t StartSet = 0, EndSet = 3;

        bool DefaultResources = false;
    };

    /// -------------------------------------------------------

    struct DescriptorSetManager
    {
        /**
         * Input Resources (map of set->binding->resource)
         *
         * Invalidated input resources will attempt to be assigned on Renderer::BeginRenderPass
         * This is useful for resources that may not exist at RenderPass creation but will be
         * present during actual rendering
         */
        std::map<uint32_t, std::map<uint32_t, RenderPassInput>> InputResources;
        std::map<uint32_t, std::map<uint32_t, RenderPassInput>> InvalidatedInputResources;
        std::map<std::string, RenderPassInputDeclaration> InputDeclarations;

        /// Per-frame in flight
        std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;

        struct WriteDescriptor
        {
            VkWriteDescriptorSet WriteDescriptorSet{};
            std::vector<void *> ResourceHandles;
        };
        std::vector<std::map<uint32_t, std::map<uint32_t, WriteDescriptor>>> WriteDescriptorMap;

        DescriptorSetManager() = default;
        DescriptorSetManager(const DescriptorSetManager &other);
        DescriptorSetManager(const DescriptorSetManagerSpecification &specification);
        static DescriptorSetManager Copy(const DescriptorSetManager &other);

        void AddInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet);
        void AddInput(std::string_view name, Ref<UniformBuffer> uniformBuffer);
        void AddInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet);
        void AddInput(std::string_view name, Ref<StorageBuffer> storageBuffer);
        //void AddInput(std::string_view name, Ref<Texture2D> texture, uint32_t index = 0);
        //void AddInput(std::string_view name, Ref<TextureCube> textureCube);
        void AddInput(std::string_view name, Ref<Image2D> image);
        void AddInput(std::string_view name, Ref<ImageView> image);

        template <typename T>
        Ref<T> GetInput(std::string_view name)
        {
            if (const RenderPassInputDeclaration *decl = GetInputDeclaration(name))
            {
                auto setIt = InputResources.find(decl->Set);
                if (setIt != InputResources.end())
                {
                    auto resourceIt = setIt->second.find(decl->Binding);
                    if (resourceIt != setIt->second.end())
                        return resourceIt->second.Input[0].As<T>();
                }
            }
            return nullptr;
        }

        [[nodiscard]] bool IsInvalidated(uint32_t set, uint32_t binding) const;
        bool Validate();
        void Bake();

        [[nodiscard]] std::set<uint32_t> HasBufferSets() const;
        void InvalidateAndUpdate();

        [[nodiscard]] VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
        [[nodiscard]] bool HasDescriptorSets() const;
        [[nodiscard]] uint32_t GetFirstSetIndex() const;
        [[nodiscard]] const std::vector<VkDescriptorSet> &GetDescriptorSets(uint32_t frameIndex) const;
        [[nodiscard]] bool IsInputValid(std::string_view name) const;
        [[nodiscard]] const RenderPassInputDeclaration *GetInputDeclaration(std::string_view name) const;

    private:
        void Init();
        DescriptorSetManagerSpecification m_Specification;
        VkDescriptorPool m_DescriptorPool = nullptr;
    };

} // namespace SceneryEditorX

/// -------------------------------------------------------
