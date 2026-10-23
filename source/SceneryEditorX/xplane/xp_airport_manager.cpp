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
 * xp_airport_manager.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_airport_manager.h"
#include "xp_text.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <set>
#include <thread>
#include <nlohmann/json.hpp>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace fs = std::filesystem;

	namespace
	{
		constexpr const char *LOG_TAG = "XPlaneAirports";
		constexpr const char *SETTINGS_FILE = "xplane_airports.json";
		constexpr const char *CACHE_FILE = "airport_index.bin";

		using VFS::FromUtf8;
		using VFS::IsDirectory;
		using VFS::IsFile;
		using VFS::PathKey;
		using VFS::ToUtf8;

		fs::path AptDatOf(const fs::path &package)
		{
			return package / "Earth nav data" / "apt.dat";
		}

		struct State
		{
			std::mutex mutex;
			std::vector<fs::path> trackedPackages;
			std::shared_ptr<const AirportIndex> index = std::make_shared<AirportIndex>();
			std::string message = "Not scanned";
			double lastScanMs = 0.0;

			std::atomic<ScanState> scanState{ ScanState::Idle };
			std::atomic<uint64_t> generation{ 0 };
			std::atomic<bool> cancel{ false };

			std::thread worker;
			std::condition_variable wake;
			bool scanRequested = false;
			bool stopping = false;
			bool initialized = false;

			~State()
			{
				{
					std::scoped_lock lock(mutex);
					stopping = true;
				}
				cancel.store(true);
				wake.notify_all();
				if (worker.joinable())
					worker.detach();
			}
		};

		State &GetState()
		{
			static State state;
			return state;
		}

		fs::path GetSettingsPath() { return IO::FileSystem::GetPersistentStoragePath() / SETTINGS_FILE; }
		fs::path GetCachePath() { return VFS::GetAppLocation(VFS::AppLocation::Cache) / CACHE_FILE; }	// Rebuilt when missing

		void LoadSettings(std::vector<fs::path> &tracked)
		{
			std::string text;
			if (!ReadTextFile(GetSettingsPath(), text))
				return;
			try
			{
				const nlohmann::json json = nlohmann::json::parse(text);
				if (json.contains("trackedPackages") && json["trackedPackages"].is_array())
				{
					for (const auto &entry : json["trackedPackages"])
					{
						if (entry.is_string())
							tracked.push_back(FromUtf8(entry.get<std::string>()));
					}
				}
			}
			catch (const std::exception &e)
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "Ignoring unreadable {}: {}", GetSettingsPath().string(), e.what());
			}
		}

		void SaveSettings(const std::vector<fs::path> &tracked)
		{
			nlohmann::json json;
			json["version"] = 1;
			json["trackedPackages"] = nlohmann::json::array();
			for (const fs::path &folder : tracked)
				json["trackedPackages"].push_back(ToUtf8(folder));

			std::string error;
			if (!VFS::WriteFileAtomic(GetSettingsPath(), json.dump(2), &error))
				SEDX_CORE_WARN_TAG(LOG_TAG, "Could not save the airport settings: {}", error);
		}

		/**
		 * @brief apt.dat sources in X-Plane's load order: tracked packs, Custom Scenery (unlisted packs first, then
		 * scenery_packs.ini order), then Global Airports.
		 */
		std::vector<AirportSource> DiscoverSources(const std::vector<fs::path> &tracked, const InstallInfo &install)
		{
			std::vector<AirportSource> sources;
			std::set<std::string> seen;

			const auto add = [&](const fs::path &package, const PackageSource kind, const bool enabled, std::string name = {})
			{
				const fs::path aptDat = AptDatOf(package);
				const std::string key = PathKey(package);
				if (!seen.insert(key).second || !IsFile(aptDat))
					return;

				AirportSource source;
				source.aptDat = aptDat;
				source.package = package;
				source.packageName = name.empty() ? package.filename().string() : std::move(name);
				source.source = kind;
				source.enabledInSim = enabled;
				source.priority = static_cast<int>(sources.size());
				sources.push_back(std::move(source));
			};

			for (const fs::path &folder : tracked)
				add(folder, PackageSource::Tracked, true);

			if (!install.IsValid())
				return sources;

			const fs::path customScenery = VFS::GetCustomSceneryFolder(install.root);
			std::vector<SceneryPackEntry> entries;
			if (std::string ini; ReadTextFile(customScenery / "scenery_packs.ini", ini))
			{
				std::string error;
				if (!ParseSceneryPacks(ini, install.root, entries, &error))
					SEDX_CORE_WARN_TAG(LOG_TAG, "scenery_packs.ini: {}", error);
			}

			std::set<std::string> listed;
			for (const SceneryPackEntry &entry : entries)
				listed.insert(PathKey(entry.path));

			// Packs X-Plane hasn't recorded in the ini yet load first on its next start.
			std::error_code ec;
			std::vector<fs::path> unlisted;
			for (const fs::directory_entry &entry : fs::directory_iterator(customScenery, fs::directory_options::skip_permission_denied, ec))
			{
				if (entry.is_directory(ec) && !listed.contains(PathKey(entry.path())))
					unlisted.push_back(entry.path());
			}
			std::ranges::sort(unlisted);
			for (const fs::path &path : unlisted)
				add(path, PackageSource::CustomScenery, true);

			bool globalListed = false;
			for (const SceneryPackEntry &entry : entries)
			{
				if (entry.isGlobalAirports)
				{
					globalListed = true;
					add(install.root / "Global Scenery" / "Global Airports", PackageSource::GlobalScenery, entry.enabled, "Global Airports");
				}
				else
					add(entry.path, PackageSource::CustomScenery, entry.enabled);
			}
			if (!globalListed)
				add(install.root / "Global Scenery" / "Global Airports", PackageSource::GlobalScenery, true, "Global Airports");

			// Any other Global Scenery pack with airports (demo areas)
			for (const fs::directory_entry &entry : fs::directory_iterator(install.root / "Global Scenery", fs::directory_options::skip_permission_denied, ec))
			{
				if (entry.is_directory(ec))
					add(entry.path(), PackageSource::GlobalScenery, true);
			}
			return sources;
		}

		void WorkerLoop()
		{
			State &state = GetState();
			while (true)
			{
				std::vector<fs::path> tracked;
				{
					std::unique_lock lock(state.mutex);
					state.wake.wait(lock, [&state] { return state.scanRequested || state.stopping; });
					if (state.stopping)
						return;
					state.scanRequested = false;
					tracked = state.trackedPackages;
					state.message = "Scanning airports...";
				}
				state.cancel.store(false);
				state.scanState = ScanState::Scanning;
				const auto start = std::chrono::steady_clock::now();

				const InstallInfo install = InspectInstall(LibraryManager::GetInstallPath());
				auto index = std::make_shared<AirportIndex>();
				std::string message;
				try
				{
					AirportIndex cache;
					cache.LoadCache(GetCachePath());

					const std::vector<AirportSource> sources = DiscoverSources(tracked, install);
					bool cancelled = false;
					for (const AirportSource &source : sources)
					{
						if (state.cancel.load())
						{
							cancelled = true;
							break;
						}
						index->AddSource(source, &cache, &state.cancel);
					}
					if (cancelled)
						continue;	// A new scan was requested; it starts right away

					index->Finish();
					if (!index->SaveCache(GetCachePath()))
						SEDX_CORE_WARN_TAG(LOG_TAG, "Could not write the airport cache {}", GetCachePath().string());

					size_t packs = 0;
					for (const AirportSource &source : index->GetSources())
						packs += source.error.empty() ? 1 : 0;
					message = std::to_string(index->GetAirports().size()) + " airports (" + std::to_string(index->Get3DCount()) + " 3D) in " + std::to_string(packs) + " scenery packs";
					if (!install.IsValid() && tracked.empty())
						message = install.root.empty() ? "Set the X-Plane folder to index its airports" : install.GetProblem();
				}
				catch (const std::exception &e)
				{
					message = std::string("Airport scan failed: ") + e.what();
					SEDX_CORE_ERROR_TAG(LOG_TAG, "{}", message);
					state.scanState = ScanState::Failed;
					std::scoped_lock lock(state.mutex);
					state.message = message;
					continue;
				}

				const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
				{
					std::scoped_lock lock(state.mutex);
					state.index = std::move(index);
					state.message = message;
					state.lastScanMs = ms;
				}
				++state.generation;
				state.scanState = ScanState::Ready;
				SEDX_CORE_INFO_TAG(LOG_TAG, "{} ({:.0f} ms)", message, ms);
			}
		}

		void RequestScanLocked(State &state)
		{
			state.scanRequested = true;
			state.cancel.store(true);	// Restart a scan that's running
			state.wake.notify_one();
		}
	}

	// -------------------------------------------------------

	void AirportManager::Init()
	{
		State &state = GetState();
		{
			std::scoped_lock lock(state.mutex);
			if (state.initialized)
				return;
			state.initialized = true;
			state.stopping = false;
			LoadSettings(state.trackedPackages);
		}

		state.worker = std::thread(WorkerLoop);
		std::scoped_lock lock(state.mutex);
		RequestScanLocked(state);
	}

	void AirportManager::Shutdown()
	{
		State &state = GetState();
		std::vector<fs::path> tracked;
		{
			std::scoped_lock lock(state.mutex);
			if (!state.initialized)
				return;
			state.initialized = false;
			state.stopping = true;
			tracked = state.trackedPackages;
		}
		state.cancel.store(true);
		state.wake.notify_all();
		if (state.worker.joinable())
			state.worker.join();
		SaveSettings(tracked);
	}

	void AirportManager::Rescan()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		if (state.initialized)
			RequestScanLocked(state);
	}

	ScanState AirportManager::GetScanState()
	{
		return GetState().scanState.load();
	}

	std::string AirportManager::GetScanMessage()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.message;
	}

	uint64_t AirportManager::GetGeneration()
	{
		return GetState().generation.load();
	}

	std::shared_ptr<const AirportIndex> AirportManager::GetIndex()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.index;
	}

	double AirportManager::GetLastScanMilliseconds()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.lastScanMs;
	}

	const std::vector<fs::path> &AirportManager::GetTrackedPackages()
	{
		// Only the main thread changes the list, so handing out a reference is fine.
		return GetState().trackedPackages;
	}

	bool AirportManager::AddTrackedPackage(const fs::path &folder)
	{
		if (!IsDirectory(folder) || !IsFile(AptDatOf(folder)))
			return false;

		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		const std::string key = PathKey(folder);
		for (const fs::path &existing : state.trackedPackages)
		{
			if (PathKey(existing) == key)
				return false;
		}
		state.trackedPackages.push_back(folder);
		SaveSettings(state.trackedPackages);
		if (state.initialized)
			RequestScanLocked(state);
		return true;
	}

	void AirportManager::RemoveTrackedPackage(const fs::path &folder)
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		const std::string key = PathKey(folder);
		const size_t before = state.trackedPackages.size();
		std::erase_if(state.trackedPackages, [&key](const fs::path &existing) { return PathKey(existing) == key; });
		if (state.trackedPackages.size() != before)
		{
			SaveSettings(state.trackedPackages);
			if (state.initialized)
				RequestScanLocked(state);
		}
	}

	fs::path AirportManager::GetDefaultPackageRoot()
	{
		const InstallInfo &install = LibraryManager::GetInstallInfo();
		return install.IsValid() ? install.root / "Custom Scenery" : fs::path();
	}
}

// -------------------------------------------------------
