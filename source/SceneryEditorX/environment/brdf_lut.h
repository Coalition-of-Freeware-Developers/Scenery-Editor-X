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
 * brdf_lut.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	/**
	 * The split-sum specular table (Karis 2013) for the BRDF X-Plane 12 shades with: GGX with alpha = roughness^2,
	 * height-correlated Smith visibility, Schlick Fresnel. Texel (x, y) holds the scale (A) and bias (B) applied to F0 at
	 * N.V = x / size and perceptual roughness = y / size; light_integration.slang computes the same table on the GPU and
	 * the shaders sample it at uv = value + 0.5 / size.
	 */
	struct BrdfLut
	{
		uint32_t size = 0;
		std::vector<float> values;	// size * size RG pairs, rows of increasing roughness

		[[nodiscard]] float A(const uint32_t x, const uint32_t y) const { return values[(static_cast<size_t>(y) * size + x) * 2]; }
		[[nodiscard]] float B(const uint32_t x, const uint32_t y) const { return values[(static_cast<size_t>(y) * size + x) * 2 + 1]; }
	};

	constexpr uint32_t BRDF_LUT_SIZE = 256;
	constexpr uint32_t BRDF_LUT_SAMPLES = 1024;		// Hammersley points per texel
	constexpr float BRDF_LUT_MIN_N_DOT_V = 1e-5f;
	constexpr float BRDF_LUT_MIN_ROUGHNESS = 1e-3f;

	/* @brief One texel of the table: the integrals A and B at a N.V and perceptual roughness. */
	void IntegrateBrdf(float nDotV, float roughness, uint32_t sampleCount, float &a, float &b);

	/* @brief The whole table (size x size, a few hundred milliseconds at the defaults). */
	BrdfLut ComputeBrdfLut(uint32_t size = BRDF_LUT_SIZE, uint32_t sampleCount = BRDF_LUT_SAMPLES);
}

// -------------------------------------------------------
