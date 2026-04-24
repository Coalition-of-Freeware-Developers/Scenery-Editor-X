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
#include <SceneryEditorX/renderer/vulkan/queue_manager.h>
#include <SceneryEditorX/utils/size_macro_utils.h>
#include <cstring>
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
			case StageType::Vertex:					return VK_SHADER_STAGE_VERTEX_BIT;
			case StageType::TessellationControl:    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case StageType::TessellationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case StageType::Geometry:				return VK_SHADER_STAGE_GEOMETRY_BIT;
			case StageType::Fragment:				return VK_SHADER_STAGE_FRAGMENT_BIT;
			case StageType::Compute:				return VK_SHADER_STAGE_COMPUTE_BIT;
			case StageType::Graphics:				return VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			case StageType::All:					return VK_SHADER_STAGE_ALL;
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
		m_PushConstant_Stages = 0;

#pragma region Shader Stages
		Ref<ShaderManager> shaderManager = ShaderManager::Get();
		std::array<Ref<Shader>, static_cast<uint32_t>(Renderer_Shader::MaxEnum)> shaders = shaderManager->GetShaders();
		(void)shaders;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		if (state.IsCompute())
		{
			if (Shader* computeShader = state.shaders[static_cast<uint32_t>(StageType::Compute)])
			{
				if (computeShader->IsCompiled() && computeShader->HasStage(StageType::Compute))
				{
					shaderStages.push_back(computeShader->GetShaderStage(StageType::Compute)->GetStageCreateInfo());
				}
			}
		}
		else if (state.IsGraphics())
		{
			if (Shader* vertexShader = state.shaders[static_cast<uint32_t>(StageType::Vertex)])
			{
				if (vertexShader->IsCompiled() && vertexShader->HasStage(StageType::Vertex))
					shaderStages.push_back(vertexShader->GetShaderStage(StageType::Vertex)->GetStageCreateInfo());
			}

			if (Shader* tessControlShader = state.shaders[static_cast<uint32_t>(StageType::TessellationControl)])
			{
				if (tessControlShader->IsCompiled() && tessControlShader->HasStage(StageType::TessellationControl))
					shaderStages.push_back(tessControlShader->GetShaderStage(StageType::TessellationControl)->GetStageCreateInfo());
			}

			if (Shader* tessEvalShader = state.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)])
			{
				if (tessEvalShader->IsCompiled() && tessEvalShader->HasStage(StageType::TessellationEvaluation))
					shaderStages.push_back(tessEvalShader->GetShaderStage(StageType::TessellationEvaluation)->GetStageCreateInfo());
			}

			if (Shader* geometryShader = state.shaders[static_cast<uint32_t>(StageType::Geometry)])
			{
				if (geometryShader->IsCompiled() && geometryShader->HasStage(StageType::Geometry))
					shaderStages.push_back(geometryShader->GetShaderStage(StageType::Geometry)->GetStageCreateInfo());
			}

			if (Shader* fragmentShader = state.shaders[static_cast<uint32_t>(StageType::Fragment)])
			{
				if (fragmentShader->IsCompiled() && fragmentShader->HasStage(StageType::Fragment))
					shaderStages.push_back(fragmentShader->GetShaderStage(StageType::Fragment)->GetStageCreateInfo());
			}
		}

