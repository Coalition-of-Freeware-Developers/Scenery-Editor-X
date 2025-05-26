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
#include <SceneryEditorX/platform/file_manager.hpp>
#include <GraphicsEngine/vulkan/vk_buffers.h>
#include <GraphicsEngine/vulkan/vk_descriptors.h>
#include <GraphicsEngine/vulkan/vk_pipelines.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	void Pipeline::CreatePipeline()
	{
		/// Get editor configuration
        EditorConfig config;

        std::string shaderPath(config.shaderFolder);

        Ref<Shader> vertShader = Shader::CreateFromString(shaderPath + "/vert.spv");
        Ref<Shader> fragShader = Shader::CreateFromString(shaderPath + "/frag.spv");

        std::string vertShaderPath = vertShader->GetName();
        std::string fragShaderPath = fragShader->GetName();

        SEDX_CORE_INFO("Loading vertex shader from: {}", vertShaderPath);
        SEDX_CORE_INFO("Loading fragment shader from: {}", fragShaderPath);

		/// -------------------------------------------------------

		auto vertShaderCode = IO::FileManager::ReadShaders(vertShaderPath);
		auto fragShaderCode = IO::FileManager::ReadShaders(fragShaderPath);

        /// Store the shader so we can access it later with GetShader()
        shaderPtr = vertShader;

		VkShaderModule vertShaderModule = CreateShaderModule(device->GetDevice(), vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(device->GetDevice(), fragShaderCode);

		/// -------------------------------------------------------

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

		/// -------------------------------------------------------

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		/// -------------------------------------------------------

		/// Configure vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = MeshVertex::getBindingDescription();
        auto attributeDescriptions = MeshVertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		/// -------------------------------------------------------

		/// Configure input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

		/// -------------------------------------------------------

	    /// Configure viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(vkSwapChain->GetSwapExtent().width);
        viewport.height = static_cast<float>(vkSwapChain->GetSwapExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = vkSwapChain->GetSwapExtent();

		/// -------------------------------------------------------

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        /// Configure rasterization
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; /// Vertex winding order CCW(VK_FRONT_FACE_COUNTER_CLOCKWISE) or CW(VK_FRONT_FACE_CLOCKWISE)
        rasterizer.depthBiasEnable = VK_FALSE;

		/// -------------------------------------------------------

        /// Configure multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		/// -------------------------------------------------------

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

		/// -------------------------------------------------------

        /// Configure color blending

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

		/// -------------------------------------------------------

		// Set up descriptor set layouts
        std::vector<VkDescriptorSetLayout> layouts;
        
        // Get the bindless descriptor set layout from the device
        if (device->GetBindlessResources().bindlessDescriptorSetLayout != VK_NULL_HANDLE) {
            layouts.push_back(device->GetBindlessResources().bindlessDescriptorSetLayout);
        }

        /// Create the graphics pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
        pipelineLayoutInfo.pSetLayouts = layouts.empty() ? nullptr : layouts.data();

        // Create the pipeline layout
        if (vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            SEDX_CORE_ERROR("Failed to create pipeline layout!");
        }

        // Get the render pass from the swap chain
        VkRenderPass renderPass = vkSwapChain->GetRenderPass();
        
        // Complete pipeline creation info
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        // Create the graphics pipeline
        if (vkCreateGraphicsPipelines(device->GetDevice(), pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
            SEDX_CORE_ERROR("Failed to create graphics pipeline!");
        }

        // Clean up shader modules
        vkDestroyShaderModule(device->GetDevice(), fragShaderModule, nullptr);
        vkDestroyShaderModule(device->GetDevice(), vertShaderModule, nullptr);
	}

    /// Helper function to create a shader module from shader code
    VkShaderModule Pipeline::CreateShaderModule(VkDevice device, const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            SEDX_CORE_ERROR("Failed to create shader module!");
            return VK_NULL_HANDLE;
        }
        
        return shaderModule;
    }

    /// -------------------------------------------------------

    VkExtent2D Pipeline::GetFloatSwapExtent() const {
        VkExtent2D extent = vkSwapChain->GetSwapExtent();
        return extent;
    }

	Pipeline::~Pipeline()
    {
        if (device) {
            if (pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
            }
            
            if (pipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
            }
            
            if (pipelineCache != VK_NULL_HANDLE) {
                vkDestroyPipelineCache(device->GetDevice(), pipelineCache, nullptr);
                pipelineCache = VK_NULL_HANDLE;
            }
        }
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
