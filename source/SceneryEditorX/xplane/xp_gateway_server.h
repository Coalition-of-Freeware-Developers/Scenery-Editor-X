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
 * xp_gateway_server.h
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Which Scenery Gateway the editor talks to, and the HTTP layer it talks through. The public Gateway is the
	 * default; a test server (scripts/gateway_mock/gateway_mock.py on this machine) can stand in for it so sign-in
	 * and uploads are exercised without reaching gateway.x-plane.com. Standard library only: unit tested.
	 */

	/**
	 * @struct GatewayServer
	 * @brief A Gateway API root such as "https://gateway.x-plane.com/apiv1", made by ParseGatewayServer().
	 */
	struct GatewayServer
	{
		std::string apiUrl;			// "<scheme>://<host>[:port]/<path>", no trailing slash
		std::string scheme;			// "https" / "http"
		std::string host;			// Lower case, no brackets for IPv6
		int port = 0;				// 0 when the URL names none
		bool official = false;		// gateway.x-plane.com (the public Gateway)
		bool local = false;			// localhost / 127.x / ::1: a test server on this machine

		/* @brief "gateway.x-plane.com" or "127.0.0.1:8780": names the server in the UI, keychain and cache. */
		[[nodiscard]] std::string GetKey() const;

		/* @brief The site root without the API path ("https://gateway.x-plane.com"), for web pages. */
		[[nodiscard]] std::string GetSiteUrl() const;

		[[nodiscard]] bool operator==(const GatewayServer &other) const { return apiUrl == other.apiUrl; }
	};

	/* @brief The public Gateway. */
	GatewayServer GetOfficialGatewayServer();

	/**
	 * @brief Reads a server URL the user typed: "http://127.0.0.1:8780" gets the "/apiv1" path, trailing slashes go.
	 * Only http and https; plain http only for a server on this machine, since a password would cross the network
	 * readable. False with a message when the URL can't be used.
	 */
	bool ParseGatewayServer(std::string_view text, GatewayServer &out, std::string *error = nullptr);

	// -------------------------------------------------------

	/* @brief One HTTP exchange with the Gateway, as the client hands it to a GatewayTransport. */
	struct GatewayHttpRequest
	{
		std::string method = "GET";							// "GET" / "POST"
		std::string url;
		std::vector<std::pair<std::string, std::string>> headers;
		std::string body;
		bool sendsCredentials = false;						// The body or headers carry the user's password
	};

	struct GatewayHttpResponse
	{
		bool completed = false;		// An HTTP answer came back (any status); false: no connection, cancelled, timed out
		long status = 0;			// HTTP status
		std::string body;
		std::string error;			// When !completed: what went wrong
	};

	/* @brief Progress and cancel hooks a transport calls while it moves bytes. */
	struct GatewayTransferHooks
	{
		std::function<void(uint64_t bytes)> received;					// Body bytes as they arrive
		std::function<void(double fraction)> progress;					// 0..1 when the size is known
		std::function<bool()> cancelled;								// True: abort the transfer
	};

	/* @brief Moves one request to a server and back. The default uses libcurl; tests and offline runs swap it. */
	using GatewayTransport = std::function<GatewayHttpResponse(const GatewayHttpRequest &request, const GatewayTransferHooks &hooks)>;

	/* @brief A transport that never connects: every request completes with no answer and the given reason. */
	GatewayTransport MakeOfflineGatewayTransport(std::string reason);
}

// -------------------------------------------------------
