/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * pipeline.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "pipeline.h"
#include <SceneryEditorX/renderer/vulkan/bindless_manager.h>
#include <SceneryEditorX/renderer/vulkan/blend_states.h>
#include <SceneryEditorX/renderer/vulkan/depth_stencil.h>
#include <SceneryEditorX/renderer/vulkan/descriptor.h>
#include <SceneryEditorX/renderer/vulkan/descriptor_set.h>
#include <SceneryEditorX/renderer/vulkan/image_resource.h>
#include <SceneryEditorX/renderer/vulkan/input_layout.h>
#include <SceneryEditorX/renderer/vulkan/push_constant_buffer.h>
#include <SceneryEditorX/renderer/vulkan/rasterizer.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <SceneryEditorX/renderer/vulkan/swapchain.h>
#include <SceneryEditorX/renderer/vulkan/debug/graphics_debug.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader_input.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader_manager.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader_stage.h>
#include <SceneryEditorX/utils/size_macro_utils.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

#pragma region Pipeline Cache Static Members

	static VkPipelineCache s_PipelineCache = nullptr; // shared pipeline cache for all pipelines, created on demand and saved to disk on shutdown

	/**
	 * TODO: consider using a more robust caching mechanism that can persist across runs, e.g., 
	 * by hashing pipeline states and storing compiled pipelines on disk. 
	 * This would allow for faster load times and better performance in the editor, 
	 * especially when working with complex scenes and multiple pipelines.
	 *
	 * For now, we use a simple in-memory cache for demonstration purposes. 
	 */
	constexpr std::string_view PIPELINE_CACHE_PATH = "pipeline_cache.bin";

	/**
	 * @brief Converts a StageType enum to a bitmask for pipeline state tracking
	 * @param type The StageType enum value to convert
	 * @return A bitmask representing the shader stage
	 */
	static uint32_t ShaderTypeToMask(StageType type)
	{
		switch (type)
		{
			case StageType::Vertex:					return BIT(0);
			case StageType::TessellationControl:    return BIT(1);
			case StageType::TessellationEvaluation: return BIT(2);
			case StageType::Geometry:				return BIT(3);
			case StageType::Fragment:				return BIT(4);
			case StageType::Compute:				return BIT(5);
			case StageType::Graphics:				return BIT(5) - 1u;
			case StageType::All:					return BIT(31) - 1u;
			case StageType::None:
				SEDX_CORE_WARN_TAG("Pipeline","Shader StageType 'None' is not a valid shader stage for masking");
				break;
			default:							
				SEDX_CORE_WARN_TAG("Pipeline","Shader StageType is not recognized");
				break;
		}
	}

	/**
	 * @brief Helper to map VkShaderStageFlagBits to a small bitmask for duplicate detection
	 * @param stage The Vulkan shader stage flag bit to convert
	 * @return A bitmask representing the shader stage
	 */
	static uint32_t StageFlagToBit(const VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
			case VK_SHADER_STAGE_VERTEX_BIT:					return 1u << 0;
			case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:		return 1u << 1;
			case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:	return 1u << 2;
			case VK_SHADER_STAGE_GEOMETRY_BIT:					return 1u << 3;
			case VK_SHADER_STAGE_FRAGMENT_BIT:					return 1u << 4;
			case VK_SHADER_STAGE_COMPUTE_BIT:					return 1u << 5;
			default: 
				return 0;
		}

	}

