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
#include <SceneryEditorX/environment/environment.h>
#include <SceneryEditorX/renderer/gpu_scene.h>
#include <SceneryEditorX/renderer/gpu_timer.h>
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

			if (EditorUI::SectionHeader("GPU SCENE") && EditorUI::BeginPropertyGrid("##RendererGpuScene"))
			{
				// GPU culling, LOD selection and instancing of the deferred geometry (GpuScene)
				EditorUI::PropertyLabel("GPU-driven");
				bool enabled = GpuScene::IsEnabled();
				if (ImGui::Checkbox("##GpuSceneEnabled", &enabled))
					GpuScene::SetEnabled(enabled);
				EditorUI::ItemTooltip("Culls, picks LODs and instances the scenery on the GPU. Off: one CPU draw call per object");

				static float minPixels = 1.0f;
				static float lodScale = 1.0f;
				static bool frustum = true;
				bool changed = false;
				EditorUI::PropertyLabel("Cull below");
				changed |= ImGui::SliderFloat("##GpuSceneMinPixels", &minPixels, 0.0f, 8.0f, "%.1f px");
				EditorUI::ItemTooltip("Objects whose bounding sphere covers fewer pixels than this are not drawn");
				EditorUI::PropertyLabel("LOD scale");
				changed |= ImGui::SliderFloat("##GpuSceneLodScale", &lodScale, 0.25f, 4.0f, "%.2f");
				EditorUI::ItemTooltip("Above 1 keeps mesh LODs at higher detail for longer");
				EditorUI::PropertyLabel("Frustum culling");
				changed |= ImGui::Checkbox("##GpuSceneFrustum", &frustum);
				if (changed)
					GpuScene::SetCullingOptions(minPixels, lodScale, frustum);

				EditorUI::PropertyLabel("Occlusion culling");
				bool occlusion = GpuScene::IsOcclusionCullingEnabled();
				if (ImGui::Checkbox("##GpuSceneOcclusion", &occlusion))
					GpuScene::SetOcclusionCulling(occlusion);
				EditorUI::ItemTooltip("Skips objects hidden behind nearer ones (two-phase Hi-Z test against this frame's depth)");

				const GpuScene::Stats &stats = GpuScene::GetStats();
				ValueText("Instances", "%u (%u active)", stats.instances, stats.enabledInstances);
				if (GpuScene::IsOcclusionActive())
				{
					ValueText("Visible", "%u (%u newly visible)", stats.visibleInstances, stats.lateInstances);
					ValueText("Occluded", "%u", stats.occludedInstances);
				}
				else
				{
					ValueText("Visible", "%u", stats.visibleInstances);
				}
				ValueText("Batches", "%u", stats.batches);
				ValueText("Draw commands", "%u of %u drawing", stats.drawnCommands, stats.commands);
				ValueText("Uploads", "%u instances, %u transforms checked", stats.uploadedInstances, stats.validatedTransforms);
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("XP12 EMULATION") && EditorUI::BeginPropertyGrid("##RendererXp12"))
			{
				// The switchable passes of the X-Plane 12 frame graph (--no-<name> / --with-<name> on the command line)
				RendererFeatures features = Renderer::GetFeatures();
				bool changed = false;
				for (const RendererFeatureInfo &feature : GetRendererFeatureTable())
				{
					const bool available = feature.implementedInPhase <= RENDERER_IMPLEMENTED_PHASE;
					EditorUI::PropertyLabel(feature.label);
					ImGui::PushID(feature.name);
					ImGui::BeginDisabled(!available);
					changed |= ImGui::Checkbox("##Feature", &(features.*feature.member));
					ImGui::EndDisabled();
					ImGui::PopID();
					if (available)
						EditorUI::ItemTooltip(feature.description);
					else
					{
						char tooltip[256];
						snprintf(tooltip, sizeof(tooltip), "%s\nNot in this build yet (plan phase %d)", feature.description, feature.implementedInPhase);
						EditorUI::ItemTooltip(tooltip);
					}
				}
				if (changed)
					Renderer::SetFeatures(features);

				EditorUI::PropertyLabel("Tone curve");
				static constexpr const char *curves[] = { "AgX (X-Plane 12.2+)", "ACES fit", "Reinhard" };
				int curve = Renderer::GetToneCurve() - 1;
				ImGui::BeginDisabled(!features.tonemap);
				if (ImGui::Combo("##ToneCurve", &curve, curves, 3))
					Renderer::SetToneCurve(curve + 1);
				ImGui::EndDisabled();

				EditorUI::PropertyLabel("Passes");
				if (ImGui::Button("All off"))
					Renderer::SetFeatures(RendererFeatures::AllOff());
				ImGui::SameLine();
				if (ImGui::Button("Defaults"))
					Renderer::SetFeatures(RendererFeatures{});
				EditorUI::ItemTooltip("All off: the frame graph with every switchable pass off (A/B against the plain renderer)");
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("EXPOSURE", false) && EditorUI::BeginPropertyGrid("##RendererExposure"))
			{
				// X-Plane 12's camera: histogram-metered EV100 pulled toward the sun elevation's target (or a fixed EV100),
				// then the output chain's art controls. Edits apply from the next frame; resources/environment/xp12_environment.json has the defaults.
				Renderer::EnvironmentConstants &constants = Renderer::EditEnvironmentConstants();
				ExposureSettings &exposure = constants.exposure;

				EditorUI::PropertyLabel("Mode");
				int mode = exposure.automatic ? 0 : 1;
				static constexpr const char *modes[] = { "Auto (metered)", "Manual" };
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::Combo("##ExposureMode", &mode, modes, 2))
				{
					exposure.automatic = mode == 0;
					Renderer::ResetExposure();
				}
				EditorUI::PropertyLabel("Manual EV100");
				ImGui::BeginDisabled(exposure.automatic);
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##ManualEv", &exposure.manualEv100, -2.0f, 18.0f, "%.2f");
				ImGui::EndDisabled();
				EditorUI::PropertyLabel("Compensation");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##Compensation", &exposure.compensation, -4.0f, 4.0f, "%+.2f EV");
				EditorUI::ItemTooltip("On top of the automatic exposure; positive brightens");

				const Renderer::ExposureReadout readout = Renderer::GetExposureReadout();
				if (readout.valid)
				{
					ValueText("Exposed at", "EV100 %.2f", readout.ev100);
					const float sunElevation = static_cast<float>(Environment::Get().GetSnapshot().derived.sun.apparentElevationDeg);
					ValueText("Metered", "EV100 %.2f", readout.meteredEv100);
					ValueText("Target", "EV100 %.2f (mid %.2f, bias %+.2f, range %.1f-%.1f)", readout.targetEv100, GetExposureMid(exposure, sunElevation),
						GetExposureBias(exposure, sunElevation), exposure.evMin, exposure.evMax);
					ValueText("Mean luminance", "%.3g nits", std::exp2(readout.meanLog2Nits));
				}
				else
				{
					ValueText("Exposed at", "not metered yet");
				}
				EditorUI::PropertyLabel("Adaptation");
				if (ImGui::Button("Reset"))
					Renderer::ResetExposure();
				EditorUI::ItemTooltip("Jump straight to the metered exposure");

				EditorUI::PropertyLabel("Bloom");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##Bloom", &constants.bloomIntensity, 0.0f, 0.25f, "%.3f");
				EditorUI::PropertyLabel("Fusion shadows");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##FusionShadows", &constants.exposureFusion.x, 0.0f, 4.0f, "%.2f EV");
				EditorUI::ItemTooltip("Exposure fusion: how far the shadows exposure is lifted (exposure_fusion/shadows)");
				EditorUI::PropertyLabel("Fusion highlights");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##FusionHighlights", &constants.fusionHighlights, 0.0f, 4.0f, "%.2f EV");
				EditorUI::PropertyLabel("Fusion strength");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##FusionStrength", &constants.fusionStrength, 0.0f, 1.0f, "%.2f");
				EditorUI::PropertyLabel("Saturation");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##Saturation", &constants.colorSlope.w, 0.0f, 2.0f, "%.2f");
				EditorUI::ItemTooltip("ASC-CDL saturation after the tone curve (color_management/sdr)");
				EditorUI::PropertyLabel("Grade slope");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##SlopeStrength", &constants.colorSlopeStrength, 0.0f, 1.0f, "%.2f");
				EditorUI::ItemTooltip("Share of X-Plane's color_management/sdr/slope applied after SEDX's AgX (0.5 balances sky and ground "
									  "against X-Plane 12.4.4; 1 is X-Plane's full cooling)");
				EditorUI::PropertyLabel("FSR sharpness");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##RcasSharpness", &constants.rcasSharpness, 0.0f, 1.0f, "%.2f");
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("ATMOSPHERE", false) && EditorUI::BeginPropertyGrid("##RendererAtmosphere"))
			{
				// The sky, haze and probes of plan Phase 6 (the visibility itself is in the ENVIRONMENT panel)
				Renderer::EnvironmentConstants &constants = Renderer::EditEnvironmentConstants();
				EditorUI::PropertyLabel("Haze layer top");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##HazeTop", &constants.hazeTopM, 100.0f, 5000.0f, "%.0f m");
				EditorUI::ItemTooltip("The surface haze layer's top above the ground (at least; very clear air mixes higher)");
				EditorUI::PropertyLabel("Haze fade band");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##HazeBand", &constants.hazeBandM, 10.0f, 5000.0f, "%.0f m");
				EditorUI::PropertyLabel("Aerial range");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##AerialRange", &constants.aerialRangeM, 16000.0f, 256000.0f, "%.0f m");
				EditorUI::ItemTooltip("How far the aerial perspective volume reaches");
				EditorUI::PropertyLabel("Airglow");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##Airglow", &constants.airglowNits, 0.0f, 0.01f, "%.5f nits", ImGuiSliderFlags_Logarithmic);
				EditorUI::ItemTooltip("The night sky's floor: airglow and unresolved starlight");
				EditorUI::PropertyLabel("Stars");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##Stars", &constants.starBrightness, 0.0f, 16.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
				EditorUI::PropertyLabel("Probe LOD bias");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##ProbeBias", &constants.probeSpecularBias, -2.0f, 2.0f, "%.2f");
				EditorUI::ItemTooltip("Reflections' blur: added to roughness x 5 when the probes are sampled");
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("SHADOWS", false) && EditorUI::BeginPropertyGrid("##RendererShadows"))
			{
				// Sun and cloud shadows of plan Phase 7 (X-Plane's shadow controls; the QUALITY shadow level sets the cascades)
				Renderer::EnvironmentConstants &constants = Renderer::EditEnvironmentConstants();
				EditorUI::PropertyLabel("Soft edges");
				ImGui::Checkbox("##ShadowSoft", &constants.shadowSoft);
				EditorUI::ItemTooltip("3 x 3 filtered shadow-map lookups (shadow/want_soft)");
				EditorUI::PropertyLabel("Receiver plane");
				ImGui::Checkbox("##ShadowReceiverPlane", &constants.shadowReceiverPlane);
				EditorUI::ItemTooltip("Soft lookups compare against the receiver's own plane, so thin gaps keep their shadow "
									  "(sedx/csm_receiver_plane); off: X-Plane's caster bias alone");
				EditorUI::PropertyLabel("Slope bias");
				ImGui::SetNextItemWidth(-FLT_MIN);
				float &slopeBias = constants.shadowReceiverPlane ? constants.shadowReceiverPlaneSlopeBias : constants.shadowBiasSlope;
				ImGui::SliderFloat("##ShadowBiasSlope", &slopeBias, 0.0f, 8.0f, "%.2f");
				EditorUI::ItemTooltip("Casters pushed away from the light by this many times their depth slope (shadow/bias_slope, "
									  "or sedx/csm_receiver_plane_slope_bias with the receiver plane)");
				EditorUI::PropertyLabel("Normal offset");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##ShadowNormalBias", &constants.shadowNormalBias, 0.0f, 4.0f, "%.2f texels");
				EditorUI::ItemTooltip("Receivers moved along their normal before the lookup (shadow/bias_n_constant; X-Plane uses 0)");
				EditorUI::PropertyLabel("Split lambda");
				ImGui::SetNextItemWidth(-FLT_MIN);
				float lambda = static_cast<float>(constants.shadow.lambda);
				if (ImGui::SliderFloat("##ShadowLambda", &lambda, 0.0f, 1.0f, "%.2f"))
					constants.shadow.lambda = lambda;
				EditorUI::ItemTooltip("1 logarithmic .. 0 uniform cascade splits (shadow/csm/lambda)");
				EditorUI::PropertyLabel("Cloud shadows");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##CloudShadowStrength", &constants.cloudShadowStrength, 0.0f, 1.0f, "%.2f");
				EditorUI::ItemTooltip("Share of the sunlight clouds can take away (clouds/cloud_shadow_lighten_ratio)");

				const CsmSetup &cascades = Renderer::GetShadowCascades();
				static constexpr const char *lights[] = { "none", "the sun", "the moon" };
				ValueText("Cast by", "%s", lights[std::min(Renderer::GetShadowLight(), 2u)]);
				for (uint32_t i = 0; i < cascades.count; ++i)
				{
					char label[32];
					snprintf(label, sizeof(label), "Cascade %u", i);
					ValueText(label, "%.1f - %.1f m, %.1f cm texels", cascades.cascades[i].depthNear, cascades.cascades[i].depthFar,
						cascades.cascades[i].texelSize * 100.0);
				}
				ValueText("GPU casters", "%u instance draws", GpuScene::GetStats().shadowInstances);
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("QUALITY", false) && EditorUI::BeginPropertyGrid("##RendererQuality"))
			{
				// X-Plane 12's graphics settings levels (Resources/settings.txt); passes read them as they arrive
				RendererQualityPreset preset = Renderer::GetQualityPreset();
				bool changed = false;
				const auto level = [&changed](const char *label, const char *id, int &value, const int count, const char *(*name)(int))
				{
					EditorUI::PropertyLabel(label);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::BeginCombo(id, name(value)))
					{
						for (int i = 0; i < count; ++i)
						{
							if (ImGui::Selectable(name(i), i == value))
							{
								value = i;
								changed = true;
							}
						}
						ImGui::EndCombo();
					}
				};
				level("Shadows", "##Shadows", preset.shadows, SHADOW_LEVELS, GetShadowLevelName);
				level("Ambient occlusion", "##Ao", preset.ao, AO_LEVELS, GetAoLevelName);
				level("Clouds", "##Clouds", preset.clouds, CLOUD_LEVELS, GetCloudLevelName);
				level("FSR", "##Fsr", preset.fsr, FSR_LEVELS, GetFsrLevelName);
				level("Draw distance", "##DrawDistance", preset.drawDistance, DRAW_DISTANCE_LEVELS, GetDrawDistanceLevelName);
				EditorUI::PropertyLabel("FXAA");
				changed |= ImGui::Checkbox("##Fxaa", &preset.fxaa);
				EditorUI::PropertyLabel("Preset");
				if (ImGui::Button("X-Plane defaults"))
				{
					preset = GetDefaultQualityPreset();
					changed = true;
				}
				EditorUI::ItemTooltip("X-Plane 12's defaults for desktop Vulkan (vkmtl_async): what the parity references use");
				if (changed)
					Renderer::SetQualityPreset(preset);
				const ShadowQuality shadow = GetShadowQuality(preset.shadows);
				ValueText("Shadow maps", "%d cascades to %.0f m, %u px", shadow.exteriorCascades, shadow.exteriorFarM, shadow.mapSize);
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("DEBUG VIEW", false) && EditorUI::BeginPropertyGrid("##RendererDebugView"))
			{
				EditorUI::PropertyLabel("Show");
				const Renderer_DebugView current = Renderer::GetDebugView();
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::BeginCombo("##DebugView", GetDebugViewName(current)))
				{
					for (int v = 0; v < static_cast<int>(Renderer_DebugView::Count); ++v)
					{
						const Renderer_DebugView view = static_cast<Renderer_DebugView>(v);
						const int phase = GetDebugViewPhase(view);
						char label[96];
						if (phase > RENDERER_IMPLEMENTED_PHASE)
							snprintf(label, sizeof(label), "%s (phase %d)", GetDebugViewName(view), phase);
						else
							snprintf(label, sizeof(label), "%s", GetDebugViewName(view));
						if (ImGui::Selectable(label, view == current))
							Renderer::SetDebugView(view);
					}
					ImGui::EndCombo();
				}
				EditorUI::EndPropertyGrid();
			}

			if (EditorUI::SectionHeader("GPU TIMINGS", false) && EditorUI::BeginPropertyGrid("##RendererGpuTimings"))
			{
				// Per pass, from timestamp queries of the last finished frame (also in the --perf-log counters)
				for (const GpuTimer::Result &result : GpuTimer::GetResults())
					ValueText(result.name.c_str(), "%.3f ms", result.milliseconds);
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
