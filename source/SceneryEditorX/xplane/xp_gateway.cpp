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
 * xp_gateway.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_gateway.h"
#include "xp_text.h"
#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>
#include <thread>
#include <curl/curl.h>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace fs = std::filesystem;

	namespace
	{
		constexpr const char *LOG_TAG = "Gateway";
		constexpr const char *USER_AGENT = "SceneryEditorX/1.0 (+https://github.com/The3dVehicleguy/Scenery-Editor-X)";
		constexpr int64_t AIRPORTS_MAX_AGE_SECONDS = 24 * 60 * 60;

		struct State
		{
			std::mutex mutex;				// Guards the queue, active, airports, airportsTimestamp
			std::condition_variable wake;
			std::deque<std::shared_ptr<GatewayRequest>> queue;
			std::shared_ptr<GatewayRequest> active;
			std::shared_ptr<const std::vector<GatewayAirport>> airports;
			int64_t airportsTimestamp = 0;

			std::thread worker;
			bool stopping = false;
			bool initialized = false;

			~State()
			{
				// Shutdown() joins the worker; if the process exits without it, don't let a joinable thread terminate us.
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

		fs::path CacheDirectory()
		{
			return VFS::GetAppLocation(VFS::AppLocation::Cache) / "gateway";		// Downloads: fetched again when missing
		}

		fs::path AirportsCachePath()
		{
			return CacheDirectory() / "airports.json";
		}

		fs::path SceneryCachePath(const int sceneryId)
		{
			return CacheDirectory() / "scenery" / (std::to_string(sceneryId) + ".json");
		}

		/* @brief Write time of a file as seconds since the Unix epoch, 0 when it doesn't exist. */
		int64_t FileTimestamp(const fs::path &path)
		{
			std::error_code ec;
			const fs::file_time_type time = fs::last_write_time(path, ec);
			if (ec)
				return 0;
			const auto age = fs::file_time_type::clock::now() - time;
			const auto when = std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::system_clock::duration>(age);
			return std::chrono::duration_cast<std::chrono::seconds>(when.time_since_epoch()).count();
		}

		int64_t NowSeconds()
		{
			return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}

		bool WriteCacheFile(const fs::path &path, const std::string &text)
		{
			return VFS::WriteFileAtomic(path, text);
		}

		/* @brief Percent-encodes a path segment (airport codes are plain, but the Gateway accepts anything). */
		std::string EncodeSegment(const std::string_view text)
		{
			std::string out;
			for (const unsigned char c : text)
			{
				if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
					out.push_back(static_cast<char>(c));
				else
				{
					char buffer[4];
					snprintf(buffer, sizeof(buffer), "%%%02X", c);
					out.append(buffer);
				}
			}
			return out;
		}

		// -------------------------------------------------------

		struct Download
		{
			GatewayRequest *request = nullptr;
			std::string *body = nullptr;
		};

		size_t WriteBody(const char *data, const size_t size, const size_t count, void *userdata)
		{
			Download *download = static_cast<Download *>(userdata);
			const size_t bytes = size * count;
			download->body->append(data, bytes);
			download->request->m_Bytes.fetch_add(bytes);
			return bytes;
		}

		int ReportProgress(void *userdata, const curl_off_t downloadTotal, const curl_off_t downloaded, curl_off_t, curl_off_t)
		{
			const Download *download = static_cast<Download *>(userdata);
			if (download->request->m_Cancel.load())
				return 1;	// Aborts the transfer
			if (downloadTotal > 0)
				download->request->m_Progress.store(static_cast<float>(static_cast<double>(downloaded) / static_cast<double>(downloadTotal)));
			return 0;
		}

		bool HttpGet(const std::string &url, GatewayRequest &request, std::string &body, std::string &error)
		{
			CURL *curl = curl_easy_init();
			if (!curl)
			{
				error = "libcurl could not be initialized";
				return false;
			}

			Download download{ &request, &body };
			char errorBuffer[CURL_ERROR_SIZE] = {};
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
			curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");		// gzip: the airport list is 20 MB uncompressed
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 64L);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBody);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &download);
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ReportProgress);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &download);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

			const CURLcode result = curl_easy_perform(curl);
			long httpStatus = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
			curl_easy_cleanup(curl);

			if (result == CURLE_ABORTED_BY_CALLBACK)
			{
				error = "Cancelled";
				return false;
			}
			if (result != CURLE_OK)
			{
				error = errorBuffer[0] ? errorBuffer : curl_easy_strerror(result);
				return false;
			}
			if (httpStatus == 404)
			{
				error = "Not found on the Gateway (HTTP 404)";
				return false;
			}
			if (httpStatus < 200 || httpStatus >= 300)
			{
				error = "The Gateway answered HTTP " + std::to_string(httpStatus);
				return false;
			}
			return true;
		}

		// -------------------------------------------------------

		/* @brief Runs one request on the worker; returns false with request.m_Error set on failure. */
		bool Process(GatewayRequest &request)
		{
			State &state = GetState();
			std::string body;
			std::string error;
			const auto fail = [&request](std::string message)
			{
				request.m_Error = std::move(message);
				return false;
			};

			switch (request.m_Kind)
			{
				case GatewayRequest::Kind::Airports:
				{
					const fs::path cachePath = AirportsCachePath();
					int64_t timestamp = FileTimestamp(cachePath);
					bool fromCache = false;
					if (!request.m_ForceRefresh && timestamp > 0 && (request.m_CacheOnly || NowSeconds() - timestamp < AIRPORTS_MAX_AGE_SECONDS))
						fromCache = ReadTextFile(cachePath, body) && !body.empty();

					if (!fromCache)
					{
						if (request.m_CacheOnly)
							return fail("No cached airport list yet");
						if (!HttpGet(std::string(GATEWAY_API_URL) + "/airports", request, body, error))
							return fail(error);
						timestamp = NowSeconds();
					}

					auto airports = std::make_shared<std::vector<GatewayAirport>>();
					if (!ParseGatewayAirports(body, *airports, &error))
						return fail(error);
					if (!fromCache && !WriteCacheFile(cachePath, body))
						SEDX_CORE_WARN_TAG(LOG_TAG, "Could not cache the airport list in {}", cachePath.string());

					request.m_FromCache.store(fromCache);
					request.airports = airports;
					{
						std::scoped_lock lock(state.mutex);
						state.airports = airports;
						state.airportsTimestamp = timestamp;
					}
					SEDX_CORE_INFO_TAG(LOG_TAG, "{} Gateway airports {}", airports->size(), fromCache ? "from the cache" : "downloaded");
					return true;
				}
				case GatewayRequest::Kind::Airport:
				{
					if (!HttpGet(std::string(GATEWAY_API_URL) + "/airport/" + EncodeSegment(request.m_Key), request, body, error))
						return fail(error);
					auto details = std::make_shared<GatewayAirportDetails>();
					if (!ParseGatewayAirport(body, *details, &error))
						return fail(error);
					request.airport = details;
					return true;
				}
				case GatewayRequest::Kind::Scenery:
				{
					const int sceneryId = std::atoi(request.m_Key.c_str());
					const fs::path cachePath = SceneryCachePath(sceneryId);
					bool fromCache = false;
					if (!request.m_ForceRefresh)
						fromCache = ReadTextFile(cachePath, body) && !body.empty();
					if (!fromCache)
					{
						if (!HttpGet(std::string(GATEWAY_API_URL) + "/scenery/" + std::to_string(sceneryId), request, body, error))
							return fail(error);
					}

					auto pack = std::make_shared<GatewaySceneryPack>();
					if (!ParseGatewayScenery(body, *pack, &error))
					{
						if (fromCache)
						{
							// A damaged cache entry: fetch again.
							std::error_code ec;
							fs::remove(cachePath, ec);
							request.m_ForceRefresh = true;
							return Process(request);
						}
						return fail(error);
					}
					if (!fromCache && !WriteCacheFile(cachePath, body))
						SEDX_CORE_WARN_TAG(LOG_TAG, "Could not cache scenery {} in {}", sceneryId, cachePath.string());

					request.m_FromCache.store(fromCache);
					request.scenery = pack;
					SEDX_CORE_INFO_TAG(LOG_TAG, "Scenery {} ({} {}) {}: {} KB", sceneryId, pack->icao, pack->airportName, fromCache ? "from the cache" : "downloaded", pack->masterZip.size() / 1024);
					return true;
				}
				case GatewayRequest::Kind::Stats:
				{
					if (!HttpGet(std::string(GATEWAY_API_URL) + "/stats", request, body, error))
						return fail(error);
					auto stats = std::make_shared<GatewayStats>();
					if (!ParseGatewayStats(body, *stats, &error))
						return fail(error.empty() ? "Unexpected stats from the Gateway" : error);
					request.stats = stats;
					return true;
				}
				case GatewayRequest::Kind::Releases:
				{
					if (!HttpGet(std::string(GATEWAY_API_URL) + "/releases", request, body, error))
						return fail(error);
					auto releases = std::make_shared<std::vector<GatewayRelease>>();
					if (!ParseGatewayReleases(body, *releases, &error))
						return fail(error);
					request.releases = releases;
					return true;
				}
			}
			return fail("Unknown request");
		}

		void WorkerLoop()
		{
			State &state = GetState();
			while (true)
			{
				std::shared_ptr<GatewayRequest> request;
				{
					std::unique_lock lock(state.mutex);
					state.wake.wait(lock, [&state] { return !state.queue.empty() || state.stopping; });
					if (state.stopping)
						return;
					request = std::move(state.queue.front());
					state.queue.pop_front();
					state.active = request;
				}

				if (request->m_Cancel.load())
				{
					request->m_State.store(GatewayRequestState::Cancelled, std::memory_order_release);
				}
				else
				{
					request->m_State.store(GatewayRequestState::Running, std::memory_order_release);
					const bool ok = Process(*request);
					if (ok)
						request->m_State.store(GatewayRequestState::Done, std::memory_order_release);
					else if (request->m_Cancel.load())
						request->m_State.store(GatewayRequestState::Cancelled, std::memory_order_release);
					else
					{
						if (!request->m_CacheOnly)
							SEDX_CORE_WARN_TAG(LOG_TAG, "{} {} failed: {}", request->m_Path, request->m_Key, request->m_Error);
						request->m_State.store(GatewayRequestState::Failed, std::memory_order_release);
					}
				}

				std::scoped_lock lock(state.mutex);
				state.active.reset();
			}
		}

		std::shared_ptr<GatewayRequest> Enqueue(const GatewayRequest::Kind kind, std::string key, std::string path, const bool forceRefresh, const bool cacheOnly = false)
		{
			auto request = std::make_shared<GatewayRequest>();
			request->m_Kind = kind;
			request->m_Key = std::move(key);
			request->m_Path = std::move(path);
			request->m_ForceRefresh = forceRefresh;
			request->m_CacheOnly = cacheOnly;

			State &state = GetState();
			{
				std::scoped_lock lock(state.mutex);
				if (!state.initialized)
				{
					request->m_Error = "The Gateway client isn't initialized";
					request->m_State.store(GatewayRequestState::Failed, std::memory_order_release);
					return request;
				}
				state.queue.push_back(request);
			}
			state.wake.notify_one();
			return request;
		}

		/* @brief True when a zip entry name is safe to write under a folder (no absolute paths, no ".." parts). */
		bool SafeRelativeName(const std::string &name)
		{
			if (name.empty() || name.front() == '/' || name.find(':') != std::string::npos)
				return false;
			size_t start = 0;
			while (start <= name.size())
			{
				size_t end = name.find('/', start);
				if (end == std::string::npos)
					end = name.size();
				const std::string_view part(name.data() + start, end - start);
				if (part == "..")
					return false;
				if (end >= name.size())
					break;
				start = end + 1;
			}
			return true;
		}
	}

	// -------------------------------------------------------

	const char *GetGatewayRequestStateName(const GatewayRequestState state)
	{
		switch (state)
		{
			case GatewayRequestState::Queued:		return "Queued";
			case GatewayRequestState::Running:		return "Downloading";
			case GatewayRequestState::Done:			return "Done";
			case GatewayRequestState::Failed:		return "Failed";
			case GatewayRequestState::Cancelled:	return "Cancelled";
		}
		return "Unknown";
	}

	bool GatewayRequest::IsFinished() const
	{
		const GatewayRequestState state = GetState();
		return state == GatewayRequestState::Done || state == GatewayRequestState::Failed || state == GatewayRequestState::Cancelled;
	}

	double GatewayRequest::GetSeconds() const
	{
		return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_Queued).count();
	}

	// -------------------------------------------------------

	void GatewayClient::Init()
	{
		State &state = GetState();
		{
			std::scoped_lock lock(state.mutex);
			if (state.initialized)
				return;
			state.initialized = true;
			state.stopping = false;
		}

		curl_global_init(CURL_GLOBAL_DEFAULT);
		state.worker = std::thread(WorkerLoop);

		// The cached airport list is big enough to be worth parsing off the main thread, and nothing goes to the
		// network until the user asks for it.
		Enqueue(GatewayRequest::Kind::Airports, std::string(), "/airports", false, true);
		SEDX_CORE_INFO_TAG(LOG_TAG, "Gateway client ready (cache: {})", CacheDirectory().string());
	}

	void GatewayClient::Shutdown()
	{
		State &state = GetState();
		{
			std::scoped_lock lock(state.mutex);
			if (!state.initialized)
				return;
			state.initialized = false;
			state.stopping = true;
			for (const std::shared_ptr<GatewayRequest> &request : state.queue)
			{
				request->m_Cancel.store(true);
				request->m_State.store(GatewayRequestState::Cancelled, std::memory_order_release);
			}
			state.queue.clear();
			if (state.active)
				state.active->Cancel();
		}
		state.wake.notify_all();
		if (state.worker.joinable())
			state.worker.join();
		state.airports.reset();
		curl_global_cleanup();
	}

	std::shared_ptr<GatewayRequest> GatewayClient::FetchAirports(const bool forceRefresh)
	{
		return Enqueue(GatewayRequest::Kind::Airports, std::string(), "/airports", forceRefresh);
	}

	std::shared_ptr<GatewayRequest> GatewayClient::FetchAirport(const std::string &code)
	{
		return Enqueue(GatewayRequest::Kind::Airport, code, "/airport", true);
	}

	std::shared_ptr<GatewayRequest> GatewayClient::FetchScenery(const int sceneryId, const bool forceRefresh)
	{
		return Enqueue(GatewayRequest::Kind::Scenery, std::to_string(sceneryId), "/scenery", forceRefresh);
	}

	std::shared_ptr<GatewayRequest> GatewayClient::FetchStats()
	{
		return Enqueue(GatewayRequest::Kind::Stats, std::string(), "/stats", true);
	}

	std::shared_ptr<GatewayRequest> GatewayClient::FetchReleases()
	{
		return Enqueue(GatewayRequest::Kind::Releases, std::string(), "/releases", true);
	}

	std::shared_ptr<const std::vector<GatewayAirport>> GatewayClient::GetAirports()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.airports;
	}

	int64_t GatewayClient::GetAirportsTimestamp()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.airportsTimestamp;
	}

	std::shared_ptr<GatewayRequest> GatewayClient::GetActiveRequest()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.active;
	}

	size_t GatewayClient::GetQueuedCount()
	{
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		return state.queue.size();
	}

	fs::path GatewayClient::GetCacheDirectory()
	{
		return CacheDirectory();
	}

	void GatewayClient::ClearCache()
	{
		std::error_code ec;
		fs::remove_all(CacheDirectory(), ec);
		State &state = GetState();
		std::scoped_lock lock(state.mutex);
		state.airports.reset();
		state.airportsTimestamp = 0;
	}

	// -------------------------------------------------------

	bool GatewayClient::UnpackScenery(const GatewaySceneryPack &pack, std::vector<ZipEntry> &outFiles, std::string *error)
	{
		outFiles.clear();
		std::vector<ZipEntry> master;
		if (!ReadZipArchive(pack.masterZip, master, error))
			return false;

		// WorldEditor uploads <ICAO>.dat, <ICAO>.txt (the DSF as text) and <ICAO>_Scenery_Pack.zip, which holds the
		// real scenery pack folder (Earth nav data/apt.dat, the binary DSF, README, COPYING).
		ZipEntry *nested = nullptr;
		for (ZipEntry &entry : master)
		{
			const std::string lower = ToLower(entry.name);
			if (lower.ends_with("_scenery_pack.zip") || (!nested && lower.ends_with(".zip")))
				nested = &entry;
		}

		if (nested)
		{
			std::vector<ZipEntry> inner;
			std::string innerError;
			if (!ReadZipArchive(nested->data, inner, &innerError))
			{
				if (error)
					*error = "Scenery pack zip inside the download: " + innerError;
				return false;
			}

			// Strip the single top-level folder WorldEditor wraps the pack in.
			std::string prefix;
			bool common = !inner.empty();
			for (const ZipEntry &entry : inner)
			{
				const size_t slash = entry.name.find('/');
				if (slash == std::string::npos)
				{
					common = false;
					break;
				}
				const std::string top = entry.name.substr(0, slash + 1);
				if (prefix.empty())
					prefix = top;
				else if (prefix != top)
				{
					common = false;
					break;
				}
			}

			for (ZipEntry &entry : inner)
			{
				if (common)
					entry.name.erase(0, prefix.size());
				if (entry.name.empty() || !SafeRelativeName(entry.name))
					continue;
				outFiles.push_back(std::move(entry));
			}
		}
		else
		{
			// Older uploads: the loose apt.dat becomes the pack; anything else is kept beside it for reference.
			for (ZipEntry &entry : master)
			{
				const std::string lower = ToLower(entry.name);
				if (!SafeRelativeName(entry.name))
					continue;
				if (lower.ends_with(".dat"))
					entry.name = "Earth nav data/apt.dat";
				else
					entry.name = "Gateway/" + entry.name;
				outFiles.push_back(std::move(entry));
			}
		}

		const bool hasAptDat = std::ranges::any_of(outFiles, [](const ZipEntry &entry) { return ToLower(entry.name) == "earth nav data/apt.dat"; });
		if (!hasAptDat)
		{
			if (error)
				*error = "The download holds no Earth nav data/apt.dat";
			return false;
		}
		return true;
	}

	bool GatewayClient::InstallScenery(const GatewaySceneryPack &pack, const fs::path &packageFolder, fs::path *aptDat, std::string *error)
	{
		std::vector<ZipEntry> files;
		if (!UnpackScenery(pack, files, error))
			return false;

		std::error_code ec;
		fs::create_directories(packageFolder, ec);
		if (ec)
		{
			if (error)
				*error = "Could not create " + packageFolder.string() + ": " + ec.message();
			return false;
		}

		for (const ZipEntry &entry : files)
		{
			// Names stay inside the pack: "../", absolute names and drive roots in a hostile archive are refused
			fs::path target;
			if (!VFS::ResolveInside(packageFolder, entry.name, target))
			{
				if (error)
					*error = "The Gateway archive names a file outside the pack: " + entry.name;
				return false;
			}
			const std::string_view bytes(reinterpret_cast<const char *>(entry.data.data()), entry.data.size());
			if (!VFS::WriteFileAtomic(target, bytes, error))
				return false;
		}

		if (aptDat)
			*aptDat = packageFolder / "Earth nav data" / "apt.dat";
		SEDX_CORE_INFO_TAG(LOG_TAG, "Installed Gateway scenery {} ({} files) to {}", pack.info.sceneryId, files.size(), packageFolder.string());
		return true;
	}
}

// -------------------------------------------------------
