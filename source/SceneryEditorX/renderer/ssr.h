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
 * ssr.h
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/*
	 * Screen-space reflections (plan Phase 9, TASK-060 / 061; XP12: z_minmax, ssr_deferred): the CPU half, standard library
	 * only and unit tested. X-Plane's ssr/* art controls and SEDX's own, the reflection weight by gloss, the sizes of the
	 * min / max depth pyramid, and CPU twins of the pyramid (z_minmax.slang) and of the hierarchical-Z march
	 * (ssr_common.slang), so the traversal is checked against an analytic scene without a GPU.
	 *
	 * The march follows AMD FidelityFX SSSR 1.5 (MIT, FidelityFX SDK 1.1.4, ffx_sssr_intersect.h): a ray in screen space
	 * (u, v, reverse-Z depth) steps to the edge of the pyramid cell it is in, or to the cell's nearest surface, going up a
	 * level after each free step and down after each blocked one; it ends on a surface when it is blocked at level 0.
	 * SEDX differences: the pyramid keeps the farthest and the nearest depth (X-Plane's rg32f z_minmax), the march reads the
	 * nearest; it stops when the ray leaves the screen or the depth range; one ray along the mirror direction per pixel
	 * (X-Plane's ssr/sample_count 1), its gloss lobe taken from blurred mips of the previous frame (ssr/use_prefiltered)
	 * instead of SSSR's stochastic rays and denoiser.
	 */

	inline constexpr float XP_GLOSS_SCALE = 0.96875f;	// X-Plane 12: perceptual roughness = 1 - 0.96875 x gloss

	/*
	 * A ray starts this many pixel widths (at its depth) off its surface, along the normal: the depth buffer holds one depth
	 * per pixel, a staircase that a ray leaving a surface at a grazing angle would otherwise hit within a pixel or two.
	 */
	inline constexpr float SSR_ORIGIN_OFFSET_PIXELS = 1.0f;

	/* @brief X-Plane's reflection controls (sim/private/controls/ssr/*, X-Plane 12.4.4's values) and SEDX's (sedx/ssr_*). */
	struct SsrSettings
	{
		float minGloss = 0.5f;			// ssr/min_gloss: surfaces less glossy keep the probes' reflections alone
		float lodBias = 0.0f;			// ssr/lod_bias: added to the history mip a reflection's lobe picks
		bool usePrefiltered = true;		// ssr/use_prefiltered: glossy lobes read blurred mips of the previous frame
		uint32_t sampleCount = 1;		// ssr/sample_count: rays per pixel (SEDX traces the mirror direction only)
		float glossFade = 0.1f;			// sedx/ssr_gloss_fade: gloss over which reflections fade in above min_gloss (ASSUMPTION)
		uint32_t maxSteps = 96;			// sedx/ssr_max_steps: Hi-Z iterations before a ray gives up
		float thicknessM = 0.25f;		// sedx/ssr_thickness_m: how far behind a surface a hit still counts (ASSUMPTION)
		float thicknessRatio = 0.015f;	// sedx/ssr_thickness_ratio: plus this share of the hit's view depth (ASSUMPTION)
		float edgeFade = 0.05f;			// sedx/ssr_edge_fade: reflections fade out over this share of the screen at its edges
		float historyClamp = 256.0f;	// sedx/ssr_history_clamp: brightest pre-exposed texel the history keeps (no sun glare)
		float lobeScale = 1.3f;			// sedx/ssr_lobe_scale: lobe half width (tangent) per unit GGX alpha (ASSUMPTION)
	};

	/* @brief X-Plane's gloss (normal-map alpha) for a perceptual roughness, and back. */
	float GlossFromRoughness(float roughness);
	float RoughnessFromGloss(float gloss);

	/* @brief The share of a surface's specular reflection SSR may replace: 0 below ssr/min_gloss, 1 from glossFade above. */
	float ReflectionGlossWeight(float roughness, const SsrSettings &settings);

	/**
	 * @brief The history mip a reflection reads: the size, in pixels at the hit, of the lobe's footprint (the GGX lobe's
	 * half width lobeScale x alpha, alpha = roughness^2, at hitDistanceM from the reflecting point), as a mip level plus
	 * ssr/lod_bias. Level 0 for mirrors, or for all with ssr/use_prefiltered off.
	 * @param hitPixelSizeM Width of one pixel at the hit's depth (m).
	 */
	float ReflectionConeMip(float roughness, float hitDistanceM, float hitPixelSizeM, const SsrSettings &settings);

	struct PyramidLevel
	{
		uint32_t width = 0;
		uint32_t height = 0;
	};

	/* @brief Levels of the min / max depth pyramid: level 0 the render size, each next half of it rounded down, to 1 x 1. */
	std::vector<PyramidLevel> ComputeDepthPyramidLevels(uint32_t width, uint32_t height);

	/**
	 * @brief CPU twin of z_minmax.slang: per level, each texel's farthest and nearest reverse-Z depth (the smallest and the
	 * largest value) over the level-0 texels it covers. The last texel of a row or column whose level above had an odd size
	 * also covers the odd one out, so level-0 texel p always falls in texel min(p >> level, size - 1).
	 */
	struct DepthPyramid
	{
		std::vector<PyramidLevel> levels;
		std::vector<std::vector<float>> farthest;	// Per level, row major
		std::vector<std::vector<float>> nearest;

		/* @brief The texel's value, its coordinates clamped to the level. */
		float Farthest(uint32_t level, int32_t x, int32_t y) const;
		float Nearest(uint32_t level, int32_t x, int32_t y) const;
	};
	DepthPyramid BuildDepthPyramid(const std::vector<float> &depth, uint32_t width, uint32_t height);

	/* @brief A point or direction in screen space: u, v (0-1, v down) and reverse-Z depth (1 near plane, 0 far). */
	struct SsrScreenVector
	{
		float u = 0.0f;
		float v = 0.0f;
		float z = 0.0f;
	};

	struct SsrMarchResult
	{
		bool hit = false;				// Ended on a surface (still to be checked for thickness and facing)
		SsrScreenVector position;		// Where the march ended
		uint32_t iterations = 0;
	};

	/**
	 * @brief CPU twin of ssr_common.slang's xp_ssr_march: the hierarchical-Z march of a screen-space ray (origin and a
	 * direction of any length) through the pyramid's nearest depths.
	 */
	SsrMarchResult MarchHiZ(const DepthPyramid &pyramid, SsrScreenVector origin, SsrScreenVector direction, uint32_t maxSteps);
}

// -------------------------------------------------------
