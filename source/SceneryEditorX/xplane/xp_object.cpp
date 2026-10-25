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
 * xp_object.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xp_object.h"
#include "xp_library_manager.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <deque>
#include <future>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <SceneryEditorX/core/threading/thread_pool.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/vulkan/enums.h>
#include <SceneryEditorX/filesystem/vfs.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * @struct ObjectLoadJob
	 * @brief The CPU half of an object load: built on a worker thread, then uploaded step by step on the main thread.
	 */
	struct ObjectLoadJob
	{
		struct Batch
		{
			ObjectBatch batch;						// subMesh and material are filled in on the main thread
			Obj8DrawState state;					// Picks the material
			std::vector<Vertex_PosTexNorTan> vertices;
			std::vector<uint32_t> indices;
		};

		std::filesystem::path path;
		std::chrono::steady_clock::time_point requested;
		bool submitted = false;						// Main thread: handed to the thread pool (or run)
		std::future<void> done;						// The worker task (ObjectAsset::Load() waits on it)
		std::atomic<ObjectLoadState> state{ ObjectLoadState::Queued };

		// Written by the worker; the main thread reads them once state is Uploading or Failed.
		Obj8Data data;
		std::vector<Batch> batches;
		std::vector<ObjectLight> lights;
		DecodedTexture albedo;
		DecodedTexture normal;
		DecodedTexture lit;
		DecodedTexture drapedAlbedo;
		DecodedTexture drapedNormal;
		DecodedTexture weather;				// WEATHER mask
		std::vector<std::pair<PolygonDecal, DecodedTexture>> decals;
		float boundsMin[3] = { 0.0f, 0.0f, 0.0f };
		float boundsMax[3] = { 0.0f, 0.0f, 0.0f };
		std::string error;
		float cpuMilliseconds = 0.0f;

		// Main thread: how far the upload got
		uint32_t step = 0;
		float uploadMilliseconds = 0.0f;
		uint32_t uploadFrames = 0;					// Frames the upload was spread over
		float longestFrameMilliseconds = 0.0f;		// Most upload time spent in one of them
		float frameMilliseconds = 0.0f;
		uint64_t lastFrame = ~0ull;
	};

	namespace
	{
		constexpr const char *LOG_TAG = "XPlaneObject";

		/*
		 * OBJ8 triangles are front-facing when their vertices run clockwise; the renderer's pipelines use
		 * counter-clockwise front faces (see the renderer GPU conventions), so every triangle is re-wound.
		 */
		constexpr bool REVERSE_WINDING = true;

		/*
		 * ATTR_blend surfaces are split at this alpha: texels at or above it go through the deferred passes (and keep
		 * writing depth, so an object still sorts against itself), the rest are blended by the forward transparent pass.
		 */
		constexpr float BLEND_SPLIT_ALPHA = 0.9f;

		/* @brief Luminance of a white LIT texel when the OBJ names none (lighting/scenery_ref_nts, X-Plane 12.4.4: 33). */
		constexpr float DEFAULT_SCENERY_LIT_NITS = 33.0f;
		/* @brief X-Plane clamps GLOBAL_luminance here (the largest half-float below infinity, minus margin). */
		constexpr float MAX_LIT_NITS = 65530.0f;

		/* @brief Main-thread time per frame for finishing loads (uploads); one step always runs, so loads progress. */
		constexpr float UPLOAD_BUDGET_MS = 4.0f;

		/*
		 * Loads decoding at once. More would take every core from the main and render threads and keep that many
		 * decoded mip chains in memory; the rest wait their turn in request order.
		 */
		constexpr uint32_t MAX_CONCURRENT_LOADS = 4;

		/* @brief Upload steps of a decoded load: the textures one by one, then geometry and materials. */
		enum UploadStep : uint32_t
		{
			UploadAlbedo,
			UploadNormal,
			UploadLit,
			UploadDrapedAlbedo,
			UploadDrapedNormal,
			UploadDecals,
			UploadGeometry
		};

		std::unordered_map<std::string, Ref<ObjectAsset>> s_Cache;	// Ready and in-flight assets
		std::vector<Ref<ObjectAsset>> s_Pending;					// In flight, in request order
		std::deque<std::shared_ptr<ObjectLoadJob>> s_Waiting;		// Requested, not handed to the thread pool yet
		std::vector<std::shared_ptr<ObjectLoadJob>> s_Running;		// Handed to the thread pool, not decoded yet
		uint32_t s_LoadsRequested = 0;								// Since s_Pending was last empty
		uint32_t s_LoadsFinished = 0;
		uint64_t s_UpdateFrame = 0;									// Counts ObjectAsset::Update() calls (upload statistics)

		// Workers resolve lights, so the catalog is shared under a lock and replaced, never modified.
		std::mutex s_LightMutex;
		std::shared_ptr<const LightCatalog> s_LightCatalog = std::make_shared<LightCatalog>();
		std::filesystem::path s_LightCatalogInstall;		// Install the catalog was looked up in ("" = not looked up yet)
		uint64_t s_LightCatalogGeneration = 0;				// Bumped when a lights.txt replaces the catalog

		// Main thread only
		constexpr uint64_t NO_LIGHT_TEXTURE = ~0ull;
		Ref<ImageResource> s_LightTexture;
		TextureInfo s_LightTextureInfo;
		uint64_t s_LightTextureGeneration = NO_LIGHT_TEXTURE;	// Catalog generation the texture was loaded for

		float MillisecondsSince(const std::chrono::steady_clock::time_point start)
		{
			return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
		}

		/* @brief lights.txt inside an X-Plane install, or an empty path. */
		std::filesystem::path FindLightsTxt(const std::filesystem::path &installRoot)
		{
			if (installRoot.empty())
				return {};

			const std::optional<std::filesystem::path> file = VFS::FindCaseInsensitive(installRoot / "Resources" / "bitmaps" / "world" / "lites" / "lights.txt");
			return file ? *file : std::filesystem::path();
		}

		/* @brief The X-Plane install an OBJ lives in (a parent folder with Resources/bitmaps/world/lites), or empty. */
		std::filesystem::path FindInstallAbove(const std::filesystem::path &objectPath)
		{
			for (std::filesystem::path folder = objectPath.parent_path(); !folder.empty() && folder != folder.root_path(); folder = folder.parent_path())
			{
				if (!FindLightsTxt(folder).empty())
					return folder;
			}
			return {};
		}

		std::string CacheKey(const std::filesystem::path &path)
		{
			std::string key = std::filesystem::absolute(path).lexically_normal().string();
			for (char &c : key)
			{
				if (c >= 'A' && c <= 'Z')
					c = static_cast<char>(c - 'A' + 'a');
			}
			return key;
		}

		/* @brief Per-vertex tangents from UV derivatives (s right, t up), orthogonalized against the normal. */
		void ComputeTangents(std::vector<Vertex_PosTexNorTan> &vertices, const std::vector<uint32_t> &indices)
		{
			std::vector<float> accum(vertices.size() * 3, 0.0f);
			for (size_t i = 0; i + 2 < indices.size(); i += 3)
			{
				const Vertex_PosTexNorTan &v0 = vertices[indices[i]];
				const Vertex_PosTexNorTan &v1 = vertices[indices[i + 1]];
				const Vertex_PosTexNorTan &v2 = vertices[indices[i + 2]];

				const float e1[3] = { v1.pos[0] - v0.pos[0], v1.pos[1] - v0.pos[1], v1.pos[2] - v0.pos[2] };
				const float e2[3] = { v2.pos[0] - v0.pos[0], v2.pos[1] - v0.pos[1], v2.pos[2] - v0.pos[2] };
				const float du1 = v1.tex[0] - v0.tex[0], dv1 = v1.tex[1] - v0.tex[1];
				const float du2 = v2.tex[0] - v0.tex[0], dv2 = v2.tex[1] - v0.tex[1];
				const float det = du1 * dv2 - du2 * dv1;
				if (std::abs(det) < 1e-12f)
					continue;

				const float r = 1.0f / det;
				const float t[3] = { (e1[0] * dv2 - e2[0] * dv1) * r, (e1[1] * dv2 - e2[1] * dv1) * r, (e1[2] * dv2 - e2[2] * dv1) * r };
				for (int k = 0; k < 3; ++k)
				{
					for (int c = 0; c < 3; ++c)
						accum[static_cast<size_t>(indices[i + k]) * 3 + c] += t[c];
				}
			}

			for (size_t v = 0; v < vertices.size(); ++v)
			{
				const float *n = vertices[v].nor;
				float t[3] = { accum[v * 3], accum[v * 3 + 1], accum[v * 3 + 2] };
				const float d = t[0] * n[0] + t[1] * n[1] + t[2] * n[2];
				for (int c = 0; c < 3; ++c)
					t[c] -= n[c] * d;

				float length = std::sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
				if (length < 1e-8f)
				{
					// No usable UV gradient: any vector perpendicular to the normal.
					const float axis[3] = { std::abs(n[0]) < 0.9f ? 1.0f : 0.0f, std::abs(n[0]) < 0.9f ? 0.0f : 1.0f, 0.0f };
					const float dd = axis[0] * n[0] + axis[1] * n[1] + axis[2] * n[2];
					for (int c = 0; c < 3; ++c)
						t[c] = axis[c] - n[c] * dd;
					length = std::sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
				}

				for (int c = 0; c < 3; ++c)
					vertices[v].tan[c] = length > 1e-8f ? t[c] / length : (c == 0 ? 1.0f : 0.0f);
			}
		}

		// -------------------------------------------------------
		// Worker side: no GPU work and no ObjectAsset access
		// -------------------------------------------------------

		/* @brief Reads and decodes one of the OBJ's textures; a reference to a missing file warns. */
		DecodedTexture DecodeTexture(const std::filesystem::path &folder, const std::string &reference, const TextureUsage usage)
		{
			DecodedTexture texture;
			if (reference.empty())
				return texture;

			const std::filesystem::path file = TextureLoader::ResolveTexturePath(folder, reference);
			if (file.empty())
			{
				texture.info.error = "File not found";
				SEDX_CORE_WARN_TAG(LOG_TAG, "Texture '{}' not found next to '{}'", reference, folder.string());
				return texture;
			}
			return TextureLoader::Decode(file, usage);
		}

		/**
		 * @brief The OBJ's detail and normal decals (its rows, then its DECAL_LIB library's), up to two of each, decoded:
		 * standard shading options (12.1.0+) that draw like the draped polygons' (xp_material.slang xp_drape_*).
		 */
		void DecodeDecals(ObjectLoadJob &job, const std::filesystem::path &folder)
		{
			std::vector<std::pair<PolygonDecal, std::filesystem::path>> rows;
			for (const Obj8Decal &row : job.data.decals)
			{
				// Back to the row's words: directive, leading numbers, texture, trailing numbers
				std::vector<std::string> words = { row.directive };
				for (const float value : row.values)
					words.push_back(std::to_string(value));
				words.push_back(row.texture);
				for (const float value : row.trailing)
					words.push_back(std::to_string(value));
				const std::vector<std::string_view> tokens(words.begin(), words.end());
				PolygonDecal decal;
				if (ParsePolygonDecalRow(tokens, decal))
					rows.emplace_back(decal, folder);
			}
			if (!job.data.decalLibrary.empty())
			{
				const std::filesystem::path library = LibraryManager::Resolve(job.data.decalLibrary);
				DecalLibraryAsset asset;
				std::string error;
				if (!library.empty() && LoadDecalLibraryAsset(library, asset, &error))
				{
					for (const PolygonDecal &decal : asset.decals)
						rows.emplace_back(decal, library.parent_path());
				}
			}

			int details = 0, normals = 0;
			for (const auto &[decal, base] : rows)
			{
				int &count = decal.normal ? normals : details;
				if (count >= 2)
					continue;
				DecodedTexture texture = DecodeTexture(base, decal.texture, decal.normal ? TextureUsage::Normal : TextureUsage::Data);
				if (!texture.valid)
					continue;
				++count;
				job.decals.emplace_back(decal, std::move(texture));
			}
		}

		/* @brief Resolves the OBJ's lights against lights.txt (unknown names are left out). */
		void ResolveLights(ObjectLoadJob &job)
		{
			const Obj8Data &data = job.data;
			if (data.lights.empty())
				return;

			const std::shared_ptr<const LightCatalog> catalog = ObjectAsset::GetLightCatalog(job.path);
			const std::string fileName = job.path.filename().string();
			std::unordered_set<std::string> reported;

			for (uint32_t i = 0; i < static_cast<uint32_t>(data.lights.size()); ++i)
			{
				const Obj8Light &source = data.lights[i];
				const bool needsCatalog = source.kind == Obj8Light::Kind::Named || source.kind == Obj8Light::Kind::Param;
				if (needsCatalog && !catalog->IsLoaded())
				{
					if (reported.insert("").second)
						SEDX_CORE_WARN_TAG(LOG_TAG, "{}: lights.txt not found (set the X-Plane folder); named lights are skipped", fileName);
					continue;
				}

				ObjectLight light;
				std::string warning;
				if (!catalog->Resolve(source, light.resolved, &warning))
				{
					if (reported.insert(source.name).second)
						SEDX_CORE_WARN_TAG(LOG_TAG, "{}: {}", fileName, warning);
					continue;
				}

				if (light.resolved.billboards.empty() && light.resolved.spills.empty())
					continue; // v8 ground halos only

				light.source = i;
				light.node = source.node;
				light.lod = source.lod;
				light.visibility = source.visibility;
				std::copy_n(source.position, 3, light.position);
				job.lights.push_back(std::move(light));
			}
		}

		/* @brief Parses the OBJ, decodes its textures, resolves its lights and builds the vertices of every batch. */
		bool BuildObjectData(ObjectLoadJob &job)
		{
			const std::filesystem::path &path = job.path;
			Obj8Data &data = job.data;

			std::string parseError;
			if (!LoadObj8(path, data, &parseError))
			{
				SEDX_CORE_ERROR_TAG(LOG_TAG, "Failed to load '{}': {}", path.string(), parseError);
				job.error = parseError;
				return false;
			}

			for (const std::string &warning : data.warnings)
				SEDX_CORE_WARN_TAG(LOG_TAG, "{}: {}", path.filename().string(), warning);

			if (data.batches.empty() && data.lights.empty())
			{
				const std::string message = "The object has no triangles or lights to draw";
				SEDX_CORE_WARN_TAG(LOG_TAG, "{}: {}", path.string(), message);
				job.error = message;
				return false;
			}

			// Textures (uploaded later, on the main thread)
			const std::filesystem::path folder = path.parent_path();
			job.albedo = DecodeTexture(folder, data.texture, TextureUsage::Albedo);
			job.normal = DecodeTexture(folder, data.textureNormal, TextureUsage::Normal);
			job.lit = DecodeTexture(folder, data.textureLit, TextureUsage::Lit);
			job.drapedAlbedo = DecodeTexture(folder, data.textureDraped, TextureUsage::Albedo);
			job.drapedNormal = DecodeTexture(folder, data.textureDrapedNormal, TextureUsage::Normal);
			job.weather = DecodeTexture(folder, data.weatherMask, TextureUsage::Data);
			DecodeDecals(job, folder);

			ResolveLights(job);

			std::vector<int32_t> remap(data.vertices.size(), -1);
			std::vector<uint32_t> touched;
			bool firstVertex = true;

			for (const Obj8Batch &batch : data.batches)
			{
				std::vector<Vertex_PosTexNorTan> vertices;
				std::vector<uint32_t> indices;
				indices.reserve(batch.GetIndexCount());

				for (const Obj8Batch::Range &range : batch.ranges)
				{
					for (uint32_t i = range.offset; i + 2 < range.offset + range.count; i += 3)
					{
						uint32_t triangle[3] = { data.indices[i], data.indices[i + 1], data.indices[i + 2] };
						if constexpr (REVERSE_WINDING)
							std::swap(triangle[1], triangle[2]);

						for (const uint32_t source : triangle)
						{
							if (remap[source] < 0)
							{
								const Obj8Vertex &v = data.vertices[source];
								Vertex_PosTexNorTan out;
								for (int c = 0; c < 3; ++c)
								{
									out.pos[c] = v.position[c];
									out.nor[c] = v.normal[c];

									job.boundsMin[c] = firstVertex ? v.position[c] : std::min(job.boundsMin[c], v.position[c]);
									job.boundsMax[c] = firstVertex ? v.position[c] : std::max(job.boundsMax[c], v.position[c]);
								}
								firstVertex = false;

								const float length = std::sqrt(out.nor[0] * out.nor[0] + out.nor[1] * out.nor[1] + out.nor[2] * out.nor[2]);
								if (length > 1e-8f)
								{
									for (float &component : out.nor)
										component /= length;
								}
								else
								{
									out.nor[0] = 0.0f;
									out.nor[1] = 1.0f;
									out.nor[2] = 0.0f;
								}

								// (s, t) stay as written: textures are stored with row 0 at t = 0 (see xp_texture_loader.h)
								out.tex[0] = v.uv[0];
								out.tex[1] = v.uv[1];

								remap[source] = static_cast<int32_t>(vertices.size());
								vertices.push_back(out);
								touched.push_back(source);
							}
							indices.push_back(static_cast<uint32_t>(remap[source]));
						}
					}
				}

				for (const uint32_t source : touched)
					remap[source] = -1;
				touched.clear();

				if (indices.empty())
					continue;

				ComputeTangents(vertices, indices);

				ObjectLoadJob::Batch out;
				out.batch.node = batch.node;
				out.batch.lod = batch.lod;
				out.batch.visibility = batch.visibility;
				out.batch.triangleCount = static_cast<uint32_t>(indices.size() / 3);
				out.batch.castShadow = batch.state.castShadow && !data.globalNoShadow;
				out.batch.draped = batch.state.draped;
				out.state = batch.state;
				out.vertices = std::move(vertices);
				out.indices = std::move(indices);
				job.batches.push_back(std::move(out));
			}

			if (job.batches.empty() && job.lights.empty())
			{
				job.error = "The object has no triangles or lights to draw";
				return false;
			}

			// Static light positions count towards the bounds (framing light-only fixtures).
			for (const ObjectLight &light : job.lights)
			{
				for (int c = 0; c < 3; ++c)
				{
					job.boundsMin[c] = firstVertex ? light.position[c] : std::min(job.boundsMin[c], light.position[c]);
					job.boundsMax[c] = firstVertex ? light.position[c] : std::max(job.boundsMax[c], light.position[c]);
				}
				firstVertex = false;
			}
			return true;
		}

		/* @brief Thread pool task: runs the CPU half of a load and hands it to the main thread through the job's state. */
		void RunLoadJob(ObjectLoadJob &job)
		{
			job.state.store(ObjectLoadState::Decoding, std::memory_order_relaxed);
			const auto start = std::chrono::steady_clock::now();

			bool built = false;
			try
			{
				built = BuildObjectData(job);
			}
			catch (const std::exception &e)
			{
				job.error = e.what();
				SEDX_CORE_ERROR_TAG(LOG_TAG, "Failed to load '{}': {}", job.path.string(), job.error);
			}
			catch (...)
			{
				job.error = "Unexpected error while loading";
				SEDX_CORE_ERROR_TAG(LOG_TAG, "Failed to load '{}': {}", job.path.string(), job.error);
			}

			job.cpuMilliseconds = MillisecondsSince(start);
			job.state.store(built ? ObjectLoadState::Uploading : ObjectLoadState::Failed, std::memory_order_release);
		}

		/* @brief Hands waiting loads to the thread pool while fewer than MAX_CONCURRENT_LOADS are decoding. */
		void SubmitWaitingLoads()
		{
			std::erase_if(s_Running, [](const std::shared_ptr<ObjectLoadJob> &job)
			{
				const ObjectLoadState state = job->state.load(std::memory_order_acquire);
				return state != ObjectLoadState::Queued && state != ObjectLoadState::Decoding;
			});

			// Leave a core for the main thread on small machines.
			const uint32_t threads = ThreadPool::GetThreadCount();
			const uint32_t limit = std::clamp(threads > 1 ? threads - 1 : 1u, 1u, MAX_CONCURRENT_LOADS);
			while (!s_Waiting.empty() && s_Running.size() < limit)
			{
				std::shared_ptr<ObjectLoadJob> job = std::move(s_Waiting.front());
				s_Waiting.pop_front();
				job->submitted = true;

				if (threads == 0)
				{
					RunLoadJob(*job); // no thread pool (tools, tests): load in place
					continue;
				}

				// The task keeps the job alive: an asset dropped meanwhile (ClearCache) just discards the result.
				job->done = ThreadPool::Submit([job] { RunLoadJob(*job); });
				s_Running.push_back(std::move(job));
			}
		}
	}

	// -------------------------------------------------------

	const char *GetObjectLoadStateName(const ObjectLoadState state)
	{
		switch (state)
		{
			case ObjectLoadState::NotLoaded:	return "Not loaded";
			case ObjectLoadState::Queued:		return "Queued";
			case ObjectLoadState::Decoding:		return "Decoding";
			case ObjectLoadState::Uploading:	return "Uploading";
			case ObjectLoadState::Ready:		return "Ready";
			case ObjectLoadState::Failed:		return "Failed";
		}
		return "?";
	}

	// -------------------------------------------------------

	Ref<ObjectAsset> ObjectAsset::Request(const std::filesystem::path &path)
	{
		const std::string key = CacheKey(path);
		if (const auto it = s_Cache.find(key); it != s_Cache.end())
			return it->second; // ready, or in flight for an earlier placement

		Ref<ObjectAsset> asset = CreateRef<ObjectAsset>();
		asset->m_Path = path;
		asset->m_State = ObjectLoadState::Queued;

		const std::shared_ptr<ObjectLoadJob> job = std::make_shared<ObjectLoadJob>();
		job->path = path;
		job->requested = std::chrono::steady_clock::now();
		asset->m_Job = job;

		s_Cache.emplace(key, asset);
		s_Pending.push_back(asset);
		s_Waiting.push_back(job);
		++s_LoadsRequested;

		SubmitWaitingLoads();
		return asset;
	}

	Ref<ObjectAsset> ObjectAsset::Load(const std::filesystem::path &path, std::string *error)
	{
		const Ref<ObjectAsset> asset = Request(path);
		if (asset->m_Job)
		{
			ObjectLoadJob &job = *asset->m_Job;
			if (!job.submitted)
			{
				// Still waiting for a free slot: decode it here instead of waiting behind the others.
				std::erase_if(s_Waiting, [&job](const std::shared_ptr<ObjectLoadJob> &waiting) { return waiting.get() == &job; });
				job.submitted = true;
				RunLoadJob(job);
			}
			else if (job.done.valid())
			{
				job.done.wait();
			}

			const ObjectLoadState state = job.state.load(std::memory_order_acquire);
			if (state == ObjectLoadState::Uploading)
			{
				while (!asset->FinishStep())
				{
				}
			}
			else
			{
				if (state != ObjectLoadState::Failed && job.error.empty())
					job.error = "The load was cancelled";
				asset->Fail();
			}

			++s_LoadsFinished;
			std::erase_if(s_Pending, [&asset](const Ref<ObjectAsset> &pending) { return pending.Get() == asset.Get(); });
			if (s_Pending.empty())
				s_LoadsRequested = s_LoadsFinished = 0;
		}

		if (asset->HasFailed())
		{
			if (error)
				*error = asset->GetError();
			return nullptr;
		}
		return asset;
	}

	void ObjectAsset::Update()
	{
		if (s_Pending.empty())
			return;

		SubmitWaitingLoads();
		++s_UpdateFrame;

		const auto start = std::chrono::steady_clock::now();
		bool worked = false;
		for (size_t i = 0; i < s_Pending.size();)
		{
			const Ref<ObjectAsset> asset = s_Pending[i]; // keeps the asset alive while Fail() drops it from the cache
			const ObjectLoadState state = asset->m_Job->state.load(std::memory_order_acquire);
			if (state == ObjectLoadState::Failed)
			{
				asset->Fail();
			}
			else if (state == ObjectLoadState::Uploading)
			{
				bool finished = false;
				while (!finished && (!worked || MillisecondsSince(start) < UPLOAD_BUDGET_MS))
				{
					finished = asset->FinishStep();
					worked = true;
				}
				if (!finished)
					break; // out of time: carry on next frame
			}
			else
			{
				++i; // still on a worker; later requests may finish first
				continue;
			}

			++s_LoadsFinished;
			s_Pending.erase(s_Pending.begin() + static_cast<std::ptrdiff_t>(i));
		}

		if (s_Pending.empty())
			s_LoadsRequested = s_LoadsFinished = 0;
	}

	ObjectLoadQueue ObjectAsset::GetLoadQueue()
	{
		ObjectLoadQueue queue;
		queue.requested = s_LoadsRequested;
		queue.finished = s_LoadsFinished;
		queue.loads.reserve(s_Pending.size());
		for (const Ref<ObjectAsset> &asset : s_Pending)
			queue.loads.push_back({ asset->m_Path, asset->GetState(), MillisecondsSince(asset->m_Job->requested) / 1000.0f });
		return queue;
	}

	ObjectLoadState ObjectAsset::GetLoadState(const std::filesystem::path &path)
	{
		const auto it = s_Cache.find(CacheKey(path));
		return it != s_Cache.end() ? it->second->GetState() : ObjectLoadState::NotLoaded;
	}

	std::vector<Ref<ObjectAsset>> ObjectAsset::GetLoaded()
	{
		std::vector<Ref<ObjectAsset>> assets;
		assets.reserve(s_Cache.size());
		for (const auto &[key, asset] : s_Cache)
		{
			if (asset->IsReady())
				assets.push_back(asset);
		}
		return assets;
	}

	void ObjectAsset::ClearCache()
	{
		// Loads still on a worker keep their job until they finish; nothing picks the results up any more.
		s_Waiting.clear();
		s_Running.clear();
		s_Pending.clear();
		s_LoadsRequested = s_LoadsFinished = 0;
		s_Cache.clear();
		s_LightTexture.Reset();
		s_LightTextureInfo = TextureInfo{};
		s_LightTextureGeneration = NO_LIGHT_TEXTURE;
	}

	std::shared_ptr<const LightCatalog> ObjectAsset::GetLightCatalog(const std::filesystem::path &objectPath)
	{
		// The configured install wins; otherwise the install the object was loaded from (--xp-object).
		std::filesystem::path install = LibraryManager::GetInstallPath();
		if (FindLightsTxt(install).empty() && !objectPath.empty())
			install = FindInstallAbove(objectPath);

		{
			std::scoped_lock lock(s_LightMutex);
			if (install.empty() || install == s_LightCatalogInstall)
				return s_LightCatalog;
		}

		const std::filesystem::path file = FindLightsTxt(install);
		if (file.empty())
		{
			std::scoped_lock lock(s_LightMutex);
			return s_LightCatalog;
		}

		// Parsed outside the lock: the main thread reads the catalog every night frame.
		const std::shared_ptr<LightCatalog> catalog = std::make_shared<LightCatalog>();
		std::string error;
		const bool loaded = catalog->Load(file, &error);

		std::scoped_lock lock(s_LightMutex);
		if (install == s_LightCatalogInstall)
			return s_LightCatalog; // another worker loaded it meanwhile

		s_LightCatalogInstall = install;
		if (loaded)
		{
			s_LightCatalog = catalog;
			++s_LightCatalogGeneration;
			SEDX_CORE_INFO_TAG(LOG_TAG, "Loaded {} light definitions from '{}'", s_LightCatalog->GetLightCount(), file.string());
		}
		else
		{
			SEDX_CORE_WARN_TAG(LOG_TAG, "Failed to load '{}': {}", file.string(), error);
		}
		return s_LightCatalog;
	}

	Ref<ImageResource> ObjectAsset::GetLightTexture(TextureInfo *info)
	{
		std::shared_ptr<const LightCatalog> catalog;
		uint64_t generation = 0;
		{
			std::scoped_lock lock(s_LightMutex);
			catalog = s_LightCatalog;
			generation = s_LightCatalogGeneration;
		}

		// Once per catalog: another lights.txt brings its own texture.
		if (generation != s_LightTextureGeneration && catalog->IsLoaded())
		{
			s_LightTextureGeneration = generation;
			s_LightTexture.Reset();
			s_LightTextureInfo = TextureInfo{};
			const std::filesystem::path file = TextureLoader::ResolveTexturePath(catalog->GetPath().parent_path(), catalog->GetTexture());
			if (!file.empty())
				s_LightTexture = TextureLoader::Load(file, TextureUsage::Lit, &s_LightTextureInfo);
			if (!s_LightTexture)
				SEDX_CORE_WARN_TAG(LOG_TAG, "Light texture '{}' not found next to '{}'", catalog->GetTexture(), catalog->GetPath().string());
		}

		if (info)
			*info = s_LightTextureInfo;
		return s_LightTexture;
	}

	uint32_t ObjectAsset::GetBillboardCount() const
	{
		uint32_t count = 0;
		for (const ObjectLight &light : m_Lights)
			count += static_cast<uint32_t>(light.resolved.billboards.size());
		return count;
	}

	uint32_t ObjectAsset::GetSpillCount() const
	{
		uint32_t count = 0;
		for (const ObjectLight &light : m_Lights)
			count += static_cast<uint32_t>(light.resolved.spills.size());
		return count;
	}

	ObjectAsset::~ObjectAsset() = default;

	ObjectLoadState ObjectAsset::GetState() const
	{
		return m_Job ? m_Job->state.load(std::memory_order_acquire) : m_State;
	}

	// -------------------------------------------------------
	// Main-thread side: ImageResource creation uploads through the immediate queue, which isn't thread-safe
	// -------------------------------------------------------

	bool ObjectAsset::FinishStep()
	{
		ObjectLoadJob &job = *m_Job;
		const auto start = std::chrono::steady_clock::now();

		switch (job.step++)
		{
			case UploadAlbedo:
				m_Textures.albedo = TextureLoader::Upload(job.albedo, &m_Textures.albedoInfo);
				break;
			case UploadNormal:
				m_Textures.normal = TextureLoader::Upload(job.normal, &m_Textures.normalInfo);
				break;
			case UploadLit:
				m_Textures.lit = TextureLoader::Upload(job.lit, &m_Textures.litInfo);
				break;
			case UploadDrapedAlbedo:
				m_Textures.drapedAlbedo = TextureLoader::Upload(job.drapedAlbedo, &m_Textures.drapedInfo);
				break;
			case UploadDrapedNormal:
			{
				TextureInfo drapedNormalInfo;
				m_Textures.drapedNormal = TextureLoader::Upload(job.drapedNormal, &drapedNormalInfo);
				break;
			}
			case UploadDecals:
				for (auto &[decal, texture] : job.decals)
				{
					if (Ref<ImageResource> image = TextureLoader::Upload(texture))
						m_Textures.decals.push_back({ decal, std::move(image) });
				}
				m_Textures.weather = TextureLoader::Upload(job.weather);
				break;
			default:
				FinishGeometry();
				break;
		}

		const float elapsed = MillisecondsSince(start);
		job.uploadMilliseconds += elapsed;
		if (job.lastFrame != s_UpdateFrame)
		{
			job.lastFrame = s_UpdateFrame;
			job.frameMilliseconds = 0.0f;
			++job.uploadFrames;
		}
		job.frameMilliseconds += elapsed;
		job.longestFrameMilliseconds = std::max(job.longestFrameMilliseconds, job.frameMilliseconds);

		if (m_State != ObjectLoadState::Ready)
			return false;

		m_LoadMilliseconds = job.cpuMilliseconds + job.uploadMilliseconds;
		SEDX_CORE_INFO_TAG(LOG_TAG, "Loaded '{}': {} triangles, {} batches, {} materials, {} animation groups, {} LODs, {} lights ({} billboards, {} spill) "
			"({:.1f} ms on a worker, {:.1f} ms uploading over {} frames (longest {:.1f} ms), {:.0f} ms after the request)",
			m_Path.filename().string(), m_Data.GetTriangleCount(), m_Batches.size(), m_Materials.size(), m_Data.nodes.size(), m_Data.lods.size(),
			m_Lights.size(), GetBillboardCount(), GetSpillCount(), job.cpuMilliseconds, job.uploadMilliseconds, job.uploadFrames,
			job.longestFrameMilliseconds, MillisecondsSince(job.requested));

		m_Job.reset();
		return true;
	}

	void ObjectAsset::FinishGeometry()
	{
		ObjectLoadJob &job = *m_Job;
		m_Data = std::move(job.data);
		m_Lights = std::move(job.lights);
		std::copy_n(job.boundsMin, 3, m_BoundsMin);
		std::copy_n(job.boundsMax, 3, m_BoundsMax);

		// Custom billboards are cut from the object's own texture; without one there is nothing to show.
		if (!m_Textures.albedo)
		{
			for (ObjectLight &light : m_Lights)
				std::erase_if(light.resolved.billboards, [](const LightBillboardSpec &billboard) { return billboard.customTexture; });
		}

		// Light fixtures (lib/airport/lights/...) often carry lights only: no mesh then.
		if (!job.batches.empty())
		{
			m_Mesh = CreateRef<Mesh>();
			m_Mesh->SetFlags(0); // keep X-Plane's units and geometry: no normalization, optimization or generated LODs
			m_Mesh->SetResourceName(m_Path.filename().string());
			m_Mesh->SetResourceFilePath(m_Path.string());
		}

		for (ObjectLoadJob::Batch &batch : job.batches)
		{
			ObjectBatch out = std::move(batch.batch);
			out.material = GetOrCreateMaterial(batch.state);
			m_Mesh->AddGeometry(batch.vertices, batch.indices, false, &out.subMesh);
			m_Batches.push_back(std::move(out));
		}
		job.batches.clear();

		// Appends the geometry to the renderer's global vertex / index buffers (uploaded at the start of the next frame).
		if (m_Mesh)
			m_Mesh->CreateGpuBuffers();

		m_State = ObjectLoadState::Ready;
	}

	void ObjectAsset::Fail()
	{
		m_Error = m_Job && !m_Job->error.empty() ? m_Job->error : "Unknown error";
		m_State = ObjectLoadState::Failed;
		m_Job.reset();

		// Out of the cache, so placing the file again tries again.
		if (const auto it = s_Cache.find(CacheKey(m_Path)); it != s_Cache.end() && it->second.Get() == this)
			s_Cache.erase(it);
	}

	uint32_t ObjectAsset::GetOrCreateMaterial(const Obj8DrawState &state)
	{
		const bool useDraped = state.draped && m_Textures.drapedAlbedo;
		const TextureInfo &albedoInfo = useDraped ? m_Textures.drapedInfo : m_Textures.albedoInfo;

		// ATTR_blend (the default) blends translucent texels; ATTR_no_blend cuts them out at its ratio, and
		// GLOBAL_no_blend does so for the whole object whatever the attributes say.
		const bool blendState = state.blend && !m_Data.hasGlobalNoBlend;
		const bool blended = blendState && albedoInfo.hasAlpha;
		const float noBlendRatio = m_Data.hasGlobalNoBlend ? m_Data.globalNoBlend : state.alphaCutoff;
		const float alphaCutoff = blended ? BLEND_SPLIT_ALPHA : (blendState ? 0.0f : std::clamp(noBlendRatio, 0.001f, 1.0f));

		// Only the parts of the draw state the renderer can express split materials.
		const auto same_material = [](const Obj8DrawState &a, const Obj8DrawState &b)
		{
			return a.cull == b.cull && a.draped == b.draped && a.blend == b.blend && (a.blend || a.alphaCutoff == b.alphaCutoff) &&
				a.shinyRatio == b.shinyRatio && a.hasLightLevel == b.hasLightLevel && a.lightLevelBrightness == b.lightLevelBrightness;
		};

		for (uint32_t i = 0; i < static_cast<uint32_t>(m_MaterialStates.size()); ++i)
		{
			if (same_material(m_MaterialStates[i], state))
				return i;
		}

		ImageResource *albedo = useDraped ? m_Textures.drapedAlbedo.Get() : m_Textures.albedo.Get();
		ImageResource *normal = useDraped ? m_Textures.drapedNormal.Get() : m_Textures.normal.Get();
		ImageResource *lit = useDraped ? nullptr : m_Textures.lit.Get();	// Draped geometry has no LIT texture of its own in OBJ8

		Ref<MaterialAsset> material = CreateRef<MaterialAsset>();
		material->SetName(m_Path.stem().string() + "_" + std::to_string(m_Materials.size()));

		// Without a texture X-Plane draws a gray proxy.
		const float tone = albedo ? 1.0f : 0.6f;
		material->SetProperty(MaterialClass::ColorR, tone);
		material->SetProperty(MaterialClass::ColorG, tone);
		material->SetProperty(MaterialClass::ColorB, tone);
		material->SetProperty(MaterialClass::ColorA, 1.0f);
		material->SetProperty(MaterialClass::TextureTilingX, 1.0f);
		material->SetProperty(MaterialClass::TextureTilingY, 1.0f);

		// X-Plane 12's material rules (xp_material.slang): the normal map carries gloss (A) and, with NORMAL_METALNESS,
		// F0 / metalness (B); GLOBAL_specular / ATTR_shiny_rat scale the gloss (1 with a normal map when neither is set)
		const bool translucency = !useDraped && m_Data.normalTranslucency;
		const bool metalness = useDraped ? m_Data.drapedNormalMetalness : m_Data.normalMetalness;
		const float specularGain = std::max(m_Data.hasGlobalSpecular ? m_Data.globalSpecular : (state.shinyRatio > 0.0f ? state.shinyRatio : (normal ? 1.0f : 0.0f)), 0.0f);
		const float defaultF0 = m_Data.blendGlass ? 0.06f : (translucency ? 0.0f : ((normal || specularGain > 0.0f) ? 0.04f : 0.0f));
		material->SetProperty(MaterialProperty::XpMaterial, 1.0f);
		material->SetProperty(MaterialProperty::XpNormalMode, normal ? (metalness ? 2.0f : (translucency ? 3.0f : 1.0f)) : 0.0f);
		material->SetProperty(MaterialProperty::XpSpecularGain, specularGain);
		material->SetProperty(MaterialProperty::XpSuperRoughness, std::clamp(m_Data.superRoughness, 0.0f, 1.0f));
		material->SetProperty(MaterialProperty::XpDefaultF0, defaultF0);
		material->SetProperty(MaterialClass::Roughness, 1.0f);
		material->SetProperty(MaterialClass::Metalness, 0.0f);
		material->SetProperty(MaterialClass::Normal, normal ? 1.0f : 0.0f);

		// LIT textures are luminance in nits: ATTR_light_level's brightness, else GLOBAL_luminance, else X-Plane's
		// scenery default (lighting/scenery_ref_nts, 33 nits). Never hidden in daylight: exposure is.
		if (lit)
		{
			const float nits = state.hasLightLevel && state.lightLevelBrightness > 0.0f ? state.lightLevelBrightness
				: (m_Data.globalLuminance > 0.0f ? m_Data.globalLuminance : DEFAULT_SCENERY_LIT_NITS);
			material->SetProperty(MaterialProperty::EmissiveNits, std::min(nits, MAX_LIT_NITS));
		}
		material->SetProperty(MaterialProperty::CullMode, static_cast<float>(state.cull ? CullMode::Back : CullMode::None));

		// Blended surfaces are split: the deferred passes keep texels at or above the cutoff, the forward transparent
		// pass blends the rest. BLEND_GLASS keeps reflections at full strength where the glass is clear.
		material->SetProperty(MaterialProperty::AlphaCutoff, alphaCutoff);
		material->SetProperty(MaterialProperty::BlendMode, blended ? (m_Data.blendGlass ? 2.0f : 1.0f) : 0.0f);

		material->SetTexture(MaterialTextureType::Color, albedo);
		material->SetTexture(MaterialTextureType::Normal, normal);
		material->SetTexture(MaterialTextureType::Emission, lit);

		// Weather receptivity (weather.slang): the WEATHER mask in emission slot 3; SNOW_ALBEDO_LUMA keeps its r and g
		// weights (the shipped art leaves b and a at 0)
		material->SetTexture(MaterialTextureType::Emission, m_Textures.weather.Get(), 3);
		material->SetProperty(MaterialProperty::XpWeatherTransparent, m_Data.weatherTransparent ? 1.0f : 0.0f);
		material->SetProperty(MaterialProperty::XpSnowAlbedoLuma, m_Data.hasSnowAlbedoLuma ? 1.0f : 0.0f);
		material->SetProperty(MaterialProperty::XpSnowAlbedoLumaR, m_Data.snowAlbedoLuma[0]);
		material->SetProperty(MaterialProperty::XpSnowAlbedoLumaG, m_Data.snowAlbedoLuma[1]);

		// Decals (xp_material.slang xp_drape_*): color slots 2-3, normal slots 1-2; their scales repeat per albedo tile,
		// so the UVs are the "meters" they count in. Their strength is the rows' constant term (TEXTURE_CONTROL keys
		// aren't read yet)
		int details = 0, normals = 0;
		for (const ObjectDecal &entry : m_Textures.decals)
		{
			const PolygonDecal &decal = entry.decal;
			const int slot = decal.normal ? normals++ : details++;
			if (slot >= 2)
				continue;
			if (decal.normal)
			{
				material->SetTexture(MaterialTextureType::Normal, entry.image.Get(), 1 + slot);
				material->SetProperty(slot == 0 ? MaterialProperty::XpNormalDecal0Scale : MaterialProperty::XpNormalDecal1Scale, decal.scaleS);
				material->SetProperty(slot == 0 ? MaterialProperty::XpNormalDecal0Gain : MaterialProperty::XpNormalDecal1Gain, decal.strength * decal.gain);
			}
			else
			{
				material->SetTexture(MaterialTextureType::Color, entry.image.Get(), 2 + slot);
				material->SetProperty(slot == 0 ? MaterialProperty::XpDecal0Scale : MaterialProperty::XpDecal1Scale, decal.scaleS);
				material->SetProperty(slot == 0 ? MaterialProperty::XpDecal0Strength : MaterialProperty::XpDecal1Strength, decal.strength);
			}
			if (slot == 0)
				material->SetProperty(decal.normal ? MaterialProperty::XpNormalDecalAspect : MaterialProperty::XpDecalAspect, decal.scaleT / std::max(1e-6f, decal.scaleS));
		}
		if (!m_Textures.decals.empty())
		{
			material->SetProperty(MaterialProperty::XpDrapeMetersU, 1.0f);
			material->SetProperty(MaterialProperty::XpDrapeMetersV, 1.0f);
		}

		m_Materials.push_back(material);
		m_MaterialStates.push_back(state);
		return static_cast<uint32_t>(m_Materials.size()) - 1;
	}

	void ComputeVertexTangents(std::vector<Vertex_PosTexNorTan> &vertices, const std::vector<uint32_t> &indices)
	{
		ComputeTangents(vertices, indices);
	}

}

// -------------------------------------------------------
