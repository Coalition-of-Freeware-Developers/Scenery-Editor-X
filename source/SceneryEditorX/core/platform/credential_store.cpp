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
 * credential_store.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include "credential_store.h"
#include <map>
#include <mutex>
#include <utility>

#if defined(_WIN32)
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#include <wincred.h>
#elif defined(__APPLE__)
	#include <Security/Security.h>
#elif defined(__unix__)
	#include <dlfcn.h>
#endif

// -------------------------------------------------------

namespace SceneryEditorX::Credentials
{
	namespace
	{
		Status Fail(std::string *error, std::string message, const Status status = Status::Failed)
		{
			if (error)
				*error = std::move(message);
			return status;
		}

		/* @brief Secrets are kept to a size every store takes (Windows allows 2560 bytes). */
		constexpr size_t MAX_SECRET_BYTES = 2048;

		// -------------------------------------------------------

		class MemoryStore final : public CredentialStore
		{
		public:
			~MemoryStore() override
			{
				for (auto &[key, secret] : m_Secrets)
					SecureClear(secret);
			}

			[[nodiscard]] const char *GetName() const override { return "this session only"; }
			[[nodiscard]] bool IsAvailable() const override { return true; }
			[[nodiscard]] bool IsPersistent() const override { return false; }

			Status Store(const std::string_view service, const std::string_view account, const std::string_view secret, std::string *error) override
			{
				if (secret.size() > MAX_SECRET_BYTES)
					return Fail(error, "The secret is too long to store");
				std::scoped_lock lock(m_Mutex);
				std::string &slot = m_Secrets[Key(service, account)];
				SecureClear(slot);
				slot.assign(secret);
				return Status::Ok;
			}

			Status Load(const std::string_view service, const std::string_view account, std::string &secret, std::string *) override
			{
				SecureClear(secret);
				std::scoped_lock lock(m_Mutex);
				const auto it = m_Secrets.find(Key(service, account));
				if (it == m_Secrets.end())
					return Status::NotFound;
				secret = it->second;
				return Status::Ok;
			}

			Status Erase(const std::string_view service, const std::string_view account, std::string *) override
			{
				std::scoped_lock lock(m_Mutex);
				const auto it = m_Secrets.find(Key(service, account));
				if (it == m_Secrets.end())
					return Status::NotFound;
				SecureClear(it->second);
				m_Secrets.erase(it);
				return Status::Ok;
			}

		private:
			static std::string Key(const std::string_view service, const std::string_view account)
			{
				std::string key(service);
				key.push_back('\n');
				key.append(account);
				return key;
			}

			std::mutex m_Mutex;
			std::map<std::string, std::string> m_Secrets;
		};

		// -------------------------------------------------------

#if defined(_WIN32)

		std::wstring Widen(const std::string_view text)
		{
			if (text.empty())
				return {};
			const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
			std::wstring wide(static_cast<size_t>(length), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);
			return wide;
		}

		std::string WindowsError(const char *what, const DWORD code)
		{
			char buffer[256] = {};
			const DWORD length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0, buffer, sizeof(buffer), nullptr);
			std::string message = std::string(what) + " (Windows error " + std::to_string(code) + ")";
			if (length > 0)
			{
				std::string text(buffer, length);
				while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ' || text.back() == '.'))
					text.pop_back();
				message += ": " + text;
			}
			return message;
		}

		/**
		 * Generic credentials under "<service>/<account>" (what Credential Manager lists as the Internet or network
		 * address), the account as the user name. CRED_PERSIST_LOCAL_MACHINE: kept for this Windows user on this
		 * computer, encrypted by Windows (DPAPI), not roamed to other machines.
		 */
		class WindowsStore final : public CredentialStore
		{
		public:
			[[nodiscard]] const char *GetName() const override { return "Windows Credential Manager"; }
			[[nodiscard]] bool IsAvailable() const override { return true; }
			[[nodiscard]] bool IsPersistent() const override { return true; }

