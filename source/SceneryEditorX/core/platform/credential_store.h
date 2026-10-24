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
 * credential_store.h
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX::Credentials
{
	/* @brief How a credential store call went. */
	enum class Status : uint8_t
	{
		Ok,
		NotFound,		// Nothing is stored under that service and account
		Unavailable,	// This system has no secure store (Linux without a Secret Service): nothing was written
		Failed			// The store refused (locked keychain, access denied, secret too long): see the error text
	};

	const char *GetStatusName(Status status);

	/**
	 * @class CredentialStore
	 * @brief Somewhere to keep secrets (passwords) that the operating system protects for the signed-in user: Windows
	 * Credential Manager, the macOS keychain, or the freedesktop Secret Service on Linux (GNOME Keyring, KWallet),
	 * which is loaded at run time so the editor starts without libsecret. Secrets never go into the editor's own
	 * settings files. A secret is found by its service ("Scenery Editor X - X-Plane Scenery Gateway") and account
	 * (the user name, plus the server when it isn't the default one).
	 *
	 * Standard library + the system API of each OS only, so it builds in the unit tests.
	 */
	class CredentialStore
	{
	public:
		virtual ~CredentialStore() = default;

		/* @brief What the user knows the store as, e.g. "Windows Credential Manager". */
		[[nodiscard]] virtual const char *GetName() const = 0;

		/* @brief False when there's nowhere safe to keep secrets (every call then returns Unavailable). */
		[[nodiscard]] virtual bool IsAvailable() const = 0;

		/* @brief True when secrets outlive the process (the memory store's don't). */
		[[nodiscard]] virtual bool IsPersistent() const = 0;

		/* @brief Saves (or replaces) the secret. */
		virtual Status Store(std::string_view service, std::string_view account, std::string_view secret, std::string *error = nullptr) = 0;

		/* @brief Reads the secret into `secret` (cleared first). */
		virtual Status Load(std::string_view service, std::string_view account, std::string &secret, std::string *error = nullptr) = 0;

		/* @brief Deletes the secret. NotFound when there was none. */
		virtual Status Erase(std::string_view service, std::string_view account, std::string *error = nullptr) = 0;
	};

	/* @brief The operating system's store. Never null: IsAvailable() says whether it can be used. */
	std::unique_ptr<CredentialStore> MakeSystemStore();

	/* @brief Keeps secrets in this process only: for tests, and for the session when the system has no store. */
	std::unique_ptr<CredentialStore> MakeMemoryStore();

	/* @brief Overwrites the characters, then empties the string (best effort: copies made earlier aren't reached). */
	void SecureClear(std::string &secret);
	void SecureClear(char *buffer, size_t size);
}

// -------------------------------------------------------
