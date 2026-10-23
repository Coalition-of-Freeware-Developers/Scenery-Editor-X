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
 * renderer_quality.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "renderer_quality.h"
#include <algorithm>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		template <typename T, int N>
		const T &At(const T (&table)[N], const int level)
		{
			return table[std::clamp(level, 0, N - 1)];
		}
	}

	ShadowQuality GetShadowQuality(const int level)
	{
		// Level 0's interior far limit isn't set in settings.txt (its line repeats the exterior key): SEDX uses 500
		static constexpr ShadowQuality table[SHADOW_LEVELS] = {
			{ false, 0.999f, 1, 3, 500.0f, 500.0f, 2048 },
			{ true, 0.7f, 2, 3, 120.0f, 120.0f, 1024 },
			{ true, 0.7f, 3, 3, 250.0f, 250.0f, 2048 },
			{ true, 0.7f, 3, 3, 500.0f, 500.0f, 4096 },
			{ true, 0.5f, 4, 3, 1000.0f, 500.0f, 4096 },
		};
		return At(table, level);
	}

	AoQuality GetAoQuality(const int level)
	{
		static constexpr AoQuality table[AO_LEVELS] = {
			{ false, 2, true },
			{ true, 2, true },
			{ true, 3, true },
			{ true, 4, true },
			{ true, 4, false },
		};
		return At(table, level);
	}

	CloudQuality GetCloudQuality(const int level)
	{
		static constexpr CloudQuality table[CLOUD_LEVELS] = {
			{ 0.20f, 980, 20.0f, false, 3 },
			{ 0.25f, 980, 25.0f, true, 5 },
			{ 0.50f, 1920, 50.0f, true, 5 },
			{ 0.50f, 1920, 50.0f, true, 8 },
		};
		return At(table, level);
	}

	FsrQuality GetFsrQuality(const int level)
	{
		// FSR 1 modes: ultra quality 1.3x, quality 1.5x, balanced 1.7x, performance 2x (per axis)
		static constexpr FsrQuality table[FSR_LEVELS] = {
			{ false, 3, 0.0f, 1.0f },
			{ true, 3, -0.38f, 1.0f / 1.3f },
			{ true, 2, -0.58f, 1.0f / 1.5f },
			{ true, 1, -0.79f, 1.0f / 1.7f },
			{ true, 0, -1.0f, 1.0f / 2.0f },
		};
		return At(table, level);
	}

	DrawDistanceQuality GetDrawDistanceQuality(const int level)
	{
		static constexpr DrawDistanceQuality table[DRAW_DISTANCE_LEVELS] = {
			{ 5.0f, 0.75f },
			{ 3.375f, 0.65f },
			{ 2.25f, 0.65f },
			{ 1.5f, 0.6f },
			{ 1.0f, 0.6f },
		};
		return At(table, level);
	}

	const char *GetShadowLevelName(const int level)
	{
		static constexpr const char *names[SHADOW_LEVELS] = { "Aircraft only", "Low", "Medium", "High", "Maximum" };
		return At(names, level);
	}

	const char *GetAoLevelName(const int level)
	{
		static constexpr const char *names[AO_LEVELS] = { "Off", "Low", "Medium", "High", "Highest" };
		return At(names, level);
	}

	const char *GetCloudLevelName(const int level)
	{
		static constexpr const char *names[CLOUD_LEVELS] = { "Low", "Medium", "High", "Maximum" };
		return At(names, level);
	}

	const char *GetFsrLevelName(const int level)
	{
		static constexpr const char *names[FSR_LEVELS] = { "Off", "Ultra quality", "Quality", "Balanced", "Performance" };
		return At(names, level);
	}

	const char *GetDrawDistanceLevelName(const int level)
	{
		static constexpr const char *names[DRAW_DISTANCE_LEVELS] = { "Minimal", "Low", "Medium", "High", "Very high" };
		return At(names, level);
	}

	int GetTextureResolution(const int level)
	{
		return std::clamp(level, 0, TEXTURE_RESOLUTION_LEVELS - 1) + 1;
	}

	RendererQualityPreset GetDefaultQualityPreset()
	{
		// vkmtl_async defaults: shadows 2, SSAO 2, clouds 2, FXAA on, FSR off, anisotropy 16, texture resolution 4,
		// draw distance 3 (all), MSAA off
		return RendererQualityPreset{};
	}
}

// -------------------------------------------------------
