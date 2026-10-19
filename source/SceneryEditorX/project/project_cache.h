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
 * project_cache.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "project_sex.h"
#include "scenery_project.h"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/* @brief What a project file looked like when the cache recorded it. */
	struct SourceStamp
	{
		std::string path;					// Absolute, normalized (lower-case on Windows)
		uint64_t size = 0;
		int64_t modified = 0;				// Last write time, filesystem clock ticks
		uint64_t contentHash = 0;			// FNV-1a of the bytes (0 when not computed)

		[[nodiscard]] bool SameFile(const SourceStamp &other) const { return path == other.path && size == other.size && modified == other.modified; }
	};

	/* @brief Stamps a file (false when it doesn't exist). */
	bool GetSourceStamp(const std::filesystem::path &path, SourceStamp &stamp, bool hashContent = false);

	/* @brief What a recovery file holds, for the "restore unsaved changes?" question. */
	struct RecoveryInfo
	{
		std::filesystem::path file;
		std::string projectName;
		std::string written;				// ISO 8601 UTC
		SourceStamp base;					// The project file the edits started from
		bool sourceChangedSince = false;	// The project file was changed (or removed) after the recovery was written
	};

	/**
	 * @class ProjectCache
	 * @brief The editor's per-project cache, kept outside the project folder (so a git working tree stays clean):
	 *
	 *  - snapshot: the project as last opened or saved, in .sex form. While the project file is unchanged, opening it
	 *    reads the snapshot instead of parsing the .edX / earth.wed.xml again
	 *  - recovery: unsaved edits, written by autosave without touching the project file; offered on the next open
	 *    after a crash and removed by a successful save or an explicit discard
	 *  - backups: the previous version of the project file before each save (the newest few are kept)
	 *
	 * Every write is atomic. Damaged cache entries are ignored (the project file is the truth).
	 */
	class ProjectCache
	{
	public:
		explicit ProjectCache(std::filesystem::path root = {}) : m_Root(std::move(root)) {}

		[[nodiscard]] const std::filesystem::path &GetRoot() const { return m_Root; }
		[[nodiscard]] bool IsEnabled() const { return !m_Root.empty(); }

		/* @brief The cache folder of one project file. */
		[[nodiscard]] std::filesystem::path GetEntryFolder(const std::filesystem::path &source) const;

		bool StoreSnapshot(const std::filesystem::path &source, const SceneryProject &project, std::string *error = nullptr, const SexWriteOptions &options = {}) const;

		/* @brief Loads the snapshot when the project file is unchanged since it was taken. False on a miss or a stale entry. */
		bool LoadSnapshot(const std::filesystem::path &source, SceneryProject &out, std::string *error = nullptr) const;

		bool WriteRecovery(const std::filesystem::path &source, const SceneryProject &project, std::string *error = nullptr, const SexWriteOptions &options = {}) const;
		[[nodiscard]] std::optional<RecoveryInfo> GetRecovery(const std::filesystem::path &source) const;
		bool LoadRecovery(const std::filesystem::path &source, SceneryProject &out, std::vector<std::string> *warnings = nullptr, std::string *error = nullptr) const;
		void ClearRecovery(const std::filesystem::path &source) const;

		/* @brief Copies the project file into the backups (keeping the newest `keep`). False when there is no file to back up. */
		bool StoreBackup(const std::filesystem::path &source, size_t keep = 5, std::string *error = nullptr) const;

		/* @brief Backups of a project file, newest first. */
		[[nodiscard]] std::vector<std::filesystem::path> ListBackups(const std::filesystem::path &source) const;

		/* @brief Forgets everything cached for a project file. */
		void Remove(const std::filesystem::path &source) const;

		/* @brief Recovery files of every project (newest first), for a startup "unsaved work" list. */
		[[nodiscard]] std::vector<RecoveryInfo> ListRecoveries() const;

	private:
		std::filesystem::path m_Root;
	};
}

// -------------------------------------------------------
