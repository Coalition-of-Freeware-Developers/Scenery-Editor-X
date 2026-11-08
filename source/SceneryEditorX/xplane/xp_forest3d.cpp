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
 * xp_forest3d.cpp
 * -------------------------------------------------------
 * Created: 25/10/2026
 * -------------------------------------------------------
 */
#include "xp_forest3d.h"
#include "xp_assets.h"
#include "xp_object.h"
#include "xp_texture_loader.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <unordered_map>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/buffer.h>
#include <SceneryEditorX/asset/asset_instance.h>
#include <SceneryEditorX/renderer/vulkan/enums.h>
#include <SceneryEditorX/scene/camera.h>
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

		constexpr const char *LOG_TAG = "XPlaneForest3d";
		constexpr float SWITCH_DISTANCE = 400.0f;	// Meters: 3D trees nearer, billboards farther (the shipped meshes' LODs reach 500 m)
		constexpr float SWITCH_REACH = 1.05f;		// The dithered hand-over runs half a tenth of the distance past it
		constexpr float REFILL_STEP = 32.0f;		// Meters the camera moves before the trees in reach are picked again
		constexpr float CROWN_MARGIN = 20.0f;		// Meters a crown reaches nearer the camera than its trunk
		constexpr float BUCKET_SIZE = 64.0f;		// Meters: the grid trees are looked up in
		constexpr uint64_t BUFFER_GRACE_FRAMES = 8;	// Frames a replaced instance buffer outlives its last draw (frames in flight, render thread)
		constexpr float TWO_PI = 6.28318530718f;

		/* @brief A 3D tree mesh, uploaded once for every forest that names the same art. */
		struct MeshEntry
		{
			std::string name;
			Ref<Mesh> mesh;
			float height = 1.0f;			// Top of the mesh: the meters an unscaled tree measures
			float radius = 1.0f;			// Reach from the trunk
			bool castsShadow = true;
		};

		/* @brief A forest's 3D art: per TREE row the mesh drawn near the camera, and the SHADER_3D material. */
		struct ForestEntry
		{
			struct Kind
			{
				MeshEntry *mesh = nullptr;
				float referenceHeight = 1.0f;	// TREE2's nominal height, else the mesh's own
			};

			bool everyTree = false;			// Every tree has a mesh (DrawsEveryTree())
			std::vector<Kind> kinds;		// Per ForestAsset::trees
			Ref<MaterialAsset> material;
		};

		/**
		 * @brief A forest's trees planted below an entity. Each tree kind draws from one entity below it: its trees within
		 * reach of the camera as instances of the kind's mesh, picked again as the camera moves (Refill()).
		 */
		struct Planting
		{
			uint64_t parent = 0;
			std::string name;
			const ForestEntry *forest = nullptr;
			std::vector<OverlayTree3d> trees;								// In the parent's frame
			std::unordered_map<uint64_t, std::vector<uint32_t>> buckets;	// BUCKET_SIZE grid cell -> trees
			std::vector<uint64_t> kindEntities;								// Per tree kind, 0 until it has trees in reach
			Vec3 origin = Vec3(0.0f, 0.0f, 0.0f);							// The parent's place in the world, once known
			bool originKnown = false;
			Vec3 filledAt = Vec3(0.0f, 0.0f, 0.0f);							// The camera (parent frame) the instances were picked for
			bool filled = false;
			uint32_t drawn = 0;
		};

		std::map<std::string, std::unique_ptr<MeshEntry>> s_Meshes;	// By folder, name and size
		std::map<std::string, Ref<MaterialAsset>> s_Materials;		// By textures and options
		std::map<std::string, ForestEntry> s_Forests;				// By .for file
		std::vector<Ref<ImageResource>> s_Images;					// What the materials sample (they keep raw pointers)
		std::vector<std::unique_ptr<Planting>> s_Plantings;
		std::vector<std::pair<uint64_t, Ref<Buffer>>> s_RetiredBuffers;	// Replaced instance buffers and the frame they went
		uint64_t s_Frame = 0;
		uint32_t s_DrawnTrees = 0;

		uint64_t BucketKey(const int x, const int z)
		{
			return static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32 | static_cast<uint32_t>(z);
		}

		int BucketOf(const float v)
		{
			return static_cast<int>(std::floor(v / BUCKET_SIZE));
		}

		entt::registry *GetRegistry()
		{
			Scene *scene = Scene::GetActive();
			return scene ? &scene->GetRegistry() : nullptr;
		}

		void Attach(entt::registry &registry, const entt::entity child, const entt::entity parent)
		{
			const UUID parentId = registry.get<IDComponent>(parent).pID;
			const UUID childId = registry.get<IDComponent>(child).pID;
			registry.get<RelationshipComponent>(child).pParentHandle = parentId;
			registry.get<RelationshipComponent>(parent).pChildren.push_back(childId);
			Scene::MarkStructureChanged();
		}

		Ref<ImageResource> LoadTexture(const fs::path &forestFile, const std::string &reference, const TextureUsage usage)
		{
			if (reference.empty())
				return nullptr;
			const fs::path file = TextureLoader::ResolveTexturePath(forestFile.parent_path(), reference);
			TextureInfo info;
			Ref<ImageResource> image = file.empty() ? nullptr : TextureLoader::Load(file, usage, &info);
			if (!image)
				SEDX_CORE_WARN_TAG(LOG_TAG, "{}: texture not found or unreadable: {}{}", forestFile.filename().string(), reference, info.error.empty() ? std::string() : " (" + info.error + ")");
			else
				s_Images.push_back(image);
			return image;
		}

		/* @brief SHADER_3D as an X-Plane material (as an OBJ8's with NORMAL_TRANSLUCENCY / NORMAL_METALNESS), drawn near the camera. */
		Ref<MaterialAsset> GetMaterial(const fs::path &forestFile, const ForestAsset::Material &shader)
		{
			const std::string key = (forestFile.parent_path() / shader.texture).generic_string() + "|" + shader.textureNormal + "|" + shader.weather + "|" + std::to_string(shader.normalRatio) +
				(shader.normalMetalness ? "|m" : "") + (shader.normalTranslucency ? "|t" : "") + "|" + std::to_string(shader.blendHash) + "|" + std::to_string(shader.superRoughness);
			if (const auto it = s_Materials.find(key); it != s_Materials.end())
				return it->second;

			const Ref<ImageResource> albedo = LoadTexture(forestFile, shader.texture, TextureUsage::Albedo);
			const Ref<ImageResource> normal = LoadTexture(forestFile, shader.textureNormal, TextureUsage::Normal);
			const Ref<ImageResource> weather = LoadTexture(forestFile, shader.weather, TextureUsage::Data);

			Ref<MaterialAsset> material = CreateRef<MaterialAsset>();
			material->SetName("Forest 3D " + fs::path(shader.texture).stem().string());
			material->SetProperty(MaterialClass::ColorR, albedo ? 1.0f : 0.3f);
			material->SetProperty(MaterialClass::ColorG, albedo ? 1.0f : 0.4f);
			material->SetProperty(MaterialClass::ColorB, albedo ? 1.0f : 0.2f);
			material->SetProperty(MaterialClass::ColorA, 1.0f);
			material->SetProperty(MaterialClass::TextureTilingX, 1.0f);
			material->SetProperty(MaterialClass::TextureTilingY, 1.0f);
			material->SetProperty(MaterialProperty::XpMaterial, 1.0f);
			material->SetProperty(MaterialProperty::XpNormalMode, normal ? (shader.normalMetalness ? 2.0f : (shader.normalTranslucency ? 3.0f : 1.0f)) : 0.0f);
			material->SetProperty(MaterialProperty::XpSpecularGain, normal ? 1.0f : 0.0f);
			material->SetProperty(MaterialProperty::XpSuperRoughness, std::clamp(shader.superRoughness, 0.0f, 1.0f));
			material->SetProperty(MaterialProperty::XpDefaultF0, shader.normalTranslucency || !normal ? 0.0f : 0.04f);
			material->SetProperty(MaterialProperty::XpTerrainNormalRatio, shader.normalRatio > 0.0f ? shader.normalRatio : 1.0f);
			material->SetProperty(MaterialClass::Roughness, 1.0f);
			material->SetProperty(MaterialClass::Metalness, 0.0f);
			material->SetProperty(MaterialClass::Normal, normal ? 1.0f : 0.0f);
			material->SetProperty(MaterialProperty::CullMode, static_cast<float>(CullMode::None));	// Leaves show both faces
			// BLEND_HASH: X-Plane dithers alpha around the cutoff; a plain cutout at it here
			material->SetProperty(MaterialProperty::AlphaCutoff, shader.blendHash > 0.0f ? std::clamp(shader.blendHash, 0.001f, 1.0f) : 0.5f);
			material->SetProperty(MaterialProperty::BlendMode, 0.0f);
			material->SetProperty(MaterialProperty::XpLodSwitchMode, 2.0f);
			material->SetProperty(MaterialProperty::XpLodSwitchDistance, SWITCH_DISTANCE);
			material->SetTexture(MaterialTextureType::Color, albedo.Get());
			material->SetTexture(MaterialTextureType::Normal, normal.Get());
			material->SetTexture(MaterialTextureType::Normal, TextureLoader::GetNormalCompanion(normal.Get()).Get(), NORMAL_COMPANION_SLOT);
			material->SetTexture(MaterialTextureType::Emission, weather.Get(), 3);	// WEATHER mask (weather.slang)
			s_Materials.emplace(key, material);
			return material;
		}

		/* @brief A MESH as GPU geometry: x east, y up, z south like OBJ8 (re-wound for the renderer's front faces), UVs as written. */
		MeshEntry *GetMesh(const fs::path &forestFile, const ForestAsset::Mesh &source)
		{
			const size_t vertexCount = source.vertices.size() / 8;
			const std::string key = forestFile.parent_path().generic_string() + "|" + source.name + "|" + std::to_string(vertexCount) + "|" + std::to_string(source.indices.size());
			if (const auto it = s_Meshes.find(key); it != s_Meshes.end())
				return it->second.get();

			auto entry = std::make_unique<MeshEntry>();
			entry->name = source.name;
			std::vector<Vertex_PosTexNorTan> vertices(vertexCount);
			float top = 0.0f, reach = 0.0f;
			for (size_t i = 0; i < vertexCount; ++i)
			{
				const float *v = &source.vertices[i * 8];
				Vertex_PosTexNorTan &out = vertices[i];
				const float length = std::sqrt(v[3] * v[3] + v[4] * v[4] + v[5] * v[5]);
				for (int c = 0; c < 3; ++c)
				{
					out.pos[c] = v[c];
					out.nor[c] = length > 1e-8f ? v[3 + c] / length : (c == 1 ? 1.0f : 0.0f);
				}
				out.tex[0] = v[6];
				out.tex[1] = v[7];
				top = std::max(top, v[1]);
				reach = std::max(reach, std::sqrt(v[0] * v[0] + v[2] * v[2]));
			}
			std::vector<uint32_t> indices;
			indices.reserve(source.indices.size());
			for (size_t i = 0; i + 2 < source.indices.size(); i += 3)
			{
				const uint32_t a = source.indices[i], b = source.indices[i + 1], c = source.indices[i + 2];
				if (a >= vertexCount || b >= vertexCount || c >= vertexCount)
					continue;
				indices.insert(indices.end(), { a, c, b });	// OBJ8 winding is clockwise
			}

			if (!indices.empty())
			{
				ComputeVertexTangents(vertices, indices);
				entry->mesh = CreateRef<Mesh>();
				entry->mesh->SetFlags(0);	// Meters as written: no normalization or generated LODs
				entry->mesh->SetResourceName(source.name);
				entry->mesh->AddGeometry(vertices, indices, false);
				entry->mesh->CreateGpuBuffers();
			}
			entry->height = std::max(top, 0.1f);
			entry->radius = std::max(reach, 0.1f);
			entry->castsShadow = !source.noShadow;
			return s_Meshes.emplace(key, std::move(entry)).first->second.get();
		}

		/* @brief The forest's 3D art, loaded on first use. */
		ForestEntry &GetForest(const fs::path &forestFile)
		{
			const std::string key = forestFile.generic_string();
			if (const auto it = s_Forests.find(key); it != s_Forests.end())
				return it->second;

			ForestEntry &entry = s_Forests[key];
			ForestAsset asset;
			std::string error;
			if (!LoadForestAsset(forestFile, asset, &error))
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "{}: {}", forestFile.filename().string(), error);
				return entry;
			}
			bool every = !asset.trees.empty() && !asset.material3d.texture.empty();
			entry.kinds.resize(asset.trees.size());
			for (size_t i = 0; i < asset.trees.size() && every; ++i)
			{
				const ForestAsset::Tree &tree = asset.trees[i];
				const int first = tree.meshes3d.empty() ? -1 : tree.meshes3d.front();	// The nearest step (the next, lod_*, are lighter copies)
				if (first < 0 || first >= static_cast<int>(asset.meshes.size()))
				{
					every = false;
					break;
				}
				MeshEntry *mesh = GetMesh(forestFile, asset.meshes[static_cast<size_t>(first)]);
				if (!mesh->mesh)
				{
					every = false;
					break;
				}
				entry.kinds[i].mesh = mesh;
				entry.kinds[i].referenceHeight = tree.nominalHeight > 0.0f ? tree.nominalHeight : mesh->height;
			}
			if (every)
				entry.material = GetMaterial(forestFile, asset.material3d);
			entry.everyTree = every && entry.material;
			SEDX_CORE_INFO_TAG(LOG_TAG, "{}: {} tree kinds, {}", forestFile.string(), asset.trees.size(), entry.everyTree ? "3D near the camera" : "billboards only (not every tree has a 3D mesh)");
			return entry;
		}

		/* @brief A tree as an instance: offset from its entity, turned about y, scaled from its mesh's height to its own. */
		Instance PackTree(const Vec3 &offset, const float yaw, const float scale)
		{
			Instance instance;
			instance.positionX = Instance::FloatToHalf(offset.x);
			instance.positionY = Instance::FloatToHalf(offset.y);
			instance.positionZ = Instance::FloatToHalf(offset.z);
			instance.normal_Oct = Instance::EncodeOctahedral(Vec3(0.0f, 1.0f, 0.0f));
			float turns = yaw / TWO_PI;
			turns -= std::floor(turns);
			instance.yaw_Packed = static_cast<uint8_t>(std::clamp(std::lround(turns * 255.0f), 0L, 255L));
			// Scale is packed logarithmically from 0.01 to 100 (Instance::GetMatrix)
			const float t = (std::log(std::clamp(scale, 0.01f, 100.0f)) - std::log(0.01f)) / (std::log(100.0f) - std::log(0.01f));
			instance.scale_Packed = static_cast<uint8_t>(std::clamp(std::lround(t * 255.0f), 0L, 255L));
			return instance;
		}
	}

	// -------------------------------------------------------

	float Forest3d::GetSwitchDistance()
	{
		return SWITCH_DISTANCE;
	}

	bool Forest3d::DrawsEveryTree(const std::filesystem::path &forestFile)
	{
		return !forestFile.empty() && Renderer::GetFeatures().trees3d && GetForest(forestFile).everyTree;
	}

	uint32_t Forest3d::Plant(entt::registry &registry, const entt::entity parent, const std::filesystem::path &forestFile, const std::vector<OverlayTree3d> &trees,
		const std::string &name)
	{
		if (trees.empty() || !registry.valid(parent) || !DrawsEveryTree(forestFile))
			return 0;
		const ForestEntry &forest = GetForest(forestFile);

		auto planting = std::make_unique<Planting>();
		planting->parent = static_cast<uint64_t>(registry.get<IDComponent>(parent).pID);
		planting->name = name;
		planting->forest = &forest;
		planting->kindEntities.assign(forest.kinds.size(), 0);
		planting->trees.reserve(trees.size());
		for (const OverlayTree3d &tree : trees)
		{
			if (tree.tree < 0 || tree.tree >= static_cast<int>(forest.kinds.size()) || tree.height <= 0.0f)
				continue;
			planting->buckets[BucketKey(BucketOf(tree.x), BucketOf(tree.z))].push_back(static_cast<uint32_t>(planting->trees.size()));
			planting->trees.push_back(tree);
		}
		const uint32_t planted = static_cast<uint32_t>(planting->trees.size());
		if (planted > 0)
			s_Plantings.push_back(std::move(planting));
		return planted;
	}

	namespace
	{
		/* @brief Gives each tree kind of a planting its trees within reach of the camera (local: in the parent's frame). */
		void Refill(entt::registry &registry, Planting &planting, const entt::entity parent, const Vec3 &local)
		{
			// Instances count from a point near the camera, so their half-float offsets stay small
			const Vec3 anchor(std::round(local.x / REFILL_STEP) * REFILL_STEP, 0.0f, std::round(local.z / REFILL_STEP) * REFILL_STEP);
			const float reach = SWITCH_DISTANCE * SWITCH_REACH + REFILL_STEP + CROWN_MARGIN;	// Trees the camera may come near before the next refill

			std::vector<std::vector<Instance>> instances(planting.kindEntities.size());
			for (int bx = BucketOf(local.x - reach); bx <= BucketOf(local.x + reach); ++bx)
			{
				for (int bz = BucketOf(local.z - reach); bz <= BucketOf(local.z + reach); ++bz)
				{
					const auto bucket = planting.buckets.find(BucketKey(bx, bz));
					if (bucket == planting.buckets.end())
						continue;
					for (const uint32_t index : bucket->second)
					{
						const OverlayTree3d &tree = planting.trees[index];
						const Vec3 delta(tree.x - local.x, tree.y - local.y, tree.z - local.z);
						if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z > reach * reach)
							continue;
						const ForestEntry::Kind &kind = planting.forest->kinds[static_cast<size_t>(tree.tree)];
						instances[static_cast<size_t>(tree.tree)].push_back(PackTree(Vec3(tree.x - anchor.x, tree.y - anchor.y, tree.z - anchor.z), tree.yaw,
							tree.height / std::max(kind.referenceHeight, 0.1f)));
					}
				}
			}

			planting.drawn = 0;
			for (size_t k = 0; k < instances.size(); ++k)
			{
				const Entity found = planting.kindEntities[k] ? Scene::TryGetEntityWithUUID(UUID(planting.kindEntities[k])) : Entity();
				Entity *stored = found ? Scene::GetStoredEntity(static_cast<entt::entity>(found)) : nullptr;
				if (instances[k].empty())
				{
					if (stored && stored->GetActive())
						stored->SetActive(false);
					continue;
				}
				const ForestEntry::Kind &kind = planting.forest->kinds[k];
				entt::entity handle = found ? static_cast<entt::entity>(found) : entt::null;
				if (!stored)
				{
					// The kind's first trees in reach: its entity
					handle = static_cast<entt::entity>(Scene::CreateEntity(planting.name + " " + kind.mesh->name));
					if (!registry.valid(handle))
						continue;
					Attach(registry, handle, parent);
					stored = Scene::GetStoredEntity(handle);
					Renderable *renderable = stored ? stored->AddComponent<Renderable>() : nullptr;
					if (!renderable)
						continue;
					renderable->SetMesh(kind.mesh->mesh.Get());
					renderable->SetMaterial(planting.forest->material);
					renderable->SetFlag(CastsShadows, kind.mesh->castsShadow);
					planting.kindEntities[k] = static_cast<uint64_t>(registry.get<IDComponent>(handle).pID);
				}
				Renderable *renderable = stored->GetComponent<Renderable>();
				if (!renderable)
					continue;
				registry.get<TransformComponent>(handle).translation = anchor;
				// Frames still in flight may draw the old instances: their buffer goes a few frames later
				if (Buffer *old = renderable->GetInstanceBuffer())
					s_RetiredBuffers.emplace_back(s_Frame, Ref<Buffer>(old));
				renderable->SetInstances(instances[k]);
				if (!stored->GetActive())
					stored->SetActive(true);
				planting.drawn += static_cast<uint32_t>(instances[k].size());
			}
		}
	}

	void Forest3d::Update()
	{
		++s_Frame;
		std::erase_if(s_RetiredBuffers, [](std::pair<uint64_t, Ref<Buffer>> &retired)
		{
			if (s_Frame - retired.first < BUFFER_GRACE_FRAMES)
				return false;
			retired.second->Destroy();
			return true;
		});

		Camera *camera = Scene::GetCamera();
		entt::registry *registry = GetRegistry();
		if (s_Plantings.empty() || !camera || !registry)
			return;
		const Vec3 eye = camera->GetEyePosition();

		s_DrawnTrees = 0;
		for (auto it = s_Plantings.begin(); it != s_Plantings.end();)
		{
			Planting &planting = **it;
			const Entity parent = Scene::TryGetEntityWithUUID(UUID(planting.parent));
			Entity *stored = parent ? Scene::GetStoredEntity(static_cast<entt::entity>(parent)) : nullptr;
			if (!stored)
			{
				it = s_Plantings.erase(it);	// Its forest was rebuilt or removed: the kinds' entities went with it
				continue;
			}
			if (!planting.originKnown)
			{
				planting.origin = stored->GetMatrix().GetTranslation();
				planting.originKnown = true;
			}
			const Vec3 local = eye - planting.origin;
			const Vec3 moved = local - planting.filledAt;
			if (!planting.filled || moved.x * moved.x + moved.y * moved.y + moved.z * moved.z >= REFILL_STEP * REFILL_STEP)
			{
				Refill(*registry, planting, static_cast<entt::entity>(parent), local);
				planting.filledAt = local;
				planting.filled = true;
			}
			s_DrawnTrees += planting.drawn;
			++it;
		}
	}

	uint32_t Forest3d::GetTreeCount()
	{
		uint32_t trees = 0;
		for (const std::unique_ptr<Planting> &planting : s_Plantings)
			trees += static_cast<uint32_t>(planting->trees.size());
		return trees;
	}

	uint32_t Forest3d::GetDrawnTreeCount()
	{
		return s_DrawnTrees;
	}

	void Forest3d::Shutdown()
	{
		if (GetRegistry())
		{
			for (const std::unique_ptr<Planting> &planting : s_Plantings)
			{
				for (const uint64_t id : planting->kindEntities)
				{
					if (const Entity entity = id ? Scene::TryGetEntityWithUUID(UUID(id)) : Entity())
						Scene::DestroyEntity(entity);
				}
			}
		}
		s_Plantings.clear();
		for (auto &[frame, buffer] : s_RetiredBuffers)
			buffer->Destroy();
		s_RetiredBuffers.clear();
		s_DrawnTrees = 0;
		s_Forests.clear();
		s_Materials.clear();
		s_Meshes.clear();
		s_Images.clear();
	}
}

// -------------------------------------------------------
