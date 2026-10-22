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
 * xp_terrain_scene.cpp
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#include "xp_terrain_scene.h"
#include "xp_library_manager.h"
#include "xp_terrain.h"
#include "xp_texture_loader.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <atomic>
#include <future>
#include <thread>
#include <map>
#include <set>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/renderer.h>
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

		constexpr const char *LOG_TAG = "XPlaneTerrain";
		constexpr int MAX_DEPTH = 16;
		constexpr float BORDER_LIFT = 0.01f;			// Meters border patches sit above the base, so the depth test keeps them
		constexpr float DEFAULT_LIT_NITS = 33.0f;		// LIT terrain without LUMINANCE (lighting/scenery_ref_nts in 12.4.4)
		// ASSUMPTIONS until Phase 16 calibrates them: where the composite mask is half in, and the half width of its fade
		constexpr float COMPOSITE_CENTER = 0.5f;
		constexpr float COMPOSITE_HALF_WIDTH = 0.12f;
		// X-Plane 12's open water colour when the tile has no water map (Resources/bitmaps/world/water/any.png): 13, 32, 51
		constexpr float WATER_SRGB[3] = { 13.0f / 255.0f, 32.0f / 255.0f, 51.0f / 255.0f };

		/* @brief xp_terrain.slang flags. */
		enum TerrainFlag : uint32_t
		{
			FlagBorder = 1u << 0,
			FlagCompositeBorder = 1u << 1,
			FlagComposite = 1u << 2,
			FlagHill = 1u << 3,
			FlagCliff = 1u << 4,
			FlagLit = 1u << 5,
			FlagFar = 1u << 6,
			FlagTiled = 1u << 7,
			FlagNoise = 1u << 8
		};

		float SrgbToLinear(const float value)
		{
			return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
		}

		struct TerrainType
		{
			bool loaded = false;
			TerrainAsset asset;
			fs::path file;
			std::vector<std::pair<PolygonDecal, fs::path>> decals;	// Rows and DECAL_LIB rows, with the file they're relative to
		};

		struct State
		{
			uint64_t root = 0;
			std::vector<uint64_t> parts;
			Ref<Mesh> mesh;
			std::map<std::string, Ref<MaterialAsset>> materials;
			std::map<std::string, Ref<ImageResource>> textures;
			TerrainSurface surface;
			GeoProjection projection;
			std::string areaKey;
			double datumElevation = 0.0;
			uint64_t generation = 0;
			SceneTerrain::Stats stats;
		};

		State &GetState()
		{
			static State state;
			return state;
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

		void Attach(entt::registry &registry, const entt::entity child, const entt::entity parent)
		{
			const UUID parentId = registry.get<IDComponent>(parent).pID;
			const UUID childId = registry.get<IDComponent>(child).pID;
			registry.get<RelationshipComponent>(child).pParentHandle = parentId;
			registry.get<RelationshipComponent>(parent).pChildren.push_back(childId);
			Scene::MarkStructureChanged();
		}

		/* @brief A key for "the same terrain": frame, area rings and margin. */
		std::string MakeAreaKey(const GeoProjection &projection, const TerrainArea &area)
		{
			char text[128];
			std::snprintf(text, sizeof(text), "%.7f,%.7f,%.1f|", projection.GetDatumLatitude(), projection.GetDatumLongitude(), area.margin);
			std::string key = text;
			for (const std::vector<Point2> &ring : area.rings)
			{
				for (const Point2 &p : ring)
				{
					std::snprintf(text, sizeof(text), "%.1f,%.1f;", p.x, p.z);
					key += text;
				}
				key += "|";
			}
			return key;
		}

		/**
		 * @brief The DSF holding a tile's base mesh: the highest-priority enabled scenery pack (custom scenery in
		 * scenery_packs.ini order, then the Global Scenery folders) whose tile isn't an overlay. Reads it with terrain and
		 * rasters (limited to the area).
		 */
		bool ReadBaseMesh(const int lat, const int lon, const DsfReadOptions &options, DsfFile &out, fs::path &file)
		{
			const std::shared_ptr<const LibraryIndex> index = LibraryManager::GetIndex();
			std::vector<const LibraryPackage *> packages;
			for (const LibraryPackage &package : index->GetPackages())
			{
				if (package.enabledInSim && (package.source == PackageSource::CustomScenery || package.source == PackageSource::GlobalScenery))
					packages.push_back(&package);
			}
			std::stable_sort(packages.begin(), packages.end(), [](const LibraryPackage *a, const LibraryPackage *b)
			{
				// The demo areas' meshes are a subset of the Global Scenery's
				const bool demoA = a->name.find("Demo Areas") != std::string::npos, demoB = b->name.find("Demo Areas") != std::string::npos;
				if (demoA != demoB)
					return !demoA;
				return a->priority < b->priority;
			});

			for (const LibraryPackage *package : packages)
			{
				const std::optional<fs::path> found = VFS::FindCaseInsensitive(GetDsfTilePath(package->path, lat + 0.5, lon + 0.5));
				if (!found)
					continue;
				DsfFile dsf;
				std::string error;
				if (!LoadDsf(*found, dsf, options, &error))
				{
					SEDX_CORE_WARN_TAG(LOG_TAG, "{}: {}", VFS::ToUtf8(*found), error);
					continue;
				}
				if (dsf.IsOverlay())
					continue;
				out = std::move(dsf);
				file = *found;
				return true;
			}
			return false;
		}

		/* @brief The .ter behind a terrain definition: a library path (resolved for the tile's region), else relative to the pack. */
		fs::path ResolveTerrain(const std::string &definition, const fs::path &tileFile, const int lat, const int lon)
		{
			if (!fs::path(definition).is_absolute())
			{
				if (fs::path file = LibraryManager::ResolveAt(definition, lat + 0.5, lon + 0.5); !file.empty())
					return file;
			}
			// <pack>/Earth nav data/<folder>/<tile>.dsf -> <pack>
			const fs::path pack = tileFile.parent_path().parent_path().parent_path();
			return VFS::FindCaseInsensitive((pack / definition).lexically_normal()).value_or(fs::path());
		}

		TerrainType LoadTerrainType(const fs::path &file)
		{
			TerrainType type;
			type.file = file;
			std::string error;
			if (file.empty() || !LoadTerrainAsset(file, type.asset, &error))
			{
				if (!file.empty())
					SEDX_CORE_WARN_TAG(LOG_TAG, "{}", error);
				return type;
			}
			type.loaded = true;
			for (const PolygonDecal &decal : type.asset.decals)
				type.decals.emplace_back(decal, file.parent_path());
			if (!type.asset.decalLibrary.empty())
			{
				const fs::path library = LibraryManager::Resolve(type.asset.decalLibrary);
				DecalLibraryAsset decals;
				if (!library.empty() && LoadDecalLibraryAsset(library, decals, &error))
				{
					for (const PolygonDecal &decal : decals.decals)
						type.decals.emplace_back(decal, library.parent_path());
				}
			}
			return type;
		}

		/* @brief A texture of a .ter (relative to baseFolder), decoded on a worker, uploaded once per file and usage. */
		struct TextureRequest
		{
			fs::path file;
			TextureUsage usage = TextureUsage::Albedo;
			DecodedTexture decoded;
		};

		/* @brief Decodes the requests on a few workers (the GPU upload stays on the main thread). */
		void DecodeAll(std::vector<TextureRequest *> &requests)
		{
			std::atomic<size_t> next{ 0 };
			const size_t workers = std::clamp<size_t>(std::thread::hardware_concurrency(), 1, 8);
			std::vector<std::thread> threads;
			for (size_t w = 0; w < std::min(workers, requests.size()); ++w)
			{
				threads.emplace_back([&next, &requests]()
				{
					for (size_t i = next++; i < requests.size(); i = next++)
						requests[i]->decoded = TextureLoader::Decode(requests[i]->file, requests[i]->usage);
				});
			}
			for (std::thread &thread : threads)
				thread.join();
		}

		std::string TextureKey(const fs::path &file, const TextureUsage usage)
		{
			return std::to_string(static_cast<int>(usage)) + "|" + VFS::ToUtf8(file);
		}

		/* @brief Builds the drawn parts' material for a land class: its textures in the xp_terrain.slang slots. */
		Ref<MaterialAsset> MakeTerrainMaterial(State &state, const TerrainType &type, const bool border)
		{
			const TerrainAsset &ter = type.asset;
			const fs::path folder = type.file.parent_path();
			const auto texture = [&state, &folder](const std::string &reference, const TextureUsage usage, const fs::path &base = fs::path()) -> ImageResource *
			{
				if (reference.empty())
					return nullptr;
				const fs::path file = TextureLoader::ResolveTexturePath(base.empty() ? folder : base, reference);
				if (file.empty())
					return nullptr;
				const auto it = state.textures.find(TextureKey(file, usage));
				return it != state.textures.end() ? it->second.Get() : nullptr;
			};

			Ref<MaterialAsset> material = CreateRef<MaterialAsset>();
			material->SetName("Terrain " + type.file.filename().string() + (border ? " (border)" : ""));
			material->SetProperty(MaterialClass::ColorR, 1.0f);
			material->SetProperty(MaterialClass::ColorG, 1.0f);
			material->SetProperty(MaterialClass::ColorB, 1.0f);
			material->SetProperty(MaterialClass::ColorA, 1.0f);
			material->SetProperty(MaterialClass::TextureTilingX, 1.0f);
			material->SetProperty(MaterialClass::TextureTilingY, 1.0f);
			material->SetProperty(MaterialClass::Roughness, 1.0f);
			material->SetProperty(MaterialClass::Metalness, 0.0f);
			material->SetProperty(MaterialClass::Normal, std::max(0.0f, ter.bumpLevel));
			material->SetProperty(MaterialProperty::CullMode, static_cast<float>(CullMode::Back));
			material->SetProperty(MaterialProperty::Terrain, 1.0f);
			material->SetProperty(MaterialProperty::XpMaterial, 1.0f);

			uint32_t flags = border ? FlagBorder : 0u;
			if (border && ter.compositeBorders)
				flags |= FlagCompositeBorder;

			// Base: projected from above (terrain that isn't is mapped at a kilometre a repeat)
			if (ImageResource *base = texture(ter.texture, TextureUsage::Albedo))
				material->SetTexture(MaterialTextureType::Color, base, 0);
			material->SetProperty(MaterialProperty::XpTerrainProjectedS, ter.projected ? ter.projectedS : 1000.0f);
			material->SetProperty(MaterialProperty::XpTerrainProjectedT, ter.projected ? ter.projectedT : 1000.0f);

			// Normal map, gloss and F0 by X-Plane 12's rules (xp_material.slang)
			ImageResource *normal = texture(ter.textureNormal, TextureUsage::Normal);
			if (normal)
				material->SetTexture(MaterialTextureType::Normal, normal, 0);
			material->SetProperty(MaterialProperty::XpTerrainNormalRatio, ter.normalRatio);
			material->SetProperty(MaterialProperty::XpNormalMode, normal ? (ter.normalMetalness ? 2.0f : 1.0f) : 0.0f);
			material->SetProperty(MaterialProperty::XpSpecularGain, normal ? (ter.specular >= 0.0f ? ter.specular : 1.0f) : 0.0f);
			material->SetProperty(MaterialProperty::XpSuperRoughness, std::max(0.0f, ter.superRoughness));
			material->SetProperty(MaterialProperty::XpDefaultF0, normal ? 0.04f : 0.0f);

			// Composite and its noise
			if (ImageResource *composite = texture(ter.compositeTexture, TextureUsage::Albedo))
			{
				material->SetTexture(MaterialTextureType::Roughness, composite, 0);
				flags |= FlagComposite;
			}
			material->SetProperty(MaterialProperty::XpTerrainCompositeS, ter.compositeProjected ? ter.compositeProjectedS : material->GetProperty(MaterialProperty::XpTerrainProjectedS));
			material->SetProperty(MaterialProperty::XpTerrainCompositeT, ter.compositeProjected ? ter.compositeProjectedT : material->GetProperty(MaterialProperty::XpTerrainProjectedT));
			material->SetProperty(MaterialProperty::XpTerrainCompositeCenter, COMPOSITE_CENTER);
			material->SetProperty(MaterialProperty::XpTerrainCompositeWidth, COMPOSITE_HALF_WIDTH);
			if (ImageResource *noise = texture(ter.compositeNoise, TextureUsage::Data))
			{
				material->SetTexture(MaterialTextureType::Roughness, noise, 3);
				flags |= FlagNoise;
			}

			// Slopes
			if (ImageResource *hill = ter.hill.IsUsed() ? texture(ter.hill.texture, TextureUsage::Albedo) : nullptr)
			{
				material->SetTexture(MaterialTextureType::Roughness, hill, 1);
				material->SetProperty(MaterialProperty::XpTerrainHillStart, ter.hill.angleStart);
				material->SetProperty(MaterialProperty::XpTerrainHillEnd, ter.hill.angleEnd);
				material->SetProperty(MaterialProperty::XpTerrainHillS, ter.hill.projectedS);
				material->SetProperty(MaterialProperty::XpTerrainHillT, ter.hill.projectedT);
				flags |= FlagHill;
			}
			if (ImageResource *cliff = ter.cliff.IsUsed() ? texture(ter.cliff.texture, TextureUsage::Albedo) : nullptr)
			{
				material->SetTexture(MaterialTextureType::Roughness, cliff, 2);
				material->SetProperty(MaterialProperty::XpTerrainCliffStart, ter.cliff.angleStart);
				material->SetProperty(MaterialProperty::XpTerrainCliffEnd, ter.cliff.angleEnd);
				material->SetProperty(MaterialProperty::XpTerrainCliffS, ter.cliff.projectedS);
				material->SetProperty(MaterialProperty::XpTerrainCliffT, ter.cliff.projectedT);
				flags |= FlagCliff;
			}

			// Border mask (overlay patches)
			if (border)
			{
				if (ImageResource *mask = texture(ter.borderTexture, TextureUsage::Data))
					material->SetTexture(MaterialTextureType::Metalness, mask, 0);
				else
					flags &= ~(FlagBorder | FlagCompositeBorder);	// No mask: drawn whole
			}

			// Far texture
			if (ImageResource *farTexture = ter.hasFar ? texture(ter.farTexture, TextureUsage::Albedo) : nullptr)
			{
				const float ratio = std::max(1e-4f, ter.farRatio);
				material->SetTexture(MaterialTextureType::Metalness, farTexture, 1);
				material->SetProperty(MaterialProperty::XpTerrainFarS, material->GetProperty(MaterialProperty::XpTerrainProjectedS) / ratio);
				material->SetProperty(MaterialProperty::XpTerrainFarT, material->GetProperty(MaterialProperty::XpTerrainProjectedT) / ratio);
				material->SetProperty(MaterialProperty::XpTerrainFarStart, ter.farStart);
				material->SetProperty(MaterialProperty::XpTerrainFarEnd, ter.farEnd);
				flags |= FlagFar;
			}

			// Night lighting: LUMINANCE nits (a scenery reference without it); LIT_FADE_PARAMS projects and fades it
			if (ImageResource *lit = texture(ter.textureLit, TextureUsage::Lit))
			{
				material->SetTexture(MaterialTextureType::Emission, lit, 0);
				material->SetProperty(MaterialProperty::EmissiveNits, ter.luminance > 0.0f ? ter.luminance : DEFAULT_LIT_NITS);
				const float litScale = ter.hasLitFade && ter.litFade[0] > 0.0f ? ter.litFade[0] : 0.0f;
				material->SetProperty(MaterialProperty::XpTerrainLitS, litScale > 0.0f ? litScale : material->GetProperty(MaterialProperty::XpTerrainProjectedS));
				material->SetProperty(MaterialProperty::XpTerrainLitT, litScale > 0.0f ? litScale : material->GetProperty(MaterialProperty::XpTerrainProjectedT));
				material->SetProperty(MaterialProperty::XpTerrainLitFadeStart, ter.hasLitFade ? ter.litFade[1] : 0.0f);
				material->SetProperty(MaterialProperty::XpTerrainLitFadeEnd, ter.hasLitFade ? ter.litFade[2] : 0.0f);
				flags |= FlagLit;
			}

			// Tile atlas: the map picks a tile per projected repeat
			if (ter.tiled)
			{
				material->SetProperty(MaterialProperty::XpTilesX, static_cast<float>(ter.tilesX));
				material->SetProperty(MaterialProperty::XpTilesY, static_cast<float>(ter.tilesY));
				ImageResource *map = texture(ter.tileMap, TextureUsage::Data);
				if (map)
					material->SetTexture(MaterialTextureType::Color, map, 1);
				material->SetProperty(MaterialProperty::XpTileMapWidth, map ? static_cast<float>(ter.tilePagesX) : 0.0f);
				material->SetProperty(MaterialProperty::XpTileMapHeight, map ? static_cast<float>(ter.tilePagesY) : 0.0f);
				flags |= FlagTiled;
			}

			// Decals: up to two detail and two normal ones, repeating per meter (the shader works in ground meters)
			int details = 0, normals = 0;
			const float projectedS = material->GetProperty(MaterialProperty::XpTerrainProjectedS);
			for (const auto &[decal, base] : type.decals)
			{
				int &count = decal.normal ? normals : details;
				if (count >= 2)
					continue;
				ImageResource *image = texture(decal.texture, decal.normal ? TextureUsage::Normal : TextureUsage::Data, base);
				if (!image)
					continue;
				const float perMeterS = decal.scaleS / std::max(0.01f, projectedS);
				const float perMeterT = decal.scaleT / std::max(0.01f, projectedS);
				const int slot = count++;
				if (decal.normal)
				{
					material->SetTexture(MaterialTextureType::Normal, image, 1 + slot);
					material->SetProperty(slot == 0 ? MaterialProperty::XpNormalDecal0Scale : MaterialProperty::XpNormalDecal1Scale, perMeterS);
					material->SetProperty(slot == 0 ? MaterialProperty::XpNormalDecal0Gain : MaterialProperty::XpNormalDecal1Gain, decal.strength * decal.gain);
				}
				else
				{
					material->SetTexture(MaterialTextureType::Color, image, 2 + slot);
					material->SetProperty(slot == 0 ? MaterialProperty::XpDecal0Scale : MaterialProperty::XpDecal1Scale, perMeterS);
					material->SetProperty(slot == 0 ? MaterialProperty::XpDecal0Strength : MaterialProperty::XpDecal1Strength, decal.strength);
				}
				if (slot == 0)
					material->SetProperty(decal.normal ? MaterialProperty::XpNormalDecalAspect : MaterialProperty::XpDecalAspect, perMeterT / std::max(1e-6f, perMeterS));
			}
			material->SetProperty(MaterialProperty::XpDrapeMetersU, 1.0f);
			material->SetProperty(MaterialProperty::XpDrapeMetersV, 1.0f);

			material->SetProperty(MaterialProperty::XpTerrainFlags, static_cast<float>(flags));
			return material;
		}

		/* @brief Water patches until Phase 11's water pass: a smooth surface in X-Plane's open water colour. */
		Ref<MaterialAsset> MakeWaterMaterial()
		{
			Ref<MaterialAsset> material = CreateRef<MaterialAsset>();
			material->SetName("Terrain water");
			material->SetProperty(MaterialClass::ColorR, SrgbToLinear(WATER_SRGB[0]));
			material->SetProperty(MaterialClass::ColorG, SrgbToLinear(WATER_SRGB[1]));
			material->SetProperty(MaterialClass::ColorB, SrgbToLinear(WATER_SRGB[2]));
			material->SetProperty(MaterialClass::ColorA, 1.0f);
			material->SetProperty(MaterialClass::TextureTilingX, 1.0f);
			material->SetProperty(MaterialClass::TextureTilingY, 1.0f);
			material->SetProperty(MaterialClass::Roughness, 0.1f);
			material->SetProperty(MaterialClass::Metalness, 0.0f);
			material->SetProperty(MaterialClass::Normal, 0.0f);
			material->SetProperty(MaterialProperty::CullMode, static_cast<float>(CullMode::None));
			material->SetProperty(MaterialProperty::XpMaterial, 1.0f);
			material->SetProperty(MaterialProperty::XpSpecularGain, 0.95f);		// Gloss: calm water
			material->SetProperty(MaterialProperty::XpDefaultF0, 0.02f);
			return material;
		}

		void DestroyRoot(State &state)
		{
			if (entt::registry *registry = GetRegistry(); registry && FindHandle(*registry, state.root) != entt::null)
			{
				if (const Entity root = Scene::TryGetEntityWithUUID(UUID(state.root)))
					Scene::DestroyEntity(root);
				Scene::MarkStructureChanged();
			}
			state.root = 0;
			state.parts.clear();
			state.mesh.Reset();
			state.materials.clear();
			state.textures.clear();
		}
	}

	// -------------------------------------------------------

	bool SceneTerrain::Load(const GeoProjection &projection, const TerrainArea &area, const std::string &name, const double fallbackElevation)
	{
		State &state = GetState();
		if (!IsEnabled())
		{
			Unload();
			return false;
		}
		const std::string areaKey = MakeAreaKey(projection, area) + "#" + std::to_string(LibraryManager::GetGeneration());
		if (state.root != 0 && state.areaKey == areaKey)
		{
			if (entt::registry *registry = GetRegistry(); registry && FindHandle(*registry, state.root) != entt::null)
				return true;
		}
		Unload();
		state.areaKey = areaKey;
		state.projection = projection;

		const auto start = std::chrono::steady_clock::now();
		entt::registry *registry = GetRegistry();
		if (!registry || LibraryManager::GetIndex()->GetPackages().empty())
			return false;

		// The tiles under the area, read with their terrain and rasters
		DsfReadOptions options;
		options.overlays = false;
		options.terrain = true;
		options.rasters = true;
		options.rasterNames = { "elevation", "sea_level", "spr1", "spr2", "sum1", "sum2", "fal1", "fal2", "win1", "win2" };
		area.GetGeoBox(projection, options.areaWest, options.areaSouth, options.areaEast, options.areaNorth);

		struct Tile
		{
			int lat = 0, lon = 0;
			DsfFile dsf;
			fs::path file;
		};
		std::vector<std::future<std::unique_ptr<Tile>>> reads;
		for (int lat = static_cast<int>(std::floor(options.areaSouth)); lat <= static_cast<int>(std::floor(options.areaNorth)); ++lat)
		{
			for (int lon = static_cast<int>(std::floor(options.areaWest)); lon <= static_cast<int>(std::floor(options.areaEast)); ++lon)
			{
				reads.push_back(std::async(std::launch::async, [lat, lon, options]() -> std::unique_ptr<Tile>
				{
					auto tile = std::make_unique<Tile>();
					tile->lat = lat;
					tile->lon = lon;
					return ReadBaseMesh(lat, lon, options, tile->dsf, tile->file) ? std::move(tile) : nullptr;
				}));
			}
		}
		std::vector<std::unique_ptr<Tile>> tiles;
		for (auto &read : reads)
		{
			if (std::unique_ptr<Tile> tile = read.get())
				tiles.push_back(std::move(tile));
		}
		if (tiles.empty())
		{
			SEDX_CORE_WARN_TAG(LOG_TAG, "{}: no base mesh found for the tiles around it (Global Scenery not installed?)", name);
			return false;
		}
		const double readMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

		// y = 0 at the terrain's height under the frame's datum
		state.datumElevation = fallbackElevation;
		const double datumLat = projection.GetDatumLatitude(), datumLon = projection.GetDatumLongitude();
		for (const std::unique_ptr<Tile> &tile : tiles)
		{
			const DsfRaster *elevation = tile->dsf.FindRaster("elevation");
			if (elevation && datumLat >= tile->dsf.south && datumLat <= tile->dsf.north && datumLon >= tile->dsf.west && datumLon <= tile->dsf.east)
				state.datumElevation = elevation->Sample(datumLon, datumLat, tile->dsf.west, tile->dsf.south, tile->dsf.east, tile->dsf.north);
		}

		TerrainBuildOptions build;
		build.datumElevation = state.datumElevation;
		build.area = area;
		struct TileParts
		{
			const Tile *tile = nullptr;
			std::vector<TerrainMeshPart> parts;
		};
		std::vector<TileParts> built;
		std::vector<TerrainMeshPart> all;
		for (const std::unique_ptr<Tile> &tile : tiles)
		{
			TileParts &entry = built.emplace_back();
			entry.tile = tile.get();
			BuildTerrainMeshes(tile->dsf, projection, build, entry.parts);
			all.insert(all.end(), entry.parts.begin(), entry.parts.end());
		}
		state.surface.Build(all);

		// Land classes: their .ter (resolved for the tile's library region) and every texture they use, decoded on workers
		std::map<std::string, TerrainType> types;	// By resolved file
		std::map<std::pair<const Tile *, std::string>, std::string> typeOf;
		std::map<std::string, TextureRequest> requests;
		const auto request = [&requests](const fs::path &folder, const std::string &reference, const TextureUsage usage)
		{
			if (reference.empty())
				return;
			const fs::path file = TextureLoader::ResolveTexturePath(folder, reference);
			if (file.empty())
				return;
			const std::string key = TextureKey(file, usage);
			if (requests.contains(key))
				return;
			TextureRequest &entry = requests[key];
			entry.file = file;
			entry.usage = usage;
		};
		uint32_t missing = 0;
		for (const TileParts &entry : built)
		{
			for (const TerrainMeshPart &part : entry.parts)
			{
				if (part.water || typeOf.contains({ entry.tile, part.definition }))
					continue;
				const fs::path file = ResolveTerrain(part.definition, entry.tile->file, entry.tile->lat, entry.tile->lon);
				const std::string key = VFS::ToUtf8(file);
				typeOf[{ entry.tile, part.definition }] = key;
				if (types.contains(key))
					continue;
				TerrainType type = LoadTerrainType(file);
				if (!type.loaded)
				{
					++missing;
					SEDX_CORE_WARN_TAG(LOG_TAG, "Terrain type not found or unreadable: {}", part.definition);
				}
				else
				{
					const TerrainAsset &ter = type.asset;
					const fs::path folder = file.parent_path();
					request(folder, ter.texture, TextureUsage::Albedo);
					request(folder, ter.textureNormal, TextureUsage::Normal);
					request(folder, ter.compositeTexture, TextureUsage::Albedo);
					request(folder, ter.compositeNoise, TextureUsage::Data);
					request(folder, ter.borderTexture, TextureUsage::Data);
					request(folder, ter.hill.texture, TextureUsage::Albedo);
					request(folder, ter.cliff.texture, TextureUsage::Albedo);
					request(folder, ter.farTexture, TextureUsage::Albedo);
					request(folder, ter.textureLit, TextureUsage::Lit);
					request(folder, ter.tileMap, TextureUsage::Data);
					int details = 0, normals = 0;
					for (const auto &[decal, base] : type.decals)
					{
						int &count = decal.normal ? normals : details;
						if (count++ < 2)
							request(base, decal.texture, decal.normal ? TextureUsage::Normal : TextureUsage::Data);
					}
				}
				types.emplace(key, std::move(type));
			}
		}
		std::vector<TextureRequest *> pending;
		for (auto &[key, entry] : requests)
			pending.push_back(&entry);
		DecodeAll(pending);
		for (auto &[key, entry] : requests)
		{
			TextureInfo info;
			if (Ref<ImageResource> image = TextureLoader::Upload(entry.decoded, &info))
				state.textures.emplace(key, std::move(image));
			else
				SEDX_CORE_WARN_TAG(LOG_TAG, "Terrain texture not found or unreadable: {}{}", VFS::ToUtf8(entry.file), info.error.empty() ? std::string() : " (" + info.error + ")");
		}

		// One mesh, a sub-mesh per part; the root at the scene's origin (the airport's frame)
		const Entity root = Scene::CreateEntity(name.empty() ? std::string("Terrain") : "Terrain " + name);
		const entt::entity rootHandle = static_cast<entt::entity>(root);
		if (!registry->valid(rootHandle))
			return false;
		state.root = GetId(*registry, rootHandle);
		state.mesh = CreateRef<Mesh>();
		state.mesh->SetFlags(0);
		state.mesh->SetResourceName("Terrain " + name);

		struct Drawn
		{
			std::string name;
			Ref<MaterialAsset> material;
			bool border = false;
			bool water = false;
			uint32_t order = 0;
			uint32_t subMesh = 0;
		};
		std::vector<Drawn> drawn;
		uint32_t triangles = 0;
		for (const TileParts &entry : built)
		{
			for (const TerrainMeshPart &part : entry.parts)
			{
				if (part.indices.empty())
					continue;
				Drawn item;
				item.border = part.overlay;
				item.water = part.water;
				item.order = part.order;
				if (part.water)
				{
					std::string key = "water";
					auto it = state.materials.find(key);
					if (it == state.materials.end())
						it = state.materials.emplace(key, MakeWaterMaterial()).first;
					item.material = it->second;
					item.name = "Water";
				}
				else
				{
					const std::string &typeKey = typeOf[{ entry.tile, part.definition }];
					const TerrainType &type = types[typeKey];
					if (!type.loaded)
						continue;
					const std::string key = typeKey + (part.overlay ? "|border" : "");
					auto it = state.materials.find(key);
					if (it == state.materials.end())
						it = state.materials.emplace(key, MakeTerrainMaterial(state, type, part.overlay)).first;
					item.material = it->second;
					item.name = type.file.stem().string() + (part.overlay ? " (border)" : "");
				}

				std::vector<Vertex_PosTexNorTan> vertices;
				vertices.reserve(part.vertices.size());
				const float lift = part.overlay ? BORDER_LIFT : 0.0f;
				for (const TerrainVertex &v : part.vertices)
				{
					const Vec3 normal(v.nx, v.ny, v.nz);
					const Vec3 tangent = xMath::Normalize(Vec3(1.0f, 0.0f, 0.0f) - normal * v.nx);	// East, along the surface
					vertices.emplace_back(Vec3(v.x, v.y + lift, v.z), Vec2(v.s, v.t), normal, tangent);
				}
				std::vector<uint32_t> indices = part.indices;
				state.mesh->AddGeometry(vertices, indices, false, &item.subMesh);
				triangles += static_cast<uint32_t>(indices.size() / 3);
				drawn.push_back(std::move(item));
			}
		}
		if (drawn.empty())
		{
			DestroyRoot(state);
			return false;
		}
		state.mesh->CreateGpuBuffers();

		for (const Drawn &item : drawn)
		{
			const Entity entity = Scene::CreateEntity(item.name);
			const entt::entity handle = static_cast<entt::entity>(entity);
			if (!registry->valid(handle))
				continue;
			Attach(*registry, handle, rootHandle);
			state.parts.push_back(GetId(*registry, handle));
			Entity *stored = Scene::GetStoredEntity(entity);
			Renderable *renderable = stored ? stored->AddComponent<Renderable>() : nullptr;
			if (!renderable)
				continue;
			renderable->SetMesh(state.mesh.Get(), item.subMesh);
			renderable->SetMaterial(item.material);
			renderable->SetFlag(CastsShadows, !item.border && !item.water);
			// Border patches draw over the base in the DSF's order, below everything an airport drapes
			if (item.border)
				renderable->SetDrapeOrder(Renderable::MakeDrapeOrder(0.0f, item.order));
		}

		state.stats = {};
		state.stats.tiles = static_cast<uint32_t>(tiles.size());
		state.stats.triangles = triangles;
		state.stats.parts = static_cast<uint32_t>(drawn.size());
		state.stats.terrainTypes = static_cast<uint32_t>(std::count_if(types.begin(), types.end(), [](const auto &entry) { return entry.second.loaded; }));
		state.stats.missingTypes = missing;
		state.stats.textures = static_cast<uint32_t>(state.textures.size());
		state.stats.datumElevation = state.datumElevation;
		state.stats.loadMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		std::string files;
		for (const std::unique_ptr<Tile> &tile : tiles)
			files += (files.empty() ? "" : ", ") + tile->file.filename().string() + " (" + tile->file.parent_path().parent_path().parent_path().filename().string() + ")";
		SEDX_CORE_INFO_TAG(LOG_TAG, "{}: {} triangles in {} parts from {} ({} land classes, {} missing; {} textures), y = 0 at {:.1f} m MSL, in {:.0f} ms ({:.0f} ms reading)",
			name, triangles, drawn.size(), files, state.stats.terrainTypes, missing, state.stats.textures, state.datumElevation, state.stats.loadMilliseconds, readMilliseconds);
		++state.generation;
		Renderer::SetGroundShown(true);
		return true;
	}

	void SceneTerrain::Unload()
	{
		State &state = GetState();
		if (state.root != 0 || !state.surface.IsEmpty())
			++state.generation;
		DestroyRoot(state);
		Renderer::SetGroundShown(false);
		state.surface.Clear();
		state.areaKey.clear();
		state.stats = {};
	}

	bool SceneTerrain::IsLoaded()
	{
		return GetState().root != 0 && !GetState().surface.IsEmpty();
	}

	const TerrainSurface &SceneTerrain::GetSurface()
	{
		return GetState().surface;
	}

	float SceneTerrain::GetHeight(const double x, const double z)
	{
		return GetState().surface.HeightOr(x, z, 0.0f);
	}

	double SceneTerrain::GetDatumElevation()
	{
		return GetState().datumElevation;
	}

	float SceneTerrain::ElevationToY(const double elevationMsl, const double x, const double z)
	{
		return static_cast<float>(elevationMsl - GetState().datumElevation - GetTerrainCurvatureDrop(x, z));
	}

	uint64_t SceneTerrain::GetRootId()
	{
		return GetState().root;
	}

	uint64_t SceneTerrain::GetGeneration()
	{
		return GetState().generation;
	}

	bool SceneTerrain::IsInFrame(const GeoProjection &projection)
	{
		const State &state = GetState();
		return IsLoaded() && std::abs(projection.GetDatumLatitude() - state.projection.GetDatumLatitude()) < 1e-7 &&
			   std::abs(projection.GetDatumLongitude() - state.projection.GetDatumLongitude()) < 1e-7;
	}

	bool SceneTerrain::Contains(const uint64_t entityId)
	{
		const State &state = GetState();
		entt::registry *registry = GetRegistry();
		if (!registry || state.root == 0)
			return false;
		uint64_t id = entityId;
		for (int depth = 0; id != 0 && depth < MAX_DEPTH; ++depth)
		{
			if (id == state.root)
				return true;
			const entt::entity handle = FindHandle(*registry, id);
			if (handle == entt::null)
				return false;
			const RelationshipComponent *relationship = registry->try_get<RelationshipComponent>(handle);
			id = relationship ? static_cast<uint64_t>(relationship->pParentHandle) : 0;
		}
		return false;
	}

	SceneTerrain::Stats SceneTerrain::GetStats()
	{
		return GetState().stats;
	}

	void SceneTerrain::SetEnabled(const bool enabled)
	{
		RendererFeatures features = Renderer::GetFeatures();
		features.terrain = enabled;
		Renderer::SetFeatures(features);
		if (!enabled)
			Unload();
	}

	bool SceneTerrain::IsEnabled()
	{
		return Renderer::GetFeatures().terrain;
	}

	void SceneTerrain::Shutdown()
	{
		Unload();
	}
}

// -------------------------------------------------------
