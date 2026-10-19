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
 * editor_selection.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "editor_selection.h"
#include "editor_context.h"
#include "editor_project.h"
#include "editor_scene.h"
#include <algorithm>
#include <SceneryEditorX/renderer/scene_picking.h>
#include <SceneryEditorX/xplane/xp_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorSelection
{
	using XPlane::AirportElementComponent;
	using XPlane::AirportElementKind;
	using XPlane::SceneAirports;

	namespace
	{
		struct State
		{
			std::vector<Target> targets;
			size_t active = 0;					// Index into targets (meaningless when empty)
			uint64_t lastPrimary = 0;			// What Sync() last wrote into EditorContext::selectedEntity
			uint64_t revision = 1;
			std::optional<Target> hovered;
			uint64_t hoveredRevision = 0;

			// Highlight cache: rebuilt when the selection, the hovered target or the scene's structure changed
			uint64_t highlightSelection = 0;
			uint64_t highlightHovered = ~0ull;
			uint64_t highlightStructure = ~0ull;
		};

		State &GetState()
		{
			static State state;
			return state;
		}

		entt::entity ParentOf(const entt::registry &registry, const entt::entity handle)
		{
			const RelationshipComponent *relationship = registry.try_get<RelationshipComponent>(handle);
			if (!relationship || static_cast<uint64_t>(relationship->pParentHandle) == 0)
				return entt::null;
			return Scene::TryGetEntityHandle(relationship->pParentHandle);
		}

		/* @brief The entity a target is at now, refreshing the cached UUID (cheap when it is still right). */
		uint64_t ResolveCached(Target &target)
		{
			switch (target.type)
			{
				case TargetType::Entity:
					return EditorScene::FindHandle(target.entity) != entt::null ? target.entity : 0;

				case TargetType::AirportElement:
				{
					if (const AirportElementComponent *element = SceneAirports::FindElement(target.entity))
					{
						if (element->kind == target.kind && element->index == target.index)
							return target.entity;
					}
					target.entity = SceneAirports::FindEntity(target.kind, target.index, 0);
					if (target.entity == 0)
					{
						std::vector<uint64_t> parts;
						SceneAirports::FindEntities(target.kind, target.index, parts);
						target.entity = parts.empty() ? 0 : parts.front();
					}
					return target.entity;
				}

				case TargetType::ProjectItem:
					target.entity = EditorProjects::GetPlacedEntity(target.itemId);
					return target.entity;
			}
			return 0;
		}

		void Changed(State &state)
		{
			++state.revision;
		}

		/* @brief Writes the active target's entity into the context (what the other panels show). */
		void Publish(State &state, EditorContext *context)
		{
			uint64_t primary = 0;
			if (!state.targets.empty())
			{
				state.active = std::min(state.active, state.targets.size() - 1);
				primary = ResolveCached(state.targets[state.active]);
			}
			state.lastPrimary = primary;
			if (context)
				context->selectedEntity = primary;
		}

		void AddPickIds(const entt::registry &registry, const uint64_t rootId, std::vector<uint32_t> &out)
		{
			const entt::entity root = EditorScene::FindHandle(rootId);
			if (root == entt::null)
				return;
			// Depth first through the hierarchy; every entity's id (only drawn ones show up in the pick buffer)
			std::vector<entt::entity> stack = { root };
			constexpr size_t LIMIT = 500000;
			while (!stack.empty() && out.size() < LIMIT)
			{
				const entt::entity handle = stack.back();
				stack.pop_back();
				out.push_back(ScenePicking::GetPickId(handle));
				if (const RelationshipComponent *relationship = registry.try_get<RelationshipComponent>(handle))
				{
					for (const UUID &child : relationship->pChildren)
					{
						const entt::entity childHandle = Scene::TryGetEntityHandle(child);
						if (childHandle != entt::null && registry.valid(childHandle))
							stack.push_back(childHandle);
					}
				}
			}
		}

		void AddTargetPickIds(const entt::registry &registry, const Target &target, std::vector<uint32_t> &out)
		{
			std::vector<uint64_t> roots;
			CollectEntities(target, roots);
			for (const uint64_t root : roots)
				AddPickIds(registry, root, out);
		}
	}

	// -------------------------------------------------------

	Target Target::ForEntity(const uint64_t entity)
	{
		Target target;
		target.type = TargetType::Entity;
		target.entity = entity;
		return target;
	}

	Target Target::ForElement(const AirportElementKind kind, const uint32_t index, const uint64_t entity)
	{
		Target target;
		target.type = TargetType::AirportElement;
		target.kind = kind;
		target.index = index;
		target.entity = entity;
		return target;
	}

	Target Target::ForItem(std::string itemId, const uint64_t entity)
	{
		Target target;
		target.type = TargetType::ProjectItem;
		target.itemId = std::move(itemId);
		target.entity = entity;
		return target;
	}

	bool Target::SameAs(const Target &other) const
	{
		if (type != other.type)
			return false;
		switch (type)
		{
			case TargetType::Entity:			return entity == other.entity;
			case TargetType::AirportElement:	return kind == other.kind && index == other.index;
			case TargetType::ProjectItem:		return itemId == other.itemId;
		}
		return false;
	}

	// -------------------------------------------------------

	void Sync(EditorContext *context)
	{
		State &state = GetState();

		// Another panel changed the selection (outliner, project tree, an airport rebuild re-selecting its element)
		if (context && context->selectedEntity != state.lastPrimary)
		{
			// Rebuilds that replace entities (the project's scenery, an airport) zero the context's entity as they destroy
			// it: the targets name what was selected, so they are found again below instead of being cleared
			const bool destroyed = state.lastPrimary != 0 && EditorScene::FindHandle(state.lastPrimary) == entt::null;
			if (context->selectedEntity == 0)
			{
				if (!destroyed && !state.targets.empty())
				{
					state.targets.clear();
					Changed(state);
				}
			}
			else if (const std::optional<Target> adopted = FromEntity(context->selectedEntity))
			{
				const auto it = std::ranges::find_if(state.targets, [&adopted](const Target &target) { return target.SameAs(*adopted); });
				if (it != state.targets.end())
				{
					// Already selected (found again after a rebuild): make it the active one, keep the rest
					it->entity = context->selectedEntity;
					state.active = static_cast<size_t>(it - state.targets.begin());
				}
				else
				{
					state.targets = { *adopted };
					state.active = 0;
				}
				Changed(state);
			}
		}

		// Targets whose rows / items / entities are gone drop out
		const size_t before = state.targets.size();
		const Target *active = state.targets.empty() ? nullptr : &state.targets[std::min(state.active, state.targets.size() - 1)];
		const Target activeCopy = active ? *active : Target();
		std::erase_if(state.targets, [](Target &target) { return ResolveCached(target) == 0; });
		if (state.targets.size() != before)
		{
			const auto it = std::ranges::find_if(state.targets, [&activeCopy](const Target &target) { return target.SameAs(activeCopy); });
			state.active = it != state.targets.end() ? static_cast<size_t>(it - state.targets.begin()) : (state.targets.empty() ? 0 : state.targets.size() - 1);
			Changed(state);
		}

		if (state.hovered)
		{
			Target hovered = *state.hovered;
			if (ResolveCached(hovered) == 0)
				SetHovered(std::nullopt);
			else
				state.hovered = hovered;
		}

		Publish(state, context);
	}

	const std::vector<Target> &Get()
	{
		return GetState().targets;
	}

	const Target *GetActive()
	{
		const State &state = GetState();
		return state.targets.empty() ? nullptr : &state.targets[std::min(state.active, state.targets.size() - 1)];
	}

	bool IsEmpty()
	{
		return GetState().targets.empty();
	}

	bool IsSelected(const Target &target)
	{
		return std::ranges::any_of(GetState().targets, [&target](const Target &selected) { return selected.SameAs(target); });
	}

	void Select(EditorContext *context, const Target &target, const Mode mode)
	{
		Select(context, std::vector<Target>{ target }, mode);
	}

	void Select(EditorContext *context, const std::vector<Target> &targets, const Mode mode)
	{
		State &state = GetState();
		if (mode == Mode::Replace)
		{
			state.targets.clear();
			state.active = 0;
		}

		for (const Target &target : targets)
		{
			const auto it = std::ranges::find_if(state.targets, [&target](const Target &selected) { return selected.SameAs(target); });
			const bool present = it != state.targets.end();
			const bool add = mode == Mode::Replace || mode == Mode::Add || (mode == Mode::Toggle && !present);
			if (add)
			{
				if (present)
					state.active = static_cast<size_t>(it - state.targets.begin());
				else
				{
					state.targets.push_back(target);
					state.active = state.targets.size() - 1;
				}
			}
			else if (present)
			{
				state.targets.erase(it);
				if (!state.targets.empty())
					state.active = std::min(state.active, state.targets.size() - 1);
			}
		}

		std::erase_if(state.targets, [](Target &target) { return ResolveCached(target) == 0; });
		Changed(state);
		Publish(state, context);
	}

	void Clear(EditorContext *context)
	{
		State &state = GetState();
		if (!state.targets.empty())
		{
			state.targets.clear();
			Changed(state);
		}
		Publish(state, context);
	}

	uint64_t GetRevision()
	{
		return GetState().revision;
	}

	// -------------------------------------------------------

	std::optional<Target> FromPick(const uint64_t entityId)
	{
		const entt::entity handle = EditorScene::FindHandle(entityId);
		entt::registry *registry = EditorScene::GetRegistry();
		if (handle == entt::null || !registry)
			return std::nullopt;

		// Part of a placed X-Plane object: the project item that placed it, the airport row whose lights it is, or the object
		uint64_t root = 0;
		if (XPlane::SceneObjects::FindObject(entityId, &root) && root != 0)
		{
			if (std::string item = EditorProjects::FindItemForEntity(root); !item.empty())
				return Target::ForItem(std::move(item), EditorProjects::GetPlacedEntity(item));
			AirportElementKind kind = AirportElementKind::Airport;
			uint32_t index = 0;
			if (SceneAirports::FindLightSource(root, kind, index))
				return Target::ForElement(kind, index);
			return Target::ForEntity(root);
		}

		// A project item's drawing (polygon, line, forest...)
		if (std::string item = EditorProjects::FindItemForEntity(entityId); !item.empty())
			return Target::ForItem(std::move(item), EditorProjects::GetPlacedEntity(item));

		// An apt.dat row (the pack's DSF art is selected as itself: inspectable, not editable)
		if (const AirportElementComponent *element = SceneAirports::FindElement(entityId))
		{
			if (element->kind != AirportElementKind::Airport && element->kind != AirportElementKind::Group && !SceneAirports::IsOverlayKind(element->kind))
				return Target::ForElement(element->kind, element->index, entityId);
			return Target::ForEntity(entityId);
		}

		// Anything else: its top-level ancestor (an imported model as a whole)
		entt::entity top = handle;
		for (int depth = 0; depth < 64; ++depth)
		{
			const entt::entity parent = ParentOf(*registry, top);
			if (parent == entt::null)
				break;
			top = parent;
		}
		return Target::ForEntity(EditorScene::GetID(top));
	}

	std::optional<Target> FromEntity(const uint64_t entityId)
	{
		if (EditorScene::FindHandle(entityId) == entt::null)
			return std::nullopt;

		if (const AirportElementComponent *element = SceneAirports::FindElement(entityId))
		{
			if (element->kind != AirportElementKind::Airport && element->kind != AirportElementKind::Group && !SceneAirports::IsOverlayKind(element->kind))
				return Target::ForElement(element->kind, element->index, entityId);
		}
		if (std::string item = EditorProjects::FindItemForEntity(entityId); !item.empty() && EditorProjects::GetPlacedEntity(item) == entityId)
			return Target::ForItem(std::move(item), entityId);
		return Target::ForEntity(entityId);
	}

	uint64_t Resolve(const Target &target)
	{
		Target copy = target;
		return ResolveCached(copy);
	}

	void CollectEntities(const Target &target, std::vector<uint64_t> &out)
	{
		switch (target.type)
		{
			case TargetType::Entity:
				if (EditorScene::FindHandle(target.entity) != entt::null)
					out.push_back(target.entity);
				break;
			case TargetType::AirportElement:
				SceneAirports::FindEntities(target.kind, target.index, out);
				SceneAirports::FindLightObjects(target.kind, target.index, out);
				break;
			case TargetType::ProjectItem:
				if (const uint64_t entity = EditorProjects::GetPlacedEntity(target.itemId))
					out.push_back(entity);
				break;
		}
	}

	std::string Describe(const Target &target)
	{
		if (target.type == TargetType::ProjectItem)
		{
			if (ProjectFile::ProjectDocument *document = EditorProjects::GetDocument())
			{
				if (const ProjectFile::SceneryItem *item = document->FindItem(target.itemId))
					return !item->name.empty() ? item->name : std::string(ProjectFile::GetItemKindName(item->kind));
			}
		}
		const uint64_t entity = Resolve(target);
		const entt::entity handle = EditorScene::FindHandle(entity);
		if (handle != entt::null)
			return EditorScene::GetName(handle);
		if (target.type == TargetType::AirportElement)
			return std::string(XPlane::GetAirportElementKindName(target.kind)) + " " + std::to_string(target.index + 1);
		return "Entity";
	}

	bool IsReadOnly(const Target &target, std::string *reason)
	{
		if (target.type != TargetType::Entity)
			return false;

		const auto readOnly = [reason](const char *why)
		{
			if (reason)
				*reason = why;
			return true;
		};

		if (const AirportElementComponent *element = SceneAirports::FindElement(target.entity))
		{
			if (SceneAirports::IsOverlayKind(element->kind))
				return readOnly("Drawn from the scenery pack's DSF: changes here wouldn't be saved");
			if (element->kind == AirportElementKind::Airport || element->kind == AirportElementKind::Group)
				return readOnly("Move the airport's rows, not the airport");
		}

		// An object the pack's DSF places around the airport (under the airport's "Objects" group)
		uint64_t root = 0;
		if (XPlane::SceneObjects::FindObject(target.entity, &root) && root != 0)
		{
			entt::registry *registry = EditorScene::GetRegistry();
			const entt::entity handle = EditorScene::FindHandle(root);
			const entt::entity parent = registry && handle != entt::null ? ParentOf(*registry, handle) : entt::null;
			if (parent != entt::null)
			{
				if (const AirportElementComponent *group = SceneAirports::FindElement(EditorScene::GetID(parent)))
				{
					if (group->kind == AirportElementKind::Group && static_cast<AirportElementKind>(group->index) == AirportElementKind::SceneryObject)
						return readOnly("Placed by the scenery pack's DSF: changes here wouldn't be saved");
				}
			}
		}
		return false;
	}

	void SetHovered(const std::optional<Target> &target)
	{
		State &state = GetState();
		const bool same = state.hovered.has_value() == target.has_value() && (!target || state.hovered->SameAs(*target));
		if (same)
			return;
		state.hovered = target;
		++state.hoveredRevision;
	}

	const std::optional<Target> &GetHovered()
	{
		return GetState().hovered;
	}

	void UpdateHighlight()
	{
		State &state = GetState();
		Scene *scene = Scene::GetActive();
		entt::registry *registry = EditorScene::GetRegistry();
		if (!scene || !registry)
		{
			ScenePicking::SetHighlight({}, {}, {});
			return;
		}

		const uint64_t structure = Scene::GetStructureVersion() ^ (Scene::GetDestroyVersion() << 32);
		if (state.highlightSelection == state.revision && state.highlightHovered == state.hoveredRevision && state.highlightStructure == structure)
			return;
		state.highlightSelection = state.revision;
		state.highlightHovered = state.hoveredRevision;
		state.highlightStructure = structure;

		std::vector<uint32_t> selected, active, hovered;
		for (size_t i = 0; i < state.targets.size(); ++i)
			AddTargetPickIds(*registry, state.targets[i], i == state.active ? active : selected);
		if (state.hovered && !IsSelected(*state.hovered))
			AddTargetPickIds(*registry, *state.hovered, hovered);
		ScenePicking::SetHighlight(selected, active, hovered);
	}
}

// -------------------------------------------------------
