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
 * project_io.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "project_sex.h"
#include "project_wed.h"
#include "scenery_project.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/* @brief The file formats a project reads from and writes to. */
	enum class ProjectFormat : uint8_t
	{
		Unknown,
		Edx,		// .edX JSON
		Sex,		// .sex binary
		Wed			// WorldEditor earth.wed.xml
	};

	const char *GetProjectFormatName(ProjectFormat format);

	/* @brief The format a path's name implies: .edX / .edx, .sex, earth.wed.xml or *.wed.xml (Unknown otherwise). */
	ProjectFormat GetFormatForPath(const std::filesystem::path &path);

	/* @brief The format some bytes look like (the .sex magic, a JSON object, an XML document). */
	ProjectFormat SniffFormat(std::string_view bytes);

	/* @brief Reads a whole file. */
	bool ReadFileBytes(const std::filesystem::path &path, std::string &bytes, std::string *error = nullptr);

	/**
	 * @brief Replaces a file safely: the bytes go to a temporary file beside it, which is checked and then renamed over
	 * the target, so a crash, a full disk or a failed write never leaves a half-written project (the old file stays).
	 * Folders are created as needed.
	 */
	bool WriteFileAtomic(const std::filesystem::path &path, std::string_view bytes, std::string *error = nullptr);

	struct LoadOptions
	{
		WedImportOptions wed;
		SexReadOptions sex;
	};

	/**
	 * @brief Loads a project from any supported file. The format comes from the name, or the content when the name
	 * doesn't say. WorldEditor projects are named after their pack folder.
	 * @param format Receives the format that was read (optional).
	 */
	bool LoadProjectFile(const std::filesystem::path &path, SceneryProject &out, ProjectFormat *format = nullptr,
		std::vector<std::string> *warnings = nullptr, std::string *error = nullptr, const LoadOptions &options = {});

	/* @brief Parses bytes in a known format. */
	bool ParseProject(std::string_view bytes, ProjectFormat format, SceneryProject &out, std::vector<std::string> *warnings = nullptr,
		std::string *error = nullptr, const LoadOptions &options = {});

	struct SaveOptions
	{
		WedExportOptions wed;
		SexWriteOptions sex;
	};

	/* @brief A project as the bytes of a format. */
	std::string SerializeProject(const SceneryProject &project, ProjectFormat format, std::vector<std::string> *warnings = nullptr, const SaveOptions &options = {});

	/**
	 * @brief Writes a project (atomically) in a format; Unknown takes it from the path.
	 * @return False when the format is unknown or the write failed (the previous file is untouched then).
	 */
	bool SaveProjectFile(const std::filesystem::path &path, const SceneryProject &project, ProjectFormat format = ProjectFormat::Unknown,
		std::vector<std::string> *warnings = nullptr, std::string *error = nullptr, const SaveOptions &options = {});

	/**
	 * @brief Files that keep a project's folder git friendly: .gitattributes (".edX" text with LF endings, ".sex"
	 * binary so git never converts it) and .gitignore (the editor's temporary files). Existing files are left alone.
	 * @return The files that were created.
	 */
	std::vector<std::filesystem::path> WriteGitFiles(const std::filesystem::path &projectFolder);

	// -------------------------------------------------------
	// X-Plane files of a project

	/* @brief The project's airports as an apt.dat (airports hidden in the hierarchy are left out). */
	XPlane::AptFile BuildAptFile(const SceneryProject &project);

	/**
	 * @brief Adds (or replaces, by airport id) the airports of an apt.dat to a project; new ones are placed at the
	 * top level of the hierarchy.
	 * @return Number of airports added or replaced.
	 */
	size_t AddAptAirports(SceneryProject &project, const XPlane::AptFile &file);

	/* @brief Formats a library.txt (A / 800 / LIBRARY, regions, then exports with their state switches). */
	std::string WriteLibraryText(const XPlane::LibraryFile &library);
}

// -------------------------------------------------------
