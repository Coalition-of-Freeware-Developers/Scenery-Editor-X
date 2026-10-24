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
 * editor_gateway.h
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <string>
#include <SceneryEditorX/xplane/xp_gateway_server.h>
#include <SceneryEditorX/xplane/xp_gateway_submission.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorGateway
{
	/**
	 * The user's X-Plane Scenery Gateway account in the editor: which server to talk to, the artist name, and the
	 * password, which lives only in memory and (when the user asks) in the operating system's credential store.
	 * The account settings (no password) are kept in gateway_account.json in the settings folder. Main thread only.
	 *
	 * The Gateway has no sign-in call: the name and password go with each upload, so the account is only known to be
	 * good once an upload was accepted. Uploads to the public Gateway are off (GatewayClient::AreLiveUploadsAllowed);
	 * a test server can be sent a made-up airport to check the account and the upload path.
	 *
	 * Command line, for testing without the public Gateway:
	 *   --gateway-url <url>            Use another server this session (e.g. http://127.0.0.1:8780, the test server in
	 *                                  scripts/gateway_mock); not saved. SEDX_GATEWAY_URL does the same.
	 *   --gateway-offline              No Gateway network traffic at all (cached downloads still open).
	 *   --gateway-credentials memory   Keep passwords in memory only: nothing is read from or written to the
	 *                                  system credential store.
	 *   --gateway-test-upload <name>   Upload the made-up test airport at startup as this account, with the password
	 *                                  in SEDX_GATEWAY_PASSWORD. Refused for the public Gateway (a password in the
	 *                                  environment is for tests).
	 *   --gateway-preferences          Open Preferences on the Scenery Gateway tab at startup.
	 */

	/* @brief Reads the account settings and the command line, points the Gateway client at the server, and loads a
	 * remembered password. Call after GatewayClient::Init(). */
	void Init();

	/* @brief Wipes the password from memory. */
	void Shutdown();

	/* @brief Once per frame: finishes test uploads in flight. */
	void Tick();

	/* @brief The "Scenery Gateway" page of the preferences. */
	void DrawPreferencesTab();

	/* @brief One-line account status with a button to the preferences (the AIRPORTS panel's Gateway tab). */
	void DrawStatusLine();

	/* @brief The server in use. */
	XPlane::GatewayServer GetServer();

	/* @brief True when the server isn't the public Gateway (the UI says so wherever the account shows). */
	bool IsTestServer();

	/* @brief A user name and password are known for this session (not necessarily checked with the server). */
	bool HasCredentials();

	/* @brief The server accepted an upload with this user name and password this session. */
	bool IsSignedIn();

	/* @brief The signed-in (or entered) artist name, "" when none. */
	const std::string &GetUserName();

	/**
	 * @brief Copies the credentials for a Gateway call (an upload). False when there are none. Callers clear the
	 * password with Credentials::SecureClear when done.
	 */
	bool GetCredentials(std::string &userName, std::string &password);

	/* @brief Forgets the password (memory and credential store) and the checked state. */
	void SignOut();

	/* @brief "TEST SERVER: <url>" (or "OFFLINE") in warning colors; nothing for the public Gateway. */
	void DrawServerBanner();

	/* @brief What an upload made with GetCredentials() was answered: updates the account's checked state. */
	void ReportUploadResult(const XPlane::GatewayUploadResult &result);
}

namespace SceneryEditorX::EditorPreferencesWindow
{
	/**
	 * The Edit > Preferences window. Its pages come from the subsystems that own the settings (the Scenery Gateway
	 * account for now). Main thread only.
	 */

	void Draw();

	/* @brief Opens the window; `tab` selects a page by name ("Scenery Gateway"), nullptr keeps the last one. */
	void Open(const char *tab = nullptr);
}

// -------------------------------------------------------
