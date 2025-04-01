/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_pipeline.h
* -------------------------------------------------------
* Created: 30/3/2025
* -------------------------------------------------------
*/
#pragma once

#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <SceneryEditorX/renderer/vk_device.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class vkPhysDevice;

	struct PipelineConfigInfo
	{
        PipelineConfigInfo() = default;
        PipelineConfigInfo(const PipelineConfigInfo &) = delete;
        PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

		uint32_t subpass = 0;
        VkRenderPass renderPass = nullptr;
        VkPipelineLayout pipelineLayout = nullptr;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	    VkPipelineRasterizationStateCreateInfo rasterizationInfo;

		std::vector<VkDynamicState> dynamicStateEnables;
        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
	};

	class GFXPipeline
	{
	public:
        GFXPipeline(vkPhysDevice &device,
					const std::string &vertFilepath,
					const std::string &fragFilepath,
					const PipelineConfigInfo &configInfo);

	    ~GFXPipeline();

        void bind(VkCommandBuffer commandBuffer);

		// -------------------------------------------------------
		// GETTER FUNCTIONS
        // -------------------------------------------------------
		VkPipeline GetGraphicsPipeline() const { return graphicsPipeline; }


	private:
        VkShaderModule createShaderModule(const std::vector<char> &code);

		void createGraphicsPipeline(const std::string &vertFilepath,
                                    const std::string &fragFilepath,
                                    const PipelineConfigInfo &configInfo);

        vkPhysDevice &device;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        VkShaderModule vertShaderModule = VK_NULL_HANDLE;
        VkShaderModule fragShaderModule = VK_NULL_HANDLE;
	};
} // namespace SceneryEditorX

// -------------------------------------------------------
