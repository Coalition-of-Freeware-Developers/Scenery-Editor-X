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
 * xp_gateway.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_gateway_server.h"
#include "xp_gateway_submission.h"
#include "xp_gateway_types.h"
#include "xp_zip.h"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/* @brief Where a Gateway request is. */
	enum class GatewayRequestState : uint8_t
	{
		Queued,
		Running,
		Done,
		Failed,
		Cancelled
	};

	const char *GetGatewayRequestStateName(GatewayRequestState state);

	/**
	 * @class GatewayRequest
	 * @brief One call to the Scenery Gateway, answered on a worker thread. Poll GetState() from the UI; the result
	 * members are filled in before the state turns Done and never change afterwards.
	 */
	class GatewayRequest
	{
	public:
		enum class Kind : uint8_t
		{
			Airports,	// GET /airports (the whole list, ~20 MB; cached on disk for a day)
			Airport,	// GET /airport/{code}
			Scenery,	// GET /scenery/{id} (cached on disk: approved packs don't change)
			Stats,		// GET /stats
			Releases,	// GET /releases
			Upload		// PUT /scenery: a submission with the artist's user name and password
		};

		[[nodiscard]] Kind GetKind() const { return m_Kind; }
		[[nodiscard]] const std::string &GetKey() const { return m_Key; }			// Airport code or scenery id, "" otherwise
		[[nodiscard]] GatewayRequestState GetState() const { return m_State.load(std::memory_order_acquire); }
		[[nodiscard]] bool IsFinished() const;
		[[nodiscard]] bool Succeeded() const { return GetState() == GatewayRequestState::Done; }
		[[nodiscard]] float GetProgress() const { return m_Progress.load(); }		// 0..1, or -1 when the size isn't known yet
		[[nodiscard]] uint64_t GetBytesReceived() const { return m_Bytes.load(); }
		[[nodiscard]] bool IsFromCache() const { return m_FromCache.load(); }
		[[nodiscard]] double GetSeconds() const;									// Since it was queued
		[[nodiscard]] const std::string &GetError() const { return m_Error; }		// After Failed

		/* @brief Stops the download; the state becomes Cancelled. */
		void Cancel() { m_Cancel.store(true); }

		// Results, valid once Succeeded()
		std::shared_ptr<const std::vector<GatewayAirport>> airports;
		std::shared_ptr<const GatewayAirportDetails> airport;
		std::shared_ptr<const GatewaySceneryPack> scenery;
		std::shared_ptr<const GatewayStats> stats;
		std::shared_ptr<const std::vector<GatewayRelease>> releases;
		std::shared_ptr<const GatewayUploadResult> upload;		// Set whenever an upload got as far as the server (or tried to)

		// Worker-side state: written by the GatewayClient worker, read through the getters above.
		friend class GatewayClient;

		Kind m_Kind = Kind::Stats;
		std::string m_Key;
		std::string m_Path;					// URL path under the server's API root
		GatewayServer m_Server;				// Where it goes, fixed when it was queued
		GatewayTransport m_Transport;		// Empty: libcurl
		bool m_ForceRefresh = false;
		bool m_CacheOnly = false;			// Never touch the network (startup load of the cached airport list)
		std::shared_ptr<const GatewaySubmission> m_Submission;	// Upload only
		std::string m_UserName;				// Upload only
		std::string m_Password;				// Upload only; wiped once sent
		bool m_LiveUploadsAllowed = false;	// Copied when queued
		std::atomic<GatewayRequestState> m_State{ GatewayRequestState::Queued };
		std::atomic<float> m_Progress{ -1.0f };
		std::atomic<uint64_t> m_Bytes{ 0 };
		std::atomic<bool> m_Cancel{ false };
		std::atomic<bool> m_FromCache{ false };
		std::string m_Error;
		std::chrono::steady_clock::time_point m_Queued = std::chrono::steady_clock::now();
	};

	/**
	 * @class GatewayClient
	 * @brief The editor's connection to the X-Plane Scenery Gateway (https://gateway.x-plane.com/api), or to a test
	 * server standing in for it (SetServer). Requests run one at a time on a worker thread (the Gateway asks not to be
	 * hammered) and responses that don't change are cached under <app cache>/gateway (other servers: gateway-servers/
	 * <host_port>). Main thread only.
	 */
	class GatewayClient
	{
	public:
		/* @brief Starts the worker and loads the cached airport list, if there is one (no network). */
		static void Init();

		/* @brief Cancels what's running and stops the worker. */
		static void Shutdown();

		/**
		 * @brief The list of every Gateway airport. Served from the disk cache when it's less than a day old unless
		 * forceRefresh is set. GetAirports() returns the result afterwards.
		 */
		static std::shared_ptr<GatewayRequest> FetchAirports(bool forceRefresh = false);

		/* @brief An airport with its submissions. */
		static std::shared_ptr<GatewayRequest> FetchAirport(const std::string &code);

		/* @brief A submission with its files (apt.dat and DSF). */
		static std::shared_ptr<GatewayRequest> FetchScenery(int sceneryId, bool forceRefresh = false);

		static std::shared_ptr<GatewayRequest> FetchStats();
		static std::shared_ptr<GatewayRequest> FetchReleases();

		/**
		 * @brief Uploads a submission (a new airport, or an update when parentId is set). The request finishes Done
		 * with `upload` saying what the server answered, or Failed when it was never sent (see CanUploadTo).
		 * The password is wiped from the request once it's sent and never logged.
		 */
		static std::shared_ptr<GatewayRequest> UploadScenery(std::shared_ptr<const GatewaySubmission> submission, const std::string &userName, const std::string &password);

		/* @brief Whether uploads may go to the public Gateway (false: only test servers take them). */
		static void SetLiveUploadsAllowed(bool allowed);
		static bool AreLiveUploadsAllowed();

		/* @brief The airport list from the last successful FetchAirports() (or the disk cache); null before that. */
		static std::shared_ptr<const std::vector<GatewayAirport>> GetAirports();

		/* @brief When the airport list was downloaded, as seconds since the Unix epoch (0 when there is none). */
		static int64_t GetAirportsTimestamp();

		/* @brief The request being worked on, or null when idle. */
		static std::shared_ptr<GatewayRequest> GetActiveRequest();
		static size_t GetQueuedCount();

		/* @brief The current server's cache folder. */
		static std::filesystem::path GetCacheDirectory();

		/**
		 * @brief Talks to another server from now on (requests already queued keep theirs). The airport list is
		 * reloaded from the new server's own cache.
		 */
		static void SetServer(const GatewayServer &server);
		static GatewayServer GetServer();

		/* @brief Replaces libcurl for new requests (tests, offline runs); an empty function restores it. */
		static void SetTransport(GatewayTransport transport);

		/* @brief Deletes the cached responses (the next requests go to the network). */
		static void ClearCache();

		/**
		 * @brief Turns a downloaded submission into the files of a scenery pack folder: "Earth nav data/apt.dat", the
		 * DSF overlays under "Earth nav data/+NN-MMM/" and whatever else the artist shipped (README, COPYING).
		 */
		static bool UnpackScenery(const GatewaySceneryPack &pack, std::vector<ZipEntry> &outFiles, std::string *error = nullptr);

		/**
		 * @brief Writes a submission to a scenery pack folder (created if needed; existing files are replaced).
		 * @param aptDat Receives the path of the written apt.dat (optional).
		 */
		static bool InstallScenery(const GatewaySceneryPack &pack, const std::filesystem::path &packageFolder, std::filesystem::path *aptDat = nullptr, std::string *error = nullptr);
	};

}

// -------------------------------------------------------
