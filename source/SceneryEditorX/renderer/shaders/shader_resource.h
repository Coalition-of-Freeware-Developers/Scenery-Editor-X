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
#include "SceneryEditorX/renderer/buffers/storage_buffer.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class UniformBuffer;

    /// -------------------------------------------------

    /**
     * @brief Represents an image sampler resource used in shaders
     * 
     * This structure contains information about image samplers including
     * their dimensionality and binding information for shader reflection.
     */
    struct ImageSampler
    {
        uint32_t Dimension = 2;                                      ///< Texture dimension (1D, 2D, 3D, Cube)
        uint32_t bindingPoint = 0;                                   ///< Binding point in shader
        std::string name;                                            ///< Name of the sampler in shader
        VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM; ///< Shader stage flags
        VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; ///< Descriptor type
        
        /**
         * @brief Default constructor for ImageSampler
         */
        ImageSampler() = default;
        
        /**
         * @brief Constructor with parameters
         * @param dimension Texture dimension (1, 2, 3 for 1D, 2D, 3D respectively)
         * @param binding Binding point in shader
         * @param samplerName Name of the sampler
         * @param stage Shader stage where this sampler is used
         */
        ImageSampler(uint32_t dimension, uint32_t binding, std::string samplerName, VkShaderStageFlagBits stage)
            : Dimension(dimension), bindingPoint(binding), name(std::move(samplerName)), shaderStage(stage) {}
    };

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
			VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			uint32_t offset = 0;
			uint32_t size = 0;

			//static void Serialize(StreamWriter* writer, const PushConstantRange& range) { writer->WriteRaw(range); }
			//static void Deserialize(StreamReader* reader, PushConstantRange& range) { reader->ReadRaw(range); }
		};

        struct ShaderDescriptorSet
		{
			std::unordered_map<uint32_t, UniformBuffer> uniformBuffers;
            std::unordered_map<uint32_t, Ref<StorageBuffer>> storageBuffers;
			std::unordered_map<uint32_t, ImageSampler> imageSamplers;
			std::unordered_map<uint32_t, ImageSampler> storageImages;
			std::unordered_map<uint32_t, ImageSampler> separateTextures; /// Not really an image sampler.
			std::unordered_map<uint32_t, ImageSampler> separateSamplers;

			std::unordered_map<std::string, VkWriteDescriptorSet> writeDescriptorSets;

            explicit operator bool() const { return !(storageBuffers.empty() && uniformBuffers.empty() && imageSamplers.empty() && storageImages.empty() && separateTextures.empty() && separateSamplers.empty()); }
		};

        /// -------------------------------------------------

        struct UniformBuffer
        {
            VkDescriptorBufferInfo descriptor;
            uint32_t size = 0;
            uint32_t bindingPoint = 0;
            std::string name;
            VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

            /*
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
            */

        };
    private:
        std::string name;
        uint32_t set = 0;
        uint32_t Register = 0;
        uint32_t count = 0;
    };

    //typedef std::vector<ShaderResource *> ShaderResourceList;

}

/// -------------------------------------------------------
