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
 * selection_bounds.cpp
 * -------------------------------------------------------
 * Created: 10/11/2026
 * -------------------------------------------------------
 */
#include "selection_bounds.h"
#include "editor_project.h"
#include "editor_scene.h"
#include <algorithm>
#include <cmath>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/xplane/xp_scene.h>
#include <SceneryEditorX/xplane/xp_terrain_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::SelectionBounds
{
	using EditorSelection::Target;
	using EditorSelection::TargetType;
	using ViewFraming::Box;
	using XPlane::AirportElementComponent;
	using XPlane::AirportElementKind;
	using XPlane::SceneAirports;

	namespace
	{
		constexpr size_t ENTITY_LIMIT = 250000;		// Entities walked below one target (a whole airport has tens of thousands)
		constexpr uint32_t INSTANCE_LIMIT = 100000;	// Instances of one instanced renderable
		constexpr int ITEM_DEPTH_LIMIT = 64;		// Folders inside folders

		ViewFraming::Point ToPoint(const Vec3 &v)
		{
			return { v.x, v.y, v.z };
		}

		bool AddBounds(const BoundingBox &bounds, Box &box)
		{
			return box.Add(ToPoint(bounds.GetMin()), ToPoint(bounds.GetMax()));
		}

		bool AddGroundPoint(const double x, const double z, Box &box)
		{
			return box.Add(ViewFraming::Point{ x, XPlane::SceneTerrain::GetHeight(x, z), z });
		}

		/* @brief A footprint on the ground: its corners and middle at the terrain's height there. */
		bool AddFootprint(const XPlane::Bounds2 &bounds, Box &box)
		{
			if (!bounds.valid)
				return false;
			bool any = AddGroundPoint(0.5 * (bounds.minX + bounds.maxX), 0.5 * (bounds.minZ + bounds.maxZ), box);
			any |= AddGroundPoint(bounds.minX, bounds.minZ, box);
			any |= AddGroundPoint(bounds.maxX, bounds.minZ, box);
			any |= AddGroundPoint(bounds.minX, bounds.maxZ, box);
			any |= AddGroundPoint(bounds.maxX, bounds.maxZ, box);
			return any;
		}

		bool AddGeo(const double latitude, const double longitude, Box &box)
		{
			const XPlane::Point2 local = EditorProjects::GetProjection().ToLocal(latitude, longitude);
			return AddGroundPoint(local.x, local.z, box);
		}

		/**
		 * @brief What a renderable draws, in world space: its mesh's box through the entity's world transform, or through
		 * each instance's (Renderable::GetBoundingBox() is only refreshed when the renderable ticks, so it isn't used).
		 */
		bool AddRenderable(Renderable &renderable, const Matrix &world, Box &box)
		{
			const BoundingBox &mesh = renderable.GetBoundingBoxMesh();
			if (!renderable.HasInstancing())
				return AddBounds(mesh * world, box);
			bool any = false;
			const uint32_t count = std::min(renderable.GetInstanceCount(), INSTANCE_LIMIT);
			for (uint32_t i = 0; i < count; ++i)
				any |= AddBounds(mesh * (renderable.GetInstance(i, false) * world), box);
			return any;
		}

		/* @brief The opened airport as a whole: the circle Frame All fits (its outliers left out). */
		bool AddAirportFocus(Box &box)
		{
			float x = 0.0f, z = 0.0f, radius = 0.0f;
			if (!SceneAirports::GetFocus(x, z, radius))
				return false;
			XPlane::Bounds2 bounds;
			bounds.Add(XPlane::Point2{ static_cast<double>(x) - radius, static_cast<double>(z) - radius });
			bounds.Add(XPlane::Point2{ static_cast<double>(x) + radius, static_cast<double>(z) + radius });
			return AddFootprint(bounds, box);
		}

		/* @brief An airport of the project that isn't the one shown: where its runways, helipads and datum are. */
		bool AddProjectAirport(const XPlane::AptAirport &airport, Box &box)
		{
			bool any = false;
			for (const XPlane::AptRunway &runway : airport.runways)
				for (const XPlane::AptRunwayEnd &end : runway.ends)
					any |= AddGeo(end.latitude, end.longitude, box);
			for (const XPlane::AptWaterRunway &runway : airport.waterRunways)
				for (const XPlane::AptWaterRunway::End &end : runway.ends)
					any |= AddGeo(end.latitude, end.longitude, box);
			for (const XPlane::AptHelipad &helipad : airport.helipads)
				any |= AddGeo(helipad.latitude, helipad.longitude, box);
			double latitude = 0.0, longitude = 0.0;
			if (!any && airport.GetDatum(latitude, longitude))
				any = AddGeo(latitude, longitude, box);
			return any;
		}

		bool AddItem(const ProjectFile::SceneryProject &project, const ProjectFile::SceneryItem &item, Box &box, const int depth)
		{
			bool any = false;

			// What it draws (an object with its height, a facade's walls, a forest's trees...)
			if (const uint64_t entity = EditorProjects::GetPlacedEntity(item.id))
				any |= AddEntity(entity, box);

			// What it is made of, drawn or not (hidden, missing art, a forest drawn only where the terrain is): its nodes on
			// the ground, or, when nothing of it is drawn, the point it is placed at (an object, a road junction; Raw items
			// keep no position)
			for (const ProjectFile::GeoRing &ring : item.rings)
				for (const ProjectFile::GeoNode &node : ring)
					any |= AddGeo(node.latitude, node.longitude, box);
			if (!any && (item.kind == ProjectFile::ItemKind::Object || item.kind == ProjectFile::ItemKind::RoadNode))
				any = AddGeo(item.latitude, item.longitude, box);

			// A folder or an airport: its items, and the airport itself
			if (item.kind == ProjectFile::ItemKind::Airport)
			{
				if (!item.airportId.empty() && item.airportId == EditorProjects::GetShownAirport() && AddAirportFocus(box))
					any = true;
				else
				{
					for (const ProjectFile::ProjectAirport &airport : project.airports)
						if (airport.airport.id == item.airportId)
							any |= AddProjectAirport(airport.airport, box);
				}
			}
			if (item.IsContainer() && depth < ITEM_DEPTH_LIMIT)
				for (const ProjectFile::SceneryItem &child : item.children)
					any |= AddItem(project, child, box, depth + 1);
			return any;
		}
	}

	// -------------------------------------------------------

	bool AddEntity(const uint64_t entityId, Box &box)
	{
		entt::registry *registry = EditorScene::GetRegistry();
		const entt::entity root = EditorScene::FindHandle(entityId);
		if (!registry || root == entt::null)
			return false;

		// The airport's root: the airport as Frame All shows it (the DSF art around it reaches a mile out, and a far
		// outlier such as KSEA's Space Needle would shrink the airport to a speck)
		if (const AirportElementComponent *element = SceneAirports::FindElement(entityId))
		{
			if (element->kind == AirportElementKind::Airport && AddAirportFocus(box))
				return true;
		}

		Box drawn, places;
		std::vector<entt::entity> stack = { root };
		size_t visited = 0;
		while (!stack.empty() && visited < ENTITY_LIMIT)
		{
			const entt::entity handle = stack.back();
			stack.pop_back();
			++visited;
			if (Entity *entity = EditorScene::FindEntity(handle))
			{
				const Matrix world = entity->GetMatrix();
				places.Add(ToPoint(world.GetTranslation()));
				Renderable *renderable = entity->GetComponent<Renderable>();
				if (renderable && entity->GetActive())
					AddRenderable(*renderable, world, drawn);
			}
			// An apt.dat row's footprint (pavement, a painted line) frames even while its kind is hidden
			if (const AirportElementComponent *element = SceneAirports::FindElement(EditorScene::GetID(handle)))
			{
				if (element->kind != AirportElementKind::Airport && element->kind != AirportElementKind::Group)
					AddFootprint(element->bounds, drawn);
			}
			if (const RelationshipComponent *relationship = registry->try_get<RelationshipComponent>(handle))
			{
				for (const UUID &child : relationship->pChildren)
				{
					const entt::entity childHandle = Scene::TryGetEntityHandle(child);
					if (childHandle != entt::null && registry->valid(childHandle))
						stack.push_back(childHandle);
				}
			}
		}
		return box.Add(drawn.IsValid() ? drawn : places);
	}

	bool AddTarget(const Target &target, Box &box)
	{
		switch (target.type)
		{
			case TargetType::Entity:
				return AddEntity(target.entity, box);

			case TargetType::AirportElement:
			{
				std::vector<uint64_t> entities;
				EditorSelection::CollectEntities(target, entities);
				bool any = false;
				for (const uint64_t entity : entities)
					any |= AddEntity(entity, box);
				return any;
			}

			case TargetType::ProjectItem:
			{
				ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
				const ProjectFile::SceneryItem *item = document ? document->FindItem(target.itemId) : nullptr;
				if (item)
					return AddItem(document->GetProject(), *item, box, 0);
				const uint64_t entity = EditorSelection::Resolve(target);
				return entity != 0 && AddEntity(entity, box);
			}
		}
		return false;
	}

	bool AddSelection(Box &box)
	{
		bool any = false;
		for (const Target &target : EditorSelection::Get())
			any |= AddTarget(target, box);
		return any;
	}

	bool AddGroundPoints(const std::vector<XPlane::Point2> &points, Box &box)
	{
		bool any = false;
		for (const XPlane::Point2 &point : points)
			any |= AddGroundPoint(point.x, point.z, box);
		return any;
	}

	bool AddScene(Box &box)
	{
		bool any = false;
		for (Entity *entity : Scene::GetRenderableEntities())
		{
			Renderable *renderable = entity ? entity->GetComponent<Renderable>() : nullptr;
			if (renderable && entity->GetActive() && !EditorScene::IsViewportCamera(static_cast<entt::entity>(*entity)))
				any |= AddRenderable(*renderable, entity->GetMatrix(), box);
		}
		return any;
	}
}

// -------------------------------------------------------