			Status Store(const std::string_view service, const std::string_view account, const std::string_view secret, std::string *error) override
			{
				if (secret.size() > MAX_SECRET_BYTES)
					return Fail(error, "The secret is too long to store");

				std::wstring target = Target(service, account);
				std::wstring user = Widen(account);
				std::wstring comment = L"Scenery Editor X";
				std::string blob(secret);

				CREDENTIALW credential = {};
				credential.Type = CRED_TYPE_GENERIC;
				credential.TargetName = target.data();
				credential.Comment = comment.data();
				credential.CredentialBlobSize = static_cast<DWORD>(blob.size());
				credential.CredentialBlob = reinterpret_cast<LPBYTE>(blob.data());
				credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
				credential.UserName = user.data();

				const BOOL ok = CredWriteW(&credential, 0);
				const DWORD code = ok ? 0 : GetLastError();
				SecureClear(blob);
				if (!ok)
					return Fail(error, WindowsError("Could not save to Windows Credential Manager", code));
				return Status::Ok;
			}

			Status Load(const std::string_view service, const std::string_view account, std::string &secret, std::string *error) override
			{
				SecureClear(secret);
				PCREDENTIALW credential = nullptr;
				if (!CredReadW(Target(service, account).c_str(), CRED_TYPE_GENERIC, 0, &credential))
				{
					const DWORD code = GetLastError();
					if (code == ERROR_NOT_FOUND)
						return Status::NotFound;
					return Fail(error, WindowsError("Could not read from Windows Credential Manager", code));
				}
				secret.assign(reinterpret_cast<const char *>(credential->CredentialBlob), credential->CredentialBlobSize);
				SecureZeroMemory(credential->CredentialBlob, credential->CredentialBlobSize);
				CredFree(credential);
				return Status::Ok;
			}

			Status Erase(const std::string_view service, const std::string_view account, std::string *error) override
			{
				if (!CredDeleteW(Target(service, account).c_str(), CRED_TYPE_GENERIC, 0))
				{
					const DWORD code = GetLastError();
					if (code == ERROR_NOT_FOUND)
						return Status::NotFound;
					return Fail(error, WindowsError("Could not delete from Windows Credential Manager", code));
				}
				return Status::Ok;
			}

		private:
			static std::wstring Target(const std::string_view service, const std::string_view account)
			{
				return Widen(std::string(service) + "/" + std::string(account));
			}
		};

#elif defined(__APPLE__)

		/* @brief Owns a Core Foundation object. */
		template <typename T>
		struct CFHolder
		{
			T ref = nullptr;
			explicit CFHolder(T value = nullptr) : ref(value) {}
			~CFHolder() { if (ref) CFRelease(ref); }
			CFHolder(const CFHolder &) = delete;
			CFHolder &operator=(const CFHolder &) = delete;
		};

