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
 * config_file.h
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace libconfig
{
	class Config;
}

namespace SceneryEditorX
{
	namespace VFS
	{
		class FileSystem;
	}

	/**
	 * @class ConfigFile
	 * @brief One libconfig (.cfg) file of the editor's, read and written through the VFS so it behaves the same on
	 * Windows, macOS and Linux:
	 *
	 *  - Its location is a virtual path ("user:/settings.cfg": the roaming settings folder each platform has) or a
	 *    physical one, as UTF-8 text.
	 *  - Reads find the file whatever case the disk spells it in. Writes are atomic (a crash or a full disk keeps the
	 *    previous file) and create the folders on the way; read-only mounts refuse them.
	 *  - A file that doesn't parse is reported (line and reason) and left alone: Load() gives an empty document, and
	 *    the caller decides whether to keep a copy (Backup) before saving over it.
	 *
	 * Values are addressed with dotted paths ("ui.font_size"); setting one creates the groups on the way and replaces a
	 * value of another type. Every instance has its own document, and every method is thread safe.
	 *
	 * Standard library + libconfig only (no engine headers), so it is unit tested on every platform.
	 */
	class ConfigFile
	{
	public:
		enum class LoadResult : uint8_t
		{
			Loaded,		// Read and parsed
			Missing,	// There's no file yet: the document is empty
			Invalid,	// The file doesn't parse: the document is empty, the file stays as it is
			Failed		// The file couldn't be read (or the location's mount isn't there)
		};

		/* @brief An empty document with no location yet (SetLocation names it). */
		ConfigFile();

		/**
		 * @brief An empty document kept at @p location. Nothing is read until Load().
		 * @param vfs The file system virtual locations resolve in (the editor's shared one when null).
		 */
		explicit ConfigFile(std::string location, VFS::FileSystem *vfs = nullptr);

		virtual ~ConfigFile();

		ConfigFile(const ConfigFile &) = delete;
		ConfigFile &operator=(const ConfigFile &) = delete;

		// -------------------------------------------------------
		// The file

		void SetLocation(std::string location, VFS::FileSystem *vfs = nullptr);
		[[nodiscard]] std::string GetLocation() const;

		/* @brief True when the location is "mount:/path" rather than a physical path. */
		[[nodiscard]] bool IsVirtual() const;

		/* @brief Where the file is (or would be) on disk; empty when a virtual location's mount isn't there. */
		[[nodiscard]] std::filesystem::path GetPhysicalPath() const;

		[[nodiscard]] bool FileExists() const;

		/* @brief Replaces the document with the file's contents. @p error gets the reason when it isn't Loaded or Missing. */
		LoadResult Load(std::string *error = nullptr);

		/* @brief Writes the document to the file atomically. */
		bool Save(std::string *error = nullptr);

		/* @brief Copies the file as it is on disk to its location plus @p suffix ("settings.cfg" -> "settings.cfg.invalid"), replacing an older copy. */
		bool Backup(std::string_view suffix, std::string *error = nullptr) const;

		/* @brief Replaces the document with libconfig text (nothing changes when it doesn't parse). */
		bool Parse(std::string_view text, std::string *error = nullptr);

		/* @brief The document as libconfig text, as Save() writes it. */
		[[nodiscard]] std::string Serialize() const;

		/* @brief Empties the document (the file is untouched until Save). */
		void Clear();

		/* @brief True when the document changed since it was last loaded or saved. */
		[[nodiscard]] bool IsDirty() const;

		// -------------------------------------------------------
		// Values (dotted paths)

		/* @brief True when a value (not a group) is at the path. */
		[[nodiscard]] bool HasOption(std::string_view path) const;

		/* @brief True when a group is at the path. */
		[[nodiscard]] bool HasGroup(std::string_view path) const;

		/* @brief Removes the value or the group (with everything in it) at the path. False when there was none. */
		bool RemoveOption(std::string_view path);

		/* @brief Stores a value, creating the groups on the way. False when the path can't hold one (an invalid name, a value where a group should be). */
		bool AddBoolOption(std::string_view path, bool value);
		bool AddIntOption(std::string_view path, int value);
		bool AddInt64Option(std::string_view path, int64_t value);
		bool AddFloatOption(std::string_view path, double value);
		bool AddStringOption(std::string_view path, std::string_view value);

		/* @brief A value, or @p defaultValue when there's none (numbers convert between int and float). */
		[[nodiscard]] bool GetBoolOption(std::string_view path, bool defaultValue = false) const;
		[[nodiscard]] int GetIntOption(std::string_view path, int defaultValue = 0) const;
		[[nodiscard]] int64_t GetInt64Option(std::string_view path, int64_t defaultValue = 0) const;
		[[nodiscard]] double GetFloatOption(std::string_view path, double defaultValue = 0.0) const;
		[[nodiscard]] std::string GetStringOption(std::string_view path, std::string_view defaultValue = {}) const;

		/* @brief Stores text (AddStringOption). */
		bool SetOption(std::string_view key, std::string_view value);

		/* @brief Any value as text ("true", "12", "0.5" ...); @p value is left alone when there's none. */
		bool GetOption(std::string_view key, std::string &value) const;

		/* @brief The names in a group, in file order (empty for no group). */
		[[nodiscard]] std::vector<std::string> GetChildNames(std::string_view groupPath = {}) const;

		/* @brief Every value's dotted path, depth first in file order. */
		[[nodiscard]] std::vector<std::string> GetOptionPaths() const;

	private:
		enum class ValueKind : uint8_t;

		template <typename T>
		bool Store(std::string_view path, ValueKind kind, const T &value);

		bool ReadBytes(std::string &bytes, std::string *error) const;
		bool WriteBytes(std::string_view bytes, std::string *error) const;

		mutable std::mutex m_Mutex;
		std::unique_ptr<libconfig::Config> m_Config;
		std::string m_Location;
		VFS::FileSystem *m_Vfs = nullptr;	// Null: the editor's shared one
		bool m_Dirty = false;
	};

}

// -------------------------------------------------------
