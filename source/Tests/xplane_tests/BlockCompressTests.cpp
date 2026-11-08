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
 * BlockCompressTests.cpp
 * -------------------------------------------------------
 * Created: 26/10/2026
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/xplane/xp_block_compress.h>
#include <SceneryEditorX/xplane/xp_thumbnail.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>

// -------------------------------------------------------

using namespace SceneryEditorX::XPlane;

namespace
{
	/* @brief A one-mip ATI2 (BC5) DDS file around encoded blocks, for xp_thumbnail's decoder (written apart from the encoder). */
	std::vector<uint8_t> Bc5Dds(const uint32_t width, const uint32_t height, const std::vector<std::byte> &blocks)
	{
		std::vector<uint8_t> bytes(128, 0);
		const auto put = [&bytes](const size_t offset, const uint32_t value)
		{
			for (int i = 0; i < 4; ++i)
				bytes[offset + i] = static_cast<uint8_t>(value >> (8 * i));
		};
		std::memcpy(bytes.data(), "DDS ", 4);
		put(4, 124);
		put(8, 0x1 | 0x2 | 0x4 | 0x1000);
		put(12, height);
		put(16, width);
		put(28, 1);
		put(76, 32);
		put(80, 0x4);
		std::memcpy(&bytes[84], "ATI2", 4);
		put(108, 0x1000);
		const auto *data = reinterpret_cast<const uint8_t *>(blocks.data());
		bytes.insert(bytes.end(), data, data + blocks.size());
		return bytes;
	}

	/* @brief Largest difference between a decoded BC4 block and its source values (eight-value palette, as Direct3D decodes it). */
	int Bc4Error(const uint8_t values[16], const uint8_t block[8])
	{
		const int a0 = block[0], a1 = block[1];
		float palette[8] = { static_cast<float>(a0), static_cast<float>(a1) };
		if (a0 > a1)
		{
			for (int k = 1; k < 7; ++k)
				palette[k + 1] = ((7.0f - k) * a0 + k * a1) / 7.0f;
		}
		else
		{
			for (int k = 1; k < 5; ++k)
				palette[k + 1] = ((5.0f - k) * a0 + k * a1) / 5.0f;
			palette[6] = 0.0f;
			palette[7] = 255.0f;
		}
		uint64_t bits = 0;
		for (int k = 0; k < 6; ++k)
			bits |= static_cast<uint64_t>(block[2 + k]) << (8 * k);
		int worst = 0;
		for (int i = 0; i < 16; ++i)
			worst = std::max(worst, static_cast<int>(std::lround(std::abs(palette[(bits >> (3 * i)) & 7u] - values[i]))));
		return worst;
	}
}

TEST_CASE("BC4 blocks keep flat, two-level and graded values", "[texture-compression]")
{
	uint8_t block[8];

	SECTION("A flat block is exact")
	{
		uint8_t values[16];
		std::memset(values, 131, sizeof(values));
		BlockCompress::EncodeBc4(values, block);
		CHECK(Bc4Error(values, block) == 0);
	}

	SECTION("Two levels land on the endpoints")
	{
		uint8_t values[16];
		for (int i = 0; i < 16; ++i)
			values[i] = (i % 3) == 0 ? 255 : 0;
		BlockCompress::EncodeBc4(values, block);
		CHECK(block[0] == 255);
		CHECK(block[1] == 0);
		CHECK(Bc4Error(values, block) == 0);
	}

	SECTION("A ramp is within half a step of its eight levels")
	{
		uint8_t values[16];
		for (int i = 0; i < 16; ++i)
			values[i] = static_cast<uint8_t>(40 + i * 7);	// 40-145: steps of 15
		BlockCompress::EncodeBc4(values, block);
		CHECK(Bc4Error(values, block) <= 8);
	}

	SECTION("Noise stays within half a step of its range")
	{
		std::mt19937 random(7);
		for (int run = 0; run < 200; ++run)
		{
			uint8_t values[16];
			int low = 255, high = 0;
			for (uint8_t &value : values)
			{
				value = static_cast<uint8_t>(random() & 0xFF);
				low = std::min<int>(low, value);
				high = std::max<int>(high, value);
			}
			BlockCompress::EncodeBc4(values, block);
			CHECK(Bc4Error(values, block) <= (high - low) / 14 + 1);
		}
	}
}

