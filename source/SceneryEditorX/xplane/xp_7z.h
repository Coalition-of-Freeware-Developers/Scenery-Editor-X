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
 * xp_7z.h
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * 7-zip archives as X-Plane ships its Global Scenery DSF tiles: one file in one folder, compressed with LZMA (or
	 * LZMA2, which tools such as Ortho4XP's 7-zip calls write), the archive header plain or itself LZMA packed. The
	 * decoder follows the LZMA format as Igor Pavlov describes it in the public domain LZMA SDK (lzma-specification.txt);
	 * it is written for SEDX, standard library only.
	 */

	/* @brief The six-byte signature every 7-zip archive starts with. */
	inline constexpr uint8_t SEVEN_ZIP_SIGNATURE[6] = { '7', 'z', 0xBC, 0xAF, 0x27, 0x1C };

	/* @brief True when the bytes start with the 7-zip signature. */
	bool Is7zArchive(const uint8_t *data, size_t size);

	/**
	 * @brief Unpacks the first file of a 7-zip archive held in memory. Coders: LZMA, LZMA2 and copy (one per folder; BCJ
	 * and other filter chains, encryption and multi-volume archives are reported, not decoded). The unpacked bytes are
	 * checked against the archive's CRC32 when it carries one.
	 */
	bool Extract7zFirstFile(const uint8_t *data, size_t size, std::vector<uint8_t> &out, std::string *error = nullptr);

	/**
	 * @brief Decodes an LZMA stream (as 7-zip stores it: no header, no end marker needed) into out, which must be sized to
	 * the unpacked length.
	 * @param properties The coder properties: the lc / lp / pb byte, then the dictionary size (4 bytes little-endian).
	 */
	bool DecodeLzma(const uint8_t *properties, size_t propertiesSize, const uint8_t *data, size_t size, uint8_t *out, size_t outSize, std::string *error = nullptr);

	/* @brief Decodes an LZMA2 stream (a chunked LZMA container) into out, sized to the unpacked length. */
	bool DecodeLzma2(const uint8_t *data, size_t size, uint8_t *out, size_t outSize, std::string *error = nullptr);

	/* @brief CRC-32 (IEEE 802.3, the one zip and 7-zip use). */
	uint32_t Crc32(const uint8_t *data, size_t size, uint32_t crc = 0);
}

// -------------------------------------------------------