#pragma endregion
#pragma region Layout and Push Constants

		// layout: full bindless path when a DescriptorSet layout is provided, minimal bootstrap path otherwise
		if (layout != nullptr)
		{
			// Ensure bindless descriptor set layouts exist before wiring pipeline set layouts.
			BindlessManager::Init();

			// build descriptor set layouts array - include only valid layouts to satisfy
			// VUID-VkPipelineLayoutCreateInfo-graphicsPipelineLibrary-06753.
			std::array<VkDescriptorSetLayout, static_cast<size_t>(BindlessResource::MaxEnum) + 1> layouts{};
			uint32_t setLayoutCount = 0;
			{
				layouts[setLayoutCount++] = layout->GetLayout();
				SEDX_CORE_ASSERT(layouts[0] != VK_NULL_HANDLE);

				for (size_t i = 0; i < static_cast<size_t>(BindlessResource::MaxEnum); i++)
				{
					const VkDescriptorSetLayout bindlessLayout = BindlessManager::GetLayoutForType(static_cast<BindlessResource>(i));
					if (bindlessLayout == VK_NULL_HANDLE)
					{
						SEDX_CORE_WARN_TAG("Pipeline", "Bindless layout {} unavailable while creating pipeline '{}'; skipping set slot", i, state.name ? state.name : "<unnamed>");
						continue;
					}

					layouts[setLayoutCount++] = bindlessLayout;
				}
			}

			// push constant ranges are derived from actual active shader stages in this PSO
			// to guarantee layout stage coverage matches pipeline stages.
			std::vector<VkPushConstantRange> pushConstRanges;
			VkShaderStageFlags activeStages = 0;
			if (state.IsCompute())
			{
				activeStages |= VK_SHADER_STAGE_COMPUTE_BIT;
			}
			else if (state.IsGraphics())
			{
				if (state.shaders[static_cast<uint32_t>(StageType::Vertex)] != nullptr)
					activeStages |= VK_SHADER_STAGE_VERTEX_BIT;
				if (state.shaders[static_cast<uint32_t>(StageType::TessellationControl)] != nullptr)
					activeStages |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				if (state.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)] != nullptr)
					activeStages |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				if (state.shaders[static_cast<uint32_t>(StageType::Geometry)] != nullptr)
					activeStages |= VK_SHADER_STAGE_GEOMETRY_BIT;
				if (state.shaders[static_cast<uint32_t>(StageType::Fragment)] != nullptr)
					activeStages |= VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			if (activeStages != 0)
			{
				SEDX_CORE_ASSERT(static_cast<uint32_t>(sizeof(PushConstantBuffer_Pass)) <= m_Device->GetDeviceStatics().maxPushConstantsSize,
					"PushConstantBuffer_Pass exceeds device push constant size limit");

				VkPushConstantRange pushConstantRange{};
				pushConstantRange.offset = 0;
				pushConstantRange.size = static_cast<uint32_t>(sizeof(PushConstantBuffer_Pass));
				pushConstantRange.stageFlags = activeStages;
				pushConstRanges.emplace_back(pushConstantRange);

				m_PushConstant_Stages = activeStages;
			}

			// pipeline layout
			VkPipelineLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutInfo.pushConstantRangeCount = 0;
			layoutInfo.setLayoutCount = setLayoutCount;
			layoutInfo.pSetLayouts = layouts.data();
			layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstRanges.size());
			layoutInfo.pPushConstantRanges = pushConstRanges.data();

			// create
			SEDX_VK_RESULT_ASSERT(vkCreatePipelineLayout(device->GetLogicalDevice(), &layoutInfo, nullptr, reinterpret_cast<VkPipelineLayout *>(&m_Layout)));

			// name
			Debugging::SetResourceName(m_Layout, ResourceType::PipelineLayout, state.name);
		}
		else
		{
			// Bootstrap path: no bindless descriptor sets — minimal push-constant-only VkPipelineLayout.
			// Used for passes that don't yet have a DescriptorSet layout wired (e.g. grid, text, bootstrap).
			VkPushConstantRange pcRange{};
			VkShaderStageFlags activeStages = 0;
			if (state.IsCompute())
			{
				activeStages |= VK_SHADER_STAGE_COMPUTE_BIT;
			}
			else if (state.IsGraphics())
			{
				if (state.shaders[static_cast<uint32_t>(StageType::Vertex)] != nullptr)
					activeStages |= VK_SHADER_STAGE_VERTEX_BIT;
				if (state.shaders[static_cast<uint32_t>(StageType::TessellationControl)] != nullptr)
					activeStages |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				if (state.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)] != nullptr)
					activeStages |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				if (state.shaders[static_cast<uint32_t>(StageType::Geometry)] != nullptr)
					activeStages |= VK_SHADER_STAGE_GEOMETRY_BIT;
				if (state.shaders[static_cast<uint32_t>(StageType::Fragment)] != nullptr)
					activeStages |= VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			if (activeStages == 0)
				activeStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			pcRange.stageFlags = activeStages;
			pcRange.offset = 0;
			pcRange.size = static_cast<uint32_t>(sizeof(PushConstantBuffer_Pass));
			m_PushConstant_Stages = activeStages;

			VkPipelineLayoutCreateInfo bootstrapLayoutCI{};
			bootstrapLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			bootstrapLayoutCI.pushConstantRangeCount = 1;
			bootstrapLayoutCI.pPushConstantRanges = &pcRange;
			bootstrapLayoutCI.setLayoutCount = 0;
			bootstrapLayoutCI.pSetLayouts = nullptr;

			SEDX_VK_RESULT_ASSERT(vkCreatePipelineLayout(device->GetLogicalDevice(), &bootstrapLayoutCI, nullptr, reinterpret_cast<VkPipelineLayout *>(&m_Layout)));

			Debugging::SetResourceName(m_Layout, ResourceType::PipelineLayout, state.name);
		}

