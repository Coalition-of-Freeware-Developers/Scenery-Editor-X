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
 * facade_tool.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "facade_tool.h"
#include "edit_changes.h"
#include "editor_commands.h"
#include "editor_context.h"
#include "editor_project.h"
#include "editor_selection.h"
#include "editor_theme.h"
#include "editor_tools.h"
#include "editor_ui.h"
#include "transform_tool.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/xplane/xp_library.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX::FacadeTool
{
	using ProjectFile::ItemKind;
	using ProjectFile::SceneryItem;
	using XPlane::Point2;

	namespace
	{
		constexpr const char *LOG_TAG = "Facades";
		constexpr size_t MAX_RECENT = 12;
		constexpr float CLOSE_RADIUS = 10.0f;		// Pixels: clicking this close to the first corner closes the outline
		constexpr float WALL_PICK_RADIUS = 8.0f;	// Pixels around a wall's base that still pick it
		constexpr double MIN_WALL = 0.2;			// Meters: shorter walls aren't drawn

		const ImU32 COLOR_WALL = IM_COL32(31, 138, 214, 200);
		const ImU32 COLOR_PICKED = IM_COL32(255, 154, 31, 255);
		const ImU32 COLOR_HOVERED = IM_COL32(255, 255, 255, 230);
		const ImU32 COLOR_DRAW = IM_COL32(255, 154, 31, 230);
		const ImU32 COLOR_TICK = IM_COL32(255, 220, 160, 200);
		const ImU32 COLOR_LABEL_BACK = IM_COL32(20, 20, 22, 200);

		struct State
		{
			Settings settings;
			std::vector<std::string> recent;

			// Drawing
			bool drawing = false;
			std::vector<Point2> points;			// Polygon / line corners; rectangle: the front's ends; circle: its center
			Point2 cursor;						// Where the next point goes (snapped and constrained)
			bool cursorValid = false;
			bool closeHovered = false;			// The cursor is on the first corner (a click closes)

			// Walls picked with the tool
			std::string wallItem;
			std::vector<uint32_t> walls;
			std::optional<std::pair<std::string, uint32_t>> hovered;

			std::string notice;
			double noticeUntil = 0.0;

			std::string inspected;
			bool inspectRequested = false;
			bool openRequested = false;
		};

		State &S()
		{
			static State state;
			return state;
		}

		void SetNotice(std::string text)
		{
			State &s = S();
			s.notice = std::move(text);
			s.noticeUntil = ImGui::GetTime() + 5.0;
		}

		std::string BaseName(const std::string &path)
		{
			const size_t slash = path.find_last_of("/\\");
			std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
			if (name.size() > 4 && name.ends_with(".fac"))
				name.resize(name.size() - 4);
			return name;
		}

		std::string Meters(const double meters)
		{
			char text[32];
			snprintf(text, sizeof(text), meters < 10.0 ? "%.2f m" : "%.1f m", meters);
			return text;
		}

		const SceneryItem *FindItem(const std::string &id)
		{
			ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
			return document ? document->FindItem(id) : nullptr;
		}

		bool IsRing(const SceneryItem &item)
		{
			const XPlane::FacadeAsset *asset = GetAsset(item.resource);
			return !asset || asset->ring;
		}

		/* @brief The height the DSF carries for an item (whole meters; 0 is drawn as 3 m, as the overlay builder does). */
		double DsfHeight(const float height)
		{
			const double meters = std::floor(std::clamp(static_cast<double>(height), 0.0, 65535.0));
			return meters > 0.0 ? meters : 3.0;
		}

		/* @brief A facade's height when nothing asks for another: its only one, else the first of at least 3 m. */
		float DefaultHeight(const XPlane::FacadeAsset *asset)
		{
			if (!asset)
				return 10.0f;
			const XPlane::FacadeHeights heights = XPlane::GetFacadeHeights(*asset);
			if (!heights.choices.empty())
			{
				for (const float choice : heights.choices)
				{
					if (choice >= 3.0f)
						return choice;
				}
				return heights.choices.back();
			}
			if (heights.fixed)
				return heights.maximum > 0.0f ? heights.maximum : 3.0f;
			return std::max(heights.minimum, heights.floorHeight * 3.0f);
		}

		bool ReadShape(const SceneryItem &item, Splines::Shape &shape)
		{
			if (item.rings.empty() || item.rings[0].size() < 2)
				return false;
			shape = Splines::FromGeoRings({ item.rings[0] }, IsRing(item), EditorProjects::GetProjection());
			return !shape.chains.empty() && shape.chains[0].nodes.size() >= 2;
		}

		void WriteShape(SceneryItem &item, const Splines::Shape &shape)
		{
			std::vector<ProjectFile::GeoRing> rings = Splines::ToGeoRings(shape, EditorProjects::GetProjection());
			if (rings.empty())
				return;
			if (item.rings.empty())
				item.rings.resize(1);
			item.rings[0] = std::move(rings[0]);
		}

		/**
		 * @brief Changes items as one undo step. `edit` returns false when it changed nothing (the item is put back as it
		 * was then). Locked items are left alone.
		 */
		bool EditItems(const std::vector<std::string> &ids, const std::string &label, const std::function<bool(SceneryItem &)> &edit)
		{
			ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
			if (!document)
				return false;
			EditChanges::ChangeSet changes = EditChanges::Begin();
			for (const std::string &id : ids)
			{
				SceneryItem *item = document->FindItem(id);
				if (!item || item->locked)
					continue;
				SceneryItem before = *item;
				if (!edit(*item))
				{
					*item = std::move(before);
					continue;
				}
				changes.items.push_back({ std::move(before), *item });
			}
			if (changes.items.empty())
				return false;
			EditChanges::Publish(changes);
			EditChanges::Record(label, std::move(changes));
			return true;
		}

		/* @brief Edits each facade's outline (one undo step for all of them). */
		bool EditOutlines(const std::vector<std::string> &ids, const std::string &label, const std::function<bool(Splines::Chain &, SceneryItem &)> &edit)
		{
			return EditItems(ids, label, [&edit](SceneryItem &item)
			{
				Splines::Shape shape;
				if (item.kind != ItemKind::Facade || !ReadShape(item, shape) || !edit(shape.chains[0], item))
					return false;
				WriteShape(item, shape);
				return true;
			});
		}

		/* @brief A facade item as the tools see it: its outline, walls and the layout X-Plane builds. */
		struct FacadeView
		{
			const SceneryItem *item = nullptr;
			const XPlane::FacadeAsset *asset = nullptr;
			Splines::Shape shape;
			std::vector<FacadeEdit::WallInfo> walls;
			XPlane::FacadeLayout layout;
			std::vector<std::string> names;
			double height = 3.0;				// Where the walls end (labels, picking)
		};

		bool BuildView(const std::string &id, FacadeView &out)
		{
			out = FacadeView();
			out.item = FindItem(id);
			if (!out.item || out.item->kind != ItemKind::Facade || !ReadShape(*out.item, out.shape))
				return false;
			out.asset = GetAsset(out.item->resource);
			out.walls = FacadeEdit::GetWalls(out.shape.chains[0]);
			if (out.asset)
			{
				out.layout = XPlane::LayoutFacade(*out.asset, FacadeEdit::ToSideInputs(out.walls), out.item->pickWalls, DsfHeight(out.item->height));
				out.names = XPlane::GetFacadeWallNames(*out.asset);
				out.height = out.layout.roofHeight;
			}
			out.height = std::max(out.height, 0.3);
			return true;
		}

		std::string WallName(const FacadeView &view, const int wall)
		{
			return wall >= 0 && static_cast<size_t>(wall) < view.names.size() ? view.names[static_cast<size_t>(wall)] : "Wall " + std::to_string(wall + 1);
		}

		// -------------------------------------------------------
		// Viewport drawing

		Point3 World(const Point2 &p, const double y)
		{
			return { p.x, y, p.z };
		}

		void DrawLine(ImDrawList *draw, const ViewportView &view, const Point3 &a, const Point3 &b, const ImU32 color, const float thickness)
		{
			ImVec2 sa, sb;
			if (view.WorldToScreen(a, sa) && view.WorldToScreen(b, sb))
				draw->AddLine(sa, sb, color, thickness);
		}

		void DrawLabel(ImDrawList *draw, const ImVec2 &at, const std::string &text, const ImU32 color)
		{
			const ImVec2 size = ImGui::CalcTextSize(text.c_str());
			const ImVec2 min(at.x - size.x * 0.5f - 4.0f, at.y - size.y * 0.5f - 2.0f);
			draw->AddRectFilled(min, ImVec2(min.x + size.x + 8.0f, min.y + size.y + 4.0f), COLOR_LABEL_BACK, 3.0f);
			draw->AddText(ImVec2(min.x + 4.0f, min.y + 2.0f), color, text.c_str());
		}

		/* @brief Points along a wall of an outline (both ends; curves sampled). */
		std::vector<Point2> WallPath(const Splines::Chain &chain, const uint32_t wall)
		{
			const Splines::Bezier bezier = Splines::GetSegment(chain, wall);
			if (bezier.IsStraight())
				return { bezier.p[0], bezier.p[3] };
			std::vector<Point2> points;
			const int steps = std::clamp(static_cast<int>(Splines::GetSegmentLength(bezier) / 1.0), 4, 64);
			for (int i = 0; i <= steps; ++i)
				points.push_back(bezier.Evaluate(static_cast<double>(i) / steps));
			return points;
		}

		/* @brief The middle of a wall along its curve. */
		Point2 WallMiddle(const std::vector<Point2> &path)
		{
			double total = 0.0;
			for (size_t i = 1; i < path.size(); ++i)
				total += std::hypot(path[i].x - path[i - 1].x, path[i].z - path[i - 1].z);
			double left = total * 0.5;
			for (size_t i = 1; i < path.size(); ++i)
			{
				const double piece = std::hypot(path[i].x - path[i - 1].x, path[i].z - path[i - 1].z);
				if (piece >= left && piece > 1e-9)
				{
					const double t = left / piece;
					return { path[i - 1].x + (path[i].x - path[i - 1].x) * t, path[i - 1].z + (path[i].z - path[i - 1].z) * t };
				}
				left -= piece;
			}
			return path.empty() ? Point2{} : path.back();
		}

		/* @brief Distance from a point to a screen segment, pixels. */
		float SegmentDistance(const ImVec2 &p, const ImVec2 &a, const ImVec2 &b)
		{
			const float dx = b.x - a.x, dy = b.y - a.y;
			const float length2 = dx * dx + dy * dy;
			const float t = length2 > 1e-6f ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / length2, 0.0f, 1.0f) : 0.0f;
			const float x = a.x + dx * t - p.x, y = a.y + dy * t - p.y;
			return std::sqrt(x * x + y * y);
		}

		bool InsideQuad(const ImVec2 &p, const ImVec2 quad[4])
		{
			// Same side of every edge (either winding)
			int positive = 0, negative = 0;
			for (int i = 0; i < 4; ++i)
			{
				const ImVec2 &a = quad[i], &b = quad[(i + 1) % 4];
				const float cross = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
				positive += cross > 0.0f;
				negative += cross < 0.0f;
			}
			return positive == 0 || negative == 0;
		}

		// -------------------------------------------------------
		// Drawing new facades

		Point2 SnapToGrid(Point2 point)
		{
			const TransformTool::Settings &settings = TransformTool::GetSettings();
			const bool snap = settings.snap != ImGui::GetIO().KeyCtrl;
			if (snap && settings.snapMove > 0.001f)
			{
				const double step = settings.snapMove;
				point = { std::round(point.x / step) * step, std::round(point.z / step) * step };
			}
			return point;
		}

		/* @brief Lengths the armed facade's wall type fills without stretching (cached per facade, wall and height). */
		const std::vector<double> &NaturalLengths()
		{
			static std::string key;
			static std::vector<double> lengths;
			const Settings &settings = S().settings;
			const XPlane::FacadeAsset *asset = GetAsset(settings.resource);
			const std::string wanted = settings.resource + "|" + std::to_string(settings.wallType) + "|" + std::to_string(settings.height) + "|" +
				std::to_string(XPlane::LibraryManager::GetGeneration());
			if (wanted != key)
			{
				key = wanted;
				lengths.clear();
				if (asset)
				{
					const float height = settings.height > 0.0f ? settings.height : DefaultHeight(asset);
					lengths = XPlane::GetFacadeNaturalLengths(*asset, XPlane::ChooseFacadeFloor(*asset, height), settings.wallType, 400.0, 400);
				}
			}
			return lengths;
		}

		/* @brief A segment from `from` toward `to`, its length fitted to the facade's segments when "Fit lengths" is on. */
		Point2 FitLength(const Point2 &from, const Point2 &to)
		{
			if (!S().settings.fitLengths)
				return to;
			const std::vector<double> &natural = NaturalLengths();
			const double dx = to.x - from.x, dz = to.z - from.z, length = std::hypot(dx, dz);
			if (natural.empty() || length < 1e-6)
				return to;
			const double fitted = XPlane::SnapFacadeLength(natural, length);
			return { from.x + dx / length * fitted, from.z + dz / length * fitted };
		}

		/* @brief Shift: the new wall turns by whole 45 degree steps from the previous one (from east for the first). */
		Point2 Constrain(const Point2 &from, const Point2 &to, const Point2 *before)
		{
			if (!ImGui::GetIO().KeyShift)
				return to;
			const double dx = to.x - from.x, dz = to.z - from.z, length = std::hypot(dx, dz);
			if (length < 1e-6)
				return to;
			const double base = before ? std::atan2(from.z - before->z, from.x - before->x) : 0.0;
			const double step = 3.14159265358979323846 / 4.0;
			const double angle = base + std::round((std::atan2(dz, dx) - base) / step) * step;
			return { from.x + std::cos(angle) * length, from.z + std::sin(angle) * length };
		}

		/* @brief Signed depth of a point from the line a -> b: positive to the left (the inside of a counter-clockwise footprint). */
		double DepthFrom(const Point2 &a, const Point2 &b, const Point2 &p)
		{
			const double dx = b.x - a.x, dz = b.z - a.z, length = std::hypot(dx, dz);
			if (length < 1e-9)
				return 0.0;
			// Left of travel is (dz, -dx) / length
			return ((p.x - a.x) * dz - (p.z - a.z) * dx) / length;
		}

		/* @brief The footprint being drawn, the cursor included (empty while it can't make one). */
		std::optional<Splines::Chain> PreviewChain(const State &s)
		{
			const Settings &settings = s.settings;
			switch (settings.mode)
			{
				case DrawMode::Polygon:
				case DrawMode::Line:
				{
					std::vector<Point2> points = s.points;
					if (s.cursorValid && !s.closeHovered)
						points.push_back(s.cursor);
					const bool closed = settings.mode == DrawMode::Polygon;
					if (points.size() < (closed ? 3u : 2u))
						return std::nullopt;
					return FacadeEdit::MakeChain(points, closed);
				}
				case DrawMode::Rectangle:
				{
					if (s.points.size() == 1 && s.cursorValid)
						return FacadeEdit::MakeChain({ s.points[0], s.cursor }, false);	// The front wall so far
					if (s.points.size() < 2 || !s.cursorValid)
						return std::nullopt;
					double depth = DepthFrom(s.points[0], s.points[1], s.cursor);
					if (ImGui::GetIO().KeyShift)
					{
						const double width = std::hypot(s.points[1].x - s.points[0].x, s.points[1].z - s.points[0].z);
						depth = depth < 0.0 ? -width : width;
					}
					else if (settings.fitLengths)
					{
						const std::vector<double> &natural = NaturalLengths();
						if (!natural.empty())
							depth = std::copysign(XPlane::SnapFacadeLength(natural, std::abs(depth)), depth);
					}
					if (std::abs(depth) < MIN_WALL)
						return std::nullopt;
					return FacadeEdit::MakeRectangle(s.points[0], s.points[1], depth);
				}
				case DrawMode::Circle:
				{
					if (s.points.empty() || !s.cursorValid)
						return std::nullopt;
					const double radius = std::hypot(s.cursor.x - s.points[0].x, s.cursor.z - s.points[0].z);
					if (radius < MIN_WALL)
						return std::nullopt;
					return FacadeEdit::MakeCircle(s.points[0], radius, FacadeEdit::GetCircleSides(radius, settings.circleWall));
				}
				default:
					return std::nullopt;
			}
		}

		void StopDrawing()
		{
			State &s = S();
			s.drawing = false;
			s.points.clear();
			s.closeHovered = false;
		}

		/* @brief Turns what was drawn into a facade item. */
		void Finish(EditorContext *context, const bool closeRing)
		{
			State &s = S();
			const Settings &settings = s.settings;
			std::optional<Splines::Chain> chain;
			if (settings.mode == DrawMode::Polygon || settings.mode == DrawMode::Line)
			{
				const bool closed = settings.mode == DrawMode::Polygon;
				if (s.points.size() >= (closed ? 3u : 2u))
					chain = FacadeEdit::MakeChain(s.points, closed);
				(void)closeRing;
			}
			else
				chain = PreviewChain(s);
			StopDrawing();
			if (!chain || chain->nodes.size() < 2)
				return;

			if (settings.wallType > 0)
				FacadeEdit::SetWallTypes(*chain, {}, settings.wallType);
			const XPlane::FacadeAsset *asset = GetAsset(settings.resource);
			const float height = settings.height > 0.0f ? settings.height : DefaultHeight(asset);
			const std::string id = Create(context, settings.resource, *chain, height);
			if (!id.empty())
				SetNotice("Added " + BaseName(settings.resource) + ": " + std::to_string(FacadeEdit::GetWalls(*chain).size()) + " walls. Draw another, or V to edit its nodes");
		}

		/* @brief A click on the ground while the tool is armed. */
		void Click(EditorContext *context)
		{
			State &s = S();
			if (!s.cursorValid)
				return;
			const Settings &settings = s.settings;
			const Point2 point = s.cursor;
			switch (settings.mode)
			{
				case DrawMode::Polygon:
				case DrawMode::Line:
				{
					if (s.closeHovered)
					{
						Finish(context, true);
						return;
					}
					const bool doubleClick = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
					if (doubleClick && s.points.size() >= (settings.mode == DrawMode::Polygon ? 3u : 2u))
					{
						Finish(context, false);
						return;
					}
					if (!s.points.empty() && std::hypot(point.x - s.points.back().x, point.z - s.points.back().z) < 0.05)
						return;	// The second click of a double-click lands on the corner just placed
					s.points.push_back(point);
					s.drawing = true;
					return;
				}
				case DrawMode::Rectangle:
				{
					if (s.points.size() < 2)
					{
						if (s.points.size() == 1 && std::hypot(point.x - s.points[0].x, point.z - s.points[0].z) < MIN_WALL)
							return;
						s.points.push_back(point);
						s.drawing = true;
						return;
					}
					Finish(context, true);
					return;
				}
				case DrawMode::Circle:
				{
					if (s.points.empty())
					{
						s.points.push_back(point);
						s.drawing = true;
						return;
					}
					Finish(context, true);
					return;
				}
				default:
					return;
			}
		}

		/* @brief The outline being drawn, its height box, wall lengths and segment ticks. */
		void DrawPreview(const ViewportView &view)
		{
			State &s = S();
			ImDrawList *draw = ImGui::GetWindowDrawList();
			const XPlane::FacadeAsset *asset = GetAsset(s.settings.resource);
			const double height = s.settings.height > 0.0f ? s.settings.height : DefaultHeight(asset);
			const double top = asset ? std::max(0.3, XPlane::SnapFacadeHeight(*asset, height)) : height;

			// Corners placed so far
			for (size_t i = 0; i < s.points.size(); ++i)
			{
				ImVec2 at;
				if (view.WorldToScreen(World(s.points[i], 0.0), at))
				{
					const bool first = i == 0 && s.closeHovered;
					draw->AddCircleFilled(at, first ? 7.0f : 4.0f, first ? COLOR_HOVERED : COLOR_DRAW);
				}
			}
			if (s.cursorValid)
			{
				ImVec2 at;
				if (view.WorldToScreen(World(s.cursor, 0.0), at))
					draw->AddCircle(at, 6.0f, COLOR_DRAW, 0, 1.5f);
			}

			const std::optional<Splines::Chain> chain = PreviewChain(s);
			if (!chain)
				return;
			const std::vector<FacadeEdit::WallInfo> walls = FacadeEdit::GetWalls(*chain);
			XPlane::FacadeLayout layout;
			if (asset)
			{
				Splines::Chain typed = *chain;
				if (s.settings.wallType > 0)
					FacadeEdit::SetWallTypes(typed, {}, s.settings.wallType);
				layout = XPlane::LayoutFacade(*asset, FacadeEdit::ToSideInputs(FacadeEdit::GetWalls(typed)), s.settings.wallType > 0, DsfHeight(static_cast<float>(height)));
			}
			const XPlane::FacadeAsset::Floor *floor = asset && asset->IsType2() && layout.floor >= 0 ? &asset->floors[static_cast<size_t>(layout.floor)] : nullptr;

			for (size_t w = 0; w < walls.size(); ++w)
			{
				const FacadeEdit::WallInfo &wall = walls[w];
				const Point3 a0 = World(wall.start, 0.0), b0 = World(wall.end, 0.0), a1 = World(wall.start, top), b1 = World(wall.end, top);
				DrawLine(draw, view, a0, b0, COLOR_DRAW, 2.5f);
				DrawLine(draw, view, a1, b1, COLOR_DRAW, 1.0f);
				DrawLine(draw, view, a0, a1, COLOR_DRAW, 1.0f);
				if (w + 1 == walls.size() && !chain->closed)
					DrawLine(draw, view, b0, b1, COLOR_DRAW, 1.0f);

				// Where the facade's segments meet along the wall (and how much they stretch)
				std::string label = Meters(wall.length);
				if (floor && w < layout.walls.size() && !layout.walls[w].spelling.empty())
				{
					const XPlane::FacadeWallLayout &spelled = layout.walls[w];
					double along = 0.0;
					const double dx = (wall.end.x - wall.start.x) / std::max(1e-9, wall.length), dz = (wall.end.z - wall.start.z) / std::max(1e-9, wall.length);
					for (size_t k = 0; k + 1 < spelled.spelling.size() && k < 400; ++k)
					{
						const int segment = spelled.spelling[k];
						if (segment < 0 || static_cast<size_t>(segment) >= floor->segments.size())
							continue;
						along += floor->segments[static_cast<size_t>(segment)].width * spelled.stretch;
						const Point2 at{ wall.start.x + dx * along, wall.start.z + dz * along };
						DrawLine(draw, view, World(at, 0.0), World(at, top * 0.5), COLOR_TICK, 1.0f);
					}
					if (std::abs(spelled.stretch - 1.0) > 0.005)
					{
						char stretch[32];
						snprintf(stretch, sizeof(stretch), "  x%.2f", spelled.stretch);
						label += stretch;
					}
				}
				ImVec2 at;
				if (view.WorldToScreen(World({ (wall.start.x + wall.end.x) * 0.5, (wall.start.z + wall.end.z) * 0.5 }, 0.0), at))
					DrawLabel(draw, ImVec2(at.x, at.y + 14.0f), label, IM_COL32(255, 230, 200, 255));
			}
		}

		/* @brief Lines and labels of the selected facades' walls; the hovered and picked ones stand out. */
		void DrawWalls(const ViewportView &view, const bool labels)
		{
			State &s = S();
			ImDrawList *draw = ImGui::GetWindowDrawList();
			for (const std::string &id : GetSelectedFacades())
			{
				FacadeView facade;
				if (!BuildView(id, facade))
					continue;
				const Splines::Chain &chain = facade.shape.chains[0];

				// A run of walls too short on screen for a label each (a subdivided curve) shares one: "1-49  white_curb  31.9 m"
				struct Run
				{
					size_t first = 0, last = 0;
					int type = 0;
					double length = 0.0;
					bool picked = false;
				};
				std::optional<Run> run;
				const auto flush = [&]()
				{
					if (!run)
						return;
					const size_t middle = (run->first + run->last) / 2;
					const std::vector<Point2> path = WallPath(chain, static_cast<uint32_t>(middle));
					std::string text = run->first == run->last ? std::to_string(run->first + 1) : std::to_string(run->first + 1) + "-" + std::to_string(run->last + 1);
					text += "  " + WallName(facade, run->type) + "  " + Meters(run->length);
					if (run->first == 0 && chain.closed)
						text = "FRONT  " + text;
					ImVec2 at;
					if (view.WorldToScreen(World(WallMiddle(path), facade.height), at) && view.Contains(at))
						DrawLabel(draw, ImVec2(at.x, at.y - 10.0f), text, run->picked ? COLOR_PICKED : IM_COL32(230, 230, 230, 255));
					run.reset();
				};

				for (size_t w = 0; w < facade.walls.size(); ++w)
				{
					const FacadeEdit::WallInfo &wall = facade.walls[w];
					const bool picked = s.wallItem == id && std::ranges::find(s.walls, static_cast<uint32_t>(w)) != s.walls.end();
					const bool hovered = s.hovered && s.hovered->first == id && s.hovered->second == w;
					const ImU32 color = picked ? COLOR_PICKED : hovered ? COLOR_HOVERED : COLOR_WALL;
					const std::vector<Point2> path = WallPath(chain, static_cast<uint32_t>(w));
					for (size_t i = 1; i < path.size(); ++i)
					{
						DrawLine(draw, view, World(path[i - 1], 0.02), World(path[i], 0.02), color, picked || hovered ? 3.0f : 1.5f);
						if (picked || hovered)
							DrawLine(draw, view, World(path[i - 1], facade.height), World(path[i], facade.height), color, 1.5f);
					}
					if (!labels)
						continue;

					// "3  natural_curb  12.40 m" over the wall's middle; a warning when its wall's rules don't take it
					const int type = w < facade.layout.walls.size() ? facade.layout.walls[w].wall : wall.wallType;
					ImVec2 a, b;
					const bool shortOnScreen = view.WorldToScreen(World(wall.start, facade.height), a) && view.WorldToScreen(World(wall.end, facade.height), b) &&
						std::hypot(b.x - a.x, b.y - a.y) < 70.0f;
					if (shortOnScreen && !hovered)
					{
						if (run && (run->type != type || run->last + 1 != w))
							flush();
						if (!run)
							run = Run{ w, w, type, 0.0, false };
						run->last = w;
						run->length += wall.length;
						run->picked |= picked;
						continue;
					}
					flush();
					std::string text = std::to_string(w + 1) + "  " + WallName(facade, type) + "  " + Meters(wall.length);
					if (w < facade.layout.walls.size() && !facade.layout.walls[w].fits)
						text += "  (rules don't fit)";
					if (w == 0 && chain.closed)
						text = "FRONT  " + text;
					ImVec2 at;
					if (view.WorldToScreen(World(WallMiddle(path), facade.height), at) && view.Contains(at))
						DrawLabel(draw, ImVec2(at.x, at.y - 10.0f), text, picked ? COLOR_PICKED : IM_COL32(230, 230, 230, 255));
				}
				flush();

				// The front: an arrow out of wall 0's middle
				if (!facade.walls.empty())
				{
					const FacadeEdit::WallInfo &front = facade.walls[0];
					const Point2 middle = WallMiddle(WallPath(chain, 0));
					const double radians = front.heading * 3.14159265358979323846 / 180.0;
					const double reach = std::clamp(front.length * 0.2, 0.5, 4.0);
					const Point2 tip{ middle.x + std::sin(radians) * reach, middle.z - std::cos(radians) * reach };
					ImVec2 a, b;
					if (view.WorldToScreen(World(middle, 0.05), a) && view.WorldToScreen(World(tip, 0.05), b))
					{
						draw->AddLine(a, b, COLOR_PICKED, 2.0f);
						draw->AddCircleFilled(b, 4.0f, COLOR_PICKED);
					}
				}
			}
		}

		// -------------------------------------------------------
		// Library

		struct LibraryFacades
		{
			uint64_t generation = ~0ull;
			std::map<std::string, std::vector<std::string>> folders;	// "lib/airport/ground/sidewalk_15cm" -> its .fac exports
		};

		const LibraryFacades &GetLibraryFacades()
		{
			static LibraryFacades cache;
			const uint64_t generation = XPlane::LibraryManager::GetGeneration();
			if (cache.generation == generation)
				return cache;
			cache.generation = generation;
			cache.folders.clear();
			if (const std::shared_ptr<const XPlane::LibraryIndex> index = XPlane::LibraryManager::GetIndex())
			{
				for (const XPlane::LibraryAsset &asset : index->GetAssets())
				{
					if (asset.type != XPlane::AssetType::Facade || asset.IsPrivate() || asset.IsDeprecated())
						continue;
					const size_t slash = asset.virtualPath.find_last_of('/');
					cache.folders[slash == std::string::npos ? std::string() : asset.virtualPath.substr(0, slash)].push_back(asset.virtualPath);
				}
			}
			for (auto &[folder, paths] : cache.folders)
				std::ranges::sort(paths);
			return cache;
		}

		std::string FolderOf(const std::string &path)
		{
			const size_t slash = path.find_last_of('/');
			return slash == std::string::npos ? std::string() : path.substr(0, slash);
		}

		/* @brief Selected facades, else every selected project item (for Duplicate, Hide...). */
		std::vector<std::string> SelectedItems()
		{
			std::vector<std::string> ids;
			for (const EditorSelection::Target &target : EditorSelection::Get())
			{
				if (target.type == EditorSelection::TargetType::ProjectItem)
					ids.push_back(target.itemId);
			}
			return ids;
		}

		/* @brief The walls an edit applies to on an item: the picked ones on the item they were picked on, else every wall. */
		std::vector<uint32_t> TargetWalls(const std::string &itemId)
		{
			const State &s = S();
			return s.wallItem == itemId ? s.walls : std::vector<uint32_t>();
		}

		void ArmFromLibrary(const std::string &resource)
		{
			SetResource(resource);
			EditorTools::SetTool(EditorTools::Tool::Facade);
		}
	}

	// -------------------------------------------------------

	const char *GetDrawModeName(const DrawMode mode)
	{
		switch (mode)
		{
			case DrawMode::Polygon:		return "Polygon";
			case DrawMode::Rectangle:	return "Rectangle";
			case DrawMode::Circle:		return "Circle";
			case DrawMode::Line:		return "Line";
			default:					return "";
		}
	}

	Settings &GetSettings()
	{
		return S().settings;
	}

	void SetResource(const std::string &resource)
	{
		State &s = S();
		if (resource.empty())
			return;
		if (s.settings.resource != resource)
		{
			StopDrawing();
			s.settings.resource = resource;
			s.settings.height = 0.0f;
			s.settings.wallType = 0;
			// Open runs draw as lines; closed facades keep a closed mode
			if (const XPlane::FacadeAsset *asset = GetAsset(resource))
			{
				if (!asset->ring)
					s.settings.mode = DrawMode::Line;
				else if (s.settings.mode == DrawMode::Line)
					s.settings.mode = DrawMode::Rectangle;
			}
		}
		std::erase(s.recent, resource);
		s.recent.insert(s.recent.begin(), resource);
		if (s.recent.size() > MAX_RECENT)
			s.recent.resize(MAX_RECENT);
	}

	const std::vector<std::string> &GetRecent()
	{
		return S().recent;
	}

	const XPlane::FacadeAsset *GetAsset(const std::string &resource)
	{
		return resource.empty() ? nullptr : EditorProjects::GetFacade(resource);
	}

	bool OnViewportGui(EditorContext *context, const ViewportView &view, const bool allowInput)
	{
		State &s = S();
		if (!view.valid)
			return false;
		const ImGuiIO &io = ImGui::GetIO();
		const ImVec2 mouse = io.MousePos;

		// The ground under the cursor, snapped, fitted and constrained
		Point3 hit;
		s.cursorValid = view.Contains(mouse) && view.RayToPlane(mouse, 0.0, hit);
		s.closeHovered = false;
		if (s.cursorValid)
		{
			Point2 point = SnapToGrid({ hit.x, hit.z });
			const bool chainMode = s.settings.mode == DrawMode::Polygon || s.settings.mode == DrawMode::Line;
			if (s.drawing && !s.points.empty() && (chainMode || (s.settings.mode == DrawMode::Rectangle && s.points.size() == 1)))
			{
				const Point2 *before = s.points.size() >= 2 ? &s.points[s.points.size() - 2] : nullptr;
				point = FitLength(s.points.back(), Constrain(s.points.back(), point, before));
			}
			s.cursor = point;
			if (s.drawing && s.settings.mode == DrawMode::Polygon && s.points.size() >= 3)
			{
				ImVec2 first;
				s.closeHovered = view.WorldToScreen(World(s.points.front(), 0.0), first) && std::hypot(first.x - mouse.x, first.y - mouse.y) < CLOSE_RADIUS;
			}
		}

		bool owns = false;
		s.hovered.reset();
		const bool armed = !s.settings.resource.empty() && EditorProjects::GetDocument() != nullptr;
		if (s.drawing)
		{
			owns = true;
			if (allowInput && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				Click(context);
		}
		else if (allowInput)
		{
			// A wall of a selected facade under the cursor: picking walls; else, with a facade armed, drawing
			s.hovered = FindWallAt(view, mouse);
			if (s.hovered)
			{
				owns = true;
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					const auto &[item, wall] = *s.hovered;
					if (io.KeyShift || io.KeyCtrl)
					{
						if (s.wallItem != item)
						{
							s.wallItem = item;
							s.walls.clear();
						}
						if (const auto it = std::ranges::find(s.walls, wall); it != s.walls.end())
							s.walls.erase(it);
						else
							s.walls.push_back(wall);
					}
					else
						SelectWalls(item, { wall });
				}
			}
			else if (armed && view.Contains(mouse))
			{
				owns = true;
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt)
				{
					s.walls.clear();
					Click(context);
				}
			}
		}

		DrawWalls(view, s.settings.showLabels && !s.drawing);
		if (s.drawing || (armed && s.cursorValid && !s.hovered))
			DrawPreview(view);
		return owns;
	}

	void DrawLabels(const ViewportView &view)
	{
		if (S().settings.showLabels)
			DrawWalls(view, true);
	}

	bool IsDrawing()
	{
		return S().drawing;
	}

	void Cancel()
	{
		StopDrawing();
	}

	bool HandleShortcuts(EditorContext *context)
	{
		State &s = S();
		if (s.drawing)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
			{
				Finish(context, false);
				return true;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false))
			{
				if (!s.points.empty())
					s.points.pop_back();
				if (s.points.empty())
					StopDrawing();
				return true;
			}
		}
		// 1 - 4: drawing modes
		static constexpr ImGuiKey MODE_KEYS[] = { ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4 };
		for (int m = 0; m < static_cast<int>(DrawMode::Count); ++m)
		{
			if (ImGui::IsKeyPressed(MODE_KEYS[m], false))
			{
				StopDrawing();
				s.settings.mode = static_cast<DrawMode>(m);
				return true;
			}
		}
		return false;
	}

	bool OnRightClick(EditorContext *context)
	{
		State &s = S();
		if (!s.drawing)
			return false;
		// 3ds Max style: a right-click ends a line or polygon being drawn
		if (s.settings.mode == DrawMode::Polygon || s.settings.mode == DrawMode::Line)
			Finish(context, false);
		else
			StopDrawing();
		return true;
	}

	std::string GetHint()
	{
		const State &s = S();
		if (!s.notice.empty() && ImGui::GetTime() < s.noticeUntil)
			return s.notice;
		if (!EditorProjects::GetDocument())
			return "Facades are drawn into a project: open or create one (File)";
		if (s.settings.resource.empty())
		{
			if (!GetSelectedFacades().empty())
				return "Click a wall to pick it (Shift adds), right-click for its menu  |  pick a facade in the toolbar or the library to draw";
			return "Pick a facade to draw: the toolbar, the X-PLANE LIBRARY panel (double-click a .fac) or Space > Facades";
		}
		const std::string name = BaseName(s.settings.resource);
		switch (s.settings.mode)
		{
			case DrawMode::Polygon:
				return s.drawing ? name + ": click corners  |  click the first one, double-click or Enter to close  |  Shift 45 deg  |  Backspace undo corner  |  Esc cancel"
								 : name + ": click to start a footprint  |  1-4 modes  |  Q to select";
			case DrawMode::Line:
				return s.drawing ? name + ": click to add walls  |  double-click, Enter or right-click to finish  |  Shift 45 deg  |  Backspace  |  Esc"
								 : name + ": click to start a line of walls  |  1-4 modes  |  Q to select";
			case DrawMode::Rectangle:
				return s.points.empty() ? name + ": click one end of the front wall  |  1-4 modes  |  Q to select"
					 : s.points.size() == 1 ? name + ": click the front wall's other end (Shift 45 deg)" : name + ": click to set the depth (Shift: square)";
			case DrawMode::Circle:
				return s.points.empty() ? name + ": click the center  |  1-4 modes" : name + ": click to set the radius";
			default:
				return name;
		}
	}

	void DrawToolbar(EditorContext *context, const float buttonSize)
	{
		State &s = S();
		Settings &settings = s.settings;
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const XPlane::FacadeAsset *asset = GetAsset(settings.resource);

		// Facade
		ImGui::SetNextItemWidth(170.0f * dpi);
		const std::string preview = settings.resource.empty() ? std::string("Pick a facade...") : BaseName(settings.resource);
		if (ImGui::BeginCombo("##Facade", preview.c_str(), ImGuiComboFlags_HeightLarge))
		{
			const std::vector<std::string> selected = GetSelectedFacades();
			if (!selected.empty())
			{
				if (const SceneryItem *item = FindItem(selected.front()); item && ImGui::Selectable(EditorUI::IconLabel(SEDX_ICON_CROSSHAIRS, ("Selected: " + BaseName(item->resource)).c_str()).c_str()))
					SetResource(item->resource);
				ImGui::Separator();
			}
			for (const std::string &recent : s.recent)
			{
				if (ImGui::Selectable(BaseName(recent).c_str(), recent == settings.resource))
					SetResource(recent);
				EditorUI::ItemTooltip(recent.c_str());
			}
			if (!s.recent.empty())
				ImGui::Separator();
			if (ImGui::BeginMenu(EditorUI::IconLabel(SEDX_ICON_BOOK, "Library").c_str()))
			{
				DrawLibraryMenu([](const std::string &resource) { SetResource(resource); });
				ImGui::EndMenu();
			}
			ImGui::EndCombo();
		}
		EditorUI::ItemTooltip(settings.resource.empty() ? "The facade new footprints are drawn with" : (settings.resource + (asset ? "\n" + std::string(XPlane::GetFacadeFamilyName(XPlane::GetFacadeFamily(*asset))) : std::string("\n(not found)"))).c_str());

		// Modes
		struct ModeButton
		{
			DrawMode mode;
			const char *icon;
			const char *tooltip;
		};
		static constexpr ModeButton MODES[] = {
			{ DrawMode::Polygon, SEDX_ICON_PENCIL, "Polygon (1)\nClick the corners; click the first, double-click or Enter to close" },
			{ DrawMode::Rectangle, SEDX_ICON_SQUARE_O, "Rectangle (2)\nThe front wall's two ends, then the depth (Shift: square)" },
			{ DrawMode::Circle, SEDX_ICON_CIRCLE_O, "Circle (3)\nCenter, then radius; walls about 'Circle wall' long" },
			{ DrawMode::Line, SEDX_ICON_MINUS, "Line (4)\nAn open run of walls: fences, hangars built along their length, doors" },
		};
		for (const ModeButton &button : MODES)
		{
			ImGui::SameLine(0.0f, 2.0f * dpi);
			ImGui::PushID(button.tooltip);
			if (EditorUI::StripButton(button.icon, button.tooltip, settings.mode == button.mode, buttonSize, EditorUI::StripStyle::Tinted))
			{
				StopDrawing();
				settings.mode = button.mode;
			}
			ImGui::PopID();
		}

		// Height
		ImGui::SameLine(0.0f, 8.0f * dpi);
		const float height = settings.height > 0.0f ? settings.height : DefaultHeight(asset);
		char heightText[48];
		snprintf(heightText, sizeof(heightText), "%s %.2g m", SEDX_ICON_ARROWS_V, height);
		ImGui::SetNextItemWidth(80.0f * dpi);
		if (ImGui::BeginCombo("##FacadeHeight", heightText))
		{
			if (asset)
			{
				const XPlane::FacadeHeights heights = XPlane::GetFacadeHeights(*asset);
				for (const float choice : heights.choices)
				{
					char text[32];
					snprintf(text, sizeof(text), "%.2g m", choice);
					if (ImGui::Selectable(text, std::abs(choice - height) < 0.001f))
						settings.height = choice;
				}
				if (heights.choices.empty())
				{
					ImGui::SetNextItemWidth(100.0f * dpi);
					float value = height;
					if (ImGui::DragFloat("##Custom", &value, 0.1f, std::max(0.1f, heights.minimum), std::max(heights.minimum + 1.0f, heights.maximum), "%.1f m"))
						settings.height = value;
				}
				ImGui::TextDisabled("%s", XPlane::DescribeFacadeHeights(heights).c_str());
			}
			ImGui::EndCombo();
		}
		EditorUI::ItemTooltip("Height new facades are built at (type 2 facades snap to their floors' roofs)");

		// Wall type for new facades
		if (asset && XPlane::GetFacadeWallCount(*asset) > 1)
		{
			ImGui::SameLine(0.0f, 2.0f * dpi);
			const std::vector<std::string> names = XPlane::GetFacadeWallNames(*asset);
			settings.wallType = std::clamp(settings.wallType, 0, static_cast<int>(names.size()) - 1);
			ImGui::SetNextItemWidth(120.0f * dpi);
			if (ImGui::BeginCombo("##FacadeWall", names[static_cast<size_t>(settings.wallType)].c_str()))
			{
				for (size_t i = 0; i < names.size(); ++i)
				{
					if (ImGui::Selectable((std::to_string(i + 1) + "  " + names[i]).c_str(), static_cast<int>(i) == settings.wallType))
						settings.wallType = static_cast<int>(i);
					EditorUI::ItemTooltip(XPlane::DescribeFacadeWall(*asset, static_cast<int>(i)).c_str());
				}
				ImGui::EndCombo();
			}
			EditorUI::ItemTooltip("Wall type new facades get on every wall (the first leaves the choice to X-Plane's wall rules)");
		}

		ImGui::SameLine(0.0f, 2.0f * dpi);
		if (EditorUI::StripButton(SEDX_ICON_ARROWS_H, "Fit lengths\nSnap each wall to a length the facade's segments fill without stretching\n(hangar doors, fixed-size walls, sidewalk pieces)",
				settings.fitLengths, buttonSize, EditorUI::StripStyle::Tinted))
			settings.fitLengths = !settings.fitLengths;
		ImGui::SameLine(0.0f, 2.0f * dpi);
		if (EditorUI::StripButton(SEDX_ICON_TAG, "Wall labels\nNumbers, types and lengths of the selected facades' walls", settings.showLabels, buttonSize, EditorUI::StripStyle::Tinted))
			settings.showLabels = !settings.showLabels;
		if (settings.mode == DrawMode::Circle)
		{
			ImGui::SameLine(0.0f, 4.0f * dpi);
			ImGui::SetNextItemWidth(70.0f * dpi);
			ImGui::DragFloat("##CircleWall", &settings.circleWall, 0.05f, 0.25f, 50.0f, "%.2f m");
			EditorUI::ItemTooltip("Circle wall: meters per wall of a circle (0.5 - 0.8 m suits curbs)");
		}
		(void)context;
	}

	// -------------------------------------------------------

	std::vector<std::string> GetSelectedFacades()
	{
		std::vector<std::string> ids;
		const EditorSelection::Target *active = EditorSelection::GetActive();
		for (const EditorSelection::Target &target : EditorSelection::Get())
		{
			if (target.type != EditorSelection::TargetType::ProjectItem || !IsFacade(target.itemId))
				continue;
			if (active && active->SameAs(target))
				ids.insert(ids.begin(), target.itemId);
			else
				ids.push_back(target.itemId);
		}
		return ids;
	}

	bool IsFacade(const std::string &itemId)
	{
		const SceneryItem *item = FindItem(itemId);
		return item && item->kind == ItemKind::Facade;
	}

	std::vector<FacadeEdit::WallInfo> GetWalls(const std::string &itemId)
	{
		FacadeView view;
		return BuildView(itemId, view) ? view.walls : std::vector<FacadeEdit::WallInfo>();
	}

	std::optional<std::pair<std::string, uint32_t>> FindWallAt(const ViewportView &view, const ImVec2 &screen)
	{
		std::optional<std::pair<std::string, uint32_t>> best;
		double bestDepth = 1e30;
		float bestEdge = WALL_PICK_RADIUS;
		bool bestInside = false;
		for (const std::string &id : GetSelectedFacades())
		{
			FacadeView facade;
			if (!BuildView(id, facade))
				continue;
			for (size_t w = 0; w < facade.walls.size(); ++w)
			{
				const std::vector<Point2> path = WallPath(facade.shape.chains[0], static_cast<uint32_t>(w));
				for (size_t i = 1; i < path.size(); ++i)
				{
					ImVec2 quad[4];
					double depth = 0.0;
					if (!view.WorldToScreen(World(path[i - 1], 0.0), quad[0], &depth) || !view.WorldToScreen(World(path[i], 0.0), quad[1]) ||
						!view.WorldToScreen(World(path[i], facade.height), quad[2]) || !view.WorldToScreen(World(path[i - 1], facade.height), quad[3]))
						continue;
					// On the wall's face (nearest one wins), else close to its base
					if (InsideQuad(screen, quad))
					{
						if (!bestInside || depth < bestDepth)
						{
							best = std::make_pair(id, static_cast<uint32_t>(w));
							bestDepth = depth;
							bestInside = true;
						}
						continue;
					}
					if (bestInside)
						continue;
					const float edge = std::min(SegmentDistance(screen, quad[0], quad[1]), SegmentDistance(screen, quad[3], quad[2]));
					if (edge < bestEdge)
					{
						bestEdge = edge;
						best = std::make_pair(id, static_cast<uint32_t>(w));
					}
				}
			}
		}
		return best;
	}

	const std::vector<uint32_t> &GetSelectedWalls()
	{
		return S().walls;
	}

	void SelectWalls(const std::string &itemId, const std::vector<uint32_t> &walls)
	{
		State &s = S();
		s.wallItem = itemId;
		s.walls = walls;
	}

	// -------------------------------------------------------

	bool SetWallTypes(const std::vector<std::string> &items, const std::vector<uint32_t> &walls, const int type)
	{
		// A wall type only shows when the walls are picked (the DSF carries them): turn that on with it
		const std::string label = "Set wall type" + std::string(walls.empty() ? " of every wall" : walls.size() == 1 ? "" : " of " + std::to_string(walls.size()) + " walls");
		return EditOutlines(items, label, [&walls, type](Splines::Chain &chain, SceneryItem &item)
		{
			const bool changed = FacadeEdit::SetWallTypes(chain, walls, type);
			const bool picked = !item.pickWalls;
			item.pickWalls = true;
			return changed || picked;
		});
	}

	bool SetHeight(const std::vector<std::string> &items, const float height)
	{
		return EditItems(items, "Set facade height", [height](SceneryItem &item)
		{
			if (item.kind != ItemKind::Facade || std::abs(item.height - height) < 1e-4f)
				return false;
			item.height = height;
			return true;
		});
	}

	bool SetPickWalls(const std::vector<std::string> &items, const bool pick)
	{
		return EditItems(items, pick ? "Pick facade walls" : "Let X-Plane pick facade walls", [pick](SceneryItem &item)
		{
			if (item.kind != ItemKind::Facade || item.pickWalls == pick)
				return false;
			item.pickWalls = pick;
			return true;
		});
	}

	bool SubdivideCurves(const std::vector<std::string> &items, const float pieceLength)
	{
		size_t total = 0;
		const bool done = EditOutlines(items, "Subdivide curved walls", [pieceLength, &total](Splines::Chain &chain, SceneryItem &)
		{
			size_t added = 0;
			const bool changed = FacadeEdit::SubdivideCurves(chain, pieceLength, &added);
			total += added;
			return changed;
		});
		SetNotice(done ? "Curved walls cut into " + Meters(pieceLength) + " pieces: " + std::to_string(total) + " nodes added" : "No curved walls to subdivide");
		return done;
	}

	bool SquareCorners(const std::vector<std::string> &items)
	{
		std::string why;
		const bool done = EditOutlines(items, "Square facade corners", [&why](Splines::Chain &chain, SceneryItem &) { return FacadeEdit::SquareCorners(chain, 15.0, &why); });
		SetNotice(done ? std::string("Corners squared") : why.empty() ? std::string("Nothing to square") : why);
		return done;
	}

	bool SetFrontWall(const std::string &itemId, const uint32_t wall)
	{
		const bool done = EditOutlines({ itemId }, "Set facade front", [wall](Splines::Chain &chain, SceneryItem &) { return FacadeEdit::SetFrontWall(chain, wall); });
		if (done)
			SelectWalls(itemId, { 0 });
		return done;
	}

	bool Reverse(const std::vector<std::string> &items)
	{
		const bool done = EditOutlines(items, "Reverse facade", [](Splines::Chain &chain, SceneryItem &)
		{
			Splines::ReverseChain(chain);
			return chain.nodes.size() >= 2;
		});
		S().walls.clear();
		return done;
	}

	bool Replace(const std::vector<std::string> &items, const std::string &resource)
	{
		const XPlane::FacadeAsset *target = GetAsset(resource);
		const std::vector<std::string> to = target ? XPlane::GetFacadeWallNames(*target) : std::vector<std::string>();
		const bool done = EditItems(items, "Replace facade with " + BaseName(resource), [&](SceneryItem &item)
		{
			if (item.kind != ItemKind::Facade || item.resource == resource)
				return false;
			// Walls keep their type by name (natural_curb stays natural_curb), the height snaps to the new facade's
			const XPlane::FacadeAsset *from = GetAsset(item.resource);
			const std::vector<std::string> names = from ? XPlane::GetFacadeWallNames(*from) : std::vector<std::string>();
			if (!item.rings.empty() && !to.empty())
			{
				for (ProjectFile::GeoNode &node : item.rings[0])
					node.wallType = FacadeEdit::RemapWallType(node.wallType, names, to);
			}
			if (target && target->IsType2())
				item.height = static_cast<float>(XPlane::SnapFacadeHeight(*target, item.height));
			if (item.name.empty() || item.name == BaseName(item.resource))
				item.name = BaseName(resource);
			item.resource = resource;
			item.library.clear();
			return true;
		});
		if (done)
			SetNotice("Replaced with " + BaseName(resource));
		return done;
	}

	std::vector<std::string> AddAround(EditorContext *context, const std::vector<std::string> &items, const std::string &resource, const float offset)
	{
		std::vector<std::string> added;
		const XPlane::FacadeAsset *asset = GetAsset(resource);
		for (const std::string &id : items)
		{
			const SceneryItem *item = FindItem(id);
			Splines::Shape shape;
			if (!item || !ReadShape(*item, shape))
				continue;
			Splines::Chain outline = FacadeEdit::Offset(shape.chains[0], offset, 0.65);
			if (outline.nodes.size() < 3)
				continue;
			FacadeEdit::SetWallTypes(outline, {}, 0);
			FacadeEdit::MakeCounterClockwise(outline);	// Its walls (curbs) face out, whichever way the source ran
			const std::string name = BaseName(resource) + " around " + (item->name.empty() ? std::string("facade") : item->name);
			const std::string created = Create(context, resource, outline, DefaultHeight(asset), name);
			if (!created.empty())
				added.push_back(created);
		}
		if (!added.empty())
			SetNotice("Added " + std::to_string(added.size()) + " " + BaseName(resource) + " around the selection");
		return added;
	}

	std::vector<std::string> Duplicate(EditorContext *context, const std::vector<std::string> &items)
	{
		std::vector<std::string> added;
		const XPlane::GeoProjection &projection = EditorProjects::GetProjection();
		for (const std::string &id : items)
		{
			const SceneryItem *found = FindItem(id);
			if (!found || found->IsContainer() || found->kind == ItemKind::Road || found->kind == ItemKind::RoadNode || found->kind == ItemKind::Raw)
				continue;
			SceneryItem copy = *found;
			copy.id.clear();
			const auto shift = [&projection](double &latitude, double &longitude)
			{
				const Point2 local = projection.ToLocal(latitude, longitude);
				projection.ToGeo(Point2{ local.x + 3.0, local.z + 3.0 }, latitude, longitude);
			};
			shift(copy.latitude, copy.longitude);
			for (ProjectFile::GeoRing &ring : copy.rings)
				for (ProjectFile::GeoNode &node : ring)
					shift(node.latitude, node.longitude);
			const std::string created = EditorProjects::AddItem(std::move(copy), context, "Duplicate " + (found->name.empty() ? std::string(ProjectFile::GetItemKindName(found->kind)) : found->name));
			if (!created.empty())
				added.push_back(created);
		}
		if (added.size() > 1)
		{
			std::vector<EditorSelection::Target> targets;
			for (const std::string &id : added)
				targets.push_back(EditorSelection::Target::ForItem(id));
			EditorSelection::Select(context, targets);
		}
		return added;
	}

	std::string Create(EditorContext *context, const std::string &resource, const Splines::Chain &footprint, const float height, const std::string &name)
	{
		if (!EditorProjects::GetDocument() || resource.empty() || footprint.nodes.size() < 2)
			return {};
		SceneryItem item;
		item.kind = ItemKind::Facade;
		item.resource = resource;
		item.name = name.empty() ? BaseName(resource) : name;
		item.height = height;
		Splines::Shape shape;
		shape.chains.push_back(footprint);
		item.rings = Splines::ToGeoRings(shape, EditorProjects::GetProjection());
		item.pickWalls = std::ranges::any_of(footprint.nodes, [](const Splines::Node &node) { return node.attributes.wallType != 0; });
		const std::string id = EditorProjects::AddItem(std::move(item), context, "Add facade " + (name.empty() ? BaseName(resource) : name));
		if (!id.empty())
			EDITOR_INFO_TAG(LOG_TAG, "Facade {} added: {} ({} nodes, {:.2f} m)", id, resource, footprint.nodes.size(), height);
		return id;
	}

	std::string Drop(EditorContext *context, const std::string &resource, const float position[3])
	{
		const XPlane::FacadeAsset *asset = GetAsset(resource);
		const Point2 center{ position ? position[0] : 0.0, position ? position[2] : 0.0 };
		Splines::Chain chain;
		const XPlane::FacadeFamily family = asset ? XPlane::GetFacadeFamily(*asset) : XPlane::FacadeFamily::Building;
		double width = family == XPlane::FacadeFamily::Ground ? 12.0 : 20.0;
		const double depth = family == XPlane::FacadeFamily::Ground ? 3.0 : 15.0;
		if (asset)
		{
			// Walls of a fixed size (a hangar door) come out at their natural length
			const std::vector<double> natural = XPlane::GetFacadeNaturalLengths(*asset, XPlane::ChooseFacadeFloor(*asset, DefaultHeight(asset)), 0, 200.0, 200);
			if (!natural.empty())
				width = XPlane::SnapFacadeLength(natural, width);
		}
		if (family == XPlane::FacadeFamily::Line)
			chain = FacadeEdit::MakeChain({ { center.x - width * 0.5, center.z }, { center.x + width * 0.5, center.z } }, false);
		else
			chain = FacadeEdit::MakeRectangle({ center.x - width * 0.5, center.z + depth * 0.5 }, { center.x + width * 0.5, center.z + depth * 0.5 }, depth);
		SetResource(resource);
		const std::string id = Create(context, resource, chain, DefaultHeight(asset));
		if (!id.empty())
		{
			EditorTools::SetTool(EditorTools::Tool::Facade);
			SetNotice("Dropped " + BaseName(resource) + ": V edits its nodes, click a wall to type it");
		}
		return id;
	}

	// -------------------------------------------------------

	void DrawWallTypeMenu(const std::vector<std::string> &items)
	{
		if (items.empty())
			return;
		const SceneryItem *item = FindItem(items.front());
		const XPlane::FacadeAsset *asset = item ? GetAsset(item->resource) : nullptr;
		if (!asset)
		{
			ImGui::TextDisabled("The facade can't be read");
			return;
		}
		const std::vector<std::string> names = XPlane::GetFacadeWallNames(*asset);
		const std::vector<uint32_t> walls = items.size() == 1 ? TargetWalls(items.front()) : std::vector<uint32_t>();
		ImGui::TextDisabled("%s", walls.empty() ? "Every wall" : walls.size() == 1 ? ("Wall " + std::to_string(walls.front() + 1)).c_str() : (std::to_string(walls.size()) + " picked walls").c_str());

		// The type the walls share, if they do
		const std::vector<FacadeEdit::WallInfo> info = GetWalls(items.front());
		int shared = -1;
		for (const FacadeEdit::WallInfo &wall : info)
		{
			if (!walls.empty() && std::ranges::find(walls, wall.index) == walls.end())
				continue;
			shared = shared == -1 ? wall.wallType : shared == wall.wallType ? shared : -2;
		}
		for (size_t i = 0; i < names.size(); ++i)
		{
			if (ImGui::MenuItem((std::to_string(i + 1) + "  " + names[i]).c_str(), nullptr, item->pickWalls && shared == static_cast<int>(i)))
				SetWallTypes(items, walls, static_cast<int>(i));
			EditorUI::ItemTooltip(XPlane::DescribeFacadeWall(*asset, static_cast<int>(i)).c_str());
		}
	}

	void DrawHeightMenu(const std::vector<std::string> &items)
	{
		if (items.empty())
			return;
		const SceneryItem *item = FindItem(items.front());
		const XPlane::FacadeAsset *asset = item ? GetAsset(item->resource) : nullptr;
		if (!asset)
		{
			ImGui::TextDisabled("The facade can't be read");
			return;
		}
		const XPlane::FacadeHeights heights = XPlane::GetFacadeHeights(*asset);
		std::vector<float> choices = heights.choices;
		if (choices.empty() && heights.floorHeight > 0.0f)
		{
			for (int floors = 0; floors < 12; ++floors)
			{
				const float height = heights.minimum + heights.floorHeight * static_cast<float>(floors);
				if (height > heights.maximum + 0.001f)
					break;
				choices.push_back(height);
			}
		}
		if (choices.empty())
			ImGui::TextDisabled("%s", XPlane::DescribeFacadeHeights(heights).c_str());
		for (const float choice : choices)
		{
			char text[32];
			snprintf(text, sizeof(text), "%.2g m", choice);
			if (ImGui::MenuItem(text, nullptr, std::abs(item->height - choice) < 0.01f))
				SetHeight(items, choice);
		}
	}

	void DrawReplaceMenu(const std::vector<std::string> &items)
	{
		if (items.empty())
			return;
		const SceneryItem *item = FindItem(items.front());
		if (!item)
			return;
		const LibraryFacades &library = GetLibraryFacades();
		const auto it = library.folders.find(FolderOf(item->resource));
		if (it == library.folders.end() || it->second.size() < 2)
			ImGui::TextDisabled("No other facades in %s", FolderOf(item->resource).c_str());
		else
		{
			ImGui::TextDisabled("%s", it->first.c_str());
			for (const std::string &path : it->second)
			{
				if (ImGui::MenuItem(BaseName(path).c_str(), nullptr, path == item->resource, path != item->resource))
					Replace(items, path);
			}
		}
		if (ImGui::BeginMenu("Library"))
		{
			static std::vector<std::string> s_Items;
			s_Items = items;
			DrawLibraryMenu([](const std::string &resource) { Replace(s_Items, resource); });
			ImGui::EndMenu();
		}
	}

	void DrawLibraryMenu(void (*pick)(const std::string &resource))
	{
		const LibraryFacades &library = GetLibraryFacades();
		if (library.folders.empty())
		{
			ImGui::TextDisabled(XPlane::LibraryManager::GetScanState() == XPlane::ScanState::Scanning ? "Scanning the X-Plane library..." : "No facades in the tracked libraries");
			return;
		}
		// Grouped by their first folder after lib/ ("airport", "g10"...), then the rest of their folder
		std::map<std::string, std::vector<const std::pair<const std::string, std::vector<std::string>> *>> groups;
		for (const auto &entry : library.folders)
		{
			std::string path = entry.first.starts_with("lib/") ? entry.first.substr(4) : entry.first;
			const size_t slash = path.find('/');
			groups[slash == std::string::npos ? path : path.substr(0, slash)].push_back(&entry);
		}
		for (const auto &[group, folders] : groups)
		{
			if (!ImGui::BeginMenu(group.empty() ? "(other)" : group.c_str()))
				continue;
			for (const auto *folder : folders)
			{
				std::string label = folder->first.starts_with("lib/") ? folder->first.substr(4) : folder->first;
				if (label.starts_with(group + "/"))
					label = label.substr(group.size() + 1);
				if (label == group)
					label = "(this folder)";
				if (!ImGui::BeginMenu((label + " (" + std::to_string(folder->second.size()) + ")").c_str()))
					continue;
				for (const std::string &path : folder->second)
				{
					if (ImGui::MenuItem(BaseName(path).c_str(), nullptr, path == S().settings.resource))
					{
						if (pick)
							pick(path);
						else
							ArmFromLibrary(path);
					}
					EditorUI::ItemTooltip(path.c_str());
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
	}

	void DrawProperties(EditorContext *context)
	{
		const std::vector<std::string> facades = GetSelectedFacades();
		if (facades.empty() || !EditorUI::SectionHeader("FACADE"))
			return;
		FacadeView view;
		if (!BuildView(facades.front(), view))
			return;
		const SceneryItem item = *view.item;	// A copy: an edit below replaces the document's
		const XPlane::FacadeAsset *asset = view.asset;
		const Splines::Chain &chain = view.shape.chains[0];
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		State &s = S();

		if (facades.size() > 1)
			ImGui::TextDisabled("%zu facades selected: edits apply to all of them", facades.size());

		if (EditorUI::BeginPropertyGrid("##Facade"))
		{
			EditorUI::PropertyLabel("Facade");
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::BeginCombo("##Resource", BaseName(item.resource).c_str(), ImGuiComboFlags_HeightLarge))
			{
				DrawReplaceMenu(facades);
				ImGui::EndCombo();
			}
			EditorUI::ItemTooltip((item.resource + "\nPick another facade from the same library folder (walls keep their types by name)").c_str());

			EditorUI::PropertyLabel("Kind");
			if (asset)
			{
				ImGui::TextWrapped("Type %d, %s%s", asset->IsType2() ? 2 : 1, XPlane::GetFacadeFamilyName(XPlane::GetFacadeFamily(*asset)),
					asset->scrapers.empty() ? "" : ", skyscraper");
			}
			else
				ImGui::TextColored(EditorTheme::ToStyle(EditorTheme::Palette::Error), "Not found or unreadable");

			// Height: the facade's own heights to pick from, or any value for type 1
			EditorUI::PropertyLabel("Height");
			if (asset)
			{
				const XPlane::FacadeHeights heights = XPlane::GetFacadeHeights(*asset);
				char current[48];
				snprintf(current, sizeof(current), "%.2f m", item.height);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (!heights.choices.empty())
				{
					if (ImGui::BeginCombo("##Height", current))
					{
						DrawHeightMenu(facades);
						ImGui::EndCombo();
					}
				}
				else
				{
					float value = item.height;
					ImGui::DragFloat("##Height", &value, 0.1f, 0.0f, 1000.0f, "%.2f m");
					if (ImGui::IsItemDeactivatedAfterEdit())
						SetHeight(facades, value);
				}
				EditorUI::ItemTooltip(("The facade's heights: " + XPlane::DescribeFacadeHeights(heights) + (asset->IsType2() ? "\nBuilt at " + Meters(view.layout.roofHeight) +
					" (the floor whose roof is closest)" : std::string())).c_str());
			}
			else
				ImGui::Text("%.2f m", item.height);

			EditorUI::PropertyLabel("Pick walls");
			bool pick = item.pickWalls;
			if (ImGui::Checkbox("##PickWalls", &pick))
				SetPickWalls(facades, pick);
			EditorUI::ItemTooltip("On: every wall is built with the type set here (the DSF carries them).\n"
								  "Off: X-Plane picks each wall by the facade's width and heading rules.");

			EditorUI::PropertyLabel("Footprint");
			const size_t curved = static_cast<size_t>(std::ranges::count_if(view.walls, [](const FacadeEdit::WallInfo &wall) { return wall.curved; }));
			ImGui::TextWrapped("%zu walls%s, %s%s", view.walls.size(), curved ? (", " + std::to_string(curved) + " curved").c_str() : "",
				Meters(FacadeEdit::GetPerimeter(chain)).c_str(), chain.closed ? (", " + std::to_string(static_cast<int>(std::abs(FacadeEdit::GetSignedArea(chain)))) + " m2").c_str() : " (open)");
			if (chain.closed && FacadeEdit::GetSignedArea(chain) < 0.0)
			{
				ImGui::TextColored(EditorTheme::ToStyle(EditorTheme::Palette::Warning), "Runs clockwise: its walls face in");
				EditorUI::ItemTooltip("Reverse it so the walls face out");
			}
			EditorUI::EndPropertyGrid();
		}

		// Walls: type per wall, the picked ones highlighted
		if (asset && !view.walls.empty())
		{
			const std::vector<std::string> &names = view.names;
			ImGui::SetNextItemWidth(160.0f * dpi);
			if (ImGui::BeginCombo("##AllWalls", s.wallItem == item.id && !s.walls.empty() ? "Set picked walls..." : "Set every wall..."))
			{
				DrawWallTypeMenu(facades);
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			ImGui::TextDisabled("%zu walls", view.walls.size());

			constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
			const float rows = static_cast<float>(std::min<size_t>(view.walls.size(), 10)) + 1.5f;
			if (ImGui::BeginTable("##Walls", 4, flags, ImVec2(0.0f, ImGui::GetFrameHeightWithSpacing() * rows)))
			{
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableSetupColumn("#");
				ImGui::TableSetupColumn("Length");
				ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Fit");
				ImGui::TableHeadersRow();
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(view.walls.size()));
				while (clipper.Step())
				{
					for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
					{
						const FacadeEdit::WallInfo &wall = view.walls[static_cast<size_t>(row)];
						const XPlane::FacadeWallLayout *layout = static_cast<size_t>(row) < view.layout.walls.size() ? &view.layout.walls[static_cast<size_t>(row)] : nullptr;
						ImGui::TableNextRow();
						ImGui::PushID(row);
						ImGui::TableNextColumn();
						const bool picked = s.wallItem == item.id && std::ranges::find(s.walls, wall.index) != s.walls.end();
						if (ImGui::Selectable((std::to_string(row + 1) + (row == 0 && chain.closed ? " F" : "")).c_str(), picked, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
						{
							if (ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl)
							{
								if (s.wallItem != item.id)
									SelectWalls(item.id, {});
								if (picked)
									std::erase(s.walls, wall.index);
								else
									s.walls.push_back(wall.index);
							}
							else
								SelectWalls(item.id, { wall.index });
						}
						EditorUI::ItemTooltip(row == 0 && chain.closed ? "The front wall (headings in the facade's wall rules count from it)" : "Click to pick (Shift adds)");
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(Meters(wall.length).c_str());
						ImGui::TableNextColumn();
						const int shown = layout ? layout->wall : wall.wallType;
						ImGui::SetNextItemWidth(-FLT_MIN);
						if (ImGui::BeginCombo("##Type", (std::to_string(shown + 1) + "  " + WallName(view, shown) + (item.pickWalls ? "" : " (auto)")).c_str()))
						{
							for (size_t i = 0; i < names.size(); ++i)
							{
								if (ImGui::Selectable((std::to_string(i + 1) + "  " + names[i]).c_str(), item.pickWalls && wall.wallType == static_cast<int>(i)))
									SetWallTypes({ item.id }, { wall.index }, static_cast<int>(i));
								EditorUI::ItemTooltip(XPlane::DescribeFacadeWall(*asset, static_cast<int>(i)).c_str());
							}
							ImGui::EndCombo();
						}
						ImGui::TableNextColumn();
						if (layout && !layout->fits)
							ImGui::TextColored(EditorTheme::ToStyle(EditorTheme::Palette::Warning), "%s", SEDX_ICON_EXCLAMATION_TRIANGLE);
						else if (layout && asset->IsType2())
						{
							const int percent = static_cast<int>(std::lround(layout->stretch * 100.0));
							if (std::abs(percent - 100) > 10)
								ImGui::TextColored(EditorTheme::ToStyle(EditorTheme::Palette::Warning), "%d%%", percent);
							else
								ImGui::TextDisabled("%d%%", percent);
						}
						if (layout)
							EditorUI::ItemTooltip(!layout->fits ? "The wall type's width / heading rules don't take this wall (X-Plane builds it anyway when walls are picked)"
														  : "How much the facade's segments stretch to fill the wall (100% = their own size)");
						ImGui::PopID();
					}
				}
				ImGui::EndTable();
			}
		}

		// Rebuilds
		ImGui::Spacing();
		const bool hasCurves = std::ranges::any_of(view.walls, [](const FacadeEdit::WallInfo &wall) { return wall.curved; });
		ImGui::BeginDisabled(!hasCurves);
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_SCISSORS, "Subdivide curves").c_str()))
			SubdivideCurves(facades, s.settings.subdivideLength);
		ImGui::EndDisabled();
		EditorUI::ItemTooltip("Cut curved walls into straight pieces about this long (the 3D sidewalk recipe: X-Plane stretches gaps toward\n"
							  "curve vertices, so even 0.5 - 0.8 m pieces with linear handles look right)");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70.0f * dpi);
		ImGui::DragFloat("##PieceLength", &s.settings.subdivideLength, 0.01f, 0.1f, 10.0f, "%.2f m");

		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_TH_LARGE, "Square corners").c_str()))
			SquareCorners(facades);
		EditorUI::ItemTooltip("Turn walls within 15 deg of the footprint's main axes onto them: right-angled corners");
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_EXCHANGE, "Reverse").c_str()))
			Reverse(facades);
		EditorUI::ItemTooltip("Reverse the footprint: walls face the other way (and a fence runs the other way)");
		ImGui::SameLine();
		const bool oneWall = s.wallItem == item.id && s.walls.size() == 1 && s.walls.front() != 0 && view.shape.chains[0].closed;
		ImGui::BeginDisabled(!oneWall);
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FLAG, "Make front").c_str()) && oneWall)
			SetFrontWall(item.id, s.walls.front());
		ImGui::EndDisabled();
		EditorUI::ItemTooltip("Make the picked wall the facade's front (wall 1): X-Plane measures wall headings and lays the roof from it");

		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_PLUS_SQUARE, "Add around...").c_str()))
			ImGui::OpenPopup("##AddAround");
		EditorUI::ItemTooltip("A new facade around this one's footprint: a sidewalk or curb around a building, a fence around a yard");
		if (ImGui::BeginPopup("##AddAround"))
		{
			ImGui::SetNextItemWidth(100.0f * dpi);
			ImGui::DragFloat("Offset", &s.settings.sidewalkOffset, 0.05f, -50.0f, 50.0f, "%.2f m");
			ImGui::Separator();
			static std::vector<std::string> s_Around;
			s_Around = facades;
			static EditorContext *s_Context = nullptr;
			s_Context = context;
			const LibraryFacades &library = GetLibraryFacades();
			if (const auto it = library.folders.find("lib/airport/ground/sidewalk_15cm"); it != library.folders.end())
			{
				ImGui::TextDisabled("3D sidewalks");
				for (const std::string &path : it->second)
				{
					if (ImGui::MenuItem(BaseName(path).c_str()))
						AddAround(context, facades, path, s.settings.sidewalkOffset);
				}
				ImGui::Separator();
			}
			if (ImGui::BeginMenu("Library"))
			{
				DrawLibraryMenu([](const std::string &resource) { AddAround(s_Context, s_Around, resource, S().settings.sidewalkOffset); });
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_INFO_CIRCLE, "Inspect").c_str()))
			EditorCommands::Run("facade.inspect", context);
		EditorUI::ItemTooltip("Open the facade in the FACADES inspector: floors, walls, spellings, segments");
	}

	void Inspect(const std::string &resource)
	{
		State &s = S();
		s.inspected = resource;
		s.inspectRequested = true;
		s.openRequested = true;
	}

	const std::string &GetInspected()
	{
		return S().inspected;
	}

	bool ConsumeInspectRequest()
	{
		State &s = S();
		const bool requested = s.inspectRequested;
		s.inspectRequested = false;
		return requested;
	}

	bool ConsumeOpenRequest()
	{
		State &s = S();
		const bool requested = s.openRequested;
		s.openRequested = false;
		return requested;
	}

	// -------------------------------------------------------

	void RegisterCommands()
	{
		using EditorCommands::Command;
		const auto hasFacades = [](EditorContext *) { return !GetSelectedFacades().empty(); };
		const auto hasItems = [](EditorContext *) { return !SelectedItems().empty(); };

		EditorCommands::Register({ "facade.tool", "Facade Tool", SEDX_ICON_BUILDING_O, "B", "Draw facades and edit their walls", nullptr,
			[](EditorContext *) { return EditorTools::GetTool() == EditorTools::Tool::Facade; },
			[](EditorContext *) { EditorTools::SetTool(EditorTools::Tool::Facade); } });
		for (int m = 0; m < static_cast<int>(DrawMode::Count); ++m)
		{
			const DrawMode mode = static_cast<DrawMode>(m);
			static constexpr const char *IDS[] = { "facade.draw.polygon", "facade.draw.rectangle", "facade.draw.circle", "facade.draw.line" };
			static constexpr const char *ICONS[] = { SEDX_ICON_PENCIL, SEDX_ICON_SQUARE_O, SEDX_ICON_CIRCLE_O, SEDX_ICON_MINUS };
			EditorCommands::Register({ IDS[m], std::string("Draw ") + GetDrawModeName(mode), ICONS[m], std::to_string(m + 1), "Draw a new facade with the armed facade",
				[](EditorContext *) { return EditorProjects::GetDocument() != nullptr; },
				[mode](EditorContext *) { return EditorTools::GetTool() == EditorTools::Tool::Facade && S().settings.mode == mode; },
				[mode](EditorContext *)
				{
					StopDrawing();
					S().settings.mode = mode;
					EditorTools::SetTool(EditorTools::Tool::Facade);
				} });
		}
		EditorCommands::Register({ "facade.subdivide", "Subdivide Curved Walls", SEDX_ICON_SCISSORS, "", "Cut curved walls into straight 0.5 - 0.8 m pieces (the 3D sidewalk recipe)",
			[](EditorContext *)
			{
				for (const std::string &id : GetSelectedFacades())
				{
					const std::vector<FacadeEdit::WallInfo> walls = GetWalls(id);
					if (std::ranges::any_of(walls, [](const FacadeEdit::WallInfo &wall) { return wall.curved; }))
						return true;
				}
				return false;
			},
			nullptr, [](EditorContext *) { SubdivideCurves(GetSelectedFacades(), S().settings.subdivideLength); } });
		EditorCommands::Register({ "facade.square", "Square Corners", SEDX_ICON_TH_LARGE, "", "Turn nearly square corners into right angles", hasFacades, nullptr,
			[](EditorContext *) { SquareCorners(GetSelectedFacades()); } });
		EditorCommands::Register({ "facade.reverse", "Reverse Direction", SEDX_ICON_EXCHANGE, "", "Walls face the other way", hasFacades, nullptr,
			[](EditorContext *) { Reverse(GetSelectedFacades()); } });
		EditorCommands::Register({ "facade.front", "Make Picked Wall the Front", SEDX_ICON_FLAG, "", "Wall headings and the roof are measured from the front",
			[](EditorContext *) { const State &s = S(); return s.walls.size() == 1 && s.walls.front() != 0 && IsFacade(s.wallItem); }, nullptr,
			[](EditorContext *) { const State &s = S(); SetFrontWall(s.wallItem, s.walls.front()); } });
		EditorCommands::Register({ "facade.pick_walls", "Pick Walls", SEDX_ICON_CHECK_SQUARE, "", "On: walls are built with the types set here. Off: X-Plane picks them by its rules",
			hasFacades,
			[](EditorContext *)
			{
				const std::vector<std::string> facades = GetSelectedFacades();
				return !facades.empty() && std::ranges::all_of(facades, [](const std::string &id) { const SceneryItem *item = FindItem(id); return item && item->pickWalls; });
			},
			[](EditorContext *)
			{
				const std::vector<std::string> facades = GetSelectedFacades();
				const bool all = std::ranges::all_of(facades, [](const std::string &id) { const SceneryItem *item = FindItem(id); return item && item->pickWalls; });
				SetPickWalls(facades, !all);
			} });
		EditorCommands::Register({ "facade.labels", "Wall Labels", SEDX_ICON_TAG, "", "Numbers, types and lengths of the selected facades' walls (facade tool)", nullptr,
			[](EditorContext *) { return S().settings.showLabels; },
			[](EditorContext *) { S().settings.showLabels = !S().settings.showLabels; } });
		EditorCommands::Register({ "facade.inspect", "Inspect Facade", SEDX_ICON_INFO_CIRCLE, "", "Floors, walls and their rules, spellings, segments and objects of the facade",
			[](EditorContext *) { return !GetSelectedFacades().empty() || !S().settings.resource.empty(); }, nullptr,
			[](EditorContext *)
			{
				const std::vector<std::string> facades = GetSelectedFacades();
				const SceneryItem *item = facades.empty() ? nullptr : FindItem(facades.front());
				Inspect(item ? item->resource : S().settings.resource);
			} });
		EditorCommands::Register({ "facade.use", "Draw With This Facade", SEDX_ICON_PENCIL, "", "Arm the selected facade in the facade tool", hasFacades, nullptr,
			[](EditorContext *)
			{
				if (const SceneryItem *item = FindItem(GetSelectedFacades().front()))
					ArmFromLibrary(item->resource);
			} });
		EditorCommands::Register({ "item.duplicate", "Duplicate", SEDX_ICON_FILES_O, "Ctrl+D", "Copies of the selected project items, 3 m south-east", hasItems, nullptr,
			[](EditorContext *context) { Duplicate(context, SelectedItems()); } });
		EditorCommands::Register({ "item.hide", "Hide Selection", SEDX_ICON_EYE_SLASH, "", "Hidden items aren't drawn or exported", hasItems, nullptr,
			[](EditorContext *context)
			{
				const std::vector<std::string> items = SelectedItems();
				EditorSelection::Clear(context);
				EditItems(items, "Hide " + std::to_string(items.size()) + " items", [](SceneryItem &item) { if (item.hidden) return false; item.hidden = true; return true; });
			} });
		EditorCommands::Register({ "item.unhide_all", "Show All Hidden", SEDX_ICON_EYE, "", "Show every hidden project item",
			[](EditorContext *)
			{
				const ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
				if (!document)
					return false;
				bool any = false;
				document->GetProject().ForEachItem([&any](const SceneryItem &item, int) { any |= item.hidden; return !any; });
				return any;
			},
			nullptr,
			[](EditorContext *)
			{
				ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
				std::vector<std::string> hidden;
				document->GetProject().ForEachItem([&hidden](const SceneryItem &item, int) { if (item.hidden) hidden.push_back(item.id); return true; });
				EditItems(hidden, "Show " + std::to_string(hidden.size()) + " hidden items", [](SceneryItem &item) { item.hidden = false; return true; });
			} });
		EditorCommands::Register({ "item.lock", "Lock / Unlock", SEDX_ICON_LOCK, "", "Locked items can't be moved or edited", hasItems,
			[](EditorContext *)
			{
				const std::vector<std::string> items = SelectedItems();
				return !items.empty() && std::ranges::all_of(items, [](const std::string &id) { const SceneryItem *item = FindItem(id); return item && item->locked; });
			},
			[](EditorContext *)
			{
				// EditItems leaves locked items alone, so this edits the document directly as one step
				ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
				const std::vector<std::string> items = SelectedItems();
				const bool lock = !std::ranges::all_of(items, [](const std::string &id) { const SceneryItem *item = FindItem(id); return item && item->locked; });
				EditChanges::ChangeSet changes = EditChanges::Begin();
				for (const std::string &id : items)
				{
					if (SceneryItem *item = document ? document->FindItem(id) : nullptr; item && item->locked != lock)
					{
						SceneryItem before = *item;
						item->locked = lock;
						changes.items.push_back({ std::move(before), *item });
					}
				}
				if (changes.items.empty())
					return;
				EditChanges::Publish(changes);
				EditChanges::Record(lock ? "Lock items" : "Unlock items", std::move(changes));
			} });
	}
}

// -------------------------------------------------------
