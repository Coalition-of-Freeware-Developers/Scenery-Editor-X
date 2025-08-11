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
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_descriptors.h>
#include <SceneryEditorX/renderer/vulkan/vk_pipeline.h>

#include "vk_util.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

    Pipeline::Pipeline(PipelineData &data) : pipelineSpecs(data)
    {
        SEDX_CORE_ASSERT(data.shader);
        SEDX_CORE_ASSERT(data.dstFramebuffer);
        Pipeline::Invalidate();
        //Renderer::RegisterShaderDependency(data.shader, this);
    }

    Pipeline::~Pipeline()
    {
        /*
        if (VkDevice device = RenderContext::GetCurrentDevice()->GetDevice())
        {
            if (pipeline != nullptr)
            {
                vkDestroyPipeline(device, pipeline, nullptr);
                pipeline = nullptr;
            }

            if (pipelineLayout != nullptr)
            {
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                pipelineLayout = nullptr;
            }

            if (pipelineCache != nullptr)
            {
                vkDestroyPipelineCache(device, pipelineCache, nullptr);
                pipelineCache = nullptr;
            }
        }
        */
        Renderer::SubmitResourceFree(
            [pipeline = pipeline, pipelineCache = pipelineCache, pipelineLayout = pipelineLayout]() {
                const auto vulkanDevice = RenderContext::GetCurrentDevice()->GetDevice();
                vkDestroyPipeline(vulkanDevice, pipeline, nullptr);
                vkDestroyPipelineCache(vulkanDevice, pipelineCache, nullptr);
                vkDestroyPipelineLayout(vulkanDevice, pipelineLayout, nullptr);
            });

        /*
        const auto device = RenderContext::GetCurrentDevice()->GetDevice();
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineCache(device, pipelineCache, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        */

    }

    /*
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

            std::vector<VkVertexInputAttributeDescription> attributeDesc(vertexAttributes.size());
            uint32_t attributeSize = 0;
            for (int i = 0; i < vertexAttributes.size(); i++)
            {
                attributeDesc[i].binding = 0;
                attributeDesc[i].location = i;
                attributeDesc[i].format = (VkFormat)vertexAttributes[i];
                attributeDesc[i].offset = attributeSize;
                if (vertexAttributes[i] == VkFormat::VK_FORMAT_R32G32_SFLOAT)
                    attributeSize += 2 * sizeof(float);
                else if (vertexAttributes[i] == VkFormat::VK_FORMAT_R32G32_SFLOAT)
                    attributeSize += 3 * sizeof(float);
                else if (vertexAttributes[i] == VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT)
                    attributeSize += 4 * sizeof(float);
                else
                    SEDX_ASSERT(false, "Invalid Vertex Attribute");
            }

            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = attributeSize;
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)(attributeDesc.size());
            /// these points to an array of structs that describe how to load the vertex data
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.pVertexAttributeDescriptions = attributeDesc.data();

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
            pipelineInfo.renderPass = nullptr;
            /// pipelineInfo.renderPass = SwapChain::GetRenderPass();
            pipelineInfo.subpass = 0;
            /// if we were creating this pipeline by deriving it from another we should specify here
            pipelineInfo.basePipelineHandle = nullptr;
            pipelineInfo.basePipelineIndex = -1;
            pipelineInfo.pNext = &pipelineRendering;

            result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, ctx->allocatorCallback, &pipeline.resource->pipeline);
            SEDX_ASSERT(result, "Failed to create graphics pipeline!");
        }

        for (auto &shaderModule : shaderModules)
            vkDestroyShaderModule(device, shaderModule, ctx->allocatorCallback);

        {

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


			/// Get the bindless descriptor set layout from the device
			if (device->GetBindlessResources().bindlessDescriptorSetLayout != VK_NULL_HANDLE)
			    layouts.push_back(device->GetBindlessResources().bindlessDescriptorSetLayout);

            /// Create the graphics pipeline layout
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
            pipelineLayoutInfo.pSetLayouts = layouts.empty() ? nullptr : layouts.data();

            /// Create the pipeline layout
            if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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
            if (vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline.pipeline) != VK_SUCCESS)
                SEDX_CORE_ERROR("Failed to create graphics pipeline!");

            /// Clean up shader modules
            shaderPtr.Reset();
            //vkDestroyShaderModule(device->GetDevice(), fragShaderModule, nullptr);
            //vkDestroyShaderModule(device->GetDevice(), vertShaderModule, nullptr);
        }
	}
	*/

    /// -------------------------------------------------------

	void Pipeline::Invalidate()
    {
        Ref<Pipeline> instance(this);
        Renderer::Submit([instance]() mutable {

            VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
            SEDX_CORE_ASSERT(instance->pipelineSpecs.shader);
            Ref<Shader> vulkanShader = Ref<Shader>(instance->pipelineSpecs.shader);
            Ref<Framebuffer> framebuffer = instance->pipelineSpecs.dstFramebuffer.As<Framebuffer>();

            auto descriptorSetLayouts = vulkanShader->GetAllDescriptorSetLayouts();

            const auto &pushConstantRanges = vulkanShader->GetPushConstantRanges();

            /// TODO: should come from shader
            std::vector<VkPushConstantRange> vulkanPushConstantRanges(pushConstantRanges.size());
            for (uint32_t i = 0; i < pushConstantRanges.size(); i++)
            {
                const auto &pushConstantRange = pushConstantRanges[i];
                auto &vulkanPushConstantRange = vulkanPushConstantRanges[i];

                vulkanPushConstantRange.stageFlags = pushConstantRange.shaderStage;
                vulkanPushConstantRange.offset = pushConstantRange.offset;
                vulkanPushConstantRange.size = pushConstantRange.size;
            }

            // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
            // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
            VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
            pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pPipelineLayoutCreateInfo.pNext = nullptr;
            pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
            pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
            pPipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)vulkanPushConstantRanges.size();
            pPipelineLayoutCreateInfo.pPushConstantRanges = vulkanPushConstantRanges.data();

            VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &instance->pipelineLayout))

            /// Create the graphics pipeline used in this example
            /// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
            /// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
            /// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

            VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
            pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            /// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
            pipelineCreateInfo.layout = instance->pipelineLayout;
            /// Renderpass this pipeline is attached to
            pipelineCreateInfo.renderPass = framebuffer->GetRenderPass();

            /// Construct the different states making up the pipeline

            /// Input assembly state describes how primitives are assembled
            /// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
            inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssemblyState.topology = Utils::GetVKTopology(instance->pipelineSpecs.topology);

            /// Rasterization state
            VkPipelineRasterizationStateCreateInfo rasterizationState = {};
            rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationState.polygonMode = instance->pipelineSpecs.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
            rasterizationState.cullMode = instance->pipelineSpecs.backfaceCulling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
            rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizationState.depthClampEnable = VK_FALSE;
            rasterizationState.rasterizerDiscardEnable = VK_FALSE;
            rasterizationState.depthBiasEnable = VK_FALSE;
            rasterizationState.lineWidth = instance->pipelineSpecs.lineWidth; /// this is dynamic

            /**
             * Color blend state describes how blend factors are calculated (if used)
             * We need one blend attachment state per color attachment (even if blending is not used)
             */
            size_t colorAttachmentCount = framebuffer->GetSpecification().swapChainTarget ? 1 : framebuffer->GetColorAttachmentCount();
            std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates(colorAttachmentCount);
            if (framebuffer->GetSpecification().swapChainTarget)
            {
                blendAttachmentStates[0].colorWriteMask = 0xf;
                blendAttachmentStates[0].blendEnable = VK_TRUE;
                blendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                blendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                blendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
                blendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
                blendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                blendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            }
            else
            {
                for (size_t i = 0; i < colorAttachmentCount; i++)
                {
                    if (!framebuffer->GetSpecification().blend)
                        break;

                    blendAttachmentStates[i].colorWriteMask = 0xf;
                    if (!framebuffer->GetSpecification().blend)
                        break;

                    const auto &attachmentSpec = framebuffer->GetSpecification().attachments.Attachments[i];
                    FramebufferBlendMode blendMode = framebuffer->GetSpecification().blendMode == FramebufferBlendMode::None ? attachmentSpec.BlendMode : framebuffer->GetSpecification().blendMode;

                    blendAttachmentStates[i].blendEnable = attachmentSpec.Blend ? VK_TRUE : VK_FALSE;
                    blendAttachmentStates[i].colorBlendOp = VK_BLEND_OP_ADD;
                    blendAttachmentStates[i].alphaBlendOp = VK_BLEND_OP_ADD;
                    blendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

                    switch (blendMode)
                    {
                    case FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha:
                        blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        blendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                        blendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        blendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                        break;
                    case FramebufferBlendMode::OneZero:
                        blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        blendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                        break;
                    case FramebufferBlendMode::Zero_SrcColor:
                        blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                        blendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
                        break;

                    default:
                        SEDX_CORE_VERIFY(false);
                    }
                }
            }

            VkPipelineColorBlendStateCreateInfo colorBlendState = {};
            colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendState.attachmentCount = (uint32_t)blendAttachmentStates.size();
            colorBlendState.pAttachments = blendAttachmentStates.data();

            /// Viewport state sets the number of viewports and scissor used in this pipeline
            /// Note: This is actually overriden by the dynamic states (see below)
            VkPipelineViewportStateCreateInfo viewportState = {};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            /// Enable dynamic states
            /// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
            /// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
            /// For this example we will set the viewport and scissor using dynamic states
            std::vector<VkDynamicState> dynamicStateEnables;
            dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
            dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
            if (instance->DynamicLineWidth())
                dynamicStateEnables.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);

            VkPipelineDynamicStateCreateInfo dynamicState = {};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.pDynamicStates = dynamicStateEnables.data();
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

            /// Depth and stencil state containing depth and stencil compare and test operations
            /// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
            VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
            depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencilState.depthTestEnable = instance->pipelineSpecs.depthTest ? VK_TRUE : VK_FALSE;
            depthStencilState.depthWriteEnable = instance->pipelineSpecs.depthWrite ? VK_TRUE : VK_FALSE;
            depthStencilState.depthCompareOp = Utils::GetVulkanCompareOperator(instance->pipelineSpecs.depthOperator);
            depthStencilState.depthBoundsTestEnable = VK_FALSE;
            depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
            depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
            depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
            depthStencilState.stencilTestEnable = VK_FALSE;
            depthStencilState.front = depthStencilState.back;

            /// Multi sampling state
            VkPipelineMultisampleStateCreateInfo multisampleState = {};
            multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampleState.pSampleMask = nullptr;

            /// Vertex input descriptor
            VertexBufferLayout &vertexLayout = instance->pipelineSpecs.layout;
            VertexBufferLayout &instanceLayout = instance->pipelineSpecs.instanceLayout;
            VertexBufferLayout &boneInfluenceLayout = instance->pipelineSpecs.boneInfluenceLayout;

            std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;

            VkVertexInputBindingDescription &vertexInputBinding = vertexInputBindingDescriptions.emplace_back();
            vertexInputBinding.binding = 0;
            vertexInputBinding.stride = vertexLayout.GetStride();
            vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            if (instanceLayout.GetElementCount())
            {
                VkVertexInputBindingDescription &instanceInputBinding = vertexInputBindingDescriptions.emplace_back();
                instanceInputBinding.binding = 1;
                instanceInputBinding.stride = instanceLayout.GetStride();
                instanceInputBinding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
            }

            if (boneInfluenceLayout.GetElementCount())
            {
                VkVertexInputBindingDescription &boneInfluenceInputBinding = vertexInputBindingDescriptions.emplace_back();
                boneInfluenceInputBinding.binding = 2;
                boneInfluenceInputBinding.stride = boneInfluenceLayout.GetStride();
                boneInfluenceInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            }

            /// Input attribute bindings describe shader attribute locations and memory layouts
            std::vector<VkVertexInputAttributeDescription> vertexInputAttributes(vertexLayout.GetElementCount() + instanceLayout.GetElementCount() + boneInfluenceLayout.GetElementCount());

            uint32_t binding = 0;
            uint32_t location = 0;
            for (const auto &layout : {vertexLayout, instanceLayout, boneInfluenceLayout})
            {
                for (const auto &element : layout)
                {
                    vertexInputAttributes[location].binding = binding;
                    vertexInputAttributes[location].location = location;
                    vertexInputAttributes[location].format = Utils::ShaderDataTypeToVulkanFormat(element.type);
                    vertexInputAttributes[location].offset = element.offset;
                    location++;
                }
                binding++;
            }

            /// Vertex input state used for pipeline creation
            VkPipelineVertexInputStateCreateInfo vertexInputState = {};
            vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputState.vertexBindingDescriptionCount = (uint32_t)vertexInputBindingDescriptions.size();
            vertexInputState.pVertexBindingDescriptions = vertexInputBindingDescriptions.data();
            vertexInputState.vertexAttributeDescriptionCount = (uint32_t)vertexInputAttributes.size();
            vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

            const auto &shaderStages = vulkanShader->GetPipelineShaderStageCreateInfos();

            /// Set pipeline shader stage info
            pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            pipelineCreateInfo.pStages = shaderStages.data();

            /// Assign the pipeline states to the pipeline creation info structure
            pipelineCreateInfo.pVertexInputState = &vertexInputState;
            pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
            pipelineCreateInfo.pRasterizationState = &rasterizationState;
            pipelineCreateInfo.pColorBlendState = &colorBlendState;
            pipelineCreateInfo.pMultisampleState = &multisampleState;
            pipelineCreateInfo.pViewportState = &viewportState;
            pipelineCreateInfo.pDepthStencilState = &depthStencilState;
            pipelineCreateInfo.renderPass = framebuffer->GetRenderPass();
            pipelineCreateInfo.pDynamicState = &dynamicState;

            /// What is this pipeline cache?
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
            pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &instance->pipelineCache))

            /// Create rendering pipeline using the specified states
            VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, instance->pipelineCache,1, &pipelineCreateInfo, nullptr,  &instance->pipeline))
            SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_PIPELINE, instance->pipelineSpecs.debugName, instance->pipeline);
        });
    }


    /// -------------------------------------------------------

    /// Changed to utilize Shader class for shader module creation

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

    /*
    VkExtent2D Pipeline::GetFloatSwapExtent()
    {
        auto extent = SwapChain::GetSwapExtent();
        return extent;
    }
    */

	bool Pipeline::DynamicLineWidth() const
    {
        return pipelineSpecs.topology == PrimitiveTopology::Lines ||
               pipelineSpecs.topology == PrimitiveTopology::LineStrip || pipelineSpecs.wireframe;
    }

}

/// -------------------------------------------------------
