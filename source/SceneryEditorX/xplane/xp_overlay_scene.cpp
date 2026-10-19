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
#include "xp_scene.h"
#include "xp_texture_loader.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <map>
#include <unordered_map>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/vulkan/enums.h>
#include <SceneryEditorX/scene/components/component_sets.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/mesh.h>
#include <SceneryEditorX/scene/scene.h>

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
			std::vector<Vertex_PosTexNorTan> vertices;
			std::vector<uint32_t> indices;
			Bounds2 bounds;
			uint32_t subMesh = 0;
		};

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
			part.vertices.reserve(part.vertices.size() + mesh.vertices.size());
			for (const OverlayVertex &vertex : mesh.vertices)
			{
				const Vec3 normal(vertex.nx, vertex.ny, vertex.nz);
				const Vec3 tangent = std::abs(vertex.ny) > 0.9f ? Vec3(1.0f, 0.0f, 0.0f) : xMath::Normalize(xMath::Cross(Vec3(0.0f, 1.0f, 0.0f), normal));
				part.vertices.push_back(Vertex_PosTexNorTan(Vec3(vertex.x, vertex.y, vertex.z), Vec2(vertex.u, vertex.v), normal, tangent));
			}
			part.indices.reserve(part.indices.size() + mesh.indices.size());
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

		uint64_t root = 0;
		Ref<Mesh> mesh;
		std::map<std::string, Ref<MaterialAsset>> materials;		// By texture + alpha treatment (or color)
		std::map<std::string, TextureEntry> textures;				// By file
		std::unordered_map<std::string, uint64_t> entities;		// Group key -> its entity
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
			if (image)
				material->SetTexture(MaterialTextureType::Color, image);
			materials.emplace(key, material);
			return material;
		}

		/* @brief Removes everything below the root (groups take their parts and objects with them). */
		void DestroyChildren()
		{
			entt::registry *registry = GetRegistry();
			const entt::entity rootHandle = registry ? FindHandle(*registry, root) : entt::null;
			if (rootHandle == entt::null)
				return;
			const std::vector<UUID> children = registry->get<RelationshipComponent>(rootHandle).pChildren;
			for (const UUID &child : children)
			{
				if (const Entity entity = Scene::TryGetEntityWithUUID(child))
					Scene::DestroyEntity(entity);
			}
			registry->get<RelationshipComponent>(rootHandle).pChildren.clear();
			Scene::MarkStructureChanged();
		}

		/* @brief Drops the GPU resources and the bookkeeping (after the entities using them are gone). */
		void Release()
		{
			mesh.Reset();
			materials.clear();
			textures.clear();
			entities.clear();
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
		Impl &impl = *m_Impl;
		const auto start = std::chrono::steady_clock::now();
		impl.DestroyChildren();
		impl.Release();

		entt::registry *registry = GetRegistry();
		if (!registry || groups.empty())
		{
			Clear();
			return 0;
		}

		entt::entity rootHandle = FindHandle(*registry, impl.root);
		if (rootHandle == entt::null)
		{
			const Entity root = Scene::CreateEntity(rootName);
			rootHandle = static_cast<entt::entity>(root);
			if (!registry->valid(rootHandle))
			{
				impl.root = 0;
				return 0;
			}
			impl.root = GetId(*registry, rootHandle);
		}

		// Each group's meshes, merged per material
		std::vector<std::vector<Part>> parts(groups.size());
		for (size_t g = 0; g < groups.size(); ++g)
		{
			std::map<std::string, size_t> byMaterial;
			for (const OverlayMesh *mesh : groups[g].meshes)
			{
				if (!mesh || mesh->indices.empty())
					continue;
				const auto [it, added] = byMaterial.emplace(MaterialKey(*mesh), parts[g].size());
				if (added)
				{
					Part &part = parts[g].emplace_back();
					part.name = mesh->name;
					part.kind = mesh->kind;
					part.texture = mesh->texture;
					std::copy(std::begin(mesh->color), std::end(mesh->color), std::begin(part.color));
					part.cutout = mesh->cutout;
					part.cutoff = mesh->cutoff;
					part.blended = mesh->blended;
				}
				Append(parts[g][it->second], *mesh);
			}
		}

		// One mesh, a sub-mesh per part
		for (std::vector<Part> &groupParts : parts)
		{
			for (Part &part : groupParts)
			{
				if (!impl.mesh)
				{
					impl.mesh = CreateRef<Mesh>();
					impl.mesh->SetFlags(0);	// Keep the local meters as they are: no normalization or generated LODs
					impl.mesh->SetResourceName(rootName);
				}
				impl.mesh->AddGeometry(part.vertices, part.indices, false, &part.subMesh);
				impl.stats.triangles += static_cast<uint32_t>(part.indices.size() / 3);
				impl.bounds.Add(part.bounds);
				++impl.stats.parts;
			}
		}
		if (impl.mesh)
			impl.mesh->CreateGpuBuffers();

		const auto addRenderable = [&impl](const entt::entity handle, const Part &part)
		{
			Entity *stored = Scene::GetStoredEntity(handle);
			Renderable *renderable = stored ? stored->AddComponent<Renderable>() : nullptr;
			if (!renderable)
				return;
			bool missingTexture = false;
			renderable->SetMesh(impl.mesh.Get(), part.subMesh);
			renderable->SetMaterial(impl.GetMaterial(part, missingTexture));
			renderable->SetFlag(CastsShadows, part.kind == OverlayKind::Facade || part.kind == OverlayKind::Forest);
			impl.stats.missingTextures += missingTexture ? 1 : 0;
		};

		uint32_t failed = 0;
		std::string firstError;
		for (size_t g = 0; g < groups.size(); ++g)
		{
			const Group &group = groups[g];
			const entt::entity handle = static_cast<entt::entity>(Scene::CreateEntity(group.name.empty() ? std::string("Overlay") : group.name));
			if (!registry->valid(handle))
				continue;
			Attach(*registry, handle, rootHandle);
			const uint64_t id = GetId(*registry, handle);
			impl.entities[group.key] = id;
			impl.keys[id] = group.key;
			++impl.stats.groups;

			// One part draws on the group's entity; several get an entity each below it
			if (parts[g].size() == 1)
				addRenderable(handle, parts[g].front());
			else
			{
				for (const Part &part : parts[g])
				{
					const entt::entity partHandle = static_cast<entt::entity>(Scene::CreateEntity(part.name));
					if (!registry->valid(partHandle))
						continue;
					Attach(*registry, partHandle, handle);
					impl.keys[GetId(*registry, partHandle)] = group.key;
					addRenderable(partHandle, part);
				}
			}

			for (const OverlayObject *object : group.objects)
			{
				if (!object)
					continue;
				SceneObjects::PlaceOptions options;
				options.position[0] = static_cast<float>(object->x);
				options.position[1] = static_cast<float>(object->y);
				options.position[2] = static_cast<float>(object->z);
				options.headingDegrees = object->heading;
				options.virtualPath = object->resource.starts_with("lib/") ? object->resource : std::string();
				options.name = object->name;
				options.frameCamera = false;
				std::string error;
				const uint64_t objectId = SceneObjects::Place(object->file, options, &error);
				const entt::entity objectHandle = FindHandle(*registry, objectId);
				if (objectHandle == entt::null)
				{
					if (failed++ == 0)
						firstError = error;
					continue;
				}
				Attach(*registry, objectHandle, handle);
				impl.keys[objectId] = group.key;
				++impl.stats.objects;
			}
		}

		impl.stats.buildMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		SEDX_CORE_INFO_TAG(LOG_TAG, "{}: {} groups, {} parts, {} triangles, {} objects in {:.0f} ms{}", rootName, impl.stats.groups, impl.stats.parts, impl.stats.triangles,
			impl.stats.objects, impl.stats.buildMilliseconds, failed ? "; " + std::to_string(failed) + " objects could not be placed: " + firstError : std::string());
		return impl.root;
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
		const auto it = m_Impl->entities.find(std::string(key));
		return it != m_Impl->entities.end() ? it->second : 0;
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

// -------------------------------------------------------
