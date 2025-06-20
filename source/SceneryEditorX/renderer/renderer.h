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
#include <SceneryEditorX/platform/platform_states.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    // Forward declaration to break circular dependency
    class RenderContext;

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

		//GLOBAL Ref<ShaderLibrary> GetShaderLibrary();

        GLOBAL uint32_t GetRenderQueueIndex();
        GLOBAL uint32_t GetRenderQueueSubmissionIndex();
        GLOBAL uint32_t GetCurrentFrameIndex();

        //GLOBAL void WaitAndRender(RenderThread* renderThread);
		//GLOBAL void SwapQueues();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Render Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //GLOBAL void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false);
		//GLOBAL void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Compute Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //GLOBAL void BeginComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass);
		//GLOBAL void EndComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass);
		//GLOBAL void DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<Material> material, const glm::uvec3& workGroups, Buffer constants = Buffer());

		//GLOBAL void ClearImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, const ImageClearValue& clearValue, ImageSubresourceRange subresourceRange = ImageSubresourceRange());
        //GLOBAL void CopyImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);

        //GLOBAL RenderCommandQueue &GetRenderResourceReleaseQueue(uint32_t index);
    private:
        //INTERNAL RenderCommandQueue &GetRenderCommandQueue();
	};

}

/// -------------------------------------------------------
