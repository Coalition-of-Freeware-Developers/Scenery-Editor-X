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
 * ui_impl.cpp
 * -------------------------------------------------------
 * Created: 15/03/2026
 * -------------------------------------------------------
 */
#include "ui_impl.h"
#include "source/imgui/imgui_internal.h"
#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/window/monitor_data.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/blend_states.h>
#include <SceneryEditorX/renderer/vulkan/buffer.h>
#include <SceneryEditorX/renderer/vulkan/depth_stencil.h>
#include <SceneryEditorX/renderer/vulkan/queue_manager.h>
#include <SceneryEditorX/renderer/vulkan/rasterizer.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <SceneryEditorX/renderer/vulkan/swapchain.h>
#include <SceneryEditorX/renderer/vulkan/debug/graphics_debug.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader_stage.h>

// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	ViewportResources g_ViewportData;
	Ref<ImageResource> g_FontAtlas;
	Ref<DepthStencilState> g_DepthStencil_State;
	Ref<RasterizerState> g_Rasterizer_State;
	Ref<BlendState> g_BlendState;
	Ref<Shader> g_VertexShader;
	Ref<Shader> g_FragmentShader;

	// Raw Vulkan objects for the ImGui pipeline that matches ui.slang:
	//   push constants: { float2 scale; float2 translate; }  (16 bytes)
	//   Set 0, Binding 0: Sampler2D font  (combined image sampler)
	VkDescriptorPool      g_ImGuiDescriptorPool       = VK_NULL_HANDLE;
	VkDescriptorSetLayout g_ImGuiDescriptorSetLayout  = VK_NULL_HANDLE;
	VkDescriptorSet       g_ImGuiFontDescriptorSet    = VK_NULL_HANDLE;
	VkSampler             g_ImGuiFontSampler          = VK_NULL_HANDLE;
	VkPipelineLayout      g_ImGuiPipelineLayout       = VK_NULL_HANDLE;
	VkPipeline            g_ImGuiPipeline             = VK_NULL_HANDLE;

	void DestroyResources()
	{
		const VkDevice device = RenderContext::Get()->GetDevice()->GetLogicalDevice();

		// Destroy raw Vulkan ImGui pipeline objects
		if (g_ImGuiPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(device, g_ImGuiPipeline, nullptr);
			g_ImGuiPipeline = VK_NULL_HANDLE;
		}
		if (g_ImGuiPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(device, g_ImGuiPipelineLayout, nullptr);
			g_ImGuiPipelineLayout = VK_NULL_HANDLE;
		}
		if (g_ImGuiFontSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(device, g_ImGuiFontSampler, nullptr);
			g_ImGuiFontSampler = VK_NULL_HANDLE;
		}
		if (g_ImGuiDescriptorPool != VK_NULL_HANDLE)
		{
			// Descriptor sets are freed implicitly when the pool is destroyed
			g_ImGuiFontDescriptorSet = VK_NULL_HANDLE;
			vkDestroyDescriptorPool(device, g_ImGuiDescriptorPool, nullptr);
			g_ImGuiDescriptorPool = VK_NULL_HANDLE;
		}
		if (g_ImGuiDescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(device, g_ImGuiDescriptorSetLayout, nullptr);
			g_ImGuiDescriptorSetLayout = VK_NULL_HANDLE;
		}

		g_FontAtlas = nullptr;
		g_DepthStencil_State = nullptr;
		g_Rasterizer_State = nullptr;
		g_BlendState = nullptr;
		g_VertexShader = nullptr;
		g_FragmentShader = nullptr;

		for (auto &ptr : g_ViewportData.indexBuffers)
		{
			ptr = nullptr;
		}

		for (auto &ptr : g_ViewportData.vertexBuffers)
		{
			ptr = nullptr;
		}
	}

	void Initialize()
	{
		// create required objects
		{
			g_ViewportData = ViewportResources("imgui",  Renderer::GetSwapChain());
			g_DepthStencil_State = SceneryEditorX::CreateRef<DepthStencilState>(DepthStencilSpec{ false, false, VK_COMPARE_OP_ALWAYS });
			g_Rasterizer_State = SceneryEditorX::CreateRef<RasterizerState>(RasterStateSpec{ PolygonMode::Solid, true });

			g_BlendState = SceneryEditorX::CreateRef<BlendState>(BlendStateSpec{
										true,
										VK_BLEND_FACTOR_SRC_ALPHA,           // src color
										VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // dst color
										VK_BLEND_OP_ADD,                     // color op
										VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // src alpha
										VK_BLEND_FACTOR_ZERO,                // dst alpha
										VK_BLEND_OP_ADD,                     // alpha op
										0.0f                                  // blend factor
			});
	
			// compile shaders
			{
				const std::string shaderPath = ResourceCache::GetResourceDirectory(ResourceDirectory::Shaders) + "/ui.slang";
	
				//bool async = false;
	
				g_VertexShader = CreateRef<Shader>();
				g_VertexShader->AddShaderStage(StageType::Vertex, shaderPath);
	
				g_FragmentShader = CreateRef<Shader>();
				g_FragmentShader->AddShaderStage(StageType::Fragment, shaderPath);
			}
		}
	
		// font atlas
		{
			unsigned char *pixels = nullptr;
			int atlasWidth = 0;
			int atlasHeight = 0;
			int bpp = 0;
			ImGuiIO &io = ImGui::GetIO();
			io.Fonts->GetTexDataAsRGBA32(&pixels, &atlasWidth, &atlasHeight, &bpp);
	
			// copy pixel data
			std::vector<Slice> texture_data;
			std::vector<std::byte> &mip = texture_data.emplace_back().mips.emplace_back().bytes;
			const uint32_t size = atlasWidth * atlasHeight * bpp;
			mip.resize(size);
			mip.reserve(size);
			memcpy(&mip[0], reinterpret_cast<std::byte *>(pixels), size);
	
			ImgResourceSpec spec{};
			spec.type = ImageType::Type2D;
			spec.width = atlasWidth;
			spec.height = atlasHeight;
			spec.depth = 1;
			spec.mipCount = 1;
			spec.format = VK_FORMAT_R8G8B8A8_UNORM;
			spec.flags = ShaderViews;
			spec.name = "imgui_font_atlas";
	
			// upload texture to graphics system
			g_FontAtlas = CreateRef<ImageResource>(spec, std::move(texture_data));
			io.Fonts->TexID = reinterpret_cast<ImTextureID>(g_FontAtlas.Get());
		}

		// Build dedicated Vulkan objects for the ImGui pipeline that match ui.slang:
		//   push constants: { float2 scale; float2 translate; }  (16 bytes, both stages)
		//   Set 0, Binding 0: Sampler2D font  (combined image sampler)
		// These bypass the CommandList abstraction which cannot handle this descriptor layout.
		{
			const VkDevice device = RenderContext::Get()->GetDevice()->GetLogicalDevice();
			Swapchain *swapchain = Renderer::GetSwapChain();

			// Linear sampler for the font atlas
			{
				VkSamplerCreateInfo samplerCI{};
				samplerCI.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				samplerCI.magFilter    = VK_FILTER_LINEAR;
				samplerCI.minFilter    = VK_FILTER_LINEAR;
				samplerCI.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				samplerCI.maxAnisotropy = 1.0f;
				samplerCI.minLod       = -1000.0f;
				samplerCI.maxLod       = 1000.0f;
				vkCreateSampler(device, &samplerCI, nullptr, &g_ImGuiFontSampler);
			}

			// Descriptor set layout: binding 0 = combined image sampler
			{
				VkDescriptorSetLayoutBinding binding{};
				binding.binding            = 0;
				binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.descriptorCount    = 1;
				binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
				binding.pImmutableSamplers = &g_ImGuiFontSampler;

				VkDescriptorSetLayoutCreateInfo layoutCI{};
				layoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutCI.bindingCount = 1;
				layoutCI.pBindings    = &binding;
				vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &g_ImGuiDescriptorSetLayout);
			}

			// Descriptor pool and font descriptor set
			{
				VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
				VkDescriptorPoolCreateInfo poolCI{};
				poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				poolCI.maxSets       = 1;
				poolCI.poolSizeCount = 1;
				poolCI.pPoolSizes    = &poolSize;
				vkCreateDescriptorPool(device, &poolCI, nullptr, &g_ImGuiDescriptorPool);

				VkDescriptorSetAllocateInfo allocInfo{};
				allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool     = g_ImGuiDescriptorPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts        = &g_ImGuiDescriptorSetLayout;
				vkAllocateDescriptorSets(device, &allocInfo, &g_ImGuiFontDescriptorSet);
			}

			// Pipeline layout: one descriptor set + one push constant range (scale.xy + translate.xy)
			{
				VkPushConstantRange pcRange{};
				pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				pcRange.offset     = 0;
				pcRange.size       = sizeof(float) * 4; // float2 scale + float2 translate

				VkPipelineLayoutCreateInfo layoutCI{};
				layoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				layoutCI.setLayoutCount         = 1;
				layoutCI.pSetLayouts            = &g_ImGuiDescriptorSetLayout;
				layoutCI.pushConstantRangeCount = 1;
				layoutCI.pPushConstantRanges    = &pcRange;
				vkCreatePipelineLayout(device, &layoutCI, nullptr, &g_ImGuiPipelineLayout);
			}

			// Build the VkPipeline using the compiled ui.slang shader stages
			{
				Ref<ShaderStage> vsStage   = g_VertexShader   ? g_VertexShader->GetShaderStage(StageType::Vertex)   : nullptr;
				Ref<ShaderStage> fragStage = g_FragmentShader ? g_FragmentShader->GetShaderStage(StageType::Fragment) : nullptr;

				if (vsStage && fragStage)
				{
					VkPipelineShaderStageCreateInfo stages[2] = {
						vsStage->GetStageCreateInfo(),
						fragStage->GetStageCreateInfo()
					};

					// Vertex input: matches ImDrawVert layout
					// Binding 0: pos (float2) @ 0, uv (float2) @ 8, color (uint8 x4) @ 16
					VkVertexInputBindingDescription vertBind{};
					vertBind.binding   = 0;
					vertBind.stride    = sizeof(ImDrawVert);
					vertBind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

					VkVertexInputAttributeDescription vertAttrs[3]{};
					vertAttrs[0] = { 0, 0, VK_FORMAT_R32G32_SFLOAT,       static_cast<uint32_t>(offsetof(ImDrawVert, pos)) };
					vertAttrs[1] = { 1, 0, VK_FORMAT_R32G32_SFLOAT,       static_cast<uint32_t>(offsetof(ImDrawVert, uv))  };
					vertAttrs[2] = { 2, 0, VK_FORMAT_R8G8B8A8_UNORM,      static_cast<uint32_t>(offsetof(ImDrawVert, col)) };

					VkPipelineVertexInputStateCreateInfo vertexInput{};
					vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
					vertexInput.vertexBindingDescriptionCount   = 1;
					vertexInput.pVertexBindingDescriptions      = &vertBind;
					vertexInput.vertexAttributeDescriptionCount = 3;
					vertexInput.pVertexAttributeDescriptions    = vertAttrs;

					VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
					inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
					inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

					VkPipelineViewportStateCreateInfo viewportState{};
					viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
					viewportState.viewportCount = 1;
					viewportState.scissorCount  = 1;

					VkPipelineRasterizationStateCreateInfo rasterizer{};
					rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
					rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
					rasterizer.cullMode    = VK_CULL_MODE_NONE;
					rasterizer.frontFace   = VK_FRONT_FACE_CLOCKWISE;
					rasterizer.lineWidth   = 1.0f;

					VkPipelineMultisampleStateCreateInfo msaa{};
					msaa.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
					msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

					VkPipelineColorBlendAttachmentState blendAttach{};
					blendAttach.blendEnable         = VK_TRUE;
					blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
					blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					blendAttach.colorBlendOp        = VK_BLEND_OP_ADD;
					blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
					blendAttach.alphaBlendOp        = VK_BLEND_OP_ADD;
					blendAttach.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
													  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

					VkPipelineColorBlendStateCreateInfo colorBlend{};
					colorBlend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
					colorBlend.attachmentCount = 1;
					colorBlend.pAttachments    = &blendAttach;

					VkPipelineDepthStencilStateCreateInfo depthStencil{};
					depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
					depthStencil.depthTestEnable  = VK_FALSE;
					depthStencil.depthWriteEnable = VK_FALSE;

					VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
					VkPipelineDynamicStateCreateInfo dynState{};
					dynState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
					dynState.dynamicStateCount = 2;
					dynState.pDynamicStates    = dynStates;

					// Dynamic rendering: match the active render pass formats
					VkFormat colorFmt = swapchain ? swapchain->GetImageFormat() : VK_FORMAT_B8G8R8A8_SRGB;
					VkFormat depthFmt = swapchain ? swapchain->GetDepthFormat() : VK_FORMAT_D24_UNORM_S8_UINT;

					VkPipelineRenderingCreateInfoKHR renderingCI{};
					renderingCI.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
					renderingCI.colorAttachmentCount    = 1;
					renderingCI.pColorAttachmentFormats = &colorFmt;
					renderingCI.depthAttachmentFormat   = depthFmt;

					VkGraphicsPipelineCreateInfo pipelineCI{};
					pipelineCI.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
					pipelineCI.pNext               = &renderingCI;
					pipelineCI.stageCount          = 2;
					pipelineCI.pStages             = stages;
					pipelineCI.pVertexInputState   = &vertexInput;
					pipelineCI.pInputAssemblyState = &inputAssembly;
					pipelineCI.pViewportState      = &viewportState;
					pipelineCI.pRasterizationState = &rasterizer;
					pipelineCI.pMultisampleState   = &msaa;
					pipelineCI.pColorBlendState    = &colorBlend;
					pipelineCI.pDepthStencilState  = &depthStencil;
					pipelineCI.pDynamicState       = &dynState;
					pipelineCI.layout              = g_ImGuiPipelineLayout;

					vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &g_ImGuiPipeline);
					SEDX_CORE_ASSERT(g_ImGuiPipeline != VK_NULL_HANDLE, "Failed to create ImGui Vulkan pipeline");
				}
			}
		}
	
		/*
		// setup back-end capabilities flags
		ImGuiIO &io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
		io.BackendRendererName = "UI Renderer";
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			InitializePlatformInterface();
		}*/
	}

	void Shutdown()
	{
		/**
		 * Release all GPU resources (Ref<Shader>, Ref<ImageResource>, etc.) while
		 * the VkDevice and logging system are still alive.  Without this call,
		 * g_VertexShader / g_FragmentShader are file-scope statics whose Ref<>
		 * destructors fire in the CRT static-dtor phase â€” after RenderContext and
		 * spdlog have already been destroyed â€” causing the 0x50 access violation.
		 */
		DestroyResources();
		ImGui::DestroyPlatformWindows();
	}

	bool IsWindowFocused(const char *windowName, const bool checkWindow)
	{
		ImGuiWindow* currentNavWindow = GImGui->NavWindow;

		if (checkWindow)
		{
			// Get the actual nav window (not e.g a table)
			ImGuiWindow* lastWindow = nullptr;
			while (lastWindow != currentNavWindow)
			{
				lastWindow = currentNavWindow;
				currentNavWindow = currentNavWindow->RootWindow;
			}
		}

		return currentNavWindow == ImGui::FindWindowByName(windowName);
	}

	void Render(ImDrawData *drawData, WindowData *windowData, const bool clear)
	{
		SEDX_CORE_ASSERT(!windowData, "Window data is null");
		if (!drawData || drawData->TotalVtxCount <= 0 || drawData->TotalIdxCount <= 0)
			return;

		// skip the first two frames to let the renderer fully initialize.
		// frame 0: pipeline layouts and descriptor sets are still being created.
		// frame 1: bindless draw_data buffer descriptor may not have been written yet.
		uint64_t frame = Renderer::GetFrameNumber();
		if (frame < 2)
			return;

		// get resources
		bool isMainWindow				= windowData == nullptr;
		ViewportResources* vpResources	= isMainWindow ? &g_ViewportData : windowData->viewportResources.get();
		Swapchain* swapchain			= isMainWindow ? Renderer::GetSwapChain() : windowData->swapchain.Get();
		uint32_t bufferIndex			= vpResources->bufferIndex;
		vpResources->bufferIndex		= (vpResources->bufferIndex + 1) % BUFFER_COUNT;
		Buffer* vertexBuffer			= vpResources->vertexBuffers[bufferIndex].get();
		Buffer* indexBuffer				= vpResources->indexBuffers[bufferIndex].get();
		CommandList* cmdList			= Renderer::GetCommandListPresent();

		// if that's a child window, update it's swapchain and give it a command list
		if (!isMainWindow)
		{
			swapchain->AcquireNextImage();
			Ref<Device> device = RenderContext::Get()->GetDevice();

			auto queueManager = device ? device->GetQueueManager() : nullptr;
			windowData->cmdList = queueManager ? queueManager->NextCommandList() : nullptr;
			cmdList = windowData->cmdList;

			windowData->cmdList->Begin();
			
		}
	
		// when the engine splash screen is shown, the command list is not valid as the renderer is initializing
		if (!cmdList || cmdList->GetState() != CommandState::Recording)
			return;
	
		// update vertex and index buffers
		{
			// grow vertex buffer as needed
			if (vertexBuffer->GetElementCount() < static_cast<uint32_t>(drawData->TotalVtxCount))
			{
				const uint32_t count = vertexBuffer->GetElementCount();
				const uint32_t count_new = drawData->TotalVtxCount + 15000;
				vpResources->vertexBuffers[bufferIndex] = CreateScope<Buffer>(sizeof(ImDrawVert), count_new, nullptr, true, vertexBuffer->GetObjectName().c_str());

				if (count != 0)
				{
					EDITOR_INFO_TAG("UI Implementation", "Vertex buffer has been re-allocated to fit {} vertices", count_new);
				}
			}
	
			// grow index buffer as needed
			if (indexBuffer->GetElementCount() < static_cast<uint32_t>(drawData->TotalIdxCount))
			{
				const uint32_t count = indexBuffer->GetElementCount();
				const uint32_t count_new = drawData->TotalIdxCount + 30000;
				vpResources->indexBuffers[bufferIndex] = CreateScope<Buffer>(sizeof(ImDrawIdx), count_new, nullptr, true, indexBuffer->GetObjectName().c_str());

				if (count != 0)
				{
					EDITOR_INFO_TAG("UI Implementation", "Index buffer has been re-allocated to fit {} indices", count_new);
				}
			}

			// copy all imgui vertices into a single buffer
			ImDrawVert *vtx_dst = static_cast<ImDrawVert *>(vertexBuffer->GetMappedData());
			ImDrawIdx *idx_dst = static_cast<ImDrawIdx *>(indexBuffer->GetMappedData());
			if (vtx_dst && idx_dst)
			{
				for (auto i = 0; i < drawData->CmdListsCount; i++)
				{
					const ImDrawList *imguiCmdList = drawData->CmdLists[i];

					memcpy(vtx_dst, imguiCmdList->VtxBuffer.Data, imguiCmdList->VtxBuffer.Size * sizeof(ImDrawVert));
					memcpy(idx_dst, imguiCmdList->IdxBuffer.Data, imguiCmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

					vtx_dst += imguiCmdList->VtxBuffer.Size;
					idx_dst += imguiCmdList->IdxBuffer.Size;
				}
			}
		}

		// set pipeline state
		static PipelineState pso			= {};
		pso.name							= "imgui";
		pso.shaders[static_cast<uint32_t>(StageType::Vertex)]	= g_VertexShader.Get();
		pso.shaders[static_cast<uint32_t>(StageType::Fragment)]	= g_FragmentShader.Get();
		pso.rasterizerState                 = g_Rasterizer_State.Get();
		pso.blendState                      = g_BlendState.Get();
		pso.depthStencil_State              = g_DepthStencil_State.Get();
		pso.renderTarget_Swapchain          = swapchain;
		pso.clearColor[0]                   = clear ? Color::Black() : COLOR_DONT_CARE;

		// Bind the dedicated ImGui pipeline and descriptor set directly.
		// This bypasses CommandList::SetPipelineState() which goes through the bootstrap /
		// bindless path that is incompatible with ui.slang's simple descriptor layout.
		{
			VkCommandBuffer cb = cmdList->GetCommandBuffer();
			SEDX_CORE_ASSERT(cb != VK_NULL_HANDLE, "ImGui: command buffer is null");

			if (g_ImGuiPipeline == VK_NULL_HANDLE || g_ImGuiPipelineLayout == VK_NULL_HANDLE)
			{
				SEDX_CORE_WARN_TAG("UI", "ImGui pipeline not ready, skipping render");
				return;
			}

			vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_ImGuiPipeline);
			vkCmdSetCullMode(cb, VK_CULL_MODE_NONE);

			// Set viewport to cover the full display
			{
				VkViewport vp{};
				vp.x        = drawData->DisplayPos.x;
				vp.y        = drawData->DisplayPos.y;
				vp.width    = drawData->DisplaySize.x;
				vp.height   = drawData->DisplaySize.y;
				vp.minDepth = 0.0f;
				vp.maxDepth = 1.0f;
				vkCmdSetViewport(cb, 0, 1, &vp);
			}

			// Bind the vertex and index buffers
			{
				VkBuffer vb = vertexBuffer->Get();
				VkDeviceSize offset = 0;
				vkCmdBindVertexBuffers(cb, 0, 1, &vb, &offset);
				vkCmdBindIndexBuffer(cb, indexBuffer->Get(), 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
			}

			// Push scale and translate as push constants (ui.slang: float2 scale, float2 translate)
			{
				float scale[2]     = { 2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y };
				float translate[2] = { -1.0f - drawData->DisplayPos.x * scale[0], -1.0f - drawData->DisplayPos.y * scale[1] };
				vkCmdPushConstants(cb, g_ImGuiPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2, scale);
				vkCmdPushConstants(cb, g_ImGuiPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
			}

			// Render all ImGui draw commands
			uint32_t global_vtx_offset = 0;
			uint32_t global_idx_offset = 0;
			for (uint32_t i = 0; std::cmp_less(i, drawData->CmdListsCount); i++)
			{
				ImDrawList *cmdListImgui = drawData->CmdLists[i];

				for (uint32_t cmd_i = 0; std::cmp_less(cmd_i, cmdListImgui->CmdBuffer.Size); cmd_i++)
				{
					const ImDrawCmd *pcmd = &cmdListImgui->CmdBuffer[cmd_i];

					if (pcmd->UserCallback != nullptr)
					{
						pcmd->UserCallback(cmdListImgui, pcmd);
					}
					else
					{
						// Bind the font descriptor set (or per-draw texture if provided)
						VkDescriptorSet descSet = g_ImGuiFontDescriptorSet;
						if (ImageResource *texture = reinterpret_cast<ImageResource *>(pcmd->GetTexID()))
						{
							// Per-draw textures are not yet supported via raw Vulkan path;
							// fall back to the font atlas so at least text renders.
							// TODO: allocate per-texture descriptor sets from a larger pool.
						}
						vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
							g_ImGuiPipelineLayout, 0, 1, &descSet, 0, nullptr);

						// Set scissor rectangle
						VkRect2D scissor{};
						scissor.offset.x      = static_cast<int32_t>(std::max(pcmd->ClipRect.x - drawData->DisplayPos.x, 0.0f));
						scissor.offset.y      = static_cast<int32_t>(std::max(pcmd->ClipRect.y - drawData->DisplayPos.y, 0.0f));
						scissor.extent.width  = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
						scissor.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y);
						vkCmdSetScissor(cb, 0, 1, &scissor);

						vkCmdDrawIndexed(cb, pcmd->ElemCount, 1,
							pcmd->IdxOffset + global_idx_offset,
							static_cast<int32_t>(pcmd->VtxOffset + global_vtx_offset), 0);
					}
				}

				global_idx_offset += static_cast<uint32_t>(cmdListImgui->IdxBuffer.Size);
				global_vtx_offset += static_cast<uint32_t>(cmdListImgui->VtxBuffer.Size);
			}
		}

		// for child windows, submit and prepare for presentation
		if (!isMainWindow)
		{
			cmdList->Submit(nullptr, true);
		}
	}
	  
	void WindowCreate(ImGuiViewport *viewport)
	{
		SEDX_CORE_ASSERT(viewport->PlatformHandle, "Viewport platform handle is null");
	
		// NOTE: platformHandle is SDL_Window, PlatformHandleRaw is HWND
		SDL_Window *sdl_window = SDL_GetWindowFromID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(viewport->PlatformHandle)));
	
		WindowData *window = new WindowData();
		SwapchainSpec spec{};
		spec.sdlWindow = sdl_window;
		spec.width = static_cast<uint32_t>(viewport->Size.x);
		spec.height = static_cast<uint32_t>(viewport->Size.y);
		spec.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // vsync on to avoid tearing in imgui viewports
		spec.bufferCount = 2;
		spec.name = "child_window_swapchain";
		window->swapchain = CreateRef<Swapchain>(spec);
	
		window->viewportResources = CreateScope<ViewportResources>("imgui_child_window", window->swapchain.Get());
		viewport->RendererUserData = window;
		SEDX_CORE_TRACE_TAG("WindowCreate", "Created window called for {}", viewport->ID);
	}

	void WindowDestroy(ImGuiViewport *viewport)
	{
		if (WindowData *window = static_cast<WindowData *>(viewport->RendererUserData))
		{
			viewport->RendererUserData = nullptr;
			delete window;
		}
	}

	void WindowResize(ImGuiViewport *viewport, const ImVec2 size)
	{
		static_cast<WindowData*>(viewport->RendererUserData)->swapchain->Resize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
		SEDX_CORE_TRACE_TAG("WindowResize", "Resized window called for {}", viewport->ID);
	}

	void WindowRender(ImGuiViewport *viewport, void*)
	{
		const bool clear = !(viewport->Flags & ImGuiViewportFlags_NoRendererClear);
		Render(viewport->DrawData, static_cast<WindowData *>(viewport->RendererUserData), clear);
		SEDX_CORE_TRACE_TAG("WindowRender", "Rendered window called for {}", viewport->ID);
	}

	void WindowPresent(ImGuiViewport *viewport, void*)
	{
		WindowData *window = static_cast<WindowData*>(viewport->RendererUserData);
		window->swapchain->Present(window->cmdList);
		SEDX_CORE_TRACE_TAG("WindowPresent", "Presented window called for {}", viewport->ID);
	}

}

// -------------------------------------------------------
