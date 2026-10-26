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
 * xp_overlay_shading.h
 * -------------------------------------------------------
 * Created: 25/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_overlay.h"
#include "xp_texture_loader.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#include <SceneryEditorX/scene/material.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * @brief The textures an overlay's shading samples besides its albedo (tile map, decals, normal map, WEATHER mask),
	 * with the usage each loads as: for loading them ahead of ApplyDrapeShading().
	 */
	template <typename Shading>
	void GetDrapeShadingTextures(const Shading &shading, std::vector<std::pair<std::filesystem::path, TextureUsage>> &out)
	{
		if (shading.shading == OverlayShading::Tiled && !shading.tileMap.empty())
			out.emplace_back(shading.tileMap, TextureUsage::Data);
		int details = 0, normals = 0;
		for (const OverlayDecal &decal : shading.decals)
		{
			int &count = decal.normal ? normals : details;
			if (count++ < 2)
				out.emplace_back(decal.texture, decal.normal ? TextureUsage::Normal : TextureUsage::Data);
		}
		if (!shading.textureNormal.empty())
			out.emplace_back(shading.textureNormal, TextureUsage::Normal);
		if (!shading.weatherMask.empty())
			out.emplace_back(shading.weatherMask, TextureUsage::Data);
	}

	/**
	 * @brief Draped-polygon shading on an overlay material (xp_material.slang xp_drape_*): the tile atlas's mapping
	 * texture in color slot 1, detail decals in color slots 2-3, normal decals in normal slots 1-2, X-Plane's material
	 * from the normal map in normal slot 0, the WEATHER mask in emission slot 3.
	 * @param shading An OverlayMesh, or a part that copied its shading fields (same names).
	 * @param load (path, usage) -> ImageResource*: the scene's texture cache (null when missing).
	 * @return What it adds to the material's cache key.
	 */
	template <typename Shading, typename Load>
	std::string ApplyDrapeShading(const Shading &shading, Load &&load, MaterialAsset &material)
	{
		std::string key;
		float mode = 0.0f;
		if (shading.shading == OverlayShading::Markings)
		{
			// The atlas's channels are masks: R paint, G border (loaded linear, decoded in the shader)
			mode = 2.0f;
			material.SetProperty(MaterialProperty::XpDrapeMiscR, shading.color[0]);
			material.SetProperty(MaterialProperty::XpDrapeMiscG, shading.color[1]);
			material.SetProperty(MaterialProperty::XpDrapeMiscB, shading.color[2]);
			material.SetProperty(MaterialProperty::XpDrapeMiscA, shading.color[3]);
			char text[96];
			std::snprintf(text, sizeof(text), "|markings%.3f,%.3f,%.3f,%.3f", shading.color[0], shading.color[1], shading.color[2], shading.color[3]);
			key += text;
		}
		else if (shading.shading == OverlayShading::Tiled)
		{
			mode = 1.0f;
			ImageResource *map = load(shading.tileMap, TextureUsage::Data);
			material.SetProperty(MaterialProperty::XpTilesX, static_cast<float>(shading.tilesX));
			material.SetProperty(MaterialProperty::XpTilesY, static_cast<float>(shading.tilesY));
			material.SetProperty(MaterialProperty::XpTileMapWidth, map ? static_cast<float>(shading.tileMapWidth) : 0.0f);
			material.SetProperty(MaterialProperty::XpTileMapHeight, map ? static_cast<float>(shading.tileMapHeight) : 0.0f);
			if (map)
				material.SetTexture(MaterialTextureType::Color, map, 1);
			key += "|tiles" + std::to_string(shading.tilesX) + "x" + std::to_string(shading.tilesY) + (map ? "|" + shading.tileMap.generic_string() : std::string());
		}

		// Up to two detail and two normal decals, in file order
		int details = 0, normals = 0;
		for (const OverlayDecal &decal : shading.decals)
		{
			const bool normal = decal.normal;
			int &count = normal ? normals : details;
			if (count >= 2)
				continue;
			ImageResource *image = load(decal.texture, normal ? TextureUsage::Normal : TextureUsage::Data);
			if (!image)
				continue;
			// Repeats per meter, so decals keep their own scale whatever the atlas's
			const float perMeterS = decal.scaleS / std::max(0.01f, shading.metersPerU);
			const float perMeterT = decal.scaleT / std::max(0.01f, shading.metersPerV);
			const int slot = count;
			if (normal)
			{
				material.SetTexture(MaterialTextureType::Normal, image, 1 + slot);
				material.SetProperty(slot == 0 ? MaterialProperty::XpNormalDecal0Scale : MaterialProperty::XpNormalDecal1Scale, perMeterS);
				material.SetProperty(slot == 0 ? MaterialProperty::XpNormalDecal0Gain : MaterialProperty::XpNormalDecal1Gain, decal.strength * decal.gain);
			}
			else
			{
				material.SetTexture(MaterialTextureType::Color, image, 2 + slot);
				material.SetProperty(slot == 0 ? MaterialProperty::XpDecal0Scale : MaterialProperty::XpDecal1Scale, perMeterS);
				material.SetProperty(slot == 0 ? MaterialProperty::XpDecal0Strength : MaterialProperty::XpDecal1Strength, decal.strength);
			}
			// t repeats per meter relative to s (1 for square decal texels; a _PROJ row may set its own)
			if (slot == 0)
				material.SetProperty(normal ? MaterialProperty::XpNormalDecalAspect : MaterialProperty::XpDecalAspect, perMeterT / std::max(1e-6f, perMeterS));
			++count;
			char text[160];
			std::snprintf(text, sizeof(text), "|%s%.4f,%.4f,%.3f,%.3f:", normal ? "nd" : "d", decal.scaleS, decal.scaleT, decal.strength, decal.gain);
			key += text + decal.texture.generic_string();
		}
		if (details + normals > 0)
		{
			// u / v units in meters: the shader turns UVs into meters for the decals' per-meter scales
			material.SetProperty(MaterialProperty::XpDrapeMetersU, shading.metersPerU);
			material.SetProperty(MaterialProperty::XpDrapeMetersV, shading.metersPerV);
			char text[64];
			std::snprintf(text, sizeof(text), "|m%.3f,%.3f", shading.metersPerU, shading.metersPerV);
			key += text;
		}

		// X-Plane's material (xp_material.slang): with a normal map the surface takes its gloss (A, x SPECULAR) and, with
		// NORMAL_METALNESS, its F0 / metalness (B), as OBJ8s and terrain do; normal slot 0 (decals use 1-2), laid out
		// like the albedo (tile atlases pick the same cell) at TEXTURE_NORMAL's ratio. Without one it stays plain and matte.
		if (ImageResource *normalMap = load(shading.textureNormal, TextureUsage::Normal))
		{
			material.SetTexture(MaterialTextureType::Normal, normalMap, 0);
			material.SetProperty(MaterialProperty::XpMaterial, 1.0f);
			material.SetProperty(MaterialProperty::XpNormalMode, shading.normalMetalness ? 2.0f : 1.0f);
			material.SetProperty(MaterialProperty::XpSpecularGain, shading.specular);
			material.SetProperty(MaterialProperty::XpSuperRoughness, shading.superRoughness);
			material.SetProperty(MaterialProperty::XpDefaultF0, 0.04f);
			material.SetProperty(MaterialProperty::XpTerrainNormalRatio, shading.normalRatio);
			char text[96];
			std::snprintf(text, sizeof(text), "|n%.3f,%d,%.3f,%.3f:", shading.normalRatio, shading.normalMetalness ? 1 : 0, shading.specular, shading.superRoughness);
			key += text + shading.textureNormal.generic_string();
		}

		// Weather receptivity (weather.slang): the WEATHER mask in emission slot 3, laid out like the albedo
		if (ImageResource *mask = load(shading.weatherMask, TextureUsage::Data))
		{
			material.SetTexture(MaterialTextureType::Emission, mask, 3);
			key += "|wm" + shading.weatherMask.generic_string();
		}
		if (shading.weatherTransparent)
		{
			material.SetProperty(MaterialProperty::XpWeatherTransparent, 1.0f);
			key += "|wt";
		}
		if (shading.hasSnowAlbedoLuma)
		{
			material.SetProperty(MaterialProperty::XpSnowAlbedoLuma, 1.0f);
			material.SetProperty(MaterialProperty::XpSnowAlbedoLumaR, shading.snowAlbedoLuma[0]);
			material.SetProperty(MaterialProperty::XpSnowAlbedoLumaG, shading.snowAlbedoLuma[1]);
			char text[64];
			std::snprintf(text, sizeof(text), "|sl%.3f,%.3f", shading.snowAlbedoLuma[0], shading.snowAlbedoLuma[1]);
			key += text;
		}
		material.SetProperty(MaterialProperty::XpDrapeMode, mode);
		return key;
	}
}

// -------------------------------------------------------
