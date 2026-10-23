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
 * transform_tool.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "transform_tool.h"
#include "airport_edit.h"
#include "edit_changes.h"
#include "editor_airports.h"
#include "editor_context.h"
#include "editor_project.h"
#include "editor_scene.h"
#include "editor_selection.h"
#include "viewport_gizmo.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>
#include <SceneryEditorX/project/spline_edit.h>
#include <SceneryEditorX/renderer/gpu_scene.h>
#include <SceneryEditorX/xplane/xp_scene.h>
#include <SceneryEditorX/xplane/xp_terrain_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::TransformTool
{
	using EditorSelection::Target;
	using EditorSelection::TargetType;
	using XPlane::AirportElementKind;

	namespace
	{
		constexpr double PI = 3.14159265358979323846;
		constexpr double DEGREES = 180.0 / PI;
		constexpr uint32_t GIZMO_OWNER = 1;

		/* @brief A row-vector 4 x 4 matrix (p' = p * M) in double: Entity::GetMatrix's and the gizmo's convention. */
		struct Mat4d
		{
			double m[4][4] = { { 1.0, 0.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0, 0.0 }, { 0.0, 0.0, 0.0, 1.0 } };

			static Mat4d FromGizmo(const ViewportGizmo::Matrix &matrix)
			{
				Mat4d out;
				for (int r = 0; r < 4; ++r)
					for (int c = 0; c < 4; ++c)
						out.m[r][c] = matrix.m[r][c];
				return out;
			}

			static Mat4d FromMatrix(const xMath::Matrix &x)
			{
				Mat4d out;
				out.m[0][0] = x.m00; out.m[0][1] = x.m01; out.m[0][2] = x.m02; out.m[0][3] = x.m03;
				out.m[1][0] = x.m10; out.m[1][1] = x.m11; out.m[1][2] = x.m12; out.m[1][3] = x.m13;
				out.m[2][0] = x.m20; out.m[2][1] = x.m21; out.m[2][2] = x.m22; out.m[2][3] = x.m23;
				out.m[3][0] = x.m30; out.m[3][1] = x.m31; out.m[3][2] = x.m32; out.m[3][3] = x.m33;
				return out;
			}

			Mat4d operator*(const Mat4d &other) const
			{
				Mat4d out;
				for (int r = 0; r < 4; ++r)
				{
					for (int c = 0; c < 4; ++c)
					{
						double sum = 0.0;
						for (int k = 0; k < 4; ++k)
							sum += m[r][k] * other.m[k][c];
						out.m[r][c] = sum;
					}
				}
				return out;
			}

			[[nodiscard]] Point3 TransformPoint(const Point3 &p) const
			{
				return { p.x * m[0][0] + p.y * m[1][0] + p.z * m[2][0] + m[3][0],
						 p.x * m[0][1] + p.y * m[1][1] + p.z * m[2][1] + m[3][1],
						 p.x * m[0][2] + p.y * m[1][2] + p.z * m[2][2] + m[3][2] };
			}

			[[nodiscard]] Point3 GetTranslation() const { return { m[3][0], m[3][1], m[3][2] }; }

			/* @brief Inverse of an affine matrix (last column 0 0 0 1). */
			[[nodiscard]] Mat4d InverseAffine() const
			{
				const double a = m[0][0], b = m[0][1], c = m[0][2];
				const double d = m[1][0], e = m[1][1], f = m[1][2];
				const double g = m[2][0], h = m[2][1], i = m[2][2];
				const double det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
				Mat4d out;
				if (std::abs(det) < 1e-18)
					return out;
				const double inv = 1.0 / det;
				out.m[0][0] = (e * i - f * h) * inv; out.m[0][1] = (c * h - b * i) * inv; out.m[0][2] = (b * f - c * e) * inv;
				out.m[1][0] = (f * g - d * i) * inv; out.m[1][1] = (a * i - c * g) * inv; out.m[1][2] = (c * d - a * f) * inv;
				out.m[2][0] = (d * h - e * g) * inv; out.m[2][1] = (b * g - a * h) * inv; out.m[2][2] = (a * e - b * d) * inv;
				for (int col = 0; col < 3; ++col)
					out.m[3][col] = -(m[3][0] * out.m[0][col] + m[3][1] * out.m[1][col] + m[3][2] * out.m[2][col]);
				return out;
			}

			[[nodiscard]] bool IsIdentity(const double tolerance = 1e-7) const
			{
				for (int r = 0; r < 4; ++r)
					for (int c = 0; c < 4; ++c)
						if (std::abs(m[r][c] - (r == c ? 1.0 : 0.0)) > tolerance)
							return false;
				return true;
			}

			/* @brief Turn about the vertical (radians, counter-clockwise seen from above) of a matrix whose rotation is a yaw. */
			[[nodiscard]] double GetYaw() const { return std::atan2(m[2][0], m[0][0]); }
		};

		Mat4d Translation(const Point3 &t)
		{
			Mat4d out;
			out.m[3][0] = t.x;
			out.m[3][1] = t.y;
			out.m[3][2] = t.z;
			return out;
		}

		/* @brief A local transform from a matrix: translation, rotation (TransformComponent's quaternion) and scale. */
		void Decompose(const Mat4d &matrix, TransformComponent &out)
		{
			double rows[3][3];
			double scale[3];
			for (int r = 0; r < 3; ++r)
			{
				scale[r] = std::sqrt(matrix.m[r][0] * matrix.m[r][0] + matrix.m[r][1] * matrix.m[r][1] + matrix.m[r][2] * matrix.m[r][2]);
				const double inverse = scale[r] > 1e-12 ? 1.0 / scale[r] : 0.0;
				for (int c = 0; c < 3; ++c)
					rows[r][c] = matrix.m[r][c] * inverse;
			}
			const double det = rows[0][0] * (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1]) - rows[0][1] * (rows[1][0] * rows[2][2] - rows[1][2] * rows[2][0]) + rows[0][2] * (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]);
			if (det < 0.0)
			{
				scale[0] = -scale[0];
				for (double &value : rows[0])
					value = -value;
			}

			// The row-vector rotation is the transpose of the column-vector one the quaternion stands for: R[i][j] = rows[j][i]
			const auto R = [&rows](const int i, const int j) { return rows[j][i]; };
			double w, x, y, z;
			const double trace = R(0, 0) + R(1, 1) + R(2, 2);
			if (trace > 0.0)
			{
				const double s = std::sqrt(trace + 1.0) * 2.0;
				w = 0.25 * s;
				x = (R(2, 1) - R(1, 2)) / s;
				y = (R(0, 2) - R(2, 0)) / s;
				z = (R(1, 0) - R(0, 1)) / s;
			}
			else if (R(0, 0) > R(1, 1) && R(0, 0) > R(2, 2))
			{
				const double s = std::sqrt(1.0 + R(0, 0) - R(1, 1) - R(2, 2)) * 2.0;
				w = (R(2, 1) - R(1, 2)) / s;
				x = 0.25 * s;
				y = (R(0, 1) + R(1, 0)) / s;
				z = (R(0, 2) + R(2, 0)) / s;
			}
			else if (R(1, 1) > R(2, 2))
			{
				const double s = std::sqrt(1.0 + R(1, 1) - R(0, 0) - R(2, 2)) * 2.0;
				w = (R(0, 2) - R(2, 0)) / s;
				x = (R(0, 1) + R(1, 0)) / s;
				y = 0.25 * s;
				z = (R(1, 2) + R(2, 1)) / s;
			}
			else
			{
				const double s = std::sqrt(1.0 + R(2, 2) - R(0, 0) - R(1, 1)) * 2.0;
				w = (R(1, 0) - R(0, 1)) / s;
				x = (R(0, 2) + R(2, 0)) / s;
				y = (R(1, 2) + R(2, 1)) / s;
				z = 0.25 * s;
			}

			out.translation = Vec3(static_cast<float>(matrix.m[3][0]), static_cast<float>(matrix.m[3][1]), static_cast<float>(matrix.m[3][2]));
			out.SetRotation(Quat(static_cast<float>(w), static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
			out.scale = Vec3(static_cast<float>(scale[0]), static_cast<float>(scale[1]), static_cast<float>(scale[2]));
		}

		Mat4d WorldOf(const entt::entity handle)
		{
			Entity *entity = EditorScene::FindEntity(handle);
			return entity ? Mat4d::FromMatrix(entity->GetMatrix()) : Mat4d();
		}

		Mat4d ParentWorldOf(const entt::entity handle)
		{
			const RelationshipComponent *relationship = EditorScene::TryGet<RelationshipComponent>(handle);
			if (!relationship || static_cast<uint64_t>(relationship->pParentHandle) == 0)
				return Mat4d();
			const entt::entity parent = Scene::TryGetEntityHandle(relationship->pParentHandle);
			return parent != entt::null ? WorldOf(parent) : Mat4d();
		}

		/* @brief How an edited target is written back. */
		enum class ItemKind : uint8_t
		{
			Entity,			// A scene entity: its transform is the edit
			Object,			// An X-Plane object (free or a project item): position and heading, synced into the project
			AirportPoint,	// An apt.dat point feature: written into the row, its marker follows
			AirportRow,		// A runway, helipad, pavement, line or boundary: written into the row, the row rebuilt
			ProjectShape,	// A project item made of nodes: its rings transformed, the project's scenery redrawn
			ProjectPoint	// A project object drawn with the scenery (an .agp): its position and heading
		};

		/* @brief A selected target as the gizmo sees it (cached while the selection and documents hold still). */
		struct Descriptor
		{
			Target target;
			ItemKind kind = ItemKind::Entity;
			std::vector<uint64_t> entities;	// Moved as the preview while dragging
			bool translateY = true;
			bool rotateFree = true;
			bool scale = true;
			bool scaleY = true;
			bool staticCenter = false;		// Center from the row / item data rather than from the first entity
			Point3 center;
			double radius = 1.0;
		};

		struct Preview
		{
			uint64_t entity = 0;
			TransformComponent start;		// Local, as it was when the drag began
			Mat4d startWorld;
			Mat4d parentInverse;
		};

		struct Item
		{
			Descriptor descriptor;
			std::vector<Preview> previews;
			AirportEdit::Snapshot rowBefore;
			std::optional<ProjectFile::SceneryItem> itemBefore;
		};

		struct State
		{
			Settings settings;

			std::vector<Descriptor> descriptors;
			uint64_t descriptorKey = ~0ull;
			uint32_t readOnly = 0;
			std::string readOnlyNote;

			bool dragging = false;
			Operation operation = Operation::Translate;
			std::vector<Item> items;
			bool translateY = true;
			std::string status;
			std::string notice;		// About the last drag ("Cancelled"), shown for a moment
			double noticeUntil = 0.0;
		};

		State &S()
		{
			static State state;
			return state;
		}

		uint64_t CurrentKey()
		{
			uint64_t key = EditorSelection::GetRevision() * 0x9E3779B97F4A7C15ull;
			if (const XPlane::AirportDocument *airport = EditorAirports::GetDocument())
				key ^= airport->GetRevision() * 0xC2B2AE3D27D4EB4Full;
			if (ProjectFile::ProjectDocument *project = EditorProjects::GetDocument())
				key ^= project->GetRevision() * 0x165667B19E3779F9ull;
			key ^= Scene::GetStructureVersion() * 0x27D4EB2F165667C5ull;
			key ^= Scene::GetDestroyVersion() * 0x94D049BB133111EBull;
			return key;
		}

		void ExtendBounds(XPlane::Bounds2 &bounds, const XPlane::GeoProjection &projection, const std::vector<ProjectFile::GeoRing> &rings)
		{
			for (const ProjectFile::GeoRing &ring : rings)
				for (const ProjectFile::GeoNode &node : ring)
					bounds.Add(projection.ToLocal(node.latitude, node.longitude));
		}

		void SetCenter(Descriptor &descriptor, const XPlane::Bounds2 &bounds)
		{
			if (!bounds.valid)
				return;
			const XPlane::Point2 center = bounds.GetCenter();
			descriptor.center = { center.x, XPlane::SceneTerrain::GetHeight(center.x, center.z), center.z };
			descriptor.radius = std::max(1.0, bounds.GetRadius());
			descriptor.staticCenter = true;
		}

		std::optional<Descriptor> MakeDescriptor(const Target &target)
		{
			Descriptor descriptor;
			descriptor.target = target;

			switch (target.type)
			{
				case TargetType::Entity:
				{
					if (EditorScene::FindHandle(target.entity) == entt::null)
						return std::nullopt;
					uint64_t root = 0;
					if (XPlane::SceneObjects::FindObject(target.entity, &root) && root == target.entity)
					{
						descriptor.kind = ItemKind::Object;
						descriptor.rotateFree = false;
						descriptor.scale = false;
					}
					descriptor.entities = { target.entity };
					return descriptor;
				}

				case TargetType::AirportElement:
				{
					if (!AirportEdit::IsMovableKind(target.kind) || !EditorAirports::GetDocument())
						return std::nullopt;
					descriptor.translateY = false;
					descriptor.rotateFree = false;
					descriptor.scaleY = false;
					XPlane::SceneAirports::FindEntities(target.kind, target.index, descriptor.entities);
					if (AirportEdit::IsPointKind(target.kind))
					{
						descriptor.kind = ItemKind::AirportPoint;
						descriptor.scale = false;
					}
					else
					{
						descriptor.kind = ItemKind::AirportRow;
						descriptor.scale = target.kind != AirportElementKind::Helipad;
						XPlane::SceneAirports::FindLightObjects(target.kind, target.index, descriptor.entities);
						XPlane::Bounds2 bounds;
						for (const uint64_t id : descriptor.entities)
						{
							if (const XPlane::AirportElementComponent *element = XPlane::SceneAirports::FindElement(id))
								bounds.Add(element->bounds);
						}
						SetCenter(descriptor, bounds);
					}
					if (descriptor.entities.empty())
						return std::nullopt;
					return descriptor;
				}

				case TargetType::ProjectItem:
				{
					ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
					const ProjectFile::SceneryItem *item = document ? document->FindItem(target.itemId) : nullptr;
					const uint64_t entity = EditorProjects::GetPlacedEntity(target.itemId);
					if (!item || entity == 0 || item->locked)
						return std::nullopt;
					descriptor.entities = { entity };
					descriptor.rotateFree = false;
					descriptor.scaleY = false;
					if (item->kind == ProjectFile::ItemKind::Object)
					{
						uint64_t root = 0;
						const bool placedObject = XPlane::SceneObjects::FindObject(entity, &root) && root == entity;
						descriptor.kind = placedObject ? ItemKind::Object : ItemKind::ProjectPoint;
						descriptor.translateY = placedObject && item->elevationMode != ProjectFile::ElevationMode::Ground;
						descriptor.scale = false;
						if (!placedObject)
						{
							const XPlane::Point2 local = EditorProjects::GetProjection().ToLocal(item->latitude, item->longitude);
							descriptor.center = { local.x, XPlane::SceneTerrain::GetHeight(local.x, local.z), local.z };
							descriptor.staticCenter = true;
						}
						return descriptor;
					}
					if (!ProjectFile::HasNodes(item->kind) || item->rings.empty())
						return std::nullopt;
					descriptor.kind = ItemKind::ProjectShape;
					descriptor.translateY = false;
					XPlane::Bounds2 bounds;
					ExtendBounds(bounds, EditorProjects::GetProjection(), item->rings);
					SetCenter(descriptor, bounds);
					return descriptor;
				}
			}
			return std::nullopt;
		}

		void Refresh(State &s)
		{
			const uint64_t key = CurrentKey();
			if (key == s.descriptorKey)
				return;
			s.descriptorKey = key;
			s.descriptors.clear();
			s.readOnly = 0;
			s.readOnlyNote.clear();
			for (const Target &target : EditorSelection::Get())
			{
				std::string reason;
				if (EditorSelection::IsReadOnly(target, &reason))
				{
					++s.readOnly;
					if (s.readOnlyNote.empty())
						s.readOnlyNote = reason;
					continue;
				}
				if (std::optional<Descriptor> descriptor = MakeDescriptor(target))
					s.descriptors.push_back(std::move(*descriptor));
			}
		}

		/* @brief Where an entity-centered descriptor is now (live: other tools and panels move entities too). */
		Point3 LiveCenter(const Descriptor &descriptor, double *yaw = nullptr)
		{
			if (descriptor.staticCenter || descriptor.entities.empty())
				return descriptor.center;
			const entt::entity handle = EditorScene::FindHandle(descriptor.entities.front());
			if (handle == entt::null)
				return descriptor.center;
			const Mat4d world = WorldOf(handle);
			if (yaw)
				*yaw = world.GetYaw();
			return world.GetTranslation();
		}

		const Descriptor *ActiveDescriptor(const State &s)
		{
			const Target *active = EditorSelection::GetActive();
			if (active)
			{
				for (const Descriptor &descriptor : s.descriptors)
					if (descriptor.target.SameAs(*active))
						return &descriptor;
			}
			return s.descriptors.empty() ? nullptr : &s.descriptors.back();
		}

		/**
		 * @brief Where the gizmo sits while idle: the selection's center (or the active object's pivot), its axes turned
		 * to the active object's heading when local.
		 */
		ViewportGizmo::Frame RestFrame(const State &s, const bool local)
		{
			XPlane::Bounds2 bounds;
			Point3 sum;
			for (const Descriptor &descriptor : s.descriptors)
			{
				const Point3 center = LiveCenter(descriptor);
				bounds.Add(XPlane::Point2{ center.x, center.z });
				sum.y += center.y;
			}
			const XPlane::Point2 middle = bounds.GetCenter();
			Point3 pivot{ middle.x, sum.y / static_cast<double>(std::max<size_t>(1, s.descriptors.size())), middle.z };

			const Descriptor *active = ActiveDescriptor(s);
			double yaw = 0.0;
			if (active)
			{
				const Point3 activeCenter = LiveCenter(*active, &yaw);
				if (s.settings.pivotActive)
					pivot = activeCenter;
			}
			return ViewportGizmo::Frame::Turned(pivot, local && active ? yaw : 0.0);
		}

		void Begin(State &s, const Operation operation)
		{
			s.dragging = true;
			s.operation = operation;
			s.items.clear();
			s.translateY = true;

			XPlane::AirportDocument *airport = EditorAirports::GetDocument();
			ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
			for (const Descriptor &descriptor : s.descriptors)
			{
				Item &item = s.items.emplace_back();
				item.descriptor = descriptor;
				s.translateY &= descriptor.translateY;
				for (const uint64_t id : descriptor.entities)
				{
					const entt::entity handle = EditorScene::FindHandle(id);
					const TransformComponent *transform = EditorScene::TryGet<TransformComponent>(handle);
					if (!transform)
						continue;
					Preview &preview = item.previews.emplace_back();
					preview.entity = id;
					preview.start = *transform;
					preview.startWorld = WorldOf(handle);
					preview.parentInverse = ParentWorldOf(handle).InverseAffine();
				}
				if (airport && descriptor.target.type == TargetType::AirportElement)
					item.rowBefore = AirportEdit::Capture(airport->airport, descriptor.target.kind, descriptor.target.index);
				if (project && descriptor.target.type == TargetType::ProjectItem)
				{
					if (const ProjectFile::SceneryItem *source = project->FindItem(descriptor.target.itemId))
						item.itemBefore = *source;
				}
			}
		}

		/* @brief The drag so far as a world transform about the pivot; ground placements stay on the ground. */
		Mat4d DragDelta(const State &s, const ViewportGizmo::Matrix &drag)
		{
			Mat4d delta = Mat4d::FromGizmo(drag);
			if (!s.translateY)
				delta.m[3][1] = 0.0;
			return delta;
		}

		void EndDrag(State &s)
		{
			s.dragging = false;
			s.items.clear();
			s.status.clear();
			s.descriptorKey = ~0ull;	// Rows and items moved: centers again from the data
		}

		void ApplyPreview(State &s, const Mat4d &delta)
		{
			const bool terrain = XPlane::SceneTerrain::IsLoaded();
			for (Item &item : s.items)
			{
				// Objects and point features stand on the terrain: moved across it they keep their height above the ground
				const bool followGround = terrain && (item.descriptor.kind == ItemKind::Object || item.descriptor.kind == ItemKind::ProjectPoint || item.descriptor.kind == ItemKind::AirportPoint);
				for (const Preview &preview : item.previews)
				{
					const entt::entity handle = EditorScene::FindHandle(preview.entity);
					TransformComponent *transform = EditorScene::TryGet<TransformComponent>(handle);
					if (!transform)
						continue;
					Mat4d world = preview.startWorld * delta;
					if (followGround)
					{
						const Point3 from = preview.startWorld.GetTranslation(), to = world.GetTranslation();
						world.m[3][1] += XPlane::SceneTerrain::GetHeight(to.x, to.z) - XPlane::SceneTerrain::GetHeight(from.x, from.z);
					}
					Decompose(world * preview.parentInverse, *transform);
					GpuScene::MarkTransformDirty(handle);
				}
			}
		}

		void Restore(State &s)
		{
			for (Item &item : s.items)
			{
				for (const Preview &preview : item.previews)
				{
					const entt::entity handle = EditorScene::FindHandle(preview.entity);
					if (TransformComponent *transform = EditorScene::TryGet<TransformComponent>(handle))
					{
						*transform = preview.start;
						GpuScene::MarkTransformDirty(handle);
					}
				}
			}
		}

		std::string DescribeDrag(const State &s, const Mat4d &delta)
		{
			const size_t count = s.items.size();
			char text[160];
			switch (s.operation)
			{
				case Operation::Translate:
				{
					const Point3 t = delta.GetTranslation();
					snprintf(text, sizeof(text), "Move %zu: %.2f m  (east %.2f, north %.2f, up %.2f)", count, std::sqrt(t.x * t.x + t.y * t.y + t.z * t.z), t.x, -t.z, t.y);
					break;
				}
				case Operation::Rotate:
					snprintf(text, sizeof(text), "Rotate %zu: %.1f deg", count, delta.GetYaw() * DEGREES);
					break;
				case Operation::Scale:
				{
					const double sx = std::sqrt(delta.m[0][0] * delta.m[0][0] + delta.m[0][2] * delta.m[0][2]);
					const double sz = std::sqrt(delta.m[2][0] * delta.m[2][0] + delta.m[2][2] * delta.m[2][2]);
					snprintf(text, sizeof(text), "Scale %zu: x%.3f / x%.3f", count, sx, sz);
					break;
				}
			}
			return text;
		}

		const char *OperationVerb(const Operation operation)
		{
			switch (operation)
			{
				case Operation::Translate:	return "Move";
				case Operation::Rotate:		return "Rotate";
				case Operation::Scale:		return "Scale";
			}
			return "Transform";
		}

		void Commit(State &s, const Mat4d &delta)
		{
			if (delta.IsIdentity())
			{
				Restore(s);
				return;
			}

			// The drag as a ground-plane transform: x' = a x + b z + tx, z' = c x + d z + tz (the X-Z part of the row-vector matrix)
			Splines::Affine2 affine;
			affine.a = delta.m[0][0];
			affine.b = delta.m[2][0];
			affine.c = delta.m[0][2];
			affine.d = delta.m[2][2];
			affine.tx = delta.m[3][0];
			affine.tz = delta.m[3][2];
			const double turnDegrees = delta.GetYaw() * DEGREES;

			XPlane::AirportDocument *airport = EditorAirports::GetDocument();
			ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
			EditChanges::ChangeSet changes = EditChanges::Begin();

			for (Item &item : s.items)
			{
				const Descriptor &descriptor = item.descriptor;
				switch (descriptor.kind)
				{
					case ItemKind::Entity:
					case ItemKind::Object:
					{
						// The preview is the edit: the entities stay where the drag left them
						for (const Preview &preview : item.previews)
						{
							if (const TransformComponent *transform = EditorScene::TryGet<TransformComponent>(EditorScene::FindHandle(preview.entity)))
								changes.entities.push_back({ preview.entity, preview.start, *transform });
						}
						changes.syncProject |= descriptor.target.type == TargetType::ProjectItem || descriptor.kind == ItemKind::Object;
						break;
					}

					case ItemKind::AirportPoint:
					case ItemKind::AirportRow:
					{
						// Into the row; markers follow it, shapes are rebuilt (which replaces the preview)
						const AirportElementKind kind = descriptor.target.kind;
						const uint32_t index = descriptor.target.index;
						if (!airport || !AirportEdit::TransformRow(airport->airport, kind, index, affine, turnDegrees, XPlane::SceneAirports::GetProjection()))
							break;
						changes.rows.push_back({ kind, index, item.rowBefore, AirportEdit::Capture(airport->airport, kind, index) });
						break;
					}

					case ItemKind::ProjectShape:
					case ItemKind::ProjectPoint:
					{
						ProjectFile::SceneryItem *source = project ? project->FindItem(descriptor.target.itemId) : nullptr;
						if (!source || !item.itemBefore)
							break;
						const XPlane::GeoProjection &projection = EditorProjects::GetProjection();
						if (descriptor.kind == ItemKind::ProjectShape)
						{
							Splines::Shape shape = Splines::FromGeoRings(source->rings, true, projection);
							Splines::TransformShape(shape, affine);
							source->rings = Splines::ToGeoRings(shape, projection);
							if (source->kind == ProjectFile::ItemKind::Polygon || source->kind == ProjectFile::ItemKind::Orthophoto)
								source->textureHeading = AirportEdit::TurnHeading(source->textureHeading, turnDegrees);
						}
						else
						{
							const XPlane::Point2 moved = affine.Apply(projection.ToLocal(source->latitude, source->longitude));
							projection.ToGeo(moved, source->latitude, source->longitude);
							source->heading = AirportEdit::TurnHeading(source->heading, turnDegrees);
						}
						changes.items.push_back({ *item.itemBefore, *source });
						break;
					}
				}
			}

			EditChanges::Publish(changes);
			const size_t count = s.items.size();
			const std::string label = std::string(OperationVerb(s.operation)) + " " + (count == 1 ? EditorSelection::Describe(s.items.front().descriptor.target) : std::to_string(count) + " items");
			EditChanges::Record(label, std::move(changes));
		}
	}

	// -------------------------------------------------------

	Settings &GetSettings()
	{
		return S().settings;
	}

	Capabilities GetCapabilities()
	{
		State &s = S();
		if (!s.dragging)
			Refresh(s);
		Capabilities capabilities;
		capabilities.editable = static_cast<uint32_t>(s.descriptors.size());
		capabilities.readOnly = s.readOnly;
		capabilities.note = s.readOnlyNote;
		for (const Descriptor &descriptor : s.descriptors)
		{
			capabilities.translateY &= descriptor.translateY;
			capabilities.rotateFree &= descriptor.rotateFree;
			capabilities.scale &= descriptor.scale;
			capabilities.scaleY &= descriptor.scaleY;
		}
		if (s.descriptors.empty())
		{
			capabilities.translateY = capabilities.rotateFree = capabilities.scale = capabilities.scaleY = false;
			if (capabilities.note.empty() && !EditorSelection::IsEmpty())
				capabilities.note = "The selection can't be moved";
		}
		return capabilities;
	}

	bool OnViewportGui(EditorContext *, const ViewportView &view, const Operation operation, const bool allowInput)
	{
		State &s = S();
		if (!view.valid)
		{
			if (s.dragging)
				Cancel();
			return false;
		}

		const Capabilities capabilities = GetCapabilities();
		if (capabilities.editable == 0 && !s.dragging)
		{
			s.status.clear();
			return false;
		}

		// What the selection allows of the operation
		using ViewportGizmo::Handle;
		ViewportGizmo::Options options;
		switch (operation)
		{
			case Operation::Translate:
				options.mode = ViewportGizmo::Mode::Translate;
				options.handles = capabilities.translateY ? Handle::Everything : Handle::Ground;
				break;
			case Operation::Rotate:
				options.mode = ViewportGizmo::Mode::Rotate;
				options.handles = capabilities.rotateFree ? Handle::Axes | Handle::ViewRing : Handle::AxisY;
				break;
			case Operation::Scale:
				options.mode = ViewportGizmo::Mode::Scale;
				if (capabilities.scale)
					options.handles = capabilities.scaleY ? Handle::Everything : Handle::Ground;
				options.uniformGround = !capabilities.scaleY;
				break;
		}
		if (options.handles == 0 && !s.dragging)
		{
			s.status = operation == Operation::Scale ? "X-Plane objects and point features can't be scaled" : std::string();
			return false;
		}
		options.snap = s.settings.snap != ImGui::GetIO().KeyCtrl;
		options.snapMove = s.settings.snapMove;
		options.snapAngle = s.settings.snapAngle;
		options.snapScale = s.settings.snapScale;

		// Scale always works along the active object's axes: world axes would skew a turned object
		const ViewportGizmo::Frame frame = RestFrame(s, s.settings.localSpace || operation == Operation::Scale);
		const ViewportGizmo::Result result = ViewportGizmo::Manipulate(GIZMO_OWNER, view, frame, options, allowInput);
		ViewportGizmo::Draw(view);

		if (result.started && !s.dragging)
			Begin(s, operation);
		if (s.dragging && (result.active || result.finished))
		{
			const Mat4d delta = DragDelta(s, result.delta);
			ApplyPreview(s, delta);
			if (result.finished)
				Commit(s, delta);
			else
				s.status = DescribeDrag(s, delta);
		}
		// Released (written above), cancelled, or dropped by the gizmo (e.g. the frame it wasn't shown in)
		if (s.dragging && !result.active)
		{
			if (!result.finished)
				Restore(s);
			EndDrag(s);
		}

		return result.hovered || result.active;
	}

	bool IsUsing()
	{
		return S().dragging;
	}

	void Cancel()
	{
		State &s = S();
		if (!s.dragging)
			return;
		Restore(s);
		ViewportGizmo::Cancel();
		EndDrag(s);
		s.notice = "Cancelled";
		s.noticeUntil = ImGui::GetTime() + 2.0;
	}

	const std::string &GetStatus()
	{
		const State &s = S();
		if (s.status.empty() && !s.notice.empty() && ImGui::GetTime() < s.noticeUntil)
			return s.notice;
		return s.status;
	}

	bool TransformSelection(const double east, const double north, const double turnDegrees, const double scale)
	{
		State &s = S();
		if (s.dragging)
			return false;
		Refresh(s);
		if (s.descriptors.empty())
			return false;

		const Point3 pivot = RestFrame(s, false).pivot;
		const double angle = turnDegrees / DEGREES;
		Mat4d turn;
		turn.m[0][0] = std::cos(angle) * scale;
		turn.m[0][2] = -std::sin(angle) * scale;
		turn.m[2][0] = std::sin(angle) * scale;
		turn.m[2][2] = std::cos(angle) * scale;
		const Mat4d delta = Translation({ -pivot.x, -pivot.y, -pivot.z }) * turn * Translation({ pivot.x + east, pivot.y, pivot.z - north });

		const Operation operation = std::abs(scale - 1.0) > 1e-12 ? Operation::Scale : std::abs(turnDegrees) > 1e-12 ? Operation::Rotate : Operation::Translate;
		Begin(s, operation);
		ApplyPreview(s, delta);
		Commit(s, delta);
		EndDrag(s);
		return true;
	}

	bool GetSelectionCenter(Point3 &center, double &radius)
	{
		State &s = S();
		if (!s.dragging)
			Refresh(s);
		if (s.descriptors.empty())
			return false;
		XPlane::Bounds2 bounds;
		double top = 0.0;
		for (const Descriptor &descriptor : s.descriptors)
		{
			const Point3 point = LiveCenter(descriptor);
			const double r = descriptor.staticCenter ? descriptor.radius : 5.0;
			bounds.Add(XPlane::Point2{ point.x - r, point.z - r });
			bounds.Add(XPlane::Point2{ point.x + r, point.z + r });
			top = std::max(top, point.y);
		}
		const XPlane::Point2 middle = bounds.GetCenter();
		center = { middle.x, top * 0.5, middle.z };
		radius = std::max(2.0, bounds.GetRadius());
		return true;
	}
}

// -------------------------------------------------------
