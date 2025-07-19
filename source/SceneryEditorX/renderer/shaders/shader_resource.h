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
#include <unordered_map>
#include <SceneryEditorX/filestreaming/filestream_reader.h>
#include <SceneryEditorX/filestreaming/filestream_writer.h>

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
		
		struct PushConstantRange
		{
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			uint32_t Offset = 0;
			uint32_t Size = 0;

			static void Serialize(StreamWriter* writer, const PushConstantRange& range) { writer->WriteRaw(range); }
			static void Deserialize(StreamReader* reader, PushConstantRange& range) { reader->ReadRaw(range); }
		};

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

        /// -------------------------------------------------

        struct UniformBuffer
        {
            VkDescriptorBufferInfo Descriptor;
            uint32_t Size = 0;
            uint32_t BindingPoint = 0;
            std::string Name;
            VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

            static void Serialize(StreamWriter *serializer, const UniformBuffer &instance)
            {
                serializer->WriteRaw(instance.Descriptor);
                serializer->WriteRaw(instance.Size);
                serializer->WriteRaw(instance.BindingPoint);
                serializer->WriteString(instance.Name);
                serializer->WriteRaw(instance.ShaderStage);
            }

            static void Deserialize(StreamReader *deserializer, UniformBuffer &instance)
            {
                deserializer->ReadRaw(instance.Descriptor);
                deserializer->ReadRaw(instance.Size);
                deserializer->ReadRaw(instance.BindingPoint);
                deserializer->ReadString(instance.Name);
                deserializer->ReadRaw(instance.ShaderStage);
            }
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
