/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* renderer.h
* -------------------------------------------------------
* Created: 7/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <cstdint>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/core/threading/thread_manager.h>
#include <SceneryEditorX/platform/platform_states.h>
#include <SceneryEditorX/renderer/command_queue.h>
#include <SceneryEditorX/renderer/render_data.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward declaration to break circular dependency
    class RenderContext;

    /// -------------------------------------------------------

    class Renderer
	{
	public:
        typedef void (*RenderCommandFn)(void *);

		GLOBAL Ref<RenderContext> GetContext();

        GLOBAL void Init();
		GLOBAL void Shutdown();
        GLOBAL void BeginFrame();
        GLOBAL void EndFrame();
        GLOBAL void SubmitFrame();

        /// -------------------------------------------------------

        template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				/**
				 * @note: Instead of destroying we could try and enforce all items to be trivally destructible
				 * however some items like uniforms which contain std::strings still exist for now.
				 * static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
 				 */
				pFunc->~FuncT();
			};
			auto storageBuffer = GetCommandQueue().Allocate(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
		}

		template<typename FuncT>
		static void SubmitResourceFree(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				/**
				 * @note: Instead of destroying we could try and enforce all items to be trivally destructible
				 * however some items like uniforms which contain std::strings still exist for now.
				 * static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				 */
				pFunc->~FuncT();
			};

			if (ThreadManager::checkRenderThread())
			{
				const uint32_t index = GetCurrentFrameIndex();
				auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
				new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
			}
			else
			{
				const uint32_t index = GetCurrentFrameIndex();
				Submit([renderCmd, func, index]()
				{
					auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
					new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
				});
			}
		}

		//GLOBAL Ref<ShaderLibrary> GetShaderLibrary();
        GLOBAL CommandQueue &GetRenderResourceReleaseQueue(uint32_t index);
        GLOBAL uint32_t GetRenderQueueIndex();
        GLOBAL uint32_t GetRenderQueueSubmissionIndex();
        GLOBAL uint32_t GetCurrentFrameIndex();
        GLOBAL RenderData &GetRenderData();
        GLOBAL void SetRenderData(const RenderData &renderData);
        GLOBAL void RenderThreadFunc(ThreadManager *renderThread);
        GLOBAL void WaitAndRender(ThreadManager* renderThread);
		GLOBAL void SwapQueues();
        GLOBAL uint32_t GetCurrentRenderThreadFrameIndex();
        GLOBAL uint32_t GetDescriptorAllocationCount(uint32_t frameIndex = 0);
        VkSampler CreateSampler(VkSamplerCreateInfo samplerCreateInfo);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Render Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //GLOBAL void BeginRenderPass(Ref<CommandBuffer> CommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false);
		//GLOBAL void EndRenderPass(Ref<CommandBuffer> CommandBuffer);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Compute Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //GLOBAL void BeginComputePass(Ref<CommandBuffer> CommandBuffer, Ref<ComputePass> computePass);
		//GLOBAL void EndComputePass(Ref<CommandBuffer> CommandBuffer, Ref<ComputePass> computePass);
		//GLOBAL void DispatchCompute(Ref<CommandBuffer> CommandBuffer, Ref<ComputePass> computePass, Ref<Material> material, const glm::uvec3& workGroups, Buffer constants = Buffer());

		//GLOBAL void ClearImage(Ref<CommandBuffer> CommandBuffer, Ref<Image2D> image, const ImageClearValue& clearValue, ImageSubresourceRange subresourceRange = ImageSubresourceRange());
        //GLOBAL void CopyImage(Ref<CommandBuffer> CommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);

        //GLOBAL CommandQueue &GetRenderResourceReleaseQueue(uint32_t index);
    private:
        INTERNAL CommandQueue &GetCommandQueue();
        INTERNAL CommandQueue resourceFreeQueue[3];

	};

}

/// -------------------------------------------------------
