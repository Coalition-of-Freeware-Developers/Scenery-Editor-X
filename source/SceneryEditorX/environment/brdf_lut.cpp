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
 * brdf_lut.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "brdf_lut.h"
#include <algorithm>
#include <cmath>
#include <numbers>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	namespace
	{
		/* @brief Van der Corput radical inverse in base 2 (the second Hammersley coordinate). */
		double RadicalInverse(uint32_t bits)
		{
			bits = (bits << 16u) | (bits >> 16u);
			bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
			bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
			bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
			bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
			return static_cast<double>(bits) * 2.3283064365386963e-10;
		}
	}

	void IntegrateBrdf(const float nDotV, const float roughness, const uint32_t sampleCount, float &a, float &b)
	{
		// Importance sampled GGX half vectors around N = +Z, V in the XZ plane
		const double nv = std::clamp(static_cast<double>(nDotV), static_cast<double>(BRDF_LUT_MIN_N_DOT_V), 1.0);
		const double r = std::clamp(static_cast<double>(roughness), static_cast<double>(BRDF_LUT_MIN_ROUGHNESS), 1.0);
		const double alpha = r * r;
		const double alpha2 = alpha * alpha;
		const double vx = std::sqrt(std::max(1.0 - nv * nv, 0.0));
		const double vz = nv;

		double sumA = 0.0;
		double sumB = 0.0;
		for (uint32_t i = 0; i < sampleCount; ++i)
		{
			const double phi = 2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(sampleCount);
			const double xi = RadicalInverse(i);
			const double cosTheta = std::sqrt((1.0 - xi) / (1.0 + (alpha2 - 1.0) * xi));
			const double sinTheta = std::sqrt(std::max(1.0 - cosTheta * cosTheta, 0.0));
			const double hx = std::cos(phi) * sinTheta;
			const double hz = cosTheta;

			const double vDotH = vx * hx + vz * hz;
			const double nl = 2.0 * vDotH * hz - vz;	// L = reflect(-V, H), its z
			if (nl <= 0.0)
				continue;

			// Height-correlated Smith (Heitz 2014): V = G2 / (4 N.L N.V); the sample weight is G2 V.H / (N.H N.V)
			const double nh = std::max(hz, 1e-12);
			const double vh = std::max(vDotH, 0.0);
			const double lambdaV = nl * std::sqrt(nv * nv * (1.0 - alpha2) + alpha2);
			const double lambdaL = nv * std::sqrt(nl * nl * (1.0 - alpha2) + alpha2);
			const double visibility = 0.5 / (lambdaV + lambdaL);
			const double weight = 4.0 * visibility * nl * vh / nh;

			const double fc = std::pow(1.0 - vh, 5.0);	// Schlick
			sumA += (1.0 - fc) * weight;
			sumB += fc * weight;
		}

		a = static_cast<float>(sumA / sampleCount);
		b = static_cast<float>(sumB / sampleCount);
	}

	BrdfLut ComputeBrdfLut(const uint32_t size, const uint32_t sampleCount)
	{
		BrdfLut lut;
		lut.size = size;
		lut.values.resize(static_cast<size_t>(size) * size * 2);
		for (uint32_t y = 0; y < size; ++y)
		{
			for (uint32_t x = 0; x < size; ++x)
			{
				float a = 0.0f;
				float b = 0.0f;
				IntegrateBrdf(static_cast<float>(x) / static_cast<float>(size), static_cast<float>(y) / static_cast<float>(size), sampleCount, a, b);
				lut.values[(static_cast<size_t>(y) * size + x) * 2] = a;
				lut.values[(static_cast<size_t>(y) * size + x) * 2 + 1] = b;
			}
		}
		return lut;
	}
}

// -------------------------------------------------------
