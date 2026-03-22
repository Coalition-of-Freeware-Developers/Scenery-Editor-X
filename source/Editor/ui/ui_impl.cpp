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
#include "panels/texure_viewer.h"
#include <cstring>
#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/core/resource/asset_resource.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/window/monitor_data.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <SceneryEditorX/renderer/vulkan/buffer.h>
#include <SceneryEditorX/renderer/vulkan/queue_manager.h>
#include <SceneryEditorX/renderer/vulkan/swapchain.h>
#include <SceneryEditorX/renderer/vulkan/debug/graphics_debug.h>
#include <SceneryEditorX/renderer/vulkan/pipeline/pipeline_state.h>

// -------------------------------------------------------

using namespace SceneryEditorX;

namespace UI
{
	ViewportResources g_ViewportData;

	Ref<ImageResource> g_FontAtlas;
	Ref<DepthStencilState> g_DepthStencil_State;
	Ref<RasterizerState> g_Rasterizer_State;
	Ref<BlendState> g_BlendState;
	Ref<Shader> g_VertexShader;
	Ref<Shader> g_FragmentShader;

	void InitializePlatformInterface()
	{
		ImGuiPlatformIO &platformIo = ImGui::GetPlatformIO();
		platformIo.Renderer_CreateWindow = WindowCreate;
		platformIo.Renderer_DestroyWindow = WindowDestroy;
		platformIo.Renderer_SetWindowSize = WindowResize;
		platformIo.Renderer_RenderWindow = WindowRender;
		platformIo.Renderer_SwapBuffers = WindowPresent;
	}

	void DestroyResources()
	{
		g_FontAtlas = nullptr;
		g_DepthStencil_State = nullptr;
		g_Rasterizer_State = nullptr;
		g_BlendState = nullptr;
		g_VertexShader = nullptr;
		g_FragmentShader = nullptr;
	
		for (auto &ptr : g_ViewportData.index_buffers)
		{
			ptr = nullptr;
		}
	
		for (auto &ptr : g_ViewportData.vertex_buffers)
		{
			ptr = nullptr;
		}
	}

	void Initialize()
	{
		// create required objects
		{
			g_ViewportData = ViewportResources("imgui");
			g_DepthStencil_State = SceneryEditorX::CreateRef<DepthStencilState>(false, false, VK_COMPARE_OP_ALWAYS);
			g_Rasterizer_State = SceneryEditorX::CreateRef<RasterizerState>(PolygonMode::Solid, true);
	
			g_BlendState = SceneryEditorX::CreateRef<BlendState>(true,
																 VK_BLEND_FACTOR_SRC_ALPHA,           // source blend
																 VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // destination blend
																 VK_BLEND_OP_ADD,                     // blend op
																 VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // source blend alpha
																 VK_BLEND_FACTOR_ZERO, // destination blend alpha
																 VK_BLEND_OP_ADD       // destination op alpha
			);
	
			// compile shaders
			{
				const std::string shaderPath =
					ResourceCache::GetResourceDirectory(ResourceDirectory::Shaders) + "/ui.slang";
	
				bool async = false;
				(void)async;
	
				g_VertexShader = CreateRef<Shader>();
				g_VertexShader->AddShaderStage(Stage::Vertex, shaderPath);
	
				g_FragmentShader = CreateRef<Shader>();
				g_FragmentShader->AddShaderStage(Stage::Fragment, shaderPath);
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
			memcpy(mip.data(), reinterpret_cast<std::byte *>(pixels), size);
	
			ImgResourceSpec spec{};
			spec.type = ImageType::Type2D;
			spec.width = atlasWidth;
			spec.height = atlasHeight;
			spec.depth = 1;
			spec.mipCount = 1;
			spec.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
			spec.flags = ShaderViews;
			spec.name = "imgui_font_atlas";
	
			// upload texture to graphics system
			g_FontAtlas = CreateRef<ImageResource>(spec, std::move(texture_data));
			io.Fonts->TexID = reinterpret_cast<ImTextureID>(g_FontAtlas.Get());
		}
	
		// setup back-end capabilities flags
		ImGuiIO &io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
		io.BackendRendererName = "UI Renderer";
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			InitializePlatformInterface();
		}
	}

	void Shutdown()
	{
		ImGui::DestroyPlatformWindows();
	}

