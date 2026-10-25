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
 * renderer_quality.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * X-Plane 12's rendering quality levels (Resources/settings.txt, renopt_*), as SEDX presets (plan TASK-022, REQ-010,
	 * REQ-011, REQ-020). Standard library only. Values are X-Plane's, by setting level; the comments name the art
	 * controls each sets.
	 */

	/* @brief renopt_shadow_quality_04: shadow/scenery_shadows, total_fade_ratio, csm_split_*, csm/far_limit_*, fbo/shadow_cam_size. */
	struct ShadowQuality
	{
		bool sceneryShadows = true;
		float fadeRatio = 0.7f;
		int exteriorCascades = 3;
		int interiorCascades = 3;
		float exteriorFarM = 250.0f;
		float interiorFarM = 250.0f;
		uint32_t mapSize = 2048;
	};

	/* @brief renopt_SSAO_04: ssao/enable, quality_level (CACAO 2-4), downsample. */
	struct AoQuality
	{
		bool enabled = true;
		int cacaoQuality = 3;
		bool halfResolution = true;
	};

	/* @brief renopt_clouds_02: cloud/resolution_scale, resolution_max, noise_hi_limit_km, enable_color, shadow_steps. */
	struct CloudQuality
	{
		float resolutionScale = 0.5f;
		uint32_t resolutionMax = 1920;
		float noiseHighLimitKm = 50.0f;
		bool color = true;
		int shadowSteps = 5;
	};

	/* @brief renopt_FSR_04: fsr/enable, quality, lod_bias (the texture LOD bias the upscaler asks for). */
	struct FsrQuality
	{
		bool enabled = false;
		int quality = 3;
		float textureLodBias = 0.0f;
		float renderScale = 1.0f;	// Render resolution per output pixel (FSR 1 modes: 1.3x, 1.5x, 1.7x, 2x per axis)
	};

	/* @brief renopt_draw_distance04: reno/LOD_bias_rat, terrain/fade_start_rat. */
	struct DrawDistanceQuality
	{
		float lodBiasRatio = 1.5f;
		float terrainFadeStartRatio = 0.6f;
	};

	inline constexpr int SHADOW_LEVELS = 5;
	inline constexpr int AO_LEVELS = 5;
	inline constexpr int CLOUD_LEVELS = 4;
	inline constexpr int FSR_LEVELS = 5;
	inline constexpr int DRAW_DISTANCE_LEVELS = 5;
	inline constexpr int TEXTURE_RESOLUTION_LEVELS = 5;

	ShadowQuality GetShadowQuality(int level);
	AoQuality GetAoQuality(int level);
	CloudQuality GetCloudQuality(int level);
	FsrQuality GetFsrQuality(int level);
	DrawDistanceQuality GetDrawDistanceQuality(int level);

	/* @brief X-Plane's menu names for a level ("off", "low", ...). */
	const char *GetShadowLevelName(int level);
	const char *GetAoLevelName(int level);
	const char *GetCloudLevelName(int level);
	const char *GetFsrLevelName(int level);
	const char *GetDrawDistanceLevelName(int level);

	/* @brief reno/tex_res for a renopt_tex_res_04 level (0-4 -> 1-5). */
	int GetTextureResolution(int level);

	/* @brief The whole set of levels, X-Plane's settings screen in one struct. */
	struct RendererQualityPreset
	{
		int shadows = 2;
		int ao = 2;
		int clouds = 2;
		int fsr = 0;
		bool fxaa = true;
		int anisotropy = 16;		// 1, 2, 4, 8, 16, 32
		int textureResolution = 4;
		int drawDistance = 3;
		int msaa = 0;				// X-Plane's hdr/msaa_hw (0 none .. 3 8x); SEDX's deferred path has none

		bool operator==(const RendererQualityPreset &) const = default;
	};

	/* @brief X-Plane's defaults for the vkmtl_async configuration (desktop Vulkan): what parity scenarios compare against. */
	RendererQualityPreset GetDefaultQualityPreset();
}

// -------------------------------------------------------
