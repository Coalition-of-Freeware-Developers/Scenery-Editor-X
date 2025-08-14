/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* compute_pipeline.h
* -------------------------------------------------------
* Created: 25/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/shaders/shader.h>
#include <SceneryEditorX/renderer/texture.h>
#include <SceneryEditorX/utils/vulkan/vk_includes.h>
#include "vulkan/vk_enums.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	// Forward declarations
	class CommandBuffer;

	class ComputePipeline : public RefCounted
	{
	public:
		ComputePipeline(const Ref<Shader>& computeShader);

		void Execute(const VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		virtual void Begin(Ref<CommandBuffer> commandBuffer = nullptr);
		virtual void Begin_RenderThread(Ref<CommandBuffer> commandBuffer = nullptr);
		void Dispatch(const UVec3& workGroups) const;
		virtual void End();

		[[nodiscard]] virtual Ref<Shader> GetShader() const { return m_Shader; }

		[[nodiscard]] VkCommandBuffer GetActiveCommandBuffer() const { return m_ActiveComputeCommandBuffer; }
		[[nodiscard]] VkPipelineLayout GetLayout() const { return m_ComputePipelineLayout; }

		void SetPushConstants(const Buffer& constants) const;
		void CreatePipeline();

		virtual void BufferMemoryBarrier(Ref<CommandBuffer> commandBuffer, Ref<StorageBuffer> storageBuffer, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess);
		virtual void BufferMemoryBarrier(Ref<CommandBuffer> commandBuffer, Ref<StorageBuffer> storageBuffer, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess);
		virtual void ImageMemoryBarrier(Ref<CommandBuffer> commandBuffer, Ref<Image2D> image, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess);
		virtual void ImageMemoryBarrier(Ref<CommandBuffer> commandBuffer, Ref<Image2D> image, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess);
	private:
		void CreateRenderThreadPipeline();

		Ref<Shader> m_Shader;
		VkPipelineLayout m_ComputePipelineLayout = nullptr;
		VkPipelineCache m_PipelineCache = nullptr;
		VkPipeline m_ComputePipeline = nullptr;
		VkCommandBuffer m_ActiveComputeCommandBuffer = nullptr;
		bool m_UsingGraphicsQueue = false;
	};

}

/// -------------------------------------------------------
