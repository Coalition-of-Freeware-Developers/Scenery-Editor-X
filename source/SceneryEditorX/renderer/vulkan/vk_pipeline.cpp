/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_pipeline.cpp
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/vulkan/vk_pipeline.h>
#include <SceneryEditorX/platform/file_manager.hpp>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	Pipeline::Pipeline()
	{
		//SEDX_CORE_ASSERT()
        Create();
	}

    Pipeline::~Pipeline()
    {
        /*
        if (device)
        {
            if (pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
            }

            if (pipelineLayout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
            }

            if (pipelineCache != VK_NULL_HANDLE)
            {
                vkDestroyPipelineCache(device->GetDevice(), pipelineCache, nullptr);
                pipelineCache = VK_NULL_HANDLE;
            }
        }
        */

        const auto device = RenderContext::GetCurrentDevice()->GetDevice();
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineCache(device, pipelineCache, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }

    void Pipeline::Create()
	{
        Pipeline &pipeline = *this; /// Reference to the current pipeline instance
        auto device = RenderContext::GetCurrentDevice()->GetDevice();
        auto ctx = RenderContext::Get();
	    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(stages.size());
        std::vector<VkShaderModule> shaderModules(stages.size());
        for (int i = 0; i < stages.size(); i++)
        {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = pipeline.stageBytes[i].size();
            createInfo.pCode = (const uint32_t *)(pipeline.stageBytes[i].data());
            auto result = vkCreateShaderModule(device, &createInfo, ctx->allocatorCallback, &shaderModules[i]);
            SEDX_ASSERT(result, "Failed to create shader module!");
            shaderStages[i] = {};
            shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i].stage = (VkShaderStageFlagBits)stages[i].stage;
            shaderStages[i].module = shaderModules[i];
            /// function to invoke inside the shader module it's possible to combine multiple shaders into a single module using different entry points.
            shaderStages[i].pName = pipeline.stages[i].entryPoint.c_str();
            /// this allows us to specify values for shader constants
            shaderStages[i].pSpecializationInfo = nullptr;
        }

        BindlessResources bindlessResource;
        std::vector<VkDescriptorSetLayout> layouts;
        layouts.push_back(bindlessResource.bindlessDescriptorLayout);

	    VkPushConstantRange pushConstant{};
        pushConstant.offset = 0;
        pushConstant.size = 256;
        pushConstant.stageFlags = VK_SHADER_STAGE_ALL;

	    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

        auto result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        SEDX_ASSERT(result == VK_SUCCESS, "Failed to create pipeline layout!");

        if (point == PipelineType::Compute)
        {
            SEDX_ASSERT(shaderStages.size() == 1, "Compute pipeline only support 1 stage.");

            VkComputePipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.stage = shaderStages[0];
            pipelineInfo.layout = pipeline.resource->layout;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.basePipelineIndex = -1;
            pipelineInfo.pNext = VK_NULL_HANDLE;
            result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, ctx->allocatorCallback, &pipeline.resource->pipeline);

            SEDX_ASSERT(result, "Failed to create compute pipeline!");
        }
        else
        {
            VkPipelineRasterizationStateCreateInfo rasterizer = {};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            /// fragments beyond near and far planes are clamped to them
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            /// line thickness in terms of number of fragments
            rasterizer.lineWidth = 1.0f;
            if (cullFront)
                rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
            else
                rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f;
            rasterizer.depthBiasClamp = 0.0f;
            rasterizer.depthBiasSlopeFactor = 0.0f;

            VkPipelineMultisampleStateCreateInfo multisampling = {};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.minSampleShading = 0.5f;
            multisampling.pSampleMask = nullptr;

            VkPipelineDepthStencilStateCreateInfo depthStencil = {};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.minDepthBounds = 0.0f;
            depthStencil.maxDepthBounds = 1.0f;
            depthStencil.stencilTestEnable = VK_FALSE;
            depthStencil.front = {};
            depthStencil.back = {};

            std::vector<VkVertexInputAttributeDescription> attributeDescs(vertexAttributes.size());
            uint32_t attributeSize = 0;
            for (int i = 0; i < vertexAttributes.size(); i++)
            {
                attributeDescs[i].binding = 0;
                attributeDescs[i].location = i;
                attributeDescs[i].format = (VkFormat)vertexAttributes[i];
                attributeDescs[i].offset = attributeSize;
                if (vertexAttributes[i] == ImageFormat::RG32_sfloat)
                {
                    attributeSize += 2 * sizeof(float);
                }
                else if (vertexAttributes[i] == ImageFormat::RGB32_sfloat)
                {
                    attributeSize += 3 * sizeof(float);
                }
                else if (vertexAttributes[i] == ImageFormat::RGBA32_sfloat)
                {
                    attributeSize += 4 * sizeof(float);
                }
                else
                {
                    SEDX_ASSERT(false, "Invalid Vertex Attribute");
                }
            }

            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = attributeSize;
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)(attributeDescs.size());
            /// these points to an array of structs that describe how to load the vertex data
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

            std::vector<VkDynamicState> dynamicStates;
            dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
            dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

            /// define the type of input of our pipeline
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            if (lineTopology)
            {
                inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
            }
            else
                inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            /// with this parameter true we can break up lines and triangles in _STRIP topology modes
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkPipelineDynamicStateCreateInfo dynamicCreate = {};
            dynamicCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicCreate.pDynamicStates = dynamicStates.data();
            dynamicCreate.dynamicStateCount = dynamicStates.size();

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = nullptr;
            viewportState.scissorCount = 1;
            viewportState.pScissors = nullptr;

            VkPipelineRenderingCreateInfoKHR pipelineRendering{};
            pipelineRendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
            pipelineRendering.colorAttachmentCount = colorFormats.size();
            pipelineRendering.pColorAttachmentFormats = (VkFormat *)colorFormats.data();
            pipelineRendering.depthAttachmentFormat = useDepth ? (VkFormat)depthFormat : VK_FORMAT_UNDEFINED;
            pipelineRendering.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
            pipelineRendering.viewMask = 0;

            std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(colorFormats.size());
            for (int i = 0; i < colorFormats.size(); i++)
            {
                blendAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
                blendAttachments[i].colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
                blendAttachments[i].colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
                blendAttachments[i].colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
                blendAttachments[i].blendEnable = VK_FALSE;
            }

            VkPipelineColorBlendStateCreateInfo colorBlendState{};
            colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendState.logicOpEnable = VK_FALSE;
            colorBlendState.logicOp = VK_LOGIC_OP_COPY;
            colorBlendState.attachmentCount = blendAttachments.size();
            colorBlendState.pAttachments = blendAttachments.data();
            colorBlendState.blendConstants[0] = 0.0f;
            colorBlendState.blendConstants[1] = 0.0f;
            colorBlendState.blendConstants[2] = 0.0f;
            colorBlendState.blendConstants[3] = 0.0f;

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = shaderStages.size();
            pipelineInfo.pStages = shaderStages.data();
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.pColorBlendState = &colorBlendState;
            pipelineInfo.pDynamicState = &dynamicCreate;
            pipelineInfo.layout = pipeline.resource->layout;
            pipelineInfo.renderPass = VK_NULL_HANDLE;
            /// pipelineInfo.renderPass = SwapChain::GetRenderPass();
            pipelineInfo.subpass = 0;
            /// if we were creating this pipeline by deriving it from another we should specify here
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.basePipelineIndex = -1;
            pipelineInfo.pNext = &pipelineRendering;

            result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, ctx->allocatorCallback, &pipeline.resource->pipeline);
            SEDX_ASSERT(result, "Failed to create graphics pipeline!");
        }

        for (auto &shaderModule : shaderModules)
            vkDestroyShaderModule(device, shaderModule, ctx->allocatorCallback);

        /*{

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

            VkShaderModule vertShaderModule = shaderPtr->CreateShaderModule(vertShaderCode);
            VkShaderModule fragShaderModule = shaderPtr->CreateShaderModule(fragShaderCode);

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
            scissor.offset = {.x = 0,.y = 0};
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

            ///< Configure color blending
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

            /// Set up descriptor set layouts
            std::vector<VkDescriptorSetLayout> layouts;

            /*
        /// Get the bindless descriptor set layout from the device
        if (device->GetBindlessResources().bindlessDescriptorSetLayout != VK_NULL_HANDLE)
            layouts.push_back(device->GetBindlessResources().bindlessDescriptorSetLayout);
        #1#

            /// Create the graphics pipeline layout
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
            pipelineLayoutInfo.pSetLayouts = layouts.empty() ? nullptr : layouts.data();

            /// Create the pipeline layout
            if (vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
                SEDX_CORE_ERROR_TAG("Graphics Pipeline", "Failed to create pipeline layout!");

            /// Get the render pass from the swap chain
            VkRenderPass renderPass = vkSwapChain->GetRenderPass();
        
            /// Complete pipeline creation info
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

            /// Create the graphics pipeline
            if (vkCreateGraphicsPipelines(device->GetDevice(), pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
                SEDX_CORE_ERROR("Failed to create graphics pipeline!");

            /// Clean up shader modules
            shaderPtr.Reset();
            //vkDestroyShaderModule(device->GetDevice(), fragShaderModule, nullptr);
            //vkDestroyShaderModule(device->GetDevice(), vertShaderModule, nullptr);
        }*/
	}

    // Changed to utilize Shader class for shader module creation
    /*
    /// Helper function to create a shader module from shader code
    VkShaderModule Pipeline::CreateShaderModule(VkDevice device, const std::vector<char>& code)
{
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
            SEDX_CORE_ERROR("Failed to create shader module!");
            return VK_NULL_HANDLE;
        }
        
        return shaderModule;
    }
    */

    /// -------------------------------------------------------

    VkExtent2D Pipeline::GetFloatSwapExtent() const
    {
        auto extent = vkSwapChain->GetSwapExtent();
        return extent;
    }

    /*
	bool Pipeline::dynamicLineWidth() const
    {
        return m_Specification.Topology == PrimitiveTopology::Lines ||
               m_Specification.Topology == PrimitiveTopology::LineStrip || m_Specification.Wireframe;
    }
	*/

} // namespace SceneryEditorX

/// -------------------------------------------------------
