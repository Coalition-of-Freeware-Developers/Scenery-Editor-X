/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader_resource.h
* -------------------------------------------------------
* Created: 8/6/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class UniformBuffer;
	class StorageBuffer;
	class ImageSampler;

    /// -------------------------------------------------

    class ShaderResource
    {
    public:
		ShaderResource() = default;
        ShaderResource(std::string name, const uint32_t set, const uint32_t resourceRegister, const uint32_t count)
            : name(std::move(name)), set(set), Register(resourceRegister), count(count) { }

		[[nodiscard]] virtual const std::string& GetName() const { return name; }
        [[nodiscard]] virtual uint32_t GetSet() const { return set; }
		[[nodiscard]] virtual uint32_t GetRegister() const { return Register; }
		[[nodiscard]] virtual uint32_t GetCount() const { return count; }

        struct ShaderDescriptorSet
		{
			std::unordered_map<uint32_t, UniformBuffer> UniformBuffers;
			std::unordered_map<uint32_t, StorageBuffer> StorageBuffers;
			std::unordered_map<uint32_t, ImageSampler> ImageSamplers;
			std::unordered_map<uint32_t, ImageSampler> StorageImages;
			std::unordered_map<uint32_t, ImageSampler> SeparateTextures; // Not really an image sampler.
			std::unordered_map<uint32_t, ImageSampler> SeparateSamplers;

			std::unordered_map<std::string, VkWriteDescriptorSet> WriteDescriptorSets;

            explicit operator bool() const { return !(StorageBuffers.empty() && UniformBuffers.empty() && ImageSamplers.empty() && StorageImages.empty() && SeparateTextures.empty() && SeparateSamplers.empty()); }
		};
    private:
        std::string name;
        uint32_t set = 0;
        uint32_t Register = 0;
        uint32_t count = 0;
    };

    typedef std::vector<ShaderResource *> ShaderResourceList;

}

/// -------------------------------------------------------
