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
 * xp_library_manager.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xp_library_manager.h"
#include "xp_text.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <nlohmann/json.hpp>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/logging/logging.hpp>

#ifdef _WIN32
	#include <windows.h>
	#include <shobjidl.h>
	#include <shlguid.h>
#endif

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace fs = std::filesystem;

	namespace
	{
		constexpr const char *LOG_TAG = "XPlaneLibrary";
		constexpr const char *SETTINGS_FILE = "xplane_library.json";

		using VFS::FromUtf8;
		using VFS::IsDirectory;
		using VFS::IsFile;
		using VFS::PathKey;
		using VFS::ToUtf8;

#ifdef _WIN32
		/* @brief Target of a Windows shortcut (.lnk); X-Plane follows shortcuts placed in Custom Scenery. */
		fs::path ResolveShortcut(const fs::path &link)
		{
			const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
			fs::path target;

			IShellLinkW *shellLink = nullptr;
			if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void **>(&shellLink))))
			{
				IPersistFile *file = nullptr;
				if (SUCCEEDED(shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void **>(&file))))
				{
					if (SUCCEEDED(file->Load(link.wstring().c_str(), STGM_READ)))
					{
						wchar_t buffer[MAX_PATH] = {};
						if (SUCCEEDED(shellLink->GetPath(buffer, MAX_PATH, nullptr, SLGP_RAWPATH)))
							target = buffer;
					}
					file->Release();
				}
				shellLink->Release();
			}

			if (SUCCEEDED(init))
				CoUninitialize();
			return target;
		}
