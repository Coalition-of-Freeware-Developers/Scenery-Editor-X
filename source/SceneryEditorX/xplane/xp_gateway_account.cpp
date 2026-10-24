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
 * xp_gateway_account.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include "xp_gateway_account.h"
#include <nlohmann/json.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		using Json = nlohmann::json;

		constexpr int SETTINGS_VERSION = 1;
	}

	// -------------------------------------------------------

	bool ParseGatewayAccountSettings(const std::string_view json, GatewayAccountSettings &out, std::string *error)
	{
		out = {};
		try
		{
			const Json root = Json::parse(json.begin(), json.end());
			if (!root.is_object())
			{
				if (error)
					*error = "The Gateway account settings aren't a JSON object";
				return false;
			}
			if (const auto it = root.find("userName"); it != root.end() && it->is_string())
				out.userName = it->get<std::string>();
			if (const auto it = root.find("rememberPassword"); it != root.end() && it->is_boolean())
				out.rememberPassword = it->get<bool>();
			if (const auto it = root.find("serverUrl"); it != root.end() && it->is_string())
				out.serverUrl = it->get<std::string>();
			// A "password" key (hand edited, or from some other tool) is never read: passwords live in the credential store
			return true;
		}
		catch (const std::exception &e)
		{
			if (error)
				*error = std::string("Unreadable Gateway account settings: ") + e.what();
			return false;
		}
	}

	std::string WriteGatewayAccountSettings(const GatewayAccountSettings &settings)
	{
		Json root;
		root["version"] = SETTINGS_VERSION;
		root["userName"] = settings.userName;
		root["rememberPassword"] = settings.rememberPassword;
		if (!settings.serverUrl.empty())
			root["serverUrl"] = settings.serverUrl;
		return root.dump(2);
	}

	std::string GetGatewayCredentialAccount(const GatewayServer &server, const std::string_view userName)
	{
		std::string account(userName);
		if (!server.official)
			account += "@" + server.GetKey();
		return account;
	}

	bool ValidateGatewayUserName(const std::string_view userName, std::string *error)
	{
		const auto fail = [error](const char *message)
		{
			if (error)
				*error = message;
			return false;
		};
		if (userName.empty())
			return fail("Enter your Gateway user name");
		if (userName.size() > 64)
			return fail("The user name is too long");
		if (userName.front() == ' ' || userName.back() == ' ')
			return fail("The user name can't start or end with a space");
		for (const char c : userName)
		{
			if (static_cast<unsigned char>(c) < 32 || c == 127)
				return fail("The user name has a character that can't be typed");
		}
		return true;
	}
}

// -------------------------------------------------------
