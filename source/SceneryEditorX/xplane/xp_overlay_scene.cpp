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
 * xp_overlay_scene.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "xp_overlay_scene.h"
#include "xp_forest3d.h"
#include "xp_scene.h"
#include "xp_terrain_scene.h"
#include "xp_texture_loader.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <map>
#include <thread>
#include <unordered_map>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/vulkan/enums.h>
#include <SceneryEditorX/scene/components/component_sets.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/mesh.h>
#include <SceneryEditorX/scene/scene.h>

// Building a mile of tile scenery copies, lifts and converts a million vertices: keep it optimized in MSVC debug builds
#if defined(_MSC_VER) && !defined(__clang__) && !defined(NDEBUG)
#pragma optimize("gt", on)
#endif

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		namespace fs = std::filesystem;

		constexpr const char *LOG_TAG = "XPlaneOverlay";
		constexpr float BLEND_SPLIT_ALPHA = 0.9f;	// Alpha at or above this stays in the deferred passes of a blended material (as SceneAirports)
		constexpr int MAX_DEPTH = 64;				// Parent links followed looking for a group

		/* @brief A group's meshes that share a material: one sub-mesh. */
		struct Part
		{
			std::string name;
			OverlayKind kind = OverlayKind::DrapedPolygon;
			fs::path texture;
			float color[4] = { 0.6f, 0.6f, 0.6f, 1.0f };
			bool cutout = false;
			float cutoff = 0.5f;
			bool blended = false;
			bool lodSwitch = false;			// Forest billboards handing over to 3D trees near the camera (XPlane::Forest3d)
			std::vector<Vertex_PosTexNorTan> vertices;
			std::vector<uint32_t> indices;
			Bounds2 bounds;
			uint32_t subMesh = 0;
			float height = 0.0f;			// Draped kinds: the layer height the builder drew them at
			uint32_t placement = 0;			// The DSF placement of the first mesh
		};

		/* @brief Kinds drawn flat on the ground, in drape order (Renderable::SetDrapeOrder) rather than by depth. */
		bool IsDraped(const OverlayKind kind)
		{
			return kind != OverlayKind::Forest && kind != OverlayKind::Facade;
		}

		std::string MaterialKey(const OverlayMesh &mesh)
		{
			char color[64];
			std::snprintf(color, sizeof(color), "|%.3f,%.3f,%.3f,%.3f", mesh.color[0], mesh.color[1], mesh.color[2], mesh.color[3]);
			return std::to_string(static_cast<int>(mesh.kind)) + "|" + mesh.texture.generic_string() + color + (mesh.cutout ? "|c" + std::to_string(mesh.cutoff) : "") + (mesh.blended ? "|b" : "");
		}

		/* @brief Appends a mesh's geometry as built (positions, UVs, normals) with a tangent perpendicular to the normal. */
		void Append(Part &part, const OverlayMesh &mesh)
		{
			const uint32_t base = static_cast<uint32_t>(part.vertices.size());
			for (const OverlayVertex &vertex : mesh.vertices)
			{
				const Vec3 normal(vertex.nx, vertex.ny, vertex.nz);
				const Vec3 tangent = std::abs(vertex.ny) > 0.9f ? Vec3(1.0f, 0.0f, 0.0f) : xMath::Normalize(xMath::Cross(Vec3(0.0f, 1.0f, 0.0f), normal));
				part.vertices.push_back(Vertex_PosTexNorTan(Vec3(vertex.x, vertex.y, vertex.z), Vec2(vertex.u, vertex.v), normal, tangent));
			}
			for (const uint32_t index : mesh.indices)
				part.indices.push_back(base + index);
			part.bounds.Add(mesh.bounds);
		}

		entt::registry *GetRegistry()
		{
			Scene *scene = Scene::GetActive();
			return scene ? &scene->GetRegistry() : nullptr;
		}

		entt::entity FindHandle(entt::registry &registry, const uint64_t id)
		{
			if (id == 0)
				return entt::null;
			const Entity entity = Scene::TryGetEntityWithUUID(UUID(id));
			const entt::entity handle = entity ? static_cast<entt::entity>(entity) : entt::null;
			return registry.valid(handle) ? handle : entt::null;
		}

		uint64_t GetId(entt::registry &registry, const entt::entity handle)
		{
			const IDComponent *id = registry.try_get<IDComponent>(handle);
			return id ? static_cast<uint64_t>(id->pID) : 0;
		}

		uint64_t GetParentId(entt::registry &registry, const uint64_t id)
		{
			const entt::entity handle = FindHandle(registry, id);
			if (handle == entt::null)
				return 0;
			const RelationshipComponent *relationship = registry.try_get<RelationshipComponent>(handle);
			return relationship ? static_cast<uint64_t>(relationship->pParentHandle) : 0;
		}

		void Attach(entt::registry &registry, const entt::entity child, const entt::entity parent)
		{
			const UUID parentId = registry.get<IDComponent>(parent).pID;
			const UUID childId = registry.get<IDComponent>(child).pID;
			registry.get<RelationshipComponent>(child).pParentHandle = parentId;
			registry.get<RelationshipComponent>(parent).pChildren.push_back(childId);
			Scene::MarkStructureChanged();
		}
	}

	// -------------------------------------------------------

	struct OverlayScene::Impl
	{
		struct TextureEntry
		{
			Ref<ImageResource> image;
			TextureInfo info;
		};

		/* @brief A drawn part of a group: its entity and what its drape order is made of. */
		struct PartState
		{
			uint64_t entity = 0;
			OverlayKind kind = OverlayKind::DrapedPolygon;
			float height = 0.0f;
			uint32_t placementOffset = 0;	// Its DSF placement minus the group's first
		};

		/* @brief What one group shows: its entity, its own mesh (so it can be rebuilt alone), its parts and objects. */
		struct GroupState
		{
			uint64_t entity = 0;
			uint64_t fingerprint = 0;
			uint32_t firstPlacement = 0;
			std::string name;
			Ref<Mesh> mesh;
			std::vector<PartState> parts;
			std::vector<uint64_t> objects;
			Bounds2 bounds;
			uint32_t triangles = 0;
			uint32_t missingTextures = 0;
		};

		uint64_t root = 0;
		uint32_t drapeSequenceBase = 0;								// SetDrapeSequenceBase()
		bool onTerrain = false;										// SetOnTerrain()
		bool clip = false;
		std::map<std::string, Ref<MaterialAsset>> materials;		// By texture + alpha treatment (or color)
		std::map<std::string, TextureEntry> textures;				// By file
		std::unordered_map<std::string, GroupState> groups;		// By group key
		std::unordered_map<uint64_t, std::string> keys;			// Group, part and object root entities -> group key
		Bounds2 bounds;
		Stats stats;

		/* @brief The part's texture (loaded once per file) with its alpha treatment; its flat color when there's no texture. */
		Ref<MaterialAsset> GetMaterial(const Part &part, bool &missingTexture)
		{
			ImageResource *image = nullptr;
			bool hasAlpha = false;
			std::string key;
			missingTexture = false;
			if (!part.texture.empty())
			{
				key = part.texture.generic_string();
				auto it = textures.find(key);
				if (it == textures.end())
				{
					TextureEntry entry;
					const fs::path file = TextureLoader::ResolveTexturePath(part.texture.parent_path(), part.texture.filename().string());
					if (!file.empty())
						entry.image = TextureLoader::Load(file, TextureUsage::Albedo, &entry.info);
					if (!entry.image)
						SEDX_CORE_WARN_TAG(LOG_TAG, "Texture not found or unreadable: {}{}", key, entry.info.error.empty() ? std::string() : " (" + entry.info.error + ")");
					it = textures.emplace(key, std::move(entry)).first;
				}
				image = it->second.image.Get();
				hasAlpha = it->second.info.hasAlpha;
				missingTexture = image == nullptr;
			}
			if (!image)
			{
				char color[64];
				std::snprintf(color, sizeof(color), "color:%.3f,%.3f,%.3f,%.3f", part.color[0], part.color[1], part.color[2], part.color[3]);
				key = color;
			}

			const bool cutout = image && hasAlpha && part.cutout;
			const bool blended = image && hasAlpha && part.blended && !cutout;
			if (cutout)
				key += "|cut" + std::to_string(part.cutoff);
			if (blended)
				key += "|blend";
			const bool foliage = part.kind == OverlayKind::Forest;
			if (foliage)
				key += "|foliage";
			if (part.lodSwitch)
				key += "|far";

			if (const auto it = materials.find(key); it != materials.end())
				return it->second;

			Ref<MaterialAsset> material = CreateRef<MaterialAsset>();
			material->SetName(image ? "Overlay " + part.texture.filename().string() : "Overlay color");
			material->SetProperty(MaterialClass::ColorR, image ? 1.0f : part.color[0]);
			material->SetProperty(MaterialClass::ColorG, image ? 1.0f : part.color[1]);
			material->SetProperty(MaterialClass::ColorB, image ? 1.0f : part.color[2]);
			material->SetProperty(MaterialClass::ColorA, image ? 1.0f : part.color[3]);
			material->SetProperty(MaterialClass::TextureTilingX, 1.0f);
			material->SetProperty(MaterialClass::TextureTilingY, 1.0f);
			material->SetProperty(MaterialClass::Roughness, 0.9f);
			material->SetProperty(MaterialClass::Metalness, 0.0f);
			material->SetProperty(MaterialClass::Normal, 0.0f);
			material->SetProperty(MaterialProperty::CullMode, static_cast<float>(CullMode::None));
			// Like the OBJ8 materials: cutouts alpha-test at the asset's ratio; blended art keeps its opaque texels in the
			// deferred passes and blends the rest in the transparent pass.
			material->SetProperty(MaterialProperty::AlphaCutoff, blended ? BLEND_SPLIT_ALPHA : (cutout ? std::clamp(part.cutoff, 0.001f, 1.0f) : 0.0f));
			material->SetProperty(MaterialProperty::BlendMode, blended ? 1.0f : 0.0f);
			material->SetProperty(MaterialProperty::XpFoliage, foliage ? 1.0f : 0.0f);
			if (part.lodSwitch)
			{
				material->SetProperty(MaterialProperty::XpLodSwitchMode, 1.0f);
				material->SetProperty(MaterialProperty::XpLodSwitchDistance, Forest3d::GetSwitchDistance());
			}
			if (image)
				material->SetTexture(MaterialTextureType::Color, image);
			materials.emplace(key, material);
			return material;
		}

		uint64_t MakeDrapeOrder(const PartState &part, const uint32_t firstPlacement) const
		{
			return Renderable::MakeDrapeOrder(part.height, drapeSequenceBase + std::min(firstPlacement + part.placementOffset, 0x7FFFFFFFu));
		}

		/* @brief Takes a group's entities out of the scene (its parts and objects go with it) and forgets its keys. */
		void DestroyGroup(GroupState &group)
		{
			keys.erase(group.entity);
			for (const PartState &part : group.parts)
				keys.erase(part.entity);
			for (const uint64_t object : group.objects)
				keys.erase(object);
			if (entt::registry *registry = GetRegistry(); registry && FindHandle(*registry, group.entity) != entt::null)
			{
				if (const Entity entity = Scene::TryGetEntityWithUUID(UUID(group.entity)))
					Scene::DestroyEntity(entity);
				Scene::MarkStructureChanged();
			}
			group.mesh.Reset();	// After the renderables using it are gone; its geometry space is reused frames later
		}

		/* @brief Builds a group's entity below the root: its meshes merged per material into its own mesh, its objects. */
		void BuildGroup(entt::registry &registry, const entt::entity rootHandle, const Group &group, GroupState &state, uint32_t &failed, std::string &firstError)
		{
			state = GroupState{};
			state.fingerprint = group.fingerprint;
			state.firstPlacement = group.firstPlacement;
			state.name = group.name;

			std::vector<Part> parts;
			std::map<std::string, size_t> byMaterial;
			static const TerrainSurface flat;
			const TerrainSurface &surface = onTerrain ? SceneTerrain::GetSurface() : flat;
			OverlayMesh draped;
			std::map<fs::path, std::vector<OverlayTree3d>> trees3d;	// X-Plane 12 forests' 3D trees, on the ground
			for (const OverlayMesh *mesh : group.meshes)
			{
				if (!mesh || mesh->indices.empty())
					continue;
				const bool lodSwitch = mesh->kind == OverlayKind::Forest && !mesh->trees3d.empty() && Forest3d::DrawsEveryTree(mesh->forestFile);
				if (lodSwitch)
				{
					std::vector<OverlayTree3d> &planted = trees3d[mesh->forestFile];
					for (OverlayTree3d tree : mesh->trees3d)
					{
						if (float ground = 0.0f; !surface.IsEmpty())
						{
							if (!surface.HeightAt(tree.x, tree.z, ground) && clip)
								continue;
							tree.y = ground;
						}
						planted.push_back(tree);
					}
				}
				const auto [it, added] = byMaterial.emplace(MaterialKey(*mesh) + (lodSwitch ? "|3d" : ""), parts.size());
				if (added)
				{
					Part &part = parts.emplace_back();
					part.name = mesh->name;
					part.kind = mesh->kind;
					part.texture = mesh->texture;
					std::copy(std::begin(mesh->color), std::end(mesh->color), std::begin(part.color));
					part.cutout = mesh->cutout;
					part.cutoff = mesh->cutoff;
					part.blended = mesh->blended;
					part.lodSwitch = lodSwitch;
					part.height = mesh->vertices.empty() ? 0.0f : mesh->vertices.front().y;
					part.placement = mesh->placement;
				}
				if (surface.IsEmpty())
				{
					Append(parts[it->second], *mesh);
					continue;
				}
				// On the terrain: drapes cut along it, facades and forests standing on it
				if (clip && !IsDraped(mesh->kind) && mesh->bounds.valid)
				{
					float ground = 0.0f;
					if (!surface.HeightAt(0.5 * (mesh->bounds.minX + mesh->bounds.maxX), 0.5 * (mesh->bounds.minZ + mesh->bounds.maxZ), ground) && mesh->kind != OverlayKind::Forest)
						continue;	// Forests are planted tree by tree where the caller allows
				}
				draped = *mesh;
				if (IsDraped(mesh->kind))
					surface.Drape(draped, clip);
				else
					surface.Lift(draped.vertices);
				Append(parts[it->second], draped);
			}
			std::erase_if(parts, [](const Part &part) { return part.indices.empty(); });	// Clipped away

			for (Part &part : parts)
			{
				if (!state.mesh)
				{
					state.mesh = CreateRef<Mesh>();
					state.mesh->SetFlags(0);	// Keep the local meters as they are: no normalization or generated LODs
					state.mesh->SetResourceName(group.name);
				}
				state.mesh->AddGeometry(part.vertices, part.indices, false, &part.subMesh);
				state.triangles += static_cast<uint32_t>(part.indices.size() / 3);
				state.bounds.Add(part.bounds);
			}
			if (state.mesh)
				state.mesh->CreateGpuBuffers();

			const entt::entity handle = static_cast<entt::entity>(Scene::CreateEntity(group.name.empty() ? std::string("Overlay") : group.name));
			if (!registry.valid(handle))
				return;
			Attach(registry, handle, rootHandle);
			state.entity = GetId(registry, handle);
			keys[state.entity] = group.key;

			const auto addRenderable = [&](const entt::entity partHandle, const Part &part)
			{
				PartState partState;
				partState.entity = GetId(registry, partHandle);
				partState.kind = part.kind;
				partState.height = part.height;
				partState.placementOffset = part.placement >= group.firstPlacement ? part.placement - group.firstPlacement : 0;

				Entity *stored = Scene::GetStoredEntity(partHandle);
				Renderable *renderable = stored ? stored->AddComponent<Renderable>() : nullptr;
				if (renderable)
				{
					bool missingTexture = false;
					renderable->SetMesh(state.mesh.Get(), part.subMesh);
					renderable->SetMaterial(GetMaterial(part, missingTexture));
					renderable->SetFlag(CastsShadows, part.kind == OverlayKind::Facade || part.kind == OverlayKind::Forest);
					if (IsDraped(part.kind))
						renderable->SetDrapeOrder(MakeDrapeOrder(partState, group.firstPlacement));
					state.missingTextures += missingTexture ? 1 : 0;
				}
				state.parts.push_back(partState);
			};

			// One part draws on the group's entity; several get an entity each below it
			if (parts.size() == 1)
				addRenderable(handle, parts.front());
			else
			{
				for (const Part &part : parts)
				{
					const entt::entity partHandle = static_cast<entt::entity>(Scene::CreateEntity(part.name));
					if (!registry.valid(partHandle))
						continue;
					Attach(registry, partHandle, handle);
					keys[GetId(registry, partHandle)] = group.key;
					addRenderable(partHandle, part);
				}
			}

			for (const auto &[forestFile, trees] : trees3d)
				Forest3d::Plant(registry, handle, forestFile, trees, (group.name.empty() ? std::string("Forest") : group.name) + " 3D trees");

			for (const OverlayObject *object : group.objects)
			{
				if (!object)
					continue;
				if (float ground = 0.0f; clip && onTerrain && !SceneTerrain::GetSurface().HeightAt(object->x, object->z, ground))
					continue;
				SceneObjects::PlaceOptions options;
				options.position[0] = static_cast<float>(object->x);
				// On the terrain: MSL at their elevation, graded autogen on the ground under its tile's ground point, the rest
				// on the ground under themselves
				options.position[1] = !onTerrain ? static_cast<float>(object->msl ? 0.0 : object->y)
									: object->msl && SceneTerrain::IsLoaded() ? SceneTerrain::ElevationToY(object->y, object->x, object->z)
									: object->graded ? SceneTerrain::GetHeight(object->groundX, object->groundZ) + static_cast<float>(object->y)
										: SceneTerrain::GetHeight(object->x, object->z) + static_cast<float>(object->msl ? 0.0 : object->y);
				options.position[2] = static_cast<float>(object->z);
				options.headingDegrees = object->heading;
				options.virtualPath = object->resource.starts_with("lib/") ? object->resource : std::string();
				options.name = object->name;
				options.frameCamera = false;
				std::string error;
				const uint64_t objectId = SceneObjects::Place(object->file, options, &error);
				const entt::entity objectHandle = FindHandle(registry, objectId);
				if (objectHandle == entt::null)
				{
					if (failed++ == 0)
						firstError = error;
					continue;
				}
				Attach(registry, objectHandle, handle);
				keys[objectId] = group.key;
				state.objects.push_back(objectId);
			}
		}

		/**
		 * @brief Loads the textures the groups' meshes name that aren't loaded yet: decoded on a few workers, uploaded here
		 * (GetMaterial then finds them). A mile of tile scenery names dozens of 2048 x 2048 textures.
		 */
		void Prefetch(const std::vector<Group> &groups)
		{
			struct Request
			{
				std::string key;
				fs::path file;
				DecodedTexture decoded;
			};
			std::vector<Request> requests;
			std::unordered_map<std::string, bool> seen;
			for (const Group &group : groups)
			{
				for (const OverlayMesh *mesh : group.meshes)
				{
					if (!mesh || mesh->texture.empty())
						continue;
					std::string key = mesh->texture.generic_string();
					if (textures.contains(key) || !seen.emplace(key, true).second)
						continue;
					Request &request = requests.emplace_back();
					request.file = TextureLoader::ResolveTexturePath(mesh->texture.parent_path(), mesh->texture.filename().string());
					request.key = std::move(key);
				}
			}
			if (requests.empty())
				return;
			std::atomic<size_t> next{ 0 };
			const size_t workers = std::min(requests.size(), std::clamp<size_t>(std::thread::hardware_concurrency(), 1, 8));
			std::vector<std::thread> threads;
			for (size_t w = 0; w < workers; ++w)
			{
				threads.emplace_back([&next, &requests]()
				{
					for (size_t i = next++; i < requests.size(); i = next++)
					{
						if (!requests[i].file.empty())
							requests[i].decoded = TextureLoader::Decode(requests[i].file, TextureUsage::Albedo);
					}
				});
			}
			for (std::thread &thread : threads)
				thread.join();
			for (Request &request : requests)
			{
				TextureEntry entry;
				if (!request.file.empty())
					entry.image = TextureLoader::Upload(request.decoded, &entry.info);
				if (!entry.image)
					SEDX_CORE_WARN_TAG(LOG_TAG, "Texture not found or unreadable: {}{}", request.key, entry.info.error.empty() ? std::string() : " (" + entry.info.error + ")");
				textures.emplace(request.key, std::move(entry));
			}
		}

		/* @brief The totals of what every group shows. */
		void Recount()
		{
			const double milliseconds = stats.buildMilliseconds;
			stats = {};
			stats.buildMilliseconds = milliseconds;
			bounds = {};
			for (const auto &[key, group] : groups)
			{
				++stats.groups;
				stats.parts += static_cast<uint32_t>(group.parts.size());
				stats.triangles += group.triangles;
				stats.objects += static_cast<uint32_t>(group.objects.size());
				stats.missingTextures += group.missingTextures;
				bounds.Add(group.bounds);
			}
		}

		/* @brief Drops the GPU resources and the bookkeeping (after the entities using them are gone). */
		void Release()
		{
			for (auto &[key, group] : groups)
				group.mesh.Reset();
			groups.clear();
			materials.clear();
			textures.clear();
			keys.clear();
			bounds = {};
			stats = {};
		}
	};

	// -------------------------------------------------------

	OverlayScene::OverlayScene() : m_Impl(std::make_unique<Impl>())
	{
	}

	OverlayScene::~OverlayScene() = default;

	uint64_t OverlayScene::Build(const std::string &rootName, const std::vector<Group> &groups)
	{
		// Everything from scratch (textures load again), keeping the root so selecting it survives
		Impl &impl = *m_Impl;
		for (auto &[key, group] : impl.groups)
			impl.DestroyGroup(group);
		impl.groups.clear();
		impl.keys.clear();
		impl.materials.clear();
		impl.textures.clear();
		std::vector<Group> fresh = groups;
		for (Group &group : fresh)
			group.reuse = false;
		return Update(rootName, fresh);
	}

	uint64_t OverlayScene::Update(const std::string &rootName, const std::vector<Group> &groups)
	{
		Impl &impl = *m_Impl;
		const auto start = std::chrono::steady_clock::now();
		entt::registry *registry = GetRegistry();
		if (!registry || groups.empty())
		{
			Clear();
			return 0;
		}

		entt::entity rootHandle = FindHandle(*registry, impl.root);
		if (rootHandle == entt::null)
		{
			// A new root (the first build, or the scene lost it): nothing of the old groups is left to reuse
			for (auto &[key, group] : impl.groups)
				group.mesh.Reset();
			impl.groups.clear();
			impl.keys.clear();
			const Entity root = Scene::CreateEntity(rootName);
			rootHandle = static_cast<entt::entity>(root);
			if (!registry->valid(rootHandle))
			{
				impl.root = 0;
				return 0;
			}
			impl.root = GetId(*registry, rootHandle);
		}

		// Groups no longer listed go
		std::unordered_map<std::string, const Group *> listed;
		listed.reserve(groups.size());
		for (const Group &group : groups)
			listed[group.key] = &group;
		for (auto it = impl.groups.begin(); it != impl.groups.end();)
		{
			if (listed.contains(it->first))
			{
				++it;
				continue;
			}
			impl.DestroyGroup(it->second);
			it = impl.groups.erase(it);
		}

		impl.Prefetch(groups);
		uint32_t failed = 0, built = 0, kept = 0;
		std::string firstError;
		for (const Group &group : groups)
		{
			const auto existing = impl.groups.find(group.key);
			if (group.reuse && existing != impl.groups.end() && existing->second.fingerprint == group.fingerprint && existing->second.entity != 0)
			{
				// Unchanged: only its place among the drapes may have moved
				Impl::GroupState &state = existing->second;
				if (state.firstPlacement != group.firstPlacement)
				{
					state.firstPlacement = group.firstPlacement;
					for (const Impl::PartState &part : state.parts)
					{
						if (!IsDraped(part.kind))
							continue;
						Entity *stored = Scene::GetStoredEntity(FindHandle(*registry, part.entity));
						if (Renderable *renderable = stored ? stored->GetComponent<Renderable>() : nullptr)
							renderable->SetDrapeOrder(impl.MakeDrapeOrder(part, state.firstPlacement));
					}
				}
				++kept;
				continue;
			}
			if (group.reuse && group.meshes.empty() && group.objects.empty())
				continue;	// Meant to be kept but not shown (a lost root): nothing to build it from
			if (existing != impl.groups.end())
				impl.DestroyGroup(existing->second);
			impl.BuildGroup(*registry, rootHandle, group, impl.groups[group.key], failed, firstError);
			++built;
		}

		impl.stats.buildMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		impl.Recount();
		SEDX_CORE_INFO_TAG(LOG_TAG, "{}: {} groups ({} built, {} kept), {} parts, {} triangles, {} objects in {:.0f} ms ({} 3D trees in all){}", rootName, impl.stats.groups, built, kept,
			impl.stats.parts, impl.stats.triangles, impl.stats.objects, impl.stats.buildMilliseconds, Forest3d::GetTreeCount(),
			failed ? "; " + std::to_string(failed) + " objects could not be placed: " + firstError : std::string());
		return impl.root;
	}

	uint64_t OverlayScene::GetFingerprint(const std::string_view key) const
	{
		const auto it = m_Impl->groups.find(std::string(key));
		return it != m_Impl->groups.end() ? it->second.fingerprint : 0;
	}

	void OverlayScene::SetDrapeSequenceBase(const uint32_t base)
	{
		m_Impl->drapeSequenceBase = base;
	}

	void OverlayScene::SetOnTerrain(const bool onTerrain, const bool clip)
	{
		m_Impl->onTerrain = onTerrain;
		m_Impl->clip = clip;
	}

	void OverlayScene::Clear()
	{
		Impl &impl = *m_Impl;
		if (entt::registry *registry = GetRegistry())
		{
			if (FindHandle(*registry, impl.root) != entt::null)
			{
				if (const Entity root = Scene::TryGetEntityWithUUID(UUID(impl.root)))
					Scene::DestroyEntity(root);
			}
		}
		impl.root = 0;
		impl.Release();
	}

	uint64_t OverlayScene::GetRootId() const
	{
		return m_Impl->root;
	}

	uint64_t OverlayScene::FindEntity(const std::string_view key) const
	{
		const auto it = m_Impl->groups.find(std::string(key));
		return it != m_Impl->groups.end() ? it->second.entity : 0;
	}

	std::string OverlayScene::FindKey(const uint64_t entityId) const
	{
		const Impl &impl = *m_Impl;
		entt::registry *registry = GetRegistry();
		if (!registry || impl.root == 0)
			return {};
		uint64_t id = entityId;
		for (int depth = 0; id != 0 && id != impl.root && depth < MAX_DEPTH; ++depth)
		{
			if (const auto it = impl.keys.find(id); it != impl.keys.end())
				return it->second;
			id = GetParentId(*registry, id);
		}
		return {};
	}

	bool OverlayScene::Contains(const uint64_t entityId) const
	{
		const Impl &impl = *m_Impl;
		entt::registry *registry = GetRegistry();
		if (!registry || impl.root == 0)
			return false;
		uint64_t id = entityId;
		for (int depth = 0; id != 0 && depth < MAX_DEPTH; ++depth)
		{
			if (id == impl.root)
				return true;
			id = GetParentId(*registry, id);
		}
		return false;
	}

	Bounds2 OverlayScene::GetBounds() const
	{
		return m_Impl->bounds;
	}

	const OverlayScene::Stats &OverlayScene::GetStats() const
	{
		return m_Impl->stats;
	}
}

#if defined(_MSC_VER) && !defined(__clang__) && !defined(NDEBUG)
#pragma optimize("", on)
#endif

// -------------------------------------------------------
