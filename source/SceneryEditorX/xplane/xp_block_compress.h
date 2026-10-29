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
 * xp_block_compress.h
 * -------------------------------------------------------
 * Created: 26/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane::BlockCompress
{
	/**
	 * Block compression helpers for TextureLoader, standard library only: 4x4 blocks out of RGBA8 images and the BC4 /
	 * BC5 encoder X-Plane 12's normal-map split needs (BC5 normal, BC5 blue + alpha). BC1 / BC3 colour blocks come from
	 * stb_dxt in the loader.
	 */

	/**
	 * @brief Copies the 4x4 block at block column bx, row by out of an RGBA8 image (rows of width x 4 bytes); pixels
	 * past the right or bottom edge repeat the last column / row, so mips under 4 texels fill whole blocks.
	 * @param block 16 pixels, rows of 4, 4 bytes each.
	 */
	void ReadBlock(const uint8_t *rgba, uint32_t width, uint32_t height, uint32_t bx, uint32_t by, uint8_t block[64]);

	/**
	 * @brief Encodes 16 values (a block's channel, rows of 4) as a BC4 block: the extremes as endpoints, six steps
	 * between them, each value on its nearest step.
	 */
	void EncodeBc4(const uint8_t values[16], uint8_t out[8]);

	/* @brief Encodes two channels of an RGBA block (channel and channel + 1) as a BC5 block (16 bytes: two BC4 halves). */
	void EncodeBc5(const uint8_t block[64], int channel, uint8_t out[16]);

	/* @brief Blocks across and down for a mip (at least one each). */
	uint32_t BlocksAcross(uint32_t width);
	uint32_t BlocksDown(uint32_t height);

	/**
	 * @brief Encodes block rows [firstRow, endRow) of an RGBA8 image as BC5 holding two of its channels: 0 red and
	 * green (a normal map's normal), 2 blue and alpha (its F0 / translucency and gloss).
	 * @param out BlocksAcross(width) x BlocksDown(height) x 16 bytes, rows of blocks top to bottom as the image's rows.
	 */
	void CompressBc5Rows(const uint8_t *rgba, uint32_t width, uint32_t height, int channel, uint32_t firstRow, uint32_t endRow, std::byte *out);

	/* @brief A whole image as BC5 (CompressBc5Rows over every block row). */
	std::vector<std::byte> CompressBc5(const uint8_t *rgba, uint32_t width, uint32_t height, int channel);
}

// -------------------------------------------------------
