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
 * xp_block_compress.cpp
 * -------------------------------------------------------
 * Created: 26/10/2026
 * -------------------------------------------------------
 */
#include "xp_block_compress.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane::BlockCompress
{
	void ReadBlock(const uint8_t *rgba, const uint32_t width, const uint32_t height, const uint32_t bx, const uint32_t by, uint8_t block[64])
	{
		for (uint32_t y = 0; y < 4; ++y)
		{
			const uint32_t sy = std::min(by * 4 + y, height - 1);
			for (uint32_t x = 0; x < 4; ++x)
			{
				const uint32_t sx = std::min(bx * 4 + x, width - 1);
				std::memcpy(block + (y * 4 + x) * 4, rgba + (static_cast<size_t>(sy) * width + sx) * 4, 4);
			}
		}
	}

	void EncodeBc4(const uint8_t values[16], uint8_t out[8])
	{
		int high = 0, low = 255;
		for (int i = 0; i < 16; ++i)
		{
			high = std::max<int>(high, values[i]);
			low = std::min<int>(low, values[i]);
		}

		// Endpoint 0 above endpoint 1 selects the eight-value mode: index 0 = high, 1 = low, 2-7 the steps from high
		// down. A flat block keeps both endpoints equal and every index 0.
		out[0] = static_cast<uint8_t>(high);
		out[1] = static_cast<uint8_t>(low);
		uint64_t indices = 0;
		if (high > low)
		{
			int palette[8];
			palette[0] = high;
			palette[1] = low;
			for (int k = 1; k < 7; ++k)
				palette[k + 1] = ((7 - k) * high + k * low + 3) / 7;

			for (int i = 0; i < 16; ++i)
			{
				int best = 0, bestError = 256;
				for (int k = 0; k < 8; ++k)
				{
					const int error = std::abs(palette[k] - values[i]);
					if (error < bestError)
					{
						bestError = error;
						best = k;
					}
				}
				indices |= static_cast<uint64_t>(best) << (3 * i);
			}
		}
		for (int k = 0; k < 6; ++k)
			out[2 + k] = static_cast<uint8_t>((indices >> (8 * k)) & 0xFFu);
	}

	void EncodeBc5(const uint8_t block[64], const int channel, uint8_t out[16])
	{
		uint8_t first[16], second[16];
		for (int i = 0; i < 16; ++i)
		{
			first[i] = block[i * 4 + channel];
			second[i] = block[i * 4 + channel + 1];
		}
		EncodeBc4(first, out);
		EncodeBc4(second, out + 8);
	}

	uint32_t BlocksAcross(const uint32_t width)
	{
		return std::max(1u, (width + 3) / 4);
	}

	uint32_t BlocksDown(const uint32_t height)
	{
		return std::max(1u, (height + 3) / 4);
	}

	void CompressBc5Rows(const uint8_t *rgba, const uint32_t width, const uint32_t height, const int channel, const uint32_t firstRow, const uint32_t endRow, std::byte *out)
	{
		const uint32_t across = BlocksAcross(width);
		uint8_t block[64];
		for (uint32_t by = firstRow; by < endRow; ++by)
		{
			for (uint32_t bx = 0; bx < across; ++bx)
			{
				ReadBlock(rgba, width, height, bx, by, block);
				EncodeBc5(block, channel, reinterpret_cast<uint8_t *>(out + (static_cast<size_t>(by) * across + bx) * 16));
			}
		}
	}

	std::vector<std::byte> CompressBc5(const uint8_t *rgba, const uint32_t width, const uint32_t height, const int channel)
	{
		std::vector<std::byte> out(static_cast<size_t>(BlocksAcross(width)) * BlocksDown(height) * 16);
		CompressBc5Rows(rgba, width, height, channel, 0, BlocksDown(height), out.data());
		return out;
	}
}

// -------------------------------------------------------