		CFStringRef MakeString(const std::string_view text)
		{
			return CFStringCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(text.data()), static_cast<CFIndex>(text.size()), kCFStringEncodingUTF8, false);
		}

		std::string KeychainError(const char *what, const OSStatus status)
		{
			std::string message = std::string(what) + " (OSStatus " + std::to_string(static_cast<long>(status)) + ")";
			CFHolder<CFStringRef> text(SecCopyErrorMessageString(status, nullptr));
			if (text.ref)
			{
				char buffer[256] = {};
				if (CFStringGetCString(text.ref, buffer, sizeof(buffer), kCFStringEncodingUTF8))
					message += std::string(": ") + buffer;
			}
			return message;
		}

		/* @brief Generic passwords in the user's login keychain (kSecClassGenericPassword, service + account). */
		class KeychainStore final : public CredentialStore
		{
		public:
			[[nodiscard]] const char *GetName() const override { return "macOS Keychain"; }
			[[nodiscard]] bool IsAvailable() const override { return true; }
			[[nodiscard]] bool IsPersistent() const override { return true; }

			Status Store(const std::string_view service, const std::string_view account, const std::string_view secret, std::string *error) override
			{
				if (secret.size() > MAX_SECRET_BYTES)
					return Fail(error, "The secret is too long to store");

				CFHolder<CFMutableDictionaryRef> query(MakeQuery(service, account));
				CFHolder<CFDataRef> data(CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(secret.data()), static_cast<CFIndex>(secret.size())));
				CFHolder<CFMutableDictionaryRef> update(CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
				CFDictionarySetValue(update.ref, kSecValueData, data.ref);

				OSStatus status = SecItemUpdate(query.ref, update.ref);
				if (status == errSecItemNotFound)
				{
					CFHolder<CFStringRef> label(MakeString(std::string(service) + " (" + std::string(account) + ")"));
					CFDictionarySetValue(query.ref, kSecValueData, data.ref);
					CFDictionarySetValue(query.ref, kSecAttrLabel, label.ref);
					status = SecItemAdd(query.ref, nullptr);
				}
				if (status != errSecSuccess)
					return Fail(error, KeychainError("Could not save to the keychain", status));
				return Status::Ok;
			}

			Status Load(const std::string_view service, const std::string_view account, std::string &secret, std::string *error) override
			{
				SecureClear(secret);
				CFHolder<CFMutableDictionaryRef> query(MakeQuery(service, account));
				CFDictionarySetValue(query.ref, kSecReturnData, kCFBooleanTrue);
				CFDictionarySetValue(query.ref, kSecMatchLimit, kSecMatchLimitOne);

				CFTypeRef result = nullptr;
				const OSStatus status = SecItemCopyMatching(query.ref, &result);
				CFHolder<CFTypeRef> owned(result);
				if (status == errSecItemNotFound)
					return Status::NotFound;
				if (status != errSecSuccess || !result || CFGetTypeID(result) != CFDataGetTypeID())
					return Fail(error, KeychainError("Could not read from the keychain", status));

				const CFDataRef data = static_cast<CFDataRef>(result);
				secret.assign(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), static_cast<size_t>(CFDataGetLength(data)));
				return Status::Ok;
			}

			Status Erase(const std::string_view service, const std::string_view account, std::string *error) override
			{
				CFHolder<CFMutableDictionaryRef> query(MakeQuery(service, account));
				const OSStatus status = SecItemDelete(query.ref);
				if (status == errSecItemNotFound)
					return Status::NotFound;
				if (status != errSecSuccess)
					return Fail(error, KeychainError("Could not delete from the keychain", status));
				return Status::Ok;
			}

		private:
			static CFMutableDictionaryRef MakeQuery(const std::string_view service, const std::string_view account)
			{
				CFMutableDictionaryRef query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
				CFHolder<CFStringRef> serviceText(MakeString(service));
				CFHolder<CFStringRef> accountText(MakeString(account));
				CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
				CFDictionarySetValue(query, kSecAttrService, serviceText.ref);
				CFDictionarySetValue(query, kSecAttrAccount, accountText.ref);
				return query;
			}
		};