#pragma endregion

	Pipeline::Pipeline(PipelineState &state, DescriptorSet *layout)
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		m_Device = device;
		m_State = state;

		// shader stages
		std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
		constexpr StageType k_StageOrder[] = {StageType::Vertex,
											  StageType::Geometry,
											  StageType::TessellationControl,
											  StageType::TessellationEvaluation,
											  StageType::Fragment,
											  StageType::Compute};
		for (StageType s : k_StageOrder)
		{
			auto it = m_State.shaders[static_cast<uint32_t>(s)];
			if (it && it->IsCompiled())
			{
				if (Ref<ShaderStage> stageRef = it->GetShaderStage(s))
				{
					shader_stages.push_back(stageRef->GetStageCreateInfo());
				}
			}
		}

		// layout: full bindless path when a DescriptorSet layout is provided, minimal bootstrap path otherwise
		if (layout != nullptr)
		{
			// build descriptor set layouts array - must match order of appearance in common_resources.slang
			std::array<VkDescriptorSetLayout, static_cast<size_t>(BindlessResource::MaxEnum) + 1> layouts;
			{
				layouts[0] = layout->GetLayout();
				SEDX_CORE_ASSERT(layouts[0] != VK_NULL_HANDLE);

				for (size_t i = 0; i < static_cast<size_t>(BindlessResource::MaxEnum); i++)
				{
					layouts[i + 1] = BindlessManager::GetLayoutForType(static_cast<BindlessResource>(i));
					SEDX_CORE_ASSERT(layouts[i + 1] != VK_NULL_HANDLE);
				}
			}

			// push constant buffers
			std::vector<VkPushConstantRange> push_constant_ranges;
			for (const Descriptor &descriptor : layout->GetDescriptors())
			{
				if (descriptor.GetType() == DescriptorType::PushConstantBuffer)
				{
					SEDX_CORE_ASSERT(descriptor.GetStructSize() <= m_Device->GetDeviceStatics().maxPushConstantsSize);

					VkPushConstantRange push_constant_range = {};
					push_constant_range.size = descriptor.GetStructSize();
					push_constant_range.stageFlags |=
						(descriptor.GetStage() & ShaderTypeToMask(StageType::Vertex)) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
					push_constant_range.stageFlags |=
						(descriptor.GetStage() & ShaderTypeToMask(StageType::TessellationControl))
							? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
							: 0;
					push_constant_range.stageFlags |=
						(descriptor.GetStage() & ShaderTypeToMask(StageType::TessellationEvaluation))
							? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
							: 0;
					push_constant_range.stageFlags |= (descriptor.GetStage() & ShaderTypeToMask(StageType::Fragment))
														  ? VK_SHADER_STAGE_FRAGMENT_BIT
														  : 0;
					push_constant_range.stageFlags |= (descriptor.GetStage() & ShaderTypeToMask(StageType::Compute))
														  ? VK_SHADER_STAGE_COMPUTE_BIT
														  : 0;


					// store the stages for use in PushConstants calls
					m_PushConstant_Stages |= push_constant_range.stageFlags;

					push_constant_ranges.emplace_back(push_constant_range);
				}
			}

			// pipeline layout
			VkPipelineLayoutCreateInfo pipeline_layout_info = {};
			pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipeline_layout_info.pushConstantRangeCount = 0;
			pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(layouts.size());
			pipeline_layout_info.pSetLayouts = layouts.data();
			pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
			pipeline_layout_info.pPushConstantRanges = push_constant_ranges.data();

			// create
			SEDX_VK_RESULT_ASSERT(vkCreatePipelineLayout(device->GetLogicalDevice(),
														 &pipeline_layout_info,
														 nullptr,
														 reinterpret_cast<VkPipelineLayout *>(&m_Layout)));

			// name
			Debugging::SetResourceName(m_Layout, ResourceType::PipelineLayout, state.name);
		}
		else
		{
			// Bootstrap path: no bindless descriptor sets — minimal push-constant-only VkPipelineLayout.
			// Used for passes that don't yet have a DescriptorSet layout wired (e.g. grid, text, bootstrap).
			VkPushConstantRange pcRange{};
			pcRange.stageFlags =
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
			pcRange.offset = 0;
			pcRange.size = static_cast<uint32_t>(sizeof(PushConstantBuffer_Pass));
			m_PushConstant_Stages = pcRange.stageFlags;

			VkPipelineLayoutCreateInfo bootstrapLayoutCI{};
			bootstrapLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			bootstrapLayoutCI.pushConstantRangeCount = 1;
			bootstrapLayoutCI.pPushConstantRanges = &pcRange;
			bootstrapLayoutCI.setLayoutCount = 0;
			bootstrapLayoutCI.pSetLayouts = nullptr;

			SEDX_VK_RESULT_ASSERT(vkCreatePipelineLayout(device->GetLogicalDevice(),
														 &bootstrapLayoutCI,
														 nullptr,
														 reinterpret_cast<VkPipelineLayout *>(&m_Layout)));
			Debugging::SetResourceName(m_Layout, ResourceType::PipelineLayout, state.name);
		}

		if (state.IsCompute())
		{
			VkComputePipelineCreateInfo pipeline_info = {};
			pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipeline_info.layout = m_Layout;
			pipeline_info.stage = shader_stages[0];

			SEDX_VK_RESULT_ASSERT(vkCreateComputePipelines(m_Device->GetLogicalDevice(),
														   static_cast<VkPipelineCache>(GetPipelineCache()),
														   1,
														   &pipeline_info,
														   nullptr,
														   reinterpret_cast<VkPipeline *>(&m_Pipeline)));
			Debugging::SetResourceName(static_cast<void *>(m_Pipeline), ResourceType::Pipeline, state.name);
		}
		else if (state.IsGraphics())
		{
			// viewport & scissor
			std::vector<VkDynamicState> dynamic_states = {};
			VkPipelineDynamicStateCreateInfo dynamic_state = {};
			VkViewport vkViewport = {};
			VkRect2D scissor = {};
			VkPipelineViewportStateCreateInfo viewport_state = {};
			{
				// dynamic states
				{
					dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
					dynamic_state.pNext = nullptr;
					dynamic_state.flags = 0;
					dynamic_states.push_back(VK_DYNAMIC_STATE_VIEWPORT);
					if (m_State.IsGraphics())
					{
						dynamic_states.push_back(VK_DYNAMIC_STATE_SCISSOR);
						dynamic_states.push_back(VK_DYNAMIC_STATE_CULL_MODE);
						// Only add fragment shading rate dynamic state when the PSO actually uses a VRS
						// input texture.  Adding it unconditionally causes validation errors on every
						// draw call for pipelines (e.g. ImGui) that never call vkCmdSetFragmentShadingRateKHR.
						if (Device::GetDeviceStatics().isShadingRateSupported && m_State.vrsInputTexture)
						{
							dynamic_states.push_back(VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR);
						}
					}
					dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
					dynamic_state.pDynamicStates = dynamic_states.data();
				}

				// viewport
				vkViewport.x = 0;
				vkViewport.y = 0;
				vkViewport.width = static_cast<float>(m_State.GetWidth());
				vkViewport.height = static_cast<float>(m_State.GetHeight());
				vkViewport.minDepth = 0.0f;
				vkViewport.maxDepth = 1.0f;

				// scissor
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent.width = static_cast<uint32_t>(vkViewport.width);
				scissor.extent.height = static_cast<uint32_t>(vkViewport.height);

				// viewport state
				viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewport_state.viewportCount = 1;
				viewport_state.pViewports = &vkViewport;
				viewport_state.scissorCount = 1;
				viewport_state.pScissors = &scissor;
			}

			// binding and vertex attribute descriptions
			std::vector<VkVertexInputBindingDescription> vertex_input_binding_descs;
			std::vector<VkVertexInputAttributeDescription> vertex_attribute_descs;
			Shader *shader_vertex = nullptr;
			{
				auto it = m_State.shaders[static_cast<uint32_t>(StageType::Vertex)];
				if (it && it->IsCompiled())
					shader_vertex = it;
			}

			Ref<InputLayout> vertexInputLayout;
			if (shader_vertex)
			{
				// determine vertex type from the stage's input attributes and build an InputLayout
				Ref<ShaderStage> vertStage = shader_vertex->GetShaderStage(StageType::Vertex);
				if (vertStage)
				{
					const auto &inputs = vertStage->GetInput();
					bool hasNorTan = false, hasColor = false, hasTex = false, has2D = false;
					for (const ShaderInput &inp : inputs)
					{
						if (inp.debugName == "NORMAL" || inp.debugName == "TANGENT")
							hasNorTan = true;
						if (inp.debugName == "COLOR")
							hasColor = true;
						if (inp.debugName == "TEXCOORD")
							hasTex = true;
					}
					VertexType vtype = VertexType::MaxEnum;
					if (hasNorTan)
						vtype = VertexType::PositionUvNormalTangent;
					else if (hasColor)
						vtype = VertexType::PositionColor;
					else if (hasTex && has2D)
						vtype = VertexType::Position2dUvColor8;
					else if (hasTex)
						vtype = VertexType::PositionUv;
					else
						vtype = VertexType::Position;
					vertexInputLayout = CreateRef<InputLayout>();
					vertexInputLayout->Create(vtype);
				}
			}
			InputLayout *input_layout = vertexInputLayout.Get();
			if (input_layout)
			{
				const auto &attribute_descs = input_layout->GetAttributeDescriptions();

				// vertex buffer (binding 0) - for per-vertex attributes like position, uv, color, normal, tangent
				bool has_vertex_attributes = false;
				bool is_geometry_pass_vertex = false;
				for (const auto &desc : attribute_descs)
				{
					// check for per-vertex attributes
					if (desc.name == "POSITION" || desc.name == "TEXCOORD" || desc.name == "COLOR" ||
						desc.name == "NORMAL" || desc.name == "TANGENT")
					{
						has_vertex_attributes = true;

						// geometry pass vertices have normal or tangent
						if (desc.name == "NORMAL" || desc.name == "TANGENT")
						{
							is_geometry_pass_vertex = true;
						}
					}
				}

				// add vertex buffer binding if there are per-vertex attributes
				if (has_vertex_attributes)
				{
					vertex_input_binding_descs.push_back({
						0,                             // binding
						input_layout->GetVertexSize(), // stride
						VK_VERTEX_INPUT_RATE_VERTEX    // input rate
					});

					// add attribute descriptions for per-vertex attributes
					for (const auto &desc : attribute_descs)
					{
						if (desc.name == "POSITION" || desc.name == "TEXCOORD" || desc.name == "COLOR" ||
							desc.name == "NORMAL" || desc.name == "TANGENT")
						{
							vertex_attribute_descs.push_back({
								desc.location, // location
								0,             // binding (vertex buffer)
								desc.format,   // format
								desc.offset    // offset
							});
						}
					}
				}

				// instance buffer (binding 1) - for instance transform (position, rotation, scale)
				if (is_geometry_pass_vertex)
				{
					vertex_input_binding_descs.emplace_back(1,
															static_cast<uint32_t>(sizeof(Instance)),
															VK_VERTEX_INPUT_RATE_INSTANCE);
					uint32_t start_index = static_cast<uint32_t>(vertex_attribute_descs.size());
					vertex_attribute_descs.emplace_back(start_index++,
														1,
														VK_FORMAT_R16_SFLOAT,
														static_cast<uint32_t>(offsetof(Instance, positionX)));
					vertex_attribute_descs.emplace_back(start_index++,
														1,
														VK_FORMAT_R16_SFLOAT,
														static_cast<uint32_t>(offsetof(Instance, positionY)));
					vertex_attribute_descs.emplace_back(start_index++,
														1,
														VK_FORMAT_R16_SFLOAT,
														static_cast<uint32_t>(offsetof(Instance, positionZ)));
					vertex_attribute_descs.emplace_back(start_index++,
														1,
														VK_FORMAT_R16_UINT,
														static_cast<uint32_t>(offsetof(Instance, normal_Oct)));
					vertex_attribute_descs.emplace_back(start_index++,
														1,
														VK_FORMAT_R8_UINT,
														static_cast<uint32_t>(offsetof(Instance, yaw_Packed)));
					vertex_attribute_descs.emplace_back(start_index++,
														1,
														VK_FORMAT_R8_UINT,
														static_cast<uint32_t>(offsetof(Instance, scale_Packed)));
				}
			}
			// vertex input state
			VkPipelineVertexInputStateCreateInfo vertexInputState = {};
			{
				vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputState.vertexBindingDescriptionCount =
					static_cast<uint32_t>(vertex_input_binding_descs.size());
				vertexInputState.pVertexBindingDescriptions = vertex_input_binding_descs.data();
				vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descs.size());
				vertexInputState.pVertexAttributeDescriptions = vertex_attribute_descs.data();
			}

			// input assembly state
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
			{
				inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssemblyState.topology =
					m_State.HasTessellation() ? VK_PRIMITIVE_TOPOLOGY_PATCH_LIST : m_State.primitiveTopology;
				inputAssemblyState.primitiveRestartEnable = VK_FALSE;
			}

			// tessellation state
			VkPipelineTessellationStateCreateInfo tessellationState = {};
			{
				tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
				tessellationState.patchControlPoints = m_State.HasTessellation() ? 3 : 1;
			}

			// rasterizer state
			VkPipelineRasterizationStateCreateInfo rasterizerState = {};
			if (m_State.rasterizerState)
			{
				rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterizerState.depthClampEnable = !m_State.rasterizerState->IsDepthClipEnabled();
				rasterizerState.rasterizerDiscardEnable = VK_FALSE;
				rasterizerState.polygonMode = static_cast<VkPolygonMode>(m_State.rasterizerState->GetPolygonMode());
				rasterizerState.lineWidth = m_State.rasterizerState->GetLineWidth();
				rasterizerState.cullMode = static_cast<uint32_t>(CullMode::Back);
				rasterizerState.frontFace = VK_FRONT_FACE_CLOCKWISE;
				rasterizerState.depthBiasEnable = m_State.rasterizerState->GetDepthBias() != 0.0f ? VK_TRUE : VK_FALSE;
				rasterizerState.depthBiasConstantFactor =
					floor(m_State.rasterizerState->GetDepthBias() * (float)(1 << 24));
				rasterizerState.depthBiasClamp = m_State.rasterizerState->GetDepthBiasClamp();
				rasterizerState.depthBiasSlopeFactor = m_State.rasterizerState->GetDepthBiasSlope();
			}

			// multisampling
			VkPipelineMultisampleStateCreateInfo multisample_state = {};
			{
				multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisample_state.sampleShadingEnable = VK_FALSE;
				multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			}

			// blend state
			VkPipelineColorBlendStateCreateInfo color_blend_state = {};
			std::vector<VkPipelineColorBlendAttachmentState> blend_state_attachments;
			if (m_State.blendState)
			{
				// attachments
				{
					// same blend state for all
					VkPipelineColorBlendAttachmentState blend_state_attachment = {};
					blend_state_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
															VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
					blend_state_attachment.blendEnable = m_State.blendState->IsBlendEnabled() ? VK_TRUE : VK_FALSE;
					blend_state_attachment.srcColorBlendFactor = m_State.blendState->GetSrcColor();
					blend_state_attachment.dstColorBlendFactor = m_State.blendState->GetDstColor();
					blend_state_attachment.colorBlendOp = m_State.blendState->GetColorOp();
					blend_state_attachment.srcAlphaBlendFactor = m_State.blendState->GetSrcAlpha();
					blend_state_attachment.dstAlphaBlendFactor = m_State.blendState->GetDstAlpha();
					blend_state_attachment.alphaBlendOp = m_State.blendState->GetAlphaOp();

					// swapchain
					if (m_State.renderTarget_Swapchain)
					{
						blend_state_attachments.push_back(blend_state_attachment);
					}

					// render target(s)
					for (uint8_t i = 0; i < MAX_RENDER_TARGET_COUNT; i++)
					{
						if (m_State.renderTarget_ColorTextures[i] != nullptr)
						{
							blend_state_attachments.push_back(blend_state_attachment);
						}
					}
				}

				color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				color_blend_state.logicOpEnable = VK_FALSE;
				color_blend_state.logicOp = VK_LOGIC_OP_COPY;
				color_blend_state.attachmentCount = static_cast<uint32_t>(blend_state_attachments.size());
				color_blend_state.pAttachments = blend_state_attachments.data();
				color_blend_state.blendConstants[0] = m_State.blendState->GetBlendFactor();
				color_blend_state.blendConstants[1] = m_State.blendState->GetBlendFactor();
				color_blend_state.blendConstants[2] = m_State.blendState->GetBlendFactor();
				color_blend_state.blendConstants[3] = m_State.blendState->GetBlendFactor();
			}

			// depth-stencil state
			VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
			if (m_State.depthStencil_State)
			{
				depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				depth_stencil_state.depthTestEnable = m_State.depthStencil_State->IsDepthTestEnabled();
				depth_stencil_state.depthWriteEnable = m_State.depthStencil_State->IsDepthWriteEnabled();
				depth_stencil_state.depthCompareOp = m_State.depthStencil_State->GetDepthCompareOp();
				depth_stencil_state.stencilTestEnable = m_State.depthStencil_State->IsStencilTestEnabled();
				depth_stencil_state.front.compareOp = m_State.depthStencil_State->GetStencilCompFunc();
				depth_stencil_state.front.failOp = m_State.depthStencil_State->GetStencilFailOp();
				depth_stencil_state.front.depthFailOp = m_State.depthStencil_State->GetStencilDepthFailOp();
				depth_stencil_state.front.passOp = m_State.depthStencil_State->GetStencilPassOp();
				depth_stencil_state.front.compareMask = m_State.depthStencil_State->GetStencilReadMask();
				depth_stencil_state.front.writeMask = m_State.depthStencil_State->GetStencilWriteMask();
				depth_stencil_state.front.reference = 1;
				depth_stencil_state.back = depth_stencil_state.front;
				depth_stencil_state.minDepthBounds = 1.0f; // functionality of DirectX with reverse-z
				depth_stencil_state.maxDepthBounds = 0.0f; // functionality of DirectX with reverse-z
			}

			// pipeline
			{
				// enable dynamic rendering - VK_KHR_dynamic_rendering
				// this means no render passes and no frame buffer objects
				VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info = {};
				VkPipelineFragmentShadingRateStateCreateInfoKHR fragment_shading_rate_state = {};
				std::vector<VkFormat> attachment_formats_color;
				VkFormat attachment_format_depth = VK_FORMAT_UNDEFINED;
				VkFormat attachment_format_stencil = VK_FORMAT_UNDEFINED;
				{
					// swapchain buffer as a render target
					if (m_State.renderTarget_Swapchain)
					{
						attachment_formats_color.push_back(m_State.renderTarget_Swapchain->GetImageFormat());
						// If the PSO has no explicit depth texture but targets a swapchain, inherit
						// the swapchain's depth format so the pipeline declaration matches the active
						// dynamic render pass (which always has a depth attachment).
						if (!m_State.renderTarget_DepthTexture)
						{
							attachment_format_depth = m_State.renderTarget_Swapchain->GetDepthFormat();
							attachment_format_stencil = VK_FORMAT_UNDEFINED; // stencil kept separate if needed
						}
					}
					else // regular render target(s)
					{
						for (uint32_t i = 0; i < MAX_RENDER_TARGET_COUNT; i++)
						{
							ImageResource *texture = m_State.renderTarget_ColorTextures[i];
							if (texture == nullptr)
								break;

							attachment_formats_color.push_back(texture->GetFormat());
						}
					}

					// depth
					if (m_State.renderTarget_DepthTexture)
					{
						ImageResource *tex_depth = m_State.renderTarget_DepthTexture;
						attachment_format_depth = tex_depth->GetFormat();
						attachment_format_stencil =
							tex_depth->IsStencilFormat() ? attachment_format_depth : VK_FORMAT_UNDEFINED;
					}

					// variable rate shading
					if (m_State.vrsInputTexture)
					{
						fragment_shading_rate_state.sType =
							VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR;
						fragment_shading_rate_state.combinerOps[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
						fragment_shading_rate_state.combinerOps[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
						fragment_shading_rate_state.fragmentSize = {.width = 1, .height = 1};

						pipeline_rendering_create_info.pNext = &fragment_shading_rate_state;
					}

					// put everything together
					pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
					pipeline_rendering_create_info.colorAttachmentCount =
						static_cast<uint32_t>(attachment_formats_color.size());
					pipeline_rendering_create_info.pColorAttachmentFormats = attachment_formats_color.data();
					pipeline_rendering_create_info.depthAttachmentFormat = attachment_format_depth;
					pipeline_rendering_create_info.stencilAttachmentFormat = attachment_format_stencil;
					pipeline_rendering_create_info.viewMask = m_State.isMultiview ? 0b11 : 0;
				}

				// create
				{
					VkGraphicsPipelineCreateInfo pipelineInfo = {};
					pipelineInfo.pNext = &pipeline_rendering_create_info;
					pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
					pipelineInfo.stageCount = static_cast<uint32_t>(shader_stages.size());
					pipelineInfo.pStages = shader_stages.data();
					pipelineInfo.pVertexInputState = &vertexInputState;
					pipelineInfo.pInputAssemblyState = &inputAssemblyState;
					pipelineInfo.pTessellationState = &tessellationState;
					pipelineInfo.pDynamicState = &dynamic_state;
					pipelineInfo.pViewportState = &viewport_state;
					pipelineInfo.pRasterizationState = &rasterizerState;
					pipelineInfo.pMultisampleState = &multisample_state;
					pipelineInfo.pColorBlendState = &color_blend_state;
					pipelineInfo.pDepthStencilState = &depth_stencil_state;
					pipelineInfo.layout = m_Layout;
					pipelineInfo.flags = m_State.vrsInputTexture
											 ? VK_PIPELINE_CREATE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR
											 : 0;

					SEDX_VK_RESULT_ASSERT(vkCreateGraphicsPipelines(m_Device->GetLogicalDevice(),
																	static_cast<VkPipelineCache>(GetPipelineCache()),
																	1,
																	&pipelineInfo,
																	nullptr,
																	reinterpret_cast<VkPipeline *>(&m_Pipeline)));
					Debugging::SetResourceName(m_Pipeline, ResourceType::Pipeline, state.name);
				}
			}
		}

		SEDX_CORE_ASSERT(m_Pipeline != nullptr);
	}

	Pipeline::~Pipeline()
	{
		Destroy();

		// pipeline cache - save to disk before destroying
		SavePipelineCache();
		if (s_PipelineCache && m_Device)
		{
			vkDestroyPipelineCache(m_Device->GetLogicalDevice(), s_PipelineCache, nullptr);
			s_PipelineCache = nullptr;
		}
	}

	Pipeline::Pipeline(Pipeline &&other) noexcept : m_Device(std::move(other.m_Device)), m_Pipeline(std::exchange(other.m_Pipeline, VK_NULL_HANDLE)),
		  m_State(other.m_State), m_Layout(std::exchange(other.m_Layout, VK_NULL_HANDLE)),
		  m_PushConstant_Stages(other.m_PushConstant_Stages), m_Destroyed(std::exchange(other.m_Destroyed, true))
	{
	}

	Pipeline &Pipeline::operator=(Pipeline &&other) noexcept
	{
		if (this != &other)
		{
			Destroy();
			m_Device = std::move(other.m_Device);
			m_Pipeline = std::exchange(other.m_Pipeline, VK_NULL_HANDLE);
			m_State = other.m_State;
			m_Layout = std::exchange(other.m_Layout, VK_NULL_HANDLE);
			m_PushConstant_Stages = other.m_PushConstant_Stages;
			m_Destroyed = std::exchange(other.m_Destroyed, true);
		}

		return *this;
	}

	VkPipelineCache Pipeline::GetPipelineCache()
	{
		return s_PipelineCache;
	}
	
	void Pipeline::Destroy(const VkDevice device)
	{
		if (m_Destroyed)
			return;

		VkDevice logicalDevice = device;
		if (logicalDevice == VK_NULL_HANDLE)
		{
			if (m_Device)
			{
				logicalDevice = m_Device->GetLogicalDevice();
			}
			else if (Ref<RenderContext> ctx = RenderContext::Get())
			{
				if (Ref<Device> dev = ctx->GetDevice())
				{
					logicalDevice = dev->GetLogicalDevice();
				}
			}
		}

		if (logicalDevice != VK_NULL_HANDLE)
		{
			if (m_Pipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(logicalDevice, m_Pipeline, nullptr);
				m_Pipeline = VK_NULL_HANDLE;
			}

			if (m_Layout != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(logicalDevice, m_Layout, nullptr);
				m_Layout = VK_NULL_HANDLE;
			}
		}

		m_Destroyed = true;
	}

	VkPipeline Pipeline::CreateGraphics(const GraphicsCreateInfo &info)
	{
		if (!info.shaderManager || info.device == VK_NULL_HANDLE)
			return VK_NULL_HANDLE;

		const ShaderManager &shaderManager = *info.shaderManager;

		// Shader stages (use provided shader manager)
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		shaderStages.reserve(shaderManager.StageCount());

		for (size_t i = 0; i < shaderManager.StageCount(); ++i)
		{
			VkPipelineShaderStageCreateInfo sci{};
			sci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			sci.stage = shaderManager.StageAt(i);
			sci.module = shaderManager.ModuleAt(i);

			if (sci.stage == VK_SHADER_STAGE_VERTEX_BIT)
			{
				sci.pName = info.vertexEntryPoint ? info.vertexEntryPoint : "main";
			}
			else if (sci.stage == VK_SHADER_STAGE_FRAGMENT_BIT)
			{
				sci.pName = info.fragmentEntryPoint ? info.fragmentEntryPoint : "main";
			}
			else
			{
				sci.pName = (sci.stage == VK_SHADER_STAGE_COMPUTE_BIT) ? "main_comp" : "main";
			}

			shaderStages.push_back(sci);
		}

		// Vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputState{};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = 1;
		vertexInputState.pVertexBindingDescriptions = &info.vertexBinding;
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(info.vertexAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = info.vertexAttributes.data();

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// Dynamic states (viewport + scissor)
		std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_CULL_MODE};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		// Rasterization
		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		// Temporary debug-friendly rasterization defaults:
		// disable culling so winding/order mismatches cannot make the entire frame black.
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.lineWidth = 1.0f;

		// Multisample
		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// Depth/stencil
		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		// Color blend
		VkPipelineColorBlendAttachmentState blendAttachment{};
		blendAttachment.colorWriteMask = 0xF;
		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachment;

		// Rendering info (dynamic rendering)
		VkPipelineRenderingCreateInfo renderingCI{};
		renderingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingCI.colorAttachmentCount = 1;
		renderingCI.pColorAttachmentFormats = &info.colorFormat;
		renderingCI.depthAttachmentFormat = info.depthFormat;

		// Viewport state (no static viewport because we use dynamic state)
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = &renderingCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState = &vertexInputState;
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pDynamicState = &dynamicState;

		{
			if (info.layout == VK_NULL_HANDLE)
			{
				SEDX_CORE_ERROR_TAG("Pipeline", "invalid pipeline layout (VK_NULL_HANDLE)");
				return VK_NULL_HANDLE;
			}

			// Validate shader stages: non-null modules and no duplicate stage flags
			uint32_t seenStageMask = 0;
			for (uint32_t i = 0; i < pipelineCI.stageCount; ++i)
			{
				const VkPipelineShaderStageCreateInfo &s = pipelineCI.pStages[i];
				if (s.module == VK_NULL_HANDLE)
				{
					SEDX_CORE_ERROR_TAG("Pipeline",
										"invalid shader module at stage index {} (stage flag: {})",
										i,
										static_cast<uint32_t>(s.stage));
					return VK_NULL_HANDLE;
				}

				uint32_t bit = StageFlagToBit(s.stage);
				if (bit == 0)
				{
					SEDX_CORE_ERROR_TAG("Pipeline",
										"unsupported or unknown shader stage flag at index {}: {}",
										i,
										static_cast<uint32_t>(s.stage));
					return VK_NULL_HANDLE;
				}

				if (seenStageMask & bit)
				{
					SEDX_CORE_ERROR_TAG("Pipeline",
										"duplicate shader stage detected at index {} (stage flag: {})",
										i,
										static_cast<uint32_t>(s.stage));
					return VK_NULL_HANDLE;
				}
				seenStageMask |= bit;
			}

			pipelineCI.layout = info.layout;
		}

		VkPipeline pipeline = VK_NULL_HANDLE;
		VkResult r = vkCreateGraphicsPipelines(info.device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline);
		if (r != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("Pipeline", "vkCreateGraphicsPipelines failed: {}", r);
			return VK_NULL_HANDLE;
		}

		return pipeline;
	}

	void Pipeline::CreatePipelineCache()
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();

		VkPipelineCacheCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

		// try to load a previously saved cache from disk
		std::vector<uint8_t> cacheData;
		{
			SEDX_CORE_TRACE_TAG("Pipeline", "Attempting to load pipeline cache from disk");

			std::ifstream file(std::string(PIPELINE_CACHE_PATH), std::ios::binary | std::ios::ate);
			if (file.is_open())
			{
				SEDX_CORE_TRACE_TAG("Pipeline", "Loading pipeline cache from disk");
				if (size_t size = file.tellg(); size > 0)
				{
					cacheData.resize(size);
					file.seekg(0, std::ios::beg);
					file.read(reinterpret_cast<char *>(cacheData.data()), size);
					// Format the file size into a human-readable string using the project's utility
					char humanSizeBuf[64] = {0};
					HumanSize(static_cast<double>(size), humanSizeBuf, sizeof(humanSizeBuf));
					SEDX_CORE_TRACE_TAG("Pipeline", "Pipeline cache loaded from disk with size: {}", humanSizeBuf);
				}
			}
		}

		if (!cacheData.empty())
		{
			createInfo.initialDataSize = cacheData.size();
			createInfo.pInitialData = cacheData.data();
		}

		SEDX_VK_RESULT_ASSERT(
			vkCreatePipelineCache(device->GetLogicalDevice(), &createInfo, nullptr, &s_PipelineCache));
	}

	void Pipeline::SavePipelineCache()
	{
		if (!s_PipelineCache)
			return;

		const Ref<Device> device = RenderContext::Get()->GetDevice();

		size_t dataSize = 0;
		SEDX_CORE_TRACE_TAG("Pipeline", "Attempting to save pipeline cache to disk");
		SEDX_VK_RESULT_ASSERT(vkGetPipelineCacheData(device->GetLogicalDevice(), s_PipelineCache, &dataSize, nullptr));
		if (dataSize > 0)
		{
			std::vector<uint8_t> data(dataSize);
			SEDX_VK_RESULT_ASSERT(
				vkGetPipelineCacheData(device->GetLogicalDevice(), s_PipelineCache, &dataSize, data.data()));
			std::ofstream file(std::string(PIPELINE_CACHE_PATH), std::ios::binary);
			if (file.is_open())
			{
				file.write(reinterpret_cast<const char *>(data.data()), dataSize);

				char humanSizeBuf[64] = {0};
				HumanSize(static_cast<double>(dataSize), humanSizeBuf, sizeof(humanSizeBuf));
				SEDX_CORE_TRACE_TAG("Pipeline", "Pipeline cache saved to disk with size: {}", humanSizeBuf);
			}
		}
	}

}

// -------------------------------------------------------
