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
 * gpu_scene.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "gpu_scene.h"
#include "renderer.h"
#include "renderer_declarations.h"
#include "scene_picking.h"
#include "vulkan/bindless_manager.h"
#include "vulkan/buffer.h"
#include "vulkan/command_list.h"
#include "vulkan/image_resource.h"
#include "vulkan/memory_allocator.h"
#include "vulkan/push_constant_buffer.h"
#include "vulkan/queue_manager.h"
#include "vulkan/pipeline/pipeline_state.h"
#include "vulkan/shader/shader_manager.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <SceneryEditorX/core/frame_profiler.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/components/component_sets.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr const char *LOG_TAG = "GpuScene";

		// Layouts shared with gpu_scene_types.slang
		constexpr uint32_t INSTANCE_ENABLED = 1u << 0;
		constexpr uint32_t INSTANCE_CASTS_SHADOW = 1u << 1;	// GPU_INSTANCE_CASTS_SHADOW
		constexpr uint32_t INSTANCE_PICK_SHIFT = 8u;	// GPU_INSTANCE_PICK_SHIFT: bits 8..31 hold the editor pick id
		constexpr uint32_t MAX_LODS = 4;

		struct GpuInstance
		{
			float world0[4];	// rows 0-2 of the column-vector world matrix
			float world1[4];
			float world2[4];
			float boundsMin[3];
			uint32_t batch;
			float boundsMax[3];
			uint32_t flags;
		};
		static_assert(sizeof(GpuInstance) == 80, "GpuInstance must match gpu_scene_types.slang");

		struct GpuBatch
		{
			uint32_t firstCommand;
			uint32_t lodCount;
			uint32_t materialIndex;
			uint32_t flags;
		};
		static_assert(sizeof(GpuBatch) == 16, "GpuBatch must match gpu_scene_types.slang");

		struct DrawCommand
		{
			uint32_t indexCount;
			uint32_t instanceCount;	// The cull pass's append counter
			uint32_t firstIndex;
			int32_t vertexOffset;
			uint32_t firstInstance;	// Where this command's visible instances start
		};
		static_assert(sizeof(DrawCommand) == sizeof(VkDrawIndexedIndirectCommand), "DrawCommand must be a VkDrawIndexedIndirectCommand");

		/* @brief The record after the late commands whose instanceCount counts occluded instances (gpu_occluded_counter_command). */
		constexpr uint32_t OccludedCounterCommand(const uint32_t lateOffset) { return 2u * lateOffset; }

		/* @brief The first draw of sun-shadow cascade c: the cascades' command sets follow the occluded counter. */
		constexpr uint32_t ShadowCommand(const uint32_t lateOffset, const uint32_t cascade) { return 2u * lateOffset + 1u + cascade * lateOffset; }

		constexpr uint32_t NO_BATCH = UINT32_MAX;
		/* @brief Frames an instance keeps being checked every frame after its transform last changed. */
		constexpr uint32_t DYNAMIC_FRAMES = 240;
		/* @brief Share of the static instances whose transform is re-read each frame (they all are within this many frames). */
		constexpr uint32_t ROUND_ROBIN_FRAMES = 64;

		/* @brief A mesh range of one LOD in the global geometry buffer. */
		struct DrawRange
		{
			uint32_t indexCount = 0;
			uint32_t firstIndex = 0;
			int32_t vertexOffset = 0;
		};

		/**
		 * @brief Identifies a batch: LOD 0's global mesh range and the material. The geometry buffer only appends, so
		 * a range never names other geometry later, even after the mesh that owned it is gone.
		 */
		struct BatchKey
		{
			DrawRange lod0;
			const MaterialAsset *material = nullptr;

			bool operator==(const BatchKey &other) const
			{
				return lod0.indexCount == other.lod0.indexCount && lod0.firstIndex == other.lod0.firstIndex &&
					   lod0.vertexOffset == other.lod0.vertexOffset && material == other.material;
			}
		};

		struct BatchKeyHash
		{
			size_t operator()(const BatchKey &key) const noexcept
			{
				size_t hash = std::hash<const void *>{}(key.material);
				hash = hash * 31u ^ key.lod0.firstIndex;
				hash = hash * 31u ^ key.lod0.indexCount;
				hash = hash * 31u ^ static_cast<uint32_t>(key.lod0.vertexOffset);
				return hash;
			}
		};

		struct Batch
		{
			BatchKey key;
			std::array<DrawRange, MAX_LODS> lods{};
			uint32_t lodCount = 1;
			uint32_t instances = 0;		// Live instances; the material pointer may dangle once this is 0
			uint32_t capacity = 0;		// Visible-list slots reserved per LOD command
			uint32_t bucket = 0;
			uint32_t firstCommand = 0;
		};

		struct Proxy
		{
			const Renderable *renderable = nullptr;
			Entity *entity = nullptr;
			entt::entity handle = entt::null; // the entity's registry handle: stored entities are recycled (Scene)
			uint32_t batch = NO_BATCH;
			uint64_t seenGeneration = 0;
			uint32_t dynamicFrames = 0;	// > 0: checked every frame (counts down while the transform holds still)
			bool live = false;
			bool enabled = false;
			bool castsShadow = false;	// RenderableFlags::CastsShadows as last written
			xMath::Matrix world = xMath::Matrix::IDENTITY; // Row-vector, as last written
		};

		/* @brief Per command list: upload staging and the cull-result readback (both reused only after it completed). */
		struct FrameSlot
		{
			Ref<Buffer> staging;
			uint64_t stagingCapacity = 0;
			Ref<Buffer> readback;
			uint32_t readbackCapacity = 0;	// Commands
			uint32_t readbackCommands = 0;	// Commands copied by the last use, 0 = nothing to read
			uint32_t readbackLateOffset = 0;// The late offset of the layout they were copied with
		};

		struct State
		{
			bool enabled = true;
			bool drawable = false;	// Enabled and the GPU-scene shaders built: the CPU passes leave its renderables alone


			std::vector<Proxy> proxies;					// Slot = instance index
			std::vector<uint32_t> freeSlots;
			std::unordered_map<const Renderable *, uint32_t> slotOf;
			uint32_t liveProxies = 0;
			uint64_t syncedStructureVersion = 0;
			uint64_t generation = 0;

			std::vector<Batch> batches;
			std::unordered_map<BatchKey, uint32_t, BatchKeyHash> batchOf;
			bool layoutDirty = true;
			bool materialsDirty = false;				// OnMaterialsChanged(): buckets re-read after the next sync

			std::vector<GpuInstance> instances;			// CPU mirror of the instance buffer
			std::vector<uint8_t> instanceDirty;
			std::vector<uint32_t> dirtySlots;

			std::vector<GpuBatch> gpuBatches;
			std::vector<DrawCommand> commandTemplates;	// instanceCount = 0: early commands, late commands, occluded counter, shadow cascades
			uint32_t lateOffset = 0;					// Early commands = late commands = commands per phase
			uint32_t visibleSlots = 0;					// Both phases' visible lists
			std::array<GpuScene::Bucket, GpuScene::BUCKET_COUNT> buckets{};
			bool hasDraws = false;
			bool uploadLayout = false;

			uint32_t roundRobinCursor = 0;

			// GPU buffers (device local) and their element capacities
			Ref<Buffer> instanceBuffer, batchBuffer, commandBuffer, templateBuffer, visibleBuffer, visibilityBuffer;
			uint32_t instanceCapacity = 0, batchCapacity = 0, commandCapacity = 0, visibleCapacity = 0, visibilityCapacity = 0;

			std::unordered_map<const CommandList *, FrameSlot> frameSlots;

			float minPixelSize = 1.0f;
			float lodScale = 1.0f;
			bool frustumCulling = true;
			bool occlusionCulling = true;
			bool occlusionActive = false;				// This frame culls in two phases

			GpuScene::Stats stats;

			std::mutex pendingMutex;					// MarkTransformDirty() may come from the main thread
			std::vector<entt::entity> pendingTransforms;
		};

		State &S()
		{
			static State state;
			return state;
		}

		/* @brief Bucket index of a material: cull mode x alpha testing in the depth prepass. */
		uint32_t BucketOf(const MaterialAsset &material)
		{
			uint32_t cull = 2; // back
			switch (static_cast<CullMode>(material.GetProperty(MaterialProperty::CullMode)))
			{
				case CullMode::None:	cull = 0; break;
				case CullMode::Front:	cull = 1; break;
				default:				cull = 2; break;
			}
			const bool alphaTested = material.IsAlphaTested() || material.GetProperty(MaterialProperty::AlphaCutoff) > 0.0f;
			return cull * 2u + (alphaTested ? 1u : 0u);
		}

		/* @brief The batch key of a renderable, or false when the GPU scene doesn't draw it. */
		bool MakeKey(const Renderable &renderable, BatchKey &key)
		{
			const MaterialAsset *material = renderable.GetMaterialAsset();
			if (!material || renderable.GetLodCount() == 0 || renderable.HasInstancing())
				return false;
			// tessellated, draped (drawn in their drape order), and CullMode::All (draws nothing), stay on the CPU path
			if (renderable.IsDraped() || material->GetProperty(MaterialProperty::Tessellation) > 0.0f ||
				static_cast<CullMode>(material->GetProperty(MaterialProperty::CullMode)) == CullMode::All)
				return false;

			key.lod0 = { renderable.GetIndexCount(0), renderable.GetIndexOffset(0), static_cast<int32_t>(renderable.GetVertexOffset(0)) };
			key.material = material;
			return key.lod0.indexCount > 0;
		}

		void MarkDirty(State &s, const uint32_t slot)
		{
			if (!s.instanceDirty[slot])
			{
				s.instanceDirty[slot] = 1;
				s.dirtySlots.push_back(slot);
			}
		}

		/* @brief Writes a proxy's GPU record: transform rows, world bounds (local box through the world matrix), flags. */
		void WriteInstance(State &s, const uint32_t slot)
		{
			const Proxy &proxy = s.proxies[slot];
			GpuInstance &gpu = s.instances[slot];
			if (!proxy.live)
			{
				std::memset(&gpu, 0, sizeof(gpu));
				MarkDirty(s, slot);
				return;
			}

			const xMath::Matrix &m = proxy.world;
			const float rows[3][4] = {
				{ m.m00, m.m10, m.m20, m.m30 },
				{ m.m01, m.m11, m.m21, m.m31 },
				{ m.m02, m.m12, m.m22, m.m32 } };
			std::memcpy(gpu.world0, rows[0], sizeof(gpu.world0));
			std::memcpy(gpu.world1, rows[1], sizeof(gpu.world1));
			std::memcpy(gpu.world2, rows[2], sizeof(gpu.world2));

			// center through the matrix, half extents through its absolute 3x3 (row-vector: p' = p * M)
			const xMath::BoundingBox &local = proxy.renderable->GetBoundingBoxMesh();
			const xMath::Vec3 lo = local.GetMin();
			const xMath::Vec3 hi = local.GetMax();
			const float c[3] = { (lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f, (lo.z + hi.z) * 0.5f };
			const float e[3] = { (hi.x - lo.x) * 0.5f, (hi.y - lo.y) * 0.5f, (hi.z - lo.z) * 0.5f };
			for (int axis = 0; axis < 3; ++axis)
			{
				const float *row = rows[axis];
				const float center = c[0] * row[0] + c[1] * row[1] + c[2] * row[2] + row[3];
				const float extent = e[0] * std::abs(row[0]) + e[1] * std::abs(row[1]) + e[2] * std::abs(row[2]);
				gpu.boundsMin[axis] = center - extent;
				gpu.boundsMax[axis] = center + extent;
			}

			gpu.batch = proxy.batch;
			gpu.flags = (proxy.enabled ? INSTANCE_ENABLED : 0u) | (proxy.castsShadow ? INSTANCE_CASTS_SHADOW : 0u) |
						(ScenePicking::GetPickId(proxy.handle) << INSTANCE_PICK_SHIFT);
			MarkDirty(s, slot);
		}

		/* @brief The batch for a key, created (or revived when empty) from a renderable that has it. */
		uint32_t AcquireBatch(State &s, const BatchKey &key, const Renderable &renderable)
		{
			uint32_t index = 0;
			if (const auto it = s.batchOf.find(key); it != s.batchOf.end())
				index = it->second;
			else
			{
				index = static_cast<uint32_t>(s.batches.size());
				s.batches.emplace_back().key = key;
				s.batchOf.emplace(key, index);
				s.layoutDirty = true;
			}

			Batch &batch = s.batches[index];
			if (batch.instances == 0)
			{
				// new, or empty since its last instance went (its material may be another object at the same address)
				batch.key = key;
				batch.lodCount = std::clamp(renderable.GetLodCount(), 1u, MAX_LODS);
				for (uint32_t lod = 0; lod < batch.lodCount; ++lod)
					batch.lods[lod] = { renderable.GetIndexCount(lod), renderable.GetIndexOffset(lod), static_cast<int32_t>(renderable.GetVertexOffset(lod)) };
				batch.bucket = BucketOf(*key.material);
				s.layoutDirty = true;
			}

			if (++batch.instances > batch.capacity)
			{
				batch.capacity = std::max(16u, batch.instances + batch.instances / 2);
				s.layoutDirty = true;
			}
			return index;
		}

		void ReleaseBatch(State &s, const uint32_t index)
		{
			if (index < s.batches.size() && s.batches[index].instances > 0 && --s.batches[index].instances == 0)
				s.layoutDirty = true; // its commands go
		}

		uint32_t AddProxy(State &s, const Renderable &renderable, Entity *entity, const BatchKey &key)
		{
			uint32_t slot = 0;
			if (!s.freeSlots.empty())
			{
				slot = s.freeSlots.back();
				s.freeSlots.pop_back();
			}
			else
			{
				slot = static_cast<uint32_t>(s.proxies.size());
				s.proxies.emplace_back();
				s.instances.emplace_back();
				s.instanceDirty.push_back(0);
			}

			Proxy &proxy = s.proxies[slot];
			proxy = Proxy{};
			proxy.renderable = &renderable;
			proxy.entity = entity;
			proxy.handle = static_cast<entt::entity>(*entity);
			proxy.live = true;
			proxy.enabled = entity->GetActive();
			proxy.castsShadow = renderable.HasFlag(RenderableFlags::CastsShadows);
			proxy.batch = AcquireBatch(s, key, renderable);
			proxy.world = entity->GetMatrix();
			proxy.seenGeneration = s.generation;

			s.slotOf[&renderable] = slot;
			++s.liveProxies;
			WriteInstance(s, slot);
			return slot;
		}

		void RemoveProxy(State &s, const uint32_t slot)
		{
			Proxy &proxy = s.proxies[slot];
			if (!proxy.live)
				return;

			ReleaseBatch(s, proxy.batch);
			if (const auto it = s.slotOf.find(proxy.renderable); it != s.slotOf.end() && it->second == slot)
				s.slotOf.erase(it);
			proxy = Proxy{};
			s.freeSlots.push_back(slot);
			--s.liveProxies;
			WriteInstance(s, slot);
		}

		/**
		 * @brief Adds, moves and removes proxies to match the scene's renderables. Runs when the structure version moved:
		 * only then can renderables appear or vanish, so the pointers kept by live proxies are valid in between.
		 */
		void SyncStructure(State &s)
		{
			SEDX_PERF_SCOPE("Render: GpuScene sync");
			++s.generation;

			for (Entity *entity : Scene::GetRenderableEntities())
			{
				const Renderable *renderable = entity ? entity->GetComponent<Renderable>() : nullptr;
				if (!renderable)
					continue;

				const auto found = s.slotOf.find(renderable);
				BatchKey key;
				if (!MakeKey(*renderable, key))
				{
					if (found != s.slotOf.end())
						RemoveProxy(s, found->second); // no longer drawn here (material or mesh changed)
					continue;
				}

				if (found == s.slotOf.end())
				{
					AddProxy(s, *renderable, entity, key);
					continue;
				}

				const uint32_t slot = found->second;
				Proxy &proxy = s.proxies[slot];
				if (proxy.entity != entity || proxy.handle != static_cast<entt::entity>(*entity))
				{
					// a new renderable at a destroyed one's address (possibly on a recycled stored entity too)
					RemoveProxy(s, slot);
					AddProxy(s, *renderable, entity, key);
					continue;
				}

				proxy.seenGeneration = s.generation;
				if (!(s.batches[proxy.batch].key == key))
				{
					ReleaseBatch(s, proxy.batch);
					proxy.batch = AcquireBatch(s, key, *renderable);
					WriteInstance(s, slot);
				}
			}

			// renderables that went away
			for (uint32_t slot = 0; slot < static_cast<uint32_t>(s.proxies.size()); ++slot)
			{
				if (s.proxies[slot].live && s.proxies[slot].seenGeneration != s.generation)
					RemoveProxy(s, slot);
			}
		}

		/* @brief Moves the instances below the entities passed to MarkTransformDirty() onto the per-frame check. */
		void ApplyPendingTransforms(State &s)
		{
			std::vector<entt::entity> pending;
			{
				std::scoped_lock lock(s.pendingMutex);
				pending.swap(s.pendingTransforms);
			}
			if (pending.empty())
				return;

			Scene *scene = Scene::GetActive();
			if (!scene)
				return;

			entt::registry &registry = scene->GetRegistry();
			std::vector<entt::entity> stack(pending.begin(), pending.end());
			for (size_t guard = 0; !stack.empty() && guard < 1'000'000; ++guard)
			{
				const entt::entity handle = stack.back();
				stack.pop_back();
				if (!registry.valid(handle))
					continue;

				if (Entity *stored = Scene::GetStoredEntity(handle))
				{
					if (const Renderable *renderable = stored->GetComponent<Renderable>())
					{
						if (const auto it = s.slotOf.find(renderable); it != s.slotOf.end())
							s.proxies[it->second].dynamicFrames = DYNAMIC_FRAMES;
					}
				}

				if (const RelationshipComponent *relationship = registry.try_get<RelationshipComponent>(handle))
				{
					for (const UUID &child : relationship->pChildren)
					{
						if (const entt::entity childHandle = Scene::TryGetEntityHandle(child); childHandle != entt::null)
							stack.push_back(childHandle);
					}
				}
			}
		}

		/**
		 * @brief Active flags of every instance, and transforms of the enabled ones that moved recently plus a
		 * round-robin share of the rest (an animated object is noticed within ROUND_ROBIN_FRAMES and then followed).
		 */
		void ValidateInstances(State &s)
		{
			SEDX_PERF_SCOPE("Render: GpuScene validate");
			const uint32_t count = static_cast<uint32_t>(s.proxies.size());
			if (count == 0)
				return;

			const uint32_t budget = std::max(256u, count / ROUND_ROBIN_FRAMES + 1);
			const uint32_t windowStart = s.roundRobinCursor % count;
			s.roundRobinCursor = (windowStart + budget) % count;

			uint32_t validated = 0;
			uint32_t enabledCount = 0;
			for (uint32_t slot = 0; slot < count; ++slot)
			{
				Proxy &proxy = s.proxies[slot];
				if (!proxy.live)
					continue;

				const bool enabled = proxy.entity->GetActive();
				enabledCount += enabled ? 1u : 0u;
				const bool justEnabled = enabled && !proxy.enabled;
				const bool inWindow = (slot + count - windowStart) % count < budget;
				bool changed = enabled != proxy.enabled;
				proxy.enabled = enabled;

				// the PROPERTIES panel's cast-shadows switch changes no structure version: noticed here
				const bool castsShadow = proxy.renderable->HasFlag(RenderableFlags::CastsShadows);
				changed |= castsShadow != proxy.castsShadow;
				proxy.castsShadow = castsShadow;

				if (enabled && (justEnabled || inWindow || proxy.dynamicFrames > 0))
				{
					++validated;
					const xMath::Matrix world = proxy.entity->GetMatrix();
					if (std::memcmp(&world, &proxy.world, sizeof(world)) != 0)
					{
						proxy.world = world;
						proxy.dynamicFrames = DYNAMIC_FRAMES;
						changed = true;
					}
					else if (proxy.dynamicFrames > 0)
					{
						--proxy.dynamicFrames;
					}
				}

				if (changed)
					WriteInstance(s, slot);
			}

			s.stats.validatedTransforms = validated;
			s.stats.enabledInstances = enabledCount;
		}

		/* @brief Command ranges per bucket, visible-list regions per command, the batch table and the command templates. */
		void LayoutCommands(State &s)
		{
			std::array<std::vector<uint32_t>, GpuScene::BUCKET_COUNT> byBucket;
			for (uint32_t i = 0; i < static_cast<uint32_t>(s.batches.size()); ++i)
			{
				if (s.batches[i].instances > 0)
					byBucket[s.batches[i].bucket].push_back(i);
			}

			s.gpuBatches.assign(s.batches.size(), GpuBatch{});
			s.commandTemplates.clear();
			uint32_t region = 0;
			for (uint32_t b = 0; b < GpuScene::BUCKET_COUNT; ++b)
			{
				GpuScene::Bucket &bucket = s.buckets[b];
				bucket.firstCommand = static_cast<uint32_t>(s.commandTemplates.size());
				bucket.cullMode = b / 2 == 0 ? CullMode::None : (b / 2 == 1 ? CullMode::Front : CullMode::Back);
				bucket.alphaTested = (b & 1u) != 0;

				for (const uint32_t index : byBucket[b])
				{
					Batch &batch = s.batches[index];
					batch.firstCommand = static_cast<uint32_t>(s.commandTemplates.size());
					for (uint32_t lod = 0; lod < batch.lodCount; ++lod)
					{
						const DrawRange &range = batch.lods[lod];
						s.commandTemplates.push_back({ range.indexCount, 0u, range.firstIndex, range.vertexOffset, region });
						region += batch.capacity;
					}

					// live batches only: an empty batch's material may be gone
					s.gpuBatches[index] = { batch.firstCommand, batch.lodCount, batch.key.material->GetIndex(), 0u };
				}
				bucket.commandCount = static_cast<uint32_t>(s.commandTemplates.size()) - bucket.firstCommand;
			}

			// The late pass's commands (two-phase occlusion culling): the same draws with their visible lists in the
			// second region of the visible-instance buffer, then the occluded-instance counter (never drawn), then the
			// same draws once per sun-shadow cascade, each with a region of its own
			s.lateOffset = static_cast<uint32_t>(s.commandTemplates.size());
			if (s.lateOffset > 0)
			{
				for (uint32_t i = 0; i < s.lateOffset; ++i)
				{
					DrawCommand late = s.commandTemplates[i];
					late.firstInstance += region;
					s.commandTemplates.push_back(late);
				}
				s.commandTemplates.push_back(DrawCommand{});
				SEDX_CORE_ASSERT(s.commandTemplates.size() == OccludedCounterCommand(s.lateOffset) + 1u);

				for (uint32_t cascade = 0; cascade < GpuScene::SHADOW_CASCADES; ++cascade)
				{
					for (uint32_t i = 0; i < s.lateOffset; ++i)
					{
						DrawCommand shadow = s.commandTemplates[i];
						shadow.firstInstance += (2u + cascade) * region;
						s.commandTemplates.push_back(shadow);
					}
				}
				SEDX_CORE_ASSERT(s.commandTemplates.size() == ShadowCommand(s.lateOffset, GpuScene::SHADOW_CASCADES));
			}

			s.visibleSlots = region * (2u + GpuScene::SHADOW_CASCADES);
			s.layoutDirty = false;
			s.uploadLayout = true;
		}

		/**
		 * @brief A device-local buffer with room for at least `count` elements. Growing waits for the GPU (frames in
		 * flight read the old one through the bindless set) and points the set at the new buffer.
		 * @return True when the buffer was (re)created: its contents are undefined.
		 */
		bool EnsureBuffer(Ref<Buffer> &buffer, uint32_t &capacity, const uint32_t count, const uint32_t stride, const char *name, const BindlessResource resource)
		{
			if (buffer && count <= capacity)
				return false;

			if (buffer)
			{
				QueueManager::WaitIdleAll();
				buffer->Destroy();
			}

			capacity = std::max({ count + count / 2, capacity * 2, 64u });
			buffer = CreateRef<Buffer>(stride, capacity, nullptr, false, name);
			if (!buffer || !buffer->Get())
			{
				SEDX_CORE_ERROR_TAG(LOG_TAG, "Could not allocate '{}' ({} x {} bytes)", name, capacity, stride);
				buffer = nullptr;
				capacity = 0;
				return true;
			}
			BindlessManager::UpdateBuffer(resource, buffer.Get());
			return true;
		}

		/* @brief Host-visible staging for one command list, at least `bytes` large. */
		Buffer *AcquireStaging(FrameSlot &slot, const uint64_t bytes)
		{
			if (!slot.staging || slot.stagingCapacity < bytes)
			{
				// the list's previous use has completed (QueueManager::NextCommandList waits), so nothing reads it
				if (slot.staging)
					slot.staging->Destroy();
				slot.stagingCapacity = std::max<uint64_t>(bytes + bytes / 2, 256 * 1024);
				slot.staging = CreateRef<Buffer>(1, static_cast<uint32_t>(slot.stagingCapacity), nullptr, true, "gpu_scene_staging");
			}
			return slot.staging.Get();
		}

		void BufferBarrier(CommandList *cmdList, const VkPipelineStageFlags2 srcStages, const VkAccessFlags2 srcAccess, const VkPipelineStageFlags2 dstStages, const VkAccessFlags2 dstAccess)
		{
			VkMemoryBarrier2 barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
			barrier.srcStageMask = srcStages;
			barrier.srcAccessMask = srcAccess;
			barrier.dstStageMask = dstStages;
			barrier.dstAccessMask = dstAccess;

			VkDependencyInfo dependency{};
			dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependency.memoryBarrierCount = 1;
			dependency.pMemoryBarriers = &barrier;
			vkCmdPipelineBarrier2(cmdList->GetCommandBuffer(), &dependency);
		}

		/* @brief Sums the instance counts the cull passes wrote the last time this command list ran. */
		void ReadBack(State &s, FrameSlot &slot)
		{
			const uint32_t lateOffset = slot.readbackLateOffset;
			if (slot.readbackCommands == 0 || slot.readbackCommands <= OccludedCounterCommand(lateOffset) || !slot.readback)
				return;

			const auto *commands = static_cast<const DrawCommand *>(slot.readback->GetMappedData() ? slot.readback->GetMappedData() : slot.readback->Map());
			if (!commands)
				return;

			uint32_t early = 0, late = 0, drawn = 0, shadow = 0;
			for (uint32_t i = 0; i < lateOffset; ++i)
			{
				early += commands[i].instanceCount;
				late += commands[lateOffset + i].instanceCount;
				drawn += commands[i].instanceCount + commands[lateOffset + i].instanceCount > 0 ? 1u : 0u;
			}
			if (slot.readbackCommands >= ShadowCommand(lateOffset, GpuScene::SHADOW_CASCADES))
			{
				for (uint32_t i = ShadowCommand(lateOffset, 0); i < ShadowCommand(lateOffset, GpuScene::SHADOW_CASCADES); ++i)
					shadow += commands[i].instanceCount;
			}
			s.stats.shadowInstances = shadow;
			s.stats.visibleInstances = early + late;
			s.stats.earlyInstances = early;
			s.stats.lateInstances = late;
			s.stats.occludedInstances = commands[OccludedCounterCommand(lateOffset)].instanceCount;
			s.stats.drawnCommands = drawn;
			slot.readbackCommands = 0;
		}

		/* @brief Copies this frame's cull results (every command) to the command list's readback buffer for the stats. */
		void CopyCullResults(State &s, CommandList *cmdList)
		{
			FrameSlot &frame = s.frameSlots[cmdList];
			const uint32_t commandCount = static_cast<uint32_t>(s.commandTemplates.size());
			if (!frame.readback || frame.readbackCapacity < commandCount)
			{
				if (frame.readback)
					frame.readback->Destroy();
				frame.readbackCapacity = commandCount + commandCount / 2 + 16;

				// host-cached: the CPU reads it back (the mappable Buffer constructor gives write-combined memory)
				VmaAllocationCreateInfo allocInfo{};
				allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
				allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
				frame.readback = CreateRef<Buffer>(MemoryAllocator::GetAllocator(), static_cast<VkDeviceSize>(frame.readbackCapacity) * sizeof(DrawCommand),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT, allocInfo);
			}
			if (frame.readback && frame.readback->Get())
			{
				const VkBufferCopy copy{ 0, 0, static_cast<VkDeviceSize>(commandCount) * sizeof(DrawCommand) };
				vkCmdCopyBuffer(cmdList->GetCommandBuffer(), s.commandBuffer->Get(), frame.readback->Get(), 1, &copy);
				frame.readbackCommands = commandCount;
				frame.readbackLateOffset = s.lateOffset;
			}
		}

		/* @brief Pushes the cull constants shared by the passes (see gpu_cull.slang) and dispatches one thread per instance. */
		void DispatchCull(State &s, CommandList *cmdList)
		{
			PushConstantBuffer_Pass constants{};
			constants.drawIndex = static_cast<uint32_t>(s.proxies.size());
			constants.SetF4Value(s.minPixelSize, s.lodScale, s.frustumCulling ? 1.0f : 0.0f, 0.0f);
			constants.SetF3Value(static_cast<float>(s.lateOffset), s.occlusionActive ? 1.0f : 0.0f, static_cast<float>(ShadowCommand(s.lateOffset, 0)));
			cmdList->PushConstants(constants);
			cmdList->Dispatch((constants.drawIndex + 63u) / 64u, 1, 1);
		}

		/* @brief Whether the two-phase cull can run this frame: its shaders and the Hi-Z target exist. */
		bool CanCullOcclusion()
		{
			if (!ShaderManager::IsShaderAvailable(Renderer_Shader::gpu_cull_late) || !ShaderManager::IsShaderAvailable(Renderer_Shader::downsample_min) ||
				!ShaderManager::IsShaderAvailable(Renderer_Shader::blit))
				return false;

			ImageResource *hiz = Renderer::GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders_hiz);
			const ImageResource *depth = Renderer::GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
			return hiz && depth && hiz->Get() && *hiz->Get() != VK_NULL_HANDLE && hiz->GetMipCount() > 1 &&
				   hiz->GetResourceState() == ResourceState::PreparedForGpu;
		}
	}

	// -------------------------------------------------------

	void GpuScene::SetEnabled(const bool enabled)
	{
		S().enabled = enabled;
	}

	bool GpuScene::IsEnabled()
	{
		return S().enabled;
	}

	void GpuScene::SetOcclusionCulling(const bool enabled)
	{
		S().occlusionCulling = enabled;
	}

	bool GpuScene::IsOcclusionCullingEnabled()
	{
		return S().occlusionCulling;
	}

	bool GpuScene::IsOcclusionActive()
	{
		return S().occlusionActive;
	}

	uint32_t GpuScene::GetLateCommandOffset()
	{
		return S().lateOffset;
	}

	bool GpuScene::IsGpuDriven(const Renderable *renderable)
	{
		// enabled too: a toggle takes effect before Update() refreshes drawable, and the CPU list is built first
		const State &s = S();
		BatchKey key;
		return s.drawable && s.enabled && renderable && MakeKey(*renderable, key);
	}

	void GpuScene::OnMaterialsChanged()
	{
		// Buckets (cull mode, alpha test) and the bindless material slots are re-read in Update(), after the sync:
		// until then a batch can still count instances whose entities (and materials) were just destroyed.
		S().materialsDirty = true;
	}

	void GpuScene::MarkTransformDirty(const entt::entity entity)
	{
		State &s = S();
		std::scoped_lock lock(s.pendingMutex);
		s.pendingTransforms.push_back(entity);
	}

	void GpuScene::SetCullingOptions(const float minPixelSize, const float lodScale, const bool frustumCulling)
	{
		State &s = S();
		s.minPixelSize = std::max(minPixelSize, 0.0f);
		s.lodScale = std::max(lodScale, 0.01f);
		s.frustumCulling = frustumCulling;
	}

	const GpuScene::Stats &GpuScene::GetStats()
	{
		return S().stats;
	}

	const std::array<GpuScene::Bucket, GpuScene::BUCKET_COUNT> &GpuScene::GetBuckets()
	{
		return S().buckets;
	}

	Buffer *GpuScene::GetCommandBuffer()
	{
		return S().commandBuffer.Get();
	}

	bool GpuScene::HasDraws()
	{
		return S().hasDraws;
	}

	void GpuScene::Update(CommandList *cmdList)
	{
		SEDX_PERF_SCOPE("Render: GpuScene update");
		State &s = S();
		const bool wasOcclusionActive = s.occlusionActive;
		s.hasDraws = false;
		s.occlusionActive = false;
		s.stats.uploadedInstances = 0;
		s.drawable = s.enabled && ShaderManager::IsShaderAvailable(Renderer_Shader::gpu_cull) &&
					 ShaderManager::IsShaderAvailable(Renderer_Shader::depth_prepass_gpu) && ShaderManager::IsShaderAvailable(Renderer_Shader::gbuffer_gpu);
		if (!cmdList)
			return;


		if (!s.drawable)
		{
			if (s.liveProxies > 0)
			{
				// drop everything; re-enabling starts over
				for (uint32_t slot = 0; slot < static_cast<uint32_t>(s.proxies.size()); ++slot)
					RemoveProxy(s, slot);
				s.syncedStructureVersion = 0;
			}
			return;
		}

		// Empty batches pile up as airports are unloaded and rebuilt; past a point start over (the sync below brings
		// every instance back)
		if (const size_t empty = std::ranges::count_if(s.batches, [](const Batch &batch) { return batch.instances == 0; });
			empty > 4096 && empty > 3 * (s.batches.size() - empty))
		{
			for (uint32_t slot = 0; slot < static_cast<uint32_t>(s.proxies.size()); ++slot)
				RemoveProxy(s, slot);
			s.batches.clear();
			s.batchOf.clear();
			s.syncedStructureVersion = 0;
			s.layoutDirty = true;
		}

		if (const uint64_t version = Scene::GetStructureVersion(); version != s.syncedStructureVersion)
		{
			SyncStructure(s);
			s.syncedStructureVersion = version;
		}
		if (s.materialsDirty)
		{
			for (Batch &batch : s.batches)
			{
				if (batch.instances > 0)
					batch.bucket = BucketOf(*batch.key.material);
			}
			s.materialsDirty = false;
			s.layoutDirty = true;
		}
		ApplyPendingTransforms(s);
		ValidateInstances(s);
		if (s.layoutDirty)
			LayoutCommands(s);

		s.stats.instances = s.liveProxies;
		s.stats.batches = static_cast<uint32_t>(std::ranges::count_if(s.batches, [](const Batch &batch) { return batch.instances > 0; }));
		s.stats.commands = s.lateOffset;

		// GPU buffers (growing one invalidates its contents: everything is written again)
		const uint32_t instanceCount = static_cast<uint32_t>(s.proxies.size());
		if (EnsureBuffer(s.instanceBuffer, s.instanceCapacity, std::max(instanceCount, 1u), sizeof(GpuInstance), "gpu_scene_instances", BindlessResource::GpuInstances))
		{
			for (uint32_t slot = 0; slot < instanceCount; ++slot)
				MarkDirty(s, slot);
		}
		bool layoutBuffersRecreated = EnsureBuffer(s.batchBuffer, s.batchCapacity, std::max(static_cast<uint32_t>(s.gpuBatches.size()), 1u), sizeof(GpuBatch), "gpu_scene_batches", BindlessResource::GpuBatches);
		{
			// commands and their templates grow together
			uint32_t templateCapacity = s.commandCapacity;
			const uint32_t commandCount = std::max(static_cast<uint32_t>(s.commandTemplates.size()), 1u);
			layoutBuffersRecreated |= EnsureBuffer(s.commandBuffer, s.commandCapacity, commandCount, sizeof(DrawCommand), "gpu_scene_commands", BindlessResource::GpuCommands);
			if (!s.templateBuffer || commandCount > templateCapacity || templateCapacity != s.commandCapacity)
			{
				if (s.templateBuffer)
					s.templateBuffer->Destroy(); // the GPU was idled when the command buffer grew with it
				s.templateBuffer = CreateRef<Buffer>(sizeof(DrawCommand), s.commandCapacity, nullptr, false, "gpu_scene_command_templates");
				layoutBuffersRecreated = true;
			}
		}
		layoutBuffersRecreated |= EnsureBuffer(s.visibleBuffer, s.visibleCapacity, std::max(s.visibleSlots, 1u), sizeof(uint32_t), "gpu_scene_visible_instances", BindlessResource::GpuVisibleInstances);
		if (layoutBuffersRecreated)
			s.uploadLayout = true;

		// Last frame's visibility per instance (occlusion culling). Starts as all visible: the first two-phase frame
		// draws everything in view early, and a slot that changes hands only costs a frame of early drawing.
		const bool occlusion = s.occlusionCulling && CanCullOcclusion();
		bool resetVisibility = EnsureBuffer(s.visibilityBuffer, s.visibilityCapacity, std::max(instanceCount, 1u), sizeof(uint32_t), "gpu_scene_visibility", BindlessResource::GpuVisibility);
		resetVisibility |= occlusion && !wasOcclusionActive;

		if (!s.instanceBuffer || !s.batchBuffer || !s.commandBuffer || !s.templateBuffer || !s.visibleBuffer || !s.visibilityBuffer)
			return;

		// What this frame uploads, staged in the command list's own buffer
		FrameSlot &frame = s.frameSlots[cmdList];
		ReadBack(s, frame);

		std::ranges::sort(s.dirtySlots);
		const uint64_t instanceBytes = s.dirtySlots.size() * sizeof(GpuInstance);
		const uint64_t batchBytes = s.uploadLayout ? s.gpuBatches.size() * sizeof(GpuBatch) : 0;
		const uint64_t commandBytes = s.uploadLayout ? s.commandTemplates.size() * sizeof(DrawCommand) : 0;
		const uint64_t totalBytes = instanceBytes + batchBytes + commandBytes;

		std::vector<VkBufferCopy> instanceCopies;
		VkBufferCopy batchCopy{}, commandCopy{};
		if (totalBytes > 0)
		{
			Buffer *staging = AcquireStaging(frame, totalBytes);
			auto *mapped = static_cast<uint8_t *>(staging && staging->GetMappedData() ? staging->GetMappedData() : (staging ? staging->Map() : nullptr));
			if (!mapped)
			{
				SEDX_CORE_ERROR_TAG(LOG_TAG, "Staging buffer unavailable; GPU scene upload skipped this frame");
				return;
			}

			// dirty instances, consecutive slots merged into one copy region
			uint64_t offset = 0;
			for (size_t i = 0; i < s.dirtySlots.size();)
			{
				const uint32_t first = s.dirtySlots[i];
				size_t run = 1;
				while (i + run < s.dirtySlots.size() && s.dirtySlots[i + run] == first + run)
					++run;

				const uint64_t bytes = run * sizeof(GpuInstance);
				std::memcpy(mapped + offset, &s.instances[first], bytes);
				instanceCopies.push_back({ offset, static_cast<VkDeviceSize>(first) * sizeof(GpuInstance), bytes });
				offset += bytes;
				i += run;
			}

			if (batchBytes > 0)
			{
				std::memcpy(mapped + offset, s.gpuBatches.data(), batchBytes);
				batchCopy = { offset, 0, batchBytes };
				offset += batchBytes;
			}
			if (commandBytes > 0)
			{
				std::memcpy(mapped + offset, s.commandTemplates.data(), commandBytes);
				commandCopy = { offset, 0, commandBytes };
			}
		}

		s.stats.uploadedInstances = static_cast<uint32_t>(s.dirtySlots.size());
		for (const uint32_t slot : s.dirtySlots)
			s.instanceDirty[slot] = 0;
		s.dirtySlots.clear();

		// Record: earlier frames' reads of these buffers finish first (same queue), then the copies land
		cmdList->EndRenderPass();
		BufferBarrier(cmdList,
			VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT | VK_ACCESS_2_TRANSFER_READ_BIT);

		const VkCommandBuffer cb = cmdList->GetCommandBuffer();
		if (!instanceCopies.empty())
			vkCmdCopyBuffer(cb, frame.staging->Get(), s.instanceBuffer->Get(), static_cast<uint32_t>(instanceCopies.size()), instanceCopies.data());
		if (batchCopy.size > 0)
			vkCmdCopyBuffer(cb, frame.staging->Get(), s.batchBuffer->Get(), 1, &batchCopy);
		if (commandCopy.size > 0)
			vkCmdCopyBuffer(cb, frame.staging->Get(), s.templateBuffer->Get(), 1, &commandCopy);
		if (resetVisibility)
			vkCmdFillBuffer(cb, s.visibilityBuffer->Get(), 0, VK_WHOLE_SIZE, 1u);
		s.uploadLayout = false;

		s.hasDraws = s.lateOffset > 0;
		s.occlusionActive = s.hasDraws && occlusion;
	}

	void GpuScene::Cull(CommandList *cmdList)
	{
		State &s = S();
		if (!s.hasDraws || !cmdList)
			return;

		SEDX_PERF_SCOPE("Render: GpuScene cull (record)");
		const VkCommandBuffer cb = cmdList->GetCommandBuffer();
		cmdList->EndRenderPass();

		// the templates (uploaded above or in an earlier frame) reset every command's instance counter, once the draws and
		// culls recorded before have read them (an extra view culls again in the same command list)
		BufferBarrier(cmdList, VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT);
		const VkBufferCopy reset{ 0, 0, s.commandTemplates.size() * sizeof(DrawCommand) };
		vkCmdCopyBuffer(cb, s.templateBuffer->Get(), s.commandBuffer->Get(), 1, &reset);

		BufferBarrier(cmdList, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
			VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);

		PipelineState pso;
		pso.name = "gpu_cull";
		pso.shaders[static_cast<uint32_t>(StageType::Compute)] = ShaderManager::GetShader(Renderer_Shader::gpu_cull);
		cmdList->SetPipelineState(pso);
		DispatchCull(s, cmdList);

		// the draws read the commands (indirect) and the visible lists (vertex shader); the shadow cull appends to its own
		// commands; the readback copies the counts
		BufferBarrier(cmdList, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_TRANSFER_READ_BIT);
	}

	void GpuScene::CullShadows(CommandList *cmdList, const uint32_t cascadeCount)
	{
		State &s = S();
		if (!s.hasDraws || !cmdList || cascadeCount == 0 || !ShaderManager::IsShaderAvailable(Renderer_Shader::gpu_cull_shadow))
			return;

		SEDX_PERF_SCOPE("Render: GpuScene shadow cull (record)");
		cmdList->EndRenderPass();

		// Cull() reset every command (the cascades' too) and made its writes visible to compute
		PipelineState pso;
		pso.name = "gpu_cull_shadow";
		pso.shaders[static_cast<uint32_t>(StageType::Compute)] = ShaderManager::GetShader(Renderer_Shader::gpu_cull_shadow);
		cmdList->SetPipelineState(pso);
		DispatchCull(s, cmdList);

		// the cascades' draws read their commands and visible lists; the readback copies the counts
		BufferBarrier(cmdList, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_TRANSFER_READ_BIT);
	}

	uint32_t GpuScene::GetShadowCommandOffset(const uint32_t cascade)
	{
		return ShadowCommand(S().lateOffset, std::min(cascade, SHADOW_CASCADES - 1));
	}

	void GpuScene::RecordReadBack(CommandList *cmdList)
	{
		State &s = S();
		if (!s.hasDraws || !cmdList)
			return;
		cmdList->EndRenderPass();
		CopyCullResults(s, cmdList);
	}

	void GpuScene::CullLate(CommandList *cmdList, ImageResource *hiz)
	{
		State &s = S();
		if (!s.occlusionActive || !s.hasDraws || !cmdList || !hiz)
			return;

		SEDX_PERF_SCOPE("Render: GpuScene late cull (record)");
		cmdList->EndRenderPass();

		// the early pass read the visibility this pass rewrites (its command writes were made visible to the draws)
		BufferBarrier(cmdList, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);

		PipelineState pso;
		pso.name = "gpu_cull_late";
		pso.shaders[static_cast<uint32_t>(StageType::Compute)] = ShaderManager::GetShader(Renderer_Shader::gpu_cull_late);
		cmdList->SetPipelineState(pso);
		cmdList->SetTexture(Renderer_BindingsSrv::tex, hiz);
		DispatchCull(s, cmdList);

		// the late draws read their commands and visible lists; the readback copies every command's count
		BufferBarrier(cmdList, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_TRANSFER_READ_BIT);
	}

	void GpuScene::Shutdown()
	{
		State &s = S();
		for (Ref<Buffer> *buffer : { &s.instanceBuffer, &s.batchBuffer, &s.commandBuffer, &s.templateBuffer, &s.visibleBuffer, &s.visibilityBuffer })
		{
			if (*buffer)
			{
				(*buffer)->Destroy();
				*buffer = nullptr;
			}
		}
		for (auto &entry : s.frameSlots)
		{
			if (entry.second.staging)
				entry.second.staging->Destroy();
			if (entry.second.readback)
				entry.second.readback->Destroy();
		}
		s.frameSlots.clear();

		s.proxies.clear();
		s.freeSlots.clear();
		s.slotOf.clear();
		s.batches.clear();
		s.batchOf.clear();
		s.instances.clear();
		s.instanceDirty.clear();
		s.dirtySlots.clear();
		s.gpuBatches.clear();
		s.commandTemplates.clear();
		s.buckets = {};
		s.liveProxies = 0;
		s.lateOffset = 0;
		s.visibleSlots = 0;
		s.instanceCapacity = s.batchCapacity = s.commandCapacity = s.visibleCapacity = s.visibilityCapacity = 0;
		s.syncedStructureVersion = 0;
		s.layoutDirty = true;
		s.hasDraws = false;
		s.occlusionActive = false;
		s.stats = {};
	}

}

// -------------------------------------------------------
