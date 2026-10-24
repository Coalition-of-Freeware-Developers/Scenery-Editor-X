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

		/* @brief Raw deflate (no zlib header) of some bytes; false when zlib fails. */
		bool Deflate(const std::string &data, std::vector<uint8_t> &out)
		{
			z_stream stream{};
			if (deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
				return false;
			out.resize(deflateBound(&stream, static_cast<uLong>(data.size())));
			stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
			stream.avail_in = static_cast<uInt>(data.size());
			stream.next_out = out.data();
			stream.avail_out = static_cast<uInt>(out.size());
			const int result = deflate(&stream, Z_FINISH);
			out.resize(stream.total_out);
			deflateEnd(&stream);
			return result == Z_STREAM_END;
		}

		void Put16(std::vector<uint8_t> &out, const uint32_t value)
		{
			out.push_back(static_cast<uint8_t>(value & 0xFF));
			out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
		}

		void Put32(std::vector<uint8_t> &out, const uint32_t value)
		{
			for (int shift = 0; shift < 32; shift += 8)
				out.push_back(static_cast<uint8_t>((value >> shift) & 0xFF));
		}
	}

	std::vector<uint8_t> WriteZipArchive(const std::vector<ZipFile> &files, const ZipMethod method)
	{
		constexpr uint32_t DOS_TIME = 0;
		constexpr uint32_t DOS_DATE = (0u << 9) | (1u << 5) | 1u;	// 1980-01-01
		constexpr uint32_t UTF8_NAMES = 1u << 11;

		std::vector<uint8_t> out;
		std::vector<uint8_t> directory;
		std::vector<uint8_t> compressed;
		for (const ZipFile &file : files)
		{
			const uint32_t crc = static_cast<uint32_t>(crc32(crc32(0L, Z_NULL, 0), reinterpret_cast<const Bytef *>(file.data.data()), static_cast<uInt>(file.data.size())));
			const uint32_t size = static_cast<uint32_t>(file.data.size());
			const bool deflated = method == ZipMethod::Deflated && !file.data.empty() && Deflate(file.data, compressed) && compressed.size() < file.data.size();
			const uint16_t entryMethod = deflated ? METHOD_DEFLATE : METHOD_STORED;
			const uint32_t storedSize = deflated ? static_cast<uint32_t>(compressed.size()) : size;
			const uint32_t offset = static_cast<uint32_t>(out.size());

			Put32(out, SIGNATURE_LOCAL_HEADER);
			Put16(out, 20);							// Version needed: 2.0
			Put16(out, UTF8_NAMES);
			Put16(out, entryMethod);
			Put16(out, DOS_TIME);
			Put16(out, DOS_DATE);
			Put32(out, crc);
			Put32(out, storedSize);
			Put32(out, size);
			Put16(out, static_cast<uint32_t>(file.name.size()));
			Put16(out, 0);
			out.insert(out.end(), file.name.begin(), file.name.end());
			if (deflated)
				out.insert(out.end(), compressed.begin(), compressed.end());
			else
				out.insert(out.end(), file.data.begin(), file.data.end());

			Put32(directory, SIGNATURE_CENTRAL_DIRECTORY);
			Put16(directory, (3u << 8) | 20u);		// Made by: Unix, 2.0
			Put16(directory, 20);
			Put16(directory, UTF8_NAMES);
			Put16(directory, entryMethod);
			Put16(directory, DOS_TIME);
			Put16(directory, DOS_DATE);
			Put32(directory, crc);
			Put32(directory, storedSize);
			Put32(directory, size);
			Put16(directory, static_cast<uint32_t>(file.name.size()));
			Put16(directory, 0);					// Extra
			Put16(directory, 0);					// Comment
			Put16(directory, 0);					// Disk
			Put16(directory, 0);					// Internal attributes
			Put32(directory, 0100644u << 16);		// External attributes: a regular file, rw-r--r--
			Put32(directory, offset);
			directory.insert(directory.end(), file.name.begin(), file.name.end());
		}

		const uint32_t directoryOffset = static_cast<uint32_t>(out.size());
		out.insert(out.end(), directory.begin(), directory.end());
		Put32(out, SIGNATURE_END_OF_CENTRAL_DIRECTORY);
		Put16(out, 0);
		Put16(out, 0);
		Put16(out, static_cast<uint32_t>(files.size()));
		Put16(out, static_cast<uint32_t>(files.size()));
		Put32(out, static_cast<uint32_t>(directory.size()));
		Put32(out, directoryOffset);
		Put16(out, 0);
		return out;
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
