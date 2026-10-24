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
 * xp_gateway_server.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include "xp_gateway_server.h"
#include "xp_gateway_types.h"
#include <cctype>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		std::string Lower(const std::string_view text)
		{
			std::string out(text);
			for (char &c : out)
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return out;
		}

		std::string_view Trim(std::string_view text)
		{
			while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
				text.remove_prefix(1);
			while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
				text.remove_suffix(1);
			return text;
		}

		bool Fail(std::string *error, const char *message)
		{
			if (error)
				*error = message;
			return false;
		}

		bool IsLocalHost(const std::string &host)
		{
			if (host == "localhost" || host.ends_with(".localhost") || host == "::1")
				return true;
			// 127.0.0.0/8
			if (!host.starts_with("127."))
				return false;
			int dots = 0;
			for (const char c : host)
			{
				if (c == '.')
					++dots;
				else if (!std::isdigit(static_cast<unsigned char>(c)))
					return false;
			}
			return dots == 3;
		}

		bool ValidHostCharacters(const std::string &host, const bool ipv6)
		{
			for (const char c : host)
			{
				const unsigned char u = static_cast<unsigned char>(c);
				if (std::isalnum(u) || c == '-' || c == '.' || (ipv6 && c == ':'))
					continue;
				return false;
			}
			return !host.empty();
		}
	}

	// -------------------------------------------------------

	std::string GatewayServer::GetKey() const
	{
		std::string key = host.find(':') != std::string::npos ? "[" + host + "]" : host;
		if (port != 0)
			key += ":" + std::to_string(port);
		return key;
	}

	std::string GatewayServer::GetSiteUrl() const
	{
		return scheme + "://" + GetKey();
	}

	GatewayServer GetOfficialGatewayServer()
	{
		GatewayServer server;
		ParseGatewayServer(GATEWAY_API_URL, server);
		return server;
	}

	bool ParseGatewayServer(const std::string_view text, GatewayServer &out, std::string *error)
	{
		out = {};
		std::string_view url = Trim(text);
		if (url.empty())
			return Fail(error, "Enter the server's address, e.g. http://127.0.0.1:8780");

		// Scheme
		const size_t schemeEnd = url.find("://");
		if (schemeEnd == std::string_view::npos)
			return Fail(error, "The address needs http:// or https:// in front");
		GatewayServer server;
		server.scheme = Lower(url.substr(0, schemeEnd));
		if (server.scheme != "https" && server.scheme != "http")
			return Fail(error, "Only http:// and https:// addresses can be used");
		url.remove_prefix(schemeEnd + 3);

		// Authority: no user info (a password in a URL ends up in logs)
		const size_t pathStart = url.find_first_of("/?#");
		std::string_view authority = url.substr(0, pathStart);
		std::string_view path = pathStart == std::string_view::npos ? std::string_view() : url.substr(pathStart);
		if (authority.find('@') != std::string_view::npos)
			return Fail(error, "Leave the user name and password out of the address");
		if (path.find_first_of("?#") != std::string_view::npos)
			return Fail(error, "The address can't have a query (?) or fragment (#)");

		std::string_view portText;
		bool ipv6 = false;
		if (authority.starts_with('['))
		{
			const size_t close = authority.find(']');
			if (close == std::string_view::npos)
				return Fail(error, "The IPv6 address is missing its closing ]");
			server.host = Lower(authority.substr(1, close - 1));
			ipv6 = true;
			const std::string_view rest = authority.substr(close + 1);
			if (!rest.empty())
			{
				if (rest.front() != ':')
					return Fail(error, "Unexpected text after the IPv6 address");
				portText = rest.substr(1);
			}
		}
		else
		{
			const size_t colon = authority.rfind(':');
			server.host = Lower(authority.substr(0, colon));
			if (colon != std::string_view::npos)
				portText = authority.substr(colon + 1);
		}
		if (!ValidHostCharacters(server.host, ipv6))
			return Fail(error, "The server name isn't valid");

		if (!portText.empty())
		{
			int port = 0;
			for (const char c : portText)
			{
				if (!std::isdigit(static_cast<unsigned char>(c)))
					return Fail(error, "The port must be a number");
				port = port * 10 + (c - '0');
				if (port > 65535)
					return Fail(error, "The port must be 1 to 65535");
			}
			if (port == 0)
				return Fail(error, "The port must be 1 to 65535");
			// The scheme's own port says nothing extra
			if (!((server.scheme == "https" && port == 443) || (server.scheme == "http" && port == 80)))
				server.port = port;
		}

		server.official = server.host == "gateway.x-plane.com";
		server.local = IsLocalHost(server.host);
		if (server.scheme == "http" && !server.local)
			return Fail(error, "Plain http:// is only allowed for a server on this computer (the password would be readable on the network); use https://");

		// Path: the API root ("/apiv1" when none is given), without trailing slashes
		std::string cleanPath(path);
		while (!cleanPath.empty() && cleanPath.back() == '/')
			cleanPath.pop_back();
		if (cleanPath.empty())
			cleanPath = "/apiv1";

		server.apiUrl = server.GetSiteUrl() + cleanPath;
		out = std::move(server);
		return true;
	}

	GatewayTransport MakeOfflineGatewayTransport(std::string reason)
	{
		return [reason = std::move(reason)](const GatewayHttpRequest &, const GatewayTransferHooks &)
		{
			GatewayHttpResponse response;
			response.completed = false;
			response.error = reason;
			return response;
		};
	}
}

// -------------------------------------------------------
