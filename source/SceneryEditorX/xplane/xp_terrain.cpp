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
 * xp_terrain.cpp
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#include "xp_terrain.h"
#include "xp_text.h"
#include <algorithm>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		bool Fail(std::string *error, const std::string &message)
		{
			if (error)
				*error = message;
			return false;
		}

		/* @brief The rest of a line from a word on, backslashes turned into slashes. */
		std::string PathFrom(const std::string_view line, const size_t token)
		{
			std::string path(RestOfLine(line, token));
			std::ranges::replace(path, '\\', '/');
			return path;
		}

		float Number(const std::vector<std::string_view> &tokens, const size_t index, const float fallback)
		{
			return index < tokens.size() ? ParseFloatLenient(tokens[index], fallback) : fallback;
		}

		/* @brief AUTO_SLOPE_HILL / _CLIFF <projected s> <projected t> <angle start> <angle end> <texture>. */
		void ReadSlope(const std::vector<std::string_view> &tokens, const std::string_view line, TerrainSlopeLayer &out)
		{
			if (tokens.size() < 6)
				return;
			out.projectedS = Number(tokens, 1, 1000.0f);
			out.projectedT = Number(tokens, 2, 1000.0f);
			out.angleStart = Number(tokens, 3, 90.0f);
			out.angleEnd = Number(tokens, 4, 90.0f);
			out.texture = PathFrom(line, 5);
		}
	}

	// -------------------------------------------------------

	std::vector<std::string> TerrainAsset::GetTextures() const
	{
		std::vector<std::string> textures;
		const auto add = [&textures](const std::string &path)
		{
			if (!path.empty() && std::ranges::find(textures, path) == textures.end())
				textures.push_back(path);
		};
		add(texture);
		add(textureNormal);
		add(compositeTexture);
		add(compositeNoise);
		add(borderTexture);
		add(hill.texture);
		add(cliff.texture);
		add(tileMap);
		add(textureLit);
		add(farTexture);
		for (const PolygonDecal &decal : decals)
			add(decal.texture);
		add(textureControl);
		add(textureTerrain);
		add(textureModulator);
		add(seasonal);
		add(seasonalHill);
		add(seasonalCliff);
		add(seasonalFar);
		add(seasonalComposite);
		add(weather);
		add(waterColorMask);
		return textures;
	}

	bool ParseTerrainAsset(const std::string_view text, TerrainAsset &out, std::string *error)
	{
		out = TerrainAsset();
		AssetHeader header;
		const size_t start = ParseAssetHeader(text, header);
		if (start == std::string_view::npos)
			return Fail(error, "Not an X-Plane TERRAIN file (bad header)");
		if (header.type != "TERRAIN")
			return Fail(error, "Expected a TERRAIN file, found " + header.type);

		LineReader reader(text.substr(std::min(start, text.size())));
		std::string_view line;
		std::vector<std::string_view> tokens;
		while (reader.Next(line))
		{
			Tokenize(line, tokens);
			if (tokens.empty() || tokens[0].front() == '#')
				continue;
			const std::string_view key = tokens[0];

			// Base texture
			if (key == "BASE_TEX" || key == "TEXTURE" || key == "BASE_TEX_NOWRAP" || key == "TEXTURE_NOWRAP")
			{
				out.texture = PathFrom(line, 1);
				out.wrap = !key.ends_with("_NOWRAP");
			}
			else if (key == "PROJECTED" && tokens.size() >= 3)
			{
				out.projected = true;
				out.projectedS = Number(tokens, 1, 1000.0f);
				out.projectedT = Number(tokens, 2, 1000.0f);
			}
			else if (key == "PROJECT_ANGLE" && tokens.size() >= 5)
			{
				for (size_t i = 0; i < 4; ++i)
					out.projectAngle[i] = Number(tokens, i + 1, out.projectAngle[i]);
			}
			else if (key == "PROJECT_OFFSET" && tokens.size() >= 3)
			{
				out.projectOffset[0] = Number(tokens, 1, 0.0f);
				out.projectOffset[1] = Number(tokens, 2, 0.0f);
			}
			else if (key == "AUTO_HEADING")
				out.autoHeading = true;
			else if (key == "AUTO_VARY")
				out.autoVary = true;

			// Normal map and shading
			else if ((key == "NORMAL_TEX" || key == "TEXTURE_NORMAL" || key == "NORMAL_TEX_NOWRAP" || key == "TEXTURE_NORMAL_NOWRAP") && tokens.size() >= 2)
			{
				// <ratio> <file>, or (TEXTURE_NORMAL as OBJs write it) just <file>
				const float ratio = tokens.size() >= 3 ? ParseFloatLenient(tokens[1], -1.0f) : -1.0f;
				if (ratio > 0.0f)
				{
					out.normalRatio = ratio;
					out.textureNormal = PathFrom(line, 2);
				}
				else
				{
					out.normalRatio = 1.0f;
					out.textureNormal = PathFrom(line, 1);
				}
				out.normalWrap = !key.ends_with("_NOWRAP");
			}
			else if (key == "NORMAL_METALNESS")
				out.normalMetalness = true;
			else if (key == "BUMP_LEVEL" && tokens.size() >= 2)
				out.bumpLevel = Number(tokens, 1, 1.0f);
			else if (key == "SPECULAR" && tokens.size() >= 2)
				out.specular = Number(tokens, 1, 1.0f);
			else if (key == "SUPER_ROUGHNESS" && tokens.size() >= 2)
				out.superRoughness = std::clamp(Number(tokens, 1, 0.5f), 0.0f, 1.0f);
			else if (key == "NO_ALPHA")
				out.noAlpha = true;

			// Composite
			else if (key == "COMPOSITE_TEX" || key == "COMPOSITE_TEXTURE")
				out.compositeTexture = PathFrom(line, 1);
			else if (key == "COMPOSITE_PROJECTED" && tokens.size() >= 3)
			{
				out.compositeProjected = true;
				out.compositeProjectedS = Number(tokens, 1, 1000.0f);
				out.compositeProjectedT = Number(tokens, 2, 1000.0f);
			}
			else if (key == "COMPOSITE_PARAMS")
			{
				out.compositeParams.clear();
				for (size_t i = 1; i < tokens.size(); ++i)
					out.compositeParams.push_back(ParseFloatLenient(tokens[i], 0.0f));
			}
			else if (key == "COMPOSITE_NOISE")
				out.compositeNoise = PathFrom(line, 1);
			else if (key == "COMPOSITE_DETAIL")
				out.compositeDetail = PathFrom(line, 1);
			else if (key == "COMPOSITE_BORDERS")
				out.compositeBorders = true;
			else if (key == "BORDER_TEX" || key == "BORDER_TEX_NOWRAP")
				out.borderTexture = PathFrom(line, 1);

			// Slopes
			else if (key == "AUTO_SLOPE")
				out.autoSlope = true;
			else if (key == "AUTO_SLOPE_HEADING")
				out.autoSlopeHeading = true;
			else if (key == "AUTO_SLOPE_HILL")
				ReadSlope(tokens, line, out.hill);
			else if (key == "AUTO_SLOPE_CLIFF")
				ReadSlope(tokens, line, out.cliff);

			// Tile atlas
			else if (key == "TEXTURE_TILE" && tokens.size() >= 3)
			{
				out.tiled = true;
				out.tilesX = std::clamp(ParseInt(tokens[1], 1), 1, 64);
				out.tilesY = std::clamp(ParseInt(tokens[2], 1), 1, 64);
				if (tokens.size() >= 5)
				{
					out.tilePagesX = std::clamp(ParseInt(tokens[3], 0), 0, 4096);
					out.tilePagesY = std::clamp(ParseInt(tokens[4], 0), 0, 4096);
				}
				if (tokens.size() >= 6)
					out.tileMap = PathFrom(line, 5);
			}

			// Night lighting and far textures
			else if (key == "TEXTURE_LIT" || key == "LIT_TEX" || key == "TEXTURE_LIT_NOWRAP")
				out.textureLit = PathFrom(line, 1);
			else if (key == "LUMINANCE" && tokens.size() >= 2)
				out.luminance = std::clamp(Number(tokens, 1, 0.0f), 0.0f, 65530.0f);
			else if (key == "LIT_FADE_PARAMS" && tokens.size() >= 4)
			{
				out.hasLitFade = true;
				for (size_t i = 0; i < 3; ++i)
					out.litFade[i] = Number(tokens, i + 1, 0.0f);
			}
			else if (key == "FAR" && tokens.size() >= 5)
			{
				out.hasFar = true;
				out.farStart = Number(tokens, 1, 0.0f);
				out.farEnd = Number(tokens, 2, 0.0f);
				out.farRatio = Number(tokens, 3, 1.0f);
				out.farTexture = PathFrom(line, 4);
			}

			// Detailing
			else if (key == "DECAL_LIB" && tokens.size() >= 2)
				out.decalLibrary = PathFrom(line, 1);
			else if (key == "TEXTURE_CONTROL" && tokens.size() >= 2)
				out.textureControl = PathFrom(line, 1);
			else if (key == "TEXTURE_TERRAIN" && tokens.size() >= 2)
			{
				// <s> <t> <file> as shipped, or <file> as the spec shows
				float s = 0.0f, t = 0.0f;
				if (tokens.size() >= 4 && TryParseFloat(tokens[1], s) && TryParseFloat(tokens[2], t))
				{
					out.terrainScaleS = s;
					out.terrainScaleT = t;
					out.textureTerrain = PathFrom(line, 3);
				}
				else
					out.textureTerrain = PathFrom(line, 1);
			}
			else if (key == "TEXTURE_MODULATOR" && tokens.size() >= 2)
				out.textureModulator = PathFrom(line, 1);

			// Seasons
			else if (key == "SEASONAL")
				out.seasonal = PathFrom(line, 1);
			else if (key == "SEASONAL_HILL")
				out.seasonalHill = PathFrom(line, 1);
			else if (key == "SEASONAL_CLIFF")
				out.seasonalCliff = PathFrom(line, 1);
			else if (key == "SEASONAL_FAR")
				out.seasonalFar = PathFrom(line, 1);
			else if (key == "SEASONAL_COMPOSITE")
				out.seasonalComposite = PathFrom(line, 1);

			// Weather
			else if (key == "WEATHER" && tokens.size() >= 2)
				out.weather = PathFrom(line, 1);
			else if (key == "WEATHER_TRANSPARENT")
				out.weatherTransparent = true;
			else if (key == "SNOW_ALBEDO_LUMA" && tokens.size() >= 3)
			{
				out.hasSnowAlbedoLuma = true;
				for (size_t i = 0; i < 4; ++i)
					out.snowAlbedoLuma[i] = Number(tokens, i + 1, 0.0f);
			}

			// Water, physics and the rest
			else if (key == "WATER_COLOR_MASK" && tokens.size() >= 2)
				out.waterColorMask = PathFrom(line, 1);
			else if (key == "SURFACE" && tokens.size() >= 2)
				out.surface = std::string(tokens[1]);
			else if (key == "WET")
				out.wet = true;
			else if (key == "BUMPY")
				out.bumpy = true;
			else if (key == "DYNAMIC_TERRAIN")
				out.dynamic = true;
			else if (key == "CREASE_ANGLE" && tokens.size() >= 2)
				out.creaseAngle = Number(tokens, 1, -1.0f);
			else if (key == "PREVIEW_RGB" && tokens.size() >= 4)
			{
				out.hasPreview = true;
				for (size_t i = 0; i < 3; ++i)
					out.preview[i] = Number(tokens, i + 1, 0.0f);
			}
			else if (key == "LOAD_CENTER" && tokens.size() >= 5)
			{
				out.hasLoadCenter = true;
				for (size_t i = 0; i < 4; ++i)
					out.loadCenter[i] = Number(tokens, i + 1, 0.0f);
			}
			else
			{
				PolygonDecal decal;
				if (ParsePolygonDecalRow(tokens, decal))
					out.decals.push_back(std::move(decal));
				else if (std::ranges::find(out.unknown, key) == out.unknown.end())
					out.unknown.emplace_back(key);
			}
		}

		if (out.texture.empty())
			return Fail(error, "The terrain has no BASE_TEX");
		return true;
	}

	bool LoadTerrainAsset(const std::filesystem::path &path, TerrainAsset &out, std::string *error)
	{
		std::string text;
		if (!ReadTextFile(path, text))
			return Fail(error, "Could not read " + path.string());
		std::string parseError;
		if (!ParseTerrainAsset(text, out, &parseError))
			return Fail(error, path.filename().string() + ": " + parseError);
		return true;
	}

	bool IsDsfWaterTerrain(const std::string_view definition)
	{
		return definition == "terrain_Water" || definition == "water";
	}
}

// -------------------------------------------------------
