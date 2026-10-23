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
 * ssr.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 *
 * The hierarchical-Z march is adapted from AMD FidelityFX SSSR 1.5 (FidelityFX SDK 1.1.4, ffx_sssr_intersect.h):
 * Copyright (C) 2024 Advanced Micro Devices, Inc. Released under the MIT license
 * (docs/third_party/LICENSE-FidelityFX-MIT.txt).
 */
#include "ssr.h"
#include <algorithm>
#include <cfloat>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX
{
	float GlossFromRoughness(const float roughness)
	{
		return std::clamp((1.0f - roughness) / XP_GLOSS_SCALE, 0.0f, 1.0f);
	}

	float RoughnessFromGloss(const float gloss)
	{
		return 1.0f - XP_GLOSS_SCALE * std::clamp(gloss, 0.0f, 1.0f);
	}

	float ReflectionGlossWeight(const float roughness, const SsrSettings &settings)
	{
		const float gloss = GlossFromRoughness(roughness);
		if (gloss < settings.minGloss)
			return 0.0f;
		if (settings.glossFade <= 0.0f)
			return 1.0f;
		const float t = std::clamp((gloss - settings.minGloss) / settings.glossFade, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	float ReflectionConeMip(const float roughness, const float hitDistanceM, const float hitPixelSizeM, const SsrSettings &settings)
	{
		if (!settings.usePrefiltered)
			return std::max(settings.lodBias, 0.0f);
		const float alpha = roughness * roughness;
		const float footprintM = 2.0f * settings.lobeScale * alpha * std::max(hitDistanceM, 0.0f);
		const float pixels = footprintM / std::max(hitPixelSizeM, 1e-6f);
		return std::max(std::log2(std::max(pixels, 1.0f)) + settings.lodBias, 0.0f);
	}

	std::vector<PyramidLevel> ComputeDepthPyramidLevels(const uint32_t width, const uint32_t height)
	{
		std::vector<PyramidLevel> levels;
		PyramidLevel level{ std::max(width, 1u), std::max(height, 1u) };
		levels.push_back(level);
		while (level.width > 1 || level.height > 1)
		{
			level = { std::max(level.width / 2, 1u), std::max(level.height / 2, 1u) };
			levels.push_back(level);
		}
		return levels;
	}

	float DepthPyramid::Farthest(const uint32_t level, const int32_t x, const int32_t y) const
	{
		const PyramidLevel &size = levels[level];
		const int32_t cx = std::clamp(x, 0, static_cast<int32_t>(size.width) - 1);
		const int32_t cy = std::clamp(y, 0, static_cast<int32_t>(size.height) - 1);
		return farthest[level][static_cast<size_t>(cy) * size.width + static_cast<size_t>(cx)];
	}

	float DepthPyramid::Nearest(const uint32_t level, const int32_t x, const int32_t y) const
	{
		const PyramidLevel &size = levels[level];
		const int32_t cx = std::clamp(x, 0, static_cast<int32_t>(size.width) - 1);
		const int32_t cy = std::clamp(y, 0, static_cast<int32_t>(size.height) - 1);
		return nearest[level][static_cast<size_t>(cy) * size.width + static_cast<size_t>(cx)];
	}

	DepthPyramid BuildDepthPyramid(const std::vector<float> &depth, const uint32_t width, const uint32_t height)
	{
		DepthPyramid pyramid;
		pyramid.levels = ComputeDepthPyramidLevels(width, height);
		pyramid.farthest.resize(pyramid.levels.size());
		pyramid.nearest.resize(pyramid.levels.size());
		pyramid.farthest[0] = depth;
		pyramid.nearest[0] = depth;

		for (size_t level = 1; level < pyramid.levels.size(); ++level)
		{
			const PyramidLevel in = pyramid.levels[level - 1];
			const PyramidLevel out = pyramid.levels[level];
			std::vector<float> &farthest = pyramid.farthest[level];
			std::vector<float> &nearest = pyramid.nearest[level];
			farthest.resize(static_cast<size_t>(out.width) * out.height);
			nearest.resize(farthest.size());
			for (uint32_t y = 0; y < out.height; ++y)
			{
				for (uint32_t x = 0; x < out.width; ++x)
				{
					// The 2 x 2 texels below, plus the odd one out on the last row / column (z_minmax.slang)
					uint32_t lastX = x * 2 + 1;
					uint32_t lastY = y * 2 + 1;
					if (x == out.width - 1 && (in.width & 1u) != 0)
						++lastX;
					if (y == out.height - 1 && (in.height & 1u) != 0)
						++lastY;
					lastX = std::min(lastX, in.width - 1);
					lastY = std::min(lastY, in.height - 1);

					// (not near / far: Windows headers define those as macros)
					float farthestDepth = FLT_MAX;
					float nearestDepth = -FLT_MAX;
					for (uint32_t sy = y * 2; sy <= lastY; ++sy)
					{
						for (uint32_t sx = x * 2; sx <= lastX; ++sx)
						{
							farthestDepth = std::min(farthestDepth, pyramid.farthest[level - 1][static_cast<size_t>(sy) * in.width + sx]);
							nearestDepth = std::max(nearestDepth, pyramid.nearest[level - 1][static_cast<size_t>(sy) * in.width + sx]);
						}
					}
					farthest[static_cast<size_t>(y) * out.width + x] = farthestDepth;
					nearest[static_cast<size_t>(y) * out.width + x] = nearestDepth;
				}
			}
		}
		return pyramid;
	}

	SsrMarchResult MarchHiZ(const DepthPyramid &pyramid, const SsrScreenVector origin, const SsrScreenVector direction, const uint32_t maxSteps)
	{
		SsrMarchResult result;
		result.position = origin;
		if (pyramid.levels.empty())
			return result;

		// Each level's cells are the screen's size halved per level (the last cell of an odd level reaches a little
		// further; its depths cover that)
		const float width = static_cast<float>(pyramid.levels[0].width);
		const float height = static_cast<float>(pyramid.levels[0].height);
		const int32_t topLevel = static_cast<int32_t>(pyramid.levels.size()) - 1;
		const auto inverse = [](const float d) { return d != 0.0f ? 1.0f / d : FLT_MAX; };
		const float invU = inverse(direction.u);
		const float invV = inverse(direction.v);
		const float invZ = inverse(direction.z);

		// Cell boundaries ahead of the ray, nudged past by 0.005 texel so the next lookup lands in the next cell
		const float floorU = direction.u < 0.0f ? 0.0f : 1.0f;
		const float floorV = direction.v < 0.0f ? 0.0f : 1.0f;
		const float nudgeU = (direction.u < 0.0f ? -0.005f : 0.005f) / width;
		const float nudgeV = (direction.v < 0.0f ? -0.005f : 0.005f) / height;

		int32_t level = 0;
		float resU = width;
		float resV = height;

		// Out of the origin's own texel first, so the ray doesn't find the surface it leaves
		float t = 0.0f;
		{
			const float planeU = (std::floor(origin.u * resU) + floorU) / resU + nudgeU;
			const float planeV = (std::floor(origin.v * resV) + floorV) / resV + nudgeV;
			t = std::min((planeU - origin.u) * invU, (planeV - origin.v) * invV);
		}
		SsrScreenVector p{ origin.u + t * direction.u, origin.v + t * direction.v, origin.z + t * direction.z };

		uint32_t i = 0;
		for (; i < maxSteps && level >= 0; ++i)
		{
			// Off the screen, in front of the near plane or past the far one: nothing to find
			if (p.u < 0.0f || p.u > 1.0f || p.v < 0.0f || p.v > 1.0f || p.z <= 0.0f || p.z >= 1.0f)
			{
				result.position = p;
				result.iterations = i;
				return result;
			}

			const float cellU = std::floor(p.u * resU);
			const float cellV = std::floor(p.v * resV);
			const float surface = pyramid.Nearest(static_cast<uint32_t>(level), static_cast<int32_t>(cellU), static_cast<int32_t>(cellV));

			// The nearest of the cell's far edges and, for a ray going away from the camera, the cell's nearest surface
			const float tU = ((cellU + floorU) / resU + nudgeU - origin.u) * invU;
			const float tV = ((cellV + floorV) / resV + nudgeV - origin.v) * invV;
			const float tZ = direction.z < 0.0f ? (surface - origin.z) * invZ : FLT_MAX;
			const float tMin = std::min(std::min(tU, tV), tZ);

			// Reverse-Z: the ray is in front of the surface while its depth is the larger. In front, it moves on to the cell's
			// edge (a free step: up a level) or to the surface (down a level); behind it, it stays and looks closer.
			const bool inFront = surface < p.z;
			const bool freeStep = tMin != tZ && inFront;
			if (inFront)
				t = tMin;
			p = { origin.u + t * direction.u, origin.v + t * direction.v, origin.z + t * direction.z };

			if (freeStep)
			{
				if (level < topLevel)
				{
					++level;
					resU *= 0.5f;
					resV *= 0.5f;
				}
			}
			else
			{
				--level;
				resU *= 2.0f;
				resV *= 2.0f;
			}
		}

		result.hit = level < 0;
		result.position = p;
		result.iterations = i;
		return result;
	}
}

// -------------------------------------------------------
