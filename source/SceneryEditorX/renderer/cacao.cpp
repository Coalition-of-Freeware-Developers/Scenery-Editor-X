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
 * cacao.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 *
 * Ported from AMD FidelityFX CACAO 1.4 (FidelityFX SDK 1.1.4, src/components/cacao/ffx_cacao.cpp):
 *
 * Copyright (C) 2024 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "cacao.h"
#include <algorithm>
#include <cmath>
#include <numbers>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		// X-Plane 12.4.4's dynamic values 600 m above the ground (radius 22 m, fade-in to 100 m), as a share of the height
		constexpr double DYNAMIC_RADIUS_PER_M = 22.0 / 600.0;
		constexpr double DYNAMIC_FADE_IN_PER_M = 100.0 / 600.0;

		uint32_t DivideRoundingUp(const uint32_t value, const uint32_t divisor)
		{
			return (value + divisor - 1) / divisor;
		}

		CacaoVec4 Size(const uint32_t width, const uint32_t height)
		{
			const float w = static_cast<float>(std::max(width, 1u));
			const float h = static_cast<float>(std::max(height, 1u));
			return { w, h, 1.0f / w, 1.0f / h };
		}

		CacaoVec4 Row(const std::array<double, 3> &v)
		{
			return { static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]), 0.0f };
		}
	}

	CacaoSettings ApplyCacaoDynamicValues(const CacaoSettings &settings, const double cameraHeightAboveGroundM)
	{
		CacaoSettings result = settings;
		if (!settings.dynamicValues)
			return result;
		const double height = std::max(cameraHeightAboveGroundM, 0.0);
		result.radius = static_cast<float>(std::max<double>(settings.radius, DYNAMIC_RADIUS_PER_M * height));
		result.fadeInToM = static_cast<float>(std::max<double>(settings.fadeInToM, DYNAMIC_FADE_IN_PER_M * height));
		return result;
	}

	CacaoBufferSizes ComputeCacaoBufferSizes(const uint32_t width, const uint32_t height, const bool downsampled)
	{
		const uint32_t halfWidth = (width + 1) / 2;
		const uint32_t halfHeight = (height + 1) / 2;
		const uint32_t quarterWidth = (halfWidth + 1) / 2;
		const uint32_t quarterHeight = (halfHeight + 1) / 2;
		const uint32_t eighthWidth = (quarterWidth + 1) / 2;
		const uint32_t eighthHeight = (quarterHeight + 1) / 2;

		CacaoBufferSizes sizes;
		sizes.outputWidth = width;
		sizes.outputHeight = height;
		sizes.downsampled = downsampled;
		if (downsampled)
		{
			sizes.ssaoWidth = quarterWidth;
			sizes.ssaoHeight = quarterHeight;
			sizes.deinterleavedDepthWidth = quarterWidth;
			sizes.deinterleavedDepthHeight = quarterHeight;
			sizes.importanceWidth = eighthWidth;
			sizes.importanceHeight = eighthHeight;
		}
		else
		{
			sizes.ssaoWidth = halfWidth;
			sizes.ssaoHeight = halfHeight;
			sizes.deinterleavedDepthWidth = halfWidth;
			sizes.deinterleavedDepthHeight = halfHeight;
			sizes.importanceWidth = quarterWidth;
			sizes.importanceHeight = quarterHeight;
		}
		return sizes;
	}

	CacaoQuality GetCacaoRunQuality(const CacaoQuality requested)
	{
		return std::clamp(requested, CacaoQuality::Medium, CacaoQuality::Highest);
	}

	CacaoConstants ComputeCacaoConstants(const CacaoSettings &settings, const CacaoQuality quality, const CacaoBufferSizes &sizes, const CacaoCamera &camera)
	{
		CacaoConstants c{};

		// Reverse-Z (depth = n (f / z - 1) / (f - n)) solved for the view depth z in CACAO's form z = mul / (add - depth)
		const double n = std::max(camera.nearPlane, 1e-4);
		const double f = std::max(camera.farPlane, n * 1.001);
		const float depthLinearizeMul = static_cast<float>(-n * f / (f - n));
		const float depthLinearizeAdd = static_cast<float>(-n / (f - n));
		const float tanHalfFovX = static_cast<float>(camera.tanHalfFovX);
		const float tanHalfFovY = static_cast<float>(camera.tanHalfFovY);
		c.depthUnpack = { depthLinearizeMul, depthLinearizeAdd, tanHalfFovX, tanHalfFovY };

		// The depth buffer is the output's size, so its uv maps to view space as the screen's does
		c.ndcToView = { tanHalfFovX * 2.0f, tanHalfFovY * -2.0f, -tanHalfFovX, tanHalfFovY };
		c.depthUvToView = c.ndcToView;

		const float radius = std::clamp(settings.radius, 0.0f, 100000.0f);
		c.effect = { radius, std::clamp(settings.shadowMultiplier * 4.3f, 0.0f, 10.0f), std::clamp(settings.shadowPower, 0.0f, 10.0f),
			std::clamp(settings.shadowClamp, 0.0f, 1.0f) };

		// Fade-out (1 near, 0 far) and X-Plane's fade-in: occlusion x min(fade-out, 1 - saturate(depth x mul + add))
		const float fadeOutRange = std::max(settings.fadeOutToM - settings.fadeOutFromM, 1e-3f);
		const float fadeInRange = std::max(settings.fadeInToM - settings.fadeInFromM, 1e-3f);
		c.fade = { -1.0f / fadeOutRange, settings.fadeOutFromM / fadeOutRange + 1.0f, -1.0f / fadeInRange, settings.fadeInFromM / fadeInRange + 1.0f };

		// 1.2 keeps the on-screen sampling disk from growing past most of the screen close to the camera, independent of
		// the field of view (AMD's comment); quality low and lowest (not run) would widen it
		const float samplingRadiusNearLimit = radius * 1.2f / std::max(tanHalfFovY, 1e-6f);
		c.sampling = { std::clamp(settings.horizonAngleThreshold, 0.0f, 1.0f), 1.0f / std::max(samplingRadiusNearLimit, 1e-6f), 0.9992f,
			-1.0f / std::max(radius, 1e-6f) };

		// 9: PostprocessImportanceMapB adds every ninth pixel's importance to the load counter
		const double importancePixels = static_cast<double>(std::max(sizes.importanceWidth, 1u)) * static_cast<double>(std::max(sizes.importanceHeight, 1u));
		c.adaptive = { static_cast<float>(9.0 / (importancePixels * 255.0)), settings.adaptiveQualityLimit, std::clamp(1.0f - settings.sharpness, 0.0f, 1.0f),
			settings.detailShadowStrength };

		c.bilateral = { settings.bilateralSigmaSquared, settings.bilateralSimilarityDistanceSigma,
			static_cast<float>(std::min(settings.blurPassCount, CACAO_MAX_BLUR_PASSES)), std::max(settings.exteriorCurve, 0.0f) };

		c.ssaoSize = Size(sizes.ssaoWidth, sizes.ssaoHeight);
		c.depthSize = Size(sizes.outputWidth, sizes.outputHeight);
		c.outputSize = Size(sizes.outputWidth, sizes.outputHeight);
		c.importanceSize = Size(sizes.importanceWidth, sizes.importanceHeight);
		c.deinterleavedSize = Size(sizes.deinterleavedDepthWidth, sizes.deinterleavedDepthHeight);
		c.output = { std::clamp(settings.exteriorStrength, 0.0f, 1.0f), static_cast<float>(GetCacaoRunQuality(quality)), sizes.downsampled ? 1.0f : 0.0f, 0.0f };

		for (uint32_t pass = 0; pass < CACAO_LAYERS; ++pass)
		{
			c.perPassFullResUvOffset[pass] = { static_cast<float>(pass % 2) / c.ssaoSize[0], static_cast<float>(pass / 2) / c.ssaoSize[1], 0.0f, 0.0f };
		}

		// The sampling pattern's rotation and scale per layer and sub-pass (no temporal supersampling offsets)
		constexpr int SUB_PASS_MAP[CACAO_PATTERN_SUBPASSES] = { 0, 1, 4, 3, 2 };
		constexpr float SUB_PASSES = static_cast<float>(CACAO_PATTERN_SUBPASSES);
		for (uint32_t pass = 0; pass < CACAO_LAYERS; ++pass)
		{
			for (uint32_t subPass = 0; subPass < CACAO_PATTERN_SUBPASSES; ++subPass)
			{
				const float a = static_cast<float>(pass);
				const float b = static_cast<float>(SUB_PASS_MAP[subPass]);
				const float angle = (a + b / SUB_PASSES) * std::numbers::pi_v<float> * 0.5f;
				const float ca = std::cos(angle);
				const float sa = std::sin(angle);
				const float scale = 1.0f + (a - 1.5f + (b - (SUB_PASSES - 1.0f) * 0.5f) / SUB_PASSES) * 0.07f;
				c.patternRotScale[pass * CACAO_PATTERN_SUBPASSES + subPass] = { scale * ca, scale * -sa, -scale * sa, -scale * ca };
			}
		}

		c.normalsToView = { Row(camera.right), Row(camera.up), Row(camera.forward) };
		return c;
	}

	double CacaoViewDepth(const CacaoConstants &constants, const double depth)
	{
		// ffxInvertSafe: 1 / x, 0 for 0
		const double denominator = static_cast<double>(constants.depthUnpack[1]) - depth;
		return denominator == 0.0 ? 0.0 : static_cast<double>(constants.depthUnpack[0]) / denominator;
	}

	std::vector<CacaoDispatch> PlanCacaoDispatches(const CacaoQuality requested, const CacaoBufferSizes &sizes, const uint32_t blurPassCount)
	{
		const CacaoQuality quality = GetCacaoRunQuality(requested);
		const uint32_t blurPasses = std::min(blurPassCount, CACAO_MAX_BLUR_PASSES);
		std::vector<CacaoDispatch> dispatches;

		const auto add = [&dispatches](const CacaoKernel kernel, const uint32_t x, const uint32_t y, const uint32_t z, const bool readsBlurred = false)
		{
			dispatches.push_back({ kernel, std::max(x, 1u), std::max(y, 1u), std::max(z, 1u), readsBlurred });
		};

		// Prepare: depths and their mips (medium and up), normals from the g-buffer
		add(CacaoKernel::PrepareDepths, DivideRoundingUp(sizes.deinterleavedDepthWidth, CACAO_GROUP_PREPARE),
			DivideRoundingUp(sizes.deinterleavedDepthHeight, CACAO_GROUP_PREPARE), 1);
		add(CacaoKernel::PrepareNormals, DivideRoundingUp(sizes.ssaoWidth, CACAO_GROUP_PREPARE), DivideRoundingUp(sizes.ssaoHeight, CACAO_GROUP_PREPARE), 1);

		// Highest: the base pass and the importance map that steers the adaptive pass
		if (quality == CacaoQuality::Highest)
		{
			add(CacaoKernel::GenerateBase, DivideRoundingUp(sizes.ssaoWidth, CACAO_GROUP_GENERATE), DivideRoundingUp(sizes.ssaoHeight, CACAO_GROUP_GENERATE), CACAO_LAYERS);
			const uint32_t x = DivideRoundingUp(sizes.importanceWidth, CACAO_GROUP_IMPORTANCE);
			const uint32_t y = DivideRoundingUp(sizes.importanceHeight, CACAO_GROUP_IMPORTANCE);
			add(CacaoKernel::ImportanceMap, x, y, 1);
			add(CacaoKernel::ImportanceMapA, x, y, 1);
			add(CacaoKernel::ImportanceMapB, x, y, 1);
		}

		// Generate: medium runs every fifth pixel of a row per slice (5 slices per layer)
		if (quality == CacaoQuality::Medium)
		{
			const uint32_t x = (DivideRoundingUp(sizes.ssaoWidth, CACAO_GROUP_SPARSE_X) + 4) / 5;
			add(CacaoKernel::Generate, x, DivideRoundingUp(sizes.ssaoHeight, CACAO_GROUP_SPARSE_Y), 5 * CACAO_LAYERS);
		}
		else
		{
			add(CacaoKernel::Generate, DivideRoundingUp(sizes.ssaoWidth, CACAO_GROUP_GENERATE), DivideRoundingUp(sizes.ssaoHeight, CACAO_GROUP_GENERATE), CACAO_LAYERS);
		}

		// Blur: each 16 x 16 group covers a 64 x 48 tile less the passes' borders
		if (blurPasses > 0)
		{
			const uint32_t tileWidth = 4 * CACAO_GROUP_BLUR - 2 * blurPasses;
			const uint32_t tileHeight = 3 * CACAO_GROUP_BLUR - 2 * blurPasses;
			add(CacaoKernel::Blur, DivideRoundingUp(sizes.ssaoWidth, tileWidth), DivideRoundingUp(sizes.ssaoHeight, tileHeight), CACAO_LAYERS);
		}

		// Output: the bilateral upscale writes 2 x 2 pixels per thread
		if (sizes.downsampled)
			add(CacaoKernel::Upscale, DivideRoundingUp(sizes.outputWidth, 2 * CACAO_GROUP_UPSCALE), DivideRoundingUp(sizes.outputHeight, 2 * CACAO_GROUP_UPSCALE), 1, blurPasses > 0);
		else
			add(CacaoKernel::Apply, DivideRoundingUp(sizes.outputWidth, CACAO_GROUP_APPLY), DivideRoundingUp(sizes.outputHeight, CACAO_GROUP_APPLY), 1, blurPasses > 0);
		return dispatches;
	}
}

// -------------------------------------------------------
