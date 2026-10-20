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
 * editor_tools.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "editor_tools.h"
#include "airport_edit.h"
#include "editor_airports.h"
#include "editor_camera.h"
#include "editor_context.h"
#include "editor_history.h"
#include "editor_project.h"
#include "editor_scene.h"
#include "editor_selection.h"
#include "editor_commands.h"
#include "editor_theme.h"
#include "facade_tool.h"
#include "node_tool.h"
#include "tools_selftest.h"
#include "transform_tool.h"
#include "viewport_menus.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <unordered_set>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/scene_picking.h>
#include <SceneryEditorX/scene/camera.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorTools
{
	using EditorSelection::Mode;
	using EditorSelection::Target;
	using EditorSelection::TargetType;

	namespace
	{
		constexpr const char *LOG_TAG = "Tools";
		constexpr float MARQUEE_THRESHOLD = 4.0f;	// Pixels the mouse moves before a press becomes a box selection
		constexpr int CLICK_RADIUS = 4;				// A click hits what is within this many pixels (thin lines, lights)
		constexpr int HOVER_RADIUS = 2;
		constexpr float MENU_CLICK_DRAG = 5.0f;		// A right press that moves less than this many pixels...
		constexpr double MENU_CLICK_SECONDS = 0.35;	// ...and lets go within this long opens the quad menu (else it looked around)

		/* @brief A pick region requested from the GPU, waiting for its result. */
		struct PendingPick
		{
			uint64_t ticket = 0;
			Mode mode = Mode::Replace;
			int centerX = 0;			// Click / hover: the cursor's pixel in the region
			int centerY = 0;
		};

		struct State
		{
			Tool tool = Tool::Select;

			// Press / marquee gesture (left button on the image, no tool handle under it)
			bool pressed = false;
			bool marquee = false;
			ImVec2 pressMouse;
			Mode pressMode = Mode::Replace;
			bool toolOwnsMouse = false;

			PendingPick click;
			PendingPick box;
			PendingPick hover;
			ImVec2 lastHoverMouse = ImVec2(-1.0f, -1.0f);
			uint64_t lastHoverStructure = 0;

			// Right-click quad menu: the press, then the pick of what was under it
			bool rightPressed = false;
			ImVec2 rightPressMouse;
			double rightPressTime = 0.0;
			PendingPick menu;
			ImVec2 menuMouse;
			ViewportView menuView;
			ViewportView lastView;				// The viewport as drawn last frame
		};

		State &S()
		{
			static State state;
			return state;
		}

		Mode ModeFromKeys(const ImGuiIO &io)
		{
			if (io.KeyCtrl)
				return Mode::Toggle;
			if (io.KeyShift)
				return Mode::Add;
			return Mode::Replace;
		}

		/* @brief A screen position on the image as a render-target pixel. */
		void ToPixel(const ViewportView &view, const float renderWidth, const float renderHeight, const ImVec2 &screen, int &x, int &y)
		{
			x = static_cast<int>(std::floor((screen.x - view.imageMin.x) / std::max(1.0f, view.imageSize.x) * renderWidth));
			y = static_cast<int>(std::floor((screen.y - view.imageMin.y) / std::max(1.0f, view.imageSize.y) * renderHeight));
		}

		uint64_t RequestAround(const int x, const int y, const int radius)
		{
			return ScenePicking::Request(x - radius, y - radius, static_cast<uint32_t>(radius * 2 + 1), static_cast<uint32_t>(radius * 2 + 1));
		}

		/* @brief The pick id nearest the cursor's pixel in a result (0 when the region is empty). */
		uint32_t NearestId(const ScenePicking::Result &result, const int centerX, const int centerY)
		{
			uint32_t best = 0;
			int bestDistance = 1 << 30;
			for (uint32_t row = 0; row < result.height; ++row)
			{
				for (uint32_t column = 0; column < result.width; ++column)
				{
					const uint32_t id = result.ids[static_cast<size_t>(row) * result.width + column];
					if (id == 0)
						continue;
					const int dx = static_cast<int>(result.x + column) - centerX;
					const int dy = static_cast<int>(result.y + row) - centerY;
					const int distance = dx * dx + dy * dy;
					if (distance < bestDistance)
					{
						bestDistance = distance;
						best = id;
					}
				}
			}
			return best;
		}

		std::optional<Target> TargetForPickId(const uint32_t pickId)
		{
			entt::registry *registry = EditorScene::GetRegistry();
			if (!registry || pickId == 0)
				return std::nullopt;
			const entt::entity handle = ScenePicking::Resolve(*registry, pickId);
			if (handle == entt::null)
				return std::nullopt;
			return EditorSelection::FromPick(EditorScene::GetID(handle));
		}

		void PollResults(EditorContext *context)
		{
			State &s = S();
			ScenePicking::Result result;

			if (s.click.ticket != 0 && ScenePicking::TryGetResult(s.click.ticket, result))
			{
				s.click.ticket = 0;
				const std::optional<Target> target = TargetForPickId(NearestId(result, s.click.centerX, s.click.centerY));
				if (target)
					EditorSelection::Select(context, *target, s.click.mode);
				else if (s.click.mode == Mode::Replace)
				{
					EditorSelection::Clear(context);	// Clicking nothing deselects
					NodeTool::ClearNodeSelection();
				}
			}

			if (s.box.ticket != 0 && ScenePicking::TryGetResult(s.box.ticket, result))
			{
				s.box.ticket = 0;
				// Each distinct id once, then each target once
				std::vector<uint32_t> ids = result.ids;
				std::sort(ids.begin(), ids.end());
				ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
				std::vector<Target> targets;
				for (const uint32_t id : ids)
				{
					if (const std::optional<Target> target = TargetForPickId(id))
					{
						if (std::ranges::none_of(targets, [&target](const Target &known) { return known.SameAs(*target); }))
							targets.push_back(*target);
					}
				}
				if (!targets.empty() || s.box.mode == Mode::Replace)
				{
					const Mode mode = s.box.mode == Mode::Toggle ? Mode::Remove : s.box.mode;
					EditorSelection::Select(context, targets, mode);
				}
				EDITOR_INFO_TAG(LOG_TAG, "Box selection: {} item{}", targets.size(), targets.size() == 1 ? "" : "s");
			}

			if (s.hover.ticket != 0 && ScenePicking::TryGetResult(s.hover.ticket, result))
			{
				s.hover.ticket = 0;
				EditorSelection::SetHovered(TargetForPickId(NearestId(result, s.hover.centerX, s.hover.centerY)));
			}

			if (s.menu.ticket != 0 && ScenePicking::TryGetResult(s.menu.ticket, result))
			{
				s.menu.ticket = 0;
				// What was right-clicked becomes the selection (unless it is part of it already), then its menu opens
				const std::optional<Target> target = TargetForPickId(NearestId(result, s.menu.centerX, s.menu.centerY));
				if (target && !EditorSelection::IsSelected(*target))
					EditorSelection::Select(context, *target);
				// A selected facade's wall under the cursor is picked for the menu's wall commands
				if (const auto wall = FacadeTool::FindWallAt(s.menuView, s.menuMouse))
				{
					const std::vector<uint32_t> &picked = FacadeTool::GetSelectedWalls();
					if (std::ranges::find(picked, wall->second) == picked.end())
						FacadeTool::SelectWalls(wall->first, { wall->second });
				}
				Point3 ground;
				const bool onGround = s.menuView.RayToPlane(s.menuMouse, 0.0, ground);
				ViewportMenus::OpenQuadMenu(context, s.menuMouse, target, onGround ? std::optional<Point3>(ground) : std::nullopt);
			}
		}

		void DrawOverlay(const ViewportView &view)
		{
			State &s = S();
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			const ImVec2 imageMax(view.imageMin.x + view.imageSize.x, view.imageMin.y + view.imageSize.y);
			drawList->PushClipRect(view.imageMin, imageMax, true);

			if (s.pressed && s.marquee)
			{
				const ImVec2 mouse = ImGui::GetIO().MousePos;
				const ImVec2 min(std::min(mouse.x, s.pressMouse.x), std::min(mouse.y, s.pressMouse.y));
				const ImVec2 max(std::max(mouse.x, s.pressMouse.x), std::max(mouse.y, s.pressMouse.y));
				drawList->AddRectFilled(min, max, IM_COL32(255, 154, 31, 32));
				drawList->AddRect(min, max, IM_COL32(255, 154, 31, 220), 0.0f, 0, 1.5f);
			}

			// The hovered target's name by the cursor (nothing while dragging or over a handle)
			const std::optional<Target> &hovered = EditorSelection::GetHovered();
			if (hovered && !s.pressed && !IsBusy() && !s.toolOwnsMouse && view.Contains(ImGui::GetIO().MousePos))
			{
				const std::string name = EditorSelection::Describe(*hovered) + (EditorSelection::IsReadOnly(*hovered) ? "  (read-only)" : "");
				const ImVec2 mouse = ImGui::GetIO().MousePos;
				const ImVec2 size = ImGui::CalcTextSize(name.c_str());
				const ImVec2 at(mouse.x + 16.0f, mouse.y + 18.0f);
				drawList->AddRectFilled(ImVec2(at.x - 4.0f, at.y - 2.0f), ImVec2(at.x + size.x + 4.0f, at.y + size.y + 2.0f), IM_COL32(20, 20, 22, 200), 3.0f);
				drawList->AddText(at, IM_COL32(230, 230, 230, 255), name.c_str());
			}

			// Help / status line, bottom left
			const std::string hint = GetHint();
			if (!hint.empty())
			{
				const float dpi = ImGui::GetStyle().FontScaleDpi;
				const ImVec2 size = ImGui::CalcTextSize(hint.c_str());
				const ImVec2 at(view.imageMin.x + 10.0f * dpi, imageMax.y - size.y - 10.0f * dpi);
				drawList->AddRectFilled(ImVec2(at.x - 6.0f * dpi, at.y - 3.0f * dpi), ImVec2(at.x + size.x + 6.0f * dpi, at.y + size.y + 3.0f * dpi), IM_COL32(20, 20, 22, 190), 2.0f * dpi);
				drawList->AddText(at, EditorTheme::ToU32(EditorTheme::Palette::TextLabel), hint.c_str());
			}
			drawList->PopClipRect();
		}

		/* @brief Selected rows (by kind, highest index first) and items, removed from the airport and project as one undo step. */
		struct RowCopy
		{
			XPlane::AirportElementKind kind = XPlane::AirportElementKind::Airport;
			uint32_t index = 0;
			AirportEdit::Snapshot row;
		};

		struct ItemCopy
		{
			ProjectFile::SceneryItem item;
			std::string parentId;
			size_t index = 0;
		};

		struct Deletion
		{
			std::vector<RowCopy> rows;		// Highest index first (the order they are erased in)
			std::vector<ItemCopy> items;	// The order they were removed in
			const XPlane::AirportDocument *airport = nullptr;
			std::string airportId;
			const ProjectFile::ProjectDocument *project = nullptr;
		};

		bool EraseRows(const Deletion &deletion)
		{
			XPlane::AirportDocument *airport = EditorAirports::GetDocument();
			if (deletion.rows.empty())
				return true;
			if (!airport || airport != deletion.airport || airport->airport.id != deletion.airportId)
				return false;
			bool ok = true;
			for (const RowCopy &row : deletion.rows)
				ok &= AirportEdit::EraseRow(airport->airport, row.kind, row.index);
			EditorAirports::MarkEdited(true);
			return ok;
		}

		bool RestoreRows(const Deletion &deletion)
		{
			XPlane::AirportDocument *airport = EditorAirports::GetDocument();
			if (deletion.rows.empty())
				return true;
			if (!airport || airport != deletion.airport || airport->airport.id != deletion.airportId)
				return false;
			bool ok = true;
			for (auto it = deletion.rows.rbegin(); it != deletion.rows.rend(); ++it)
				ok &= AirportEdit::InsertRow(airport->airport, it->kind, it->index, it->row);
			EditorAirports::MarkEdited(true);
			return ok;
		}

		bool RemoveItems(const Deletion &deletion, EditorContext *context)
		{
			if (deletion.items.empty())
				return true;
			if (EditorProjects::GetDocument() != deletion.project)
				return false;
			bool ok = true;
			for (const ItemCopy &copy : deletion.items)
				ok &= EditorProjects::RemoveItem(copy.item.id, context);
			return ok;
		}

		bool RestoreItems(const Deletion &deletion)
		{
			if (deletion.items.empty())
				return true;
			if (EditorProjects::GetDocument() != deletion.project)
				return false;
			bool ok = true;
			for (auto it = deletion.items.rbegin(); it != deletion.items.rend(); ++it)
				ok &= EditorProjects::InsertItem(it->item, it->parentId, it->index);
			return ok;
		}
	}

	// -------------------------------------------------------

	Tool GetTool()
	{
		return S().tool;
	}

	void SetTool(const Tool tool)
	{
		State &s = S();
		if (s.tool == tool || tool == Tool::Count)
			return;
		if (TransformTool::IsUsing())
			TransformTool::Cancel();
		if (NodeTool::IsDragging())
			NodeTool::Cancel();
		if (FacadeTool::IsDrawing())
			FacadeTool::Cancel();
		s.tool = tool;
	}

	const char *GetToolName(const Tool tool)
	{
		switch (tool)
		{
			case Tool::Select:	return "Select";
			case Tool::Move:	return "Move";
			case Tool::Rotate:	return "Rotate";
			case Tool::Scale:	return "Scale";
			case Tool::Nodes:	return "Nodes";
			case Tool::Facade:	return "Facade";
			default:			return "";
		}
	}

	const char *GetToolKey(const Tool tool)
	{
		switch (tool)
		{
			case Tool::Select:	return "Q";
			case Tool::Move:	return "W";
			case Tool::Rotate:	return "E";
			case Tool::Scale:	return "R";
			case Tool::Nodes:	return "V";
			case Tool::Facade:	return "B";
			default:			return "";
		}
	}

	const char *GetToolIcon(const Tool tool)
	{
		switch (tool)
		{
			case Tool::Select:	return SEDX_ICON_MOUSE_POINTER;
			case Tool::Move:	return SEDX_ICON_ARROWS;
			case Tool::Rotate:	return SEDX_ICON_REPEAT;
			case Tool::Scale:	return SEDX_ICON_EXPAND;
			case Tool::Nodes:	return SEDX_ICON_SHARE_ALT;
			case Tool::Facade:	return SEDX_ICON_BUILDING_O;
			default:			return "";
		}
	}

	const char *GetToolTooltip(const Tool tool)
	{
		switch (tool)
		{
			case Tool::Select:	return "Select (Q)\nClick: select what is under the cursor\nShift: add, Ctrl: toggle, drag: box select";
			case Tool::Move:	return "Move (W)\nDrag the gizmo's arrows or planes; objects on the ground stay on it\nCtrl: snap (inverts the snapping setting)";
			case Tool::Rotate:	return "Rotate (E)\nTurn about the vertical (X-Plane objects and rows have a heading only)\nCtrl: snap";
			case Tool::Scale:	return "Scale (R)\nScale shapes on the ground, entities on any axis\nCtrl: snap";
			case Tool::Nodes:	return "Nodes (V)\nEdit the bezier nodes of pavement, lines, boundaries, runway ends and project shapes\n"
										"Drag nodes, handles or the curve itself; double-click a curve to insert; Ctrl+click past a line's end to extend it\n"
										"Shift+C/S/Y/F: corner, smooth, symmetric, free nodes; Shift+B: edit as a B-spline; Del deletes nodes";
			case Tool::Facade:	return "Facade (B)\nDraw facades with the armed one: polygon, rectangle, circle or line (1-4)\n"
										"Click a selected facade's wall to pick it, right-click for its wall types, heights and rebuilds\n"
										"Arm a facade from the toolbar, the X-PLANE LIBRARY panel or Space > Facades";
			default:			return "";
		}
	}

	bool BeginViewport(EditorContext *context, const ViewportView &view, const bool imageHovered)
	{
		State &s = S();
		s.lastView = view;
		const ImGuiIO &io = ImGui::GetIO();
		// The tools get the mouse unless the camera has it (Alt orbits, right / middle buttons look and pan) or a box is being drawn
		const bool cameraButtons = io.KeyAlt || ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle);
		const bool allowInput = imageHovered && !s.pressed && !cameraButtons;

		bool owns = false;
		switch (s.tool)
		{
			case Tool::Move:
				owns = TransformTool::OnViewportGui(context, view, TransformTool::Operation::Translate, allowInput);
				break;
			case Tool::Rotate:
				owns = TransformTool::OnViewportGui(context, view, TransformTool::Operation::Rotate, allowInput);
				break;
			case Tool::Scale:
				owns = TransformTool::OnViewportGui(context, view, TransformTool::Operation::Scale, allowInput);
				break;
			case Tool::Nodes:
				owns = NodeTool::OnViewportGui(context, view, allowInput && !ViewportMenus::IsOpen());
				break;
			case Tool::Facade:
				owns = FacadeTool::OnViewportGui(context, view, allowInput && !ViewportMenus::IsOpen());
				break;
			default:
				break;
		}
		s.toolOwnsMouse = owns;
		return owns;
	}

	void EndViewport(EditorContext *context, const ViewportView &view, const bool canvasHovered, const float renderWidth, const float renderHeight)
	{
		State &s = S();
		const ImGuiIO &io = ImGui::GetIO();
		const ImVec2 mouse = io.MousePos;

		if (view.valid && renderWidth > 0.0f && renderHeight > 0.0f)
		{
			// A left press on the image that no tool took: a click (pick) or, once it moves, a box
			if (!s.toolOwnsMouse && canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt)
			{
				s.pressed = true;
				s.marquee = false;
				s.pressMouse = mouse;
				s.pressMode = ModeFromKeys(io);
			}

			if (s.pressed)
			{
				const float dx = mouse.x - s.pressMouse.x, dy = mouse.y - s.pressMouse.y;
				if (!s.marquee && dx * dx + dy * dy > MARQUEE_THRESHOLD * MARQUEE_THRESHOLD)
					s.marquee = true;

				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
				{
					if (s.marquee)
					{
						const ImVec2 min(std::max(view.imageMin.x, std::min(mouse.x, s.pressMouse.x)), std::max(view.imageMin.y, std::min(mouse.y, s.pressMouse.y)));
						const ImVec2 max(std::min(view.imageMin.x + view.imageSize.x, std::max(mouse.x, s.pressMouse.x)), std::min(view.imageMin.y + view.imageSize.y, std::max(mouse.y, s.pressMouse.y)));
						// The node tool boxes nodes (drawn on screen, no readback needed); everything else boxes objects
						if (!(s.tool == Tool::Nodes && NodeTool::SelectNodesInRect(min, max, s.pressMode != Mode::Replace)))
						{
							int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
							ToPixel(view, renderWidth, renderHeight, min, x0, y0);
							ToPixel(view, renderWidth, renderHeight, max, x1, y1);
							ScenePicking::Cancel(s.box.ticket);
							s.box.ticket = ScenePicking::Request(x0, y0, static_cast<uint32_t>(std::max(1, x1 - x0 + 1)), static_cast<uint32_t>(std::max(1, y1 - y0 + 1)));
							s.box.mode = s.pressMode;
						}
					}
					else
					{
						int x = 0, y = 0;
						ToPixel(view, renderWidth, renderHeight, s.pressMouse, x, y);
						ScenePicking::Cancel(s.click.ticket);
						s.click.ticket = RequestAround(x, y, CLICK_RADIUS);
						s.click.mode = s.pressMode;
						s.click.centerX = x;
						s.click.centerY = y;
					}
					s.pressed = false;
					s.marquee = false;
				}
			}

			// A quick right-click (no drag: a drag looks around) opens the quad menu for what is under the cursor, once its
			// pick comes back. While a facade is being drawn it finishes the shape instead (3ds Max style).
			const bool overImage = canvasHovered || (s.toolOwnsMouse && view.Contains(mouse));
			if (overImage && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				s.rightPressed = true;
				s.rightPressMouse = mouse;
				s.rightPressTime = ImGui::GetTime();
			}
			if (s.rightPressed && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
			{
				s.rightPressed = false;
				const float dx = mouse.x - s.rightPressMouse.x, dy = mouse.y - s.rightPressMouse.y;
				const bool quick = dx * dx + dy * dy < MENU_CLICK_DRAG * MENU_CLICK_DRAG && ImGui::GetTime() - s.rightPressTime < MENU_CLICK_SECONDS;
				if (quick && !(s.tool == Tool::Facade && FacadeTool::OnRightClick(context)))
				{
					int x = 0, y = 0;
					ToPixel(view, renderWidth, renderHeight, s.rightPressMouse, x, y);
					ScenePicking::Cancel(s.menu.ticket);
					s.menu.ticket = RequestAround(x, y, CLICK_RADIUS);
					s.menu.centerX = x;
					s.menu.centerY = y;
					s.menuMouse = s.rightPressMouse;
					s.menuView = view;
				}
			}

			// What is under the cursor (asked again when the mouse or the scene moved; one request at a time)
			const bool idle = canvasHovered && !s.pressed && !s.toolOwnsMouse && !IsBusy() && !ImGui::IsAnyMouseDown();
			if (idle)
			{
				const uint64_t structure = Scene::GetStructureVersion();
				const bool moved = std::abs(mouse.x - s.lastHoverMouse.x) > 0.5f || std::abs(mouse.y - s.lastHoverMouse.y) > 0.5f || structure != s.lastHoverStructure;
				if (moved && s.hover.ticket == 0)
				{
					int x = 0, y = 0;
					ToPixel(view, renderWidth, renderHeight, mouse, x, y);
					s.hover.ticket = RequestAround(x, y, HOVER_RADIUS);
					s.hover.centerX = x;
					s.hover.centerY = y;
					s.lastHoverMouse = mouse;
					s.lastHoverStructure = structure;
				}
			}
			else if (!canvasHovered || s.toolOwnsMouse || IsBusy())
			{
				ScenePicking::Cancel(s.hover.ticket);
				s.hover.ticket = 0;
				s.lastHoverMouse = ImVec2(-1.0f, -1.0f);
				EditorSelection::SetHovered(std::nullopt);
			}
		}

		PollResults(context);
		if (view.valid)
		{
			// Wall labels of the selected facades follow the other tools too (the facade tool draws its own)
			if (s.tool != Tool::Facade && s.tool != Tool::Nodes)
				FacadeTool::DrawLabels(view);
			DrawOverlay(view);
		}
		if (ToolsSelfTest::IsEnabled())
			ToolsSelfTest::Tick(context, view, renderWidth, renderHeight);
	}

	bool IsBusy()
	{
		const State &s = S();
		return (s.pressed && s.marquee) || TransformTool::IsUsing() || NodeTool::IsDragging();
	}

	void HandleShortcuts(EditorContext *context)
	{
		const ImGuiIO &io = ImGui::GetIO();
		if (io.WantTextInput)
			return;

		// Del and Esc wherever the keyboard isn't typing (routed globally, so a focused text field wins)
		// (an open quad menu or hotbox takes Esc itself)
		if (ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_RouteGlobal))
			DeleteSelection(context);
		if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal) && !ViewportMenus::IsOpen())
			Escape(context);

		// Tool keys only over the viewport, and not while the right button flies the camera (WASD / Q E move it then)
		const bool viewport = context && (context->viewportHovered || context->viewportFocused);
		if (!viewport || ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle) || ViewportMenus::IsOpen())
			return;

		if (io.KeyCtrl && !io.KeyAlt && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_A, false) && GetTool() == Tool::Nodes)
			NodeTool::SelectAllNodes();
		if (io.KeyCtrl && !io.KeyAlt && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_D, false))
			EditorCommands::Run("item.duplicate", context);
		if (io.KeyCtrl || io.KeyAlt || io.KeySuper)
			return;
		// Shift+letter: the node tool's node types, straighten and B-spline; the facade tool's Enter, Backspace and modes
		if (GetTool() == Tool::Nodes && NodeTool::HandleShortcuts())
			return;
		if (GetTool() == Tool::Facade && FacadeTool::HandleShortcuts(context))
			return;

		if (ImGui::IsKeyPressed(ImGuiKey_Q, false))
			SetTool(Tool::Select);
		if (ImGui::IsKeyPressed(ImGuiKey_W, false))
			SetTool(Tool::Move);
		if (ImGui::IsKeyPressed(ImGuiKey_E, false))
			SetTool(Tool::Rotate);
		if (ImGui::IsKeyPressed(ImGuiKey_R, false))
			SetTool(Tool::Scale);
		if (ImGui::IsKeyPressed(ImGuiKey_V, false))
			SetTool(Tool::Nodes);
		if (ImGui::IsKeyPressed(ImGuiKey_B, false) && !io.KeyShift)
			SetTool(Tool::Facade);
		if (ImGui::IsKeyPressed(ImGuiKey_F, false))
			FrameSelection();
		if (ImGui::IsKeyPressed(ImGuiKey_X, false))
			TransformTool::GetSettings().localSpace = !TransformTool::GetSettings().localSpace;
	}

	bool GetViewCenterOnGround(float position[3])
	{
		const ViewportView &view = S().lastView;
		Point3 ground;
		if (!view.valid || !view.RayToPlane(ImVec2(view.imageMin.x + view.imageSize.x * 0.5f, view.imageMin.y + view.imageSize.y * 0.5f), 0.0, ground))
			return false;
		position[0] = static_cast<float>(ground.x);
		position[1] = static_cast<float>(ground.y);
		position[2] = static_cast<float>(ground.z);
		return true;
	}

	void FrameSelection()
	{
		Point3 center;
		double radius = 0.0;
		if (!TransformTool::GetSelectionCenter(center, radius))
			return;

		Camera *camera = EditorCamera::Get();
		entt::registry *registry = EditorScene::GetRegistry();
		if (!camera || !registry)
			return;
		auto view = registry->view<TransformComponent, CameraComponent>();
		for (const entt::entity handle : view)
		{
			if (!view.get<CameraComponent>(handle).isPrimary)
				continue;
			TransformComponent &transform = view.get<TransformComponent>(handle);
			const CameraComponent &cameraData = view.get<CameraComponent>(handle);

			// Keep looking the way the camera looks; back off until the selection fits the narrower field of view
			const Vec3 euler = transform.GetRotationEuler();
			const double cosPitch = std::cos(static_cast<double>(euler.x));
			Point3 forward{ cosPitch * std::sin(static_cast<double>(euler.y)), std::sin(static_cast<double>(euler.x)), cosPitch * std::cos(static_cast<double>(euler.y)) };
			if (forward.y > -0.2)
			{
				// Looking at or above the horizon: tilt down so the ground selection is seen from above
				const double yaw = euler.y;
				const double pitch = -0.6;
				forward = { std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw) };
			}
			const double halfFov = std::max(0.1, static_cast<double>(cameraData.horizontalFov_Rad) * 0.5 * 0.75);
			const double distance = std::max(8.0, radius / std::tan(halfFov) * 1.15);
			const Vec3 target(static_cast<float>(center.x), static_cast<float>(center.y), static_cast<float>(center.z));
			const Vec3 eye(static_cast<float>(center.x - forward.x * distance), static_cast<float>(center.y - forward.y * distance), static_cast<float>(center.z - forward.z * distance));
			camera->LookAt(transform, eye, target);
			break;
		}
	}

	bool DeleteSelection(EditorContext *context)
	{
		if (GetTool() == Tool::Nodes && NodeTool::GetSelectedNodeCount() > 0)
			return NodeTool::DeleteSelectedNodes();
		if (TransformTool::IsUsing() || NodeTool::IsDragging())
			return false;

		const std::vector<Target> targets = EditorSelection::Get();
		if (targets.empty())
			return false;

		auto deletion = std::make_shared<Deletion>();
		deletion->airport = EditorAirports::GetDocument();
		deletion->airportId = deletion->airport ? deletion->airport->airport.id : std::string();
		deletion->project = EditorProjects::GetDocument();

		std::vector<uint64_t> entities;
		for (const Target &target : targets)
		{
			if (EditorSelection::IsReadOnly(target))
				continue;
			switch (target.type)
			{
				case TargetType::AirportElement:
				{
					if (!deletion->airport || !AirportEdit::IsMovableKind(target.kind))
						break;
					AirportEdit::Snapshot row = AirportEdit::Capture(deletion->airport->airport, target.kind, target.index);
					if (!std::holds_alternative<std::monostate>(row))
						deletion->rows.push_back({ target.kind, target.index, std::move(row) });
					break;
				}
				case TargetType::ProjectItem:
				{
					ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
					const ProjectFile::SceneryItem *item = project ? project->FindItem(target.itemId) : nullptr;
					ItemCopy copy;
					if (item && EditorProjects::FindItemLocation(target.itemId, copy.parentId, copy.index))
					{
						copy.item = *item;
						deletion->items.push_back(std::move(copy));
					}
					break;
				}
				case TargetType::Entity:
					entities.push_back(target.entity);
					break;
			}
		}

		// Erase rows from the highest index down per kind, so the indices recorded stay right
		std::sort(deletion->rows.begin(), deletion->rows.end(), [](const RowCopy &a, const RowCopy &b)
		{
			return a.kind != b.kind ? a.kind < b.kind : a.index > b.index;
		});
		// Items: later siblings first, so re-inserting in reverse order lands each at its index
		std::sort(deletion->items.begin(), deletion->items.end(), [](const ItemCopy &a, const ItemCopy &b)
		{
			return a.parentId != b.parentId ? a.parentId < b.parentId : a.index > b.index;
		});

		EditorSelection::Clear(context);
		const bool rowsErased = EraseRows(*deletion);
		const bool itemsRemoved = RemoveItems(*deletion, context);
		for (const uint64_t entity : entities)
			EditorScene::Destroy(entity);
		if (!entities.empty())
			EDITOR_WARN_TAG(LOG_TAG, "Deleted {} scene entit{} (not undoable: only airport rows and project items are)", entities.size(), entities.size() == 1 ? "y" : "ies");

		if (!deletion->rows.empty() || !deletion->items.empty())
		{
			EditorHistory::Command command;
			const size_t count = deletion->rows.size() + deletion->items.size();
			command.label = "Delete " + std::to_string(count) + " item" + (count == 1 ? "" : "s");
			for (const RowCopy &row : deletion->rows)
				command.bytes += AirportEdit::SizeOf(row.row);
			command.bytes += deletion->items.size() * sizeof(ItemCopy);
			command.undo = [deletion]() { return RestoreRows(*deletion) && RestoreItems(*deletion); };
			command.redo = [deletion, context]() { return EraseRows(*deletion) && RemoveItems(*deletion, context); };
			EditorHistory::Push(std::move(command));
		}
		return rowsErased || itemsRemoved || !entities.empty();
	}

	void Escape(EditorContext *context)
	{
		if (TransformTool::IsUsing())
		{
			TransformTool::Cancel();
			return;
		}
		if (NodeTool::IsDragging())
		{
			NodeTool::Cancel();
			return;
		}
		if (FacadeTool::IsDrawing())
		{
			FacadeTool::Cancel();
			return;
		}
		State &s = S();
		if (s.pressed)
		{
			s.pressed = false;
			s.marquee = false;
			return;
		}
		if (GetTool() == Tool::Nodes && NodeTool::GetSelectedNodeCount() > 0)
		{
			NodeTool::ClearNodeSelection();
			return;
		}
		EditorSelection::Clear(context);
	}

	std::string GetHint()
	{
		const State &s = S();
		if (s.pressed && s.marquee)
			return s.tool == Tool::Nodes && NodeTool::HasShapes() ? "Box: select the nodes inside" : "Box: select everything visible inside (Shift adds, Ctrl removes)";

		switch (s.tool)
		{
			case Tool::Move:
			case Tool::Rotate:
			case Tool::Scale:
			{
				if (const std::string &status = TransformTool::GetStatus(); !status.empty())
					return status;
				const TransformTool::Capabilities capabilities = TransformTool::GetCapabilities();
				if (capabilities.editable == 0)
					return capabilities.note.empty() ? std::string("Click something to select it, then drag the gizmo") : capabilities.note;
				std::string hint = std::string(GetToolName(s.tool)) + " " + std::to_string(capabilities.editable) + (capabilities.editable == 1 ? " item" : " items");
				hint += TransformTool::GetSettings().localSpace ? "  |  local axes (X)" : "  |  world axes (X)";
				hint += TransformTool::GetSettings().snap ? "  |  snapping (Ctrl: off)" : "  |  Ctrl: snap";
				if (capabilities.readOnly > 0)
					hint += "  |  " + std::to_string(capabilities.readOnly) + " read-only left out";
				return hint;
			}
			case Tool::Nodes:
				NodeTool::HasShapes();	// Follows the selection before describing it
				return NodeTool::GetHint();
			case Tool::Facade:
				return FacadeTool::GetHint();
			default:
				return EditorSelection::IsEmpty() ? std::string("Click to select  |  drag a box  |  right-click: quad menu, hold Space: hotbox  |  Alt+LMB orbit, MMB pan, RMB look (+WASD)")
												  : std::to_string(EditorSelection::Get().size()) + " selected  |  W move, E rotate, R scale, V nodes, B facades, F frame, Del delete  |  right-click: menu";
		}
	}
}

// -------------------------------------------------------
