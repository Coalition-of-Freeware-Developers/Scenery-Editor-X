 /**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * bindless_manager.h
 * -------------------------------------------------------
 * Created: 06/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include "descriptor.h"
#include "descriptor_set.h"
#include "render_data.h"
#include "pipeline/pipeline_state.h"
#include <unordered_map>
#include <vector>
#include <SceneryEditorX/utils/inheritance.h>

// --------------------------------------------------------------

namespace SceneryEditorX
{
	class Device;
	class ImageResource;
	class Sampler;
	class Buffer;

	/**
	 * Shader register slot shifts (required to produce spirv from hlsl)
	 * 000-099 is push constant buffer range
	 */
	constexpr uint32_t SHADER_REGISTER_SHIFT_U   = 100;
	constexpr uint32_t SHADER_REGISTER_SHIFT_B   = 200;
	constexpr uint32_t SHADER_REGISTER_SHIFT_T   = 300;
	constexpr uint32_t SHADER_REGISTER_SHIFT_S   = 400;

	/**
	 * @enum BindlessResource
	 * @brief Defines the types of bindless resources that can be used in the rendering system.
	 * @note Must match order of appearance in common_resources.slang
	 */
	enum class BindlessResource
	{
		MaterialTextures,
		MaterialParameters,
		LightParameters,
		Aabbs,
		DrawData,
		SamplersComparison,
		SamplersRegular,
		GeometryVertices,
		GeometryIndices,
		Instances,
		MaxEnum
	};

	/**
	 * @class BindlessManager
	 * @brief Manages bindless resources, including allocation, updating, and descriptor set management for various resource types used in the rendering system.
	 */
	class BindlessManager : public SharedResource
	{
	public:
		BindlessManager();
		~BindlessManager();

		/**
		 * @struct ResourceConfig
		 * @brief ResourceConfig defines the configuration for a bindless resource type, including its descriptor type, register shift, slot, count, and name.
		 */
		struct ResourceConfig
		{
			VkDescriptorType type;   // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, etc.
			uint32_t registerShift;  // shader_register_shift_t or _s
			uint32_t slot;           // hlsl register slot
			uint32_t count;          // descriptor array count
			const char *name;
		};

		/* @brief Initializes the bindless manager, including creating descriptor pools and set layouts for all bindless resource types. */
		static void Init();

		/* @brief Adds the bindless resources to the descriptor pool. */
		static void AddToDescriptorPool();

		/* @brief Shuts down the bindless manager, releasing all resources. */
		static void Shutdown();

		/**
		 * @brief Allocates a bindless resource of the specified type.
		 * @param type The type of bindless resource to allocate.
		 * @return The index of the allocated resource.
		 */
		uint32_t Allocate(BindlessResource type);

		/**
		 * @brief Retrieves the descriptor cache for a given hash.
		 * @param hash The hash value of the descriptor cache to retrieve.
		 */
		static std::vector<Descriptor> GetDescriptorCache(uint64_t hash);

		/**
		 * @brief Frees a previously allocated bindless resource.
		 * @param type The type of bindless resource to free.
		 * @param index The index of the resource to free.
		 */
		void Free(BindlessResource type, uint32_t index);

		/**
		 * @brief Updates the buffer for a given bindless resource type.
		 * @param type The type of bindless resource to update.
		 * @param buffer The buffer to update.
		 */
		static void UpdateBuffer(BindlessResource type, const Buffer *buffer);

		/**
		 * @brief Retrieves the descriptors from the pipeline state.
		 * @param pipelineState The pipeline state to retrieve descriptors from.
		 * @param outDescriptors The array to store the retrieved descriptors.
		 * @param outCount The number of descriptors retrieved.
		 */
		void GetDescriptorsFromPipelineState(PipelineState& pipelineState, Descriptor* outDescriptors, size_t& outCount);

		/**
		 * @brief Updates the samplers for a given bindless resource type.
		 * @param type The type of bindless resource to update.
		 * @param samplers The array of samplers to update.
		 * @param count The number of samplers in the array.
		 */
		static void UpdateSamplers(BindlessResource type, const Ref<Sampler> *samplers, uint32_t count);

		/**
		 * @brief Updates the images for a given bindless resource type.
		 * @param imageArrays The array of image resources to update.
		 */
		static void UpdateImages(const std::array<ImageResource*, MAX_ARRAY_SIZE> *imageArrays);

		/**
		 * @brief Creates a descriptor set layout for a given bindless resource type.
		 * @param type The type of bindless resource for which to create the set layout.
		 */
		static void CreateSetLayout(BindlessResource type);

		/**
		 * @brief Retrieves the binding for a given bindless resource type.
		 * @param type The type of bindless resource for which to retrieve the binding.
		 * @return The binding index of the specified bindless resource type.
		 */
		static uint32_t GetBinding(BindlessResource type);

		/**
		 * @brief Retrieves the descriptor set layout for a given bindless resource type.
		 * @param type The bindless resource type whose layout to retrieve.
		 * @return The VkDescriptorSetLayout for the specified type, or VK_NULL_HANDLE if not yet created.
		 */
		static VkDescriptorSetLayout GetLayoutForType(BindlessResource type);

		/**
		 * @brief Retrieves the descriptor set layout.
		 * @return The descriptor set layout.
		 */
		[[nodiscard]] VkDescriptorSetLayout GetLayout() const { return m_Layout; }

		/**
		 * @brief Retrieves the descriptor set.
		 * @return The descriptor set.
		 */
		[[nodiscard]] VkDescriptorSet GetSet() const { return m_Set; }

		/**
		 * @brief Retrieves the raw VkDescriptorSet for a specific bindless resource type.
		 *
		 * Used by DescriptorSet::SetBindless to bind the per-type bindless sets to the command buffer.
		 *
		 * @param type The bindless resource type whose set to retrieve.
		 * @return The VkDescriptorSet handle, or VK_NULL_HANDLE if not yet allocated.
		 */
		static VkDescriptorSet GetSetForType(BindlessResource type);

		/**
		 * @brief Retrieves the shared descriptor set cache used by all descriptor sets.
		 * @return Reference to the static descriptor set map keyed by binding hash.
		 */
		static std::unordered_map<uint64_t, DescriptorSet>& GetDescriptorSets();

	private:
		/**
		 * @struct ResourceAllocationState
		 * @brief Per-bindless-resource allocation tracking for array-slot management.
		 */
		struct ResourceAllocationState
		{
			uint32_t nextIndex = 0;
			std::vector<uint32_t> freeIndices;
		};

		Ref<Device> m_Device;
		VkDescriptorPool m_Pool = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
		VkDescriptorSet m_Set = VK_NULL_HANDLE;

		std::array<ResourceAllocationState, static_cast<size_t>(BindlessResource::MaxEnum)> m_Resources;
	};


}

// --------------------------------------------------------------