#elif defined(__unix__)

		/*
		 * The freedesktop Secret Service through libsecret, opened with dlopen: the declarations below mirror
		 * <libsecret/secret.h> and <glib.h> (stable ABI since libsecret 0.18), so neither is needed to build and a
		 * system without them just reports the store as unavailable.
		 */
		struct GErrorABI
		{
			uint32_t domain;
			int code;
			char *message;
		};

		struct SecretSchemaAttributeABI
		{
			const char *name;
			int type;					// SECRET_SCHEMA_ATTRIBUTE_STRING = 0
		};

		struct SecretSchemaABI
		{
			const char *name;
			int flags;					// SECRET_SCHEMA_NONE = 0
			SecretSchemaAttributeABI attributes[32];
			int reserved;
			void *reserved1;
			void *reserved2;
			void *reserved3;
			void *reserved4;
			void *reserved5;
			void *reserved6;
			void *reserved7;
		};

		using StoreSyncFn = int (*)(const SecretSchemaABI *, const char *collection, const char *label, const char *password, void *cancellable, GErrorABI **error, ...);
		using LookupSyncFn = char *(*)(const SecretSchemaABI *, void *cancellable, GErrorABI **error, ...);
		using ClearSyncFn = int (*)(const SecretSchemaABI *, void *cancellable, GErrorABI **error, ...);
		using PasswordFreeFn = void (*)(char *);
		using ErrorFreeFn = void (*)(GErrorABI *);

		class SecretServiceStore final : public CredentialStore
		{
		public:
			SecretServiceStore()
			{
				m_Schema.name = "org.sceneryeditorx.Credential";
				m_Schema.attributes[0] = { "service", 0 };
				m_Schema.attributes[1] = { "account", 0 };

				for (const char *name : { "libsecret-1.so.0", "libsecret-1.so" })
				{
					m_Library = dlopen(name, RTLD_NOW | RTLD_LOCAL);
					if (m_Library)
						break;
				}
				if (!m_Library)
					return;

				m_StoreSync = reinterpret_cast<StoreSyncFn>(dlsym(m_Library, "secret_password_store_sync"));
				m_LookupSync = reinterpret_cast<LookupSyncFn>(dlsym(m_Library, "secret_password_lookup_sync"));
				m_ClearSync = reinterpret_cast<ClearSyncFn>(dlsym(m_Library, "secret_password_clear_sync"));
				m_PasswordFree = reinterpret_cast<PasswordFreeFn>(dlsym(m_Library, "secret_password_free"));
				m_ErrorFree = reinterpret_cast<ErrorFreeFn>(dlsym(m_Library, "g_error_free"));	// From glib, which libsecret loads
				if (!m_StoreSync || !m_LookupSync || !m_ClearSync || !m_PasswordFree || !m_ErrorFree)
				{
					dlclose(m_Library);
					m_Library = nullptr;
				}
			}

			~SecretServiceStore() override
			{
				if (m_Library)
					dlclose(m_Library);
			}

			[[nodiscard]] const char *GetName() const override { return "Secret Service (GNOME Keyring / KWallet)"; }
			[[nodiscard]] bool IsAvailable() const override { return m_Library != nullptr; }
			[[nodiscard]] bool IsPersistent() const override { return true; }

			Status Store(const std::string_view service, const std::string_view account, const std::string_view secret, std::string *error) override
			{
				if (!m_Library)
					return Fail(error, "libsecret isn't installed: there is no Secret Service to save to", Status::Unavailable);
				if (secret.size() > MAX_SECRET_BYTES)
					return Fail(error, "The secret is too long to store");

				const std::string serviceText(service);
				const std::string accountText(account);
				const std::string label = serviceText + " (" + accountText + ")";
				std::string password(secret);
				GErrorABI *gerror = nullptr;
				const int ok = m_StoreSync(&m_Schema, nullptr, label.c_str(), password.c_str(), nullptr, &gerror,
					"service", serviceText.c_str(), "account", accountText.c_str(), static_cast<const char *>(nullptr));
				SecureClear(password);
				if (!ok || gerror)
					return Fail(error, TakeError("Could not save to the Secret Service", gerror));
				return Status::Ok;
			}

			Status Load(const std::string_view service, const std::string_view account, std::string &secret, std::string *error) override
			{
				SecureClear(secret);
				if (!m_Library)
					return Fail(error, "libsecret isn't installed: there is no Secret Service to read from", Status::Unavailable);

				const std::string serviceText(service);
				const std::string accountText(account);
				GErrorABI *gerror = nullptr;
				char *password = m_LookupSync(&m_Schema, nullptr, &gerror,
					"service", serviceText.c_str(), "account", accountText.c_str(), static_cast<const char *>(nullptr));
				if (gerror)
				{
					if (password)
						m_PasswordFree(password);
					return Fail(error, TakeError("Could not read from the Secret Service", gerror));
				}
				if (!password)
					return Status::NotFound;
				secret.assign(password);
				m_PasswordFree(password);		// Zeroes it
				return Status::Ok;
			}

			Status Erase(const std::string_view service, const std::string_view account, std::string *error) override
			{
				if (!m_Library)
					return Fail(error, "libsecret isn't installed: there is no Secret Service", Status::Unavailable);

				const std::string serviceText(service);
				const std::string accountText(account);
				GErrorABI *gerror = nullptr;
				const int removed = m_ClearSync(&m_Schema, nullptr, &gerror,
					"service", serviceText.c_str(), "account", accountText.c_str(), static_cast<const char *>(nullptr));
				if (gerror)
					return Fail(error, TakeError("Could not delete from the Secret Service", gerror));
				return removed ? Status::Ok : Status::NotFound;
			}

		private:
			std::string TakeError(const char *what, GErrorABI *gerror) const
			{
				std::string message(what);
				if (gerror)
				{
					if (gerror->message)
						message += std::string(": ") + gerror->message;
					m_ErrorFree(gerror);
				}
				return message;
			}

			SecretSchemaABI m_Schema = {};
			void *m_Library = nullptr;
			StoreSyncFn m_StoreSync = nullptr;
			LookupSyncFn m_LookupSync = nullptr;
			ClearSyncFn m_ClearSync = nullptr;
			PasswordFreeFn m_PasswordFree = nullptr;
			ErrorFreeFn m_ErrorFree = nullptr;
		};

