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
 * project_sex.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "scenery_project.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/**
	 * .sex: the project serialized to binary. Loads and saves several times faster than .edX and is what the edit
	 * cache (project_cache.h) keeps, but isn't meant for diffs: commit the .edX, keep .sex for speed and shipping.
	 *
	 * Layout (little-endian):
	 *   0   magic        89 'S' 'E' 'X' 0D 0A 1A 0A   (PNG-style: a text-mode copy or a git line-ending
	 *                                                   conversion changes these bytes, so mangling is detected)
	 *   8   uint16       format major (SEX_FORMAT_MAJOR, readers refuse a newer major)
	 *   10  uint16       format minor (additions older readers can skip)
	 *   12  uint32       flags (0)
	 *   16  uint32       chunk count
	 *   20  uint32       CRC-32 of the chunk table
	 *   24  uint64       total file size (truncation check)
	 *   32  chunk table  per chunk: char tag[4], uint32 version, uint64 offset, uint64 size, uint32 CRC-32, uint32 0
	 *   ... chunk payloads, each encoded with BinaryWriter (project_archive.h)
	 *
	 * Chunks: INFO (project header, datum, preferences, properties), LIBS (library references), APRT (one per
	 * airport, in order), ITEM (the scene hierarchy), LIBR (library projects' library.txt). Unknown tags are skipped,
	 * so newer editors can add chunks without breaking older ones.
	 */

	inline constexpr uint16_t SEX_FORMAT_MAJOR = 1;
	inline constexpr uint16_t SEX_FORMAT_MINOR = 0;
	inline constexpr size_t SEX_HEADER_SIZE = 32;
	inline constexpr size_t SEX_CHUNK_ENTRY_SIZE = 32;

	/* @brief A chunk of a .sex file. */
	struct SexChunkInfo
	{
		std::string tag;					// 4 characters
		uint32_t version = 1;
		uint64_t offset = 0;
		uint64_t size = 0;
		uint32_t crc = 0;
	};

	/* @brief Extra data stored with a project (the edit cache keeps its source stamp this way). */
	struct SexExtraChunk
	{
		std::string tag;					// 4 characters, upper case for the editor's own
		std::string bytes;
	};

	/**
	 * @brief Change counters per section of a project, from the document that edits it. A section whose counter
	 * matches the cached one is copied instead of encoded again. 0 = unknown: always encode.
	 */
	struct SexSectionRevisions
	{
		uint64_t info = 0;
		uint64_t libraries = 0;
		uint64_t items = 0;
		uint64_t library = 0;
		std::vector<uint64_t> airports;		// Parallel to SceneryProject::airports
	};

	/**
	 * @class SexEncodeCache
	 * @brief Encoded chunks from the last write, so quick saves and autosaves of a big project only encode what changed.
	 */
	class SexEncodeCache
	{
	public:
		void Clear() { m_Entries.clear(); m_Hits = m_Misses = 0; }

		[[nodiscard]] size_t GetHits() const { return m_Hits; }
		[[nodiscard]] size_t GetMisses() const { return m_Misses; }

		/* @brief Cached bytes for a section at a revision, or nullptr. */
		const std::string *Find(const std::string &key, uint64_t revision);
		void Store(const std::string &key, uint64_t revision, std::string bytes);

	private:
		struct Entry
		{
			uint64_t revision = 0;
			std::string bytes;
		};

		std::unordered_map<std::string, Entry> m_Entries;
		size_t m_Hits = 0;
		size_t m_Misses = 0;
	};

	struct SexWriteOptions
	{
		SexEncodeCache *cache = nullptr;
		const SexSectionRevisions *revisions = nullptr;
		std::vector<SexExtraChunk> extraChunks;
	};

	struct SexReadOptions
	{
		bool allowPartial = false;			// Load the intact chunks when some are damaged (recovery), with warnings
	};

	/* @brief Encodes a project as .sex bytes. */
	std::string WriteSex(const SceneryProject &project, const SexWriteOptions &options = {});

	/* @brief True when the bytes start with the .sex magic. */
	bool IsSexData(const void *data, size_t size);

	/**
	 * @brief Validates the header and the chunk table (not the payloads).
	 * @return False for a file that isn't .sex, is from a newer major version, or is truncated.
	 */
	bool ReadSexChunkTable(const void *data, size_t size, std::vector<SexChunkInfo> &chunks, std::string *error = nullptr);

	/**
	 * @brief Decodes .sex bytes. Every chunk's CRC is checked before it's decoded.
	 * @return False (with the reason) when the file is damaged, unless options.allowPartial keeps what's intact.
	 */
	bool ParseSex(const void *data, size_t size, SceneryProject &out, std::vector<std::string> *warnings = nullptr, std::string *error = nullptr,
		const SexReadOptions &options = {});

	/* @brief Just the project header (INFO chunk): fast enough for a recent-projects list. */
	bool ReadSexInfo(const void *data, size_t size, ProjectInfo &info, std::string *error = nullptr);

	/* @brief The verified payload of the first chunk with a tag, or false when there is none (or it's damaged). */
	bool FindSexChunk(const void *data, size_t size, std::string_view tag, std::string &bytes);
}

// -------------------------------------------------------
