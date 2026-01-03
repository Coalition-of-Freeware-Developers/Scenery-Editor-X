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
#include "texture.h"
#include "shaders/shader.h"
#include "enums.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	// Forward declarations
	class CommandBuffer;

	class ComputePipeline : public RefCounted
	{
	public:
		ComputePipeline(Ref<Shader> computeShader);

		void Execute(VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		void Begin(Ref<CommandBuffer> commandBuffer = nullptr);
		void Begin_RenderThread(Ref<CommandBuffer> commandBuffer = nullptr);
		void Dispatch(const UVec3& workGroups) const;
		void End();

		[[nodiscard]] Ref<Shader> GetShader() const { return m_Shader; }
		[[nodiscard]] VkCommandBuffer GetActiveCommandBuffer() { return m_ActiveComputeCommandBuffer; }
		[[nodiscard]] VkPipelineLayout GetLayout() const { return m_ComputePipelineLayout; }

		void SetPushConstants(const Buffer& constants) const;
		void CreatePipeline();

		void BufferMemoryBarrier(Ref<CommandBuffer> commandBuffer, Ref<StorageBuffer> storageBuffer, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess);
		void BufferMemoryBarrier(Ref<CommandBuffer> commandBuffer, Ref<StorageBuffer> storageBuffer, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess);
		void ImageMemoryBarrier(Ref<CommandBuffer> commandBuffer, Ref<Image2D> image, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess);
		void ImageMemoryBarrier(Ref<CommandBuffer> commandBuffer, Ref<Image2D> image, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess);
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

// -------------------------------------------------------