#endif

		/* @brief Where there's no system store at all: everything is Unavailable. */
		class NoStore final : public CredentialStore
		{
		public:
			[[nodiscard]] const char *GetName() const override { return "no secure store"; }
			[[nodiscard]] bool IsAvailable() const override { return false; }
			[[nodiscard]] bool IsPersistent() const override { return false; }

			Status Store(std::string_view, std::string_view, std::string_view, std::string *error) override
			{
				return Fail(error, "This system has no secure credential store", Status::Unavailable);
			}

			Status Load(std::string_view, std::string_view, std::string &secret, std::string *error) override
			{
				SecureClear(secret);
				return Fail(error, "This system has no secure credential store", Status::Unavailable);
			}

			Status Erase(std::string_view, std::string_view, std::string *error) override
			{
				return Fail(error, "This system has no secure credential store", Status::Unavailable);
			}
		};
	}

	// -------------------------------------------------------

	const char *GetStatusName(const Status status)
	{
		switch (status)
		{
			case Status::Ok:			return "Ok";
			case Status::NotFound:		return "Not found";
			case Status::Unavailable:	return "Unavailable";
			case Status::Failed:		return "Failed";
		}
		return "Unknown";
	}

	std::unique_ptr<CredentialStore> MakeSystemStore()
	{
#if defined(_WIN32)
		return std::make_unique<WindowsStore>();
#elif defined(__APPLE__)
		return std::make_unique<KeychainStore>();
#elif defined(__unix__)
		auto store = std::make_unique<SecretServiceStore>();
		if (store->IsAvailable())
			return store;
		return std::make_unique<NoStore>();
#else
		return std::make_unique<NoStore>();
#endif
	}

	std::unique_ptr<CredentialStore> MakeMemoryStore()
	{
		return std::make_unique<MemoryStore>();
	}

	void SecureClear(char *buffer, const size_t size)
	{
		if (!buffer)
			return;
		// volatile: the compiler may not drop the stores as dead (the buffer is about to be freed or reused)
		volatile char *p = buffer;
		for (size_t i = 0; i < size; ++i)
			p[i] = 0;
	}

	void SecureClear(std::string &secret)
	{
		SecureClear(secret.data(), secret.capacity());
		secret.clear();
	}
}

// -------------------------------------------------------
