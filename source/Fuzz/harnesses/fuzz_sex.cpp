/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_sex.cpp
 * -------------------------------------------------------
 * The binary project format (.sex: project_sex). Every chunk carries a CRC32, which random bytes never match, so
 * each input runs twice: as it is (the damage and partial-recovery paths) and with every checksum recomputed (the
 * chunk decoders behind the checks). A project that loads must save again, load back and save the same bytes, in
 * .sex and as .edX.
 * -------------------------------------------------------
 */
#include "common/fuzz_common.h"
#include <SceneryEditorX/project/project_edx.h>
#include <SceneryEditorX/project/project_sex.h>
#include <SceneryEditorX/project/scenery_project.h>
#include <cstring>

using namespace SceneryEditorX::ProjectFile;

namespace
{
	uint32_t GetU32(const std::string &bytes, const size_t at)
	{
		uint32_t value = 0;
		std::memcpy(&value, bytes.data() + at, sizeof(value));	// The format is little-endian, like every target we build for
		return value;
	}

	uint64_t GetU64(const std::string &bytes, const size_t at)
	{
		uint64_t value = 0;
		std::memcpy(&value, bytes.data() + at, sizeof(value));
		return value;
	}

	void PutU32(std::string &bytes, const size_t at, const uint32_t value)
	{
		std::memcpy(bytes.data() + at, &value, sizeof(value));
	}

	/* @brief Recomputes the chunk CRCs (entry + 24) and the table CRC (header + 20) where the layout allows. */
	void FixChecksums(std::string &bytes)
	{
		if (bytes.size() < SEX_HEADER_SIZE)
			return;
		const uint64_t count = GetU32(bytes, 16);
		if (SEX_HEADER_SIZE + count * SEX_CHUNK_ENTRY_SIZE > bytes.size())
			return;
		for (uint64_t i = 0; i < count; ++i)
		{
			const size_t entry = SEX_HEADER_SIZE + static_cast<size_t>(i) * SEX_CHUNK_ENTRY_SIZE;
			const uint64_t offset = GetU64(bytes, entry + 8);
			const uint64_t size = GetU64(bytes, entry + 16);
			if (offset <= bytes.size() && size <= bytes.size() - offset)
				PutU32(bytes, entry + 24, Crc32(bytes.data() + offset, static_cast<size_t>(size)));
		}
		PutU32(bytes, 20, Crc32(bytes.data() + SEX_HEADER_SIZE, static_cast<size_t>(count * SEX_CHUNK_ENTRY_SIZE)));
	}

	void Load(const std::string &bytes)
	{
		std::vector<SexChunkInfo> chunks;
		(void)ReadSexChunkTable(bytes.data(), bytes.size(), chunks);
		ProjectInfo info;
		(void)ReadSexInfo(bytes.data(), bytes.size(), info);
		std::string chunk;
		(void)FindSexChunk(bytes.data(), bytes.size(), "INFO", chunk);

		SexReadOptions recovery;
		recovery.allowPartial = true;
		SceneryProject partial;
		std::vector<std::string> warnings;
		std::string error;
		(void)ParseSex(bytes.data(), bytes.size(), partial, &warnings, &error, recovery);

		SceneryProject project;
		if (!ParseSex(bytes.data(), bytes.size(), project, &warnings, &error))
			return;

		const std::string written = WriteSex(project);
		SceneryProject reread;
		SEDX_FUZZ_CHECK(ParseSex(written.data(), written.size(), reread, nullptr, &error), "written .sex doesn't load: " + error);
		SEDX_FUZZ_CHECK(WriteSex(reread) == written, ".sex bytes change on a second save");

		const std::string edx = WriteEdx(project);
		SceneryProject fromEdx;
		SEDX_FUZZ_CHECK(ParseEdx(edx, fromEdx, nullptr, &error), "a project loaded from .sex doesn't save as loadable .edX: " + error);
	}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	std::string bytes(reinterpret_cast<const char *>(data), size);
	Load(bytes);
	FixChecksums(bytes);
	Load(bytes);
	return 0;
}
