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
 * editor_gateway.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include "editor_gateway.h"
#include "editor_theme.h"
#include "editor_ui.h"
#include <cstdlib>
#include <memory>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/platform/credential_store.h>
#include <SceneryEditorX/core/platform/platform_context.h>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/xplane/xp_gateway.h>
#include <SceneryEditorX/xplane/xp_gateway_account.h>
#include <SceneryEditorX/xplane/xp_text.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorGateway
{
	namespace fs = std::filesystem;
	using namespace XPlane;

	namespace
	{
		constexpr const char *LOG_TAG = "Gateway";
		constexpr const char *SETTINGS_FILE = "gateway_account.json";
		constexpr const char *PASSWORD_ENV = "SEDX_GATEWAY_PASSWORD";
		constexpr const char *URL_ENV = "SEDX_GATEWAY_URL";

		/**
		 * What we know about the account this session. The Gateway has no sign-in call: the name and password are only
		 * checked by an upload, so "Accepted" / "Rejected" come from the last upload's answer.
		 */
		enum class Account : uint8_t
		{
			None,		// No user name / password
			Entered,	// Have both; no upload has used them yet this session
			Checking,	// A (test) upload is on its way
			Accepted,	// The server took an upload with them
			Rejected	// The server refused an upload as unauthorized (wrong name or password, or X-Plane build)
		};

		struct State
		{
			bool initialized = false;
			GatewayAccountSettings settings;			// What's saved (never the password)
			GatewayServer server;						// In use
			bool serverFromCommandLine = false;			// --gateway-url / SEDX_GATEWAY_URL: this session only
			bool offline = false;						// --gateway-offline
			std::unique_ptr<Credentials::CredentialStore> store;
			bool memoryStoreForced = false;				// --gateway-credentials memory

			std::string userName;						// The account in use (may differ from the form while editing)
			std::string password;						// In memory for the session only
			Account account = Account::None;
			std::string accountMessage;					// Why it was rejected, or what a check found
			bool passwordRemembered = false;			// A password is in the store for userName on this server
			std::shared_ptr<GatewayRequest> check;		// Test upload in flight
			std::string startupTestUpload;				// --gateway-test-upload <name>

			// The preferences form
			char userBuffer[128] = {};
			char passwordBuffer[256] = {};
			char serverBuffer[512] = {};
			bool useTestServer = false;
			std::string formError;
			std::string formNotice;
		};

		State &GetState()
		{
			static State state;
			return state;
		}

		ImVec4 Color(const ImVec4 &palette)
		{
			return EditorTheme::ToStyle(palette);
		}

		fs::path SettingsPath()
		{
			return IO::FileSystem::GetPersistentStoragePath() / SETTINGS_FILE;
		}

		void LoadSettings(State &state)
		{
			std::string text;
			if (!ReadTextFile(SettingsPath(), text))
				return;
			std::string error;
			if (!ParseGatewayAccountSettings(text, state.settings, &error))
				EDITOR_WARN_TAG(LOG_TAG, "Ignoring {}: {}", SettingsPath().string(), error);
		}

		void SaveSettings(const State &state)
		{
			std::string error;
			if (!VFS::WriteFileAtomic(SettingsPath(), WriteGatewayAccountSettings(state.settings), &error))
				EDITOR_WARN_TAG(LOG_TAG, "Could not save the Gateway account settings: {}", error);
		}

		void Copy(char *buffer, const size_t size, const std::string &text)
		{
			snprintf(buffer, size, "%s", text.c_str());
		}

		std::string CredentialAccount(const State &state, const std::string &userName)
		{
			return GetGatewayCredentialAccount(state.server, userName);
		}

		/* @brief The settings' server, or the public Gateway when it's empty or unusable. */
		GatewayServer SettingsServer(const State &state)
		{
			if (state.settings.serverUrl.empty())
				return GetOfficialGatewayServer();
			GatewayServer server;
			std::string error;
			if (ParseGatewayServer(state.settings.serverUrl, server, &error))
				return server;
			EDITOR_WARN_TAG(LOG_TAG, "Saved Gateway server '{}' can't be used ({}); using the public Gateway", state.settings.serverUrl, error);
			return GetOfficialGatewayServer();
		}

		void CancelCheck(State &state)
		{
			if (state.check)
				state.check->Cancel();
			state.check.reset();
			if (state.account == Account::Checking)
				state.account = state.password.empty() ? Account::None : Account::Entered;
		}

		void ForgetSessionPassword(State &state)
		{
			CancelCheck(state);
			Credentials::SecureClear(state.password);
			Credentials::SecureClear(state.passwordBuffer, sizeof(state.passwordBuffer));
			state.account = Account::None;
			state.accountMessage.clear();
		}

		/* @brief Reads the remembered password for the settings' user name on the current server, if any. */
		void LoadRememberedPassword(State &state)
		{
			state.passwordRemembered = false;
			state.userName = state.settings.userName;
			if (state.userName.empty() || !state.settings.rememberPassword)
				return;

			std::string error;
			const Credentials::Status status = state.store->Load(GATEWAY_CREDENTIAL_SERVICE, CredentialAccount(state, state.userName), state.password, &error);
			if (status == Credentials::Status::Ok)
			{
				state.passwordRemembered = true;
				state.account = Account::Entered;
				EDITOR_INFO_TAG(LOG_TAG, "Gateway account {} on {}: password from {}", state.userName, state.server.GetKey(), state.store->GetName());
			}
			else if (status == Credentials::Status::Failed)
				EDITOR_WARN_TAG(LOG_TAG, "Could not read the Gateway password from {}: {}", state.store->GetName(), error);
		}

		/* @brief Switches servers: the session's password belonged to the old one. */
		void ApplyServer(State &state, const GatewayServer &server)
		{
			if (server == state.server)
				return;
			ForgetSessionPassword(state);
			state.server = server;
			GatewayClient::SetServer(server);
			LoadRememberedPassword(state);
		}

		/* @brief Checks the account the only way the Gateway allows: uploads a made-up airport. Test servers only. */
		void StartTestUpload(State &state)
		{
			CancelCheck(state);
			if (state.userName.empty() || state.password.empty())
				return;
			if (state.server.official)
			{
				state.accountMessage = "test uploads only go to a test server";
				return;
			}
			state.check = GatewayClient::UploadScenery(std::make_shared<GatewaySubmission>(MakeGatewayTestSubmission()), state.userName, state.password);
			state.account = Account::Checking;
			state.accountMessage.clear();
		}

		/* @brief What an upload's answer says about the account. */
		void ApplyUploadResult(State &state, const GatewayUploadResult &result)
		{
			using Outcome = GatewayUploadResult::Outcome;
			switch (result.outcome)
			{
				case Outcome::Uploaded:
				case Outcome::UnknownAirport:	// Got past the credentials
				case Outcome::Rejected:
				case Outcome::Unconfirmed:
					state.account = result.outcome == Outcome::Uploaded ? Account::Accepted : Account::Entered;
					break;
				case Outcome::Unauthorized:
					state.account = Account::Rejected;
					break;
				case Outcome::NoAnswer:
					state.account = state.password.empty() ? Account::None : Account::Entered;
					break;
			}
			state.accountMessage = result.message;
		}

		void ReadCommandLine(State &state)
		{
			const PlatformContext *platform = Application::Get().GetPlatformContext();
			if (!platform)
				return;
			const std::vector<std::string> &args = platform->GetCommandLineArgs();
			for (size_t i = 0; i < args.size(); ++i)
			{
				const bool hasValue = i + 1 < args.size();
				if (args[i] == "--gateway-url" && hasValue)
				{
					GatewayServer server;
					std::string error;
					if (ParseGatewayServer(args[++i], server, &error))
					{
						state.server = server;
						state.serverFromCommandLine = true;
					}
					else
						EDITOR_ERROR_TAG(LOG_TAG, "--gateway-url {}: {}", args[i], error);
				}
				else if (args[i] == "--gateway-offline")
					state.offline = true;
				else if (args[i] == "--gateway-credentials" && hasValue)
				{
					const std::string &mode = args[++i];
					if (mode == "memory")
						state.memoryStoreForced = true;
					else if (mode != "system")
						EDITOR_WARN_TAG(LOG_TAG, "--gateway-credentials {}: expected 'memory' or 'system'", mode);
				}
				else if (args[i] == "--gateway-test-upload" && hasValue)
					state.startupTestUpload = args[++i];
				else if (args[i] == "--gateway-preferences")
					EditorPreferencesWindow::Open("Scenery Gateway");
			}
		}

		// -------------------------------------------------------

		/* @brief The banner every account screen shows while the server isn't the public Gateway. */
		void DrawServerBanner(const State &state)
		{
			if (state.offline)
			{
				ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s  OFFLINE: Gateway requests are blocked (--gateway-offline)", SEDX_ICON_EXCLAMATION_TRIANGLE);
				return;
			}
			if (state.server.official)
				return;
			ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s  TEST SERVER: %s", SEDX_ICON_EXCLAMATION_TRIANGLE, state.server.apiUrl.c_str());
			EditorUI::ItemTooltip(state.server.local
				? "A Gateway test server on this computer. Nothing you do here reaches the public X-Plane Scenery Gateway."
				: "Not the public X-Plane Scenery Gateway.");
		}

		const char *AccountText(const Account account)
		{
			switch (account)
			{
				case Account::None:		return "Not signed in";
				case Account::Entered:	return "Ready: the Gateway checks the password on your first upload";
				case Account::Checking:	return "Uploading a test airport...";
				case Account::Accepted:	return "Signed in: the server accepted an upload";
				case Account::Rejected:	return "The server refused the upload: check the user name and password";
			}
			return "";
		}

		ImVec4 AccountColor(const Account account)
		{
			switch (account)
			{
				case Account::Accepted:	return Color(EditorTheme::Palette::Accent);
				case Account::Rejected:	return Color(EditorTheme::Palette::Error);
				case Account::Checking:	return Color(EditorTheme::Palette::Warning);
				default:				return Color(EditorTheme::Palette::TextLabel);
			}
		}

		/* @brief Saves the form: user name, remember choice, and the password (to the store when remembered). */
		void ApplyForm(State &state, const bool testUpload)
		{
			state.formError.clear();
			state.formNotice.clear();

			const std::string userName(state.userBuffer);
			std::string error;
			if (!ValidateGatewayUserName(userName, &error))
			{
				state.formError = error;
				return;
			}

			std::string password(state.passwordBuffer);
			Credentials::SecureClear(state.passwordBuffer, sizeof(state.passwordBuffer));
			const bool userChanged = userName != state.userName;

			// A different user: the old password doesn't belong to them
			if (userChanged)
			{
				ForgetSessionPassword(state);
				state.passwordRemembered = false;
			}
			state.userName = userName;

			if (!password.empty())
			{
				Credentials::SecureClear(state.password);
				state.password = std::move(password);
				state.account = Account::Entered;
			}
			Credentials::SecureClear(password);

			if (userChanged && state.password.empty())
			{
				// Maybe the store already knows this user's password
				std::string loaded;
				if (state.settings.rememberPassword && state.store->Load(GATEWAY_CREDENTIAL_SERVICE, CredentialAccount(state, userName), loaded) == Credentials::Status::Ok)
				{
					state.password = std::move(loaded);
					state.passwordRemembered = true;
					state.account = Account::Entered;
				}
				Credentials::SecureClear(loaded);
			}

			// Remember (or don't) in the credential store
			const std::string account = CredentialAccount(state, userName);
			if (state.settings.rememberPassword && !state.password.empty() && state.store->IsAvailable())
			{
				if (state.store->Store(GATEWAY_CREDENTIAL_SERVICE, account, state.password, &error) == Credentials::Status::Ok)
				{
					state.passwordRemembered = true;
					state.formNotice = std::string("Password saved in ") + state.store->GetName();
				}
				else
				{
					state.passwordRemembered = false;
					state.formError = error;
				}
			}
			else if (!state.settings.rememberPassword)
			{
				const Credentials::Status erased = state.store->Erase(GATEWAY_CREDENTIAL_SERVICE, account, &error);
				state.passwordRemembered = false;
				if (erased == Credentials::Status::Failed)
					state.formError = error;
				else if (!state.password.empty())
					state.formNotice = "Password kept for this session only";
			}

			state.settings.userName = userName;
			SaveSettings(state);

			if (testUpload)
				StartTestUpload(state);
		}

		void DrawServerSection(State &state)
		{
			ImGui::SeparatorText("Server");
			if (state.serverFromCommandLine)
			{
				ImGui::TextWrapped("This session uses %s (set with --gateway-url or SEDX_GATEWAY_URL; not saved).", state.server.apiUrl.c_str());
				return;
			}

			bool changed = false;
			if (ImGui::RadioButton("X-Plane Scenery Gateway (gateway.x-plane.com)", !state.useTestServer))
			{
				state.useTestServer = false;
				changed = true;
			}
			if (ImGui::RadioButton("Test server", state.useTestServer))
			{
				state.useTestServer = true;
				if (!state.serverBuffer[0])
					Copy(state.serverBuffer, sizeof(state.serverBuffer), "http://127.0.0.1:8780/apiv1");
			}
			EditorUI::ItemTooltip("A Gateway stand-in for trying uploads safely, e.g. scripts/gateway_mock/gateway_mock.py on this computer");

			if (state.useTestServer)
			{
				ImGui::SetNextItemWidth(-120.0f * ImGui::GetStyle().FontScaleDpi);
				ImGui::InputTextWithHint("##GatewayServerUrl", "http://127.0.0.1:8780/apiv1", state.serverBuffer, sizeof(state.serverBuffer));
				ImGui::SameLine();
				if (ImGui::Button("Use Server"))
					changed = true;
			}
			ImGui::TextDisabled("In use: %s", state.server.apiUrl.c_str());

			if (changed)
			{
				state.formError.clear();
				GatewayServer server = GetOfficialGatewayServer();
				std::string error;
				if (state.useTestServer && !ParseGatewayServer(state.serverBuffer, server, &error))
					state.formError = error;
				else
				{
					state.settings.serverUrl = server.official ? std::string() : server.apiUrl;
					SaveSettings(state);
					ApplyServer(state, server);
					Copy(state.userBuffer, sizeof(state.userBuffer), state.userName);
					state.formNotice = "Now using " + server.apiUrl;
				}
			}
		}

		void DrawAccountSection(State &state)
		{
			ImGui::SeparatorText("Artist account");
			const float dpi = ImGui::GetStyle().FontScaleDpi;

			if (EditorUI::BeginPropertyGrid("##GatewayAccount", 110.0f))
			{
				EditorUI::PropertyLabel("User name");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##GatewayUser", "Your Gateway user name", state.userBuffer, sizeof(state.userBuffer));

				EditorUI::PropertyLabel("Password");
				ImGui::SetNextItemWidth(-FLT_MIN);
				const char *hint = state.password.empty() ? "Password" : "(unchanged - type to replace)";
				const bool enter = ImGui::InputTextWithHint("##GatewayPassword", hint, state.passwordBuffer, sizeof(state.passwordBuffer),
					ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
				if (enter)
					ApplyForm(state, false);

				EditorUI::PropertyLabel("Remember");
				bool remember = state.settings.rememberPassword;
				const bool storeAvailable = state.store->IsAvailable() && state.store->IsPersistent();
				ImGui::BeginDisabled(!storeAvailable);
				const std::string rememberLabel = storeAvailable ? std::string("Save the password in ") + state.store->GetName()
					: std::string("Kept for this session only");
				if (ImGui::Checkbox(rememberLabel.c_str(), &remember))
				{
					state.settings.rememberPassword = remember;
					ApplyForm(state, false);
				}
				ImGui::EndDisabled();
				if (!storeAvailable)
					EditorUI::ItemTooltip(state.memoryStoreForced
						? "--gateway-credentials memory: passwords stay in memory for this session"
						: "This system has no secure credential store (install libsecret / a keyring); the password is kept for this session only");
				else
					EditorUI::ItemTooltip("Encrypted and kept for your user account by the operating system. Scenery Editor X never writes it to its own files.");

				EditorUI::PropertyLabel("Status");
				ImGui::TextColored(AccountColor(state.account), "%s", AccountText(state.account));
				if (!state.accountMessage.empty())
				{
					ImGui::SameLine();
					ImGui::TextDisabled("(%s)", state.accountMessage.c_str());
				}

				EditorUI::EndPropertyGrid();
			}

			// Buttons
			const bool hasInput = state.userBuffer[0] != '\0';
			ImGui::BeginDisabled(!hasInput);
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_SIGN_IN, "Save").c_str(), ImVec2(100.0f * dpi, 0.0f)))
				ApplyForm(state, false);
			ImGui::EndDisabled();
			EditorUI::ItemTooltip("Use this account (and save the password, if remembered). The Gateway has no separate sign-in: "
				"it checks the user name and password each time you upload.");
			ImGui::SameLine();
			if (!state.server.official)
			{
				// The one way to check an account: an upload. Only test servers ever get this one.
				ImGui::BeginDisabled(!hasInput || state.offline || state.account == Account::Checking || (state.password.empty() && !state.passwordBuffer[0]));
				if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_CLOUD_UPLOAD, "Send Test Upload").c_str(), ImVec2(160.0f * dpi, 0.0f)))
					ApplyForm(state, true);
				ImGui::EndDisabled();
				EditorUI::ItemTooltip("Upload a made-up airport (XSDX) to the test server with this account, to see that sign-in and uploads work");
				ImGui::SameLine();
			}
			ImGui::BeginDisabled(state.password.empty() && !state.passwordRemembered);
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_SIGN_OUT, "Sign Out").c_str(), ImVec2(110.0f * dpi, 0.0f)))
			{
				SignOut();
				state.formNotice = "Signed out; the saved password was removed";
			}
			ImGui::EndDisabled();
			EditorUI::ItemTooltip("Forget the password: in memory and in the credential store");

			if (!state.formError.empty())
				ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s", state.formError.c_str());
			else if (!state.formNotice.empty())
				ImGui::TextDisabled("%s", state.formNotice.c_str());

			// Accounts are made and managed on the website
			ImGui::Spacing();
			if (ImGui::SmallButton(EditorUI::IconLabel(SEDX_ICON_EXTERNAL_LINK, "Gateway website (register, reset password)").c_str()))
				IO::FileSystem::OpenUrl(GetOfficialGatewayServer().GetSiteUrl());

			// Uploads to the public Gateway wait for validation that matches WorldEditor's (and Laminar's OK)
			if (state.server.official && !GatewayClient::AreLiveUploadsAllowed())
			{
				ImGui::Spacing();
				ImGui::TextWrapped("%s  Uploading to the public Gateway isn't switched on in this version yet. The account is kept "
					"ready for it; to try uploads, use a test server.", SEDX_ICON_INFO_CIRCLE);
			}
		}
	}

	// -------------------------------------------------------

	void Init()
	{
		State &state = GetState();
		if (state.initialized)
			return;
		state.initialized = true;

		LoadSettings(state);
		state.server = SettingsServer(state);
		if (const char *url = std::getenv(URL_ENV); url && *url)
		{
			GatewayServer server;
			std::string error;
			if (ParseGatewayServer(url, server, &error))
			{
				state.server = server;
				state.serverFromCommandLine = true;
			}
			else
				EDITOR_ERROR_TAG(LOG_TAG, "{}={}: {}", URL_ENV, url, error);
		}
		ReadCommandLine(state);

		state.store = state.memoryStoreForced ? Credentials::MakeMemoryStore() : Credentials::MakeSystemStore();
		if (!state.store->IsAvailable())
		{
			EDITOR_WARN_TAG(LOG_TAG, "No secure credential store on this system: a Gateway password is kept for the session only");
			state.store = Credentials::MakeMemoryStore();
		}

		GatewayClient::SetServer(state.server);
		if (state.offline)
			GatewayClient::SetTransport(MakeOfflineGatewayTransport("Gateway requests are blocked (--gateway-offline)"));
		if (!state.server.official)
			EDITOR_WARN_TAG(LOG_TAG, "Using a Gateway TEST SERVER: {}", state.server.apiUrl);

		LoadRememberedPassword(state);

		// The form starts from what's saved
		state.useTestServer = !state.server.official;
		Copy(state.userBuffer, sizeof(state.userBuffer), state.userName);
		Copy(state.serverBuffer, sizeof(state.serverBuffer), state.server.official ? std::string() : state.server.apiUrl);

		// --gateway-test-upload: an automatic account + upload check for test runs, never against the public Gateway
		if (!state.startupTestUpload.empty())
		{
			const char *password = std::getenv(PASSWORD_ENV);
			if (state.server.official)
				EDITOR_ERROR_TAG(LOG_TAG, "--gateway-test-upload only works with a test server (--gateway-url): it reads the password from {}", PASSWORD_ENV);
			else if (!password || !*password)
				EDITOR_ERROR_TAG(LOG_TAG, "--gateway-test-upload {}: set {} to the test account's password", state.startupTestUpload, PASSWORD_ENV);
			else
			{
				ForgetSessionPassword(state);
				state.userName = state.startupTestUpload;
				state.password = password;
				Copy(state.userBuffer, sizeof(state.userBuffer), state.userName);
				StartTestUpload(state);
			}
		}
	}

	void Shutdown()
	{
		State &state = GetState();
		if (!state.initialized)
			return;
		ForgetSessionPassword(state);
		state.store.reset();
		state.initialized = false;
	}

	void Tick()
	{
		State &state = GetState();
		if (!state.check || !state.check->IsFinished())
			return;

		const std::shared_ptr<GatewayRequest> request = std::move(state.check);
		state.check.reset();
		if (request->upload)
		{
			ApplyUploadResult(state, *request->upload);
			if (state.account == Account::Accepted)
				EDITOR_INFO_TAG(LOG_TAG, "Test upload to {} as {}: {}", state.server.GetKey(), state.userName, state.accountMessage);
			else
				EDITOR_WARN_TAG(LOG_TAG, "Test upload to {} as {}: {}", state.server.GetKey(), state.userName, state.accountMessage);
		}
		else
		{
			// Never sent (cancelled, blocked): says nothing about the credentials
			state.account = state.password.empty() ? Account::None : Account::Entered;
			state.accountMessage = request->GetError();
			if (request->GetState() != GatewayRequestState::Cancelled)
				EDITOR_WARN_TAG(LOG_TAG, "Test upload not sent: {}", request->GetError());
		}
	}

	void DrawPreferencesTab()
	{
		State &state = GetState();
		if (!state.initialized)
		{
			EditorUI::EmptyState("The Gateway client isn't running.");
			return;
		}

		ImGui::TextWrapped("Your X-Plane Scenery Gateway artist account, for uploading airports and updates. "
			"The password is kept in memory, or in %s when you choose to save it; never in Scenery Editor X's own files.",
			state.store->IsPersistent() ? state.store->GetName() : "the system credential store");
		ImGui::Spacing();
		DrawServerBanner(state);

		DrawServerSection(state);
		DrawAccountSection(state);
	}

	void DrawStatusLine()
	{
		State &state = GetState();
		if (!state.initialized)
			return;

		if (state.offline || !state.server.official)
			DrawServerBanner(state);

		if (state.account == Account::Accepted)
			ImGui::TextColored(AccountColor(state.account), "%s  Signed in as %s", SEDX_ICON_USER, state.userName.c_str());
		else if (!state.userName.empty())
			ImGui::TextColored(AccountColor(state.account), "%s  %s: %s", SEDX_ICON_USER, state.userName.c_str(), AccountText(state.account));
		else
			ImGui::TextDisabled("%s  Not signed in to the Gateway", SEDX_ICON_USER);
		ImGui::SameLine();
		if (ImGui::SmallButton(EditorUI::IconLabel(SEDX_ICON_COG, "Account...").c_str()))
			EditorPreferencesWindow::Open("Scenery Gateway");
		EditorUI::ItemTooltip("Gateway account and server: Edit > Preferences > Scenery Gateway");
	}

	GatewayServer GetServer()
	{
		return GetState().server;
	}

	bool IsTestServer()
	{
		return !GetState().server.official;
	}

	bool HasCredentials()
	{
		const State &state = GetState();
		return !state.userName.empty() && !state.password.empty();
	}

	bool IsSignedIn()
	{
		return GetState().account == Account::Accepted;
	}

	const std::string &GetUserName()
	{
		return GetState().userName;
	}

	bool GetCredentials(std::string &userName, std::string &password)
	{
		const State &state = GetState();
		if (state.userName.empty() || state.password.empty())
			return false;
		userName = state.userName;
		password = state.password;
		return true;
	}

	void SignOut()
	{
		State &state = GetState();
		if (!state.userName.empty() && state.store)
		{
			std::string error;
			if (state.store->Erase(GATEWAY_CREDENTIAL_SERVICE, CredentialAccount(state, state.userName), &error) == Credentials::Status::Failed)
				EDITOR_WARN_TAG(LOG_TAG, "Could not remove the Gateway password from {}: {}", state.store->GetName(), error);
		}
		state.passwordRemembered = false;
		ForgetSessionPassword(state);
		EDITOR_INFO_TAG(LOG_TAG, "Signed out of the Gateway");
	}

	void DrawServerBanner()
	{
		const State &state = GetState();
		if (state.initialized)
			DrawServerBanner(state);
	}

	void ReportUploadResult(const GatewayUploadResult &result)
	{
		State &state = GetState();
		if (state.initialized && !state.password.empty())
			ApplyUploadResult(state, result);
	}
}