TEST_CASE("BC5 normal maps split into normal and blue + alpha, as X-Plane 12 compresses them", "[texture-compression]")
{
	// 6 x 5: a partial block column and row; every channel a different gradient
	constexpr uint32_t W = 6, H = 5;
	std::vector<uint8_t> rgba(W * H * 4);
	for (uint32_t y = 0; y < H; ++y)
	{
		for (uint32_t x = 0; x < W; ++x)
		{
			uint8_t *p = &rgba[(y * W + x) * 4];
			p[0] = static_cast<uint8_t>(100 + x * 10);	// normal x
			p[1] = static_cast<uint8_t>(90 + y * 12);	// normal y
			p[2] = static_cast<uint8_t>(x < 3 ? 0 : 200);	// F0 / metalness: two levels
			p[3] = static_cast<uint8_t>(30 + (x + y) * 9);	// gloss
		}
	}

	CHECK(BlockCompress::BlocksAcross(W) == 2);
	CHECK(BlockCompress::BlocksDown(H) == 2);
	CHECK(BlockCompress::BlocksAcross(1) == 1);
	CHECK(BlockCompress::BlocksDown(2) == 1);

	const std::vector<std::byte> normal = BlockCompress::CompressBc5(rgba.data(), W, H, 0);
	const std::vector<std::byte> companion = BlockCompress::CompressBc5(rgba.data(), W, H, 2);
	REQUIRE(normal.size() == 4 * 16);
	REQUIRE(companion.size() == 4 * 16);

	RgbaImage decodedNormal, decodedCompanion;
	REQUIRE(DecodeDds(Bc5Dds(W, H, normal), 1, decodedNormal));
	REQUIRE(DecodeDds(Bc5Dds(W, H, companion), 1, decodedCompanion));
	REQUIRE(decodedNormal.width == W);
	REQUIRE(decodedNormal.height == H);
	REQUIRE(decodedCompanion.width == W);

	for (uint32_t y = 0; y < H; ++y)
	{
		for (uint32_t x = 0; x < W; ++x)
		{
			const uint8_t *source = &rgba[(y * W + x) * 4];
			const uint8_t *n = &decodedNormal.pixels[(y * W + x) * 4];
			const uint8_t *c = &decodedCompanion.pixels[(y * W + x) * 4];
			CAPTURE(x, y);
			CHECK(std::abs(n[0] - source[0]) <= 4);		// the normal in red and green
			CHECK(std::abs(n[1] - source[1]) <= 4);
			CHECK(std::abs(c[0] - source[2]) <= 1);		// blue as red: two levels, exact
			CHECK(std::abs(c[1] - source[3]) <= 4);		// alpha as green
		}
	}
}

TEST_CASE("Blocks past a small mip's edge repeat its last row and column", "[texture-compression]")
{
	// A 2 x 1 mip fills one block: columns 2-3 repeat column 1, rows 1-3 repeat row 0
	const uint8_t rgba[8] = { 10, 20, 30, 40, 50, 60, 70, 80 };
	uint8_t block[64];
	BlockCompress::ReadBlock(rgba, 2, 1, 0, 0, block);
	for (int y = 0; y < 4; ++y)
	{
		for (int x = 0; x < 4; ++x)
		{
			const uint8_t *expected = x == 0 ? rgba : rgba + 4;
			CAPTURE(x, y);
			CHECK(std::memcmp(block + (y * 4 + x) * 4, expected, 4) == 0);
		}
	}

	const std::vector<std::byte> one = BlockCompress::CompressBc5(rgba, 2, 1, 0);
	CHECK(one.size() == 16);
	RgbaImage decoded;
	REQUIRE(DecodeDds(Bc5Dds(2, 1, one), 1, decoded));
	CHECK(decoded.pixels[0] == 10);
	CHECK(decoded.pixels[1] == 20);
	CHECK(decoded.pixels[4] == 50);
	CHECK(decoded.pixels[5] == 60);
}

// -------------------------------------------------------
