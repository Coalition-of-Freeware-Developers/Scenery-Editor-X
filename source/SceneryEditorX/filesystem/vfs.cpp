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
 * vfs.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "vfs.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <random>

// -------------------------------------------------------

namespace SceneryEditorX::VFS
{
	namespace fs = std::filesystem;

	namespace
	{
		char ToLowerAscii(const char c)
		{
			return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
		}

		std::string ToLowerAscii(std::string_view text)
		{
			std::string lower(text);
			std::ranges::transform(lower, lower.begin(), [](const char c) { return ToLowerAscii(c); });
			return lower;
		}

		bool EqualsNoCase(std::string_view a, std::string_view b)
		{
			return a.size() == b.size() && std::ranges::equal(a, b, [](const char x, const char y) { return ToLowerAscii(x) == ToLowerAscii(y); });
		}

		bool Fail(std::string *error, std::string message)
		{
			if (error)
				*error = std::move(message);
			return false;
		}

		/* @brief Hex digits that make a temporary name unique among concurrent writers. */
		std::string MakeUniqueSuffix()
		{
			static std::atomic<uint64_t> counter{ 0 };
			static const uint64_t seed = []
			{
				std::random_device device;
				return (static_cast<uint64_t>(device()) << 32) ^ device() ^
					static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
			}();
			uint64_t value = seed ^ (++counter * 0x9e3779b97f4a7c15ull);
			value ^= value >> 31;
			char text[17];
			static constexpr char DIGITS[] = "0123456789abcdef";
			for (int i = 15; i >= 0; --i)
			{
				text[i] = DIGITS[value & 15u];
				value >>= 4;
			}
			text[16] = '\0';
			return text;
		}

		/* @brief An environment variable as a path (UTF-16 on Windows, so non-ASCII user names survive), empty when unset. */
		fs::path GetEnvPath(const char *name)
		{
#if defined(_WIN32)
			std::wstring wide(name, name + std::char_traits<char>::length(name));
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
			const wchar_t *value = _wgetenv(wide.c_str());
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
			return value && *value ? fs::path(value) : fs::path();
#else
			const char *value = std::getenv(name);
			return value && *value ? FromUtf8(value) : fs::path();
#endif
		}

		/* @brief Absolute and lexically normal, without a trailing separator (what IsInside compares). */
		fs::path Normalized(const fs::path &path)
		{
			std::error_code ec;
			fs::path absolute = fs::absolute(path, ec);
			if (ec)
				absolute = path;
			absolute = absolute.lexically_normal();
			if (absolute.has_relative_path() && absolute.filename().empty())
				absolute = absolute.parent_path();
			return absolute;
		}

		bool SameComponent(const fs::path &a, const fs::path &b)
		{
#if defined(_WIN32) || defined(__APPLE__)
			return EqualsNoCase(ToUtf8(a), ToUtf8(b));	// The platform's file systems ignore case by default
#else
			return a == b;
#endif
		}

