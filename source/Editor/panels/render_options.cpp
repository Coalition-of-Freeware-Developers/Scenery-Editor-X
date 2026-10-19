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
 * render_options.cpp
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#include "render_options.h"
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <Editor/core/editor_layer.h>
#include <SceneryEditorX/core/time/fps_timer.h>
#include <SceneryEditorX/core/window/monitor_data.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/ui/ui.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	static int s_ColumnCount = 2;
	static ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable;

	#define WIDTH_INPUT_NUMERIC (120.0f * Window::GetDpiScale())
	#define WIDTH_COMBO_BOX (120.0f * Window::GetDpiScale())

	static std::vector<SDL_DisplayMode> s_DisplayModes;
	static std::vector<std::string> s_DisplayModes_String;

	static std::unordered_map<std::string, float> s_RenderOptions = {{"r.variable_rate_shading", 0.0f},
																	 {"r.dynamic_resolution", 0.0f},
																	 {"r.resolution_scale", 1.0f},
																	 {"r.sharpness", 0.5f},
																	 {"r.ssao", 1.0f},
																	 {"r.hdr", 0.0f},
																	 {"r.gamma", 2.2f},
																	 {"r.auto_exposure_adaptation_speed", 1.0f},
																	 {"r.tonemapping", 0.0f},
																	 {"r.bloom", 0.5f},
																	 {"r.motion_blur", 0.0f},
																	 {"r.depth_of_field", 0.0f},
																	 {"r.film_grain", 0.0f},
																	 {"r.chromatic_aberration", 0.0f},
																	 {"r.vhs", 0.0f},
																	 {"r.dithering", 0.0f},
																	 {"r.fog", 0.0f},
																	 {"r.cloud_coverage", 0.45f},
																	 {"r.cloud_shadows", 1.0f},
																	 {"r.vsync", 0.0f},
																	 {"r.performance_metrics", 0.0f},
																	 {"r.transform_handle", 1.0f},
																	 {"r.selection_outline", 1.0f},
																	 {"r.lights", 1.0f},
																	 {"r.audio_sources", 0.0f},
																	 {"r.grid", 1.0f},
																	 {"r.picking_ray", 0.0f},
																	 {"r.physics", 0.0f},
																	 {"r.aabb", 0.0f},
																	 {"r.wireframe", 0.0f},
																	 {"r.hiz_occlusion", 1.0f},
																	 {"r.antialiasing_upsampling", 0.0f}};

	static float GetRenderOption(const char *key)
	{
		auto it = s_RenderOptions.find(key);
		return it != s_RenderOptions.end() ? it->second : 0.0f;
	}

	static void SetRenderOption(const char *key, float value)
	{
		s_RenderOptions[key] = value;
	}

	static bool OptionHeader(const char *title, bool default_open = true)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		return UI::CollapsingHeader(title, default_open ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
	}

	static void OptionFirstColumn()
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
	}

	static void OptionSecondColumn()
	{
		ImGui::TableSetColumnIndex(1);
	}

	static void OptionCheckBox(const char *label, const char *renderOption, const char *tooltip = nullptr)
	{
		OptionFirstColumn();
		ImGui::Text(label);
		if (tooltip)
		{
			UI::Tooltip(tooltip);
		}

		OptionSecondColumn();
		ImGui::PushID(static_cast<int>(ImGui::GetCursorPosY()));
		bool value = GetRenderOption(renderOption) != 0.0f;
		UI::CheckBox("", &value);
		SetRenderOption(renderOption, value ? 1.0f : 0.0f);
		ImGui::PopID();
	}

	static bool OptionComboBox(const char *label, const std::vector<std::string> &options, uint32_t &selection_index, const char *tooltip = nullptr)
	{
		OptionFirstColumn();
		ImGui::Text(label);
		if (tooltip)
		{
			UI::Tooltip(tooltip);
		}

		OptionSecondColumn();
		ImGui::PushID(static_cast<int>(ImGui::GetCursorPosY()));
		ImGui::PushItemWidth(WIDTH_COMBO_BOX);
		bool result = UI::ComboBox("", options, &selection_index);
		ImGui::PopItemWidth();
		ImGui::PopID();
		return result;
	}

	static bool OptionValue(const char *label, const char *render_option, const char *tooltip = nullptr, float step = 0.1f,
							float min = 0.0f,
							float max = std::numeric_limits<float>::max(), const char *format = "%.3f")
	{
		OptionFirstColumn();
		ImGui::Text(label);
		if (tooltip)
		{
			UI::Tooltip(tooltip);
		}

		bool changed = false;
		OptionSecondColumn();
		{
			float value = GetRenderOption(render_option);

			ImGui::PushID(static_cast<int>(ImGui::GetCursorPosY()));
			ImGui::PushItemWidth(WIDTH_INPUT_NUMERIC);
			changed = ImGui::InputFloat("", &value, step, 0.0f, format);
			ImGui::PopItemWidth();
			ImGui::PopID();
			value = xMath::Clamp(value, min, max);

			if (changed)
			{
				SetRenderOption(render_option, value);
			}
		}

		return changed;
	}

	static uint32_t GetDisplayModeIndex(const xMath::Vec2 &resolution)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(s_DisplayModes.size()); i++)
		{
			const SDL_DisplayMode &display_mode = s_DisplayModes[i];
			if (static_cast<uint32_t>(display_mode.w) == static_cast<uint32_t>(resolution.x) &&
				static_cast<uint32_t>(display_mode.h) == static_cast<uint32_t>(resolution.y))
			{
				return i;
			}
		}

		return 0;
	}

	RenderOptions::RenderOptions(EditorLayer *editor) : UI::EditorPanel()
	{
		m_Title = "Renderer Options";
		m_Visible = false;
		m_Alpha = 1.0f;
		m_InitialSize = Vec2(MonitorData::GetWidth() * 0.25f, MonitorData::GetHeight() * 0.5f);
		m_Editor = editor;
	}

	void RenderOptions::OnVisible()
	{
		s_DisplayModes.clear();
		s_DisplayModes_String.clear();

		MonitorData monitor_data;
		int mode_count = 0;
		const SDL_DisplayMode **modes = monitor_data.GetVideoModes(monitor_data.GetCurrentMonitorIndex(), &mode_count);
		if (modes && mode_count > 0)
		{
			for (int i = 0; i < mode_count; ++i)
			{
				if (!modes[i])
				{
					continue;
				}

				s_DisplayModes.emplace_back(*modes[i]);
				s_DisplayModes_String.emplace_back(std::to_string(modes[i]->w) + "x" + std::to_string(modes[i]->h));
			}

			SDL_free(const_cast<SDL_DisplayMode **>(modes));
		}

		if (s_DisplayModes.empty())
		{
			SDL_DisplayMode fallback_mode{};
			fallback_mode.w = static_cast<int>(MonitorData::GetWidth());
			fallback_mode.h = static_cast<int>(MonitorData::GetHeight());
			s_DisplayModes.emplace_back(fallback_mode);
			s_DisplayModes_String.emplace_back(std::to_string(fallback_mode.w) + "x" + std::to_string(fallback_mode.h));
		}
	}

	void RenderOptions::OnTickVisible()
	{
		if (ImGui::BeginTabBar("##renderer_options_tabs"))
		{
			if (ImGui::BeginTabItem("Rendering"))
			{
				if (ImGui::BeginTable("##rendering", s_ColumnCount, flags))
				{
					ImGui::TableSetupColumn("Option");
					ImGui::TableSetupColumn("Value");
					ImGui::TableHeadersRow();

					if (OptionHeader("Resolution"))
					{
						xMath::Vec2 res_render = Renderer::GetRendererResolution();
						uint32_t res_render_index = GetDisplayModeIndex(res_render);
						if (OptionComboBox("Render resolution", s_DisplayModes_String, res_render_index))
						{
							Renderer::SetRendererResolution(static_cast<uint32_t>(s_DisplayModes[res_render_index].w),
															static_cast<uint32_t>(s_DisplayModes[res_render_index].h));
						}

						xMath::Vec2 res_output = Renderer::GetOutputResolution();
						uint32_t res_output_index = GetDisplayModeIndex(res_output);
						if (OptionComboBox("Output resolution", s_DisplayModes_String, res_output_index))
						{
							Renderer::SetOutputResolution(static_cast<uint32_t>(s_DisplayModes[res_output_index].w),
														  static_cast<uint32_t>(s_DisplayModes[res_output_index].h));
						}

						OptionCheckBox("Variable rate shading",
									   "r.variable_rate_shading",
									   "Improves performance by varying shading detail per pixel");
						OptionCheckBox("Dynamic resolution",
									   "r.dynamic_resolution",
									   "Scales render resolution automatically based on GPU load");

						ImGui::BeginDisabled(GetRenderOption("r.dynamic_resolution") != 0.0f);
						OptionValue("Resolution scale",
									"r.resolution_scale",
									"Adjusts the percentage of the render resolution",
									0.01f);
						ImGui::EndDisabled();
					}

					if (OptionHeader("Anti-Aliasing & Upscaling"))
					{
						static std::vector<std::string> upsamplers = {"Off", "FXAA", "FSR 3", "XeSS 3"};

						uint32_t mode = static_cast<uint32_t>(GetRenderOption("r.antialiasing_upsampling"));
						if (OptionComboBox("Upsampling method", upsamplers, mode))
						{
							SetRenderOption("r.antialiasing_upsampling", static_cast<float>(mode));
						}

						bool use_rcas = mode == 2;
						std::string label = use_rcas ? "Sharpness (RCAS)" : "Sharpness (CAS)";
						std::string tooltip = use_rcas ? "AMD FidelityFX Robust Contrast Adaptive Sharpening"
													   : "AMD FidelityFX Contrast Adaptive Sharpening";
						OptionValue(label.c_str(), "r.sharpness", tooltip.c_str(), 0.1f, 0.0f, 1.0f);
					}

					if (OptionHeader("Screen-space Effects"))
					{
						OptionCheckBox("Ambient Occlusion (SSAO)", "r.ssao");
					}

					ImGui::EndTable();
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Output"))
			{
				if (ImGui::BeginTable("##output", s_ColumnCount, flags))
				{
					ImGui::TableSetupColumn("Option");
					ImGui::TableSetupColumn("Value");
					ImGui::TableHeadersRow();

					if (OptionHeader("Display"))
					{
						OptionCheckBox("HDR", "r.hdr", "Enable high dynamic range output");
						ImGui::BeginDisabled(GetRenderOption("r.hdr") != 0.0f);
						OptionValue("Gamma", "r.gamma");
						ImGui::EndDisabled();
						OptionValue("Exposure adaptation speed",
									"r.auto_exposure_adaptation_speed",
									"Negative value disables adaptation",
									0.1f,
									-1.0f);
					}

					if (OptionHeader("Tone Mapping"))
					{
						static std::vector<std::string> tonemapping =
							{"ACES", "AgX", "Reinhard", "ACES Nautilus", "Gran Turismo 7", "Off"};
						uint32_t index = static_cast<uint32_t>(GetRenderOption("r.tonemapping"));
						if (OptionComboBox("Algorithm", tonemapping, index))
						{
							SetRenderOption("r.tonemapping", static_cast<float>(index));
						}
					}

					ImGui::EndTable();
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Camera"))
			{
				if (ImGui::BeginTable("##camera", s_ColumnCount, flags))
				{
					ImGui::TableSetupColumn("Effect");
					ImGui::TableSetupColumn("Value");
					ImGui::TableHeadersRow();

					OptionValue("Bloom intensity", "r.bloom", "Blend factor, set to 0 to disable", 0.01f);
					OptionCheckBox("Motion blur", "r.motion_blur", "Controlled by camera shutter speed");
					OptionCheckBox("Depth of field", "r.depth_of_field", "Controlled by camera aperture");
					OptionCheckBox("Film grain", "r.film_grain", "Simulates old film camera noise");
					OptionCheckBox("Chromatic aberration", "r.chromatic_aberration", "Lens color fringing effect");
					OptionCheckBox("VHS effect", "r.vhs", "Retro VHS look");
					OptionCheckBox("Dithering", "r.dithering", "Reduces color banding in gradients");

					ImGui::EndTable();
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("World"))
			{
				if (ImGui::BeginTable("##world", s_ColumnCount, flags))
				{
					ImGui::TableSetupColumn("Option");
					ImGui::TableSetupColumn("Value");
					ImGui::TableHeadersRow();

					OptionValue("Fog density", "r.fog", "Controls atmospheric fog strength", 0.1f);

					if (OptionHeader("Volumetric Clouds"))
					{
						OptionValue("Coverage",
									"r.cloud_coverage",
									"Sky coverage (0=clear, 1=overcast)",
									0.05f,
									0.0f,
									1.0f,
									"%.2f");
						OptionValue("Shadow Intensity",
									"r.cloud_shadows",
									"Cloud shadow intensity on ground",
									0.1f,
									0.0f,
									2.0f,
									"%.2f");
					}

					ImGui::EndTable();
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Debug"))
			{
				if (ImGui::BeginTable("##debug", s_ColumnCount, flags))
				{
					ImGui::TableSetupColumn("Option");
					ImGui::TableSetupColumn("Value");
					ImGui::TableHeadersRow();

					if (OptionHeader("Performance"))
					{
						OptionCheckBox("VSync", "r.vsync", "Synchronize frame updates with monitor refresh");
						OptionFirstColumn();
						std::string fps_label =
							"FPS Limit (" +
							std::string(FPSTimer::GetFpsLimitType() == FrameLimits::FixedToMonitor ? "Fixed to monitor"
										: FPSTimer::GetFpsLimitType() == FrameLimits::Unlocked     ? "Unlocked"
																								   : "Fixed") +
							")";
						ImGui::Text(fps_label.c_str());
						OptionSecondColumn();
						{
							float fps_target = FPSTimer::GetFpsLimit();
							ImGui::PushItemWidth(WIDTH_INPUT_NUMERIC);
							ImGui::InputFloat("##fps_limit", &fps_target, 0.0f, 0.0f, "%.1f");
							ImGui::PopItemWidth();
							FPSTimer::SetFpsLimit(fps_target);
						}
						OptionCheckBox("Show performance metrics", "r.performance_metrics");
					}

					if (OptionHeader("Shader Modules"))
					{
						OptionFirstColumn();
						ImGui::Text("common.slang");
						OptionSecondColumn();
						ImGui::TextUnformatted(ShaderManager::IsSlangModuleAvailable("common") ? "Available" : "Missing");

						OptionFirstColumn();
						ImGui::Text("resources.slang");
						OptionSecondColumn();
						ImGui::TextUnformatted(ShaderManager::IsSlangModuleAvailable("resources") ? "Available" : "Missing");

						OptionFirstColumn();
						ImGui::Text("constants.slang");
						OptionSecondColumn();
						ImGui::TextUnformatted(ShaderManager::IsSlangModuleAvailable("constants") ? "Available" : "Missing");
					}

					if (OptionHeader("Debug Visuals"))
					{
						OptionCheckBox("Transform handles", "r.transform_handle");
						OptionCheckBox("Selection outline", "r.selection_outline");
						OptionCheckBox("Lights", "r.lights");
						OptionCheckBox("Audio sources", "r.audio_sources");
						OptionCheckBox("Grid", "r.grid");
						OptionCheckBox("Picking ray", "r.picking_ray");
						OptionCheckBox("Physics", "r.physics");
						OptionCheckBox("AABBs", "r.aabb");
						OptionCheckBox("Wireframe", "r.wireframe");
						OptionCheckBox("Occlusion culling", "r.hiz_occlusion", "For development purposes");
					}

					ImGui::EndTable();
				}
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
}

// -------------------------------------------------------
