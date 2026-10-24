/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_common.h
 * -------------------------------------------------------
 * What every harness shares: the libFuzzer entry points and SEDX_FUZZ_CHECK, which turns a broken invariant (a file we
 * wrote that we can't read back, an index past the end of the vertices) into a crash the engine records like any other.
 * -------------------------------------------------------
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

// The entry points every engine (libFuzzer, AFL++, the standalone replay driver) calls
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv);

namespace SceneryEditorX::Fuzz
{
	[[noreturn]] inline void Fail(const char *condition, const std::string &message, const char *file, const int line)
	{
		std::fprintf(stderr, "\n==== SEDX fuzz invariant failed: %s\n     %s\n     at %s:%d\n", condition, message.c_str(), file, line);
		std::fflush(stderr);
		std::abort();
	}

	/* @brief The input as text (the X-Plane text formats are bytes, any encoding). */
	inline std::string_view AsText(const uint8_t *data, const size_t size)
	{
		return { reinterpret_cast<const char *>(data), size };
	}
}

/* @brief Aborts with the message when the condition is false, in every build type (assert() vanishes in release builds). */
#define SEDX_FUZZ_CHECK(condition, message) \
	do \
	{ \
		if (!(condition)) \
			::SceneryEditorX::Fuzz::Fail(#condition, (message), __FILE__, __LINE__); \
	} while (false)
