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
 * BrdfLutTests.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/environment/brdf_lut.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>

// -------------------------------------------------------

namespace fs = std::filesystem;
using namespace SceneryEditorX::Environment;
using Catch::Matchers::WithinAbs;

namespace
{
	float A(const float nDotV, const float roughness)
	{
		float a = 0.0f;
		float b = 0.0f;
		IntegrateBrdf(nDotV, roughness, BRDF_LUT_SAMPLES, a, b);
		return a;
	}

	float B(const float nDotV, const float roughness)
	{
		float a = 0.0f;
		float b = 0.0f;
		IntegrateBrdf(nDotV, roughness, BRDF_LUT_SAMPLES, a, b);
		return b;
	}

	/* @brief <X-Plane 12>/Resources/brdf_lookup of the first install listed in x-plane_install_12.txt, or empty. */
	fs::path FindBrdfLookup()
	{
		for (const fs::path &root : SceneryEditorX::VFS::FindXPlaneInstalls(12))
		{
			const fs::path file = root / "Resources" / "brdf_lookup";
			if (fs::exists(file))
				return file;
		}
		return {};
	}
}

TEST_CASE("The BRDF table follows the split-sum integrals of GGX with height-correlated Smith", "[brdf]")
{
	SECTION("A smooth surface seen head-on reflects exactly F0")
	{
		CHECK_THAT(A(1.0f, 0.0f), WithinAbs(1.0, 1e-4));
		CHECK_THAT(B(1.0f, 0.0f), WithinAbs(0.0, 1e-4));
	}

	SECTION("At grazing angles a smooth surface reflects everything (Schlick's F90 = 1)")
	{
		const float nv = 1.0f / 256.0f;
		CHECK_THAT(A(nv, 0.0f) + B(nv, 0.0f), WithinAbs(1.0, 2e-3));
		CHECK(B(nv, 0.0f) > 0.95f);
	}

	SECTION("Roughness loses energy to shadowing and masking (single scattering)")
	{
		float previous = 2.0f;
		for (int r = 0; r <= 8; ++r)
		{
			const float roughness = static_cast<float>(r) / 8.0f;
			const float total = A(0.5f, roughness) + B(0.5f, roughness);
			CHECK(total <= previous + 1e-4f);
			previous = total;
		}
		// Fully rough, head-on: under a third of F0 survives (the converged integral is 0.3069)
		CHECK_THAT(A(1.0f, 1.0f), WithinAbs(0.3069, 2e-3));
	}

	SECTION("Mid-table values")
	{
		// Converged values of the integrals
		CHECK_THAT(A(0.5f, 0.5f), WithinAbs(0.8338, 2e-3));
		CHECK_THAT(B(0.5f, 0.5f), WithinAbs(0.0219, 1e-3));
	}

	SECTION("The table is laid out N.V across, roughness down, at value = index / size")
	{
		const BrdfLut lut = ComputeBrdfLut(16, 256);
		REQUIRE(lut.size == 16);
		REQUIRE(lut.values.size() == 16u * 16u * 2u);
		float a = 0.0f;
		float b = 0.0f;
		IntegrateBrdf(5.0f / 16.0f, 9.0f / 16.0f, 256, a, b);
		CHECK(lut.A(5, 9) == a);
		CHECK(lut.B(5, 9) == b);
		CHECK(lut.A(15, 0) > lut.A(0, 0));	// Head-on reflects F0 itself, grazing the white Fresnel term
	}
}

TEST_CASE("The BRDF table matches X-Plane 12's Resources/brdf_lookup", "[brdf][xplane-install]")
{
	const fs::path file = FindBrdfLookup();
	if (file.empty())
	{
		WARN("No X-Plane 12 install found (x-plane_install_12.txt): skipped");
		return;
	}

	// 256 x 256 RG32F, N.V across, roughness down, no header
	std::ifstream in(file, std::ios::binary);
	std::vector<float> reference(BRDF_LUT_SIZE * BRDF_LUT_SIZE * 2);
	in.read(reinterpret_cast<char *>(reference.data()), static_cast<std::streamsize>(reference.size() * sizeof(float)));
	REQUIRE(static_cast<size_t>(in.gcount()) == reference.size() * sizeof(float));

	const BrdfLut lut = ComputeBrdfLut();
	float maxError = 0.0f;
	uint32_t worstX = 0;
	uint32_t worstY = 0;
	for (uint32_t y = 0; y < BRDF_LUT_SIZE; ++y)
	{
		// Column 0 (N.V = 0) is single-precision noise in the shipped table (not monotonic in roughness); shading never
		// samples it because N.V is clamped above it
		for (uint32_t x = 1; x < BRDF_LUT_SIZE; ++x)
		{
			const size_t i = (static_cast<size_t>(y) * BRDF_LUT_SIZE + x) * 2;
			const float error = std::max(std::abs(lut.values[i] - reference[i]), std::abs(lut.values[i + 1] - reference[i + 1]));
			if (error > maxError)
			{
				maxError = error;
				worstX = x;
				worstY = y;
			}
		}
	}
	INFO("Worst texel (" << worstX << ", " << worstY << "): " << maxError);
	CHECK(maxError <= 2e-3f);
}

TEST_CASE("The GPU's BRDF table matches the CPU's", "[brdf][gpu-dump]")
{
	// SceneryEditorX --dump-brdf-lut <file> writes the table light_integration.slang made; point SEDX_BRDF_LUT_DUMP at it
	const char *dump = std::getenv("SEDX_BRDF_LUT_DUMP");
	if (!dump || !fs::exists(dump))
	{
		WARN("SEDX_BRDF_LUT_DUMP names no --dump-brdf-lut file: skipped");
		return;
	}

	std::ifstream in(dump, std::ios::binary);
	std::vector<float> gpu(BRDF_LUT_SIZE * BRDF_LUT_SIZE * 2);
	in.read(reinterpret_cast<char *>(gpu.data()), static_cast<std::streamsize>(gpu.size() * sizeof(float)));
	REQUIRE(static_cast<size_t>(in.gcount()) == gpu.size() * sizeof(float));

	// Every 8th texel (32 x 32), N.V = 0 excluded as above
	float maxError = 0.0f;
	for (uint32_t y = 0; y < BRDF_LUT_SIZE; y += 8)
	{
		for (uint32_t x = 8; x < BRDF_LUT_SIZE; x += 8)
		{
			float a = 0.0f;
			float b = 0.0f;
			IntegrateBrdf(static_cast<float>(x) / BRDF_LUT_SIZE, static_cast<float>(y) / BRDF_LUT_SIZE, BRDF_LUT_SAMPLES, a, b);
			const size_t i = (static_cast<size_t>(y) * BRDF_LUT_SIZE + x) * 2;
			maxError = std::max({ maxError, std::abs(gpu[i] - a), std::abs(gpu[i + 1] - b) });
		}
	}
	INFO("Largest difference: " << maxError);
	CHECK(maxError <= 2e-3f);
}

// -------------------------------------------------------
