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
 * scene_viewport.cpp
 * -------------------------------------------------------
 * Created: 01/08/2025
 * -------------------------------------------------------
 */
#include "scene_viewport.h"
#include <Editor/core/editor_camera.h>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_tools.h>
#include <Editor/core/editor_ui.h>
#include <Editor/core/editor_xplane.h>
#include <Editor/core/facade_tool.h>
#include <Editor/core/node_tool.h>
#include <Editor/core/transform_tool.h>
#include <Editor/core/viewport_menus.h>
#include <Editor/core/viewport_view.h>
#include <Editor/settings/editor_preferences.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/ui_impl.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/xplane/xp_library.h>
#include <SceneryEditorX/xplane/xp_texture_loader.h>
#include <portable-file-dialogs.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		/* Frames the panel size must stay unchanged before render targets are recreated at the new size. */
		constexpr uint32_t RESIZE_SETTLE_FRAMES = 6;

		/* Below this the renderer refuses to render (see Renderer::BeginFrame). */
		constexpr uint32_t MIN_RENDER_SIZE = 64;

		constexpr float TOOLBAR_HEIGHT = 30.0f;

		// SetFixedRenderSize(): parity captures render at the reference's size whatever the panel's
		uint32_t s_FixedWidth = 0;
		uint32_t s_FixedHeight = 0;

		std::filesystem::path s_RequestedReference;
		SceneViewport::ReferenceView s_RequestedReferenceView = SceneViewport::ReferenceView::Split;
	}

	void SceneViewport::RequestReference(const std::filesystem::path &png, const ReferenceView view)
	{
		s_RequestedReference = png;
		s_RequestedReferenceView = view;
	}

	void SceneViewport::SetFixedRenderSize(const uint32_t width, const uint32_t height)
	{
		s_FixedWidth = width;
		s_FixedHeight = height;
	}

	SceneViewport::SceneViewport(EditorContext *context) : m_Context(context)
	{
		m_Title = "VIEWPORT";
	}

	bool SceneViewport::PickGround(const float u, const float v, const float aspect, float out[3])
	{
		// Same camera the renderer draws through (perspective or orthographic): a unit-high image of the given aspect
		if (aspect <= 0.0f)
			return false;
		const ViewportView view = ViewportView::Build(ImVec2(0.0f, 0.0f), ImVec2(aspect, 1.0f), aspect);
		Point3 hit;
		if (!view.valid || !view.RayToPlane(ImVec2(u * aspect, v), 0.0, hit))
			return false;	// Looking at or above the horizon
		out[0] = static_cast<float>(hit.x);
		out[1] = 0.0f;
		out[2] = static_cast<float>(hit.z);
		return true;
	}

	void SceneViewport::OnUIRender(bool &isOpen)
	{
		// The viewport fills its dock node edge to edge; its tab bar only shows if other panels get docked into it.
		ImGuiWindowClass windowClass;
		windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar;
		ImGui::SetNextWindowClass(&windowClass);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		const bool visible = EditorUI::BeginPanel("VIEWPORT###Viewport", &isOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar();

		bool hovered = false;
		bool dragging = false;

		if (visible)
		{
			Camera *camera = EditorCamera::Get();
			DrawToolbar(camera);

			const ImVec2 panelMin = ImGui::GetCursorScreenPos();
			const ImVec2 panelSize = ImGui::GetContentRegionAvail();
			ImVec2 imageMin = panelMin;
			ImVec2 imageSize = panelSize;
			uint32_t width  = static_cast<uint32_t>(std::max(panelSize.x, 0.0f));
			uint32_t height = static_cast<uint32_t>(std::max(panelSize.y, 0.0f));
			if (s_FixedWidth > 0 && s_FixedHeight > 0 && panelSize.x > 0.0f && panelSize.y > 0.0f)
			{
				// A fixed render size is shown fitted into the panel, centered, with black bars
				width = s_FixedWidth;
				height = s_FixedHeight;
				const float scale = std::min(panelSize.x / static_cast<float>(width), panelSize.y / static_cast<float>(height));
				imageSize = ImVec2(std::floor(static_cast<float>(width) * scale), std::floor(static_cast<float>(height) * scale));
				imageMin = ImVec2(std::floor(panelMin.x + (panelSize.x - imageSize.x) * 0.5f), std::floor(panelMin.y + (panelSize.y - imageSize.y) * 0.5f));
				ImGui::GetWindowDrawList()->AddRectFilled(panelMin, ImVec2(panelMin.x + panelSize.x, panelMin.y + panelSize.y), IM_COL32(0, 0, 0, 255));
			}
			UpdateRenderSize(width, height);
			if (!s_RequestedReference.empty())
			{
				if (LoadReference(s_RequestedReference))
					m_ReferenceView = s_RequestedReferenceView;
				s_RequestedReference.clear();
			}
			UpdateReferenceOverlay();

			m_ViewportBounds[0] = Vec2(imageMin.x, imageMin.y);
			m_ViewportBounds[1] = Vec2(imageMin.x + imageSize.x, imageMin.y + imageSize.y);

			if (imageSize.x > 0.0f && imageSize.y > 0.0f)
			{
				ImDrawList *drawList = ImGui::GetWindowDrawList();
				const ImVec2 imageMax(imageMin.x + imageSize.x, imageMin.y + imageSize.y);
				const ImTextureID texture = UI::GetTextureID(Renderer::GetRenderTarget(Renderer_RenderTarget::frame_output));
				if (texture != ImTextureID_Invalid)
				{
					// Opaque backdrop: ImGui alpha-blends images, and pixels the renderer leaves transparent must
					// show as black (what the renderer produced), not as the panel colour behind the viewport.
					drawList->AddRectFilled(imageMin, imageMax, IM_COL32(0, 0, 0, 255));
					drawList->AddImage(texture, imageMin, imageMax);
				}
				else
				{
					drawList->AddRectFilled(imageMin, imageMax, EditorTheme::ToU32(EditorTheme::Palette::Viewport));
					constexpr const char *message = "Waiting for the renderer output...";
					const ImVec2 textSize = ImGui::CalcTextSize(message);
					drawList->AddText(ImVec2(imageMin.x + (imageSize.x - textSize.x) * 0.5f, imageMin.y + (imageSize.y - textSize.y) * 0.5f),
						EditorTheme::ToU32(EditorTheme::Palette::TextDisabled), message);
				}

				// The editing tools draw over the image. Their handles come before the image's input item: ImGuizmo only
				// takes a click while no ImGui item is hovered, so the item is left out while a handle has the mouse.
				const float renderWidth = m_AppliedWidth > 0 ? static_cast<float>(m_AppliedWidth) : imageSize.x;
				const float renderHeight = m_AppliedHeight > 0 ? static_cast<float>(m_AppliedHeight) : imageSize.y;
				const ViewportView view = ViewportView::Build(imageMin, imageSize, renderWidth / renderHeight);
				const bool imageHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseHoveringRect(imageMin, imageMax);
				const bool splitHasMouse = DrawReferenceSplit(imageMin, imageSize);
				const bool toolHasMouse = EditorTools::BeginViewport(m_Context, view, imageHovered && !splitHasMouse && !EditorCamera::IsNavigating()) || splitHasMouse;

				// Camera navigation with the user's keymap: drags start over the image unless a tool, gizmo or menu has the mouse
				ViewportCamera::Input navigation;
				navigation.hovered = imageHovered && !splitHasMouse;
				navigation.blocked = toolHasMouse || EditorTools::IsBusy() || ViewportMenus::IsOpen();
				navigation.imageSize = imageSize;
				navigation.aspect = renderWidth / renderHeight;
				EditorCamera::Update(navigation);

				bool canvasHovered = false;
				ImGui::SetCursorScreenPos(imageMin);
				if (!toolHasMouse)
				{
					// The whole image is one button so a drag that starts here keeps camera control after leaving it.
					ImGui::InvisibleButton("##ViewportCanvas", imageSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
					canvasHovered = ImGui::IsItemHovered();
					dragging = ImGui::IsItemActive();

					// X-Plane library objects dropped on the image land on the ground plane under the cursor.
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(EditorXPlane::ASSET_PAYLOAD))
						{
							float ground[3] = { 0.0f, 0.0f, 0.0f };
							const ImVec2 mouse = ImGui::GetMousePos();
							const bool hit = PickGround((mouse.x - imageMin.x) / imageSize.x, (mouse.y - imageMin.y) / imageSize.y, imageSize.x / imageSize.y, ground);
							const std::string path = static_cast<const char *>(payload->Data);
							// A facade gets a default footprint on the ground there (in the open project); objects are placed as before
							if (XPlane::GetAssetType(path) == XPlane::AssetType::Facade)
								FacadeTool::Drop(m_Context, path, hit ? ground : nullptr);
							else
								EditorXPlane::PlaceLibraryAsset(path, m_Context, hit ? ground : nullptr);
						}
						ImGui::EndDragDropTarget();
					}
				}
				else
					ImGui::Dummy(imageSize);
				hovered = canvasHovered || (toolHasMouse && imageHovered);

				EditorTools::EndViewport(m_Context, view, canvasHovered, renderWidth, renderHeight);
				DrawOverlay(camera, imageMin);
			}
		}
		EditorUI::EndPanel();

		if (m_Context)
		{
			m_Context->viewportHovered = hovered;
			m_Context->viewportFocused = visible && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		}

		// Camera input: while the image is hovered (wheel, keyboard fly) or a drag that started on it continues, and no
		// tool drags (gizmo, nodes, a selection box)
		const bool typing = ImGui::GetIO().WantTextInput;
		Camera::SetInputEnabled(isOpen && !EditorTools::IsBusy() && (dragging || (hovered && !typing)));
	}

	void SceneViewport::DrawToolbar(Camera *camera)
	{
		using namespace EditorTheme;
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float height = TOOLBAR_HEIGHT * dpi;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ToStyle(Palette::PanelHeader));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f * dpi, 3.0f * dpi));
		ImGui::BeginChild("##ViewportToolbar", ImVec2(0.0f, height), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
		ImGui::PopStyleVar();

		const float buttonSize = height - 6.0f * dpi;
		const float rightEdge = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
		const bool hasCamera = camera != nullptr;

		// Left: the editing tools, then navigation mode and projection, as toggle buttons.
		for (int t = 0; t < static_cast<int>(EditorTools::Tool::Count); ++t)
		{
			const EditorTools::Tool tool = static_cast<EditorTools::Tool>(t);
			ImGui::PushID(t);
			if (EditorUI::StripButton(EditorTools::GetToolIcon(tool), EditorTools::GetToolTooltip(tool), EditorTools::GetTool() == tool, buttonSize))
				EditorTools::SetTool(tool);
			ImGui::PopID();
			ImGui::SameLine(0.0f, 2.0f * dpi);
		}

		// Tool options: snapping, gizmo axes, node edits
		TransformTool::Settings &settings = TransformTool::GetSettings();
		ImGui::SameLine(0.0f, 8.0f * dpi);
		if (EditorUI::StripButton(SEDX_ICON_MAGNET, "Snapping (hold Ctrl while dragging to invert)\nRight-click: snap steps", settings.snap, buttonSize, EditorUI::StripStyle::Tinted))
			settings.snap = !settings.snap;
		if (ImGui::BeginPopupContextItem("##SnapSteps"))
		{
			ImGui::SetNextItemWidth(120.0f * dpi);
			ImGui::DragFloat("Move (m)", &settings.snapMove, 0.05f, 0.01f, 1000.0f, "%.2f");
			ImGui::SetNextItemWidth(120.0f * dpi);
			ImGui::DragFloat("Rotate (deg)", &settings.snapAngle, 0.5f, 0.5f, 180.0f, "%.1f");
			ImGui::SetNextItemWidth(120.0f * dpi);
			ImGui::DragFloat("Scale", &settings.snapScale, 0.01f, 0.01f, 10.0f, "%.2f");
			ImGui::EndPopup();
		}
		ImGui::SameLine(0.0f, 2.0f * dpi);
		if (EditorUI::StripButton(settings.localSpace ? SEDX_ICON_CUBE : SEDX_ICON_GLOBE, settings.localSpace ? "Gizmo: local axes (X)\nClick for world axes" : "Gizmo: world axes (X)\nClick for the active object's axes", settings.localSpace, buttonSize, EditorUI::StripStyle::Tinted))
			settings.localSpace = !settings.localSpace;
		ImGui::SameLine(0.0f, 2.0f * dpi);
		if (EditorUI::StripButton(SEDX_ICON_DOT_CIRCLE_O, settings.pivotActive ? "Pivot: the active object\nClick to turn and scale about the selection's center" : "Pivot: the selection's center\nClick to turn and scale about the active object", settings.pivotActive, buttonSize, EditorUI::StripStyle::Tinted))
			settings.pivotActive = !settings.pivotActive;

		if (EditorTools::GetTool() == EditorTools::Tool::Nodes)
		{
			using Splines::NodeType;
			const bool shapes = NodeTool::HasShapes();
			const bool nodes = NodeTool::GetSelectedNodeCount() > 0;

			// Node types: the selection's own type shows active
			NodeType selectedType = NodeType::Corner;
			const bool sameType = NodeTool::GetSelectedNodeType(selectedType);
			struct TypeButton
			{
				NodeType type;
				const char *icon;
				const char *tooltip;
			};
			static constexpr TypeButton TYPE_BUTTONS[] = {
				{ NodeType::Corner, SEDX_ICON_SQUARE_O, "Corner (Shift+C)\nRemove the handles: the segments meet in a sharp point" },
				{ NodeType::Smooth, SEDX_ICON_CIRCLE_O, "Smooth (Shift+S)\nHandles opposite each other, each keeping its own length: the curve runs through without a kink" },
				{ NodeType::Symmetric, SEDX_ICON_ARROWS_H, "Symmetric (Shift+Y)\nMirror-image handles: same length either side" },
				{ NodeType::Free, SEDX_ICON_CHAIN_BROKEN, "Free (Shift+F)\nHandles move on their own (a cusp). Shift while dragging a handle does the same" },
			};
			ImGui::SameLine(0.0f, 8.0f * dpi);
			ImGui::BeginDisabled(!nodes);
			for (const TypeButton &button : TYPE_BUTTONS)
			{
				ImGui::PushID(button.tooltip);
				if (EditorUI::StripButton(button.icon, button.tooltip, sameType && selectedType == button.type, buttonSize, EditorUI::StripStyle::Tinted))
					NodeTool::SetSelectedNodeType(button.type);
				ImGui::PopID();
				ImGui::SameLine(0.0f, 2.0f * dpi);
			}
			if (EditorUI::StripButton(SEDX_ICON_MINUS, "Straighten (Shift+L)\nMake the segments between selected nodes straight", false, buttonSize))
				NodeTool::StraightenSelectedSegments();
			ImGui::SameLine(0.0f, 2.0f * dpi);
			if (EditorUI::StripButton(SEDX_ICON_TRASH, "Delete the selected nodes (Del)", false, buttonSize))
				NodeTool::DeleteSelectedNodes();
			ImGui::EndDisabled();

			// Curve mode and chain edits
			ImGui::SameLine(0.0f, 8.0f * dpi);
			ImGui::BeginDisabled(!shapes || !NodeTool::CanUseBSpline());
			const bool spline = NodeTool::IsBSplineActive();
			if (EditorUI::StripButton(SEDX_ICON_DIAMOND, spline ? "B-spline (Shift+B)\nEditing the control polygon: click to go back to nodes and handles"
															   : "B-spline (Shift+B)\nEdit the selected chains through a control polygon: the curve stays smooth (C2)\n"
																 "and only changes near the points that move. Nodes become the control points",
					spline, buttonSize, EditorUI::StripStyle::Tinted))
				NodeTool::SetBSpline(!spline);
			ImGui::EndDisabled();
			ImGui::SameLine(0.0f, 2.0f * dpi);
			bool closed = false;
			const bool closable = NodeTool::CanToggleClosed(&closed);
			ImGui::BeginDisabled(!closable);
			if (EditorUI::StripButton(SEDX_ICON_CIRCLE_O_NOTCH, closed ? "Open the line at its selected node (or its first)" : "Close the line (lines and strings)\nOr Ctrl+click its other end with one end selected",
					closable && closed, buttonSize, EditorUI::StripStyle::Tinted))
				NodeTool::ToggleClosed();
			ImGui::EndDisabled();
			ImGui::SameLine(0.0f, 2.0f * dpi);
			ImGui::BeginDisabled(!shapes);
			if (EditorUI::StripButton(SEDX_ICON_EXCHANGE, "Reverse the direction of the selected shapes (or of the selected nodes' chains)\nFacades face, strings and lines run by their direction", false, buttonSize))
				NodeTool::ReverseSelectedShapes();
			ImGui::EndDisabled();
		}

		if (EditorTools::GetTool() == EditorTools::Tool::Facade)
		{
			ImGui::SameLine(0.0f, 8.0f * dpi);
			FacadeTool::DrawToolbar(m_Context, buttonSize);
		}

		ImGui::SameLine(0.0f, 10.0f * dpi);
		ImGui::BeginDisabled(!hasCamera);
		const std::string orbitTip = std::string("Orbit navigation (") + GetNavigationKeymapName(EditorPreferences::Get().keymap) + " keymap)\n" +
									 ViewportCamera::GetKeymapSummary(EditorPreferences::Get().keymap) + "\nRight mouse looks around. Edit > Preferences changes the keymap";
		if (EditorUI::StripButton(SEDX_ICON_REFRESH, orbitTip.c_str(), EditorCamera::IsMode(ORBIT), buttonSize))
			EditorCamera::SetMode(ORBIT);
		ImGui::SameLine(0.0f, 2.0f * dpi);
		if (EditorUI::StripButton(SEDX_ICON_VIDEO_CAMERA, "Fly navigation\nRMB look, hold RMB + WASD move, Q/E down/up; the wheel moves the camera", EditorCamera::IsMode(FLY), buttonSize))
			EditorCamera::SetMode(FLY);

		ImGui::SameLine(0.0f, 10.0f * dpi);
		DrawViewMenu(buttonSize);

		// Right: the reference image, reset view.
		ImGui::SameLine(rightEdge - buttonSize * 2.0f - 2.0f * dpi);
		DrawReferenceMenu(buttonSize);
		ImGui::SameLine(rightEdge - buttonSize);
		if (EditorUI::StripButton(SEDX_ICON_CROSSHAIRS, "Reset camera to the startup view", false, buttonSize))
			EditorCamera::Reset();

		ImGui::EndDisabled();

		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	void SceneViewport::DrawOverlay(Camera *camera, const ImVec2 &imageMin) const
	{
		using namespace EditorTheme;
		const float dpi = ImGui::GetStyle().FontScaleDpi;

		char text[128];
		if (camera)
		{
			snprintf(text, sizeof(text), "%s  |  %s %s  |  %u x %u", EditorCamera::GetModeName(), ViewportCamera::GetPresetName(EditorCamera::GetViewPreset()),
				EditorCamera::IsProjection(CameraType::Orthographic) ? "Orthographic" : "Perspective", m_AppliedWidth, m_AppliedHeight);
		}
		else
		{
			snprintf(text, sizeof(text), "No camera  |  %u x %u", m_AppliedWidth, m_AppliedHeight);
		}

		const ImVec2 padding(8.0f * dpi, 4.0f * dpi);
		const ImVec2 textSize = ImGui::CalcTextSize(text);
		const ImVec2 boxMin(imageMin.x + 8.0f * dpi, imageMin.y + 8.0f * dpi);
		const ImVec2 boxMax(boxMin.x + textSize.x + padding.x * 2.0f, boxMin.y + textSize.y + padding.y * 2.0f);

		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(boxMin, boxMax, ToU32(ImVec4(Palette::Separator.x, Palette::Separator.y, Palette::Separator.z, 0.75f)), 2.0f * dpi);
		drawList->AddText(ImVec2(boxMin.x + padding.x, boxMin.y + padding.y), ToU32(Palette::TextLabel), text);

		if (m_Reference && m_ReferenceView != ReferenceView::Off)
		{
			static constexpr const char *views[] = { "", "split", "difference", "flicker" };
			const std::string name = m_ReferencePath.filename().string();
			snprintf(text, sizeof(text), "REFERENCE  %s  |  %s", name.c_str(), views[static_cast<int>(m_ReferenceView)]);
			const ImVec2 size = ImGui::CalcTextSize(text);
			const ImVec2 min(boxMin.x, boxMax.y + 4.0f * dpi);
			const ImVec2 max(min.x + size.x + padding.x * 2.0f, min.y + size.y + padding.y * 2.0f);
			drawList->AddRectFilled(min, max, ToU32(ImVec4(Palette::Separator.x, Palette::Separator.y, Palette::Separator.z, 0.75f)), 2.0f * dpi);
			drawList->AddText(ImVec2(min.x + padding.x, min.y + padding.y), ToU32(Palette::TextLabel), text);
		}
	}

	void SceneViewport::DrawViewMenu(const float buttonSize)
	{
		// Projection toggle (numpad 5), then the view presets in a drop-down
		const bool orthographic = EditorCamera::IsProjection(CameraType::Orthographic);
		if (EditorUI::StripButton(orthographic ? SEDX_ICON_TH_LARGE : SEDX_ICON_CUBE,
				orthographic ? "Orthographic projection (Numpad 5)\nClick for perspective" : "Perspective projection (Numpad 5)\nClick for orthographic", orthographic, buttonSize, EditorUI::StripStyle::Tinted))
			EditorCamera::ToggleProjection();

		using ViewportCamera::ViewPreset;
		const ViewPreset current = EditorCamera::GetViewPreset();
		ImGui::SameLine(0.0f, 2.0f * ImGui::GetStyle().FontScaleDpi);
		if (EditorUI::StripButton(SEDX_ICON_EYE, (std::string("View: ") + ViewportCamera::GetPresetName(current) + "\nTop, front, side views and framing").c_str(), current != ViewPreset::Free, buttonSize))
			ImGui::OpenPopup("##ViewMenu");

		if (!ImGui::BeginPopup("##ViewMenu"))
			return;
		ImGui::TextUnformatted("VIEW");
		ImGui::Separator();
		struct Item
		{
			ViewPreset preset;
			const char *shortcut;
		};
		static constexpr Item ITEMS[] = {
			{ ViewPreset::Top, "Numpad 7" }, { ViewPreset::Bottom, "Ctrl+Numpad 7" }, { ViewPreset::Front, "Numpad 1" },
			{ ViewPreset::Back, "Ctrl+Numpad 1" }, { ViewPreset::Right, "Numpad 3" }, { ViewPreset::Left, "Ctrl+Numpad 3" },
		};
		for (const Item &item : ITEMS)
		{
			if (ImGui::MenuItem(ViewportCamera::GetPresetName(item.preset), item.shortcut, current == item.preset))
				EditorCamera::SetViewPreset(item.preset);
		}
		if (ImGui::MenuItem("Opposite Side", "Numpad 9"))
			EditorCamera::ViewFromOtherSide();
		ImGui::Separator();
		if (ImGui::MenuItem("Perspective", "Numpad 5", !orthographic))
			EditorCamera::SetProjection(CameraType::Perspective);
		if (ImGui::MenuItem("Orthographic", "Numpad 5", orthographic))
			EditorCamera::SetProjection(CameraType::Orthographic);
		ImGui::Separator();
		if (ImGui::MenuItem("Frame Selection", "Numpad ."))
			EditorTools::FrameSelection();
		if (ImGui::MenuItem("Frame All", "Home"))
			EditorCamera::FrameAll();
		ImGui::EndPopup();
	}

	bool SceneViewport::LoadReference(const std::filesystem::path &png)
	{
		XPlane::TextureInfo info;
		Ref<ImageResource> image = XPlane::TextureLoader::Load(png, XPlane::TextureUsage::Data, &info);
		if (!image)
		{
			EDITOR_ERROR_TAG("Viewport", "Reference image {} could not be loaded: {}", png.string(), info.error);
			return false;
		}
		m_Reference = image;
		m_ReferencePath = png;
		if (m_ReferenceView == ReferenceView::Off)
			m_ReferenceView = ReferenceView::Split;
		EDITOR_INFO_TAG("Viewport", "Reference image {} ({} x {})", png.string(), info.width, info.height);
		return true;
	}

	void SceneViewport::DrawReferenceMenu(const float buttonSize)
	{
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		if (EditorUI::StripButton(SEDX_ICON_PICTURE_O, "Reference image\nCompare the view with an X-Plane 12 screenshot: split, difference or flicker", m_Reference && m_ReferenceView != ReferenceView::Off, buttonSize))
			ImGui::OpenPopup("##ReferenceMenu");

		if (!ImGui::BeginPopup("##ReferenceMenu"))
			return;

		ImGui::TextUnformatted("REFERENCE");
		ImGui::Separator();
		if (ImGui::MenuItem("Load reference image..."))
		{
			const std::string start = m_ReferencePath.empty() ? std::string() : m_ReferencePath.parent_path().string();
			const std::vector<std::string> files = pfd::open_file("Open a reference image", start, { "PNG images", "*.png", "All files", "*" }).result();
			if (!files.empty())
				LoadReference(files.front());
		}
		if (m_Reference)
			ImGui::TextDisabled("%s", m_ReferencePath.filename().string().c_str());
		else
			ImGui::TextDisabled("No image loaded");

		ImGui::Separator();
		ImGui::BeginDisabled(!m_Reference);
		static constexpr const char *labels[] = { "Off", "Split", "Difference (x gain)", "Flicker (1 Hz)" };
		for (int i = 0; i < 4; ++i)
		{
			if (ImGui::RadioButton(labels[i], m_ReferenceView == static_cast<ReferenceView>(i)))
				m_ReferenceView = static_cast<ReferenceView>(i);
		}
		ImGui::SetNextItemWidth(140.0f * dpi);
		ImGui::SliderFloat("Difference gain", &m_ReferenceGain, 1.0f, 16.0f, "%.1f");
		if (ImGui::MenuItem("Clear"))
		{
			m_Reference = nullptr;
			m_ReferencePath.clear();
			m_ReferenceView = ReferenceView::Off;
		}
		ImGui::EndDisabled();
		ImGui::EndPopup();
	}

	bool SceneViewport::DrawReferenceSplit(const ImVec2 &imageMin, const ImVec2 &imageSize)
	{
		if (!m_Reference || m_ReferenceView != ReferenceView::Split || imageSize.x <= 0.0f)
			return false;

		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float x = imageMin.x + m_ReferenceSplit * imageSize.x;
		const float halfWidth = 4.0f * dpi;
		ImGui::SetCursorScreenPos(ImVec2(x - halfWidth, imageMin.y));
		ImGui::InvisibleButton("##ReferenceSplit", ImVec2(halfWidth * 2.0f, imageSize.y));
		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();
		if (hovered || active)
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		if (active)
			m_ReferenceSplit = std::clamp((ImGui::GetIO().MousePos.x - imageMin.x) / imageSize.x, 0.0f, 1.0f);

		// A grip in the middle of the line (the renderer draws the line itself into the image)
		const ImVec2 grip(x, imageMin.y + imageSize.y * 0.5f);
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddCircleFilled(grip, 7.0f * dpi, IM_COL32(255, 255, 255, hovered || active ? 255 : 200));
		return hovered || active;
	}

	void SceneViewport::UpdateReferenceOverlay() const
	{
		Renderer::ReferenceOverlayMode mode = Renderer::ReferenceOverlayMode::Off;
		switch (m_Reference ? m_ReferenceView : ReferenceView::Off)
		{
			case ReferenceView::Split:		mode = Renderer::ReferenceOverlayMode::Split; break;
			case ReferenceView::Difference:	mode = Renderer::ReferenceOverlayMode::Difference; break;
			case ReferenceView::Flicker:	mode = std::fmod(ImGui::GetTime(), 1.0) < 0.5 ? Renderer::ReferenceOverlayMode::Reference : Renderer::ReferenceOverlayMode::Off; break;
			default: break;
		}
		Renderer::SetReferenceOverlay(m_Reference, mode, m_ReferenceSplit, m_ReferenceGain);
	}

	void SceneViewport::UpdateRenderSize(const uint32_t width, const uint32_t height)
	{
		if (width != m_PendingWidth || height != m_PendingHeight)
		{
			m_PendingWidth = width;
			m_PendingHeight = height;
			m_StableFrames = 0;
			return;
		}

		if (m_StableFrames < RESIZE_SETTLE_FRAMES)
		{
			++m_StableFrames;
			return;
		}

		if (width == m_AppliedWidth && height == m_AppliedHeight)
			return;

		if (width < MIN_RENDER_SIZE || height < MIN_RENDER_SIZE)
			return;

		// Safe from the UI: the render thread is idle while the main thread builds the frame.
		// SetRendererResolution waits for the GPU before recreating, which also covers the output targets.
		Renderer::SetViewport(static_cast<float>(width), static_cast<float>(height));
		Renderer::SetRendererResolution(width, height);
		Renderer::SetOutputResolution(width, height);

		m_AppliedWidth = width;
		m_AppliedHeight = height;
		EDITOR_INFO_TAG("Viewport", "Render resolution set to {} x {}", width, height);
	}

}

// -------------------------------------------------------
