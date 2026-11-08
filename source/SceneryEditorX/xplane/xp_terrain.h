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
 * xp_terrain.h
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_assets.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * .ter terrain types (A 800 TERRAIN), the land classes the DSF base mesh is drawn with, per the Terrain Type spec
	 * (https://developer.x-plane.com/article/terrain-type-ter-file-format-specification/) plus every directive the X-Plane 12
	 * default scenery uses without documentation (SUPER_ROUGHNESS, 6 / 7 value COMPOSITE_PARAMS, COMPOSITE_NOISE,
	 * AUTO_SLOPE_*, FAR, LUMINANCE, LIT_FADE_PARAMS, SNOW_ALBEDO_LUMA, ...) and the ones the X-Plane 12 executable reads
	 * that no default terrain uses (SEASONAL*, WATER_COLOR_MASK, COMPOSITE_DETAIL, DYNAMIC_TERRAIN, ...). Numbers accept a
	 * decimal comma (432 shipped files write "20,000000"). Texture paths are relative to the .ter. Standard library only.
	 */

	/* @brief AUTO_SLOPE_HILL / AUTO_SLOPE_CLIFF: a texture taking over on slopes steeper than a range of angles. */
	struct TerrainSlopeLayer
	{
		std::string texture;
		float projectedS = 1000.0f;			// Meters per repeat
		float projectedT = 1000.0f;
		float angleStart = 90.0f;			// Degrees from horizontal where it starts / is complete
		float angleEnd = 90.0f;

		[[nodiscard]] bool IsUsed() const { return !texture.empty(); }
	};

	/* @brief A parsed .ter. */
	struct TerrainAsset
	{
		// Base texture: projected from above (PROJECTED meters per repeat) or mapped by the DSF's s / t
		std::string texture;				// BASE_TEX / TEXTURE
		bool wrap = true;					// *_NOWRAP: clamped (orthophotos)
		bool projected = false;				// PROJECTED
		float projectedS = 1000.0f;
		float projectedT = 1000.0f;
		float projectAngle[4] = { 0.0f, 1.0f, 0.0f, 0.0f };	// PROJECT_ANGLE: projection axis x y z, rotation degrees
		float projectOffset[2] = { 0.0f, 0.0f };			// PROJECT_OFFSET, meters
		bool autoHeading = false;			// AUTO_HEADING
		bool autoVary = false;				// AUTO_VARY (X-Plane 9 terrain: four variants by position)

		// Normal map and shading
		std::string textureNormal;			// NORMAL_TEX / TEXTURE_NORMAL <ratio> <file>
		float normalRatio = 1.0f;			// Normal map repeats per base repeat
		bool normalWrap = true;
		bool normalMetalness = false;		// NORMAL_METALNESS: blue is F0 / metalness
		float bumpLevel = 1.0f;				// BUMP_LEVEL
		float specular = -1.0f;				// SPECULAR <ratio>, -1 when absent
		float superRoughness = -1.0f;		// SUPER_ROUGHNESS, -1 when absent (0.5 on most X-Plane 12 terrain)
		bool noAlpha = false;				// NO_ALPHA

		// Composite: a second texture mixed in by a noise mask
		std::string compositeTexture;		// COMPOSITE_TEX
		bool compositeProjected = false;	// COMPOSITE_PROJECTED <s> <t>
		float compositeProjectedS = 1000.0f;
		float compositeProjectedT = 1000.0f;
		std::vector<float> compositeParams;	// COMPOSITE_PARAMS: 6 values (terrain10) or 7 (terrain12); meanings undocumented
		std::string compositeNoise;			// COMPOSITE_NOISE
		std::string compositeDetail;		// COMPOSITE_DETAIL (rest of the line)
		bool compositeBorders = false;		// COMPOSITE_BORDERS: borders fade through the noise (dithered), not a blend

		// Borders: overlay patches fade in by this mask, mapped by the patch's border s / t
		std::string borderTexture;			// BORDER_TEX

		// Slopes
		bool autoSlope = false;				// AUTO_SLOPE
		bool autoSlopeHeading = false;		// AUTO_SLOPE_HEADING: slope textures turn to face downhill
		TerrainSlopeLayer hill;				// AUTO_SLOPE_HILL
		TerrainSlopeLayer cliff;			// AUTO_SLOPE_CLIFF

		// Tile atlas: TEXTURE_TILE <x tiles> <y tiles> <x pages> <y pages> <map>
		bool tiled = false;
		int tilesX = 1;
		int tilesY = 1;
		int tilePagesX = 0;
		int tilePagesY = 0;
		std::string tileMap;

		// Night lighting
		std::string textureLit;				// TEXTURE_LIT / LIT_TEX
		float luminance = 0.0f;				// LUMINANCE: nits of a white lit texel (0: absent)
		bool hasLitFade = false;			// LIT_FADE_PARAMS <lit scale m> <fade start m> <fade end m>
		float litFade[3] = { 0.0f, 0.0f, 0.0f };

		// Far texture: FAR <start m> <end m> <texture scale ratio> <texture>
		bool hasFar = false;
		float farStart = 0.0f;
		float farEnd = 0.0f;
		float farRatio = 1.0f;
		std::string farTexture;

		// Detailing (the standard shading options)
		std::string decalLibrary;			// DECAL_LIB: a .dcl library path (its decals are in addition to the rows below)
		std::vector<PolygonDecal> decals;	// DECAL / DECAL_RGBA / DECAL_PARAMS[_PROJ] / NORMAL_DECAL_PARAMS[_PROJ] rows
		std::string textureControl;			// TEXTURE_CONTROL
		std::string textureTerrain;			// TEXTURE_TERRAIN [<s> <t>] <file>
		float terrainScaleS = 0.0f;
		float terrainScaleT = 0.0f;
		std::string textureModulator;		// TEXTURE_MODULATOR

		// Seasons (X-Plane 12 reads these; no default terrain uses them)
		std::string seasonal;				// SEASONAL
		std::string seasonalHill;			// SEASONAL_HILL
		std::string seasonalCliff;			// SEASONAL_CLIFF
		std::string seasonalFar;			// SEASONAL_FAR
		std::string seasonalComposite;		// SEASONAL_COMPOSITE

		// Weather
		std::string weather;				// WEATHER <mask>
		bool weatherTransparent = false;	// WEATHER_TRANSPARENT
		bool hasSnowAlbedoLuma = false;		// SNOW_ALBEDO_LUMA <4 values>
		float snowAlbedoLuma[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		// Water and the rest
		std::string waterColorMask;			// WATER_COLOR_MASK: the terrain is water
		std::string surface;				// SURFACE (physics)
		bool wet = false;					// WET (physics: behaves as water)
		bool bumpy = false;					// BUMPY (physics)
		bool dynamic = false;				// DYNAMIC_TERRAIN
		float creaseAngle = -1.0f;			// CREASE_ANGLE, -1 when absent
		bool hasPreview = false;			// PREVIEW_RGB <r> <g> <b>
		float preview[3] = { 0.0f, 0.0f, 0.0f };
		bool hasLoadCenter = false;			// LOAD_CENTER <lat> <lon> <size m> <pixels>
		double loadCenter[4] = { 0.0, 0.0, 0.0, 0.0 };

		std::vector<std::string> unknown;	// Directives this parser doesn't know (kept for diagnostics)

		/* @brief Every texture the terrain names (relative to the .ter), each once. Library paths (DECAL_LIB) aren't included. */
		[[nodiscard]] std::vector<std::string> GetTextures() const;
	};

	/* @brief Parses .ter text. Fails without a TERRAIN header or a base texture. */
	bool ParseTerrainAsset(std::string_view text, TerrainAsset &out, std::string *error = nullptr);

	bool LoadTerrainAsset(const std::filesystem::path &path, TerrainAsset &out, std::string *error = nullptr);

	/* @brief The virtual-path name of the DSF water mesh layer ("terrain_Water" or "water"): water triangles without a .ter. */
	bool IsDsfWaterTerrain(std::string_view definition);
}

// -------------------------------------------------------