#endif

		// -------------------------------------------------------

		struct Settings
		{
			fs::path installPath;
			std::vector<fs::path> trackedFolders;
			std::map<std::string, bool> packageOverrides; // PathKey -> tracked
			bool checkFiles = true;
		};

		struct State
		{
			std::mutex mutex;			// Guards everything below except the atomics
			Settings settings;
			InstallInfo installInfo;
			std::shared_ptr<const LibraryIndex> index = std::make_shared<LibraryIndex>();
			std::string message = "Not scanned";
			double lastScanMs = 0.0;

			std::atomic<ScanState> scanState{ ScanState::Idle };
			std::atomic<uint64_t> generation{ 0 };

			std::thread worker;
			std::condition_variable wake;
			bool scanRequested = false;
			bool stopping = false;
			bool initialized = false;

			~State()
			{
				// LibraryManager::Shutdown() joins the worker; if the process exits without it, a joinable
				// std::thread would call std::terminate, so let it go (logging may already be torn down).
				{
					std::scoped_lock lock(mutex);
					stopping = true;
				}
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

		fs::path GetSettingsPath()
		{
			return IO::FileSystem::GetPersistentStoragePath() / SETTINGS_FILE;
		}

		void LoadSettings(Settings &settings)
		{
			std::string text;
			if (!ReadTextFile(GetSettingsPath(), text))
				return;

			try
			{
				const nlohmann::json json = nlohmann::json::parse(text);
				if (json.contains("installPath") && json["installPath"].is_string())
					settings.installPath = FromUtf8(json["installPath"].get<std::string>());
				if (json.contains("trackedFolders") && json["trackedFolders"].is_array())
				{
					for (const auto &entry : json["trackedFolders"])
					{
						if (entry.is_string())
							settings.trackedFolders.push_back(FromUtf8(entry.get<std::string>()));
					}
				}
				if (json.contains("packages") && json["packages"].is_object())
				{
					for (const auto &[key, value] : json["packages"].items())
					{
						if (value.is_boolean())
							settings.packageOverrides[key] = value.get<bool>();
					}
				}
				if (json.contains("checkFiles") && json["checkFiles"].is_boolean())
					settings.checkFiles = json["checkFiles"].get<bool>();
			}
			catch (const std::exception &e)
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "Ignoring unreadable {}: {}", VFS::ToUtf8(GetSettingsPath()), e.what());
			}
		}

		void SaveSettings(const Settings &settings)
		{
			nlohmann::json json;
			json["version"] = 1;
			json["installPath"] = ToUtf8(settings.installPath);
			json["trackedFolders"] = nlohmann::json::array();
			for (const fs::path &folder : settings.trackedFolders)
				json["trackedFolders"].push_back(ToUtf8(folder));
			json["packages"] = nlohmann::json::object();
			for (const auto &[key, tracked] : settings.packageOverrides)
				json["packages"][key] = tracked;
			json["checkFiles"] = settings.checkFiles;

			std::string error;
			if (!VFS::WriteFileAtomic(GetSettingsPath(), json.dump(2), &error))
				SEDX_CORE_WARN_TAG(LOG_TAG, "Could not save the library settings: {}", error);
		}

		// -------------------------------------------------------

		/**
		 * @brief Library packages in X-Plane priority order: tracked folders, Custom Scenery (new packs first, then
		 * scenery_packs.ini order), Global Scenery, then Resources/default scenery.
		 */
		std::vector<LibraryPackage> DiscoverPackages(const Settings &settings, const InstallInfo &install)
		{
			std::vector<LibraryPackage> packages;
			std::set<std::string> seen;

			const auto add = [&](const fs::path &path, const PackageSource source, const bool enabledInSim, std::string name = {})
			{
				const std::string key = PathKey(path);
				if (!seen.insert(key).second || !IsDirectory(path))
					return;

				LibraryPackage package;
				package.name = name.empty() ? path.filename().string() : std::move(name);
				package.path = path;
				package.source = source;
				package.enabledInSim = enabledInSim;
				package.hasLibrary = IsFile(path / "library.txt");
				package.priority = static_cast<int>(packages.size());

				// X-Plane only loads enabled packs; tracked folders and default scenery are always part of the index.
				package.tracked = package.hasLibrary && (source != PackageSource::CustomScenery || enabledInSim);
				if (const auto it = settings.packageOverrides.find(key); it != settings.packageOverrides.end())
					package.tracked = it->second && package.hasLibrary;

				packages.push_back(std::move(package));
			};

			for (const fs::path &folder : settings.trackedFolders)
				add(folder, PackageSource::Tracked, true);

			if (!install.IsValid())
				return packages;

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

			// Packs X-Plane hasn't recorded yet go to the top of scenery_packs.ini on its next start.
			std::error_code ec;
			for (const fs::directory_entry &entry : fs::directory_iterator(customScenery, fs::directory_options::skip_permission_denied, ec))
			{
				fs::path path = entry.path();
#ifdef _WIN32
				if (ToLower(path.extension().string()) == ".lnk")
				{
					const fs::path target = ResolveShortcut(path);
					if (target.empty())
						continue;
					path = target;
				}
#endif
				if (!IsDirectory(path) || listed.contains(PathKey(path)))
					continue;
				add(path, PackageSource::CustomScenery, true);
			}

			for (const SceneryPackEntry &entry : entries)
				add(entry.path, entry.isGlobalAirports ? PackageSource::GlobalScenery : PackageSource::CustomScenery, entry.enabled,
					entry.isGlobalAirports ? std::string("Global Airports") : std::string());

			for (const fs::directory_entry &entry : fs::directory_iterator(VFS::ResolveCase(install.root / "Global Scenery"), fs::directory_options::skip_permission_denied, ec))
			{
				if (entry.is_directory(ec))
					add(entry.path(), PackageSource::GlobalScenery, true);
			}

			std::vector<fs::path> defaults;
			for (const fs::directory_entry &entry : fs::directory_iterator(VFS::ResolveCase(install.root / "Resources" / "default scenery"), fs::directory_options::skip_permission_denied, ec))
			{
				if (entry.is_directory(ec))
					defaults.push_back(entry.path());
			}
			std::ranges::sort(defaults);
			for (const fs::path &path : defaults)
				add(path, PackageSource::DefaultScenery, true);

			return packages;
		}

		void WorkerLoop()
		{
			State &state = GetState();
			while (true)
			{
				Settings settings;
				InstallInfo install;
				{
					std::unique_lock lock(state.mutex);
					state.wake.wait(lock, [&state] { return state.scanRequested || state.stopping; });
					if (state.stopping)
						return;

					state.scanRequested = false;
					settings = state.settings;
					install = state.installInfo;
					state.message = "Scanning libraries...";
				}

				state.scanState = ScanState::Scanning;
				const auto start = std::chrono::steady_clock::now();

				auto index = std::make_shared<LibraryIndex>();
				std::string message;
				try
				{
					std::vector<LibraryPackage> packages = DiscoverPackages(settings, install);
					LibraryIndex::BuildOptions options;
					options.checkFilesExist = settings.checkFiles;
					index->Build(std::move(packages), options);

					uint32_t libraries = 0, tracked = 0;
					for (const LibraryPackage &package : index->GetPackages())
					{
						libraries += package.hasLibrary ? 1 : 0;
						tracked += package.hasLibrary && package.tracked ? 1 : 0;
					}

					message = std::to_string(index->GetAssets().size()) + " assets from " + std::to_string(tracked) + " of " +
							  std::to_string(libraries) + " libraries";
					if (!install.IsValid() && settings.trackedFolders.empty())
						message = install.root.empty() ? "Set the X-Plane folder to index its libraries" : install.GetProblem();
				}
				catch (const std::exception &e)
				{
					message = std::string("Library scan failed: ") + e.what();
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
			state.wake.notify_one();
		}
	}

	// -------------------------------------------------------

	std::string InstallInfo::GetProblem() const
	{
		if (root.empty())
			return "No X-Plane folder set";
		if (!exists)
			return "Folder not found";
		if (!hasDefaultScenery)
			return "Missing 'Resources/default scenery' (not an X-Plane folder?)";
		if (!hasCustomScenery)
			return "Missing 'Custom Scenery'";
		return {};
	}

	InstallInfo InspectInstall(const fs::path &root)
	{
		InstallInfo info;
		info.root = root;
		if (root.empty())
			return info;

		info.exists = IsDirectory(root);
		info.hasResources = IsDirectory(root / "Resources");
		info.hasDefaultScenery = IsDirectory(root / "Resources" / "default scenery");
		info.hasCustomScenery = IsDirectory(root / "Custom Scenery");
		info.hasGlobalScenery = IsDirectory(root / "Global Scenery");
		info.hasSceneryPacksIni = IsFile(root / "Custom Scenery" / "scenery_packs.ini");
		info.hasExecutable = IsFile(root / "X-Plane.exe") || IsFile(root / "X-Plane-x86_64") || IsDirectory(root / "X-Plane.app");

		// "Log.txt for X-Plane 12.4.3-r2-15ff1e4d (build ..." on the first line of the last run's log
		std::ifstream log(root / "Log.txt");
		std::string firstLine;
		if (log && std::getline(log, firstLine))
		{
			if (const size_t at = firstLine.find("X-Plane "); at != std::string::npos)
			{
				std::string version = firstLine.substr(at + 8);
				version = version.substr(0, version.find_first_of(" ("));
				// Drop the build hash ("12.4.3-r2-15ff1e4d" -> "12.4.3-r2")
				if (const size_t dash = version.find('-'); dash != std::string::npos)
				{
					if (const size_t second = version.find('-', dash + 1); second != std::string::npos)
						version.resize(second);
				}
				info.version = version;
			}
		}
		return info;
	}

	// -------------------------------------------------------

	void LibraryManager::Init()
	{
		State &state = GetState();
		{
			std::scoped_lock lock(state.mutex);
			if (state.initialized)
				return;

			LoadSettings(state.settings);
		}

		// Auto-detect outside the lock (the search reads Steam's files, the registry and several folders).
		fs::path installPath;
		{
			std::scoped_lock lock(state.mutex);
			installPath = state.settings.installPath;
		}

		if (installPath.empty() || !InspectInstall(installPath).IsValid())
		{
			for (const InstallCandidate &candidate : FindInstallCandidates())
			{
				if (InspectInstall(candidate.root).IsValid())
				{
					SEDX_CORE_INFO_TAG(LOG_TAG, "Detected X-Plane {} at '{}' ({})", candidate.majorVersion, VFS::ToUtf8(candidate.root), GetInstallSourceName(candidate.source));
					installPath = candidate.root;
					break;
				}
			}
		}

		const InstallInfo info = InspectInstall(installPath);
		{
			std::scoped_lock lock(state.mutex);
			const bool changed = PathKey(state.settings.installPath) != PathKey(installPath);
			state.settings.installPath = installPath;
			state.installInfo = info;
			state.initialized = true;
			state.stopping = false;
			if (changed)
				SaveSettings(state.settings);

			state.worker = std::thread(WorkerLoop);
			RequestScanLocked(state);
		}

		if (!info.IsValid())
			SEDX_CORE_WARN_TAG(LOG_TAG, "X-Plane folder: {}", info.GetProblem());
	}

	void LibraryManager::Shutdown()
	{
		State &state = GetState();
		{
			std::scoped_lock lock(state.mutex);
			if (!state.initialized)
				return;
			state.stopping = true;
			state.wake.notify_all();
		}

		if (state.worker.joinable())
			state.worker.join();

		std::scoped_lock lock(state.mutex);
		SaveSettings(state.settings);
		state.initialized = false;
	}

	fs::path LibraryManager::GetInstallPath()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.settings.installPath;
	}

	const InstallInfo &LibraryManager::GetInstallInfo()
	{
		// Only replaced on the main thread (SetInstallPath / Init), so the reference stays valid for the frame.
		return GetState().installInfo;
	}

	bool LibraryManager::SetInstallPath(const fs::path &path)
	{
		const InstallInfo info = InspectInstall(path);
		if (!info.IsValid())
		{
			SEDX_CORE_WARN_TAG(LOG_TAG, "'{}' is not an X-Plane folder: {}", VFS::ToUtf8(path), info.GetProblem());
			return false;
		}

		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		state.settings.installPath = path;
		state.installInfo = info;
		SaveSettings(state.settings);
		RequestScanLocked(state);
		SEDX_CORE_INFO_TAG(LOG_TAG, "X-Plane folder set to '{}'", VFS::ToUtf8(path));
		return true;
	}

	const std::vector<fs::path> &LibraryManager::GetTrackedFolders()
	{
		return GetState().settings.trackedFolders;
	}

	bool LibraryManager::AddTrackedFolder(const fs::path &path)
	{
		if (!IsDirectory(path))
			return false;

		if (!IsFile(path / "library.txt"))
			SEDX_CORE_WARN_TAG(LOG_TAG, "'{}' has no library.txt yet; it will be indexed once it has one", VFS::ToUtf8(path));

		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		const std::string key = PathKey(path);
		if (std::ranges::any_of(state.settings.trackedFolders, [&key](const fs::path &p) { return PathKey(p) == key; }))
			return false;

		state.settings.trackedFolders.push_back(path);
		SaveSettings(state.settings);
		RequestScanLocked(state);
		return true;
	}

	void LibraryManager::RemoveTrackedFolder(const fs::path &path)
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		const std::string key = PathKey(path);
		std::erase_if(state.settings.trackedFolders, [&key](const fs::path &p) { return PathKey(p) == key; });
		SaveSettings(state.settings);
		RequestScanLocked(state);
	}

	void LibraryManager::SetPackageTracked(const fs::path &packagePath, const bool tracked)
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		state.settings.packageOverrides[PathKey(packagePath)] = tracked;
		SaveSettings(state.settings);
		RequestScanLocked(state);
	}

	bool LibraryManager::GetCheckFiles()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.settings.checkFiles;
	}

	void LibraryManager::SetCheckFiles(const bool check)
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		if (state.settings.checkFiles == check)
			return;
		state.settings.checkFiles = check;
		SaveSettings(state.settings);
		RequestScanLocked(state);
	}

	void LibraryManager::Rescan()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		state.installInfo = InspectInstall(state.settings.installPath);
		RequestScanLocked(state);
	}

	ScanState LibraryManager::GetScanState()
	{
		return GetState().scanState.load();
	}

	std::string LibraryManager::GetScanMessage()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.message;
	}

	uint64_t LibraryManager::GetGeneration()
	{
		return GetState().generation.load();
	}

	std::shared_ptr<const LibraryIndex> LibraryManager::GetIndex()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.index;
	}

	fs::path LibraryManager::Resolve(const std::string_view virtualPath)
	{
		return GetIndex()->Resolve(virtualPath);
	}

	double LibraryManager::GetLastScanMilliseconds()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.lastScanMs;
	}

}

// -------------------------------------------------------
