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
 * vfs.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::VFS
{
	/**
	 * A virtual file system, so the editor reads and writes files the same way on Windows, macOS and Linux:
	 *
	 *  - Named mount points ("project", "xplane", "scenery", "user", ...) over physical folders, addressed with portable
	 *    virtual paths: "project:/Earth nav data/apt.dat". Virtual paths always use '/', can't climb out of their mount
	 *    ("..") and name mounts case-insensitively.
	 *  - Lookups fall back to a case-insensitive match per path component: packs made on Windows reference
	 *    "Objects/Hangar.obj" for a file stored as "objects/hangar.obj", which only Windows (and default macOS volumes)
	 *    forgive.
	 *  - Writes are atomic (a temporary file checked, then renamed over the target) and refused on read-only mounts
	 *    (X-Plane's own folders).
	 *  - Text is UTF-8 everywhere; ToUtf8 / FromUtf8 convert at the std::filesystem boundary.
	 *  - The per-OS folders (home, documents, config, data, cache) and X-Plane's install list come from the platform's
	 *    conventions (Known Folders, ~/Library, XDG).
	 *
	 * Standard library only, so it is unit tested on every platform. FileSystem is thread safe: the mount table is
	 * guarded, file operations go straight to the operating system.
	 */

	// -------------------------------------------------------
	// Mount names the editor uses

	inline constexpr const char *MOUNT_PROJECT = "project";		// The open project's folder (its scenery pack)
	inline constexpr const char *MOUNT_XPLANE = "xplane";		// The X-Plane 12 install (read-only)
	inline constexpr const char *MOUNT_SCENERY = "scenery";		// X-Plane's Custom Scenery folder (writable)
	inline constexpr const char *MOUNT_USER = "user";			// The editor's settings (AppLocation::Settings)
	inline constexpr const char *MOUNT_DATA = "data";			// Machine-local data worth keeping (AppLocation::Data)
	inline constexpr const char *MOUNT_CACHE = "cache";			// Rebuildable caches (AppLocation::Cache)
	inline constexpr const char *MOUNT_LOGS = "logs";			// Log files (AppLocation::Logs)
	inline constexpr const char *MOUNT_TEMP = "temp";			// Scratch files (AppLocation::Temp)

	// -------------------------------------------------------
	// Paths and names

	/* @brief A path as UTF-8 text (generic '/' separators are not forced: it is the path as the OS spells it). */
	std::string ToUtf8(const std::filesystem::path &path);

	/**
	 * @brief A path from UTF-8 text. On Windows, byte sequences that aren't UTF-8 (a file saved in another code page, a
	 * damaged one) become U+FFFD, so such a path names a file that isn't there instead of throwing; Linux and macOS
	 * keep the bytes, which can be a real file name there.
	 */
	std::filesystem::path FromUtf8(std::string_view text);

	/**
	 * @brief A relative virtual path cleaned up: '\' becomes '/', empty and "." components go, ".." removes the one
	 * before it. Leading and trailing slashes are dropped ("" is the mount's root).
	 * @param escaped Set when ".." climbs above the root (the result is then empty).
	 */
	std::string NormalizeRelative(std::string_view path, bool *escaped = nullptr);

	/**
	 * @brief Splits "mount:/relative/path" (or "mount:relative") into the mount name and the normalized relative path.
	 * False when there's no "name:" prefix, the name is empty or the path climbs out of the mount.
	 */
	bool SplitVirtualPath(std::string_view virtualPath, std::string &mount, std::string &relative);

	/* @brief "mount:/relative" from its parts (the relative path is normalized). */
	std::string MakeVirtualPath(std::string_view mount, std::string_view relative);

	/* @brief A virtual path with more components appended ("project:/Earth nav data" + "apt.dat"). */
	std::string Join(std::string_view virtualPath, std::string_view relative);

	/**
	 * @brief Why a file or folder name won't work on every platform: empty, "." / "..", characters Windows forbids
	 * (<>:"/\|?* and control characters), names Windows reserves (CON, PRN, AUX, NUL, COM1-9, LPT1-9, with or without
	 * an extension), a trailing dot or space, or longer than 255 bytes. Empty when the name is fine.
	 */
	std::string CheckPortableName(std::string_view name);

	/* @brief A name made portable: forbidden characters become '_', reserved names get a '_' suffix, trailing dots and spaces go; @p fallback when nothing is left. */
	std::string MakePortableName(std::string_view name, std::string_view fallback = "Untitled");

	/* @brief True when @p path is @p folder or inside it (compared lexically after making both absolute and normal). */
	bool IsInside(const std::filesystem::path &path, const std::filesystem::path &folder);

	/**
	 * @brief Where a relative name from an archive or a download ("Earth nav data/apt.dat") lands inside @p folder.
	 * False when it would land anywhere else: ".." above the folder, an absolute name, or a drive or server root (also
	 * one that only shows after normalizing: "a/../C:/x" is "C:/x", which replaces the folder on Windows).
	 */
	bool ResolveInside(const std::filesystem::path &folder, std::string_view name, std::filesystem::path &target);

	/**
	 * @brief A key two spellings of one path share: normal, '/' separators, no trailing slash, lower-case (ASCII) on
	 * the platforms whose file systems ignore case (Windows, macOS). For sets and maps of folders.
	 */
	std::string PathKey(const std::filesystem::path &path);

	// -------------------------------------------------------
	// Physical file operations (everything the mounts do goes through these)

	/**
	 * @brief The existing file or folder a path names, matching each component case-insensitively (ASCII) when the
	 * exact spelling isn't there. Nullopt when nothing matches.
	 */
	std::optional<std::filesystem::path> FindCaseInsensitive(const std::filesystem::path &path);

	/* @brief The path as the disk spells it when a case-insensitive match exists, else the path unchanged. */
	std::filesystem::path ResolveCase(const std::filesystem::path &path);

	/* @brief True when the path (matched case-insensitively) is a regular file. */
	bool IsFile(const std::filesystem::path &path);

	/* @brief True when the path (matched case-insensitively) is a folder. */
	bool IsDirectory(const std::filesystem::path &path);

	/* @brief Reads a whole file; a path that only differs in case from the file on disk is found too. */
	bool ReadFile(const std::filesystem::path &path, std::string &bytes, std::string *error = nullptr);

	/**
	 * @brief Replaces a file safely: the bytes go to a temporary file beside it, which is checked and then renamed over
	 * the target, so a crash, a full disk or a failed write never leaves a half-written file (the old one stays).
	 * Folders are created as needed.
	 */
	bool WriteFileAtomic(const std::filesystem::path &path, std::string_view bytes, std::string *error = nullptr);

	/* @brief Creates a folder and its parents (fine when it exists). */
	bool CreateFolders(const std::filesystem::path &path, std::string *error = nullptr);

	/* @brief Copies a file (folders created as needed). Without @p overwrite an existing target fails. (Not "CopyFile": windows.h makes that a macro.) */
	bool CopyRegularFile(const std::filesystem::path &from, const std::filesystem::path &to, bool overwrite, std::string *error = nullptr);

	// -------------------------------------------------------
	// Platform folders

	enum class Folder : uint8_t
	{
		Home,		// The user's home ("C:/Users/name", "/Users/name", "/home/name")
		Documents,	// Documents (Windows / macOS), XDG_DOCUMENTS_DIR or ~/Documents (Linux)
		Config,		// Per-user settings: %APPDATA%, ~/Library/Application Support, $XDG_CONFIG_HOME (~/.config)
		Data,		// Per-user data: %LOCALAPPDATA%, ~/Library/Application Support, $XDG_DATA_HOME (~/.local/share)
		Cache,		// Per-user cache: %LOCALAPPDATA%, ~/Library/Caches, $XDG_CACHE_HOME (~/.cache)
		Temp		// The temporary folder
	};

	/* @brief A per-user folder (empty when the platform doesn't say where it is). */
	std::filesystem::path GetFolder(Folder folder);

	/* @brief The editor's own subfolder of a per-user folder ("<Config>/SceneryEditorX"). Not created. */
	std::filesystem::path GetAppFolder(Folder folder, std::string_view appName = "SceneryEditorX");

	// -------------------------------------------------------
	// Where the editor keeps its own files

	class FileSystem;

	/**
	 * @brief Kinds of files the editor keeps for itself, each where the platform expects it:
	 *
	 *   Settings  preferences that may roam with the user   %APPDATA%\<app>               ~/Library/Application Support/<app>   $XDG_CONFIG_HOME/<app>
	 *   Data      machine-local data worth keeping          %LOCALAPPDATA%\<app>          ~/Library/Application Support/<app>   $XDG_DATA_HOME/<app>
	 *             (crash recovery, autosaves)
	 *   Cache     rebuildable: indexes, downloads,          %LOCALAPPDATA%\<app>\cache    ~/Library/Caches/<app>                $XDG_CACHE_HOME/<app>
	 *             compiled shaders (safe to delete)
	 *   Logs      log files                                 <Settings>\logs
	 *   Temp      scratch files                             <temp>\<app>
	 */
	enum class AppLocation : uint8_t
	{
		Settings,
		Data,
		Cache,
		Logs,
		Temp
	};

	struct AppLayout
	{
		std::filesystem::path settings;
		std::filesystem::path data;
		std::filesystem::path cache;
		std::filesystem::path logs;
		std::filesystem::path temp;

		[[nodiscard]] const std::filesystem::path &Get(AppLocation location) const;
	};

	/* @brief The platform's layout for an app's folders (nothing is created). Missing platform folders fall back to the working folder. */
	AppLayout MakeAppLayout(std::string_view appName = "SceneryEditorX");

	/* @brief The process-wide layout (MakeAppLayout() until SetAppLayout replaces it). Thread safe. */
	AppLayout GetAppLayout();
	void SetAppLayout(const AppLayout &layout);

	/* @brief A folder of the process-wide layout. */
	std::filesystem::path GetAppLocation(AppLocation location);

	/* @brief Creates a layout's folders and mounts them: user, data, cache, logs, temp. */
	bool MountAppLayout(FileSystem &vfs, const AppLayout &layout, std::string *error = nullptr);

	/* @brief A file or folder an older version kept somewhere else. */
	struct LegacyMove
	{
		std::filesystem::path from;
		std::filesystem::path to;
	};

	/**
	 * @brief Moves what older versions left in their old places: a file moves when there is none at the target; a
	 * folder's contents merge into the target the same way, and the old folder goes once it's empty. Nothing at a
	 * target is overwritten (what's in the way stays in the old place, and the report says so). Moves between
	 * volumes copy, then delete. Sources that don't exist, or are the target already, are skipped.
	 * @return One line per move that did something, for the log.
	 */
	std::vector<std::string> MoveLegacyFiles(const std::vector<LegacyMove> &moves);

	// -------------------------------------------------------
	// X-Plane

	/* @brief Where X-Plane lists its installs: x-plane_install_<version>.txt in %LOCALAPPDATA%, ~/Library/Preferences or ~/.x-plane. */
	std::filesystem::path GetXPlaneInstallList(int version = 12);

	/* @brief True when a folder looks like an X-Plane install (Resources, Custom Scenery or the executable). */
	bool IsXPlaneFolder(const std::filesystem::path &folder);

	/* @brief The installs X-Plane's install list names that exist on disk, in the list's order. */
	std::vector<std::filesystem::path> FindXPlaneInstalls(int version = 12);

	/* @brief The Custom Scenery folder of an X-Plane install (matched case-insensitively). */
	std::filesystem::path GetCustomSceneryFolder(const std::filesystem::path &xplaneRoot);

	// -------------------------------------------------------
	// Mounts

	/* @brief A file or folder a listing found. */
	struct Entry
	{
		std::string path;			// Virtual path ("project:/Earth nav data/apt.dat")
		std::string name;			// Its last component
		bool directory = false;
		uint64_t size = 0;			// Bytes (0 for folders)
	};

	class FileSystem
	{
	public:
		/**
		 * @brief Maps a name onto a folder (replacing a mount of that name). The folder needn't exist yet.
		 * @param readOnly Writes, copies into and removals from the mount are refused.
		 */
		bool Mount(std::string_view name, const std::filesystem::path &root, bool readOnly = false, std::string *error = nullptr);
		bool Unmount(std::string_view name);
		[[nodiscard]] bool IsMounted(std::string_view name) const;
		[[nodiscard]] bool IsReadOnly(std::string_view name) const;
		[[nodiscard]] std::optional<std::filesystem::path> GetRoot(std::string_view name) const;
		[[nodiscard]] std::vector<std::string> GetMountNames() const;

		/* @brief The physical path a virtual path names (it needn't exist). Nullopt for an unknown mount or a path climbing out. */
		[[nodiscard]] std::optional<std::filesystem::path> Resolve(std::string_view virtualPath) const;

		/* @brief The existing physical file or folder a virtual path names, matched case-insensitively when needed. */
		[[nodiscard]] std::optional<std::filesystem::path> Find(std::string_view virtualPath) const;

		/* @brief The virtual path of a physical one inside a mount (the deepest matching root wins), else nullopt. */
		[[nodiscard]] std::optional<std::string> ToVirtual(const std::filesystem::path &physical) const;

		[[nodiscard]] bool Exists(std::string_view virtualPath) const;
		[[nodiscard]] bool IsDirectory(std::string_view virtualPath) const;

		/* @brief A folder's entries, folders first then files, each group sorted by name. Recursive listings go depth-first. */
		[[nodiscard]] std::vector<Entry> List(std::string_view folder, bool recursive = false) const;

		bool Read(std::string_view virtualPath, std::string &bytes, std::string *error = nullptr) const;
		bool Write(std::string_view virtualPath, std::string_view bytes, std::string *error = nullptr) const;
		bool CreateFolders(std::string_view virtualPath, std::string *error = nullptr) const;
		bool Copy(std::string_view from, std::string_view to, bool overwrite, std::string *error = nullptr) const;

		/* @brief Removes a file or an empty folder (never a folder with something in it). */
		bool Remove(std::string_view virtualPath, std::string *error = nullptr) const;

	private:
		struct MountPoint
		{
			std::string name;
			std::filesystem::path root;
			bool readOnly = false;
		};

		[[nodiscard]] std::optional<MountPoint> FindMount(std::string_view name) const;
		std::optional<std::filesystem::path> ResolveWritable(std::string_view virtualPath, std::string *error) const;

		mutable std::shared_mutex m_Mutex;
		std::vector<MountPoint> m_Mounts;
	};

	/* @brief The editor's shared file system. */
	FileSystem &Get();
}

// -------------------------------------------------------