// -------------------------------------------------------

namespace SceneryEditorX::EditorPreferencesWindow
{
	namespace
	{
		constexpr const char *WINDOW = "Preferences###EditorPreferences";

		struct Page
		{
			const char *name;
			void (*draw)();
		};

		constexpr Page PAGES[] = {
			{ "Scenery Gateway", &EditorGateway::DrawPreferencesTab },
		};

		struct State
		{
			bool open = false;
			bool focus = false;
			const char *selectTab = nullptr;
		};

		State &GetState()
		{
			static State state;
			return state;
		}
	}

	void Open(const char *tab)
	{
		State &state = GetState();
		state.open = true;
		state.focus = true;
		state.selectTab = tab;
	}

	void Draw()
	{
		State &state = GetState();
		if (!state.open)
			return;

		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(620.0f * dpi, 460.0f * dpi), ImGuiCond_FirstUseEver);
		if (state.focus)
		{
			ImGui::SetNextWindowFocus();
			state.focus = false;
		}
		if (ImGui::Begin(WINDOW, &state.open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
		{
			if (ImGui::BeginTabBar("##PreferenceTabs"))
			{
				for (const Page &page : PAGES)
				{
					const bool select = state.selectTab && std::string_view(state.selectTab) == page.name;
					if (ImGui::BeginTabItem(page.name, nullptr, select ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
					{
						page.draw();
						ImGui::EndTabItem();
					}
				}
				state.selectTab = nullptr;
				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}
}

// -------------------------------------------------------
