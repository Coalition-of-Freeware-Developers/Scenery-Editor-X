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
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/renderer/render_context.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class Renderer
	{
	public:
        typedef void (*RenderCommandFn)(void *);

        GLOBAL Ref<RenderContext> GetContext()
        {
            return Application::Get().GetWindow().GetRenderContext();
        }

        GLOBAL void Init();
		GLOBAL void Shutdown();

        static void BeginFrame();
        static void EndFrame();

        static uint32_t GetRenderQueueIndex();
        static uint32_t GetRenderQueueSubmissionIndex();
        static uint32_t GetCurrentFrameIndex();

        //static void WaitAndRender(RenderThread* renderThread);
		//static void SwapQueues();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Render Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //static void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false);
		//static void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Compute Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //static void BeginComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass);
		//static void EndComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass);
		//static void DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<Material> material, const glm::uvec3& workGroups, Buffer constants = Buffer());

		//static void ClearImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, const ImageClearValue& clearValue, ImageSubresourceRange subresourceRange = ImageSubresourceRange());
        //static void CopyImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);

        //static RenderCommandQueue &GetRenderResourceReleaseQueue(uint32_t index);
    private:
        //static RenderCommandQueue &GetRenderCommandQueue();
	};

}

/// -------------------------------------------------------
