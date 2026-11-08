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
 * xp_scene.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xp_scene.h"
#include "xp_airport_scene.h"
#include "xp_datarefs.h"
#include "xp_forest3d.h"
#include "xp_library_manager.h"
#include "xp_weather_art.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <SceneryEditorX/asset/standards.h>
#include <SceneryEditorX/core/frame_profiler.h>
#include <SceneryEditorX/environment/environment.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/gpu_scene.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/scene/components/component_sets.h>
#include <SceneryEditorX/scene/components/lights.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr const char *LOG_TAG = "XPlaneScene";
		constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
		// An OBJ without ATTR_LOD draws to a distance X-Plane picks from its bounding sphere ("where nobody would notice it
		// go"; the formula isn't published). ASSUMPTION: until it covers about 2 pixels at 1080 lines and 60 degrees, which is
		// some 500 radii: a car goes at about 1 km, a 10 m hangar at 5 km.
		constexpr float DEFAULT_LOD_RADII = 500.0f;

		/* @brief Spill lights that light the scene at once, nearest first (the light pass loops over every light per pixel). */
		constexpr uint32_t MAX_SPILL_LIGHTS = 48;
		/* @brief Luminous efficacy the renderer's radiometric light units assume (Light::GetIntensityWatt): lm per W. */
		constexpr float LUMENS_PER_WATT = 683.0f;
		constexpr float LUMENS_PER_WATT_STERADIAN = 683.0f * 4.0f * 3.14159265358979323846f; // inverse of Light::GetIntensityWatt()
		/* @brief Lights dimmer than this (after direction, flashing and night) are not drawn. */
		constexpr float MIN_LIGHT_BRIGHTNESS = 0.002f;

		/* @brief Placement to frame once its asset has loaded (the latest FrameObject() request wins), or 0. */
		uint64_t s_FrameWhenLoaded = 0;

		/* @brief Draped OBJ batches in the order they were built: within a layer, later ones draw on top. */
		uint32_t s_DrapedBatchSequence = 0;

		/* @brief Roots placed while their asset loads: BuildLoadedObjects() visits these instead of every placement. */
		std::vector<uint64_t> s_PendingRoots;

		/* @brief Bumped when the camera moves: placements whose LODs depend on distance are evaluated again. */
		uint64_t s_CameraStamp = 1;
		Vec3 s_LastCameraPosition(0.0f, 0.0f, 0.0f);
		bool s_LastHasCamera = false;
		uint64_t s_TickCount = 0;

		/**
		 * @brief While the camera moves, a placement's LODs are re-evaluated every LOD_PHASES frames (each frame takes a
		 * different share); the frame it stops, all of them. LOD bands span hundreds of meters, so the delay is invisible.
		 */
		constexpr uint32_t LOD_PHASES = 4;

		entt::registry *GetRegistry()
		{
			Scene *scene = Scene::GetActive();
			return scene ? &scene->GetRegistry() : nullptr;
		}

		uint64_t GetId(entt::registry &registry, const entt::entity handle)
		{
			const IDComponent *id = registry.try_get<IDComponent>(handle);
			return id ? static_cast<uint64_t>(id->pID) : 0;
		}

		entt::entity FindHandle(entt::registry &registry, const uint64_t id)
		{
			if (id == 0)
				return entt::null;

			const entt::entity handle = Scene::TryGetEntityHandle(UUID(id));
			return handle != entt::null && registry.valid(handle) ? handle : entt::null;
		}

		/* @brief Fills the component's entity pointers from its UUIDs when an entity was destroyed since the last time. */
		void ResolveEntities(entt::registry &registry, const entt::entity rootHandle, XPlaneObjectComponent &object)
		{
			const uint64_t version = Scene::GetDestroyVersion();
			if (object.resolvedDestroyVersion == version && object.nodeStored.size() == object.nodeEntities.size() &&
				object.batchStored.size() == object.batchEntities.size() && object.spillStored.size() == object.spillEntities.size())
				return;

			object.rootStored = Scene::GetStoredEntity(rootHandle);

			object.nodeHandles.resize(object.nodeEntities.size());
			object.nodeStored.resize(object.nodeEntities.size());
			for (size_t i = 0; i < object.nodeEntities.size(); ++i)
			{
				object.nodeHandles[i] = FindHandle(registry, object.nodeEntities[i]);
				object.nodeStored[i] = Scene::GetStoredEntity(object.nodeHandles[i]);
			}

			object.batchStored.resize(object.batchEntities.size());
			for (size_t i = 0; i < object.batchEntities.size(); ++i)
				object.batchStored[i] = Scene::GetStoredEntity(FindHandle(registry, object.batchEntities[i]));

			object.spillStored.resize(object.spillEntities.size());
			for (size_t i = 0; i < object.spillEntities.size(); ++i)
				object.spillStored[i] = Scene::GetStoredEntity(FindHandle(registry, object.spillEntities[i]));

			object.resolvedDestroyVersion = version;
		}

		/* @brief Links child under parent in both RelationshipComponents. */
		void Attach(entt::registry &registry, const entt::entity child, const entt::entity parent)
		{
			const UUID parentId = registry.get<IDComponent>(parent).pID;
			const UUID childId = registry.get<IDComponent>(child).pID;
			registry.get<RelationshipComponent>(child).pParentHandle = parentId;
			registry.get<RelationshipComponent>(parent).pChildren.push_back(childId);
			Scene::MarkStructureChanged(); // hierarchy caches (outliner roots, ...)
		}

		/* @brief Leaf name of a dataref for entity labels ("sim/graphics/animation/windsock_psi" -> "windsock_psi"). */
		std::string ShortDatarefName(const DatarefRef &ref)
		{
			const size_t slash = ref.name.rfind('/');
			std::string name = slash == std::string::npos ? ref.name : ref.name.substr(slash + 1);
			if (ref.index >= 0)
				name += "[" + std::to_string(ref.index) + "]";
			return name;
		}

		/* @brief World-space position of the primary camera (the one the renderer draws through). */
		bool GetCameraPosition(entt::registry &registry, Vec3 &position)
		{
			bool found = false;
			auto view = registry.view<TransformComponent, CameraComponent>();
			for (const entt::entity handle : view)
			{
				const CameraComponent &camera = view.get<CameraComponent>(handle);
				if (!found || camera.isPrimary)
				{
					position = view.get<TransformComponent>(handle).translation;
					found = true;
					if (camera.isPrimary)
						break;
				}
			}
			return found;
		}

		/* @brief Which LODs are drawn for a camera distance (X-Plane rules, or the component's override). */
		/**
		 * @brief Whether a batch or light of an LOD shows at a camera distance (from the object's middle, as X-Plane measures).
		 * @param defaultFar How far an object without ATTR_LOD draws (DEFAULT_LOD_RADII), infinity to always draw it.
		 */
		bool IsLodVisible(const Obj8Data &data, const int lod, const float distance, const int lodOverride, const float defaultFar = std::numeric_limits<float>::infinity())
		{
			if (lodOverride == -2)
				return true;
			if (lod < 0)
				return lodOverride >= 0 || !data.lods.empty() || distance < defaultFar;
			if (lodOverride >= 0)
				return lod == lodOverride;

			const Obj8Lod &range = data.lods[static_cast<size_t>(lod)];
			if (data.HasAdditiveLods())
				return distance < range.farDistance; // every LOD starts at 0; nearer ones add detail
			return distance >= range.nearDistance && distance < range.farDistance;
		}

		/* @brief p * M for a row-vector world matrix (rows hold the scaled axes, the last row the translation). */
		void TransformPoint(const xMath::Matrix &m, const float p[3], float out[3])
		{
			out[0] = p[0] * m.m00 + p[1] * m.m10 + p[2] * m.m20 + m.m30;
			out[1] = p[0] * m.m01 + p[1] * m.m11 + p[2] * m.m21 + m.m31;
			out[2] = p[0] * m.m02 + p[1] * m.m12 + p[2] * m.m22 + m.m32;
		}

		void TransformDirection(const xMath::Matrix &m, const float d[3], float out[3])
		{
			out[0] = d[0] * m.m00 + d[1] * m.m10 + d[2] * m.m20;
			out[1] = d[0] * m.m01 + d[1] * m.m11 + d[2] * m.m21;
			out[2] = d[0] * m.m02 + d[1] * m.m12 + d[2] * m.m22;
		}

		/* @brief Rotation taking +Z (a Light's forward) onto a direction; identity for a zero direction. */
		Quat RotationFromForward(const float direction[3])
		{
			const float length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
			if (length < 1e-6f)
				return Quat(1.0f, 0.0f, 0.0f, 0.0f);

			const float d[3] = { direction[0] / length, direction[1] / length, direction[2] / length };
			if (d[2] < -0.9999f)
				return Quat(0.0f, 0.0f, 1.0f, 0.0f); // half turn about +Y

			// q = normalize(cross(z, d), 1 + dot(z, d))
			const float x = -d[1], y = d[0], z = 0.0f, w = 1.0f + d[2];
			const float n = std::sqrt(x * x + y * y + z * z + w * w);
			return Quat(w / n, x / n, y / n, z / n);
		}

		float MaxComponent(const float color[4])
		{
			return std::max(color[0], std::max(color[1], color[2]));
		}

		/*
		 * Peak radiant intensity (W/sr) of a spill, before night, flashing and its alpha dimmer: candela over the luminous
		 * efficacy, lit and exposed physically (plan Phase 5). Legacy spills give a reach in meters instead: 2 cd per
		 * square meter of it (ASSUMPTION: about 2 lux at the edge before the window takes it to 0).
		 */
		float SpillRadiantIntensity(const LightSpillSpec &spill)
		{
			const float range = spill.GetRange();
			const float candela = spill.IsPhotometric() ? spill.intensity : 2.0f * range * range;
			return std::max(candela, 0.0f) / LUMENS_PER_WATT;
		}

		/* @brief A spill light that could be lit this frame. */
		struct SpillCandidate
		{
			Entity *entity = nullptr;
			Light *light = nullptr;
			float lumens = 0.0f;
			float distanceSquared = 0.0f;
			XPlaneObjectComponent *object = nullptr;
		};

		/* @brief Everything one frame of light evaluation gathers over the placed objects. */
		struct LightFrame
		{
			Vec3 camera = Vec3(0.0f, 0.0f, 0.0f);
			bool hasCamera = false;
			float night = 0.0f;					// Datarefs::GetNightFactor(): lights only show at night or when forced on
			double time = 0.0;					// Sim clock (flashing)
			Ref<ImageResource> atlas;			// lights.txt texture
			float atlasInset[2] = { 0.0f, 0.0f };	// Half a texel: cells sit edge to edge
			Renderer_LightBillboardBatch atlasBatch;
			std::vector<Renderer_LightBillboardBatch> customBatches; // LIGHT_CUSTOM, one per object texture
			std::unordered_map<ImageResource *, size_t> customBatchIndex;
			std::vector<SpillCandidate> spills;
		};

		/* @brief Spreads sequenced lights (rabbits, wig-wags) by where they sit in the object. */
		float LightPhaseSeed(const ObjectLight &light)
		{
			return light.position[0] * 0.013f - light.position[2] * 0.004f;
		}

		/**
		 * @brief World matrix of the parent the last placement had. Placements sit under a few shared groups, so one
		 * entry saves walking the group / airport chain per object. Valid for one Tick(): nothing moves the groups
		 * while it runs.
		 */
		struct ParentWorldCache
		{
			entt::entity parent = entt::null;
			xMath::Matrix world = xMath::Matrix::IDENTITY;
		};

		/* @brief A placement root's world matrix: its local transform under the (cached) parent world matrix. */
		xMath::Matrix RootWorld(entt::registry &registry, const entt::entity rootHandle, Entity &root, ParentWorldCache &cache)
		{
			const TransformComponent *transform = registry.try_get<TransformComponent>(rootHandle);
			const RelationshipComponent *relationship = registry.try_get<RelationshipComponent>(rootHandle);
			if (!transform)
				return root.GetMatrix();

			const xMath::Matrix local = Entity::ComposeLocalMatrix(*transform);
			const entt::entity parent = relationship ? Scene::TryGetEntityHandle(relationship->pParentHandle) : entt::null;
			if (parent == entt::null)
				return local;

			if (cache.parent != parent)
			{
				Entity *stored = Scene::GetStoredEntity(parent);
				cache.world = stored ? stored->GetMatrix() : xMath::Matrix::IDENTITY;
				cache.parent = parent;
			}
			return local * cache.world;
		}

		/**
		 * @brief Evaluates one placed object's lights: billboards go into the frame's batches, spill becomes candidates
		 * for the nearest-first budget. Lights follow their animation group's world transform, LOD and hide / show rules.
		 */
		void CollectObjectLights(entt::registry &registry, const entt::entity rootHandle, XPlaneObjectComponent &object, LightFrame &frame,
			ParentWorldCache &parentCache)
		{
			object.visibleBillboards = 0;
			object.litSpills = 0;

			const ObjectAsset &asset = *object.asset;
			const Obj8Data &data = asset.GetData();
			const std::vector<ObjectLight> &lights = asset.GetLights();
			if (lights.empty())
				return;

			Entity *root = object.rootStored;
			if (!root)
				return;

			// Lights off (daytime, hidden object): the spills only need switching off once.
			const bool lightsOn = object.visible && frame.night > 0.0f && frame.hasCamera;
			if (!lightsOn)
			{
				if (!object.spillsDark)
				{
					for (Entity *stored : object.spillStored)
					{
						if (stored && stored->GetActive())
							stored->SetActive(false);
					}
					object.spillsDark = true;
				}
				return;
			}
			object.spillsDark = false;

			// World transforms of the root and of every animation group (scratch storage reused across objects)
			static std::vector<xMath::Matrix> nodeWorld;
			static std::vector<uint8_t> lightVisible;
			static std::vector<std::array<float, 3>> lightWorld;
			// Groups compose from their parents (parents precede children in Obj8Data::nodes), like the entity tree
			const xMath::Matrix rootWorld = RootWorld(registry, rootHandle, *root, parentCache);
			nodeWorld.assign(data.nodes.size(), rootWorld);
			for (size_t i = 0; i < object.nodeHandles.size() && i < nodeWorld.size(); ++i)
			{
				const entt::entity handle = object.nodeHandles[i];
				const TransformComponent *transform = handle != entt::null ? registry.try_get<TransformComponent>(handle) : nullptr;
				if (!transform)
					continue;

				const int parent = data.nodes[i].parent;
				const xMath::Matrix &parentWorld = parent >= 0 && static_cast<size_t>(parent) < i ? nodeWorld[static_cast<size_t>(parent)] : rootWorld;
				nodeWorld[i] = Entity::ComposeLocalMatrix(*transform) * parentWorld;
			}

			lightVisible.assign(lights.size(), 0);
			lightWorld.resize(lights.size());

			for (size_t i = 0; i < lights.size(); ++i)
			{
				const ObjectLight &light = lights[i];
				const xMath::Matrix &world = light.node >= 0 && static_cast<size_t>(light.node) < nodeWorld.size() ? nodeWorld[static_cast<size_t>(light.node)] : rootWorld;
				TransformPoint(world, light.position, lightWorld[i].data());

				lightVisible[i] = lightsOn && IsLodVisible(data, light.lod, object.cameraDistance, object.lodOverride) &&
								  EvaluateVisibilityAt(data, light.visibility, object.datarefValues);
				if (!lightVisible[i])
					continue;

				const float *position = lightWorld[i].data();
				float toCamera[3] = { frame.camera.x - position[0], frame.camera.y - position[1], frame.camera.z - position[2] };
				const float distance = std::sqrt(toCamera[0] * toCamera[0] + toCamera[1] * toCamera[1] + toCamera[2] * toCamera[2]);
				for (float &c : toCamera)
					c = distance > 1e-4f ? c / distance : 0.0f;

				for (const LightBillboardSpec &spec : light.resolved.billboards)
				{
					float direction[3];
					TransformDirection(world, spec.direction, direction);

					const float brightness = frame.night * std::max(spec.color[3], 0.0f) * EvaluateBillboardDirectionality(spec, direction, toCamera) *
											 EvaluateFlash(spec.frequency, spec.phase, spec.amplitude, frame.time) *
											 EvaluateProcessor(spec.processor, frame.time, LightPhaseSeed(light));
					if (brightness < MIN_LIGHT_BRIGHTNESS)
						continue;

					// Software lights may pass directions through the color slots: draw those white.
					float tint[3] = { std::max(spec.color[0], 0.0f), std::max(spec.color[1], 0.0f), std::max(spec.color[2], 0.0f) };
					if (tint[0] + tint[1] + tint[2] <= 0.0f)
						tint[0] = tint[1] = tint[2] = 1.0f;

					// VASI / PAPI: white above the glide slope, red below it
					if (spec.processor == LightProcessor::Vasi && std::asin(std::clamp(toCamera[1], -1.0f, 1.0f)) < 3.0f * kDegToRad)
					{
						tint[0] = 1.0f;
						tint[1] = 0.06f;
						tint[2] = 0.03f;
					}

					Renderer_LightBillboard billboard;
					std::copy_n(position, 3, billboard.position);
					billboard.radius = GetBillboardRadius(spec, distance);
					for (int c = 0; c < 3; ++c)
						billboard.color[c] = tint[c] * brightness;

					if (spec.customTexture)
					{
						ImageResource *texture = asset.GetTextures().albedo.Get();
						std::copy_n(spec.uv, 4, billboard.uv);
						billboard.useAlpha = true;

						auto [it, inserted] = frame.customBatchIndex.try_emplace(texture, frame.customBatches.size());
						if (inserted)
							frame.customBatches.push_back({ asset.GetTextures().albedo, {} });
						frame.customBatches[it->second].billboards.push_back(billboard);
					}
					else
					{
						if (!frame.atlas)
							continue;
						billboard.uv[0] = spec.uv[0] + frame.atlasInset[0];
						billboard.uv[1] = spec.uv[1] + frame.atlasInset[1];
						billboard.uv[2] = spec.uv[2] - frame.atlasInset[0];
						billboard.uv[3] = spec.uv[3] - frame.atlasInset[1];
						billboard.tintLuminance = spec.IsPhotometric(); // X-Plane 12 tints one white sprite with the light's color
						frame.atlasBatch.billboards.push_back(billboard);
					}
					++object.visibleBillboards;
				}
			}

			// Spill entities: dimmed here, switched on by the nearest-first budget in Tick()
			for (size_t k = 0; k < object.spillStored.size() && k < object.spillSources.size(); ++k)
			{
				Entity *stored = object.spillStored[k];
				Light *component = stored ? stored->GetComponent<Light>() : nullptr;
				if (!component)
					continue;

				const XPlaneObjectComponent::SpillSource source = object.spillSources[k];
				float lumens = 0.0f;
				if (source.light < lights.size() && lightVisible[source.light] && source.spill < lights[source.light].resolved.spills.size())
				{
					const ObjectLight &light = lights[source.light];
					const LightSpillSpec &spill = light.resolved.spills[source.spill];
					const float brightness = frame.night * std::max(spill.color[3], 0.0f) * EvaluateFlash(spill.frequency, spill.phase, spill.amplitude, frame.time) *
											 EvaluateProcessor(spill.processor, frame.time, LightPhaseSeed(light));
					lumens = SpillRadiantIntensity(spill) * MaxComponent(spill.color) * brightness * LUMENS_PER_WATT_STERADIAN;
				}

				if (lumens <= 0.0f)
				{
					if (stored->GetActive())
						stored->SetActive(false);
					continue;
				}

				const float *position = lightWorld[source.light].data();
				const float dx = position[0] - frame.camera.x, dy = position[1] - frame.camera.y, dz = position[2] - frame.camera.z;
				frame.spills.push_back({ stored, component, lumens, dx * dx + dy * dy + dz * dz, &object });
			}
		}

		/**
		 * @brief Builds the entity tree of a placement whose asset is ready: an entity per animation group, a Renderable
		 * per batch and a Light per spill, below the placement's root.
		 */
		void BuildObject(entt::registry &registry, const entt::entity rootHandle)
		{
			const XPlaneObjectComponent *object = registry.try_get<XPlaneObjectComponent>(rootHandle);
			if (!object || object->loaded || !object->asset || !object->asset->IsReady())
				return;

			const Ref<ObjectAsset> asset = object->asset;
			const Obj8Data &data = asset->GetData();
			std::vector<uint64_t> nodeEntities;
			std::vector<uint64_t> batchEntities;
			std::vector<uint64_t> spillEntities;
			std::vector<XPlaneObjectComponent::SpillSource> spillSources;

			// Animation groups (parents always precede children in Obj8Data::nodes)
			std::vector<entt::entity> nodeHandles(data.nodes.size(), entt::null);
			for (size_t i = 0; i < data.nodes.size(); ++i)
			{
				const Obj8AnimNode &node = data.nodes[i];
				std::string name = "Anim " + std::to_string(i);
				if (!node.ops.empty() && node.ops.front().dataref.IsSet())
					name += " (" + ShortDatarefName(node.ops.front().dataref) + ")";
				else if (!node.ops.empty())
					name += node.ops.front().type == Obj8AnimOp::Type::Rotate ? " (rotate)" : " (translate)";

				const Entity entity = Scene::CreateEntity(name);
				nodeHandles[i] = static_cast<entt::entity>(entity);
				const entt::entity parent = node.parent >= 0 ? nodeHandles[static_cast<size_t>(node.parent)] : rootHandle;
				Attach(registry, nodeHandles[i], parent);
				nodeEntities.push_back(GetId(registry, nodeHandles[i]));
			}

			// Batches: one Renderable each
			const std::vector<ObjectBatch> &batches = asset->GetBatches();
			for (size_t i = 0; i < batches.size(); ++i)
			{
				const ObjectBatch &batch = batches[i];
				std::string name = "Mesh " + std::to_string(i);
				if (batch.lod >= 0)
				{
					const Obj8Lod &lod = data.lods[static_cast<size_t>(batch.lod)];
					name += " [LOD " + std::to_string(static_cast<int>(lod.nearDistance)) + "-" + std::to_string(static_cast<int>(lod.farDistance)) + "m]";
				}
				if (batch.draped)
					name += " (draped)";

				const Entity entity = Scene::CreateEntity(name);
				const entt::entity handle = static_cast<entt::entity>(entity);
				const entt::entity parent = batch.node >= 0 ? nodeHandles[static_cast<size_t>(batch.node)] : rootHandle;
				Attach(registry, handle, parent);
				batchEntities.push_back(GetId(registry, handle));

				// ATTR_draped geometry (ground decals: grunge, sand patches, painted stops) is a drape like the .pol ones:
				// drawn in its ATTR_layer_group_draped layer's order over the pavement, lifted to that layer so it never
				// fights the ground, no shadow
				float drapeHeight = 0.0f;
				if (batch.draped)
				{
					drapeHeight = SceneAirports::GetOverlayLayerHeight(data.layerGroupDraped.name, data.layerGroupDraped.offset);
					registry.get<TransformComponent>(handle).translation.y += drapeHeight;
				}

				if (Entity *stored = Scene::GetStoredEntity(entity))
				{
					if (Renderable *renderable = stored->AddComponent<Renderable>())
					{
						renderable->SetMesh(asset->GetMesh(), batch.subMesh);
						renderable->SetMaterial(Ref<MaterialAsset>(asset->GetMaterial(batch.material)));
						renderable->SetFlag(CastsShadows, batch.castShadow && !batch.draped);
						if (batch.draped)
							renderable->SetDrapeOrder(Renderable::MakeDrapeOrder(drapeHeight, s_DrapedBatchSequence++));
					}
				}
			}

			// Spill lights: a Light component per spill, under the light's animation group so it moves with it. Tick()
			// switches them on at night and dims them (flashing, nearest-first budget).
			const std::vector<ObjectLight> &lights = asset->GetLights();
			for (uint32_t i = 0; i < static_cast<uint32_t>(lights.size()); ++i)
			{
				const ObjectLight &light = lights[i];
				const Obj8Light &source = data.lights[light.source];
				for (uint32_t s = 0; s < static_cast<uint32_t>(light.resolved.spills.size()); ++s)
				{
					const LightSpillSpec &spill = light.resolved.spills[s];
					const std::string label = source.name.empty() ? "custom" : source.name;
					const Entity entity = Scene::CreateEntity("Spill " + std::to_string(i) + " (" + label + ")");
					const entt::entity handle = static_cast<entt::entity>(entity);
					const entt::entity parent = light.node >= 0 ? nodeHandles[static_cast<size_t>(light.node)] : rootHandle;
					Attach(registry, handle, parent);

					TransformComponent &transform = registry.get<TransformComponent>(handle);
					transform.translation = Vec3(light.position[0], light.position[1], light.position[2]);
					transform.SetRotation(RotationFromForward(spill.direction));

					if (Entity *stored = Scene::GetStoredEntity(entity))
					{
						if (Light *lightComponent = stored->AddComponent<Light>())
						{
							// type first: it resets color and range to that type's defaults
							lightComponent->SetLightType(spill.IsSpot() ? LightType::Spot : LightType::Point);
							lightComponent->SetFlag(LightFlags::Shadows, false); // also clears screen-space shadows and volumetrics
							lightComponent->SetRange(spill.GetRange());
							if (spill.IsSpot())
								lightComponent->SetAngle(std::acos(std::clamp(spill.width, -1.0f, 1.0f)));

							const float peak = std::max(MaxComponent(spill.color), 1e-3f);
							lightComponent->SetLightColor(Standard(std::max(spill.color[0], 0.0f) / peak, std::max(spill.color[1], 0.0f) / peak,
								std::max(spill.color[2], 0.0f) / peak, 1.0f));
							lightComponent->SetIntensity(0.0f);
						}
						stored->SetActive(false);
					}

					spillEntities.push_back(GetId(registry, handle));
					spillSources.push_back({ i, s });
				}
			}

			// Expose the object's datarefs as editor controls.
			for (const Obj8DatarefUsage &usage : data.datarefs)
				Datarefs::Register(usage);

			// Entities were created above: look the component up again.
			XPlaneObjectComponent &component = registry.get<XPlaneObjectComponent>(rootHandle);
			component.dynamicState = !data.nodes.empty() ||
				std::ranges::any_of(batches, [](const ObjectBatch &batch) { return !batch.visibility.empty(); }) ||
				std::ranges::any_of(lights, [](const ObjectLight &light) { return !light.visibility.empty(); });
			// Without ATTR_LOD the object still has a range (DEFAULT_LOD_RADII)
			component.usesDistance = (data.lods.empty() && !batches.empty()) ||
				std::ranges::any_of(batches, [](const ObjectBatch &batch) { return batch.lod >= 0; }) ||
				std::ranges::any_of(lights, [](const ObjectLight &light) { return light.lod >= 0; });
			component.evaluatedStamp = 0;
			component.nodeEntities = std::move(nodeEntities);
			component.batchEntities = std::move(batchEntities);
			component.spillEntities = std::move(spillEntities);
			component.spillSources = std::move(spillSources);
			component.loaded = true;

			if (component.logPlacement)
			{
				const TagComponent *tag = registry.try_get<TagComponent>(rootHandle);
				SEDX_CORE_INFO_TAG(LOG_TAG, "Placed '{}' ({} batches, {} animation groups, {} lights){}", tag ? tag->pTag : asset->GetPath().filename().string(),
					batches.size(), data.nodes.size(), lights.size(), component.virtualPath.empty() ? std::string() : " as " + component.virtualPath);
			}
		}

		/* @brief Builds the placements whose asset has arrived and removes the ones whose asset failed to load. */
		void BuildLoadedObjects(entt::registry &registry)
		{
			// Only the placements still waiting (walking every placement each frame cost milliseconds with an airport)
			std::vector<entt::entity> ready;
			std::vector<entt::entity> failed;
			for (size_t i = 0; i < s_PendingRoots.size();)
			{
				const entt::entity handle = FindHandle(registry, s_PendingRoots[i]);
				const XPlaneObjectComponent *object = handle != entt::null ? registry.try_get<XPlaneObjectComponent>(handle) : nullptr;
				const bool done = !object || object->loaded || !object->asset || object->asset->IsReady() || object->asset->HasFailed();
				if (object && !object->loaded && object->asset)
				{
					if (object->asset->IsReady())
						ready.push_back(handle);
					else if (object->asset->HasFailed())
						failed.push_back(handle);
				}

				if (done)
				{
					s_PendingRoots[i] = s_PendingRoots.back();
					s_PendingRoots.pop_back();
				}
				else
					++i;
			}

			for (const entt::entity handle : ready)
			{
				BuildObject(registry, handle);
				if (const uint64_t rootId = GetId(registry, handle); rootId != 0 && rootId == s_FrameWhenLoaded)
					SceneObjects::FrameObject(rootId);
			}

			for (const entt::entity handle : failed)
			{
				const XPlaneObjectComponent &object = registry.get<XPlaneObjectComponent>(handle);
				// Once per file: a scenery pack places the same broken (or deliberately empty) object thousands of times
				static std::unordered_set<std::string> reported;
				if (reported.insert(object.asset->GetPath().string()).second)
					SEDX_CORE_ERROR_TAG(LOG_TAG, "Could not load '{}': {}", object.asset->GetPath().string(), object.asset->GetError());

				const uint64_t rootId = GetId(registry, handle);
				if (rootId == s_FrameWhenLoaded)
					s_FrameWhenLoaded = 0;
				if (const Entity entity = Scene::TryGetEntityWithUUID(UUID(rootId)))
					Scene::DestroyEntity(entity);
			}
		}

		/* @brief Animation groups evaluated and of those moved since the last Tick() (--perf-log counters). */
		uint32_t s_GroupsPosed = 0;
		uint32_t s_GroupsMoved = 0;

		/**
		 * @brief Reads the datarefs a placement's animation and hide / show rules use (Obj8Data::datarefs) into
		 * datarefValues. Main thread only.
		 * @return True when any of them moved since the last read.
		 */
		bool RefreshInputs(XPlaneObjectComponent &object, const DatarefReader &reader)
		{
			static std::vector<float> values; // scratch: swapped with the placement's
			ReadDatarefs(object.asset->GetData(), reader, values);
			if (values == object.datarefValues)
				return false;
			object.datarefValues.swap(values);
			return true;
		}

		/**
		 * @brief Camera distance, LOD, hide / show and animation for one loaded placement.
		 * @param inputsChanged Its datarefs (RefreshInputs()) or its settings changed since it was last evaluated.
		 */
		void UpdateObject(entt::registry &registry, const entt::entity rootHandle, XPlaneObjectComponent &object, const bool hasCamera,
			const Vec3 &cameraPosition, const bool inputsChanged, ParentWorldCache &parentCache)
		{
			ResolveEntities(registry, rootHandle, object);
			const Obj8Data &data = object.asset->GetData();

			// Camera distance for LOD selection, from the middle of the object's bounds (X-Plane measures from there: a
			// terminal's origin can sit hundreds of meters from its middle)
			const float *low = object.asset->GetBoundsMin(), *high = object.asset->GetBoundsMax();
			const float middle[3] = { 0.5f * (low[0] + high[0]), 0.5f * (low[1] + high[1]), 0.5f * (low[2] + high[2]) };
			const float radius = 0.5f * std::sqrt((high[0] - low[0]) * (high[0] - low[0]) + (high[1] - low[1]) * (high[1] - low[1]) + (high[2] - low[2]) * (high[2] - low[2]));
			float distance = 0.0f;
			if (hasCamera && object.rootStored)
			{
				const xMath::Matrix world = RootWorld(registry, rootHandle, *object.rootStored, parentCache);
				float at[3];
				TransformPoint(world, middle, at);
				const Vec3 delta(at[0] - cameraPosition.x, at[1] - cameraPosition.y, at[2] - cameraPosition.z);
				distance = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
			}
			object.cameraDistance = distance;
			const float defaultFar = std::max(radius, 1.0f) * DEFAULT_LOD_RADII;

			object.evaluatedStamp = s_CameraStamp;
			object.evaluatedVisible = object.visible;
			object.evaluatedLodOverride = object.lodOverride;
			object.evaluatedAnimate = object.animate;

			// Groups that draw: a visible batch or a light below them (scratch, per Obj8Data::nodes entry)
			static std::vector<uint8_t> drawn;
			drawn.assign(data.nodes.size(), 0);
			const auto markDrawn = [](const int node)
			{
				if (node >= 0 && static_cast<size_t>(node) < drawn.size())
					drawn[static_cast<size_t>(node)] = 1;
			};

			// Batches: LOD + hide / show
			object.visibleBatches = 0;
			object.visibleTriangles = 0;
			const std::vector<ObjectBatch> &batches = object.asset->GetBatches();
			for (size_t i = 0; i < object.batchStored.size() && i < batches.size(); ++i)
			{
				Entity *stored = object.batchStored[i];
				if (!stored)
					continue;

				const ObjectBatch &batch = batches[i];
				const bool visible = object.visible && IsLodVisible(data, batch.lod, distance, object.lodOverride, defaultFar) &&
									 EvaluateVisibilityAt(data, batch.visibility, object.datarefValues);
				if (stored->GetActive() != visible)
					stored->SetActive(visible);
				if (visible)
				{
					++object.visibleBatches;
					object.visibleTriangles += batch.triangleCount;
					markDrawn(batch.node);
				}
			}

			// Animation groups, once the LODs are known: like X-Plane, only geometry that draws is animated. KDEN's concourse
			// posters are some 36,000 clock-driven groups in their 0-200 m LODs (the far LODs are static). A group left out
			// stays stale and is posed once it draws; groups with lights always are (the lights follow them at night).
			if (!object.animate)
				return; // groups hold their pose; switching it back on is a settings change, which poses them again
			object.staleGroups.resize(data.nodes.size(), 1);
			if (inputsChanged)
				std::ranges::fill(object.staleGroups, uint8_t(1));
			for (const ObjectLight &light : object.asset->GetLights())
				markDrawn(light.node);
			for (size_t i = data.nodes.size(); i-- > 0;) // parents precede their children
			{
				if (drawn[i])
					markDrawn(data.nodes[i].parent);
			}

			for (size_t i = 0; i < object.nodeHandles.size() && i < data.nodes.size(); ++i)
			{
				if (!object.staleGroups[i] || !drawn[i])
					continue;
				object.staleGroups[i] = 0;

				const entt::entity handle = object.nodeHandles[i];
				TransformComponent *transform = handle != entt::null ? registry.try_get<TransformComponent>(handle) : nullptr;
				if (!transform)
					continue;

				const RigidTransform local = EvaluateAnimNodeAt(data.nodes[i], object.datarefValues);
				const Vec3 translation(local.translation[0], local.translation[1], local.translation[2]);
				const Quat rotation(local.rotation[3], local.rotation[0], local.rotation[1], local.rotation[2]);
				++s_GroupsPosed;

				// Only the groups whose pose changed are written (SetRotation() derives Euler angles) and re-read by the GPU
				// scene: stepped keyframe tables (signs, posters) hold most groups still while the sim clock runs
				const Quat current = transform->GetRotation();
				if (transform->translation.x == translation.x && transform->translation.y == translation.y && transform->translation.z == translation.z &&
					current.w == rotation.w && current.x == rotation.x && current.y == rotation.y && current.z == rotation.z)
					continue;

				transform->translation = translation;
				transform->SetRotation(rotation);
				GpuScene::MarkTransformDirty(handle); // its batches follow the pose the same frame
				++s_GroupsMoved;
			}
		}
	}

	// -------------------------------------------------------

	uint64_t SceneObjects::Place(const std::filesystem::path &file, const PlaceOptions &options, std::string *error)
	{
		entt::registry *registry = GetRegistry();
		if (!registry)
		{
			if (error)
				*error = "No active scene";
			return 0;
		}

		// Never waits: the asset may still be loading (and is shared with earlier placements of the file).
		const Ref<ObjectAsset> asset = ObjectAsset::Request(file);
		const std::string rootName = !options.name.empty() ? options.name : file.filename().string();

		// Root: the placement transform
		const Entity root = Scene::CreateEntity(rootName);
		if (!root)
		{
			if (error)
				*error = "Could not create an entity";
			return 0;
		}

		const entt::entity rootHandle = static_cast<entt::entity>(root);
		{
			TransformComponent &transform = registry->get<TransformComponent>(rootHandle);
			transform.translation = Vec3(options.position[0], options.position[1], options.position[2]);
			// DSF headings turn clockwise seen from above: a negative turn about +Y.
			transform.SetRotationEuler(Vec3(0.0f, -options.headingDegrees * kDegToRad, 0.0f));
		}

		XPlaneObjectComponent component;
		component.asset = asset;
		component.virtualPath = options.virtualPath;
		component.logPlacement = options.frameCamera; // a user placement, not one of an airport's thousands
		registry->emplace_or_replace<XPlaneObjectComponent>(rootHandle, std::move(component));
		const uint64_t rootId = GetId(*registry, rootHandle);

		if (asset->IsReady())
		{
			// This placement only: a full Tick() here walked every placed object for each new one (quadratic when an
			// airport places thousands of already-loaded objects). Its lights follow with the next Tick().
			BuildObject(*registry, rootHandle);
			Vec3 cameraPosition(0.0f, 0.0f, 0.0f);
			const bool hasCamera = GetCameraPosition(*registry, cameraPosition);
			ParentWorldCache parentCache;
			if (XPlaneObjectComponent *object = registry->try_get<XPlaneObjectComponent>(rootHandle); object && object->loaded)
			{
				RefreshInputs(*object, Datarefs::GetReader());
				UpdateObject(*registry, rootHandle, *object, hasCamera, cameraPosition, true, parentCache);
				object->evaluatedStamp = 0; // again next Tick(): the caller may still attach the root under a group
			}
		}
		else
		{
			s_PendingRoots.push_back(rootId);
			if (options.frameCamera)
				SEDX_CORE_INFO_TAG(LOG_TAG, "Placing '{}'{} while it loads", rootName, options.virtualPath.empty() ? std::string() : " (" + options.virtualPath + ")");
		}

		if (options.frameCamera)
			FrameObject(rootId);
		return rootId;
	}

	void SceneObjects::Tick(const double deltaSeconds)
	{
		// Seasonal library art follows the scenery's season (the terrain reloads when it changes: SceneTerrain::Update)
		LibraryManager::SetSeason(Environment::Get().GetDerived().scenerySeason.bits);

		// Background loads: upload what the workers finished, then fill in the placements that were waiting for it.
		{
			SEDX_PERF_SCOPE("Objects: asset uploads");
			ObjectAsset::Update();
			WeatherArt::Update();
		}
		entt::registry *registry = GetRegistry();
		if (registry)
		{
			SEDX_PERF_SCOPE("Objects: build loaded");
			BuildLoadedObjects(*registry);
		}
		{
			SEDX_PERF_SCOPE("Objects: 3D tree cells");
			Forest3d::Update();	// Forests' 3D trees near the camera
		}

		Datarefs::Tick(deltaSeconds);

		if (!registry)
		{
			Renderer::SetLightBillboards({});
			return;
		}

		Vec3 cameraPosition(0.0f, 0.0f, 0.0f);
		const bool hasCamera = GetCameraPosition(*registry, cameraPosition);
		const DatarefReader reader = Datarefs::GetReader();

		LightFrame lights;
		lights.camera = cameraPosition;
		lights.hasCamera = hasCamera;
		lights.night = Datarefs::GetNightFactor();
		lights.time = Datarefs::GetSimTime();
		if (lights.night > 0.0f)
		{
			TextureInfo atlasInfo;
			lights.atlas = ObjectAsset::GetLightTexture(&atlasInfo);
			lights.atlasBatch.texture = lights.atlas;
			lights.atlasInset[0] = atlasInfo.width > 0 ? 0.5f / static_cast<float>(atlasInfo.width) : 0.0f;
			lights.atlasInset[1] = atlasInfo.height > 0 ? 0.5f / static_cast<float>(atlasInfo.height) : 0.0f;
		}

		SEDX_PERF_SCOPE("Objects: animation, LOD, lights");
		const bool cameraMoving = hasCamera != s_LastHasCamera || xMath::Length2(cameraPosition - s_LastCameraPosition) > 1e-6f;
		if (cameraMoving)
		{
			++s_CameraStamp;
			s_LastCameraPosition = cameraPosition;
			s_LastHasCamera = hasCamera;
		}
		const uint64_t phase = s_TickCount++ % LOD_PHASES;

		ParentWorldCache parentCache;
		uint64_t index = 0;
		s_GroupsPosed = 0;
		s_GroupsMoved = 0;
		auto view = registry->view<XPlaneObjectComponent>();
		for (const entt::entity rootHandle : view)
		{
			XPlaneObjectComponent &object = view.get<XPlaneObjectComponent>(rootHandle);
			if (!object.asset || !object.loaded)
				continue; // still loading

			// A placement is evaluated again when a dataref its animation or hide / show rules read moved (the sim clock
			// does every frame), when the camera moved (LODs) or when its settings changed; otherwise it keeps its state
			const bool inputsChanged = object.dynamicState && RefreshInputs(object, reader);
			const bool settingsChanged = object.evaluatedStamp == 0 || object.visible != object.evaluatedVisible ||
										 object.lodOverride != object.evaluatedLodOverride || object.animate != object.evaluatedAnimate;
			const bool inPhase = !cameraMoving || index++ % LOD_PHASES == phase;
			const bool cameraMatters = object.usesDistance && object.evaluatedStamp != s_CameraStamp && inPhase;
			if (inputsChanged || settingsChanged || cameraMatters)
				UpdateObject(*registry, rootHandle, object, hasCamera, cameraPosition, inputsChanged || settingsChanged, parentCache);
			else
				ResolveEntities(*registry, rootHandle, object); // the lights below use the entity pointers

			// Lights: billboards and spill candidates
			CollectObjectLights(*registry, rootHandle, object, lights, parentCache);
		}
		Perf::SetCounter("anim groups posed", s_GroupsPosed);
		Perf::SetCounter("anim groups moved", s_GroupsMoved);

		// Spill: only the nearest lights light the scene
		std::ranges::sort(lights.spills, [](const SpillCandidate &a, const SpillCandidate &b) { return a.distanceSquared < b.distanceSquared; });
		for (size_t i = 0; i < lights.spills.size(); ++i)
		{
			SpillCandidate &candidate = lights.spills[i];
			const bool lit = i < MAX_SPILL_LIGHTS;
			if (lit)
			{
				candidate.light->SetIntensity(candidate.lumens);
				++candidate.object->litSpills;
			}
			if (candidate.entity->GetActive() != lit)
				candidate.entity->SetActive(lit);
		}

		// Billboards: everything drawn this frame, by texture
		std::vector<Renderer_LightBillboardBatch> billboardBatches;
		if (!lights.atlasBatch.billboards.empty())
			billboardBatches.push_back(std::move(lights.atlasBatch));
		for (Renderer_LightBillboardBatch &batch : lights.customBatches)
			billboardBatches.push_back(std::move(batch));
		{
			size_t billboards = 0;
			for (const Renderer_LightBillboardBatch &batch : billboardBatches)
				billboards += batch.billboards.size();
			Perf::SetCounter("light billboards", static_cast<double>(billboards));
		}
		Renderer::SetLightBillboards(std::move(billboardBatches));
	}

	XPlaneObjectComponent *SceneObjects::FindObject(const uint64_t entityId, uint64_t *rootId)
	{
		entt::registry *registry = GetRegistry();
		if (!registry)
			return nullptr;

		entt::entity handle = FindHandle(*registry, entityId);
		for (int depth = 0; handle != entt::null && depth < 64; ++depth)
		{
			if (XPlaneObjectComponent *object = registry->try_get<XPlaneObjectComponent>(handle))
			{
				if (rootId)
					*rootId = GetId(*registry, handle);
				return object;
			}

			const RelationshipComponent *relationship = registry->try_get<RelationshipComponent>(handle);
			handle = relationship ? FindHandle(*registry, static_cast<uint64_t>(relationship->pParentHandle)) : entt::null;
		}
		return nullptr;
	}

	void SceneObjects::FrameObject(const uint64_t rootId)
	{
		entt::registry *registry = GetRegistry();
		XPlaneObjectComponent *object = registry ? FindObject(rootId) : nullptr;
		const entt::entity rootHandle = registry ? FindHandle(*registry, rootId) : entt::null;
		if (!object || !object->asset || rootHandle == entt::null)
			return;

		// The bounds come with the asset: a placement still loading is framed when it arrives.
		if (!object->loaded)
		{
			s_FrameWhenLoaded = rootId;
			return;
		}
		s_FrameWhenLoaded = 0; // a newer request wins over one still waiting

		// Bounds center in world space (placement translation + heading only; the object is not scaled)
		const float *bmin = object->asset->GetBoundsMin();
		const float *bmax = object->asset->GetBoundsMax();
		const float local[3] = { (bmin[0] + bmax[0]) * 0.5f, (bmin[1] + bmax[1]) * 0.5f, (bmin[2] + bmax[2]) * 0.5f };
		const float extent[3] = { bmax[0] - bmin[0], bmax[1] - bmin[1], bmax[2] - bmin[2] };
		const float radius = std::max(0.5f, 0.5f * std::sqrt(extent[0] * extent[0] + extent[1] * extent[1] + extent[2] * extent[2]));

		Vec3 center(local[0], local[1], local[2]);
		if (Entity *stored = Scene::GetStoredEntity(Entity(rootHandle, Scene::GetActive())))
		{
			const xMath::Matrix world = stored->GetMatrix();
			center = Vec3(local[0] * world.m00 + local[1] * world.m10 + local[2] * world.m20 + world.m30,
						  local[0] * world.m01 + local[1] * world.m11 + local[2] * world.m21 + world.m31,
						  local[0] * world.m02 + local[1] * world.m12 + local[2] * world.m22 + world.m32);
		}

		// Three-quarter view from the south-east, far enough that the bounding sphere fits a 60 degree view.
		auto view = registry->view<TransformComponent, CameraComponent>();
		for (const entt::entity handle : view)
		{
			if (!view.get<CameraComponent>(handle).isPrimary)
				continue;

			constexpr float yaw = -35.0f * kDegToRad;
			constexpr float pitch = -25.0f * kDegToRad;
			const Vec3 forward(std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw));
			const float distance = radius / std::sin(30.0f * kDegToRad) * 1.15f;

			TransformComponent &transform = view.get<TransformComponent>(handle);
			if (Camera *camera = Scene::GetCamera())
				camera->LookAt(transform, center - forward * distance, center);	// The object becomes the orbit pivot
			else
			{
				transform.translation = center - forward * distance;
				transform.SetRotationEuler(Vec3(pitch, yaw, 0.0f));
			}
			break;
		}
	}

	void SceneObjects::Shutdown()
	{
		if (entt::registry *registry = GetRegistry())
		{
			std::vector<uint64_t> roots;
			for (const entt::entity handle : registry->view<XPlaneObjectComponent>())
				roots.push_back(GetId(*registry, handle));

			for (const uint64_t root : roots)
			{
				if (const Entity entity = Scene::TryGetEntityWithUUID(UUID(root)))
					Scene::DestroyEntity(entity);
			}
		}

		s_FrameWhenLoaded = 0;
		s_PendingRoots.clear();
		Renderer::SetLightBillboards({}); // the batches hold the light textures
		Forest3d::Shutdown();
		ObjectAsset::ClearCache();
		TextureLoader::ClearCache();
	}

	size_t SceneObjects::GetObjectCount()
	{
		entt::registry *registry = GetRegistry();
		return registry ? registry->view<XPlaneObjectComponent>().size() : 0;
	}

	size_t SceneObjects::GetLoadingObjectCount()
	{
		entt::registry *registry = GetRegistry();
		if (!registry)
			return 0;

		size_t count = 0;
		for (const entt::entity handle : registry->view<XPlaneObjectComponent>())
		{
			if (!registry->get<XPlaneObjectComponent>(handle).loaded)
				++count;
		}
		return count;
	}

}

// -------------------------------------------------------
