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
 * settings_panels.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "settings_panels.h"
#include <Editor/core/editor_camera.h>
#include <Editor/core/editor_ui.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/time/fps_timer.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/ui/ui_impl.h>
#include <SceneryEditorX/renderer/vulkan/swapchain.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		const char *FormatName(const VkFormat format)
		{
			switch (format)
			{
				case VK_FORMAT_B8G8R8A8_SRGB:				return "B8G8R8A8_SRGB";
				case VK_FORMAT_B8G8R8A8_UNORM:				return "B8G8R8A8_UNORM";
				case VK_FORMAT_R8G8B8A8_SRGB:				return "R8G8B8A8_SRGB";
				case VK_FORMAT_R8G8B8A8_UNORM:				return "R8G8B8A8_UNORM";
				case VK_FORMAT_A2B10G10R10_UNORM_PACK32:	return "A2B10G10R10_UNORM";
				case VK_FORMAT_R16G16B16A16_SFLOAT:			return "R16G16B16A16_SFLOAT";
				default:									return "Other";
			}
		}

		std::string FormatBytes(const uint64_t bytes)
		{
			constexpr double MiB = 1024.0 * 1024.0;
			char text[32];
			snprintf(text, sizeof(text), "%.1f MiB", static_cast<double>(bytes) / MiB);
			return text;
		}

		void ValueText(const char *label, const char *format, ...)
		{
			EditorUI::PropertyLabel(label);
			va_list args;
			va_start(args, format);
			ImGui::TextV(format, args);
			va_end(args);
		}
	}

	// -------------------------------------------------------

	SceneSettingsPanel::SceneSettingsPanel()
	{
		m_Title = "SCENE SETTINGS";
	}

	void SceneSettingsPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("SCENE SETTINGS###SceneSettings", &isOpen))
		{
			if (EditorUI::SectionHeader("GENERAL PROPERTIES") && EditorUI::BeginPropertyGrid("##SceneGeneral"))
			{
				const std::string &path = Scene::GetFilePath();
				ValueText("Name", "%s", Scene::GetName().c_str());
				ValueText("File", "%s", path.empty() ? "Not saved" : path.c_str());
				ValueText("Entities", "%zu", Scene::GetEntities().size());
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("ENVIRONMENT") && EditorUI::BeginPropertyGrid("##SceneEnvironment"))
			{
				EditorUI::PropertyLabel("Time of day");
				float timeOfDay = Scene::GetTimeOfDay();
				if (ImGui::SliderFloat("##TimeOfDay", &timeOfDay, 0.0f, 24.0f, "%.2f h"))
					Scene::SetTimeOfDay(timeOfDay);

				EditorUI::PropertyLabel("Wind (m/s)");
				const Vec3 &wind = Scene::GetWind();
				float windComponents[3] = { wind.x, wind.y, wind.z };
				if (ImGui::DragFloat3("##Wind", windComponents, 0.1f))
					Scene::SetWind(Vec3(windComponents[0], windComponents[1], windComponents[2]));

				EditorUI::EndPropertyGrid();
			}
		}
		EditorUI::EndPanel();
	}

	// -------------------------------------------------------

	CameraSettingsPanel::CameraSettingsPanel()
	{
		m_Title = "CAMERA";
	}

	void CameraSettingsPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("CAMERA###CameraSettings", &isOpen))
		{
			Camera *camera = EditorCamera::Get();
			if (!camera)
			{
				EditorUI::EmptyState("No scene camera");
			}
			else
			{
				if (EditorUI::SectionHeader("NAVIGATION") && EditorUI::BeginPropertyGrid("##CameraNavigation"))
				{
					EditorUI::PropertyLabel("Mode");
					int mode = EditorCamera::IsMode(FLY) ? 1 : 0;
					if (ImGui::Combo("##Mode", &mode, "Orbit\0Fly\0"))
						EditorCamera::SetMode(mode == 1 ? FLY : ORBIT);

					EditorUI::PropertyLabel("Projection");
					int projection = EditorCamera::IsProjection(CameraType::Orthographic) ? 1 : 0;
					if (ImGui::Combo("##Projection", &projection, "Perspective\0Orthographic\0"))
						EditorCamera::SetProjection(projection == 1 ? CameraType::Orthographic : CameraType::Perspective);

					if (camera->mode == ORBIT)
					{
						EditorUI::PropertyLabel("Orbit center");
						float center[3] = { camera->center.x, camera->center.y, camera->center.z };
						if (ImGui::DragFloat3("##Center", center, 0.05f))
							camera->center = Vec3(center[0], center[1], center[2]);

						EditorUI::PropertyLabel("Distance");
						float distance = camera->GetOrbitDistance();
						if (ImGui::DragFloat("##Distance", &distance, 0.05f, 0.1f, 100000.0f, "%.2f"))
							camera->SetOrbitDistance(distance);
					}
					else
					{
						EditorUI::PropertyLabel("Position");
						float eye[3] = { camera->eye.x, camera->eye.y, camera->eye.z };
						if (ImGui::DragFloat3("##Eye", eye, 0.05f))
							camera->eye = Vec3(eye[0], eye[1], eye[2]);
					}

					EditorUI::PropertyLabel("Reset");
					if (ImGui::Button("Reset to startup view", ImVec2(-FLT_MIN, 0.0f)))
						EditorCamera::Reset();

					EditorUI::EndPropertyGrid();
				}

				if (EditorUI::SectionHeader("LENS") && EditorUI::BeginPropertyGrid("##CameraLens"))
				{
					EditorUI::PropertyLabel("Field of view");
					float fov = camera->GetFovHorizontalDeg();
					if (ImGui::SliderFloat("##Fov", &fov, 10.0f, 150.0f, "%.0f deg"))
						camera->SetFovHorizontalDeg(fov);

					ValueText("Near / far", "%.2f / %.0f", camera->GetNearPlane(), camera->GetFarPlane());
					EditorUI::EndPropertyGrid();
				}

				if (EditorUI::SectionHeader("EXPOSURE", false) && EditorUI::BeginPropertyGrid("##CameraExposure"))
				{
					EditorUI::PropertyLabel("Aperture (f)");
					float aperture = camera->GetAperture();
					if (ImGui::DragFloat("##Aperture", &aperture, 0.05f, 0.7f, 32.0f, "%.1f"))
						camera->SetAperture(aperture);

					EditorUI::PropertyLabel("Shutter (s)");
					float shutter = camera->GetShutterSpeed();
					if (ImGui::DragFloat("##Shutter", &shutter, 0.0005f, 0.0001f, 1.0f, "%.4f"))
						camera->SetShutterSpeed(shutter);

					EditorUI::PropertyLabel("ISO");
					float iso = camera->GetIso();
					if (ImGui::DragFloat("##Iso", &iso, 10.0f, 50.0f, 12800.0f, "%.0f"))
						camera->SetIso(iso);

					EditorUI::EndPropertyGrid();
				}
			}
		}
		EditorUI::EndPanel();
	}

	// -------------------------------------------------------

	RendererStatsPanel::RendererStatsPanel()
	{
		m_Title = "RENDERER";
	}

	void RendererStatsPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("RENDERER###RendererStats", &isOpen))
		{
			if (EditorUI::SectionHeader("FRAME") && EditorUI::BeginPropertyGrid("##RendererFrame"))
			{
				const double frameMs = FPSTimer::GetDeltaTimeSmoothedMs();
				const auto &timers = Application::Get().m_PerformanceTimers;
				ValueText("Frame", "%llu", static_cast<unsigned long long>(Renderer::GetFrameNumber()));
				ValueText("Frame time", "%.2f ms (%.0f fps)", frameMs, frameMs > 0.0 ? 1000.0 / frameMs : 0.0);
				ValueText("Main thread", "%.2f ms work, %.2f ms wait", timers.MainThreadWorkTime, timers.MainThreadWaitTime);
				ValueText("Render thread", "%.2f ms work, %.2f ms wait", timers.RenderThreadWorkTime, timers.RenderThreadWaitTime);
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("OUTPUT") && EditorUI::BeginPropertyGrid("##RendererOutput"))
			{
				const Vec2 &render = Renderer::GetRendererResolution();
				const Vec2 &output = Renderer::GetOutputResolution();
				ValueText("Render resolution", "%.0f x %.0f", render.x, render.y);
				ValueText("Output resolution", "%.0f x %.0f", output.x, output.y);

				if (Swapchain *swapchain = Renderer::GetSwapChain())
				{
					const VkExtent2D extent = swapchain->GetExtent();
					ValueText("Swapchain", "%u x %u, %zu images", extent.width, extent.height, swapchain->GetImages().size());
					ValueText("Format", "%s", FormatName(swapchain->GetImageFormat()));
				}
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("GPU MEMORY") && EditorUI::BeginPropertyGrid("##RendererMemory"))
			{
				const GPUMemoryStats stats = Renderer::GetGPUMemoryStats();
				ValueText("Used", "%s of %s", FormatBytes(stats.used).c_str(), FormatBytes(stats.totalAvailable).c_str());
				ValueText("Allocations", "%llu", static_cast<unsigned long long>(stats.allocationCount));
				ValueText("Buffers", "%s (%llu)", FormatBytes(stats.bufferAllocationSize).c_str(), static_cast<unsigned long long>(stats.bufferAllocationCount));
				ValueText("Images", "%s (%llu)", FormatBytes(stats.imageAllocationSize).c_str(), static_cast<unsigned long long>(stats.imageAllocationCount));
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("USER INTERFACE", false) && EditorUI::BeginPropertyGrid("##RendererUI"))
			{
				const ImGuiIO &io = ImGui::GetIO();
				ValueText("Dear ImGui", "%s (docking)", IMGUI_VERSION);
				ValueText("Backends", "%s / %s", io.BackendPlatformName ? io.BackendPlatformName : "-", io.BackendRendererName ? io.BackendRendererName : "-");
				ValueText("Floating panels", "%s", (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) ? "Enabled" : "Disabled");
				ValueText("sRGB target", "%s", UI::IsLinearColorTarget() ? "Yes (theme linearized)" : "No");
				EditorUI::EndPropertyGrid();
			}
		}
		EditorUI::EndPanel();
	}

}

// -------------------------------------------------------
