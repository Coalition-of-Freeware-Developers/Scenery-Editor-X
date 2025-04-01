/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_pipeline.cpp
* -------------------------------------------------------
* Created: 30/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/vk_pipeline.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <fstream>
#include <SceneryEditorX/platform/windows/editor_config.hpp>
#include <SceneryEditorX/platform/windows/file_manager.hpp>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	GFXPipeline::GFXPipeline(vkPhysDevice& deviceData,
							 const std::string &vertFilepath,
							 const std::string &fragFilepath,
							 const PipelineConfigInfo &configInfo) : device{deviceData}
	{
        createGraphicsPipeline(vertFilepath, fragFilepath, configInfo);
	}

    GFXPipeline::~GFXPipeline()
    {
        vkDestroyShaderModule(device.GetDevice(), vertShaderModule, nullptr);
        vkDestroyShaderModule(device.GetDevice(), fragShaderModule, nullptr);
        vkDestroyPipeline(device.GetDevice(), graphicsPipeline, nullptr);
    }

	VkShaderModule GFXPipeline::createShaderModule(const std::vector<char> &code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device.GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create shader module!");
            ErrMsg("failed to create shader module!");
        }

        return shaderModule;
    }

    void GFXPipeline::createGraphicsPipeline(const std::string &vertFilepath,
                                             const std::string &fragFilepath,
                                             const PipelineConfigInfo &configInfo)
    {
		// Get editor configuration
        EditorConfig config;
        FileManager files;
        VkExtent2D swapChainExtent = device.GetSwapChainExtent();

		// If the extent is empty (0,0), use some reasonable defaults
        if (swapChainExtent.width == 0 || swapChainExtent.height == 0)
        {
            swapChainExtent = {800, 600}; // Default values
            EDITOR_LOG_WARN("Using default swap chain extent: {}x{}",
							swapChainExtent.width,
							swapChainExtent.height);
        }

		// -------------------------------------------------------

        std::string shaderPath(config.shaderFolder.data());
        // Use the provided file paths if they're not empty, otherwise use defaults
        std::string vertShaderPath = !vertFilepath.empty() ? vertFilepath : (shaderPath + "/vert.spv");
        std::string fragShaderPath = !fragFilepath.empty() ? fragFilepath : (shaderPath + "/frag.spv");

        EDITOR_LOG_INFO("Loading vertex shader from: {}", vertShaderPath);
        EDITOR_LOG_INFO("Loading fragment shader from: {}", fragShaderPath);

        // -------------------------------------------------------

        auto vertShaderCode = files.readShaderFile(vertShaderPath);
        auto fragShaderCode = files.readShaderFile(fragShaderPath);

        vertShaderModule = createShaderModule(vertShaderCode);
        fragShaderModule = createShaderModule(fragShaderCode);

        // -------------------------------------------------------

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        vertShaderStageInfo.flags = 0;
        vertShaderStageInfo.pNext = nullptr;
        vertShaderStageInfo.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        fragShaderStageInfo.flags = 0;
        fragShaderStageInfo.pNext = nullptr;
        fragShaderStageInfo.pSpecializationInfo = nullptr;

        // -------------------------------------------------------

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // -------------------------------------------------------

        // Configure vertex input
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(configInfo.bindingDescriptions.size());
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(configInfo.attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = configInfo.bindingDescriptions.data();
        vertexInputInfo.pVertexAttributeDescriptions = configInfo.attributeDescriptions.data();

        // -------------------------------------------------------

		// Create the graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
        pipelineInfo.pViewportState = &configInfo.viewportInfo;
        pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
        pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
        pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
        pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
        pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;
        pipelineInfo.layout = configInfo.pipelineLayout;
        pipelineInfo.renderPass = configInfo.renderPass;
        pipelineInfo.subpass = configInfo.subpass;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create graphics pipeline!");
            ErrMsg("Failed to create graphics pipeline!");
        }

        EDITOR_LOG_INFO("Graphics pipeline created successfully");

        // Configure input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // -------------------------------------------------------
        // Configure viewport and scissor
        // -------------------------------------------------------

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        // -------------------------------------------------------

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

		// -------------------------------------------------------
        // Configure rasterization
        // -------------------------------------------------------

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Vertex winding order CCW(VK_FRONT_FACE_COUNTER_CLOCKWISE) or CW(VK_FRONT_FACE_CLOCKWISE)
        rasterizer.depthBiasEnable = VK_FALSE;

        // -------------------------------------------------------
        // Configure multisampling
        // -------------------------------------------------------

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // -------------------------------------------------------

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        // -------------------------------------------------------
        // Configure color blending
        // -------------------------------------------------------

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;


    }

	// -------------------------------------------------------

	void GFXPipeline::bind(VkCommandBuffer commandBuffer)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    }

} // namespace SceneryEditorX

// ---------------------------------------------------------
