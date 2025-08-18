/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_descriptor_set_manager.h
* -------------------------------------------------------
* Created: 16/6/2025
* -------------------------------------------------------
*/
#pragma once
#include "vk_image_view.h"
#include "SceneryEditorX/renderer/render_pass.h"
#include "SceneryEditorX/renderer/texture.h"
#include "SceneryEditorX/renderer/buffers/storage_buffer.h"
#include "SceneryEditorX/renderer/shaders/shader.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

    inline bool IsCompatibleInput(ResourceType input, VkDescriptorType descriptorType)
    {
        switch (descriptorType)
        {
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return input == ResourceType::Texture2D || input == ResourceType::TextureCube || input == ResourceType::Image2D;
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return input == ResourceType::Image2D;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return input == ResourceType::UniformBuffer || input == ResourceType::UniformSet;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return input == ResourceType::StorageBuffer || input == ResourceType::StorageSet;
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

    /// -------------------------------------------------------

    struct DescriptorSetManagerSpecification
    {
        Ref<Shader> shader;
        std::string debugName;
        uint32_t startSet = 0;
        uint32_t endSet = 3;
        bool DefaultResources = false;
    };

    /// -------------------------------------------------------

    class DescriptorSetManager
    {
    public:
        std::map<uint32_t, std::map<uint32_t, RenderPassInput>> inputResources;
        std::map<uint32_t, std::map<uint32_t, RenderPassInput>> invalidatedInputResources;
        std::map<std::string, RenderPassInputDeclaration> inputDeclarations;
        std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;

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

        void AddInput(std::string_view name, Ref<UniformBufferSet>& uniformBufferSet);
        void AddInput(std::string_view name, Ref<UniformBuffer>& uniformBuffer);
        void AddInput(std::string_view name, Ref<StorageBufferSet>& storageBufferSet);
        void AddInput(std::string_view name, Ref<StorageBuffer>& storageBuffer);
        void AddInput(std::string_view name, Ref<Texture2D>& texture, uint32_t index = 0);
        void AddInput(std::string_view name, Ref<TextureCube>& textureCube);
        void AddInput(std::string_view name, Ref<Image2D>& image);
        void AddInput(std::string_view name, Ref<ImageView>& image);

        /// -------------------------------------------------------

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

        /// -------------------------------------------------------

        void Bake();
        bool Validate();
        [[nodiscard]] bool IsInvalidated(uint32_t set, uint32_t binding) const;
        void InvalidateAndUpdate();
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

/// -------------------------------------------------------
