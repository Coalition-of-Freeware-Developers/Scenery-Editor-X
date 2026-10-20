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
 * xp_install_finder.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/* @brief Steam app ids of the simulators the editor works with. */
	inline constexpr int STEAM_APP_XPLANE_12 = 2014780;
	inline constexpr int STEAM_APP_XPLANE_11 = 269950;

	/* @brief Where an X-Plane install was found, in the order the sources are tried. */
	enum class InstallSource : uint8_t
	{
		InstallList,	// x-plane_install_<version>.txt: X-Plane records every install it has run from
		Steam,			// A Steam library's appmanifest
		Registry,		// A Windows uninstall entry (Steam writes one per game)
		CommonFolder	// A usual folder: the desktop (Laminar's installer default), the home folder, a fixed drive
	};

	/* @brief A folder to look at, with the X-Plane version its source suggests (0: the source doesn't say). */
	struct InstallHint
	{
		std::filesystem::path path;
		int majorVersion = 0;
	};

	/* @brief Where to look. MakeInstallSearch() fills it in for this machine; tests build their own. */
	struct InstallSearch
	{
		std::vector<InstallHint> installLists;				// x-plane_install_<version>.txt files
		std::vector<std::filesystem::path> steamRoots;		// Steam folders (holding steamapps/ and config/)
		std::vector<InstallHint> registered;				// Install folders named by uninstall entries
		std::vector<InstallHint> commonFolders;				// Folders tried as they are
	};

	/* @brief An X-Plane install found on this machine. */
	struct InstallCandidate
	{
		std::filesystem::path root;
		int majorVersion = 0;								// 11 or 12: from Log.txt, else the source, else the folder name (0: unknown)
		InstallSource source = InstallSource::CommonFolder;
		bool complete = false;								// Has Resources/default scenery and Custom Scenery, which a library scan needs
		std::filesystem::file_time_type lastRun{};			// When Log.txt was written, i.e. X-Plane last ran from here (min: never)
	};

	/* @brief The folders an x-plane_install_<version>.txt lists, one per line ("K:\SteamLibrary/.../X-Plane 12/"). */
	std::vector<std::filesystem::path> ParseInstallList(std::string_view text);

	/* @brief The library folders in a Steam libraryfolders.vdf ("path" values, or the numbered values of the old layout). */
	std::vector<std::filesystem::path> ParseSteamLibraryFolders(std::string_view vdf);

	/* @brief The "installdir" of a Steam appmanifest_<app id>.acf (empty when it has none). */
	std::string ParseSteamInstallDir(std::string_view acf);

	/* @brief The major version on the first line of X-Plane's Log.txt ("Log.txt for X-Plane 12.4.3-r2 (build ..." -> 12; 0: none). */
	int ParseLogMajorVersion(std::string_view firstLine);

	/* @brief The major version in a name such as "X-Plane 11" or "X-Plane 12 Demo" (0: none). */
	int MajorVersionFromName(std::string_view name);

	/* @brief A short name for a source, for the log and the UI ("Steam", "registry", ...). */
	const char *GetInstallSourceName(InstallSource source);

	/**
	 * @brief Where X-Plane can be on this machine: X-Plane's install lists (11 and 12), the Steam folders the registry
	 * or the platform names, the uninstall entries of Steam's X-Plane apps and of anything called "X-Plane 11/12", and
	 * the usual folders on the desktop, in the home folder and on fixed drives. Removable, optical and network drives
	 * are never touched.
	 */
	InstallSearch MakeInstallSearch();

	/**
	 * @brief Every X-Plane folder the search finds, once each, best first: complete installs, then X-Plane 12 before 11,
	 * then the most recently run. Never throws: an unreadable drive or folder is skipped.
	 */
	std::vector<InstallCandidate> FindInstallCandidates(const InstallSearch &search);

	/* @brief FindInstallCandidates(MakeInstallSearch()). */
	std::vector<InstallCandidate> FindInstallCandidates();
}

// -------------------------------------------------------
