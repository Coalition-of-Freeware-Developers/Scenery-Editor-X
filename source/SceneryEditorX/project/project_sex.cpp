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
 * project_sex.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "project_sex.h"
#include "project_archive.h"

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace
	{
		constexpr uint8_t MAGIC[8] = { 0x89, 'S', 'E', 'X', 0x0D, 0x0A, 0x1A, 0x0A };

		constexpr const char *TAG_INFO = "INFO";
		constexpr const char *TAG_LIBRARIES = "LIBS";
		constexpr const char *TAG_AIRPORT = "APRT";
		constexpr const char *TAG_ITEMS = "ITEM";
		constexpr const char *TAG_LIBRARY = "LIBR";

		constexpr uint32_t CHUNK_VERSION = 1;

		/* @brief The INFO chunk's payload. */
		struct ProjectHeader
		{
			ProjectInfo info;
			GeoReference datum;
			PropertyList wedPreferences;
			PropertyList properties;
		};

		template<typename A> void Visit(A &a, ProjectHeader &v)
		{
			a.Field("info", v.info);
			a.Field("datum", v.datum);
			a.Properties("wed-preferences", v.wedPreferences);
			a.Properties("properties", v.properties);
		}

		template<typename T>
		std::string Encode(const T &value)
		{
			std::string bytes;
			BinaryWriter writer(bytes);
			writer.Write(value);
			return bytes;
		}

		template<typename T>
		bool Decode(const std::string_view bytes, T &value, std::string &error)
		{
			BinaryReader reader(bytes.data(), bytes.size());
			reader.Read(value);
			if (!reader.Ok())
			{
				error = reader.GetError();
				return false;
			}
			return true;
		}

		template<typename T>
		void Put(std::string &out, const T value)
		{
			BinaryWriter writer(out);
			writer.WriteScalar(value);
		}

		template<typename T>
		T Get(const uint8_t *data)
		{
			T value{};
			BinaryReader reader(data, sizeof(T));
			reader.ReadScalar(value);
			return value;
		}

		/* @brief A section's bytes: from the cache when its revision is unchanged, encoded (and cached) otherwise. */
		template<typename T>
		std::string EncodeCached(const T &value, const SexWriteOptions &options, const std::string &key, const uint64_t revision)
		{
			if (!options.cache || revision == 0)
				return Encode(value);
			if (const std::string *cached = options.cache->Find(key, revision))
				return *cached;
			std::string bytes = Encode(value);
			options.cache->Store(key, revision, bytes);
			return bytes;
		}

		struct PendingChunk
		{
			std::string tag;
			std::string bytes;
		};
	}

	// -------------------------------------------------------

	const std::string *SexEncodeCache::Find(const std::string &key, const uint64_t revision)
	{
		const auto it = m_Entries.find(key);
		if (it == m_Entries.end() || it->second.revision != revision)
		{
			++m_Misses;
			return nullptr;
		}
		++m_Hits;
		return &it->second.bytes;
	}

	void SexEncodeCache::Store(const std::string &key, const uint64_t revision, std::string bytes)
	{
		m_Entries[key] = Entry{ revision, std::move(bytes) };
	}

	// -------------------------------------------------------

	std::string WriteSex(const SceneryProject &project, const SexWriteOptions &options)
	{
		const SexSectionRevisions none;
		const SexSectionRevisions &revisions = options.revisions ? *options.revisions : none;

		std::vector<PendingChunk> chunks;
		{
			ProjectHeader header{ project.info, project.datum, project.wedPreferences, project.properties };
			header.info.formatVersion = FORMAT_VERSION;
			chunks.push_back({ TAG_INFO, EncodeCached(header, options, TAG_INFO, revisions.info) });
		}
		chunks.push_back({ TAG_LIBRARIES, EncodeCached(project.libraries, options, TAG_LIBRARIES, revisions.libraries) });
		for (size_t i = 0; i < project.airports.size(); ++i)
		{
			const uint64_t revision = i < revisions.airports.size() ? revisions.airports[i] : 0;
			// Keyed by the airport id, so reordering airports still hits the cache.
			chunks.push_back({ TAG_AIRPORT, EncodeCached(project.airports[i], options, std::string(TAG_AIRPORT) + ":" + project.airports[i].airport.id, revision) });
		}
		chunks.push_back({ TAG_ITEMS, EncodeCached(project.items, options, TAG_ITEMS, revisions.items) });
		if (project.library)
			chunks.push_back({ TAG_LIBRARY, EncodeCached(*project.library, options, TAG_LIBRARY, revisions.library) });
		for (const SexExtraChunk &extra : options.extraChunks)
			chunks.push_back({ extra.tag, extra.bytes });

		// Header, then the table, then the payloads in table order
		std::string table;
		uint64_t offset = SEX_HEADER_SIZE + chunks.size() * SEX_CHUNK_ENTRY_SIZE;
		uint64_t total = offset;
		for (const PendingChunk &chunk : chunks)
			total += chunk.bytes.size();

		for (const PendingChunk &chunk : chunks)
		{
			char tag[4] = { ' ', ' ', ' ', ' ' };
			for (size_t i = 0; i < 4 && i < chunk.tag.size(); ++i)
				tag[i] = chunk.tag[i];
			table.append(tag, 4);
			Put<uint32_t>(table, CHUNK_VERSION);
			Put<uint64_t>(table, offset);
			Put<uint64_t>(table, chunk.bytes.size());
			Put<uint32_t>(table, Crc32(chunk.bytes.data(), chunk.bytes.size()));
			Put<uint32_t>(table, 0);
			offset += chunk.bytes.size();
		}

		std::string out;
		out.reserve(static_cast<size_t>(total));
		out.append(reinterpret_cast<const char *>(MAGIC), sizeof(MAGIC));
		Put<uint16_t>(out, SEX_FORMAT_MAJOR);
		Put<uint16_t>(out, SEX_FORMAT_MINOR);
		Put<uint32_t>(out, 0);
		Put<uint32_t>(out, static_cast<uint32_t>(chunks.size()));
		Put<uint32_t>(out, Crc32(table.data(), table.size()));
		Put<uint64_t>(out, total);
		out.append(table);
		for (const PendingChunk &chunk : chunks)
			out.append(chunk.bytes);
		return out;
	}

	bool IsSexData(const void *data, const size_t size)
	{
		return data && size >= sizeof(MAGIC) && std::memcmp(data, MAGIC, sizeof(MAGIC)) == 0;
	}

	bool ReadSexChunkTable(const void *data, const size_t size, std::vector<SexChunkInfo> &chunks, std::string *error)
	{
		chunks.clear();
		const auto fail = [error](const std::string &message)
		{
			if (error)
				*error = message;
			return false;
		};

		if (!IsSexData(data, size))
		{
			// A git checkout with text conversion turns the magic's CR LF into LF: say so, it's the usual cause.
			const auto *bytes = static_cast<const uint8_t *>(data);
			if (data && size >= 7 && bytes[0] == 0x89 && bytes[1] == 'S' && bytes[2] == 'E' && bytes[3] == 'X' && bytes[4] == 0x0A)
				return fail("The .sex file was converted as text (line endings changed, for example by git without '*.sex binary' in .gitattributes)");
			return fail("Not a Scenery Editor X binary project (.sex)");
		}
		if (size < SEX_HEADER_SIZE)
			return fail("The .sex file is truncated (no header)");

		const auto *bytes = static_cast<const uint8_t *>(data);
		const uint16_t major = Get<uint16_t>(bytes + 8);
		const uint32_t count = Get<uint32_t>(bytes + 16);
		const uint32_t tableCrc = Get<uint32_t>(bytes + 20);
		const uint64_t total = Get<uint64_t>(bytes + 24);

		if (major > SEX_FORMAT_MAJOR)
			return fail("The .sex file was written by a newer Scenery Editor X (format " + std::to_string(major) + ")");
		if (total > size)
			return fail("The .sex file is truncated (" + std::to_string(size) + " of " + std::to_string(total) + " bytes)");

		const uint64_t tableSize = static_cast<uint64_t>(count) * SEX_CHUNK_ENTRY_SIZE;
		if (SEX_HEADER_SIZE + tableSize > size)
			return fail("The .sex chunk table is truncated");
		if (Crc32(bytes + SEX_HEADER_SIZE, static_cast<size_t>(tableSize)) != tableCrc)
			return fail("The .sex chunk table is damaged (checksum mismatch)");

		chunks.reserve(count);
		for (uint32_t i = 0; i < count; ++i)
		{
			const uint8_t *entry = bytes + SEX_HEADER_SIZE + static_cast<size_t>(i) * SEX_CHUNK_ENTRY_SIZE;
			SexChunkInfo chunk;
			chunk.tag.assign(reinterpret_cast<const char *>(entry), 4);
			chunk.version = Get<uint32_t>(entry + 4);
			chunk.offset = Get<uint64_t>(entry + 8);
			chunk.size = Get<uint64_t>(entry + 16);
			chunk.crc = Get<uint32_t>(entry + 24);
			if (chunk.offset > size || chunk.size > size - chunk.offset)
				return fail("The .sex chunk " + chunk.tag + " points past the end of the file");
			chunks.push_back(std::move(chunk));
		}
		return true;
	}

	bool ParseSex(const void *data, const size_t size, SceneryProject &out, std::vector<std::string> *warnings, std::string *error, const SexReadOptions &options)
	{
		out = SceneryProject{};
		std::vector<SexChunkInfo> chunks;
		if (!ReadSexChunkTable(data, size, chunks, error))
			return false;

		const auto *bytes = static_cast<const uint8_t *>(data);
		bool sawInfo = false;
		size_t airportIndex = 0;
		for (const SexChunkInfo &chunk : chunks)
		{
			const std::string_view payload(reinterpret_cast<const char *>(bytes + chunk.offset), static_cast<size_t>(chunk.size));
			const std::string label = chunk.tag == TAG_AIRPORT ? chunk.tag + " " + std::to_string(airportIndex++) : chunk.tag;

			const auto damaged = [&](const std::string &reason)
			{
				const std::string message = "Chunk " + label + " is damaged: " + reason;
				if (!options.allowPartial)
				{
					if (error)
						*error = message;
					return false;
				}
				if (warnings)
					warnings->push_back(message + " (skipped)");
				return true;
			};

			if (Crc32(payload.data(), payload.size()) != chunk.crc)
			{
				if (!damaged("checksum mismatch"))
					return false;
				continue;
			}

			std::string decodeError;
			bool decoded = true;
			if (chunk.tag == TAG_INFO)
			{
				ProjectHeader header;
				decoded = Decode(payload, header, decodeError);
				if (decoded)
				{
					out.info = std::move(header.info);
					out.datum = header.datum;
					out.wedPreferences = std::move(header.wedPreferences);
					out.properties = std::move(header.properties);
					sawInfo = true;
				}
			}
			else if (chunk.tag == TAG_LIBRARIES)
				decoded = Decode(payload, out.libraries, decodeError);
			else if (chunk.tag == TAG_AIRPORT)
			{
				ProjectAirport airport;
				decoded = Decode(payload, airport, decodeError);
				if (decoded)
					out.airports.push_back(std::move(airport));
			}
			else if (chunk.tag == TAG_ITEMS)
				decoded = Decode(payload, out.items, decodeError);
			else if (chunk.tag == TAG_LIBRARY)
			{
				out.library.emplace();
				decoded = Decode(payload, *out.library, decodeError);
				if (!decoded)
					out.library.reset();
			}
			// Other tags: extra data (cache stamps) or chunks from a newer editor

			if (!decoded && !damaged(decodeError))
				return false;
		}

		if (!sawInfo)
		{
			if (!options.allowPartial)
			{
				if (error)
					*error = "The .sex file has no project header (INFO chunk)";
				return false;
			}
			if (warnings)
				warnings->push_back("The project header was lost; defaults were used");
		}

		if (out.info.formatVersion > FORMAT_VERSION && warnings)
			warnings->push_back("Written by a newer editor (format " + std::to_string(out.info.formatVersion) + "); data it added may not be kept");
		out.info.formatVersion = FORMAT_VERSION;

		const std::vector<std::string> changes = NormalizeProject(out);
		if (warnings)
			warnings->insert(warnings->end(), changes.begin(), changes.end());
		return true;
	}

	bool ReadSexInfo(const void *data, const size_t size, ProjectInfo &info, std::string *error)
	{
		std::string payload;
		if (!FindSexChunk(data, size, TAG_INFO, payload))
		{
			if (error && error->empty())
				*error = "No readable project header";
			return false;
		}
		ProjectHeader header;
		std::string decodeError;
		if (!Decode(payload, header, decodeError))
		{
			if (error)
				*error = decodeError;
			return false;
		}
		info = std::move(header.info);
		return true;
	}

	bool FindSexChunk(const void *data, const size_t size, const std::string_view tag, std::string &bytes)
	{
		std::vector<SexChunkInfo> chunks;
		if (!ReadSexChunkTable(data, size, chunks, nullptr))
			return false;
		const auto *base = static_cast<const uint8_t *>(data);
		for (const SexChunkInfo &chunk : chunks)
		{
			if (chunk.tag != tag)
				continue;
			const auto *payload = reinterpret_cast<const char *>(base + chunk.offset);
			if (Crc32(payload, static_cast<size_t>(chunk.size)) != chunk.crc)
				return false;
			bytes.assign(payload, static_cast<size_t>(chunk.size));
			return true;
		}
		return false;
	}
}

// -------------------------------------------------------
