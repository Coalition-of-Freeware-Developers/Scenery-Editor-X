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
 * xp_zip.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_zip.h"
#include <cstring>
#include <zlib.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr uint32_t SIGNATURE_LOCAL_HEADER = 0x04034b50;
		constexpr uint32_t SIGNATURE_CENTRAL_DIRECTORY = 0x02014b50;
		constexpr uint32_t SIGNATURE_END_OF_CENTRAL_DIRECTORY = 0x06054b50;
		constexpr uint16_t METHOD_STORED = 0;
		constexpr uint16_t METHOD_DEFLATE = 8;
		constexpr uint16_t FLAG_ENCRYPTED = 1;

		uint16_t Read16(const uint8_t *at) { return static_cast<uint16_t>(at[0] | (at[1] << 8)); }
		uint32_t Read32(const uint8_t *at) { return static_cast<uint32_t>(at[0]) | (static_cast<uint32_t>(at[1]) << 8) | (static_cast<uint32_t>(at[2]) << 16) | (static_cast<uint32_t>(at[3]) << 24); }

		bool Fail(std::string *error, const char *message)
		{
			if (error)
				*error = message;
			return false;
		}

		bool Inflate(const uint8_t *compressed, const size_t compressedSize, std::vector<uint8_t> &out, const size_t expectedSize, std::string *error)
		{
			out.resize(expectedSize);
			if (expectedSize == 0)
				return true;

			z_stream stream{};
			if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)	// Raw deflate, no zlib header
				return Fail(error, "zlib initialization failed");

			stream.next_in = const_cast<Bytef *>(compressed);
			stream.avail_in = static_cast<uInt>(compressedSize);
			stream.next_out = out.data();
			stream.avail_out = static_cast<uInt>(out.size());
			const int result = inflate(&stream, Z_FINISH);
			const size_t produced = stream.total_out;
			inflateEnd(&stream);

			if (result != Z_STREAM_END || produced != expectedSize)
				return Fail(error, "Corrupt deflate stream in zip entry");
			return true;
		}
	}

	bool ReadZipArchive(const std::vector<uint8_t> &bytes, std::vector<ZipEntry> &out, std::string *error)
	{
		return ReadZipArchive(bytes.data(), bytes.size(), out, error);
	}

	bool ReadZipArchive(const uint8_t *bytes, const size_t size, std::vector<ZipEntry> &out, std::string *error)
	{
		out.clear();
		if (!bytes || size < 22)
			return Fail(error, "Not a zip archive (too small)");

		// The end-of-central-directory record is at the end, possibly followed by a comment (up to 64 KB).
		size_t eocd = size - 22;
		const size_t stop = size > 22 + 65535 ? size - 22 - 65535 : 0;
		while (true)
		{
			if (Read32(bytes + eocd) == SIGNATURE_END_OF_CENTRAL_DIRECTORY)
				break;
			if (eocd == stop)
				return Fail(error, "Not a zip archive (no end-of-central-directory record)");
			--eocd;
		}

		const uint16_t entryCount = Read16(bytes + eocd + 10);
		const uint32_t directorySize = Read32(bytes + eocd + 12);
		const uint32_t directoryOffset = Read32(bytes + eocd + 16);
		if (entryCount == 0xFFFF || directoryOffset == 0xFFFFFFFFu)
			return Fail(error, "ZIP64 archives aren't supported");
		if (static_cast<size_t>(directoryOffset) + directorySize > size)
			return Fail(error, "Corrupt zip archive (central directory out of range)");

		size_t position = directoryOffset;
		for (uint16_t i = 0; i < entryCount; ++i)
		{
			if (position + 46 > size || Read32(bytes + position) != SIGNATURE_CENTRAL_DIRECTORY)
				return Fail(error, "Corrupt zip archive (bad central directory entry)");

			const uint16_t flags = Read16(bytes + position + 8);
			const uint16_t method = Read16(bytes + position + 10);
			const uint32_t compressedSize = Read32(bytes + position + 20);
			const uint32_t uncompressedSize = Read32(bytes + position + 24);
			const uint16_t nameLength = Read16(bytes + position + 28);
			const uint16_t extraLength = Read16(bytes + position + 30);
			const uint16_t commentLength = Read16(bytes + position + 32);
			const uint32_t localOffset = Read32(bytes + position + 42);
			if (position + 46 + nameLength > size)
				return Fail(error, "Corrupt zip archive (entry name out of range)");

			std::string name(reinterpret_cast<const char *>(bytes + position + 46), nameLength);
			position += 46 + nameLength + extraLength + commentLength;

			for (char &c : name)
			{
				if (c == '\\')
					c = '/';
			}
			if (name.empty() || name.back() == '/')
				continue;	// Directory entry

			if (flags & FLAG_ENCRYPTED)
				return Fail(error, "Encrypted zip archives aren't supported");
			if (compressedSize == 0xFFFFFFFFu || uncompressedSize == 0xFFFFFFFFu)
				return Fail(error, "ZIP64 archives aren't supported");

			// The local header repeats the name / extra fields with its own lengths; the data follows them.
			if (static_cast<size_t>(localOffset) + 30 > size || Read32(bytes + localOffset) != SIGNATURE_LOCAL_HEADER)
				return Fail(error, "Corrupt zip archive (bad local header)");
			const uint16_t localNameLength = Read16(bytes + localOffset + 26);
			const uint16_t localExtraLength = Read16(bytes + localOffset + 28);
			const size_t dataOffset = static_cast<size_t>(localOffset) + 30 + localNameLength + localExtraLength;
			if (dataOffset + compressedSize > size)
				return Fail(error, "Corrupt zip archive (entry data out of range)");

			ZipEntry entry;
			entry.name = std::move(name);
			if (method == METHOD_STORED)
			{
				entry.data.assign(bytes + dataOffset, bytes + dataOffset + compressedSize);
			}
			else if (method == METHOD_DEFLATE)
			{
				if (!Inflate(bytes + dataOffset, compressedSize, entry.data, uncompressedSize, error))
					return false;
			}
			else
			{
				return Fail(error, "Unsupported zip compression method");
			}
			out.push_back(std::move(entry));
		}
		return true;
	}
}

// -------------------------------------------------------
