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
 * xp_install_finder.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "xp_install_finder.h"
#include "xp_text.h"
#include <SceneryEditorX/filesystem/vfs.h>
#include <algorithm>
#include <fstream>
#include <set>

#if defined(_WIN32)
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
	#include <shlobj.h>
#endif

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace fs = std::filesystem;

	namespace
	{
		/* @brief The quoted strings on a line of a Valve KeyValues file (.vdf, .acf), unescaped ("K:\\SteamLibrary"). */
		std::vector<std::string> QuotedStrings(const std::string_view line)
		{
			std::vector<std::string> strings;
			for (size_t i = 0; i < line.size(); ++i)
			{
				if (line[i] != '"')
					continue;
				std::string value;
				for (++i; i < line.size() && line[i] != '"'; ++i)
				{
					if (line[i] == '\\' && i + 1 < line.size())
						++i;
					value.push_back(line[i]);
				}
				strings.push_back(std::move(value));
			}
			return strings;
		}

		bool IsDigits(const std::string_view text)
		{
			return !text.empty() && std::ranges::all_of(text, [](const char c) { return c >= '0' && c <= '9'; });
		}

		/* @brief A folder without its trailing separator ("D:\" stays the drive's root). */
		fs::path CleanFolder(fs::path folder)
		{
			folder = folder.lexically_normal();
			if (folder.has_relative_path() && folder.filename().empty())
				folder = folder.parent_path();
			return folder;
		}

		void AddUnique(std::vector<fs::path> &folders, const fs::path &folder)
		{
			if (folder.empty())
				return;
			const fs::path clean = CleanFolder(folder);
			const std::string key = VFS::PathKey(clean);
			if (std::ranges::none_of(folders, [&key](const fs::path &known) { return VFS::PathKey(known) == key; }))
				folders.push_back(clean);
		}

		/* @brief The first line of a file (empty when it can't be read). */
		std::string FirstLine(const fs::path &path)
		{
			std::ifstream file(path, std::ios::binary);
			std::string line;
			if (file)
				std::getline(file, line);
			return line;
		}

		/* @brief Collects X-Plane folders once each, in the order they're found. */
		class Collector
		{
		public:
			void Consider(const fs::path &folder, const int versionHint, const InstallSource source)
			{
				if (folder.empty())
					return;
				const fs::path root = CleanFolder(folder);
				if (!m_Seen.insert(VFS::PathKey(root)).second || !VFS::IsXPlaneFolder(root))
					return;

				InstallCandidate candidate;
				candidate.root = root;
				candidate.source = source;
				candidate.complete = VFS::IsDirectory(root / "Resources" / "default scenery") && VFS::IsDirectory(root / "Custom Scenery");

				// The last run's log says which version ran here; a folder can be named after another one
				const fs::path log = root / "Log.txt";
				std::error_code ec;
				if (const fs::file_time_type written = fs::last_write_time(log, ec); !ec)
					candidate.lastRun = written;
				candidate.majorVersion = ParseLogMajorVersion(FirstLine(log));
				if (candidate.majorVersion == 0)
					candidate.majorVersion = versionHint;
				if (candidate.majorVersion == 0)
					candidate.majorVersion = MajorVersionFromName(VFS::ToUtf8(root.filename()));

				m_Found.push_back(std::move(candidate));
			}

			std::vector<InstallCandidate> Take() { return std::move(m_Found); }

		private:
			std::vector<InstallCandidate> m_Found;
			std::set<std::string> m_Seen;
		};

		struct SteamApp
		{
			int id;
			int majorVersion;
			const char *defaultFolder;
		};

		constexpr SteamApp STEAM_APPS[] = { { STEAM_APP_XPLANE_12, 12, "X-Plane 12" }, { STEAM_APP_XPLANE_11, 11, "X-Plane 11" } };

#if defined(_WIN32)
		/* @brief A string value of a registry key (empty when it's missing). view: KEY_WOW64_32KEY / _64KEY, or 0. */
		std::wstring ReadRegistryString(const HKEY root, const wchar_t *key, const wchar_t *value, const REGSAM view = 0)
		{
			HKEY opened = nullptr;
			if (RegOpenKeyExW(root, key, 0, KEY_READ | view, &opened) != ERROR_SUCCESS)
				return {};
			std::wstring text(2048, L'\0');
			DWORD size = static_cast<DWORD>(text.size() * sizeof(wchar_t));
			const LSTATUS status = RegGetValueW(opened, nullptr, value, RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, text.data(), &size);
			RegCloseKey(opened);
			if (status != ERROR_SUCCESS)
				return {};
			text.resize(wcsnlen(text.c_str(), text.size()));
			return text;
		}

		/* @brief The install folders of uninstall entries for Steam's X-Plane apps or anything called "X-Plane 11" / "X-Plane 12". */
		void AddUninstallEntries(std::vector<InstallHint> &found)
		{
			const wchar_t *uninstallKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
			const struct { HKEY root; REGSAM view; } hives[] = {
				{ HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY }, { HKEY_LOCAL_MACHINE, KEY_WOW64_32KEY }, { HKEY_CURRENT_USER, 0 } };

			for (const auto &[root, view] : hives)
			{
				HKEY uninstall = nullptr;
				if (RegOpenKeyExW(root, uninstallKey, 0, KEY_READ | view, &uninstall) != ERROR_SUCCESS)
					continue;

				for (DWORD index = 0;; ++index)
				{
					wchar_t name[256];
					DWORD length = static_cast<DWORD>(std::size(name));
					const LSTATUS status = RegEnumKeyExW(uninstall, index, name, &length, nullptr, nullptr, nullptr, nullptr);
					if (status == ERROR_NO_MORE_ITEMS)
						break;
					if (status != ERROR_SUCCESS)
						continue;		// A name longer than any of the ones looked for

					const std::wstring entry(name, length);
					const std::wstring subKey = std::wstring(uninstallKey) + L"\\" + entry;
					int version = 0;
					for (const SteamApp &app : STEAM_APPS)
					{
						if (entry == L"Steam App " + std::to_wstring(app.id))
							version = app.majorVersion;
					}
					if (version == 0)
					{
						const std::string displayName = VFS::ToUtf8(fs::path(ReadRegistryString(root, subKey.c_str(), L"DisplayName", view)));
						if (ToLower(displayName).starts_with("x-plane"))
							version = MajorVersionFromName(displayName);
						if (version != 11 && version != 12)
							continue;
					}

					if (const std::wstring location = ReadRegistryString(root, subKey.c_str(), L"InstallLocation", view); !location.empty())
						found.push_back({ CleanFolder(fs::path(location)), version });
				}
				RegCloseKey(uninstall);
			}
		}

		fs::path GetShellFolder(const int csidl)
		{
			wchar_t folder[MAX_PATH];
			if (FAILED(SHGetFolderPathW(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, folder)))
				return {};
			return fs::path(folder);
		}
#endif
	}

	// -------------------------------------------------------

	std::vector<fs::path> ParseInstallList(const std::string_view text)
	{
		std::vector<fs::path> folders;
		LineReader reader(text);
		std::string_view line;
		while (reader.Next(line))
		{
			if (const std::string_view folder = Trim(line); !folder.empty())
				folders.push_back(CleanFolder(VFS::FromUtf8(folder)));
		}
		return folders;
	}

	std::vector<fs::path> ParseSteamLibraryFolders(const std::string_view vdf)
	{
		std::vector<fs::path> folders;
		LineReader reader(vdf);
		std::string_view line;
		while (reader.Next(line))
		{
			const std::vector<std::string> strings = QuotedStrings(line);
			if (strings.size() < 2 || strings[1].empty())
				continue;
			// "path" "D:\\SteamLibrary" since 2021; before that "1" "D:\\SteamLibrary". Numbers inside "apps" are app sizes.
			if (ToLower(strings[0]) == "path" || (IsDigits(strings[0]) && !IsDigits(strings[1])))
				folders.push_back(VFS::FromUtf8(strings[1]));
		}
		return folders;
	}

	std::string ParseSteamInstallDir(const std::string_view acf)
	{
		LineReader reader(acf);
		std::string_view line;
		while (reader.Next(line))
		{
			if (const std::vector<std::string> strings = QuotedStrings(line); strings.size() >= 2 && ToLower(strings[0]) == "installdir")
				return strings[1];
		}
		return {};
	}

	int ParseLogMajorVersion(const std::string_view firstLine)
	{
		return MajorVersionFromName(firstLine);
	}

	int MajorVersionFromName(const std::string_view name)
	{
		const std::string lower = ToLower(name);
		for (const std::string_view product : { std::string_view("x-plane"), std::string_view("xplane") })
		{
			for (size_t at = lower.find(product); at != std::string::npos; at = lower.find(product, at + 1))
			{
				size_t i = at + product.size();
				while (i < lower.size() && (lower[i] == ' ' || lower[i] == '_' || lower[i] == '-'))
					++i;
				int version = 0;
				size_t digits = 0;
				for (; i < lower.size() && digits < 3 && lower[i] >= '0' && lower[i] <= '9'; ++i, ++digits)
					version = version * 10 + (lower[i] - '0');
				if (digits > 0)
					return version;
			}
		}
		return 0;
	}

	const char *GetInstallSourceName(const InstallSource source)
	{
		switch (source)
		{
			case InstallSource::InstallList:	return "X-Plane's install list";
			case InstallSource::Steam:			return "Steam";
			case InstallSource::Registry:		return "registry";
			case InstallSource::CommonFolder:	return "usual folder";
		}
		return "";
	}

	// -------------------------------------------------------

	InstallSearch MakeInstallSearch()
	{
		InstallSearch search;
		for (const int version : { 12, 11 })
		{
			if (fs::path list = VFS::GetXPlaneInstallList(version); !list.empty())
				search.installLists.push_back({ std::move(list), version });
		}

		const fs::path home = VFS::GetFolder(VFS::Folder::Home);
		std::vector<fs::path> bases;		// Folders an X-Plane folder is often put straight into

#if defined(_WIN32)
		AddUnique(search.steamRoots, ReadRegistryString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath"));
		AddUnique(search.steamRoots, ReadRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath", KEY_WOW64_32KEY));
		AddUnique(search.steamRoots, ReadRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath", KEY_WOW64_64KEY));
		if (const fs::path programs = GetShellFolder(CSIDL_PROGRAM_FILESX86); !programs.empty())
			AddUnique(search.steamRoots, programs / "Steam");

		AddUninstallEntries(search.registered);

		AddUnique(bases, GetShellFolder(CSIDL_DESKTOPDIRECTORY));		// Laminar's installer puts X-Plane on the desktop
		AddUnique(bases, home);
		AddUnique(bases, VFS::GetFolder(VFS::Folder::Documents));

		// Fixed drives only: asking a card reader, an optical drive or a network share can stall or fail
		const DWORD drives = GetLogicalDrives();
		for (int letter = 0; letter < 26; ++letter)
		{
			if ((drives & (1u << letter)) == 0)
				continue;
			const std::wstring root = std::wstring(1, static_cast<wchar_t>(L'A' + letter)) + L":\\";
			if (GetDriveTypeW(root.c_str()) != DRIVE_FIXED)
				continue;
			const fs::path drive(root);
			for (const fs::path &folder : { drive, drive / "Games", drive / "Program Files", drive / "Program Files (x86)", drive / "SteamLibrary" / "steamapps" / "common" })
				AddUnique(bases, folder);
		}
#elif defined(__APPLE__)
		if (!home.empty())
		{
			AddUnique(search.steamRoots, home / "Library" / "Application Support" / "Steam");
			AddUnique(bases, home / "Desktop");
			AddUnique(bases, home);
			AddUnique(bases, home / "Applications");
		}
		AddUnique(bases, "/Applications");
#else
		if (!home.empty())
		{
			AddUnique(search.steamRoots, home / ".steam" / "steam");
			AddUnique(search.steamRoots, home / ".local" / "share" / "Steam");
			AddUnique(search.steamRoots, home / ".var" / "app" / "com.valvesoftware.Steam" / ".local" / "share" / "Steam");
			AddUnique(bases, home / "Desktop");
			AddUnique(bases, home);
			AddUnique(bases, home / "Games");
		}
		AddUnique(bases, "/opt");
#endif

		for (const fs::path &base : bases)
		{
			for (const SteamApp &app : STEAM_APPS)
				search.commonFolders.push_back({ base / app.defaultFolder, app.majorVersion });
		}
		return search;
	}

	std::vector<InstallCandidate> FindInstallCandidates(const InstallSearch &search)
	{
		Collector collector;

		for (const InstallHint &list : search.installLists)
		{
			std::string text;
			if (!list.path.empty() && VFS::ReadFile(list.path, text))
			{
				for (const fs::path &folder : ParseInstallList(text))
					collector.Consider(folder, list.majorVersion, InstallSource::InstallList);
			}
		}

		for (const fs::path &steam : search.steamRoots)
		{
			// The Steam folder is a library itself; libraryfolders.vdf lists the others (in steamapps/ since 2021, config/ before)
			std::vector<fs::path> libraries = { steam };
			for (const fs::path &vdf : { steam / "steamapps" / "libraryfolders.vdf", steam / "config" / "libraryfolders.vdf" })
			{
				if (std::string text; VFS::ReadFile(vdf, text))
				{
					const std::vector<fs::path> listed = ParseSteamLibraryFolders(text);
					libraries.insert(libraries.end(), listed.begin(), listed.end());
				}
			}

			for (const fs::path &library : libraries)
			{
				const fs::path steamapps = library / "steamapps";
				for (const SteamApp &app : STEAM_APPS)
				{
					std::string installDir;
					if (std::string acf; VFS::ReadFile(steamapps / ("appmanifest_" + std::to_string(app.id) + ".acf"), acf))
						installDir = ParseSteamInstallDir(acf);
					collector.Consider(steamapps / "common" / VFS::FromUtf8(installDir.empty() ? std::string(app.defaultFolder) : installDir),
						app.majorVersion, InstallSource::Steam);
				}
			}
		}

		for (const InstallHint &entry : search.registered)
			collector.Consider(entry.path, entry.majorVersion, InstallSource::Registry);
		for (const InstallHint &folder : search.commonFolders)
			collector.Consider(folder.path, folder.majorVersion, InstallSource::CommonFolder);

		std::vector<InstallCandidate> found = collector.Take();
		std::ranges::stable_sort(found, [](const InstallCandidate &a, const InstallCandidate &b)
		{
			if (a.complete != b.complete)
				return a.complete;
			if (a.majorVersion != b.majorVersion)
				return a.majorVersion > b.majorVersion;
			return a.lastRun > b.lastRun;
		});
		return found;
	}

	std::vector<InstallCandidate> FindInstallCandidates()
	{
		return FindInstallCandidates(MakeInstallSearch());
	}
}

// -------------------------------------------------------
