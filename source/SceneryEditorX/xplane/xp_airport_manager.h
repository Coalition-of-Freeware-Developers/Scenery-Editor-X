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
 * xp_airport_manager.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_airport_index.h"
#include "xp_library_manager.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * @class AirportManager
	 * @brief The editor's catalog of X-Plane airports: every apt.dat the configured X-Plane 12 install loads (Global
	 * Airports plus each Custom Scenery pack, in scenery_packs.ini order) and the scenery packs the user tracks
	 * outside it. Scans run on a worker thread and reuse a disk cache, so the 380 MB Global Airports file is only
	 * read again when it changes. The index is published as an immutable snapshot like the library index.
	 *
	 * Settings persist in <app data>/xplane_airports.json, the cache in <app data>/airport_index.bin.
	 */
	class AirportManager
	{
	public:
		/* @brief Loads the settings and cache and starts the first scan. Call after LibraryManager::Init(). */
		static void Init();

		/* @brief Cancels a running scan, waits for the worker and saves the settings. */
		static void Shutdown();

		/* @brief Starts a background rescan (the X-Plane folder is re-read from LibraryManager). */
		static void Rescan();

		static ScanState GetScanState();
		static std::string GetScanMessage();

		/* @brief Increments every time a new index is published. */
		static uint64_t GetGeneration();

		/* @brief The latest published index; never null after Init() (empty before the first scan finishes). */
		static std::shared_ptr<const AirportIndex> GetIndex();

		static double GetLastScanMilliseconds();

		/* @brief Scenery packs outside the X-Plane folder (work in progress) that are indexed too. */
		static const std::vector<std::filesystem::path> &GetTrackedPackages();

		/* @brief Tracks a scenery pack folder (one with Earth nav data/apt.dat), saves and rescans. False when it isn't one. */
		static bool AddTrackedPackage(const std::filesystem::path &folder);
		static void RemoveTrackedPackage(const std::filesystem::path &folder);

		/* @brief Where new packs go by default: the install's Custom Scenery folder, or empty when no install is set. */
		static std::filesystem::path GetDefaultPackageRoot();
	};

}

// -------------------------------------------------------