#pragma endregion

		if (state.IsCompute())
		{
			if (layout == nullptr)
			{
				SEDX_CORE_ERROR_TAG("Pipeline", "Compute pipeline '{}' requires a valid descriptor layout (set 0); refusing vkCreateComputePipelines to avoid VUID-VkComputePipelineCreateInfo-layout-07988", state.name ? state.name : "<unnamed>");
				return;
			}

			VkPipelineShaderStageCreateInfo computeStage{};
			bool hasComputeStage = false;

			// Compute pipelines must use a compute stage from the current PSO, not the global shader list.
			if (Shader* computeShader = state.shaders[static_cast<uint32_t>(StageType::Compute)])
			{
				if (computeShader->HasStage(StageType::Compute))
				{
					computeStage = computeShader->GetShaderStage(StageType::Compute)->GetStageCreateInfo();
					hasComputeStage = true;
				}
			}

			// Fallback: if a compute stage ended up in the aggregated list, pick that one explicitly.
			if (!hasComputeStage)
			{
				for (const VkPipelineShaderStageCreateInfo& stage : shaderStages)
				{
					if (stage.stage == VK_SHADER_STAGE_COMPUTE_BIT)
					{
						computeStage = stage;
						hasComputeStage = true;
						break;
					}
				}
			}

			if (!hasComputeStage)
			{
				SEDX_CORE_ERROR_TAG("Pipeline", "Compute PSO '{}' has no compute shader stage", state.name ? state.name : "<unnamed>");
				return;
			}

			if (computeStage.module == VK_NULL_HANDLE)
			{
				SEDX_CORE_ERROR_TAG("Pipeline", "Compute PSO '{}' has null VkShaderModule for its compute stage; refusing vkCreateComputePipelines", state.name ? state.name : "<unnamed>");
				return;
			}

			if (computeStage.pName == nullptr || computeStage.pName[0] == '\0')
			{
				SEDX_CORE_ERROR_TAG("Pipeline", "Compute PSO '{}' has invalid entry point name for compute stage; refusing vkCreateComputePipelines", state.name ? state.name : "<unnamed>");
				return;
			}

			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.layout = m_Layout;
			pipelineInfo.stage = computeStage;

			VkResult createResult = vkCreateComputePipelines(m_Device->GetLogicalDevice(), static_cast<VkPipelineCache>(GetPipelineCache()),
				1, &pipelineInfo, nullptr, reinterpret_cast<VkPipeline*>(&m_Pipeline));

			// Handle mixed cache/content cases where SPIR-V entrypoint symbols differ from
			// current Slang source naming conventions (e.g. cached .spv exposing "main").
			if (createResult != VK_SUCCESS && computeStage.pName && std::strcmp(computeStage.pName, "main") != 0)
			{
				SEDX_CORE_WARN_TAG("Pipeline", "Compute pipeline '{}' entry point '{}' failed; retrying with fallback entry point 'main'",
					state.name ? state.name : "<unnamed>", computeStage.pName);

				const char* fallbackEntryPoint = "main";
				pipelineInfo.stage.pName = fallbackEntryPoint;
				createResult = vkCreateComputePipelines(m_Device->GetLogicalDevice(), static_cast<VkPipelineCache>(GetPipelineCache()),
					1, &pipelineInfo, nullptr, reinterpret_cast<VkPipeline*>(&m_Pipeline));
			}

			SEDX_VK_RESULT_ASSERT(createResult);

			Debugging::SetResourceName(static_cast<void *>(m_Pipeline), ResourceType::Pipeline, state.name);
		}
		else if (state.IsGraphics())
		{
			// viewport & scissor
			std::vector<VkDynamicState> dynamicStates = {};
			VkPipelineDynamicStateCreateInfo dynamicState = {};
			VkViewport vkViewport = {};
			VkRect2D scissor = {};
			VkPipelineViewportStateCreateInfo viewportState = {};

#pragma region Dynamic States

			{
				// dynamic states
				{
					dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
					dynamicState.pNext = nullptr;
					dynamicState.flags = 0;
					dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
					if (m_State.IsGraphics())
					{
						dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
						dynamicStates.push_back(VK_DYNAMIC_STATE_CULL_MODE);
						// Only add fragment shading rate dynamic state when the PSO actually uses a VRS
						// input texture.  Adding it unconditionally causes validation errors on every
						// draw call for pipelines (e.g. ImGui) that never call vkCmdSetFragmentShadingRateKHR.
						if (Device::GetDeviceStatics().isShadingRateSupported && m_State.vrsInputTexture)
						{
							dynamicStates.push_back(VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR);
						}
					}

					dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
					dynamicState.pDynamicStates = dynamicStates.data();
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
				viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportState.viewportCount = 1;
				viewportState.pViewports = &vkViewport;
				viewportState.scissorCount = 1;
				viewportState.pScissors = &scissor;
			}

			// binding and vertex attribute descriptions
			std::vector<VkVertexInputBindingDescription> vertexInputBindingDescs;
			std::vector<VkVertexInputAttributeDescription> vertexAttributeDescs;
			Shader *shaderVertex = nullptr;
			{
				auto it = m_State.shaders[static_cast<uint32_t>(StageType::Vertex)];
				if (it && it->IsCompiled())
					shaderVertex = it;
			}

			Ref<InputLayout> vertexInputLayout;
			if (shaderVertex)
			{
				// determine vertex type from the stage's input attributes and build an InputLayout
				if (Ref<ShaderStage> vertStage = shaderVertex->GetShaderStage(StageType::Vertex))
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

			if (InputLayout *inputLayout = vertexInputLayout.Get())
			{
				const auto &attributeDescs = inputLayout->GetAttributeDescriptions();

#pragma region Vertex Buffer Bindings

				// vertex buffer (binding 0) - for per-vertex attributes like position, uv, color, normal, tangent
				bool hasVertexAttributes = false;
				bool isGeometryPassVertex = false;

				for (const auto &desc : attributeDescs)
				{
					// check for per-vertex attributes
					if (desc.name == "POSITION" || desc.name == "TEXCOORD" || desc.name == "COLOR" || desc.name == "NORMAL" || desc.name == "TANGENT")
					{
						hasVertexAttributes = true;

						// geometry pass vertices have normal or tangent
						if (desc.name == "NORMAL" || desc.name == "TANGENT")
						{
							isGeometryPassVertex = true;
						}
					}
				}

				// add vertex buffer binding if there are per-vertex attributes
				if (hasVertexAttributes)
				{
					vertexInputBindingDescs.push_back({
						.binding = 0,
						.stride = inputLayout->GetVertexSize(),
						.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
					});

					// add attribute descriptions for per-vertex attributes
					for (const auto &desc : attributeDescs)
					{
						if (desc.name == "POSITION" || desc.name == "TEXCOORD" || desc.name == "COLOR" ||
							desc.name == "NORMAL" || desc.name == "TANGENT")
						{
							vertexAttributeDescs.push_back({
								.location = desc.location,
								.binding = 0, // (vertex buffer)
								.format = desc.format,
								.offset = desc.offset
							});
						}
					}
				}

#pragma endregion
#pragma region Instance Buffer Bindings

				// instance buffer (binding 1) - for instance transform (position, rotation, scale)
				if (isGeometryPassVertex)
				{
					vertexInputBindingDescs.emplace_back(1, static_cast<uint32_t>(sizeof(Instance)), VK_VERTEX_INPUT_RATE_INSTANCE);

					uint32_t startIndex = static_cast<uint32_t>(vertexAttributeDescs.size());
					vertexAttributeDescs.emplace_back(startIndex++, 1, VK_FORMAT_R16_SFLOAT, static_cast<uint32_t>(offsetof(Instance, positionX)));
					vertexAttributeDescs.emplace_back(startIndex++, 1, VK_FORMAT_R16_SFLOAT, static_cast<uint32_t>(offsetof(Instance, positionY)));
					vertexAttributeDescs.emplace_back(startIndex++, 1, VK_FORMAT_R16_SFLOAT, static_cast<uint32_t>(offsetof(Instance, positionZ)));
					vertexAttributeDescs.emplace_back(startIndex++, 1, VK_FORMAT_R16_UINT, static_cast<uint32_t>(offsetof(Instance, normal_Oct)));
					vertexAttributeDescs.emplace_back(startIndex++, 1, VK_FORMAT_R8_UINT, static_cast<uint32_t>(offsetof(Instance, yaw_Packed)));
					vertexAttributeDescs.emplace_back(startIndex++, 1, VK_FORMAT_R8_UINT, static_cast<uint32_t>(offsetof(Instance, scale_Packed)));
				}
			}

#pragma endregion
#pragma region Vertex Input & Assembly State

			VkPipelineVertexInputStateCreateInfo vertexInputState = {};
			{
				vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindingDescs.size());
				vertexInputState.pVertexBindingDescriptions = vertexInputBindingDescs.data();
				vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescs.size());
				vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescs.data();
			}

			// input assembly state
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
			{
				inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssemblyState.topology = m_State.HasTessellation() ? VK_PRIMITIVE_TOPOLOGY_PATCH_LIST : m_State.primitiveTopology;
				inputAssemblyState.primitiveRestartEnable = VK_FALSE;
			}

#pragma endregion
#pragma region Tessellation State

			VkPipelineTessellationStateCreateInfo tessellationState = {};
			{
				tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
				tessellationState.patchControlPoints = m_State.HasTessellation() ? 3 : 1;
			}

#pragma endregion
#pragma region Rasterizer State

			VkPipelineRasterizationStateCreateInfo rasterizerState = {};
			if (m_State.rasterizerState)
			{
				rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterizerState.depthClampEnable = !m_State.rasterizerState->IsDepthClipEnabled();
				rasterizerState.rasterizerDiscardEnable = VK_FALSE;
				switch (m_State.rasterizerState->GetPolygonMode())
				{
					case PolygonMode::Solid:
						rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
						break;
					case PolygonMode::Wireframe:
						rasterizerState.polygonMode = VK_POLYGON_MODE_LINE;
						break;
					case PolygonMode::Point:
						rasterizerState.polygonMode = VK_POLYGON_MODE_POINT;
						break;
					default:
						SEDX_CORE_WARN_TAG("Pipeline", "Invalid PolygonMode '{}' for pipeline '{}'; defaulting to VK_POLYGON_MODE_FILL",
							static_cast<uint32_t>(m_State.rasterizerState->GetPolygonMode()),
							state.name ? state.name : "<unnamed>");
						rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
						break;
				}
				rasterizerState.lineWidth = m_State.rasterizerState->GetLineWidth();
				rasterizerState.cullMode = static_cast<uint32_t>(CullMode::Back);
				rasterizerState.frontFace = VK_FRONT_FACE_CLOCKWISE;
				rasterizerState.depthBiasEnable = m_State.rasterizerState->GetDepthBias() != 0.0f ? VK_TRUE : VK_FALSE;
				rasterizerState.depthBiasConstantFactor = floor(m_State.rasterizerState->GetDepthBias() * (float)(1 << 24));
				rasterizerState.depthBiasClamp = m_State.rasterizerState->GetDepthBiasClamp();
				rasterizerState.depthBiasSlopeFactor = m_State.rasterizerState->GetDepthBiasSlope();
			}

			// multisampling
			VkPipelineMultisampleStateCreateInfo multisampleState = {};
			{
				multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampleState.sampleShadingEnable = VK_FALSE;
				multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			}

#pragma endregion
#pragma region Blend State

			VkPipelineColorBlendStateCreateInfo colorBlendState = {};
			std::vector<VkPipelineColorBlendAttachmentState> blendStateAttachments;
			if (m_State.blendState)
			{
				// attachments
				{
					// same blend state for all
					VkPipelineColorBlendAttachmentState blendStateAttachment = {};
					blendStateAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
					blendStateAttachment.blendEnable = m_State.blendState->IsBlendEnabled() ? VK_TRUE : VK_FALSE;
					blendStateAttachment.srcColorBlendFactor = m_State.blendState->GetSrcColor();
					blendStateAttachment.dstColorBlendFactor = m_State.blendState->GetDstColor();
					blendStateAttachment.colorBlendOp = m_State.blendState->GetColorOp();
					blendStateAttachment.srcAlphaBlendFactor = m_State.blendState->GetSrcAlpha();
					blendStateAttachment.dstAlphaBlendFactor = m_State.blendState->GetDstAlpha();
					blendStateAttachment.alphaBlendOp = m_State.blendState->GetAlphaOp();

					// swapchain
					if (m_State.renderTarget_Swapchain)
					{
						blendStateAttachments.push_back(blendStateAttachment);
					}

					// render target(s)
					for (uint8_t i = 0; i < MAX_RENDER_TARGET_COUNT; i++)
					{
						if (m_State.renderTarget_ColorTextures[i] != nullptr)
						{
							blendStateAttachments.push_back(blendStateAttachment);
						}
					}
				}

				colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				colorBlendState.logicOpEnable = VK_FALSE;
				colorBlendState.logicOp = VK_LOGIC_OP_COPY;
				colorBlendState.attachmentCount = static_cast<uint32_t>(blendStateAttachments.size());
				colorBlendState.pAttachments = blendStateAttachments.data();
				colorBlendState.blendConstants[0] = m_State.blendState->GetBlendFactor();
				colorBlendState.blendConstants[1] = m_State.blendState->GetBlendFactor();
				colorBlendState.blendConstants[2] = m_State.blendState->GetBlendFactor();
				colorBlendState.blendConstants[3] = m_State.blendState->GetBlendFactor();
			}

#pragma endregion
#pragma region Depth-Stencil State

			VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
			if (m_State.depthStencil_State)
			{
				depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				depthStencilState.depthTestEnable = m_State.depthStencil_State->IsDepthTestEnabled();
				depthStencilState.depthWriteEnable = m_State.depthStencil_State->IsDepthWriteEnabled();
				depthStencilState.depthCompareOp = m_State.depthStencil_State->GetDepthCompareOp();
				depthStencilState.stencilTestEnable = m_State.depthStencil_State->IsStencilTestEnabled();
				depthStencilState.front.compareOp = m_State.depthStencil_State->GetStencilCompFunc();
				depthStencilState.front.failOp = m_State.depthStencil_State->GetStencilFailOp();
				depthStencilState.front.depthFailOp = m_State.depthStencil_State->GetStencilDepthFailOp();
				depthStencilState.front.passOp = m_State.depthStencil_State->GetStencilPassOp();
				depthStencilState.front.compareMask = m_State.depthStencil_State->GetStencilReadMask();
				depthStencilState.front.writeMask = m_State.depthStencil_State->GetStencilWriteMask();
				depthStencilState.front.reference = 1;
				depthStencilState.back = depthStencilState.front;
				depthStencilState.minDepthBounds = 1.0f; // functionality of DirectX with reverse-z
				depthStencilState.maxDepthBounds = 0.0f; // functionality of DirectX with reverse-z
			}

#pragma endregion
#pragma region Pipeline

			{
				// enable dynamic rendering - VK_KHR_dynamic_rendering
				// this means no render passes and no frame buffer objects
				VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {};
				VkPipelineFragmentShadingRateStateCreateInfoKHR fragment_ShadingRateState = {};
				std::vector<VkFormat> attachmentFormats_Color;
				VkFormat attachFmt_Depth = VK_FORMAT_UNDEFINED;
				VkFormat attachFmt_Stencil = VK_FORMAT_UNDEFINED;
				{
					// swapchain buffer as a render target
					if (m_State.renderTarget_Swapchain)
					{
						attachmentFormats_Color.push_back(m_State.renderTarget_Swapchain->GetImageFormat());
						// If the PSO has no explicit depth texture but targets a swapchain, inherit
						// the swapchains depth format so the pipeline declaration matches the active
						// dynamic render pass (which always has a depth attachment).
						if (!m_State.renderTarget_DepthTexture)
						{
							attachFmt_Depth = m_State.renderTarget_Swapchain->GetDepthFormat();
							attachFmt_Stencil = VK_FORMAT_UNDEFINED; // stencil kept separate if needed
						}
					}
					else // regular render target(s)
					{
						for (uint32_t i = 0; i < MAX_RENDER_TARGET_COUNT; i++)
						{
							ImageResource *texture = m_State.renderTarget_ColorTextures[i];
							if (texture == nullptr)
								break;

							attachmentFormats_Color.push_back(texture->GetFormat());
						}
					}

					// depth
					if (m_State.renderTarget_DepthTexture)
					{
						ImageResource *texDepth = m_State.renderTarget_DepthTexture;
						attachFmt_Depth = texDepth->GetFormat();
						attachFmt_Stencil =
							texDepth->IsStencilFormat() ? attachFmt_Depth : VK_FORMAT_UNDEFINED;
					}

					// variable rate shading
					if (m_State.vrsInputTexture)
					{
						fragment_ShadingRateState.sType = VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR;
						fragment_ShadingRateState.combinerOps[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
						fragment_ShadingRateState.combinerOps[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
						fragment_ShadingRateState.fragmentSize = {.width = 1, .height = 1};

						pipelineRenderingCreateInfo.pNext = &fragment_ShadingRateState;
					}

					// put everything together
					pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
					pipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(attachmentFormats_Color.size());
					pipelineRenderingCreateInfo.pColorAttachmentFormats = attachmentFormats_Color.data();
					pipelineRenderingCreateInfo.depthAttachmentFormat = attachFmt_Depth;
					pipelineRenderingCreateInfo.stencilAttachmentFormat = attachFmt_Stencil;
					pipelineRenderingCreateInfo.viewMask = m_State.isMultiview ? 0b11 : 0;
				}

#pragma endregion
#pragma region Graphics Pipeline Creation

				{
					VkGraphicsPipelineCreateInfo pipelineInfo = {};
					pipelineInfo.pNext = &pipelineRenderingCreateInfo;
					pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
					pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
					pipelineInfo.pStages = shaderStages.data();
					pipelineInfo.pVertexInputState = &vertexInputState;
					pipelineInfo.pInputAssemblyState = &inputAssemblyState;
					pipelineInfo.pTessellationState = &tessellationState;
					pipelineInfo.pDynamicState = &dynamicState;
					pipelineInfo.pViewportState = &viewportState;
					pipelineInfo.pRasterizationState = &rasterizerState;
					pipelineInfo.pMultisampleState = &multisampleState;
					pipelineInfo.pColorBlendState = &colorBlendState;
					pipelineInfo.pDepthStencilState = &depthStencilState;
					pipelineInfo.layout = m_Layout;
					pipelineInfo.flags = m_State.vrsInputTexture ? VK_PIPELINE_CREATE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR : 0;

					VkResult createResult = vkCreateGraphicsPipelines(
						m_Device->GetLogicalDevice(),
						static_cast<VkPipelineCache>(GetPipelineCache()),
						1,
						&pipelineInfo,
						nullptr,
						reinterpret_cast<VkPipeline*>(&m_Pipeline));

					if (createResult != VK_SUCCESS)
					{
						std::vector<VkPipelineShaderStageCreateInfo> fallbackStages = shaderStages;
						bool hasNonMainEntryPoint = false;
						for (auto& stageCI : fallbackStages)
						{
							if (!stageCI.pName || std::strcmp(stageCI.pName, "main") == 0)
								continue;

							hasNonMainEntryPoint = true;
							stageCI.pName = "main";
						}

						if (hasNonMainEntryPoint)
						{
							SEDX_CORE_WARN_TAG("Pipeline", "Graphics pipeline '{}' entry point mismatch; retrying all shader stages with fallback entry point 'main'",
								state.name ? state.name : "<unnamed>");

							pipelineInfo.pStages = fallbackStages.data();
							createResult = vkCreateGraphicsPipelines(
								m_Device->GetLogicalDevice(),
								static_cast<VkPipelineCache>(GetPipelineCache()),
								1,
								&pipelineInfo,
								nullptr,
								reinterpret_cast<VkPipeline*>(&m_Pipeline));
						}
					}

					SEDX_VK_RESULT_ASSERT(createResult);

					Debugging::SetResourceName(m_Pipeline, ResourceType::Pipeline, state.name);
				}
#pragma endregion
			}
#pragma endregion
		}

		if (m_Pipeline == VK_NULL_HANDLE)
		{
			SEDX_CORE_WARN_TAG("Pipeline", "Pipeline '{}' was not created (state: graphics={}, compute={})", state.name ? state.name : "<unnamed>", state.IsGraphics(), state.IsCompute());

			// Prevent QueueManager deferred deletion from later destroying a VK_NULL_HANDLE-backed
			// or already-invalid pipeline state while command buffers may still reference previous
			// pipelines. Mark this object as already destroyed so its destructor is a no-op.
			m_Destroyed = true;
			return;
		}

		m_Destroyed = false;
	}

	Pipeline::~Pipeline()
	{
		// pipeline cache - save to disk before destroying
		SavePipelineCache();
		if (s_PipelineCache && m_Device)
		{
			vkDestroyPipelineCache(m_Device->GetLogicalDevice(), s_PipelineCache, nullptr);
			s_PipelineCache = nullptr;
		}

		DestroyPipeline();
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
			DestroyPipeline();
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
	
	void Pipeline::DestroyPipeline(const VkDevice device)
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
					logicalDevice = dev->GetLogicalDevice();
			}
		}

		if (logicalDevice != VK_NULL_HANDLE)
		{
			if (m_Pipeline != VK_NULL_HANDLE)
			{
				QueueManager::AddDeletionQueue(ResourceType::Pipeline, m_Pipeline);
				m_Pipeline = VK_NULL_HANDLE;
			}

			if (m_Layout != VK_NULL_HANDLE)
			{
				QueueManager::AddDeletionQueue(ResourceType::PipelineLayout, m_Layout);
				m_Layout = VK_NULL_HANDLE;
			}
		}

		m_Destroyed = true;
		SEDX_CORE_TRACE_TAG("Pipeline", "Pipeline destroyed");
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
