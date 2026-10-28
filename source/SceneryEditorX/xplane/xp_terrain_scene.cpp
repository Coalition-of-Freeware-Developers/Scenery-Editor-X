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
#include "xp_7z.h"
#include "xp_assets.h"
#include "xp_library_manager.h"
#include "xp_overlay_scene.h"
#include "xp_overlay_shading.h"
#include "xp_terrain.h"
#include "xp_texture_loader.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <atomic>
#include <cctype>
#include <future>
#include <memory>
#include <thread>
#include <map>
#include <set>
#include <utility>
#include <SceneryEditorX/environment/season_model.h>
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
		// X-Plane 12's open water colour and clarity when the tile has no water map (Resources/bitmaps/world/water/any.png):
		// 13, 32, 51, 191
		constexpr float WATER_SRGB[3] = { 13.0f / 255.0f, 32.0f / 255.0f, 51.0f / 255.0f };
		constexpr float WATER_CLARITY = 191.0f / 255.0f;

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

		/**
		 * @brief The airport's grounds rasterized once (5 m cells, scanlines even-odd over every ring), so the hundreds of
		 * thousands of trees and tiles a mile of autogen tests aren't each run against the boundary's rings.
		 */
		struct GroundsMask
		{
			double minX = 0.0, minZ = 0.0;
			double cell = 5.0;
			int width = 0, height = 0;
			std::vector<uint8_t> cells;

			void Build(const std::vector<std::vector<Point2>> &rings)
			{
				*this = GroundsMask();
				Bounds2 bounds;
				for (const std::vector<Point2> &ring : rings)
				{
					if (ring.size() >= 3)
						bounds.Add(GetBounds(ring));
				}
				if (!bounds.valid)
					return;
				minX = bounds.minX;
				minZ = bounds.minZ;
				width = static_cast<int>(std::ceil((bounds.maxX - minX) / cell)) + 1;
				height = static_cast<int>(std::ceil((bounds.maxZ - minZ) / cell)) + 1;
				cells.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
				std::vector<double> crossings;
				for (int row = 0; row < height; ++row)
				{
					const double z = minZ + (row + 0.5) * cell;
					crossings.clear();
					for (const std::vector<Point2> &ring : rings)
					{
						if (ring.size() < 3)
							continue;
						for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
						{
							if ((ring[i].z > z) != (ring[j].z > z))
								crossings.push_back(ring[i].x + (ring[j].x - ring[i].x) * (z - ring[i].z) / (ring[j].z - ring[i].z));
						}
					}
					std::ranges::sort(crossings);
					for (size_t k = 0; k + 1 < crossings.size(); k += 2)
					{
						const int from = std::max(0, static_cast<int>(std::ceil((crossings[k] - minX) / cell - 0.5)));
						const int to = std::min(width - 1, static_cast<int>(std::floor((crossings[k + 1] - minX) / cell - 0.5)));
						for (int column = from; column <= to; ++column)
							cells[static_cast<size_t>(row) * static_cast<size_t>(width) + static_cast<size_t>(column)] = 1;
					}
				}
			}

			[[nodiscard]] bool Contains(const Point2 &p) const
			{
				if (cells.empty())
					return false;
				const int column = static_cast<int>(std::floor((p.x - minX) / cell)), row = static_cast<int>(std::floor((p.z - minZ) / cell));
				return column >= 0 && row >= 0 && column < width && row < height && cells[static_cast<size_t>(row) * static_cast<size_t>(width) + static_cast<size_t>(column)] != 0;
			}
		};

		/* @brief A tile's own scenery in the area (its base mesh DSF's overlay placements), filtered once at load. */
		struct SceneryTile
		{
			int lat = 0, lon = 0;
			fs::path file;
			DsfFile dsf;							// Placements and definitions only (no terrain, no rasters)
			std::vector<DsfExclusion> exclusions;	// Of the higher-priority overlays of the tile
			uint32_t excluded = 0;
			uint32_t unsupported = 0;
		};

		/* @brief DSF files read ahead, unpacked, by path: Load() reads them from here instead of the disk. */
		using ReadAhead = std::map<fs::path, std::shared_ptr<const std::vector<uint8_t>>>;

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
			double fallbackElevation = 0.0;					// Load()'s, for loading again in another season
			uint8_t season = 0;								// The library season the art was resolved for
			uint64_t generation = 0;
			SceneTerrain::Stats stats;
			ReadAhead readAhead;							// ReadSeasonRasters()' files, for the next Load()

			// The tile's scenery
			std::string name;
			TerrainArea area;
			std::vector<std::vector<Point2>> grounds;		// The airport's boundary rings
			GroundsMask groundsMask;
			std::vector<SceneryTile> sceneryTiles;
			std::unique_ptr<OverlayScene> scenery;
			bool sceneryShown = false;
			bool sceneryAutogen = false;					// What the scenery was built with
			bool sceneryTrees3d = false;

			// Water parts (plan Phase 11): X-Plane's water (Renderer::Pass_Water) or, with the water switch off, the flat
			// surface of Phase 10
			struct WaterPart
			{
				uint64_t entity = 0;
				Ref<MaterialAsset> water;
				Ref<MaterialAsset> flat;
			};
			std::vector<WaterPart> waterParts;
			bool waterShown = true;							// Which of the two the parts have
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

		/* @brief "spring" ... "winter" for a library season bit, "unseasoned" for none. */
		std::string SeasonArtName(const uint8_t bits)
		{
			for (uint8_t s = 0; s < 4; ++s)
			{
				const Environment::Season season = static_cast<Environment::Season>(s);
				if (bits == Environment::GetSeasonBit(season))
				{
					std::string name = Environment::GetSeasonName(season);
					name[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(name[0])));
					return name;
				}
			}
			return "unseasoned";
		}

		/**
		 * @brief A tile's DSFs in the enabled scenery packs, highest priority first (custom scenery in scenery_packs.ini
		 * order, then the Global Scenery folders; the demo areas last).
		 */
		std::vector<fs::path> FindTileFiles(const int lat, const int lon)
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

			std::vector<fs::path> files;
			for (const LibraryPackage *package : packages)
			{
				if (std::optional<fs::path> found = VFS::FindCaseInsensitive(GetDsfTilePath(package->path, lat + 0.5, lon + 0.5)))
					files.push_back(std::move(*found));
			}
			return files;
		}

		/**
		 * @brief The DSF holding a tile's base mesh: the first of its files (FindTileFiles) that isn't an overlay. Reads it
		 * with terrain and rasters (limited to the area), and collects the exclusion zones of the overlays above it (Global
		 * Airports, custom scenery). Files read ahead are read from there.
		 */
		bool ReadBaseMesh(const int lat, const int lon, const DsfReadOptions &options, const ReadAhead &readAhead, DsfFile &out, fs::path &file,
						  std::vector<DsfExclusion> &exclusions)
		{
			for (const fs::path &found : FindTileFiles(lat, lon))
			{
				DsfFile dsf;
				std::string error;
				const auto ahead = readAhead.find(found);
				const bool read = ahead != readAhead.end() ? ReadDsf(ahead->second->data(), ahead->second->size(), dsf, options, &error) : LoadDsf(found, dsf, options, &error);
				if (!read)
				{
					SEDX_CORE_WARN_TAG(LOG_TAG, "{}: {}", VFS::ToUtf8(found), error);
					continue;
				}
				if (dsf.IsOverlay())
				{
					for (DsfExclusion &zone : dsf.GetExclusionZones())
						exclusions.push_back(std::move(zone));
					continue;
				}
				out = std::move(dsf);
				file = found;
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
				ImageResource *image = texture(decal.texture, decal.normal ? TextureUsage::Normal : TextureUsage::Decal, base);
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
					SetDecalKeys(*material, slot, decal.keys);
				}
				if (slot == 0)
					material->SetProperty(decal.normal ? MaterialProperty::XpNormalDecalAspect : MaterialProperty::XpDecalAspect, perMeterT / std::max(1e-6f, perMeterS));
			}
			material->SetProperty(MaterialProperty::XpDrapeMetersU, 1.0f);
			material->SetProperty(MaterialProperty::XpDrapeMetersV, 1.0f);

			material->SetProperty(MaterialProperty::XpTerrainFlags, static_cast<float>(flags));
			return material;
		}

		/* @brief Where X-Plane keeps a tile's water colour map: Resources/bitmaps/world/water/<10 degree block>/<tile>.png. */
		fs::path WaterMapPath(const int lat, const int lon)
		{
			const auto block = [](const int degrees) { return static_cast<int>(std::floor(degrees / 10.0)) * 10; };
			char folder[16], tile[24];
			std::snprintf(folder, sizeof(folder), "%+03d%+04d", block(lat), block(lon));
			std::snprintf(tile, sizeof(tile), "%+03d%+04d.png", lat, lon);
			const fs::path water = LibraryManager::GetInstallPath() / "Resources" / "bitmaps" / "world" / "water";
			if (std::optional<fs::path> found = VFS::FindCaseInsensitive(water / folder / tile))
				return *found;
			return VFS::FindCaseInsensitive(water / "any.png").value_or(fs::path());	// Open water, for tiles without a map
		}

		/* @brief The sea-floor decal X-Plane exports for a tile's region (lib/g10/decals/water.dcl: tropical or the rest of the world). */
		bool FindSeaFloorDecal(const int lat, const int lon, PolygonDecal &decal, fs::path &base)
		{
			const fs::path library = LibraryManager::ResolveAt("lib/g10/decals/water.dcl", lat + 0.5, lon + 0.5);
			DecalLibraryAsset decals;
			std::string error;
			if (library.empty() || !LoadDecalLibraryAsset(library, decals, &error))
				return false;
			for (const PolygonDecal &candidate : decals.decals)
			{
				if (!candidate.normal && !candidate.texture.empty())
				{
					decal = candidate;
					base = library.parent_path();
					return true;
				}
			}
			return false;
		}

		/**
		 * @brief X-Plane's water for a tile's water parts (water.slang, drawn by Renderer::Pass_Water): the tile's water map
		 * over the frame (u east from the tile's west edge, v south from its north edge, a degree each), the sea floor's
		 * decal, the open-water colour where the map is missing.
		 */
		Ref<MaterialAsset> MakeWaterMaterial(const State &state, const int lat, const int lon, ImageResource *map, ImageResource *floorDecal, const float floorStrength)
		{
			Ref<MaterialAsset> material = CreateRef<MaterialAsset>();
			char name[48];
			std::snprintf(name, sizeof(name), "Water %+03d%+04d", lat, lon);
			material->SetName(name);
			material->SetProperty(MaterialProperty::Water, 1.0f);
			material->SetProperty(MaterialClass::ColorR, SrgbToLinear(WATER_SRGB[0]));
			material->SetProperty(MaterialClass::ColorG, SrgbToLinear(WATER_SRGB[1]));
			material->SetProperty(MaterialClass::ColorB, SrgbToLinear(WATER_SRGB[2]));
			material->SetProperty(MaterialClass::ColorA, WATER_CLARITY);
			material->SetProperty(MaterialClass::TextureTilingX, 1.0f);
			material->SetProperty(MaterialClass::TextureTilingY, 1.0f);
			material->SetProperty(MaterialProperty::CullMode, static_cast<float>(CullMode::None));
			if (map)
				material->SetTexture(MaterialTextureType::Color, map, 0);
			const GeoProjection &projection = state.projection;
			material->SetProperty(MaterialProperty::XpWaterMapScaleU, static_cast<float>(1.0 / projection.GetMetersPerDegreeLongitude()));
			material->SetProperty(MaterialProperty::XpWaterMapOffsetU, static_cast<float>(projection.GetDatumLongitude() - lon));
			material->SetProperty(MaterialProperty::XpWaterMapScaleV, static_cast<float>(1.0 / projection.GetMetersPerDegreeLatitude()));
			material->SetProperty(MaterialProperty::XpWaterMapOffsetV, static_cast<float>(lat + 1 - projection.GetDatumLatitude()));
			if (floorDecal)
			{
				material->SetTexture(MaterialTextureType::Color, floorDecal, 1);
				material->SetProperty(MaterialProperty::XpDecal0Scale, 1.0f / std::max(Renderer::GetEnvironmentConstants().water.floorDecalMeters, 0.1f));
				material->SetProperty(MaterialProperty::XpDecal0Strength, std::clamp(floorStrength, 0.0f, 1.0f));
			}
			return material;
		}

		/* @brief Water patches with the water switch off (Phase 10's look): a smooth surface in X-Plane's open water colour. */
		Ref<MaterialAsset> MakeFlatWaterMaterial()
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

		// -------------------------------------------------------
		// The tile's scenery

		/**
		 * @brief Drape heights of the tile's scenery: X-Plane's layer order, all of it above the terrain's border patches
		 * (0) and below every airport drape (the airport's lowest layer sits 8 mm up), which draws over it.
		 */
		float SceneryLayerHeight(const std::string_view group, const int offset)
		{
			return 0.001f + 0.0005f * static_cast<float>(GetLayerGroupOrder(group)) + 0.00004f * static_cast<float>(std::clamp(offset, -5, 5));
		}

		bool OnGrounds(const State &state, const Point2 &p)
		{
			return state.groundsMask.Contains(p);
		}

		std::string ExtensionOf(const std::string &path)
		{
			const size_t dot = path.find_last_of('.');
			std::string extension = dot == std::string::npos ? std::string() : path.substr(dot + 1);
			std::ranges::transform(extension, extension.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return extension;
		}

		/**
		 * @brief Leaves out of a tile's scenery what X-Plane wouldn't draw: placements with a point in an exclusion zone of
		 * their kind (forests are thinned tree by tree when built), buildings and objects on the airport's grounds or off the
		 * terrain, and what isn't drawn here (anything but forests, facades, beaches, lines, polygons, strings and autogen).
		 */
		void FilterScenery(SceneryTile &tile, const State &state)
		{
			DsfFile &dsf = tile.dsf;
			const auto zoned = [&tile](const std::string_view kind, const double longitude, const double latitude)
			{
				for (const DsfExclusion &zone : tile.exclusions)
				{
					if (zone.kind == kind && zone.Contains(longitude, latitude))
						return true;
				}
				return false;
			};
			const auto onTerrain = [&state](const double longitude, const double latitude)
			{
				const Point2 local = state.projection.ToLocal(latitude, longitude);
				float y = 0.0f;
				return state.surface.HeightAt(local.x, local.z, y);
			};

			std::erase_if(dsf.polygons, [&](const DsfPolygonPlacement &polygon)
			{
				const std::string extension = polygon.definition < dsf.polygonDefinitions.size() ? ExtensionOf(dsf.polygonDefinitions[polygon.definition]) : std::string();
				const bool autogen = extension == "ags" || extension == "agb";
				if (extension != "for" && extension != "fac" && extension != "bch" && extension != "lin" && extension != "pol" && extension != "str" && !autogen)
				{
					++tile.unsupported;
					return true;
				}
				if (extension == "for" || polygon.windings.empty() || polygon.windings[0].empty())
					return false;
				const bool building = extension == "fac";
				if (building && !onTerrain(polygon.windings[0][0].Longitude(), polygon.windings[0][0].Latitude()))
					return true;	// Off the terrain: nothing to stand on (not counted as excluded)
				for (const std::vector<DsfPoint> &winding : polygon.windings)
				{
					for (const DsfPoint &point : winding)
					{
						// ASSUMPTION: autogen goes with object and facade zones (its buildings are both); its tiles are kept off
						// the airport's grounds one by one when built
						const bool excluded = autogen ? zoned("obj", point.Longitude(), point.Latitude()) || zoned("fac", point.Longitude(), point.Latitude())
													  : zoned(extension, point.Longitude(), point.Latitude());
						if (excluded || (building && OnGrounds(state, state.projection.ToLocal(point.Latitude(), point.Longitude()))))
						{
							++tile.excluded;
							return true;
						}
					}
				}
				return false;
			});
			std::erase_if(dsf.objects, [&](const DsfObjectPlacement &object)
			{
				if (!onTerrain(object.longitude, object.latitude))
					return true;
				if (zoned("obj", object.longitude, object.latitude) || OnGrounds(state, state.projection.ToLocal(object.latitude, object.longitude)))
				{
					++tile.excluded;
					return true;
				}
				return false;
			});
			std::erase_if(dsf.roads, [&](const DsfRoadSegment &segment)
			{
				for (const DsfRoadPoint &point : segment.points)
				{
					if (zoned("net", point.longitude, point.latitude))
					{
						++tile.excluded;
						return true;
					}
				}
				return false;
			});
			dsf.polygons.shrink_to_fit();
			dsf.objects.shrink_to_fit();
			dsf.roads.shrink_to_fit();
		}

		void ClearScenery(State &state)
		{
			if (state.scenery)
				state.scenery->Clear();
			state.sceneryShown = false;
			state.stats.sceneryPlacements = 0;
			state.stats.sceneryExcluded = 0;
			state.stats.sceneryUnsupported = 0;
			state.stats.sceneryTriangles = 0;
			state.stats.sceneryObjects = 0;
			state.stats.sceneryMilliseconds = 0.0;
		}

		const char *GroupNameOf(const OverlayKind kind)
		{
			switch (kind)
			{
				case OverlayKind::Forest:			return "Forests";
				case OverlayKind::Road:				return "Roads";
				case OverlayKind::Beach:			return "Beaches";
				case OverlayKind::Facade:			return "Buildings";
				case OverlayKind::Line:				return "Lines";
				case OverlayKind::DrapedPolygon:	return "Draped polygons";
				default:							return GetOverlayKindName(kind);
			}
		}

		/* @brief Builds the tiles' scenery onto the terrain: one entity per kind (forests, roads, beaches ...). */
		void BuildScenery(State &state)
		{
			ClearScenery(state);
			state.sceneryShown = true;
			state.sceneryAutogen = SceneTerrain::IsAutogenEnabled();
			state.sceneryTrees3d = Renderer::GetFeatures().trees3d;
			if (state.surface.IsEmpty() || state.sceneryTiles.empty())
				return;
			const auto start = std::chrono::steady_clock::now();

			OverlayBuilder::Options options;
			for (const std::vector<Point2> &ring : state.area.rings)
			{
				for (const Point2 &point : ring)
					options.area.Add(point);
			}
			options.areaMargin = state.area.margin;
			options.layerHeight = &SceneryLayerHeight;
			options.drawAutogen = SceneTerrain::IsAutogenEnabled();
			// Autogen tiles, buildings and objects only on the terrain and off the airport's grounds
			options.keepAutogen = [&state](const Point2 &point)
			{
				float y = 0.0f;
				return state.surface.HeightAt(point.x, point.z, y) && !OnGrounds(state, point);
			};

			std::vector<OverlayBuild> builds;
			builds.reserve(state.sceneryTiles.size());
			double buildMilliseconds = 0.0;
			std::set<std::string> warnings;
			for (const SceneryTile &tile : state.sceneryTiles)
			{
				// Library paths resolve for the tile's region; others are relative to its scenery pack
				const fs::path pack = tile.file.parent_path().parent_path().parent_path();
				const AssetResolver resolver = [lat = tile.lat, lon = tile.lon, pack](const std::string_view path) -> fs::path
				{
					const fs::path relative{ std::string(path) };
					if (!relative.is_absolute())
					{
						if (fs::path file = LibraryManager::ResolveAt(path, lat + 0.5, lon + 0.5); !file.empty())
							return file;
					}
					return VFS::FindCaseInsensitive((pack / relative).lexically_normal()).value_or(fs::path());
				};
				// Trees only on the terrain, outside the airport's grounds and the forest exclusion zones (tree by tree)
				std::vector<const DsfExclusion *> forestZones;
				for (const DsfExclusion &zone : tile.exclusions)
				{
					if (zone.kind == "for")
						forestZones.push_back(&zone);
				}
				options.keepTree = [&state, &forestZones](const Point2 &point)
				{
					float y = 0.0f;
					if (!state.surface.HeightAt(point.x, point.z, y) || OnGrounds(state, point))
						return false;
					if (!forestZones.empty())
					{
						double latitude = 0.0, longitude = 0.0;
						state.projection.ToGeo(point, latitude, longitude);
						for (const DsfExclusion *zone : forestZones)
						{
							if (zone->Contains(longitude, latitude))
								return false;
						}
					}
					return true;
				};

				OverlayBuilder builder(resolver, state.projection);
				OverlayBuild &build = builds.emplace_back();
				const auto buildStart = std::chrono::steady_clock::now();
				builder.Build(tile.dsf, options, build);
				buildMilliseconds += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - buildStart).count();
				warnings.insert(build.warnings.begin(), build.warnings.end());
				state.stats.sceneryPlacements += static_cast<uint32_t>(tile.dsf.polygons.size() + tile.dsf.roads.size() + tile.dsf.objects.size());
				state.stats.sceneryExcluded += tile.excluded;
				state.stats.sceneryUnsupported += tile.unsupported;
			}
			options.keepTree = nullptr;

			// One entity per kind
			std::map<std::string, OverlayScene::Group> byName;
			for (const OverlayBuild &build : builds)
			{
				for (const OverlayMesh &mesh : build.meshes)
				{
					OverlayScene::Group &group = byName[GroupNameOf(mesh.kind)];
					group.meshes.push_back(&mesh);
				}
				for (const OverlayObject &object : build.objects)
					byName[object.source == OverlayObject::Source::Autogen ? "Autogen" : "Objects"].objects.push_back(&object);
			}
			std::vector<OverlayScene::Group> groups;
			for (auto &[groupName, group] : byName)
			{
				group.key = groupName;
				group.name = groupName;
				groups.push_back(std::move(group));
			}

			if (!state.scenery)
				state.scenery = std::make_unique<OverlayScene>();
			state.scenery->SetOnTerrain(true, true);
			state.scenery->Build("Terrain scenery " + state.name, groups);
			state.stats.sceneryTriangles = state.scenery->GetStats().triangles;
			state.stats.sceneryMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

			size_t shown = 0;
			for (const std::string &warning : warnings)
			{
				if (shown++ < 10)
					SEDX_CORE_WARN_TAG(LOG_TAG, "Tile scenery: {}", warning);
			}
			state.stats.sceneryObjects = state.scenery->GetStats().objects;
			SEDX_CORE_INFO_TAG(LOG_TAG, "{}: tile scenery: {} placements ({} excluded, {} not drawn), {} entities, {} triangles, {} objects in {:.0f} ms ({:.0f} ms building)", state.name,
				state.stats.sceneryPlacements, state.stats.sceneryExcluded, state.stats.sceneryUnsupported, groups.size(), state.stats.sceneryTriangles, state.stats.sceneryObjects,
				state.stats.sceneryMilliseconds, buildMilliseconds);
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
			state.waterParts.clear();
			state.mesh.Reset();
			state.materials.clear();
			state.textures.clear();
		}

		/* @brief Gives the water parts X-Plane's water or the flat surface, as the water switch asks. */
		void ShowWater(State &state, const bool water)
		{
			state.waterShown = water;
			entt::registry *registry = GetRegistry();
			if (!registry)
				return;
			for (const State::WaterPart &part : state.waterParts)
			{
				if (FindHandle(*registry, part.entity) == entt::null)
					continue;
				Entity *stored = Scene::GetStoredEntity(Scene::TryGetEntityWithUUID(UUID(part.entity)));
				if (Renderable *renderable = stored ? stored->GetComponent<Renderable>() : nullptr)
					renderable->SetMaterial(water ? part.water : part.flat);
			}
			Scene::MarkStructureChanged();
		}
	}

	// -------------------------------------------------------

	bool SceneTerrain::Load(const GeoProjection &projection, const TerrainArea &area, const std::string &name, const double fallbackElevation,
							const std::vector<std::vector<Point2>> &grounds)
	{
		State &state = GetState();
		const ReadAhead readAhead = std::exchange(state.readAhead, {});	// Dropped once this load is done
		if (!IsEnabled())
		{
			Unload();
			return false;
		}
		const uint8_t season = LibraryManager::GetSeason();
		const std::string areaKey = MakeAreaKey(projection, area) + "#" + std::to_string(LibraryManager::GetGeneration()) + "#s" + std::to_string(season);
		if (state.root != 0 && state.areaKey == areaKey)
		{
			if (entt::registry *registry = GetRegistry(); registry && FindHandle(*registry, state.root) != entt::null)
				return true;
		}
		Unload();
		state.areaKey = areaKey;
		state.projection = projection;
		state.season = season;
		state.fallbackElevation = fallbackElevation;

		const auto start = std::chrono::steady_clock::now();
		entt::registry *registry = GetRegistry();
		if (!registry || LibraryManager::GetIndex()->GetPackages().empty())
			return false;

		// The tiles under the area, read with their terrain, rasters and own scenery (overlay packs: just their exclusions)
		DsfReadOptions options;
		options.overlays = true;
		options.overlaysOnlyInBaseMesh = true;
		options.terrain = true;
		options.rasters = true;
		options.rasterNames = { "elevation", "sea_level", "spr1", "spr2", "sum1", "sum2", "fal1", "fal2", "win1", "win2" };
		area.GetGeoBox(projection, options.areaWest, options.areaSouth, options.areaEast, options.areaNorth);

		struct Tile
		{
			int lat = 0, lon = 0;
			DsfFile dsf;
			fs::path file;
			std::vector<DsfExclusion> exclusions;
		};
		std::vector<std::future<std::unique_ptr<Tile>>> reads;
		for (int lat = static_cast<int>(std::floor(options.areaSouth)); lat <= static_cast<int>(std::floor(options.areaNorth)); ++lat)
		{
			for (int lon = static_cast<int>(std::floor(options.areaWest)); lon <= static_cast<int>(std::floor(options.areaEast)); ++lon)
			{
				reads.push_back(std::async(std::launch::async, [lat, lon, options, &readAhead]() -> std::unique_ptr<Tile>
				{
					auto tile = std::make_unique<Tile>();
					tile->lat = lat;
					tile->lon = lon;
					return ReadBaseMesh(lat, lon, options, readAhead, tile->dsf, tile->file, tile->exclusions) ? std::move(tile) : nullptr;
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
		build.waterCellSize = Renderer::GetEnvironmentConstants().water.gridCellM;	// Room for water.slang's tessellation
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
		// Water: each tile's water map and its region's sea-floor decal
		struct TileWater
		{
			fs::path map;
			fs::path floorDecal;
			float floorStrength = 1.0f;
		};
		std::map<const Tile *, TileWater> tileWater;
		for (const TileParts &entry : built)
		{
			if (!std::ranges::any_of(entry.parts, [](const TerrainMeshPart &part) { return part.water && !part.indices.empty(); }))
				continue;
			TileWater &water = tileWater[entry.tile];
			water.map = WaterMapPath(entry.tile->lat, entry.tile->lon);
			if (!water.map.empty())
				request(water.map.parent_path(), water.map.filename().string(), TextureUsage::Albedo);
			PolygonDecal decal;
			fs::path base;
			if (FindSeaFloorDecal(entry.tile->lat, entry.tile->lon, decal, base))
			{
				water.floorDecal = TextureLoader::ResolveTexturePath(base, decal.texture);
				water.floorStrength = decal.strength;
				request(base, decal.texture, TextureUsage::Data);
			}
		}

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
							request(base, decal.texture, decal.normal ? TextureUsage::Normal : TextureUsage::Decal);
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
			Ref<MaterialAsset> flatWater;	// Water: the material with the water switch off
			bool border = false;
			bool water = false;
			uint32_t order = 0;
			uint32_t subMesh = 0;
		};
		const auto loadedTexture = [&state](const fs::path &file, const TextureUsage usage) -> ImageResource *
		{
			const auto it = file.empty() ? state.textures.end() : state.textures.find(TextureKey(file, usage));
			return it != state.textures.end() ? it->second.Get() : nullptr;
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
					auto flat = state.materials.find("water");
					if (flat == state.materials.end())
						flat = state.materials.emplace("water", MakeFlatWaterMaterial()).first;
					const std::string key = "water|" + std::to_string(entry.tile->lat) + "," + std::to_string(entry.tile->lon);
					auto it = state.materials.find(key);
					if (it == state.materials.end())
					{
						const TileWater &water = tileWater[entry.tile];
						it = state.materials.emplace(key, MakeWaterMaterial(state, entry.tile->lat, entry.tile->lon, loadedTexture(water.map, TextureUsage::Albedo),
							loadedTexture(water.floorDecal, TextureUsage::Data), water.floorStrength)).first;
					}
					item.material = it->second;
					item.flatWater = flat->second;
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
			if (item.water)
			{
				state.waterParts.push_back({ GetId(*registry, handle), item.material, item.flatWater });
				renderable->SetMaterial(Renderer::GetFeatures().water ? item.material : item.flatWater);
			}
			else
			{
				renderable->SetMaterial(item.material);
			}
			renderable->SetFlag(CastsShadows, !item.border && !item.water);
			// Border patches draw over the base in the DSF's order, below everything an airport drapes
			if (item.border)
				renderable->SetDrapeOrder(Renderable::MakeDrapeOrder(0.0f, item.order));
		}

		state.waterShown = Renderer::GetFeatures().water;

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
		SEDX_CORE_INFO_TAG(LOG_TAG, "{}: {} triangles in {} parts from {} ({} land classes, {} missing; {} textures; {} art), y = 0 at {:.1f} m MSL, in {:.0f} ms ({:.0f} ms reading, {} file{} read ahead)",
			name, triangles, drawn.size(), files, state.stats.terrainTypes, missing, state.stats.textures, SeasonArtName(season), state.datumElevation, state.stats.loadMilliseconds,
			readMilliseconds, readAhead.size(), readAhead.size() == 1 ? "" : "s");

		// The tiles' own scenery, kept filtered so its switch can show it again without reading the tiles
		state.name = name;
		state.area = area;
		state.grounds = grounds;
		state.groundsMask.Build(grounds);
		for (std::unique_ptr<Tile> &tile : tiles)
		{
			SceneryTile &scenery = state.sceneryTiles.emplace_back();
			scenery.lat = tile->lat;
			scenery.lon = tile->lon;
			scenery.file = tile->file;
			scenery.dsf = std::move(tile->dsf);
			scenery.dsf.terrainPatches = {};
			scenery.dsf.rasters = {};
			scenery.exclusions = std::move(tile->exclusions);
			FilterScenery(scenery, state);
		}
		if (IsSceneryEnabled())
			BuildScenery(state);
		++state.generation;
		Renderer::SetGroundShown(true);
		return true;
	}

	bool SceneTerrain::ReadSeasonRasters(const double latitude, const double longitude, std::array<float, 8> &days)
	{
		State &state = GetState();
		state.readAhead.clear();
		if (!LibraryManager::GetIndex())
			return false;

		DsfReadOptions options;
		options.overlays = false;
		options.rasters = true;
		options.rasterNames.assign(Environment::DSF_SEASON_RASTERS.begin(), Environment::DSF_SEASON_RASTERS.end());
		const int lat = static_cast<int>(std::floor(latitude)), lon = static_cast<int>(std::floor(longitude));
		for (const fs::path &file : FindTileFiles(lat, lon))
		{
			// Unpacked once: the rasters come from these bytes, and so does the terrain the next Load() builds
			std::string bytes, error;
			if (!VFS::ReadFile(file, bytes, &error))
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "{}: {}", VFS::ToUtf8(file), error);
				continue;
			}
			auto unpacked = std::make_shared<std::vector<uint8_t>>();
			const uint8_t *data = reinterpret_cast<const uint8_t *>(bytes.data());
			if (!Is7zArchive(data, bytes.size()))
				unpacked->assign(data, data + bytes.size());
			else if (!Extract7zFirstFile(data, bytes.size(), *unpacked, &error))
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "{}: the compressed DSF can't be unpacked: {}", VFS::ToUtf8(file), error);
				continue;
			}
			bytes = {};
			DsfFile dsf;
			if (!ReadDsf(unpacked->data(), unpacked->size(), dsf, options, &error))
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "{}: {}", VFS::ToUtf8(file), error);
				continue;
			}
			if (IsEnabled())
				state.readAhead.emplace(file, std::move(unpacked));
			if (dsf.IsOverlay())
				continue;

			// The base mesh: its rasters at the place
			for (size_t i = 0; i < days.size(); ++i)
			{
				const DsfRaster *raster = dsf.FindRaster(Environment::DSF_SEASON_RASTERS[i]);
				if (!raster)
					return false;
				days[i] = raster->SampleNearest(longitude, latitude, dsf.west, dsf.south, dsf.east, dsf.north);
			}
			return true;
		}
		return false;
	}

	void SceneTerrain::Unload()
	{
		State &state = GetState();
		state.readAhead.clear();
		if (state.root != 0 || !state.surface.IsEmpty())
			++state.generation;
		ClearScenery(state);
		state.sceneryTiles.clear();
		state.grounds.clear();
		state.groundsMask = GroundsMask();
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
		if (state.scenery && state.scenery->Contains(entityId))
			return true;
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

	bool SceneTerrain::IsSceneryEnabled()
	{
		return Renderer::GetFeatures().terrainScenery;
	}

	bool SceneTerrain::IsAutogenEnabled()
	{
		return Renderer::GetFeatures().autogen;
	}

	void SceneTerrain::Update()
	{
		State &state = GetState();
		// Another season: its terrain and scenery art (EXPORT_SEASON) in place of this one's, as X-Plane reloads them
		if (state.root != 0 && state.season != LibraryManager::GetSeason())
		{
			const GeoProjection projection = state.projection;
			const TerrainArea area = state.area;
			const std::string name = state.name;
			const std::vector<std::vector<Point2>> grounds = state.grounds;
			SEDX_CORE_INFO_TAG(LOG_TAG, "{}: the season changed, loading the terrain again", name);
			Load(projection, area, name, state.fallbackElevation, grounds);
		}
		if (Renderer::GetFeatures().water != state.waterShown)
			ShowWater(state, Renderer::GetFeatures().water);

		const bool wanted = IsSceneryEnabled() && state.root != 0;
		if (wanted == state.sceneryShown && (!wanted || (state.sceneryAutogen == IsAutogenEnabled() && state.sceneryTrees3d == Renderer::GetFeatures().trees3d)))
			return;
		if (wanted)
			BuildScenery(state);
		else
			ClearScenery(state);
	}

	void SceneTerrain::Shutdown()
	{
		Unload();
		GetState().scenery.reset();
	}
}

// -------------------------------------------------------
