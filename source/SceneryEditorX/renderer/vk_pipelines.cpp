/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_pipelines.cpp
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/platform/windows/editor_config.hpp>
#include <SceneryEditorX/platform/windows/file_manager.hpp>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_pipelines.h>
#include <SceneryEditorX/scene/model.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	void Pipeline::CreatePipeline()
	{
		// Get editor configuration
        EditorConfig config;

        std::string shaderPath(config.shaderFolder);

        Ref<Shader> vertShader = Shader::CreateFromString(shaderPath + "/vert.spv");
        Ref<Shader> fragShader = Shader::CreateFromString(shaderPath + "/frag.spv");

        std::string vertShaderPath = vertShader->GetName();
        std::string fragShaderPath = fragShader->GetName();

        SEDX_CORE_INFO("Loading vertex shader from: {}", vertShaderPath);
        SEDX_CORE_INFO("Loading fragment shader from: {}", fragShaderPath);

		// -------------------------------------------------------

		auto vertShaderCode = IO::FileManager::ReadShaders(vertShaderPath);
		auto fragShaderCode = IO::FileManager::ReadShaders(fragShaderPath);

		VkShaderModule vertShaderModule = vkShaderPtr->CreateShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = vkShaderPtr->CreateShaderModule(fragShaderCode);

		// -------------------------------------------------------

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

		// -------------------------------------------------------

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		// -------------------------------------------------------

		// Configure vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// -------------------------------------------------------

		// Configure input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

		// -------------------------------------------------------
        // Configure viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = vkSwapChain->GetSwapExtent().width;
        viewport.height = vkSwapChain->GetSwapExtent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {.x = 0,.y = 0};
        scissor.extent = vkSwapChain->GetSwapExtent();

		// -------------------------------------------------------

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // Configure rasterization
        VkPipelineRasterizationStateCreateInfo rasterize{};
        rasterize.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterize.depthClampEnable = VK_FALSE;
        rasterize.rasterizerDiscardEnable = VK_FALSE;
        rasterize.polygonMode = VK_POLYGON_MODE_FILL;
        rasterize.lineWidth = 1.0f;
        rasterize.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterize.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Vertex winding order CCW(VK_FRONT_FACE_COUNTER_CLOCKWISE) or CW(VK_FRONT_FACE_CLOCKWISE)
        rasterize.depthBiasEnable = VK_FALSE;

		// -------------------------------------------------------

        // Configure multisampling
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

		// -------------------------------------------------------

		std::vector<VkDescriptorSetLayout> layouts;

        // Create the graphics pipeline
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = layouts.data();

        if (vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create pipeline layout!");
            ErrMsg("Failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterize;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = VK_NULL_HANDLE;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create graphics pipeline!");
            ErrMsg("Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device->GetDevice(), fragShaderModule, nullptr);
        vkDestroyShaderModule(device->GetDevice(), vertShaderModule, nullptr);
	}

	Pipeline::~Pipeline()
    {
        vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
        vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);
        pipeline = VK_NULL_HANDLE;
        pipelineLayout = VK_NULL_HANDLE;
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