	void Render(ImDrawData *drawData, WindowData *windowData, const bool clear)
	{
		if (!drawData || drawData->TotalVtxCount <= 0 || drawData->TotalIdxCount <= 0)
			return;
	
		// skip the first two frames to let the renderer fully initialize.
		// frame 0: pipeline layouts and descriptor sets are still being created.
		// frame 1: bindless draw_data buffer descriptor may not have been written yet.
		if (uint64_t frame = Renderer::GetFrameNumber(); frame < 2)
			return;
	
		// get resources
		bool isMainWindow = windowData == nullptr;
		ViewportResources *resources = isMainWindow ? &g_ViewportData : windowData->viewportResources.get();
		Swapchain *swapchain = isMainWindow ? Renderer::GetSwapChain() : windowData->swapchain.Get();
		uint32_t bufferIndex = resources->bufferIndex;
		resources->bufferIndex = (resources->bufferIndex + 1) % BUFFER_COUNT;
		Buffer *vertexBuffer = resources->vertex_buffers[bufferIndex].get();
		Buffer *indexBuffer = resources->index_buffers[bufferIndex].get();
		CommandList *cmdList = Renderer::GetCommandListPresent();
	
		// if that's a child window, update it's swapchain and give it a command list
		if (!isMainWindow)
		{
			swapchain->AcquireNextImage();
	
			Ref<Device> device = SceneryEditorX::RenderContext::Get()->GetDevice();
			auto queueManager = device ? device->GetQueueManager() : nullptr;
			windowData->cmdList = queueManager ? queueManager->NextCommandList() : nullptr;
			cmdList = windowData->cmdList;
	
			if (windowData->cmdList)
			{
				windowData->cmdList->Begin();
			}
		}
	
		// when the engine splash screen is shown, the command list is not valid as the renderer is initializing
		if (!cmdList || cmdList->GetState() != CommandState::Recording)
			return;
	
		// update vertex and index buffers
		{
			// grow vertex buffer as needed
			if (!vertexBuffer || resources->vertex_counts[bufferIndex] < static_cast<uint32_t>(drawData->TotalVtxCount))
			{
				const uint32_t count = resources->vertex_counts[bufferIndex];
				const uint32_t count_new = drawData->TotalVtxCount + 15000;
				resources->vertex_counts[bufferIndex] = count_new;
				resources->vertex_buffers[bufferIndex] =
					CreateScope<Buffer>(sizeof(ImDrawVert), count_new, nullptr, true, "imgui_vertex_buffer");
				vertexBuffer = resources->vertex_buffers[bufferIndex].get();
	
				if (count != 0)
				{
					SEDX_CORE_INFO_TAG("UI Implementation",
									   "Vertex buffer has been re-allocated to fit {} vertices",
									   count_new);
				}
			}
	
			// grow index buffer as needed
			if (!indexBuffer || resources->index_counts[bufferIndex] < static_cast<uint32_t>(drawData->TotalIdxCount))
			{
				const uint32_t count = resources->index_counts[bufferIndex];
				const uint32_t count_new = drawData->TotalIdxCount + 30000;
				resources->index_counts[bufferIndex] = count_new;
				resources->index_buffers[bufferIndex] =
					CreateScope<Buffer>(sizeof(ImDrawIdx), count_new, nullptr, true, "imgui_index_buffer");
				indexBuffer = resources->index_buffers[bufferIndex].get();
	
				if (count != 0)
				{
					SEDX_CORE_INFO_TAG("UI Implementation",
									   "Index buffer has been re-allocated to fit {} indices",
									   count_new);
				}
			}
	
			if (!vertexBuffer || !indexBuffer)
				return;
	
			if (!vertexBuffer->GetMappedData())
			{
				vertexBuffer->Map();
			}
	
			if (!indexBuffer->GetMappedData())
			{
				indexBuffer->Map();
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
		static PipelineState pso = {};
		pso.name = "imgui";
		pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = g_VertexShader.Get();
		pso.shaders[static_cast<uint32_t>(Stage::Fragment)] = g_FragmentShader.Get();
		pso.rasterizerState = g_Rasterizer_State.Get();
		pso.blendState = g_BlendState.Get();
		pso.depthStencil_State = g_DepthStencil_State.Get();
		//pso.renderTarget_swapchain     = swapchain;
		pso.clearColor[0] = clear ? PipelineStateColor{0.0f, 0.0f, 0.0f, 1.0f} : RHI_COLOR_LOAD;
	
		// start the pass
		const char *name = isMainWindow ? "imgui_window_main" : "imgui_window_child";
		bool gpu_timing = isMainWindow;
		//cmdList->BeginTimeblock(name, true, Debugging::IsGpuTimingEnabled() && gpu_timing);
		cmdList->SetPipelineState(pso);
		cmdList->SetBufferVertex(vertexBuffer);
		cmdList->SetBufferIndex(indexBuffer);
		cmdList->SetCullMode(CullMode::None);
	
		// render
		{
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
						// set scissor rectangle
						{
							xMath::Rectangle rectangle;
							rectangle.x = pcmd->ClipRect.x - drawData->DisplayPos.x;
							rectangle.y = pcmd->ClipRect.y - drawData->DisplayPos.y;
							rectangle.width = (pcmd->ClipRect.z - drawData->DisplayPos.x) - rectangle.x;
							rectangle.height = (pcmd->ClipRect.w - drawData->DisplayPos.y) - rectangle.y;
	
							cmdList->SetScissor(rectangle);
						}
	
						// push pass/draw call constants
						{
							// set texture and update texture viewer parameters
							{
								float mipLevel = 0.0f;
								float arrayLevel = 0.0f;
								bool isTextureVisualised = false;
								bool isFrameTexture = false;
								bool textureBound = false;
	
								if (ImageResource *texture = reinterpret_cast<ImageResource *>(pcmd->GetTexID()))
								{
									// during engine startup, some textures might be loading in different threads
									if (texture->GetResourceState() == ResourceState::PreparedForGpu)
									{
										cmdList->SetTexture(Renderer_BindingsSrv::tex, texture);
										textureBound = true;
									}
								}
	
								// always bind a texture to avoid uninitialized descriptor errors
								if (!textureBound)
								{
									cmdList->SetTexture(Renderer_BindingsSrv::tex, g_FontAtlas.Get());
								}
	
								// pack booleans into uint bitfield
								uint32_t flags = 0;
								if (isTextureVisualised)
								{
									flags |= (TextureViewer::GetVisualisationFlags() & Visualise_Channel_R) ? BIT(0) : 0;
									flags |= (TextureViewer::GetVisualisationFlags() & Visualise_Channel_G) ? BIT(1) : 0;
									flags |= (TextureViewer::GetVisualisationFlags() & Visualise_Channel_B) ? BIT(2) : 0;
									flags |= (TextureViewer::GetVisualisationFlags() & Visualise_Channel_A) ? BIT(3) : 0;
									flags |= (TextureViewer::GetVisualisationFlags() & Visualise_GammaCorrect) ? BIT(4) : 0;
									flags |= (TextureViewer::GetVisualisationFlags() & Visualise_Pack) ? BIT(5) : 0;
									flags |= (TextureViewer::GetVisualisationFlags() & Visualise_Boost) ? BIT(6) : 0;
									flags |= (TextureViewer::GetVisualisationFlags() & Visualise_Abs) ? BIT(7) : 0;
									flags |= (TextureViewer::GetVisualisationFlags() & Visualise_Sample_Point) ? BIT(8) : 0;
								}
								flags |= isTextureVisualised ? BIT(9) : 0;
								flags |= isFrameTexture ? BIT(10) : 0;
	
								// store bitfield in m00 and mip/array levels in m23, m30
								float packedFlags = 0.0f;
								std::memcpy(&packedFlags, &flags, sizeof(uint32_t));
								resources->pushConstantBuffer_Pass.SetF3Value(packedFlags, 0.0f, 0.0f);
								resources->pushConstantBuffer_Pass.SetF2Value(mipLevel, arrayLevel);
							}
	
							// compute transform matrix and write to the bindless draw data buffer
							{
								const float L = drawData->DisplayPos.x;
								const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
								const float T = drawData->DisplayPos.y;
								const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
	
								Matrix projection(2.0f / (R - L),
												  0.0f,
												  0.0f,
												  (R + L) / (L - R),
												  0.0f,
												  2.0f / (T - B),
												  0.0f,
												  (T + B) / (B - T),
												  0.0f,
												  0.0f,
												  0.5f,
												  0.5f,
												  0.0f,
												  0.0f,
												  0.0f,
												  1.0f);
	
								(void)projection;
								resources->pushConstantBuffer_Pass.drawIndex = 0;
							}
	
							cmdList->PushConstants(resources->pushConstantBuffer_Pass);
						}
	
						cmdList->DrawIndexed(pcmd->ElemCount,
											 1,
											 pcmd->IdxOffset + global_idx_offset,
											 pcmd->VtxOffset + global_vtx_offset);
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
		SEDX_CORE_ASSERT(viewport->PlatformHandle);
	
		// note: platformHandle is SDL_Window, PlatformHandleRaw is HWND
		SDL_Window *sdl_window =
			SDL_GetWindowFromID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(viewport->PlatformHandle)));
	
		WindowData *window = new WindowData();
		window->swapchain = CreateRef<Swapchain>();
	
		window->viewportResources = CreateScope<ViewportResources>("imgui_child_window", window->swapchain.Get());
		viewport->RendererUserData = window;
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
		static_cast<WindowData *>(viewport->RendererUserData)
			->swapchain->Resize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
	}

	void WindowRender(ImGuiViewport *viewport, void *)
	{
		const bool clear = !(viewport->Flags & ImGuiViewportFlags_NoRendererClear);
		Render(viewport->DrawData, static_cast<WindowData *>(viewport->RendererUserData), clear);
	}

	void WindowPresent(ImGuiViewport *viewport, void *)
	{
		WindowData *window = static_cast<WindowData *>(viewport->RendererUserData);
		if (!window || !window->swapchain)
			return;
	
		Ref<Device> device = SceneryEditorX::RenderContext::Get()->GetDevice();
		auto queueManager = device ? device->GetQueueManager() : nullptr;
		if (!queueManager)
			return;
	
		Ref<Queue> *graphicsQueue = queueManager->GetQueue(QueueType::Graphics);
		if (!graphicsQueue || !(*graphicsQueue))
			return;
	
		window->swapchain->Present((*graphicsQueue)->GetQueue(), window->swapchain->GetImageIndex(), VK_NULL_HANDLE);
	}

}

// -------------------------------------------------------
