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
 * texture_viewer.cpp
 * -------------------------------------------------------
 * Created: 17/03/2026
 * -------------------------------------------------------
 */
#include "texure_viewer.h"
#include "Editor/core/editor_layer.h"
#include <algorithm>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/ui/ui.h>
#include <SceneryEditorX/renderer/ui/ui_widget.h>
#include <SceneryEditorX/renderer/vulkan/image_resource.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	ImageResource *s_TextureCurrent = nullptr;
	uint32_t s_TextureIndex = 0;
	int s_MipLevel = 0;
	int s_ArrayLevel = 0;
	bool s_MagnifyingGlass = false;
	bool s_Channel_R = true;
	bool s_Channel_G = true;
	bool s_Channel_B = true;
	bool s_Channel_A = true;
	bool s_GammaCorrect = true;
	bool s_Pack = false;
	bool s_Boost = false;
	bool s_Abs = false;
	bool s_PointSampling = false;
	float s_ZoomLevel = 1.0f;
	ImVec2 s_PanOffset = ImVec2(0.0f, 0.0f);
	uint32_t s_VisualisationFlags = 0;
	std::vector<std::string> s_RenderTarget_Names;
	std::vector<ImageResource *> render_targets;

	// -------------------------------------------------------

	TextureViewer::TextureViewer(EditorLayer *editor) : Widget(editor)
	{
		m_Title = "Texture Viewer";
		m_Visible = false;
	}

	void TextureViewer::OnTick()
	{
		s_VisualisationFlags = 0;
		s_TextureCurrent = nullptr;
	}
	
	void TextureViewer::OnVisible()
	{
		s_RenderTarget_Names.clear();
		render_targets.clear();
	
		// get render targets
		std::vector<std::pair<std::string, ImageResource *>> sortedTargets;
		for (uint32_t i = 0; i < static_cast<uint32_t>(Renderer_RenderTarget::MaxEnum); ++i)
		{
			if (ImageResource *renderTarget = Renderer::GetRenderTarget(static_cast<Renderer_RenderTarget>(i)))
			{
				sortedTargets.emplace_back(renderTarget->GetObjectName(), renderTarget);
			}
		}
	
		// sort alphabetically by name
		std::ranges::sort(sortedTargets, [](const auto &a, const auto &b) { return a.first < b.first; });
	
		// populate the lists
		for (const auto &target : sortedTargets)
		{
			s_RenderTarget_Names.emplace_back(target.first);
			render_targets.emplace_back(target.second);
		}
	}
	
	void TextureViewer::OnTickVisible()
	{
		if (render_targets.empty())
			return;
	
		// clamp texture index to valid range (textures may have been deallocated)
		if (s_TextureIndex >= render_targets.size())
		{
			s_TextureIndex = 0;
		}
	
		// two columns: left for preview, right for properties
		ImGui::Columns(2, "texture_viewer_columns", false);
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.65f);
	
		//=====================================
		// preview (left)
		//=====================================
		{
			ImGui::BeginChild("texture_preview", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	
			ImVec2 childPos = ImGui::GetCursorScreenPos();
			ImVec2 childSize = ImGui::GetContentRegionAvail();
	
			// draw black border around the preview
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			drawList->AddRect(childPos, ImVec2(childPos.x + childSize.x, childPos.y + childSize.y),
				IM_COL32(0, 0, 0, 255), 0.0f, 0, 2.0f);
	
			if (ImageResource *texture = render_targets[s_TextureIndex])
			{
				s_TextureCurrent = texture;
	
				float texW = static_cast<float>(texture->GetWidth());
				float texH = static_cast<float>(texture->GetHeight());
				float aspect = texW / texH;
	
				float availW = childSize.x;
				float availH = childSize.y;
	
				float fitW = availW;
				float fitH = availW / aspect;
				if (fitH > availH)
				{
					fitH = availH;
					fitW = availH * aspect;
				}
	
				float drawW = fitW * s_ZoomLevel;
				float drawH = fitH * s_ZoomLevel;
	
				ImVec2 cursorPos = ImGui::GetCursorScreenPos();
				ImVec2 imagePos = ImVec2(cursorPos.x + s_PanOffset.x, cursorPos.y + s_PanOffset.y);
	
				ImGui::SetCursorScreenPos(imagePos);
				UI::Image(texture, {drawW, drawH}, ImColor(255, 255, 255, 255), ImColor(40, 40, 40, 255));
				ImGui::SetCursorScreenPos(cursorPos);
	
				ImGuiIO &io = ImGui::GetIO();
				ImVec2 mouseDelta = io.MouseDelta;
	
				if (ImGui::IsWindowHovered())
				{
					if (io.MouseWheel != 0.0f)
					{
						float prevZoom = s_ZoomLevel;
						s_ZoomLevel *= (io.MouseWheel > 0.0f) ? 1.1f : 0.9f;
						s_ZoomLevel = Clamp(s_ZoomLevel, 0.05f, 8.0f);
	
						ImVec2 mousePos = io.MousePos;
						ImVec2 rel = ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);
						s_PanOffset.x -= rel.x * (s_ZoomLevel / prevZoom - 1.0f);
						s_PanOffset.y -= rel.y * (s_ZoomLevel / prevZoom - 1.0f);
					}
	
					if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
					{
						s_PanOffset.x += mouseDelta.x;
						s_PanOffset.y += mouseDelta.y;
					}
	
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Middle))
					{
						s_ZoomLevel = 1.0f;
						s_PanOffset = ImVec2(0, 0);
					}
				}
	
				ImGui::Text("Zoom (Wheel): %.0f%%", s_ZoomLevel * 100.0f);
				ImGui::Text("Pan (Middle Click + Drag): %.0f, %.0f", s_PanOffset.x, s_PanOffset.y);
			}
	
			ImGui::EndChild();
		}
	
		ImGui::NextColumn();
	
		//=====================================
		// properties (right)
		//=====================================
		{
			ImGui::BeginChild("texture_properties", ImVec2(0, 0), true);
	
			// target selector
			ImGui::Text("Texture");
			ImGui::SameLine();
			UI::ComboBox("##texture", s_RenderTarget_Names, &s_TextureIndex);
	
			if (s_TextureCurrent)
			{
				// info
				if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Text("Name: %s", s_TextureCurrent->GetObjectName().c_str());
					ImGui::Text("Size: %dx%d", s_TextureCurrent->GetWidth(), s_TextureCurrent->GetHeight());
					ImGui::Text("Channels: %d", s_TextureCurrent->GetChannelCount());
					ImGui::Text("Format: %d", static_cast<int>(s_TextureCurrent->GetImageSpec().format));
					ImGui::Text("Mips: %d", s_TextureCurrent->GetMipCount());
					ImGui::Text("Array: %d", s_TextureCurrent->GetArrayLength());
				}
	
				// mip and array sliders
				if (s_TextureCurrent->GetMipCount() > 1)
				{
					ImGui::SliderInt("Mip Level",
									 &s_MipLevel,
									 0,
									 static_cast<int>(s_TextureCurrent->GetMipCount()) - 1,
									 "%d");
				}
				if (s_TextureCurrent->GetArrayLength() > 1)
				{
					ImGui::SliderInt("Array Level",
									 &s_ArrayLevel,
									 0,
									 static_cast<int>(s_TextureCurrent->GetArrayLength()) - 1,
									 "%d");
				}
	
				// channels
				if (ImGui::CollapsingHeader("Channels", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Checkbox("Red", &s_Channel_R);
					ImGui::Checkbox("Green", &s_Channel_G);
					ImGui::Checkbox("Blue", &s_Channel_B);
					ImGui::Checkbox("Alpha", &s_Channel_A);
				}
	
				// visualisation
				if (ImGui::CollapsingHeader("Visualization", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Checkbox("Gamma Correct", &s_GammaCorrect);
					ImGui::Checkbox("Pack from [-1, 1] to [0, 1]", &s_Pack);
					ImGui::Checkbox("Boost", &s_Boost);
					ImGui::Checkbox("Abs", &s_Abs);
					ImGui::Checkbox("Point Sampling", &s_PointSampling);
				}
			}
	
			ImGui::EndChild();
		}
	
		ImGui::Columns(1);
	
		// update flags
		s_VisualisationFlags = 0;
		s_VisualisationFlags |= s_Channel_R ? Visualise_Channel_R : 0;
		s_VisualisationFlags |= s_Channel_G ? Visualise_Channel_G : 0;
		s_VisualisationFlags |= s_Channel_B ? Visualise_Channel_B : 0;
		s_VisualisationFlags |= s_Channel_A ? Visualise_Channel_A : 0;
		s_VisualisationFlags |= s_GammaCorrect ? Visualise_GammaCorrect : 0;
		s_VisualisationFlags |= s_Pack ? Visualise_Pack : 0;
		s_VisualisationFlags |= s_Boost ? Visualise_Boost : 0;
		s_VisualisationFlags |= s_Abs ? Visualise_Abs : 0;
		s_VisualisationFlags |= s_PointSampling ? Visualise_Sample_Point : 0;
	}
	
	uint32_t TextureViewer::GetVisualisationFlags()
	{
		return s_VisualisationFlags;
	}
	
	int TextureViewer::GetMipLevel()
	{
		return s_MipLevel;
	}
	
	int TextureViewer::GetArrayLevel()
	{
		return s_ArrayLevel;
	}
	
	uint64_t TextureViewer::GetVisualisedTextureId()
	{
		return s_TextureCurrent ? s_TextureCurrent->GetObjectId() : 0;
	}
	
}

// -------------------------------------------------------