		/**
		 * @brief The path with each existing component spelled the way the disk spells it, the rest as given.
		 * @param complete Set when every component exists.
		 */
		fs::path MatchExistingCase(const fs::path &path, bool &complete)
		{
			complete = false;
			std::error_code ec;
			if (fs::exists(path, ec))
			{
				complete = true;
				return path;
			}

			fs::path current = path.root_path();
			bool matching = true;
			for (const fs::path &component : path.relative_path())
			{
				if (component.empty())
					continue;
				const fs::path exact = current / component;
				if (!matching || component == "." || component == ".." || fs::exists(exact, ec))
				{
					current = exact;
					continue;
				}

				// Pick the case-insensitive match; several (only on case-sensitive disks) resolve to the smallest name
				const std::string wanted = ToLowerAscii(ToUtf8(component));
				std::optional<fs::path> best;
				const fs::path folder = current.empty() ? fs::path(".") : current;
				for (fs::directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
				{
					const fs::path name = it->path().filename();
					if (ToLowerAscii(ToUtf8(name)) == wanted && (!best || name < *best))
						best = name;
				}
				if (best)
					current /= *best;
				else
				{
					current = exact;
					matching = false;	// Nothing below a missing folder can exist
				}
			}
			complete = matching && fs::exists(current, ec);
			return current;
		}

		/* @brief Folders first, then names (case-insensitively, then exactly). */
		bool EntryBefore(const Entry &a, const Entry &b)
		{
			if (a.directory != b.directory)
				return a.directory;
			const std::string la = ToLowerAscii(a.name), lb = ToLowerAscii(b.name);
			return la != lb ? la < lb : a.name < b.name;
		}

		/* @brief A line's value with surrounding whitespace and quotes removed. */
		std::string Trim(std::string_view text)
		{
			const auto blank = [](const char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
			while (!text.empty() && blank(text.front()))
				text.remove_prefix(1);
			while (!text.empty() && blank(text.back()))
				text.remove_suffix(1);
			if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
				text = text.substr(1, text.size() - 2);
			return std::string(text);
		}

#if !defined(_WIN32) && !defined(__APPLE__)
		/* @brief XDG_DOCUMENTS_DIR from ~/.config/user-dirs.dirs ("$HOME/Documents"), empty when it isn't set there. */
		fs::path ReadXdgDocuments(const fs::path &home, const fs::path &config)
		{
			std::ifstream file(config / "user-dirs.dirs");
			std::string line;
			while (std::getline(file, line))
			{
				constexpr std::string_view KEY = "XDG_DOCUMENTS_DIR=";
				const std::string trimmed = Trim(line);
				if (!trimmed.starts_with(KEY))
					continue;
				std::string value = Trim(std::string_view(trimmed).substr(KEY.size()));
				if (value.starts_with("$HOME"))
					return home / FromUtf8(NormalizeRelative(std::string_view(value).substr(5)));
				return FromUtf8(value);
			}
			return {};
		}
#endif
	}

	// -------------------------------------------------------
	// Paths and names

	std::string ToUtf8(const fs::path &path)
	{
		const std::u8string text = path.u8string();
		return std::string(text.begin(), text.end());
	}

	fs::path FromUtf8(const std::string_view text)
	{
		return fs::path(std::u8string(text.begin(), text.end()));
	}

	std::string NormalizeRelative(const std::string_view path, bool *escaped)
	{
		if (escaped)
			*escaped = false;
		std::vector<std::string_view> parts;
		size_t start = 0;
		while (start <= path.size())
		{
			size_t end = path.find_first_of("/\\", start);
			if (end == std::string_view::npos)
				end = path.size();
			const std::string_view part = path.substr(start, end - start);
			if (part == "..")
			{
				if (parts.empty())
				{
					if (escaped)
						*escaped = true;
					return {};
				}
				parts.pop_back();
			}
			else if (!part.empty() && part != ".")
				parts.push_back(part);
			start = end + 1;
		}

		std::string out;
		for (const std::string_view part : parts)
		{
			if (!out.empty())
				out += '/';
			out += part;
		}
		return out;
	}

	bool SplitVirtualPath(const std::string_view virtualPath, std::string &mount, std::string &relative)
	{
		const size_t colon = virtualPath.find(':');
		// Two characters at least, so a Windows drive ("C:/...") is never read as a mount
		if (colon == std::string_view::npos || colon < 2)
			return false;
		const std::string_view name = virtualPath.substr(0, colon);
		if (!std::ranges::all_of(name, [](const char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.'; }))
			return false;
		bool escaped = false;
		std::string rest = NormalizeRelative(virtualPath.substr(colon + 1), &escaped);
		if (escaped)
			return false;
		mount = ToLowerAscii(name);
		relative = std::move(rest);
		return true;
	}

	std::string MakeVirtualPath(const std::string_view mount, const std::string_view relative)
	{
		return ToLowerAscii(mount) + ":/" + NormalizeRelative(relative);
	}

	std::string Join(const std::string_view virtualPath, const std::string_view relative)
	{
		std::string mount, base;
		if (!SplitVirtualPath(virtualPath, mount, base))
			return NormalizeRelative(std::string(virtualPath) + "/" + std::string(relative));
		return MakeVirtualPath(mount, base.empty() ? std::string(relative) : base + "/" + std::string(relative));
	}

	std::string CheckPortableName(const std::string_view name)
	{
		if (name.empty())
			return "The name is empty";
		if (name == "." || name == "..")
			return "\"" + std::string(name) + "\" isn't a usable name";
		if (name.size() > 255)
			return "The name is longer than 255 characters";
		for (const char c : name)
		{
			if (static_cast<unsigned char>(c) < 32)
				return "The name contains a control character";
			if (std::string_view("<>:\"/\\|?*").find(c) != std::string_view::npos)
				return std::string("Windows doesn't allow '") + c + "' in names";
		}
		if (name.back() == '.' || name.back() == ' ')
			return "Windows doesn't allow names ending in a dot or a space";

		// Reserved device names, with or without an extension
		const std::string stem = ToLowerAscii(name.substr(0, name.find('.')));
		static constexpr std::string_view RESERVED[] = { "con", "prn", "aux", "nul" };
		bool reserved = std::ranges::find(RESERVED, std::string_view(stem)) != std::end(RESERVED);
		if (stem.size() == 4 && (stem.starts_with("com") || stem.starts_with("lpt")) && stem[3] >= '1' && stem[3] <= '9')
			reserved = true;
		if (reserved)
			return "\"" + std::string(name) + "\" is a reserved name on Windows";
		return {};
	}

	std::string MakePortableName(const std::string_view name, const std::string_view fallback)
	{
		std::string out;
		out.reserve(name.size());
		for (const char c : name)
		{
			const bool forbidden = static_cast<unsigned char>(c) < 32 || std::string_view("<>:\"/\\|?*").find(c) != std::string_view::npos;
			out += forbidden ? '_' : c;
		}
		while (!out.empty() && (out.back() == '.' || out.back() == ' '))
			out.pop_back();
		while (!out.empty() && out.front() == ' ')
			out.erase(out.begin());
		if (out.size() > 255)
		{
			out.resize(255);
			while (!out.empty() && (static_cast<unsigned char>(out.back()) & 0xC0u) == 0x80u)
				out.pop_back();		// Don't cut a UTF-8 sequence in half
			if (!out.empty() && static_cast<unsigned char>(out.back()) >= 0xC0u)
				out.pop_back();
		}
		if (out.empty() || out == "." || out == "..")
			return std::string(fallback);
		if (!CheckPortableName(out).empty())
		{
			// What's left is a reserved device name: keep it recognizable
			const size_t dot = out.find('.');
			out.insert(dot == std::string::npos ? out.size() : dot, "_");
		}
		return out;
	}

	bool IsInside(const fs::path &path, const fs::path &folder)
	{
		const fs::path child = Normalized(path);
		const fs::path parent = Normalized(folder);
		auto c = child.begin();
		for (auto p = parent.begin(); p != parent.end(); ++p, ++c)
		{
			if (p->empty())
				continue;
			if (c == child.end() || !SameComponent(*c, *p))
				return false;
		}
		return true;
	}

	// -------------------------------------------------------
	// Physical file operations

	std::optional<fs::path> FindCaseInsensitive(const fs::path &path)
	{
		bool complete = false;
		fs::path match = MatchExistingCase(path, complete);
		if (!complete)
			return std::nullopt;
		return match;
	}

	bool ReadFile(const fs::path &path, std::string &bytes, std::string *error)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
			return Fail(error, "Could not open " + ToUtf8(path));
		const std::streamsize size = file.tellg();
		if (size < 0)
			return Fail(error, "Could not read " + ToUtf8(path));
		bytes.resize(static_cast<size_t>(size));
		file.seekg(0, std::ios::beg);
		if (size > 0 && !file.read(bytes.data(), size))
			return Fail(error, "Could not read " + ToUtf8(path));
		return true;
	}

	bool WriteFileAtomic(const fs::path &path, const std::string_view bytes, std::string *error)
	{
		std::error_code ec;
		if (fs::is_directory(path, ec))
			return Fail(error, "Could not write " + ToUtf8(path) + ": a folder of that name is in the way");
		if (path.has_parent_path() && !CreateFolders(path.parent_path(), error))
			return false;

		fs::path temporary = path;
		temporary += ".tmp-" + MakeUniqueSuffix();
		{
			std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
			if (!file)
				return Fail(error, "Could not write " + ToUtf8(temporary));
			file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
			file.flush();
			if (!file)
			{
				file.close();
				fs::remove(temporary, ec);
				return Fail(error, "Writing " + ToUtf8(path) + " failed (disk full?)");
			}
		}

		// Check what landed on disk before it replaces anything
		if (fs::file_size(temporary, ec) != bytes.size() || ec)
		{
			fs::remove(temporary, ec);
			return Fail(error, "Writing " + ToUtf8(path) + " failed (the file came out short)");
		}

		fs::rename(temporary, path, ec);
		if (ec && fs::is_regular_file(path))
		{
			// Some runtimes won't rename over an existing file: move the old one aside first, put it back on failure.
			fs::path aside = path;
			aside += ".old-" + MakeUniqueSuffix();
			std::error_code moveError;
			fs::rename(path, aside, moveError);
			if (!moveError)
			{
				ec.clear();
				fs::rename(temporary, path, ec);
				if (ec)
					fs::rename(aside, path, moveError);
				else
					fs::remove(aside, moveError);
			}
		}
		if (ec)
		{
			std::error_code removeError;
			fs::remove(temporary, removeError);
			return Fail(error, "Could not replace " + ToUtf8(path) + ": " + ec.message());
		}
		return true;
	}

	bool CreateFolders(const fs::path &path, std::string *error)
	{
		if (path.empty())
			return true;
		std::error_code ec;
		if (fs::is_directory(path, ec))
			return true;
		fs::create_directories(path, ec);
		if (ec || !fs::is_directory(path, ec))
			return Fail(error, "Could not create " + ToUtf8(path) + (ec ? ": " + ec.message() : std::string()));
		return true;
	}

	bool CopyFile(const fs::path &from, const fs::path &to, const bool overwrite, std::string *error)
	{
		std::error_code ec;
		if (!fs::is_regular_file(from, ec))
			return Fail(error, ToUtf8(from) + " isn't a file");
		if (!overwrite && fs::exists(to, ec))
			return Fail(error, ToUtf8(to) + " already exists");
		if (to.has_parent_path() && !CreateFolders(to.parent_path(), error))
			return false;
		fs::copy_file(from, to, overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none, ec);
		if (ec)
			return Fail(error, "Could not copy " + ToUtf8(from) + " to " + ToUtf8(to) + ": " + ec.message());
		return true;
	}

	// -------------------------------------------------------
	// Platform folders

	fs::path GetFolder(const Folder folder)
	{
		std::error_code ec;
		if (folder == Folder::Temp)
		{
			fs::path temp = fs::temp_directory_path(ec);
			return ec ? fs::path() : temp;
		}

#if defined(_WIN32)
		const fs::path home = GetEnvPath("USERPROFILE");
		switch (folder)
		{
			case Folder::Home:		return home;
			case Folder::Documents:	return home.empty() ? fs::path() : home / "Documents";
			case Folder::Config:	return GetEnvPath("APPDATA");
			case Folder::Data:
			case Folder::Cache:		return GetEnvPath("LOCALAPPDATA");
			default:				return {};
		}
#elif defined(__APPLE__)
		const fs::path home = GetEnvPath("HOME");
		if (home.empty())
			return {};
		switch (folder)
		{
			case Folder::Home:		return home;
			case Folder::Documents:	return home / "Documents";
			case Folder::Config:
			case Folder::Data:		return home / "Library" / "Application Support";
			case Folder::Cache:		return home / "Library" / "Caches";
			default:				return {};
		}
#else
		const fs::path home = GetEnvPath("HOME");
		const auto xdg = [&home](const char *variable, const char *fallback)
		{
			const fs::path value = GetEnvPath(variable);
			if (!value.empty() && value.is_absolute())		// The spec says relative values are invalid
				return value;
			return home.empty() ? fs::path() : home / FromUtf8(fallback);
		};
		switch (folder)
		{
			case Folder::Home:		return home;
			case Folder::Documents:
			{
				const fs::path documents = ReadXdgDocuments(home, xdg("XDG_CONFIG_HOME", ".config"));
				return !documents.empty() ? documents : (home.empty() ? fs::path() : home / "Documents");
			}
			case Folder::Config:	return xdg("XDG_CONFIG_HOME", ".config");
			case Folder::Data:		return xdg("XDG_DATA_HOME", ".local/share");
			case Folder::Cache:		return xdg("XDG_CACHE_HOME", ".cache");
			default:				return {};
		}
#endif
	}

	fs::path GetAppFolder(const Folder folder, const std::string_view appName)
	{
		const fs::path base = GetFolder(folder);
		return base.empty() ? fs::path() : base / FromUtf8(appName);
	}

	// -------------------------------------------------------
	// X-Plane

	fs::path GetXPlaneInstallList(const int version)
	{
		const std::string file = "x-plane_install_" + std::to_string(version) + ".txt";
#if defined(_WIN32)
		const fs::path folder = GetEnvPath("LOCALAPPDATA");
		return folder.empty() ? fs::path() : folder / file;
#elif defined(__APPLE__)
		const fs::path home = GetEnvPath("HOME");
		return home.empty() ? fs::path() : home / "Library" / "Preferences" / file;
#else
		const fs::path home = GetEnvPath("HOME");
		return home.empty() ? fs::path() : home / ".x-plane" / file;
#endif
	}

	bool IsXPlaneFolder(const fs::path &folder)
	{
		std::error_code ec;
		if (folder.empty() || !fs::is_directory(folder, ec))
			return false;
		const auto has = [&folder](const char *name) { return FindCaseInsensitive(folder / name).has_value(); };
		return has("Resources") && (has("Custom Scenery") || has("X-Plane.exe") || has("X-Plane.app") || has("X-Plane-x86_64"));
	}

	std::vector<fs::path> FindXPlaneInstalls(const int version)
	{
		std::vector<fs::path> installs;
		std::string text;
		const fs::path list = GetXPlaneInstallList(version);
		if (list.empty() || !ReadFile(list, text))
			return installs;

		size_t start = 0;
		while (start < text.size())
		{
			size_t end = text.find('\n', start);
			if (end == std::string::npos)
				end = text.size();
			const std::string line = Trim(std::string_view(text).substr(start, end - start));
			start = end + 1;
			if (line.empty())
				continue;
			fs::path folder = FromUtf8(line);
			if (folder.has_relative_path() && folder.filename().empty())
				folder = folder.parent_path();		// "K:/X-Plane 12/"
			if (IsXPlaneFolder(folder) && std::ranges::none_of(installs, [&folder](const fs::path &known) { return IsInside(known, folder) && IsInside(folder, known); }))
				installs.push_back(folder);
		}
		return installs;
	}

	fs::path GetCustomSceneryFolder(const fs::path &xplaneRoot)
	{
		if (const std::optional<fs::path> found = FindCaseInsensitive(xplaneRoot / "Custom Scenery"))
			return *found;
		return xplaneRoot / "Custom Scenery";
	}

	// -------------------------------------------------------
	// Mounts

	bool FileSystem::Mount(const std::string_view name, const fs::path &root, const bool readOnly, std::string *error)
	{
		std::string mount, relative;
		if (!SplitVirtualPath(std::string(name) + ":", mount, relative))
			return Fail(error, "\"" + std::string(name) + "\" isn't a usable mount name (2+ letters, digits, '-', '_' or '.')");
		if (root.empty())
			return Fail(error, "The " + mount + " mount needs a folder");

		std::unique_lock lock(m_Mutex);
		std::erase_if(m_Mounts, [&mount](const MountPoint &point) { return point.name == mount; });
		m_Mounts.push_back({ mount, Normalized(root), readOnly });
		return true;
	}

	bool FileSystem::Unmount(const std::string_view name)
	{
		const std::string mount = ToLowerAscii(name);
		std::unique_lock lock(m_Mutex);
		return std::erase_if(m_Mounts, [&mount](const MountPoint &point) { return point.name == mount; }) > 0;
	}

	bool FileSystem::IsMounted(const std::string_view name) const
	{
		return FindMount(name).has_value();
	}

	bool FileSystem::IsReadOnly(const std::string_view name) const
	{
		const std::optional<MountPoint> mount = FindMount(name);
		return mount && mount->readOnly;
	}

	std::optional<fs::path> FileSystem::GetRoot(const std::string_view name) const
	{
		const std::optional<MountPoint> mount = FindMount(name);
		return mount ? std::optional<fs::path>(mount->root) : std::nullopt;
	}

	std::vector<std::string> FileSystem::GetMountNames() const
	{
		std::shared_lock lock(m_Mutex);
		std::vector<std::string> names;
		for (const MountPoint &point : m_Mounts)
			names.push_back(point.name);
		std::ranges::sort(names);
		return names;
	}

	std::optional<FileSystem::MountPoint> FileSystem::FindMount(const std::string_view name) const
	{
		const std::string mount = ToLowerAscii(name);
		std::shared_lock lock(m_Mutex);
		for (const MountPoint &point : m_Mounts)
		{
			if (point.name == mount)
				return point;
		}
		return std::nullopt;
	}

	std::optional<fs::path> FileSystem::Resolve(const std::string_view virtualPath) const
	{
		std::string name, relative;
		if (!SplitVirtualPath(virtualPath, name, relative))
			return std::nullopt;
		const std::optional<MountPoint> mount = FindMount(name);
		if (!mount)
			return std::nullopt;
		return relative.empty() ? mount->root : mount->root / FromUtf8(relative);
	}

	std::optional<fs::path> FileSystem::Find(const std::string_view virtualPath) const
	{
		const std::optional<fs::path> path = Resolve(virtualPath);
		return path ? FindCaseInsensitive(*path) : std::nullopt;
	}

	std::optional<std::string> FileSystem::ToVirtual(const fs::path &physical) const
	{
		const fs::path path = Normalized(physical);
		std::shared_lock lock(m_Mutex);
		const MountPoint *best = nullptr;
		size_t bestDepth = 0;
		for (const MountPoint &point : m_Mounts)
		{
			if (!IsInside(path, point.root))
				continue;
			const size_t depth = static_cast<size_t>(std::distance(point.root.begin(), point.root.end()));
			if (!best || depth > bestDepth)
			{
				best = &point;
				bestDepth = depth;
			}
		}
		if (!best)
			return std::nullopt;

		// Component-wise, so a root spelled in another case (Windows, macOS) still strips cleanly
		std::string relative;
		auto it = path.begin();
		for (size_t i = 0; i < bestDepth && it != path.end(); ++i)
			++it;
		for (; it != path.end(); ++it)
		{
			if (it->empty())
				continue;
			if (!relative.empty())
				relative += '/';
			relative += ToUtf8(*it);
		}
		return MakeVirtualPath(best->name, relative);
	}

	bool FileSystem::Exists(const std::string_view virtualPath) const
	{
		return Find(virtualPath).has_value();
	}

	bool FileSystem::IsDirectory(const std::string_view virtualPath) const
	{
		const std::optional<fs::path> path = Find(virtualPath);
		std::error_code ec;
		return path && fs::is_directory(*path, ec);
	}

	std::vector<Entry> FileSystem::List(const std::string_view folder, const bool recursive) const
	{
		std::vector<Entry> out;
		const std::optional<fs::path> path = Find(folder);
		std::error_code ec;
		if (!path || !fs::is_directory(*path, ec))
			return out;

		std::vector<Entry> entries;
		for (fs::directory_iterator it(*path, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
		{
			Entry entry;
			entry.name = ToUtf8(it->path().filename());
			entry.path = Join(folder, entry.name);
			std::error_code statError;
			entry.directory = it->is_directory(statError);
			entry.size = entry.directory ? 0 : static_cast<uint64_t>(it->file_size(statError));
			if (statError)
				entry.size = 0;
			entries.push_back(std::move(entry));
		}
		std::ranges::sort(entries, EntryBefore);

		for (Entry &entry : entries)
		{
			const bool descend = recursive && entry.directory;
			const std::string child = entry.path;
			out.push_back(std::move(entry));
			if (descend)
			{
				std::vector<Entry> below = List(child, true);
				out.insert(out.end(), std::make_move_iterator(below.begin()), std::make_move_iterator(below.end()));
			}
		}
		return out;
	}

	bool FileSystem::Read(const std::string_view virtualPath, std::string &bytes, std::string *error) const
	{
		const std::optional<fs::path> path = Find(virtualPath);
		if (!path)
			return Fail(error, std::string(virtualPath) + " doesn't exist");
		return ReadFile(*path, bytes, error);
	}

	std::optional<fs::path> FileSystem::ResolveWritable(const std::string_view virtualPath, std::string *error) const
	{
		std::string name, relative;
		if (!SplitVirtualPath(virtualPath, name, relative))
		{
			Fail(error, "\"" + std::string(virtualPath) + "\" isn't a virtual path (mount:/folder/file)");
			return std::nullopt;
		}
		const std::optional<MountPoint> mount = FindMount(name);
		if (!mount)
		{
			Fail(error, "Nothing is mounted as \"" + name + "\"");
			return std::nullopt;
		}
		if (mount->readOnly)
		{
			Fail(error, "The " + name + " mount is read-only");
			return std::nullopt;
		}
		// Existing folders keep the spelling they have on disk, so a case-sensitive disk doesn't grow a twin folder
		bool complete = false;
		return MatchExistingCase(relative.empty() ? mount->root : mount->root / FromUtf8(relative), complete);
	}

	bool FileSystem::Write(const std::string_view virtualPath, const std::string_view bytes, std::string *error) const
	{
		const std::optional<fs::path> path = ResolveWritable(virtualPath, error);
		return path && WriteFileAtomic(*path, bytes, error);
	}

	bool FileSystem::CreateFolders(const std::string_view virtualPath, std::string *error) const
	{
		const std::optional<fs::path> path = ResolveWritable(virtualPath, error);
		return path && VFS::CreateFolders(*path, error);
	}

	bool FileSystem::Copy(const std::string_view from, const std::string_view to, const bool overwrite, std::string *error) const
	{
		const std::optional<fs::path> source = Find(from);
		if (!source)
			return Fail(error, std::string(from) + " doesn't exist");
		const std::optional<fs::path> target = ResolveWritable(to, error);
		if (!target)
			return false;

		std::error_code ec;
		if (!fs::is_directory(*source, ec))
			return CopyFile(*source, *target, overwrite, error);

		// A folder: its whole tree
		if (!VFS::CreateFolders(*target, error))
			return false;
		for (fs::recursive_directory_iterator it(*source, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
		{
			const fs::path destination = *target / it->path().lexically_relative(*source);
			std::error_code typeError;
			if (it->is_directory(typeError))
			{
				if (!VFS::CreateFolders(destination, error))
					return false;
			}
			else if (!CopyFile(it->path(), destination, overwrite, error))
				return false;
		}
		if (ec)
			return Fail(error, "Could not list " + ToUtf8(*source) + ": " + ec.message());
		return true;
	}

	bool FileSystem::Remove(const std::string_view virtualPath, std::string *error) const
	{
		if (!ResolveWritable(virtualPath, error))
			return false;
		const std::optional<fs::path> path = Find(virtualPath);
		if (!path)
			return true;	// Already gone
		std::error_code ec;
		if (fs::is_directory(*path, ec) && !fs::is_empty(*path, ec))
			return Fail(error, ToUtf8(*path) + " isn't empty");
		fs::remove(*path, ec);
		if (ec)
			return Fail(error, "Could not remove " + ToUtf8(*path) + ": " + ec.message());
		return true;
	}

	FileSystem &Get()
	{
		static FileSystem fileSystem;
		return fileSystem;
	}
}

// -------------------------------------------------------
