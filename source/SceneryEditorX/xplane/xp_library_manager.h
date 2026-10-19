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
 * xp_library_manager.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_library.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/* @brief What was found at an X-Plane install path. */
	struct InstallInfo
	{
		std::filesystem::path root;
		bool exists = false;
		bool hasResources = false;
		bool hasDefaultScenery = false;
		bool hasCustomScenery = false;
		bool hasGlobalScenery = false;
		bool hasExecutable = false;
		bool hasSceneryPacksIni = false;
		std::string version;			// From the last run's Log.txt, when present ("12.4.3-r2")

		/* @brief Usable for library scanning: the scenery folders exist. */
		[[nodiscard]] bool IsValid() const { return exists && hasDefaultScenery && hasCustomScenery; }

		/* @brief One-line description of what's missing, empty when valid. */
		[[nodiscard]] std::string GetProblem() const;
	};

	/* @brief Inspects a folder for an X-Plane 12 installation. */
	InstallInfo InspectInstall(const std::filesystem::path &root);

	/**
	 * @brief X-Plane installs found on this machine: the paths X-Plane records in x-plane_install_12.txt and Steam
	 * libraries. Valid installs come first.
	 */
	std::vector<std::filesystem::path> FindInstalls();

	/* @brief State of the background library scan. */
	enum class ScanState : uint8_t
	{
		Idle,
		Scanning,
		Ready,
		Failed
	};

	/**
	 * @class LibraryManager
	 * @brief The editor's X-Plane library integration: the X-Plane 12 folder, the library packages it provides
	 * (default scenery, Custom Scenery in scenery_packs.ini order, plus folders the user tracks) and the asset index.
	 *
	 * Settings persist in <app data>/xplane_library.json. Scans run on a worker thread; the index is published as an
	 * immutable snapshot, so the UI can keep drawing the previous one while a rescan runs.
	 */
	class LibraryManager
	{
	public:
		/* @brief Loads the settings, auto-detects the X-Plane folder when none is set, and starts the first scan. */
		static void Init();

		/* @brief Waits for a running scan and saves the settings. */
		static void Shutdown();

		/* @brief The configured X-Plane folder (may be empty or invalid). */
		static std::filesystem::path GetInstallPath();
		static const InstallInfo &GetInstallInfo();

		/**
		 * @brief Sets the X-Plane folder, saves it and rescans.
		 * @return False (and nothing changes) when the folder isn't an X-Plane installation.
		 */
		static bool SetInstallPath(const std::filesystem::path &path);

		/* @brief Folders outside X-Plane tracked as library packages (e.g. a library under development). */
		static const std::vector<std::filesystem::path> &GetTrackedFolders();
		static bool AddTrackedFolder(const std::filesystem::path &path);
		static void RemoveTrackedFolder(const std::filesystem::path &path);

		/* @brief Includes or excludes a package from the asset index (saved), then rescans. */
		static void SetPackageTracked(const std::filesystem::path &packagePath, bool tracked);

		/* @brief Whether file-existence checks run during scans (slower, but flags broken exports). */
		static bool GetCheckFiles();
		static void SetCheckFiles(bool check);

		/* @brief Starts a background rescan (a running scan is restarted with the new settings). */
		static void Rescan();

		static ScanState GetScanState();
		static std::string GetScanMessage();

		/* @brief Increments every time a new index is published (UI caches compare against it). */
		static uint64_t GetGeneration();

		/* @brief The latest published index; never null after Init() (empty before the first scan finishes). */
		static std::shared_ptr<const LibraryIndex> GetIndex();

		/* @brief File X-Plane would load for a virtual path ("lib/airport/..."), or empty. */
		static std::filesystem::path Resolve(std::string_view virtualPath);

		/* @brief Duration of the last completed scan, in milliseconds. */
		static double GetLastScanMilliseconds();
	};

}

// -------------------------------------------------------
