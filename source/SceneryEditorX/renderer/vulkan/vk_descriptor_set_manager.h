/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_descriptor_set_manager.h
* -------------------------------------------------------
* Created: 26/7/2025
* -------------------------------------------------------
*/
#pragma once
//#include <SceneryEditorX/renderer/buffers/storage_buffer_set.h>
//#include <SceneryEditorX/renderer/buffers/uniform_buffer_set.h>
//#include <SceneryEditorX/renderer/vulkan/vk_enums.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /*
    struct RenderPassInput
    {
        ResourceType type = ResourceType::None;
        std::vector<Ref<RefCounted>> input;

        /// -------------------------------------------------------

        RenderPassInput() = default;
        RenderPassInput(const Ref<UniformBuffer> &uniformBuffer) : type(ResourceType::UniformBuffer), input(std::vector<Ref<RefCounted>>(1, uniformBuffer)) {}
        //RenderPassInput(const Ref<UniformBufferSet> &uniformBufferSet) : type(ResourceType::UniformBufferSet), input(std::vector<Ref<RefCounted>>(1, uniformBufferSet)) {}
        RenderPassInput(const Ref<StorageBuffer> &storageBuffer) : type(ResourceType::StorageBuffer), input(std::vector<Ref<RefCounted>>(1, storageBuffer)) {}
        RenderPassInput(const Ref<StorageBufferSet> &storageBufferSet) : type(ResourceType::StorageBufferSet), input(std::vector<Ref<RefCounted>>(1, storageBufferSet)) {}
        RenderPassInput(const Ref<Texture2D> &texture) : type(ResourceType::Texture2D), input(std::vector<Ref<RefCounted>>(1, texture)) {}
        RenderPassInput(const Ref<TextureCube> &texture) : type(ResourceType::TextureCube), input(std::vector<Ref<RefCounted>>(1, texture)) {}
        RenderPassInput(const Ref<Image2D> &image) : type(ResourceType::Image2D), input(std::vector<Ref<RefCounted>>(1, image)) {}

        /// -------------------------------------------------------

        void Set(const Ref<UniformBuffer> &uniformBuffer, const uint32_t index = 0)
        {
            type = ResourceType::UniformBuffer;
            input[index] = uniformBuffer;
        }

        /*
        void Set(const Ref<UniformBufferSet> &uniformBufferSet, const uint32_t index = 0)
        {
            type = ResourceType::UniformBufferSet;
            input[index] = uniformBufferSet;
        }
        #1#

        void Set(const Ref<StorageBuffer> &storageBuffer, const uint32_t index = 0)
        {
            type = ResourceType::StorageBuffer;
            input[index] = storageBuffer;
        }

        void Set(const Ref<StorageBufferSet> &storageBufferSet, const uint32_t index = 0)
        {
            type = ResourceType::StorageBufferSet;
            input[index] = storageBufferSet;
        }

        void Set(const Ref<Texture2D> &texture, uint32_t index = 0)
        {
            type = ResourceType::Texture2D;
            input[index] = texture;
        }

        void Set(const Ref<TextureCube> &texture, uint32_t index = 0)
        {
            type = ResourceType::TextureCube;
            input[index] = texture;
        }

        void Set(const Ref<Image2D> &image, uint32_t index = 0)
        {
            type = ResourceType::Image2D;
            input[index] = image;
        }

        void Set(const Ref<ImageView> &image, uint32_t index = 0)
        {
            type = ResourceType::Image2D;
            if (input.size() <= index)
                input.resize(index + 1);

            input[index] = image.As<RefCounted>();
        }
    };

    /// -------------------------------------------------------

	inline bool IsCompatibleInput(ResourceType input, VkDescriptorType descriptorType)
    {
        switch (descriptorType)
        {
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return input == ResourceType::Texture2D || input == ResourceType::TextureCube || input == ResourceType::Image2D;
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:	return input == ResourceType::Image2D;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:	return input == ResourceType::UniformBuffer || input == ResourceType::UniformBufferSet;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return input == ResourceType::StorageBuffer || input == ResourceType::StorageBufferSet;
        }
        return false;
    }

    /// -------------------------------------------------------

    inline ResourceInputType RenderPassInputTypeFromVulkanDescriptorType(VkDescriptorType descriptorType)
    {
        switch (descriptorType)
        {
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:	return ResourceInputType::ImageSampler2D;
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:	return ResourceInputType::StorageImage2D;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return ResourceInputType::UniformBuffer;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return ResourceInputType::StorageBuffer;
        }

        SEDX_CORE_ASSERT(false);
        return ResourceInputType::None;
    }

    /// -------------------------------------------------------

    struct RenderPassInputDeclaration
    {
        ResourceInputType type = ResourceInputType::None;
        uint32_t set = 0;
        uint32_t binding = 0;
        uint32_t count = 0;
        std::string name;
    };

    struct DescriptorSetManagerSpecification
    {
        Ref<Shader> shader;
        std::string debugName;

        /// Which descriptor sets should be managed
        uint32_t startSet = 0;
        uint32_t endSet = 3;

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
         #1#
        std::map<uint32_t, std::map<uint32_t, RenderPassInput>> inputResources;
        std::map<uint32_t, std::map<uint32_t, RenderPassInput>> invalidatedInputResources;
        std::map<std::string, RenderPassInputDeclaration> inputDeclarations;

        ///< Per-frame in flight
        std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;

        struct WriteDescriptor
        {
            VkWriteDescriptorSet writeDescriptorSet{};
            std::vector<void *> resourceHandles;
        };

        std::vector<std::map<uint32_t, std::map<uint32_t, WriteDescriptor>>> writeDescriptorMap;

        DescriptorSetManager() = default;
        DescriptorSetManager(const DescriptorSetManager &other);
        DescriptorSetManager(const DescriptorSetManagerSpecification &specification);
        static DescriptorSetManager Copy(const DescriptorSetManager &other);

        /// -------------------------------------------------------

        //void AddInput(std::string_view name, const Ref<UniformBufferSet> &uniformBufferSet);
        void AddInput(std::string_view name, const Ref<UniformBuffer> &uniformBuffer);
        void AddInput(std::string_view name, const Ref<StorageBufferSet> &storageBufferSet);
        void AddInput(std::string_view name, const Ref<StorageBuffer> &storageBuffer);
        void AddInput(std::string_view name, const Ref<Texture2D> &texture, uint32_t index = 0);
        void AddInput(std::string_view name, const Ref<TextureCube> &textureCube);
        void AddInput(std::string_view name, const Ref<Image2D> &image);
        void AddInput(std::string_view name, const Ref<ImageView> &image);

        /// -------------------------------------------------------

        template <typename T>
        Ref<T> GetInput(std::string_view name)
        {
            if (const RenderPassInputDeclaration *decl = GetInputDeclaration(name))
            {
                if (auto setIt = inputResources.find(decl->set); setIt != inputResources.end())
                {
                    if (auto resourceIt = setIt->second.find(decl->binding); resourceIt != setIt->second.end())
                        return resourceIt->second.input[0].As<T>();
                }
            }
            return nullptr;
        }

        /// -------------------------------------------------------

        void Bake();
        bool Validate();
        [[nodiscard]] bool IsInvalidated(uint32_t set, uint32_t binding) const;
        void InvalidateAndUpdate();

        [[nodiscard]] std::set<uint32_t> HasBufferSets() const;
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
    */

}

/// -------------------------------------------------------
