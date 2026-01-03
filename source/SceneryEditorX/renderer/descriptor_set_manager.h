/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* descriptor_set_manager.h
* -------------------------------------------------------
* Created: 26/7/2025
* -------------------------------------------------------
*/
#pragma once
#include "shaders/shader.h"
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/renderer/enums.h>
#include <SceneryEditorX/renderer/image_view.h>
#include <SceneryEditorX/renderer/storage_buffer.h>
#include <SceneryEditorX/renderer/texture.h>
#include <SceneryEditorX/renderer/uniform_buffer.h>
#include <SceneryEditorX/utils/pointers.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    struct RenderPassInput
    {
        ResourceType type = ResourceType::None;
        std::vector<Ref<RefCounted>> input;

        RenderPassInput() = default;
		RenderPassInput(Ref<UniformBuffer> uniformBuffer);
        RenderPassInput(Ref<UniformBufferSet> uniformBufferSet);
        RenderPassInput(Ref<StorageBuffer> storageBuffer);
        RenderPassInput(Ref<StorageBufferSet> storageBufferSet);
        RenderPassInput(Ref<Texture2D> texture);
        RenderPassInput(Ref<TextureCube> texture);
        RenderPassInput(Ref<Image2D> image);

        void Set(Ref<UniformBuffer> uniformBuffer, uint32_t index = 0);
        void Set(Ref<UniformBufferSet> uniformBufferSet, uint32_t index = 0);
        void Set(Ref<StorageBuffer> storageBuffer, uint32_t index = 0);
        void Set(Ref<StorageBufferSet> storageBufferSet, uint32_t index = 0);
        void Set(Ref<Texture2D> texture, uint32_t index = 0);
        void Set(Ref<TextureCube> texture, uint32_t index = 0);
        void Set(Ref<Image2D> image, uint32_t index = 0);
        void Set(Ref<ImageView> image, uint32_t index = 0);
    };

    inline bool IsCompatibleInput(ResourceType input, VkDescriptorType descriptorType)
    {
        switch (descriptorType)
        {
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return input == ResourceType::Texture2D || input == ResourceType::TextureCube || input == ResourceType::Image2D;
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return input == ResourceType::Image2D;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return input == ResourceType::UniformBuffer || input == ResourceType::UniformBufferSet;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return input == ResourceType::StorageBuffer || input == ResourceType::StorageBufferSet;
        }
        return false;
    }

    inline ResourceInputType RenderPassInputTypeFromVulkanDescriptorType(VkDescriptorType descriptorType)
    {
        switch (descriptorType)
        {
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return ResourceInputType::ImageSampler2D;
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return ResourceInputType::StorageImage2D;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return ResourceInputType::UniformBuffer;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return ResourceInputType::StorageBuffer;
        }
        SEDX_CORE_ASSERT(false);
        return ResourceInputType::None;
    }

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

        // Which descriptor sets should be managed
        uint32_t startSet = 0;
        uint32_t endSet = 3;

        bool defaultResources = false;
    };

    struct DescriptorSetManager
    {
        /**
         * Input Resources (map of set->binding->resource)
         *
         * Invalidated input resources will attempt to be assigned on Renderer::BeginRenderPass
         * This is useful for resources that may not exist at RenderPass creation but will be
         * present during actual rendering
         */
        std::map<uint32_t, std::map<uint32_t, RenderPassInput>> inputResources;
        std::map<uint32_t, std::map<uint32_t, RenderPassInput>> invalidatedInputResources;
        std::map<std::string, RenderPassInputDeclaration> inputDeclarations;

		// Per-frame in flight
        std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;


        /**
         * @struct WriteDescriptor
         * @brief Write Descriptor Structure
         *
         * Holds a Vulkan write descriptor set and associated resource handles.
         */
        struct WriteDescriptor
        {
            VkWriteDescriptorSet writeDescriptorSet{}; 
            std::vector<void*> resourceHandles;
        };

        std::vector<std::map<uint32_t, std::map<uint32_t, WriteDescriptor>>> writeDescriptorMap;

        DescriptorSetManager() = default;
        DescriptorSetManager(const DescriptorSetManager& other);
        DescriptorSetManager(const DescriptorSetManagerSpecification& specification);
        static DescriptorSetManager Copy(const DescriptorSetManager& other);

		void AddInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet);
        void AddInput(std::string_view name, Ref<UniformBuffer> uniformBuffer);
        void AddInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet);
        void AddInput(std::string_view name, Ref<StorageBuffer> storageBuffer);
        void AddInput(std::string_view name, Ref<Texture2D> texture, uint32_t arrayIndex = 0);
        void AddInput(std::string_view name, Ref<TextureCube> textureCube, uint32_t arrayIndex = 0);
        void AddInput(std::string_view name, Ref<Image2D> image, uint32_t arrayIndex = 0);
        void AddInput(std::string_view name, Ref<ImageView> image, uint32_t arrayIndex = 0);

        template<typename T>
        Ref<T> GetInput(std::string_view name)
        {
            if (const RenderPassInputDeclaration* decl = GetInputDeclaration(name))
            {
                if (auto setIt = inputResources.find(decl->set); setIt != inputResources.end())
                {
                    if (auto resourceIt = setIt->second.find(decl->binding); resourceIt != setIt->second.end())
                        return resourceIt->second.input[0].As<T>();
                }
            }
            return nullptr;
        }

        void Bake();
        bool Validate();
        void InvalidateAndUpdate();
        [[nodiscard]] bool IsInvalidated(uint32_t set, uint32_t binding) const;

        [[nodiscard]] std::set<uint32_t> HasBufferSets() const;
        [[nodiscard]] VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
        [[nodiscard]] bool HasDescriptorSets() const;
        [[nodiscard]] uint32_t GetFirstSetIndex() const;
        [[nodiscard]] const std::vector<VkDescriptorSet>& GetDescriptorSets(uint32_t frameIndex) const;
        [[nodiscard]] bool IsInputValid(std::string_view name) const;
        [[nodiscard]] const RenderPassInputDeclaration* GetInputDeclaration(std::string_view name) const;
    private:
        void Init();
        DescriptorSetManagerSpecification m_Specification;
        VkDescriptorPool m_DescriptorPool = nullptr;
    };

}

// -------------------------------------------------------
