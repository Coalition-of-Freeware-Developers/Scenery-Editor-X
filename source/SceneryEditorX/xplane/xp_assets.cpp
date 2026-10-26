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
 * xp_assets.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_assets.h"
#include "xp_text.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

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

		/* @brief Backslashes from Windows authoring tools become forward slashes. */
		std::string CleanPath(std::string_view text)
		{
			std::string path(Trim(text));
			for (char &c : path)
			{
				if (c == '\\')
					c = '/';
			}
			return path;
		}

		/* @brief Iterates the rows after the header, tokenized, skipping blanks and comments. */
		template<typename Visitor>
		void ForEachRow(const std::string_view text, const size_t start, Visitor &&visitor)
		{
			LineReader reader(text.substr(std::min(start, text.size())));
			std::string_view line;
			std::vector<std::string_view> tokens;
			while (reader.Next(line))
			{
				Tokenize(line, tokens);
				if (tokens.empty() || tokens[0].front() == '#')
					continue;
				visitor(tokens, line);
			}
		}

		bool CheckHeader(const std::string_view text, const char *type, AssetHeader &header, size_t &start, std::string *error)
		{
			start = ParseAssetHeader(text, header);
			if (start == std::string_view::npos)
				return Fail(error, std::string("Not an X-Plane ") + type + " file (bad header)");
			if (header.type != type)
				return Fail(error, "Expected a " + std::string(type) + " file, found " + header.type);
			return true;
		}

		template<typename Asset>
		bool LoadAsset(const std::filesystem::path &path, Asset &out, std::string *error, bool (*parse)(std::string_view, Asset &, std::string *))
		{
			std::string text;
			if (!ReadTextFile(path, text))
				return Fail(error, "Could not read " + path.string());
			std::string parseError;
			if (!parse(text, out, &parseError))
				return Fail(error, path.filename().string() + ": " + parseError);
			return true;
		}

		/* @brief The largest of tokens[first, first + count) as floats (a decal row's key / constant group). */
		float MaxOf(const std::vector<std::string_view> &tokens, const size_t first, const size_t count)
		{
			float result = 0.0f;
			for (size_t i = first; i < first + count && i < tokens.size(); ++i)
				result = std::max(result, ParseFloat(tokens[i], 0.0f));
			return result;
		}

		/**
		 * @brief Reads a decal row of a .pol / .dcl into out; false for other rows. The forms:
		 *   DECAL <scale> <texture>, DECAL_RGBA <scale> <texture>
		 *   DECAL_PARAMS <scale> <dither> <6 RGB keys> <6 alpha keys> <texture>
		 *   DECAL_PARAMS_PROJ <scale s> <scale t> <dither> <6 RGB keys> <6 alpha keys> <texture>
		 *   NORMAL_DECAL_PARAMS <scale> <dither> <r g b a keys> <constant> <texture> <gain>
		 *   NORMAL_DECAL_PARAMS_PROJ <scale s> <scale t> <dither> <r g b a keys> <constant> <texture> <gain>
		 * Each key group is four albedo keys, a control key and a constant; their largest value is the strength.
		 */
		/* @brief A key group of a DECAL_PARAMS row: R G B A weights, then two terms X-Plane adds (modulator, constant). */
		void ParseKeyGroup(const std::vector<std::string_view> &tokens, const size_t first, float (&key)[4], float &bias)
		{
			for (size_t i = 0; i < 4; ++i)
				key[i] = ParseFloat(tokens[first + i], 0.0f);
			bias = ParseFloat(tokens[first + 4], 0.0f) + ParseFloat(tokens[first + 5], 0.0f);
		}

		bool ParseDecalRow(const std::vector<std::string_view> &tokens, PolygonDecal &out)
		{
			const std::string_view key = tokens[0];
			out = PolygonDecal();
			if ((key == "DECAL" || key == "DECAL_RGBA") && tokens.size() >= 3)
			{
				// Without keys: the RGB detail in full, and with DECAL_RGBA the alpha modulation too
				out.scaleS = out.scaleT = ParseFloat(tokens[1], 1.0f);
				out.keys.alphaBias = key == "DECAL_RGBA" ? 1.0f : 0.0f;
				out.texture = CleanPath(tokens[2]);
			}
			else if (key == "DECAL_PARAMS" && tokens.size() >= 16)
			{
				// DECAL_PARAMS <scale> <dither> <RGB key: r g b a modulator constant> <alpha key: same> <file>
				out.scaleS = out.scaleT = ParseFloat(tokens[1], 1.0f);
				ParseKeyGroup(tokens, 3, out.keys.rgb, out.keys.rgbBias);
				ParseKeyGroup(tokens, 9, out.keys.alpha, out.keys.alphaBias);
				out.strength = out.keys.GetMaxStrength();
				out.texture = CleanPath(tokens[15]);
			}
			else if (key == "DECAL_PARAMS_PROJ" && tokens.size() >= 17)
			{
				out.scaleS = ParseFloat(tokens[1], 1.0f);
				out.scaleT = ParseFloat(tokens[2], 1.0f);
				ParseKeyGroup(tokens, 4, out.keys.rgb, out.keys.rgbBias);
				ParseKeyGroup(tokens, 10, out.keys.alpha, out.keys.alphaBias);
				out.strength = out.keys.GetMaxStrength();
				out.texture = CleanPath(tokens[16]);
			}
			else if (key == "NORMAL_DECAL_PARAMS" && tokens.size() >= 9)
			{
				out.normal = true;
				out.scaleS = out.scaleT = ParseFloat(tokens[1], 1.0f);
				out.strength = MaxOf(tokens, 3, 5);
				out.texture = CleanPath(tokens[8]);
				out.gain = tokens.size() >= 10 ? ParseFloat(tokens[9], 1.0f) : 1.0f;
			}
			else if (key == "NORMAL_DECAL_PARAMS_PROJ" && tokens.size() >= 10)
			{
				out.normal = true;
				out.scaleS = ParseFloat(tokens[1], 1.0f);
				out.scaleT = ParseFloat(tokens[2], 1.0f);
				out.strength = MaxOf(tokens, 4, 5);
				out.texture = CleanPath(tokens[9]);
				out.gain = tokens.size() >= 11 ? ParseFloat(tokens[10], 1.0f) : 1.0f;
			}
			else
				return false;
			out.strength = std::clamp(out.strength, 0.0f, 1.0f);
			return !out.texture.empty() && out.scaleS > 0.0f && out.scaleT > 0.0f;
		}
	}

	// -------------------------------------------------------

	float DecalKeys::GetMaxStrength() const
	{
		// The texel's channels run 0..1: the most a key reaches is its bias plus its positive weights
		const auto reach = [](const float (&key)[4], const float bias)
		{
			float most = bias;
			for (const float weight : key)
				most += std::max(weight, 0.0f);
			return std::clamp(most, 0.0f, 1.0f);
		};
		return std::max(reach(rgb, rgbBias), reach(alpha, alphaBias));
	}

	bool ParsePolygonDecalRow(const std::vector<std::string_view> &tokens, PolygonDecal &out)
	{
		return !tokens.empty() && ParseDecalRow(tokens, out);
	}

	size_t ParseAssetHeader(const std::string_view text, AssetHeader &out)
	{
		out = AssetHeader();
		LineReader reader(text);
		std::string_view line;
		int step = 0;
		while (reader.Next(line))
		{
			std::string_view trimmed = Trim(line);
			if (trimmed.size() >= 3 && static_cast<unsigned char>(trimmed[0]) == 0xEF && static_cast<unsigned char>(trimmed[1]) == 0xBB && static_cast<unsigned char>(trimmed[2]) == 0xBF)
				trimmed.remove_prefix(3);
			if (trimmed.empty())
				continue;

			if (step == 0)
			{
				if (trimmed != "I" && trimmed != "A")
					return std::string_view::npos;
				step = 1;
			}
			else if (step == 1)
			{
				std::vector<std::string_view> tokens;
				Tokenize(trimmed, tokens);
				out.version = tokens.empty() ? 0 : ParseInt(tokens[0], 0);
				if (out.version <= 0)
					return std::string_view::npos;
				step = 2;
			}
			else
			{
				std::vector<std::string_view> tokens;
				Tokenize(trimmed, tokens);
				out.type = std::string(tokens.empty() ? trimmed : tokens[0]);
				return static_cast<size_t>(line.data() - text.data()) + line.size();
			}
		}
		return std::string_view::npos;
	}

	// -------------------------------------------------------

	bool ParsePolygonAsset(const std::string_view text, PolygonAsset &out, std::string *error)
	{
		out = PolygonAsset();
		AssetHeader header;
		size_t start = 0;
		if (!CheckHeader(text, "DRAPED_POLYGON", header, start, error))
			return false;

		ForEachRow(text, start, [&](const std::vector<std::string_view> &tokens, const std::string_view line)
		{
			const std::string_view key = tokens[0];
			if (key == "TEXTURE")
			{
				out.texture = CleanPath(RestOfLine(line, 1));
				out.wrap = true;
			}
			else if (key == "TEXTURE_NOWRAP")
			{
				out.texture = CleanPath(RestOfLine(line, 1));
				out.wrap = false;
			}
			else if (key == "TEXTURE_NORMAL" && tokens.size() >= 3)
			{
				out.normalRatio = ParseFloat(tokens[1], 1.0f);
				out.textureNormal = CleanPath(RestOfLine(line, 2));
			}
			else if (key == "NORMAL_METALNESS")
				out.normalMetalness = true;
			else if (key == "SPECULAR" && tokens.size() >= 2)
				out.specular = ParseFloat(tokens[1], 1.0f);
			else if (key == "SUPER_ROUGHNESS" && tokens.size() >= 2)
				out.superRoughness = ParseFloat(tokens[1], 0.0f);
			else if (key == "SCALE" && tokens.size() >= 3)
			{
				out.scaleS = ParseFloat(tokens[1], 1000.0f);
				out.scaleT = ParseFloat(tokens[2], 1000.0f);
			}
			else if (key == "NO_ALPHA")
				out.noAlpha = true;
			else if (key == "NO_BLEND")
				out.noBlendCutoff = tokens.size() >= 2 ? ParseFloat(tokens[1], 0.5f) : 0.5f;
			else if (key == "LAYER_GROUP" && tokens.size() >= 2)
			{
				out.layerGroup = std::string(tokens[1]);
				out.layerOffset = tokens.size() >= 3 ? ParseInt(tokens[2], 0) : 0;
			}
			else if (key == "SURFACE" && tokens.size() >= 2)
				out.surface = std::string(tokens[1]);
			else if (key == "DECAL_LIB" && tokens.size() >= 2)
				out.decal = std::string(RestOfLine(line, 1));
			else if (key == "TEXTURE_TILE" || key == "RUNWAY_TILE")
			{
				// TEXTURE_TILE <x tiles> <y tiles> <x pixels> <y pixels>; RUNWAY_TILE <x tiles> <y tiles> <map width> <map height> <map>
				out.tiled = true;
				if (tokens.size() >= 3)
				{
					out.tilesX = std::clamp(ParseInt(tokens[1], 1), 1, 64);
					out.tilesY = std::clamp(ParseInt(tokens[2], 1), 1, 64);
				}
				if (key == "RUNWAY_TILE")
				{
					out.runwayTile = true;
					if (tokens.size() >= 5)
					{
						out.tileMapWidth = std::clamp(ParseInt(tokens[3], 0), 0, 4096);
						out.tileMapHeight = std::clamp(ParseInt(tokens[4], 0), 0, 4096);
					}
					if (tokens.size() >= 6)
						out.tileMap = CleanPath(RestOfLine(line, 5));
				}
			}
			else if (key == "RUNWAY_NOISE_PARAMS")
			{
				for (size_t i = 0; i < 5 && i + 1 < tokens.size(); ++i)
					out.noiseParams[i] = ParseFloat(tokens[i + 1], 0.0f);
			}
			else if (key == "RUNWAY_MARKINGS" && tokens.size() >= 6)
			{
				for (size_t i = 0; i < 4; ++i)
					out.runwayMarkingsColor[i] = std::clamp(ParseFloat(tokens[i + 1], 1.0f), 0.0f, 1.0f);
				out.runwayMarkings = CleanPath(RestOfLine(line, 5));
			}
			else if (key == "WEATHER" && tokens.size() >= 2)
				out.weather = CleanPath(RestOfLine(line, 1));
			else if (key == "WEATHER_TRANSPARENT")
				out.weatherTransparent = true;
			else if (key == "SNOW_ALBEDO_LUMA" && tokens.size() >= 3)
			{
				out.hasSnowAlbedoLuma = true;
				out.snowAlbedoLuma[0] = ParseFloat(tokens[1], 0.0f);
				out.snowAlbedoLuma[1] = ParseFloat(tokens[2], 0.0f);
			}
			else
			{
				PolygonDecal decal;
				if (ParseDecalRow(tokens, decal))
					out.decals.push_back(std::move(decal));
			}
		});

		if (out.texture.empty())
			return Fail(error, "The polygon has no TEXTURE");
		return true;
	}

	bool LoadPolygonAsset(const std::filesystem::path &path, PolygonAsset &out, std::string *error)
	{
		return LoadAsset(path, out, error, &ParsePolygonAsset);
	}

	bool ParseDecalLibraryAsset(const std::string_view text, DecalLibraryAsset &out, std::string *error)
	{
		out = DecalLibraryAsset();
		AssetHeader header;
		size_t start = 0;
		if (!CheckHeader(text, "DECAL", header, start, error))
			return false;
		ForEachRow(text, start, [&](const std::vector<std::string_view> &tokens, const std::string_view)
		{
			PolygonDecal decal;
			if (ParseDecalRow(tokens, decal))
				out.decals.push_back(std::move(decal));
		});
		return true;
	}

	bool LoadDecalLibraryAsset(const std::filesystem::path &path, DecalLibraryAsset &out, std::string *error)
	{
		return LoadAsset(path, out, error, &ParseDecalLibraryAsset);
	}

	// -------------------------------------------------------

	float LineAsset::GetWidthMeters(const size_t segment) const
	{
		if (segment >= segments.size() || textureWidth <= 0.0f)
			return 0.5f;
		const Segment &s = segments[segment];
		return std::max(0.05f, (s.s2 - s.s1 - 2.0f / textureWidth) * scaleS);
	}

	bool ParseLineAsset(const std::string_view text, LineAsset &out, std::string *error)
	{
		out = LineAsset();
		AssetHeader header;
		size_t start = 0;
		if (!CheckHeader(text, "LINE_PAINT", header, start, error))
			return false;

		struct RawSegment
		{
			int layer;
			float s1, sm, s2;
		};
		std::vector<RawSegment> rawSegments;
		struct RawCap
		{
			float s1, sm, s2, t1, t2;
		};
		std::vector<RawCap> rawStart, rawEnd;

		ForEachRow(text, start, [&](const std::vector<std::string_view> &tokens, const std::string_view line)
		{
			const std::string_view key = tokens[0];
			if (key == "TEXTURE")
				out.texture = CleanPath(RestOfLine(line, 1));
			else if (key == "TEXTURE_NORMAL" && tokens.size() >= 3)
				out.textureNormal = CleanPath(RestOfLine(line, 2));
			else if (key == "SCALE" && tokens.size() >= 3)
			{
				out.scaleS = ParseFloat(tokens[1], 100.0f);
				out.scaleT = ParseFloat(tokens[2], 100.0f);
			}
			else if (key == "TEX_WIDTH" && tokens.size() >= 2)
				out.textureWidth = ParseFloat(tokens[1], 1024.0f);
			else if (key == "TEX_HEIGHT" && tokens.size() >= 2)
				out.textureHeight = ParseFloat(tokens[1], 1024.0f);
			else if (key == "S_OFFSET" && tokens.size() >= 5)
				rawSegments.push_back({ ParseInt(tokens[1], 0), ParseFloat(tokens[2]), ParseFloat(tokens[3]), ParseFloat(tokens[4]) });
			else if ((key == "START_CAP" || key == "END_CAP") && tokens.size() >= 7)
				(key == "START_CAP" ? rawStart : rawEnd).push_back({ ParseFloat(tokens[2]), ParseFloat(tokens[3]), ParseFloat(tokens[4]), ParseFloat(tokens[5]), ParseFloat(tokens[6]) });
			else if (key == "MIRROR")
				out.mirror = true;
			else if (key == "ALIGN" && tokens.size() >= 2)
				out.align = ParseInt(tokens[1], 0);
			else if (key == "LAYER_GROUP" && tokens.size() >= 2)
			{
				out.layerGroup = std::string(tokens[1]);
				out.layerOffset = tokens.size() >= 3 ? ParseInt(tokens[2], 0) : 0;
			}
			else if (key == "DECAL_LIB" && tokens.size() >= 2)
				out.decal = std::string(RestOfLine(line, 1));
		});

		if (out.texture.empty())
			return Fail(error, "The line has no TEXTURE");
		if (rawSegments.empty())
			return Fail(error, "The line has no S_OFFSET rows");

		const float width = out.textureWidth > 0.0f ? out.textureWidth : 1024.0f;
		const float height = out.textureHeight > 0.0f ? out.textureHeight : 1024.0f;
		for (const RawSegment &raw : rawSegments)
		{
			if (raw.s2 <= raw.s1)
				continue;
			out.segments.push_back({ raw.layer, raw.s1 / width, raw.sm / width, raw.s2 / width });
		}
		for (const RawCap &raw : rawStart)
			out.startCaps.push_back({ raw.s1 / width, raw.sm / width, raw.s2 / width, raw.t1 / height, raw.t2 / height });
		for (const RawCap &raw : rawEnd)
			out.endCaps.push_back({ raw.s1 / width, raw.sm / width, raw.s2 / width, raw.t1 / height, raw.t2 / height });
		if (out.segments.empty())
			return Fail(error, "The line's S_OFFSET rows are empty");
		return true;
	}

	bool LoadLineAsset(const std::filesystem::path &path, LineAsset &out, std::string *error)
	{
		return LoadAsset(path, out, error, &ParseLineAsset);
	}

	// -------------------------------------------------------

	bool ParseObjectStringAsset(const std::string_view text, ObjectStringAsset &out, std::string *error)
	{
		out = ObjectStringAsset();
		AssetHeader header;
		size_t start = 0;
		if (!CheckHeader(text, "OBJECT_STRING", header, start, error))
			return false;

		ForEachRow(text, start, [&](const std::vector<std::string_view> &tokens, const std::string_view line)
		{
			const std::string_view key = tokens[0];
			if (key == "OBJECT" && tokens.size() >= 4)
			{
				ObjectStringAsset::Entry entry;
				entry.rotationMin = ParseFloat(tokens[1]);
				entry.rotationMax = ParseFloat(tokens[2]);
				entry.object = CleanPath(RestOfLine(line, 3));
				out.objects.push_back(std::move(entry));
			}
			else if (key == "OFFSET" && tokens.size() >= 2)
				out.offset = ParseFloat(tokens[1]);
		});

		if (out.objects.empty())
			return Fail(error, "The object string has no OBJECT rows");
		return true;
	}

	bool LoadObjectStringAsset(const std::filesystem::path &path, ObjectStringAsset &out, std::string *error)
	{
		return LoadAsset(path, out, error, &ParseObjectStringAsset);
	}

	// -------------------------------------------------------

	bool ParseForestAsset(const std::string_view text, ForestAsset &out, std::string *error)
	{
		out = ForestAsset();
		AssetHeader header;
		size_t start = 0;
		if (!CheckHeader(text, "FOREST", header, start, error))
			return false;

		bool shader2d = true;
		ForestAsset::Mesh *mesh = nullptr;			// The MESH whose VERTEX / IDX rows follow
		std::unordered_map<std::string, int> meshByName;
		ForEachRow(text, start, [&](const std::vector<std::string_view> &tokens, const std::string_view line)
		{
			const std::string_view key = tokens[0];
			if (key == "VERTEX" && mesh)
			{
				// x y z, nx ny nz, s t, three wind weights (not kept)
				if (tokens.size() >= 9)
				{
					for (size_t i = 1; i <= 8; ++i)
						mesh->vertices.push_back(ParseFloat(tokens[i]));
				}
				return;
			}
			if (key == "IDX" && mesh)
			{
				for (size_t i = 1; i < tokens.size(); ++i)
					mesh->indices.push_back(static_cast<uint32_t>(std::max(0, ParseInt(tokens[i], 0))));
				return;
			}
			mesh = nullptr;

			ForestAsset::Material &material = shader2d ? out.material2d : out.material3d;
			if (key == "MESH" && tokens.size() >= 6)
			{
				ForestAsset::Mesh &added = out.meshes.emplace_back();
				added.name = std::string(tokens[1]);
				added.lodNear = ParseFloat(tokens[2], 0.0f);
				added.lodFar = ParseFloat(tokens[3], 500.0f);
				added.vertices.reserve(static_cast<size_t>(std::clamp(ParseInt(tokens[4], 0), 0, 1 << 20)) * 8);
				added.indices.reserve(static_cast<size_t>(std::clamp(ParseInt(tokens[5], 0), 0, 1 << 22)));
				added.noShadow = tokens.size() >= 10 && tokens[9] == "NO_SHADOW";
				meshByName[added.name] = static_cast<int>(out.meshes.size() - 1);
				mesh = &added;
			}
			else if (key == "MESH_3D" && tokens.size() >= 2)
			{
				// Adds a 3D LOD step to the TREE / TREE2 above it
				const auto it = meshByName.find(std::string(tokens[1]));
				if (it != meshByName.end() && !out.trees.empty())
					out.trees.back().meshes3d.push_back(it->second);
			}
			else if (key == "TEXTURE")
			{
				material.texture = CleanPath(RestOfLine(line, 1));
				if (shader2d)
					out.texture = material.texture;
			}
			else if (key == "TEXTURE_NORMAL" && tokens.size() >= 3)
			{
				material.normalRatio = ParseFloat(tokens[1], 1.0f);
				material.textureNormal = CleanPath(RestOfLine(line, 2));
			}
			else if (key == "NORMAL_METALNESS")
				material.normalMetalness = true;
			else if (key == "NORMAL_TRANSLUCENCY")
				material.normalTranslucency = true;
			else if (key == "WEATHER" && tokens.size() >= 2)
				material.weather = CleanPath(RestOfLine(line, 1));
			else if (key == "BLEND_HASH")
				material.blendHash = tokens.size() >= 2 ? ParseFloat(tokens[1], 0.5f) : 0.5f;
			else if (key == "SUPER_ROUGHNESS" && tokens.size() >= 2)
				material.superRoughness = ParseFloat(tokens[1], 0.0f);
			else if (key == "SHADER_2D")
				shader2d = true;
			else if (key == "SHADER_3D")
				shader2d = false;
			else if (key == "SCALE_X" && tokens.size() >= 2)
				out.scaleX = ParseFloat(tokens[1], 256.0f);
			else if (key == "SCALE_Y" && tokens.size() >= 2)
				out.scaleY = ParseFloat(tokens[1], 256.0f);
			else if (key == "SPACING" && tokens.size() >= 3)
			{
				out.spacingX = ParseFloat(tokens[1], 30.0f);
				out.spacingY = ParseFloat(tokens[2], 30.0f);
			}
			else if (key == "RANDOM" && tokens.size() >= 3)
			{
				out.randomX = ParseFloat(tokens[1]);
				out.randomY = ParseFloat(tokens[2]);
			}
			else if ((key == "TREE" || key == "TREE2") && tokens.size() >= 11)
			{
				// TREE s t w y offset percent min max [TREE2: two more] quads layer
				ForestAsset::Tree tree;
				tree.s = ParseFloat(tokens[1]);
				tree.t = ParseFloat(tokens[2]);
				tree.width = ParseFloat(tokens[3]);
				tree.height = ParseFloat(tokens[4]);
				tree.offset = ParseFloat(tokens[5]);
				tree.percent = ParseFloat(tokens[6], 100.0f);
				tree.minHeight = ParseFloat(tokens[7], 10.0f);
				tree.maxHeight = ParseFloat(tokens[8], 20.0f);
				size_t next = 9;
				if (key == "TREE2")
				{
					tree.nominalHeight = ParseFloat(tokens[9], 0.0f);	// then lod_far (the global LOD rules here)
					next += 2;
				}
				tree.quads = next < tokens.size() ? std::max(1, ParseInt(tokens[next], 2)) : 2;
				tree.layer = next + 1 < tokens.size() ? ParseInt(tokens[next + 1], 0) : 0;
				if (std::abs(tree.width) > 0.001f && tree.height > 0.001f)
				{
					out.maxHeight = std::max(out.maxHeight, tree.maxHeight);
					out.trees.push_back(tree);
				}
			}
		});

		if (out.texture.empty())
			return Fail(error, "The forest has no TEXTURE");
		if (out.trees.empty())
			return Fail(error, "The forest has no TREE rows");
		return true;
	}

	bool LoadForestAsset(const std::filesystem::path &path, ForestAsset &out, std::string *error)
	{
		return LoadAsset(path, out, error, &ParseForestAsset);
	}

	// -------------------------------------------------------

	std::vector<const BeachAsset::Tile *> BeachAsset::GetTiles(const int type) const
	{
		std::vector<const Tile *> found;
		for (const Tile &tile : tiles)
		{
			if (tile.type == type)
				found.push_back(&tile);
		}
		return found;
	}

	bool ParseBeachAsset(const std::string_view text, BeachAsset &out, std::string *error)
	{
		out = BeachAsset();
		AssetHeader header;
		size_t start = 0;
		if (!CheckHeader(text, "BEACH", header, start, error))
			return false;

		ForEachRow(text, start, [&](const std::vector<std::string_view> &tokens, const std::string_view line)
		{
			const std::string_view key = tokens[0];
			if (key == "TEXTURE" || key == "BASE_TEX")
				out.texture = CleanPath(RestOfLine(line, 1));
			else if (key == "TEXTURE_NORMAL" && tokens.size() >= 3)
			{
				out.normalRatio = ParseFloat(tokens[1], 1.0f);
				out.textureNormal = CleanPath(RestOfLine(line, 2));
			}
			else if (key == "DECAL_LIB" && tokens.size() >= 2)
				out.decalLibrary = CleanPath(RestOfLine(line, 1));
			else if (key == "SCALE_X" && tokens.size() >= 2)
				out.scaleX = std::max(1.0f, ParseFloat(tokens[1], 1.0f));
			else if (key == "SCALE_Y" && tokens.size() >= 2)
				out.scaleY = std::max(1.0f, ParseFloat(tokens[1], 1.0f));
			else if (key == "SIZE_X" && tokens.size() >= 2)
				out.sizeX = ParseFloat(tokens[1], 1.0f);
			else if (key == "SIZE_Y" && tokens.size() >= 2)
				out.sizeY = ParseFloat(tokens[1], 1.0f);
			else if (key == "GROUP" && tokens.size() >= 3)
				out.groups.emplace_back(ParseInt(tokens[1]), ParseInt(tokens[2]));
			else if (key == "TILE" && tokens.size() >= 7)
			{
				// TILE type s1 s2 t1 t2 t3
				BeachAsset::Tile tile;
				tile.type = ParseInt(tokens[1]);
				tile.s1 = ParseFloat(tokens[2]);
				tile.s2 = ParseFloat(tokens[3]);
				tile.t1 = ParseFloat(tokens[4]);
				tile.t2 = ParseFloat(tokens[5]);
				tile.t3 = ParseFloat(tokens[6]);
				if (tile.s2 != tile.s1 && tile.t3 > tile.t1)
					out.tiles.push_back(tile);
			}
			else if ((key == "XONL2" || key == "XONR2") && tokens.size() >= 7)
			{
				// XONL2 type s1 s2 t1 t2 t3
				BeachAsset::Transition transition;
				transition.kind = std::string(key);
				transition.from = ParseInt(tokens[1]);
				transition.tile = { transition.from, ParseFloat(tokens[2]), ParseFloat(tokens[3]), ParseFloat(tokens[4]), ParseFloat(tokens[5]), ParseFloat(tokens[6]) };
				out.transitions.push_back(std::move(transition));
			}
			else if ((key == "XONLG" || key == "XONRG") && tokens.size() >= 8)
			{
				// XONLG group group s1 s2 t1 t2 t3
				BeachAsset::Transition transition;
				transition.kind = std::string(key);
				transition.from = ParseInt(tokens[1]);
				transition.to = ParseInt(tokens[2]);
				transition.tile = { 0, ParseFloat(tokens[3]), ParseFloat(tokens[4]), ParseFloat(tokens[5]), ParseFloat(tokens[6]), ParseFloat(tokens[7]) };
				out.transitions.push_back(std::move(transition));
			}
		});

		if (out.texture.empty())
			return Fail(error, "The beach has no TEXTURE");
		if (out.tiles.empty())
			return Fail(error, "The beach has no TILE rows");
		if (out.sizeX <= 0.0f || out.sizeY <= 0.0f)
			return Fail(error, "The beach's SIZE_X / SIZE_Y must be positive");
		return true;
	}

	bool LoadBeachAsset(const std::filesystem::path &path, BeachAsset &out, std::string *error)
	{
		return LoadAsset(path, out, error, &ParseBeachAsset);
	}

	// -------------------------------------------------------

	namespace
	{
		enum class FacadeRowKind : uint8_t
		{
			Unknown,	// Not modeled: kept verbatim inside a shader section
			Shader,		// Modeled shader rows (a shader section stays open)
			Header,		// Facade-wide switches that may appear anywhere (a shader section stays open)
			Structure	// LODs, walls, floors, segments, roofs... (closes a shader section)
		};

		FacadeRowKind GetFacadeRowKind(const std::string_view key)
		{
			static constexpr std::string_view SHADER[] = { "SHADER_WALL", "SHADER_ROOF", "TEXTURE", "TEXTURE_NORMAL", "NO_BLEND", "TWO_SIDED" };
			static constexpr std::string_view HEADER[] = { "RING", "GRADED", "DRAPED", "DOUBLED", "FLOORS_MIN", "FLOORS_MAX", "LAYER_GROUP", "LAYER_GROUP_DRAPED",
				"HARD_WALL", "HARD_ROOF", "NO_ROOF_MESH", "NO_WALL_MESH" };
			static constexpr std::string_view STRUCTURE[] = { "LOD", "TEX_SIZE", "ROOF", "ROOF_SCALE", "WALL", "WALL_RULE", "SCALE", "ROOF_SLOPE", "BASEMENT_DEPTH",
				"LEFT", "CENTER", "RIGHT", "BOTTOM", "MIDDLE", "TOP", "OBJ", "FLOOR", "SEGMENT", "SEGMENT_CURVED", "MESH", "VERTEX", "IDX", "ATTACH_DRAPED",
				"ATTACH_GRADED", "SPELLING", "ROOF_HEIGHT", "TWO_SIDED_ROOF", "ROOF_OBJ", "ROOF_OBJ_HEADING", "FACADE_SCRAPER", "FACADE_SCRAPER_MODEL",
				"FACADE_SCRAPER_MODEL_OFFSET", "FACADE_SCRAPER_PAD" };
			if (std::ranges::find(SHADER, key) != std::end(SHADER))
				return FacadeRowKind::Shader;
			if (std::ranges::find(HEADER, key) != std::end(HEADER))
				return FacadeRowKind::Header;
			if (std::ranges::find(STRUCTURE, key) != std::end(STRUCTURE))
				return FacadeRowKind::Structure;
			return FacadeRowKind::Unknown;
		}
	}

	bool FacadeAsset::WallFilter::Accepts(const double width, const double relativeHeading) const
	{
		if (width < minWidth || width > maxWidth)
			return false;
		if (minHeading == maxHeading)
			return true;	// "0 0": any heading
		const auto wrap = [](double degrees)
		{
			degrees = std::fmod(degrees, 360.0);
			return degrees < 0.0 ? degrees + 360.0 : degrees;
		};
		const double h = wrap(relativeHeading), lo = wrap(minHeading), hi = wrap(maxHeading);
		return lo < hi ? (lo <= h && h < hi) : (lo <= h || h < hi);	// A range may wrap past north (315 .. 45)
	}

	bool ParseFacadeAsset(const std::string_view text, FacadeAsset &out, std::string *error)
	{
		out = FacadeAsset();
		AssetHeader header;
		size_t start = 0;
		if (!CheckHeader(text, "FACADE", header, start, error))
			return false;
		out.version = header.version;

		bool roofSection = false;
		bool farLod = false;
		bool lodSeen = false;
		float scaleS = 1.0f;	// TEX_SIZE normalizes pixel coordinates (type 1)
		float scaleT = 1.0f;
		float texWidth = 0.0f;	// That TEX_SIZE, 0 when coordinates are fractions
		float texHeight = 0.0f;
		bool wallRows = false;	// The current type 1 wall has texture rows (a TEX_SIZE after them no longer applies to it)
		bool roofRows = false;
		// The shader section unmodeled rows are kept for: open from SHADER_WALL / SHADER_ROOF (the walls' from the start)
		// until the first structural row
		std::vector<std::string> *shaderRows = &out.wallShaderRows;
		FacadeAsset::Segment *segment = nullptr;
		FacadeAsset::Mesh *mesh = nullptr;

		const auto currentWall = [&]() -> FacadeAsset::Wall *
		{
			return out.walls.empty() ? nullptr : &out.walls.back();
		};
		const auto currentFloor = [&]() -> FacadeAsset::Floor *
		{
			return out.floors.empty() ? nullptr : &out.floors.back();
		};
		// WALL / WALL_RULE: <min width> <max width> [<min heading> <max heading>]
		const auto parseFilter = [](const std::vector<std::string_view> &tokens)
		{
			FacadeAsset::WallFilter filter;
			filter.minWidth = ParseFloat(tokens[1]);
			filter.maxWidth = ParseFloat(tokens[2], 9999.0f);
			if (tokens.size() >= 5)
			{
				filter.minHeading = ParseFloat(tokens[3]);
				filter.maxHeading = ParseFloat(tokens[4]);
			}
			return filter;
		};
		// The optional [<lo> <hi>] object density range after a row's required values
		const auto showRange = [](const std::vector<std::string_view> &tokens, const size_t first, int &lo, int &hi)
		{
			if (tokens.size() >= first + 2)
			{
				lo = std::clamp(ParseInt(tokens[first], 0), 0, 6);
				hi = std::clamp(ParseInt(tokens[first + 1], 0), 0, 6);
			}
		};

		ForEachRow(text, start, [&](const std::vector<std::string_view> &tokens, const std::string_view line)
		{
			const std::string_view key = tokens[0];

			if (key == "LOD" && tokens.size() >= 2)
			{
				// Only the nearest LOD's walls are kept
				farLod = ParseFloat(tokens[1]) > 0.1f;
				if (!farLod && !lodSeen && tokens.size() >= 3)
					out.lodFar = ParseFloat(tokens[2], out.lodFar);
				lodSeen = true;
				shaderRows = nullptr;
				return;
			}
			if (farLod)
				return;

			// Rows the parser doesn't model are kept verbatim while a shader section is open (EXPORT rows some tools
			// leave in facades are library business and dropped); structural rows close the section
			const FacadeRowKind kind = GetFacadeRowKind(key);
			if (kind == FacadeRowKind::Unknown)
			{
				if (shaderRows && !key.starts_with("EXPORT"))
					shaderRows->push_back(std::string(Trim(line)));
				return;
			}
			if (kind == FacadeRowKind::Structure)
				shaderRows = nullptr;

			if (key == "SHADER_ROOF")
			{
				roofSection = true;
				shaderRows = &out.roofShaderRows;
				return;
			}
			if (key == "SHADER_WALL")
			{
				roofSection = false;
				shaderRows = &out.wallShaderRows;
				return;
			}
			// The roof's blending and sidedness are the roof shader's business: kept with its rows
			if ((key == "NO_BLEND" || key == "TWO_SIDED") && shaderRows == &out.roofShaderRows)
			{
				shaderRows->push_back(std::string(Trim(line)));
				return;
			}
			if (key == "LAYER_GROUP" && tokens.size() >= 2)
			{
				out.layerGroup = std::string(tokens[1]);
				out.layerOffset = tokens.size() >= 3 ? ParseInt(tokens[2], 0) : 0;
				return;
			}
			if (key == "LAYER_GROUP_DRAPED" && tokens.size() >= 2)
			{
				out.drapedLayerGroup = std::string(tokens[1]);
				out.drapedLayerOffset = tokens.size() >= 3 ? ParseInt(tokens[2], 0) : 0;
				return;
			}
			if (key == "FACADE_SCRAPER" && tokens.size() >= 5)
			{
				FacadeAsset::Scraper scraper;
				scraper.minAgl = ParseFloat(tokens[1]);
				scraper.maxAgl = ParseFloat(tokens[2]);
				scraper.step = ParseFloat(tokens[3]);
				scraper.floors = ParseInt(tokens[4], 0);
				out.scrapers.push_back(scraper);
				return;
			}
			if ((key == "FACADE_SCRAPER_MODEL" || key == "FACADE_SCRAPER_MODEL_OFFSET" || key == "FACADE_SCRAPER_PAD") && !out.scrapers.empty())
			{
				FacadeAsset::Scraper &scraper = out.scrapers.back();
				++(key == "FACADE_SCRAPER_PAD" ? scraper.pads : scraper.models);
				scraper.rows.push_back(std::string(Trim(line)));
				return;
			}
			if (key == "TEXTURE")
			{
				(roofSection ? out.roofTexture : out.wallTexture) = CleanPath(RestOfLine(line, 1));
				return;
			}
			if (key == "TEXTURE_NORMAL" && tokens.size() >= 3)
			{
				(roofSection ? out.roofTextureNormal : out.wallTextureNormal) = CleanPath(RestOfLine(line, 2));
				(roofSection ? out.roofTextureNormalScale : out.wallTextureNormalScale) = ParseFloat(tokens[1], 1.0f);
				return;
			}
			if (key == "NO_BLEND")
			{
				out.noBlendCutoff = tokens.size() >= 2 ? ParseFloat(tokens[1], 0.5f) : 0.5f;
				return;
			}
			if (key == "RING" && tokens.size() >= 2)
			{
				out.ring = ParseInt(tokens[1], 1) > 0;
				return;
			}
			if (key == "TWO_SIDED" && tokens.size() >= 2)
			{
				out.twoSided = ParseInt(tokens[1], 0) > 0;
				return;
			}
			if (key == "GRADED")
			{
				out.graded = true;
				return;
			}
			if (key == "DRAPED")
			{
				out.graded = false;
				return;
			}
			if (key == "ROOF_SCALE" && tokens.size() >= 2)
			{
				if (out.IsType2())
				{
					// ROOF_SCALE <meters x> [<meters y>]: y defaults to x
					out.roofScaleS = ParseFloat(tokens[1], 1.0f);
					out.roofScaleT = tokens.size() >= 3 ? ParseFloat(tokens[2], out.roofScaleS) : out.roofScaleS;
					if (out.roofScaleT == 0.0f)
						out.roofScaleT = out.roofScaleS;
				}
				else if (tokens.size() >= 9)
				{
					// ROOF_SCALE s1 t1 s_center t_center s2 t2 size_x size_y: meters per texture repeat follows from the corners
					const float s1 = ParseFloat(tokens[1]) * scaleS, t1 = ParseFloat(tokens[2]) * scaleT;
					const float s2 = ParseFloat(tokens[5]) * scaleS, t2 = ParseFloat(tokens[6]) * scaleT;
					const float sizeX = ParseFloat(tokens[7], 10.0f), sizeY = ParseFloat(tokens[8], 10.0f);
					if (s2 > s1 && t2 > t1)
					{
						out.roofUv = { { s1, t1 }, { s2, t1 }, { s2, t2 }, { s1, t2 } };
						out.roofScaleS = sizeX / (s2 - s1);
						out.roofScaleT = sizeY / (t2 - t1);
					}
					out.roofCenter = { ParseFloat(tokens[3]) * scaleS, ParseFloat(tokens[4]) * scaleT };
					out.roofScaled = true;
					out.roofTextureWidth = texWidth;
					out.roofTextureHeight = texHeight;
					roofRows = true;
					out.hasRoof = true;
				}
				return;
			}

			if (!out.IsType2())
			{
				// Type 1
				if (key == "TEX_SIZE" && tokens.size() >= 3)
				{
					const float w = ParseFloat(tokens[1]), h = ParseFloat(tokens[2]);
					scaleS = w > 0.0f ? 1.0f / w : 1.0f;
					scaleT = h > 0.0f ? 1.0f / h : 1.0f;
					// "TEX_SIZE 1 1" is the same as fractions
					const bool fractions = w == 1.0f && h == 1.0f;
					texWidth = w > 0.0f && !fractions ? w : 0.0f;
					texHeight = h > 0.0f && !fractions ? h : 0.0f;
					// A TEX_SIZE right after WALL still applies to that wall's rows
					if (FacadeAsset::Wall *wall = currentWall(); wall && !wallRows)
					{
						wall->textureWidth = texWidth;
						wall->textureHeight = texHeight;
					}
				}
				else if (key == "WALL" && tokens.size() >= 3)
				{
					FacadeAsset::Wall wall;
					wall.minWidth = ParseFloat(tokens[1]);
					wall.maxWidth = ParseFloat(tokens[2], 9999.0f);
					wall.filters.push_back(parseFilter(tokens));
					wall.name = tokens.size() >= 6 ? std::string(RestOfLine(line, 5)) : std::string();
					wall.textureWidth = texWidth;
					wall.textureHeight = texHeight;
					out.walls.push_back(std::move(wall));
					wallRows = false;
					roofSection = false;
				}
				else if (key == "HARD_WALL" && tokens.size() >= 2)
					out.hardWall = std::string(tokens[1]);
				else if (key == "HARD_ROOF" && tokens.size() >= 2)
					out.hardRoof = std::string(tokens[1]);
				else if (key == "DOUBLED")
					out.doubled = tokens.size() < 2 || ParseInt(tokens[1], 1) > 0;
				else if (key == "ROOF" && tokens.size() >= 3)
				{
					// Roof corners, wherever they are in the LOD
					if (!roofRows)
					{
						out.roofTextureWidth = texWidth;
						out.roofTextureHeight = texHeight;
						roofRows = true;
					}
					out.roofUv.emplace_back(ParseFloat(tokens[1]) * scaleS, ParseFloat(tokens[2]) * scaleT);
					out.hasRoof = true;
				}
				else if (key == "FLOORS_MIN" && tokens.size() >= 2)
					out.floorsMin = ParseInt(tokens[1], 1);
				else if (key == "FLOORS_MAX" && tokens.size() >= 2)
					out.floorsMax = ParseInt(tokens[1], 999);
				else if (FacadeAsset::Wall *wall = currentWall())
				{
					const auto range = [&](std::vector<std::pair<float, float>> &list, const float scale)
					{
						if (tokens.size() >= 3)
							list.emplace_back(ParseFloat(tokens[1]) * scale, ParseFloat(tokens[2]) * scale);
						wallRows = true;
					};
					if (key == "WALL_RULE" && tokens.size() >= 3)
						wall->filters.push_back(parseFilter(tokens));
					else if (key == "SCALE" && tokens.size() >= 3)
					{
						wall->scaleS = ParseFloat(tokens[1], 1.0f);
						wall->scaleT = ParseFloat(tokens[2], 1.0f);
					}
					else if (key == "ROOF_SLOPE" && tokens.size() >= 2)
					{
						wall->roofSlope = ParseFloat(tokens[1]);
						wall->slant = tokens.size() >= 3 && tokens[2] == "SLANT";
					}
					else if (key == "BASEMENT_DEPTH" && tokens.size() >= 2)
					{
						wall->basement = ParseFloat(tokens[1]) * scaleT;
						wallRows = true;
					}
					else if (key == "LEFT") range(wall->leftPanels, scaleS);
					else if (key == "CENTER") range(wall->centerPanels, scaleS);
					else if (key == "RIGHT") range(wall->rightPanels, scaleS);
					else if (key == "BOTTOM") range(wall->bottomFloors, scaleT);
					else if (key == "MIDDLE") range(wall->middleFloors, scaleT);
					else if (key == "TOP") range(wall->topFloors, scaleT);
				}
				return;
			}

			// Type 2
			if (key == "OBJ" && tokens.size() >= 2)
				out.objects.push_back(CleanPath(RestOfLine(line, 1)));
			else if (key == "FLOOR")
			{
				FacadeAsset::Floor floor;
				floor.name = tokens.size() >= 2 ? std::string(RestOfLine(line, 1)) : std::string();
				out.floors.push_back(std::move(floor));
				segment = nullptr;
				mesh = nullptr;
			}
			else if (key == "SEGMENT" || key == "SEGMENT_CURVED")
			{
				FacadeAsset::Floor *floor = currentFloor();
				if (!floor)
					return;
				std::vector<FacadeAsset::Segment> &list = key == "SEGMENT" ? floor->segments : floor->curvedSegments;
				list.emplace_back();
				segment = &list.back();
				mesh = nullptr;
			}
			else if (key == "MESH" && segment && tokens.size() >= 6)
			{
				segment->meshes.emplace_back();
				mesh = &segment->meshes.back();
				mesh->group = ParseInt(tokens[1], 0);
				mesh->lodFar = ParseFloat(tokens[2], 0.0f);
				mesh->cuts = std::max(1, ParseInt(tokens[3], 1));
				const size_t vertices = static_cast<size_t>(std::max(0, ParseInt(tokens[4], 0)));
				const size_t indices = static_cast<size_t>(std::max(0, ParseInt(tokens[5], 0)));
				mesh->positions.reserve(vertices * 3);
				mesh->normals.reserve(vertices * 3);
				mesh->uvs.reserve(vertices * 2);
				mesh->indices.reserve(indices);
			}
			else if (key == "VERTEX" && mesh && tokens.size() >= 9)
			{
				for (int i = 0; i < 3; ++i)
					mesh->positions.push_back(ParseFloat(tokens[1 + static_cast<size_t>(i)]));
				for (int i = 0; i < 3; ++i)
					mesh->normals.push_back(ParseFloat(tokens[4 + static_cast<size_t>(i)]));
				mesh->uvs.push_back(ParseFloat(tokens[7]));
				mesh->uvs.push_back(ParseFloat(tokens[8]));
			}
			else if (key == "IDX" && mesh)
			{
				for (size_t i = 1; i < tokens.size(); ++i)
					mesh->indices.push_back(static_cast<uint32_t>(std::max(0, ParseInt(tokens[i], 0))));
			}
			else if ((key == "ATTACH_DRAPED" || key == "ATTACH_GRADED") && segment && tokens.size() >= 6)
			{
				FacadeAsset::Attachment attachment;
				attachment.object = ParseInt(tokens[1], 0);
				attachment.x = ParseFloat(tokens[2]);
				attachment.y = ParseFloat(tokens[3]);
				attachment.z = ParseFloat(tokens[4]);
				attachment.heading = ParseFloat(tokens[5]);
				attachment.draped = key == "ATTACH_DRAPED";
				showRange(tokens, 6, attachment.showLo, attachment.showHi);
				segment->attachments.push_back(attachment);
			}
			else if (key == "WALL" && tokens.size() >= 5)
			{
				FacadeAsset::Floor *floor = currentFloor();
				if (!floor)
					return;
				FacadeAsset::WallRule rule;
				rule.minWidth = ParseFloat(tokens[1]);
				rule.maxWidth = ParseFloat(tokens[2], 9999.0f);
				rule.minHeading = ParseFloat(tokens[3]);
				rule.maxHeading = ParseFloat(tokens[4], 360.0f);
				rule.filters.push_back(parseFilter(tokens));
				rule.name = tokens.size() >= 6 ? std::string(RestOfLine(line, 5)) : std::string();
				floor->walls.push_back(std::move(rule));
				segment = nullptr;
				mesh = nullptr;
			}
			else if (key == "WALL_RULE" && tokens.size() >= 3)
			{
				FacadeAsset::Floor *floor = currentFloor();
				if (floor && !floor->walls.empty())
					floor->walls.back().filters.push_back(parseFilter(tokens));
			}
			else if (key == "SPELLING")
			{
				FacadeAsset::Floor *floor = currentFloor();
				if (!floor || floor->walls.empty())
					return;
				std::vector<int> spelling;
				for (size_t i = 1; i < tokens.size(); ++i)
					spelling.push_back(ParseInt(tokens[i], 0));
				if (!spelling.empty())
					floor->walls.back().spellings.push_back(std::move(spelling));
			}
			else if (key == "ROOF_HEIGHT")
			{
				FacadeAsset::Floor *floor = currentFloor();
				if (!floor)
					return;
				for (size_t i = 1; i < tokens.size(); ++i)
				{
					const float height = ParseFloat(tokens[i]);
					floor->roofHeights.push_back(height);
					floor->roofs.push_back({ height, false, {} });
				}
				out.hasRoof = true;
			}
			else if (key == "TWO_SIDED_ROOF")
			{
				FacadeAsset::Floor *floor = currentFloor();
				if (floor && !floor->roofs.empty())
					floor->roofs.back().twoSided = true;
			}
			else if (key == "HARD_ROOF" && tokens.size() >= 2)
			{
				if (FacadeAsset::Floor *floor = currentFloor())
					floor->hardRoof = std::string(tokens[1]);
			}
			else if ((key == "ROOF_OBJ" && tokens.size() >= 4) || (key == "ROOF_OBJ_HEADING" && tokens.size() >= 5))
			{
				// ROOF_OBJ <index> <s> <t> [<lo> <hi>], ROOF_OBJ_HEADING <index> <s> <t> <heading> [<lo> <hi>]: on the latest roof
				FacadeAsset::Floor *floor = currentFloor();
				if (!floor || floor->roofs.empty())
					return;
				FacadeAsset::RoofObject object;
				object.object = ParseInt(tokens[1], 0);
				object.s = ParseFloat(tokens[2]);
				object.t = ParseFloat(tokens[3]);
				const bool heading = key == "ROOF_OBJ_HEADING";
				if (heading)
					object.heading = ParseFloat(tokens[4]);
				showRange(tokens, heading ? 5 : 4, object.showLo, object.showHi);
				floor->roofs.back().objects.push_back(object);
			}
			else if (key == "NO_ROOF_MESH")
				out.noRoofMesh = true;
			else if (key == "NO_WALL_MESH")
				out.noWallMesh = true;
		});

		// Segment widths (type 2): segments run along -z; their extent in z is the width spellings add up, and every
		// segment is shifted so it spans [-width, 0] (X-Plane and WorldEditor right-adjust them the same way).
		for (FacadeAsset::Floor &floor : out.floors)
		{
			for (std::vector<FacadeAsset::Segment> *list : { &floor.segments, &floor.curvedSegments })
			for (FacadeAsset::Segment &seg : *list)
			{
				float minZ = 0.0f, maxZ = 0.0f;
				bool any = false;
				const auto include = [&](const float z)
				{
					if (!any)
					{
						minZ = maxZ = z;
						any = true;
					}
					minZ = std::min(minZ, z);
					maxZ = std::max(maxZ, z);
				};
				for (const FacadeAsset::Mesh &m : seg.meshes)
				{
					for (size_t i = 0; i + 2 < m.positions.size(); i += 3)
						include(m.positions[i + 2]);
				}
				for (const FacadeAsset::Attachment &attachment : seg.attachments)
					include(attachment.z);
				seg.width = any ? std::max(0.0f, maxZ - minZ) : 0.0f;
				if (any && maxZ != 0.0f)
				{
					for (FacadeAsset::Mesh &m : seg.meshes)
					{
						for (size_t i = 0; i + 2 < m.positions.size(); i += 3)
							m.positions[i + 2] -= maxZ;
					}
					for (FacadeAsset::Attachment &attachment : seg.attachments)
						attachment.z -= maxZ;
				}
			}
		}

		// A floor's height: its top roof, else its tallest segment (roofless hangars and fences)
		for (FacadeAsset::Floor &floor : out.floors)
		{
			floor.height = 0.0f;
			for (const float h : floor.roofHeights)
				floor.height = std::max(floor.height, h);
			if (floor.roofHeights.empty())
			{
				for (const FacadeAsset::Segment &seg : floor.segments)
					for (const FacadeAsset::Mesh &m : seg.meshes)
						for (size_t i = 1; i < m.positions.size(); i += 3)
							floor.height = std::max(floor.height, m.positions[i]);
			}
		}

		// A type 1 facade without walls (some jetway placeholders) is valid: it just draws nothing.
		return true;
	}

	bool LoadFacadeAsset(const std::filesystem::path &path, FacadeAsset &out, std::string *error)
	{
		return LoadAsset(path, out, error, &ParseFacadeAsset);
	}

	// -------------------------------------------------------

	bool ParseAutogenPointAsset(const std::string_view text, AutogenPointAsset &out, std::string *error)
	{
		out = AutogenPointAsset();
		AssetHeader header;
		size_t start = 0;
		if (!CheckHeader(text, "AG_POINT", header, start, error))
			return false;

		// Coordinates are texture pixels; TEXTURE_SCALE gives pixels per texture, TEXTURE_WIDTH meters per texture
		double texS = 1.0, texT = 1.0;		// pixels -> UV
		double texX = 1.0, texY = 1.0;		// meters the whole texture covers
		bool explicitHeight = false;
		ForEachRow(text, start, [&](const std::vector<std::string_view> &tokens, const std::string_view line)
		{
			const std::string_view key = tokens[0];
			if (key == "TEXTURE")
				out.texture = CleanPath(RestOfLine(line, 1));
			else if (key == "TEXTURE_SCALE" && tokens.size() >= 3)
			{
				const double s = ParseFloat(tokens[1], 1.0f), t = ParseFloat(tokens[2], 1.0f);
				texS = s > 0.0 ? 1.0 / s : 1.0;
				texT = t > 0.0 ? 1.0 / t : 1.0;
				if (!explicitHeight)
					texY = texX * texS / texT;
			}
			else if (key == "TEXTURE_WIDTH" && tokens.size() >= 2)
			{
				texX = ParseFloat(tokens[1], 1.0f);
				if (!explicitHeight)
					texY = texX * texS / texT;
			}
			else if (key == "TEXTURE_HEIGHT" && tokens.size() >= 2)
			{
				texY = ParseFloat(tokens[1], 1.0f);
				explicitHeight = true;
			}
			else if (key == "OBJECT" && tokens.size() >= 2)
				out.objects.push_back(CleanPath(RestOfLine(line, 1)));
			else if (key == "TILE" && tokens.size() >= 5)
			{
				const double x1 = ParseFloat(tokens[1]), y1 = ParseFloat(tokens[2]), x2 = ParseFloat(tokens[3]), y2 = ParseFloat(tokens[4]);
				const double corners[4][2] = { { x1, y1 }, { x2, y1 }, { x2, y2 }, { x1, y2 } };
				out.tile.clear();
				for (const auto &corner : corners)
				{
					out.tile.push_back(static_cast<float>(corner[0] * texS * texX));
					out.tile.push_back(static_cast<float>(corner[1] * texT * texY));
					out.tile.push_back(static_cast<float>(corner[0] * texS));
					out.tile.push_back(static_cast<float>(corner[1] * texT));
				}
				out.anchorX = static_cast<float>(0.5 * (x1 + x2) * texS * texX);
				out.anchorY = static_cast<float>(0.5 * (y1 + y2) * texT * texY);
			}
			else if (key == "CROP_POLY" && tokens.size() >= 7)
			{
				out.tile.clear();
				for (size_t i = 1; i + 1 < tokens.size(); i += 2)
				{
					const double px = ParseFloat(tokens[i]), py = ParseFloat(tokens[i + 1]);
					out.tile.push_back(static_cast<float>(px * texS * texX));
					out.tile.push_back(static_cast<float>(py * texT * texY));
					out.tile.push_back(static_cast<float>(px * texS));
					out.tile.push_back(static_cast<float>(py * texT));
				}
			}
			else if (key == "ANCHOR_PT" && tokens.size() >= 3)
			{
				out.anchorX = static_cast<float>(ParseFloat(tokens[1]) * texS * texX);
				out.anchorY = static_cast<float>(ParseFloat(tokens[2]) * texT * texY);
			}
			else if (key == "ROTATION" && tokens.size() >= 2)
				out.rotation = ParseFloat(tokens[1]);
			else if (key == "HIDE_TILES")
				out.hideTiles = true;
			else if ((key == "OBJ_DRAPED" || key == "OBJ_GRADED" || key == "OBJ_DELTA" || key == "OBJ_SCRAPER") && tokens.size() >= 5)
			{
				AutogenPointAsset::Object object;
				object.x = static_cast<float>(ParseFloat(tokens[1]) * texS * texX);
				object.y = static_cast<float>(ParseFloat(tokens[2]) * texT * texY);
				object.heading = ParseFloat(tokens[3]);
				const size_t indexToken = key == "OBJ_DELTA" ? 5 : 4;
				if (indexToken >= tokens.size())
					return;
				object.object = ParseInt(tokens[indexToken], -1);
				object.draped = key != "OBJ_GRADED";
				if (object.object >= 0 && static_cast<size_t>(object.object) < out.objects.size())
					out.placedObjects.push_back(object);
			}
		});

		if (out.texture.empty() && out.placedObjects.empty())
			return Fail(error, "The autogen point has neither a TEXTURE nor objects");
		return true;
	}

	bool LoadAutogenPointAsset(const std::filesystem::path &path, AutogenPointAsset &out, std::string *error)
	{
		return LoadAsset(path, out, error, &ParseAutogenPointAsset);
	}

	// -------------------------------------------------------
	// .net

	const RoadNetworkAsset::RoadType *RoadNetworkAsset::FindRoadType(const int subtype) const
	{
		const auto byId = [this](const int id) -> const RoadType *
		{
			for (const RoadType &type : roadTypes)
				if (type.id == id)
					return &type;
			return nullptr;
		};
		for (const VirtualType &type : virtualTypes)
		{
			if (type.id == subtype && type.roadType != 0)
				if (const RoadType *road = byId(type.roadType))
					return road;
		}
		return byId(subtype);
	}

	std::string RoadNetworkAsset::GetSubtypeName(const int subtype) const
	{
		for (const VirtualType &type : virtualTypes)
			if (type.id == subtype && !type.description.empty())
				return type.description;
		return "Road type " + std::to_string(subtype);
	}

	bool ParseRoadNetworkAsset(const std::string_view text, RoadNetworkAsset &out, std::string *error)
	{
		out = RoadNetworkAsset();
		AssetHeader header;
		size_t start = 0;
		if (!CheckHeader(text, "ROADS", header, start, error))
			return false;

		// Rows apply to the road type above them; "#VROAD name" comments label the virtual type that follows
		size_t road = std::string::npos;
		size_t virtualType = std::string::npos;
		float center = 0.0f;
		float scale = 1.0f;
		std::string description;

		LineReader reader(text.substr(std::min(start, text.size())));
		std::string_view line;
		std::vector<std::string_view> tokens;
		while (reader.Next(line))
		{
			Tokenize(line, tokens);
			if (tokens.empty())
				continue;
			const std::string_view key = tokens[0];
			if (key == "#VROAD")
			{
				// "#VROAD hwy_6/city/wet (17.250000)": the name, without the width in brackets
				std::string_view name = Trim(RestOfLine(line, 1));
				if (const size_t bracket = name.find(" ("); bracket != std::string_view::npos)
					name = Trim(name.substr(0, bracket));
				description = std::string(name);
				continue;
			}
			if (key.front() == '#')
				continue;

			if (key == "TEXTURE" && tokens.size() >= 3)
				out.textures.push_back(CleanPath(RestOfLine(line, 2)));
			else if ((key == "ROAD_DRAPED" || key == "ROAD_GRADED") && tokens.size() >= 3)
			{
				RoadNetworkAsset::VirtualType type;
				type.id = ParseInt(tokens[2], 0);
				type.description = std::move(description);
				description.clear();
				virtualType = out.virtualTypes.size();
				out.virtualTypes.push_back(std::move(type));
			}
			else if ((key == "ROAD_DRAPE_CHOICE" || key == "ROAD_GRADE_CHOICE") && tokens.size() >= 3 && virtualType != std::string::npos)
			{
				int &choice = out.virtualTypes[virtualType].roadType;
				if (choice == 0)
					choice = ParseInt(tokens[2], 0);
			}
			else if (key == "ROAD_TYPE" && tokens.size() >= 5)
			{
				RoadNetworkAsset::RoadType type;
				type.id = ParseInt(tokens[1], 0);
				type.width = ParseFloat(tokens[2], 0.0f);
				type.length = ParseFloat(tokens[3], 100.0f);
				type.texture = ParseInt(tokens[4], 0);
				if (tokens.size() >= 8)
					for (int c = 0; c < 3; ++c)
						type.color[c] = ParseFloat(tokens[5 + c], 0.6f);
				road = out.roadTypes.size();
				out.roadTypes.push_back(std::move(type));
				center = 0.0f;
				scale = 1.0f;
				virtualType = std::string::npos;
			}
			else if (key == "ROAD_CENTER" && tokens.size() >= 2)
				center = ParseFloat(tokens[1], 0.0f);
			else if (key == "SCALE" && tokens.size() >= 2)
				scale = std::max(1e-6f, ParseFloat(tokens[1], 1.0f));
			else if ((key == "SEGMENT_DRAPED" || key == "SEGMENT_GRADED") && road != std::string::npos)
			{
				// SEGMENT_DRAPED tex lod_lo lod_hi t_scale left s_left right s_right [surface]
				// SEGMENT_GRADED tex lod_lo lod_hi t_scale left h_left s_left right h_right s_right [surface]
				const bool graded = key == "SEGMENT_GRADED";
				if (tokens.size() < (graded ? 11u : 9u))
					continue;
				if (ParseFloat(tokens[2], 0.0f) > 0.0f)
					continue;	// A far level of detail: the near one is drawn
				const size_t l = 5, r = graded ? 8 : 7;
				if (graded && (ParseFloat(tokens[l + 1], 0.0f) != 0.0f || ParseFloat(tokens[r + 1], 0.0f) != 0.0f))
					continue;	// Raised or sloped (curbs, walls): not a flat strip
				RoadNetworkAsset::Strip strip;
				strip.texture = ParseInt(tokens[1], 0);
				strip.left = ParseFloat(tokens[l], 0.0f) - center;
				strip.right = ParseFloat(tokens[r], 0.0f) - center;
				strip.sLeft = ParseFloat(tokens[graded ? l + 2 : l + 1], 0.0f) / scale;
				strip.sRight = ParseFloat(tokens[graded ? r + 2 : r + 1], 0.0f) / scale;
				if (strip.right != strip.left)
					out.roadTypes[road].strips.push_back(strip);
			}
		}

		if (out.roadTypes.empty())
			return Fail(error, "The network has no ROAD_TYPE");
		return true;
	}

	bool LoadRoadNetworkAsset(const std::filesystem::path &path, RoadNetworkAsset &out, std::string *error)
	{
		return LoadAsset(path, out, error, &ParseRoadNetworkAsset);
	}

	// -------------------------------------------------------

	int GetLayerGroupOrder(const std::string_view group)
	{
		static constexpr const char *ORDER[] = { "terrain", "beaches", "shoulders", "taxiways", "runways", "markings", "airports", "roads", "objects", "light_objects", "cars" };
		for (int i = 0; i < static_cast<int>(std::size(ORDER)); ++i)
		{
			if (group == ORDER[i])
				return i;
		}
		return 8;	// objects
	}
}

// -------------------------------------------------------
