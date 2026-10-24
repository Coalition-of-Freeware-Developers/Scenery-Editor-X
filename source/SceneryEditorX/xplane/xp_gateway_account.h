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
 * xp_gateway_account.h
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_gateway_server.h"
#include <string>
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * The user's Scenery Gateway artist account. The user name, the server and whether to remember the password go
	 * in the settings file; the password itself only ever goes to the operating system's credential store
	 * (core/platform/credential_store.h) under GATEWAY_CREDENTIAL_SERVICE. Standard library + nlohmann/json only.
	 *
	 * The Gateway API has no sign-in, token or session: the artist name and password travel inside each upload
	 * (PUT /apiv1/scenery, see xp_gateway_submission.h), and the upload is the only call that checks them.
	 */

	/* @brief The credential store service the Gateway password is saved under. */
	inline constexpr const char *GATEWAY_CREDENTIAL_SERVICE = "Scenery Editor X - X-Plane Scenery Gateway";

	/* @brief What's kept in user:/gateway_account.json. Never holds the password. */
	struct GatewayAccountSettings
	{
		std::string userName;			// The Gateway artist name the user signs in with
		bool rememberPassword = true;	// Keep the password in the credential store between sessions
		std::string serverUrl;			// Empty: the public Gateway; otherwise a test server's API root

		[[nodiscard]] bool operator==(const GatewayAccountSettings &other) const = default;
	};

	/* @brief Reads the settings file (unknown keys and any "password" key are ignored). */
	bool ParseGatewayAccountSettings(std::string_view json, GatewayAccountSettings &out, std::string *error = nullptr);

	/* @brief The settings file's text. */
	std::string WriteGatewayAccountSettings(const GatewayAccountSettings &settings);

	/**
	 * @brief The credential store account for a user on a server: the user name for the public Gateway,
	 * "name@host:port" for any other, so passwords for a test server never replace the real one.
	 */
	std::string GetGatewayCredentialAccount(const GatewayServer &server, std::string_view userName);

	/* @brief A user name the Gateway could have issued: 1-64 printable characters, no leading/trailing spaces. */
	bool ValidateGatewayUserName(std::string_view userName, std::string *error = nullptr);
}

// -------------------------------------------------------
