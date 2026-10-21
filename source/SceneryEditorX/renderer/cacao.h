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
 * cacao.h
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <cstdint>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/*
	 * Ambient occlusion (plan Phase 8, TASK-057 / 058; XP12: cacao): the CPU half of AMD FidelityFX CACAO 1.4 (MIT,
	 * FidelityFX SDK 1.1.4, src/components/cacao/ffx_cacao.cpp), whose shaders resources/shaders/cacao.slang ports.
	 * Standard library only, unit tested: X-Plane's ssao/* art controls, the sizes of CACAO's buffers, the constant buffer
	 * the shaders read, and the dispatches each quality runs. Renderer::Pass_AmbientOcclusion records them.
	 *
	 * Differences from AMD's host code, all for SEDX's conventions or X-Plane's behaviour:
	 *   - depth is reverse-Z (1 at the near plane, 0 at the far one): the view-depth constants are derived for it, where
	 *     AMD's read a forward-Z projection;
	 *   - normals come from the g-buffer in world space and are turned into CACAO's view space (x right, y up, z forward)
	 *     with the camera's basis;
	 *   - X-Plane 12 adds a fade-in near the camera (ssao/fade_in_from, fade_in_to) to CACAO's fade-out;
	 *   - only the qualities X-Plane's presets use are ported (CACAO medium, high, highest: renopt_SSAO_04 levels 1-4),
	 *     so lower ones run as medium;
	 *   - the output takes ssao/exterior_strength and exterior_curve (identity at X-Plane 12.4.4's values).
	 */
	inline constexpr uint32_t CACAO_LAYERS = 4;			// The 2 x 2 deinterleaved quarters of the SSAO buffer
	inline constexpr uint32_t CACAO_DEPTH_MIPS = 4;		// Deinterleaved view depth and its smart-averaged mips
	inline constexpr uint32_t CACAO_PATTERN_SUBPASSES = 5;
	inline constexpr uint32_t CACAO_MAX_BLUR_PASSES = 8;

	/* @brief FfxCacaoQuality; X-Plane's ssao/quality_level uses the same numbers. */
	enum class CacaoQuality : uint8_t
	{
		Lowest,
		Low,
		Medium,		// 5 taps, sparse (every fifth pixel per dispatch slice)
		High,		// 12 taps
		Highest,	// Adaptive: a 5-tap base pass, an importance map, then up to 31 taps where it matters
	};

	/* @brief X-Plane's CACAO art controls (sim/private/controls/ssao/*); defaults are X-Plane 12.4.4's near the ground. */
	struct CacaoSettings
	{
		float radius = 2.1f;					// ssao/radius: view-space radius of the occlusion sphere (m)
		float shadowMultiplier = 1.4f;			// ssao/shadow_multiplier
		float shadowPower = 2.4f;				// ssao/shadow_power
		float shadowClamp = 1.0f;				// ssao/shadow_clamp
		float horizonAngleThreshold = 0.4f;		// ssao/horizon_angle_threshold
		float fadeOutFromM = 1000.0f;			// ssao/fade_out_from
		float fadeOutToM = 10000.0f;			// ssao/fade_out_to
		float fadeInFromM = 0.5f;				// ssao/fade_in_from: X-Plane's own, no occlusion this close to the camera
		float fadeInToM = 2.0f;					// ssao/fade_in_to: full occlusion from here
		float adaptiveQualityLimit = 0.75f;		// ssao/adaptive_quality_limit (highest quality)
		uint32_t blurPassCount = 3;				// ssao/blur_pass_count (0-8)
		float sharpness = 1.0f;					// ssao/sharpness: 1 never blurs across edges
		float detailShadowStrength = 0.0f;		// ssao/detail_shadow_strength
		float bilateralSigmaSquared = 2.4f;		// ssao/bilateral_sigma_squared (half-resolution upscale)
		float bilateralSimilarityDistanceSigma = 2.7f;	// ssao/bilateral_similarity_distance_sigma
		float exteriorStrength = 1.0f;			// ssao/exterior_strength: share of the occlusion applied (ASSUMPTION)
		float exteriorCurve = 1.0f;				// ssao/exterior_curve: power on the visibility (ASSUMPTION)
		bool dynamicValues = true;				// ssao/dynamic_values: radius and fade-in follow the camera's height
	};

	/*
	 * X-Plane 12.4.4 with ssao/dynamic_values on (the reference probe's dumps, 2026-10-20): the camera 2 or 8 m above the
	 * ground used radius 2.1 m and a fade-in to 2 m, 600 m up radius 22 m and a fade-in to 100 m. SEDX scales both with the
	 * height above the ground through those points (ASSUMPTION: no dump between them yet). The dumps also show a second set
	 * (radius 3.9, multiplier 1.5, fade-in 2-10 m) under a "visibility" factor that follows fog in ways the dumps don't pin
	 * down; it isn't applied.
	 */
	CacaoSettings ApplyCacaoDynamicValues(const CacaoSettings &settings, double cameraHeightAboveGroundM);

	/* @brief FfxCacaoBufferSizeInfo (without the offsets, always 0). */
	struct CacaoBufferSizes
	{
		uint32_t outputWidth = 0;				// Input depth / normals and the output AO
		uint32_t outputHeight = 0;
		uint32_t ssaoWidth = 0;					// One deinterleaved layer of the SSAO buffer
		uint32_t ssaoHeight = 0;
		uint32_t deinterleavedDepthWidth = 0;
		uint32_t deinterleavedDepthHeight = 0;
		uint32_t importanceWidth = 0;
		uint32_t importanceHeight = 0;
		bool downsampled = false;				// Half resolution: the SSAO buffer's layers are a quarter of the output
	};

	/* @brief ffxCacaoUpdateBufferSizeInfo: the buffers for an output of width x height, at half resolution or not. */
	CacaoBufferSizes ComputeCacaoBufferSizes(uint32_t width, uint32_t height, bool downsampled);

	/* @brief The camera CACAO's view space follows: x right, y up, z forward (depth), seen through a reverse-Z projection. */
	struct CacaoCamera
	{
		double tanHalfFovX = 1.0;
		double tanHalfFovY = 1.0;
		double nearPlane = 0.1;					// Depth 1
		double farPlane = 10000.0;				// Depth 0
		std::array<double, 3> right = { 1.0, 0.0, 0.0 };	// World directions
		std::array<double, 3> up = { 0.0, 1.0, 0.0 };
		std::array<double, 3> forward = { 0.0, 0.0, -1.0 };
	};

	using CacaoVec4 = std::array<float, 4>;

	/*
	 * The constant buffer of cacao.slang (binding 202, struct CacaoConstants there): FfxCacaoConstants regrouped into
	 * float4s so the C++ and std140 layouts can't differ, 768 bytes (a multiple of the dynamic-offset alignment).
	 */
	struct CacaoConstants
	{
		CacaoVec4 depthUnpack;			// View depth = x / (y - depth); zw: tan(fov / 2) in x, y
		CacaoVec4 ndcToView;			// xy multiply, zw add: screen uv (0-1, y down) x view depth -> view xy
		CacaoVec4 depthUvToView;		// The same for the depth buffer's uv
		CacaoVec4 effect;				// Radius, shadow strength (multiplier x 4.3), shadow power, shadow clamp
		CacaoVec4 fade;					// Fade-out multiply, add; fade-in multiply, add (both against view depth)
		CacaoVec4 sampling;				// Horizon angle threshold, 1 / sampling radius near limit, depth precision offset, -1 / radius
		CacaoVec4 adaptive;				// Load counter average divisor, adaptive sample count limit, 1 - sharpness, detail AO strength
		CacaoVec4 bilateral;			// Sigma squared, similarity distance sigma, blur passes, exterior curve
		CacaoVec4 ssaoSize;				// Width, height, 1 / width, 1 / height
		CacaoVec4 depthSize;
		CacaoVec4 outputSize;
		CacaoVec4 importanceSize;
		CacaoVec4 deinterleavedSize;
		CacaoVec4 output;				// Exterior strength, quality (CacaoQuality), downsampled (0 / 1), 0
		std::array<CacaoVec4, CACAO_LAYERS> perPassFullResUvOffset;
		std::array<CacaoVec4, CACAO_LAYERS * CACAO_PATTERN_SUBPASSES> patternRotScale;	// [layer x 5 + sub-pass]: 2 x 2 rows
		std::array<CacaoVec4, 3> normalsToView;	// Rows: world normal -> CACAO view space
		std::array<CacaoVec4, 7> padding;
	};
	static_assert(sizeof(CacaoConstants) == 768, "CacaoConstants mirrors cacao.slang's 48 float4s");

	/* @brief ffxCacaoUpdateConstants for a quality, the buffers and the camera. */
	CacaoConstants ComputeCacaoConstants(const CacaoSettings &settings, CacaoQuality quality, const CacaoBufferSizes &sizes, const CacaoCamera &camera);

	/* @brief View depth of a reverse-Z depth-buffer value, as the shaders compute it (FFX_CACAO_ScreenSpaceToViewSpaceDepth). */
	double CacaoViewDepth(const CacaoConstants &constants, double depth);

	/* @brief The quality the shaders run for a requested one: X-Plane's presets use medium and up. */
	CacaoQuality GetCacaoRunQuality(CacaoQuality requested);

	/* @brief One compute dispatch of CACAO, in the order cacao.slang's kernels run. */
	enum class CacaoKernel : uint8_t
	{
		PrepareDepths,		// Deinterleaved view depths and 3 smart-averaged mips (CACAO's prepare *_depths_and_mips)
		PrepareNormals,		// The g-buffer's normals in CACAO's view space, deinterleaved (prepare *_normals_from_input_normals)
		GenerateBase,		// Highest: the adaptive base pass (5 taps) into the pong buffer
		ImportanceMap,		// Highest: where the base pass varies (also zeroes the load counter)
		ImportanceMapA,		// Highest: two smoothing passes; B adds to the load counter
		ImportanceMapB,
		Generate,			// The obscurance at the quality's taps into the ping buffer
		Blur,				// Edge-sensitive blur, ping -> pong
		Apply,				// Full resolution: the deinterleaved layers back into one image
		Upscale,			// Half resolution: bilateral 5 x 5 upscale to the output
	};

	struct CacaoDispatch
	{
		CacaoKernel kernel = CacaoKernel::PrepareDepths;
		uint32_t groupsX = 1;
		uint32_t groupsY = 1;
		uint32_t groupsZ = 1;
		bool readsBlurred = false;	// Apply / Upscale read the pong buffer (blurred) instead of ping
	};

	/* @brief The dispatches of one CACAO frame (cacaoDispatch). */
	std::vector<CacaoDispatch> PlanCacaoDispatches(CacaoQuality quality, const CacaoBufferSizes &sizes, uint32_t blurPassCount);

	/* @brief Thread-group sizes of cacao.slang's kernels (FFX_CACAO_* in ffx_cacao_defines.h). */
	inline constexpr uint32_t CACAO_GROUP_PREPARE = 8;
	inline constexpr uint32_t CACAO_GROUP_GENERATE = 8;
	inline constexpr uint32_t CACAO_GROUP_SPARSE_X = 4;
	inline constexpr uint32_t CACAO_GROUP_SPARSE_Y = 16;
	inline constexpr uint32_t CACAO_GROUP_IMPORTANCE = 8;
	inline constexpr uint32_t CACAO_GROUP_BLUR = 16;
	inline constexpr uint32_t CACAO_GROUP_APPLY = 8;
	inline constexpr uint32_t CACAO_GROUP_UPSCALE = 8;
}

// -------------------------------------------------------
