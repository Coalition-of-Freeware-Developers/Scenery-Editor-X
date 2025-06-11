/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_pipelines.h
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <GraphicsEngine/vulkan/resource.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class SwapChain;
	struct Viewport;

    /// -------------------------------------------------------

	class Pipeline : public RefCounted
	{
	public:
	    Pipeline();
        virtual ~Pipeline() override;

	    /**
		 * @enum PipelineStage
		 * @brief Represents the various stages of a Vulkan pipeline.
		 *
		 * This enum is used to specify the stages of the pipeline
		 * for synchronization purposes, such as when waiting for
		 * operations to complete or when setting up barriers.
		 *
		 * @note The values in this enum are bitwise OR'd together
		 * to create a bitfield representing multiple stages.
		 * @note Identical to Vulkan's VkPipelineStageFlagBits
		 */
        enum class PipelineStage
        {
            None = 0,
            TopOfPipe = 0x00000001,
            DrawIndirect = 0x00000002,
            VertexInput = 0x00000004,
            VertexShader = 0x00000008,
            TesselationControlShader = 0x00000010,
            TesselationEvaluationShader = 0x00000020,
            GeometryShader = 0x00000040,
            FragmentShader = 0x00000080,
            EarlyFragmentTests = 0x00000100,
            LateFragmentTests = 0x00000200,
            ColorAttachmentOutput = 0x00000400,
            ComputeShader = 0x00000800,
            Transfer = 0x00001000,
            BottomOfPipe = 0x00002000,
            Host = 0x00004000,
            AllGraphics = 0x00008000,
            AllCommands = 0x00010000
        };

		struct PipelineResource : Resource
        {
            VkPipeline pipeline;
            VkPipelineLayout layout;

            virtual ~PipelineResource() override
            {
                Pipeline pipelineInstance;
                pipelineInstance.Destroy();
            }
        };

	    void Create();
        void Destroy();

        VkExtent2D GetFloatSwapExtent() const;
        VkPipeline GetPipeline() const { return pipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() const { return pipelineLayout; }

	private:
        Viewport *vkViewport = nullptr;
        Ref<SwapChain> vkSwapChain;
        Ref<VulkanDevice> device;

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipelineCache pipelineCache = VK_NULL_HANDLE;
	};

} // namespace SceneryEditorX

/// -------------------------------------------------------
