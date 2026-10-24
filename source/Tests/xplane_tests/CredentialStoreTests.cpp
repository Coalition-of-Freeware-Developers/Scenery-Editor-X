/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * CredentialStoreTests.cpp
 * -------------------------------------------------------
 * The secure credential store (core/platform/credential_store.*): the in-memory store everywhere, and the
 * operating system's store (Windows Credential Manager / macOS keychain / Secret Service) only when asked for with
 * the hidden [.keychain] tag, since it writes to the signed-in user's real keychain (a uniquely named test entry
 * that the test deletes again).
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/core/platform/credential_store.h>
#include <chrono>
#include <string>

using namespace SceneryEditorX::Credentials;

namespace
{
	/* @brief Store / load / replace / erase round trip, the same for every store. */
	void CheckRoundTrip(CredentialStore &store, const std::string &service, const std::string &account)
	{
		std::string secret = "left over";
		std::string error;

		// Nothing there yet
		store.Erase(service, account);
		CHECK(store.Load(service, account, secret, &error) == Status::NotFound);
		CHECK(secret.empty());

		// Store, read back (UTF-8 and punctuation survive)
		const std::string password = "pa:ss/w\xC3\xB6rd \"x\" \xE6\x97\xA5\xE6\x9C\xAC";	// UTF-8 "pa:ss/wörd "x" 日本"
		REQUIRE(store.Store(service, account, password, &error) == Status::Ok);
		REQUIRE(store.Load(service, account, secret, &error) == Status::Ok);
		CHECK(secret == password);

		// Replacing keeps one entry with the new value
		REQUIRE(store.Store(service, account, "second", &error) == Status::Ok);
		REQUIRE(store.Load(service, account, secret, &error) == Status::Ok);
		CHECK(secret == "second");

		// Another account under the same service is separate
		REQUIRE(store.Store(service, account + "-other", "other", &error) == Status::Ok);
		REQUIRE(store.Load(service, account, secret, &error) == Status::Ok);
		CHECK(secret == "second");

		// Erase, then it's gone (and erasing again says so)
		CHECK(store.Erase(service, account, &error) == Status::Ok);
		CHECK(store.Load(service, account, secret, &error) == Status::NotFound);
		CHECK(store.Erase(service, account, &error) == Status::NotFound);
		CHECK(store.Erase(service, account + "-other", &error) == Status::Ok);

		// Oversized secrets are refused before reaching the store
		CHECK(store.Store(service, account, std::string(4096, 'x'), &error) == Status::Failed);
		CHECK_FALSE(error.empty());
		CHECK(store.Load(service, account, secret, &error) == Status::NotFound);
	}
}

TEST_CASE("Credential store: memory store round trip", "[credentials]")
{
	const auto store = MakeMemoryStore();
	REQUIRE(store->IsAvailable());
	CHECK_FALSE(store->IsPersistent());
	CheckRoundTrip(*store, "Scenery Editor X - Test", "artist");
}

TEST_CASE("Credential store: memory stores don't share secrets", "[credentials]")
{
	const auto first = MakeMemoryStore();
	const auto second = MakeMemoryStore();
	REQUIRE(first->Store("svc", "user", "secret") == Status::Ok);
	std::string secret;
	CHECK(second->Load("svc", "user", secret) == Status::NotFound);
}

TEST_CASE("Credential store: SecureClear empties and overwrites", "[credentials]")
{
	std::string secret = "hunter2 with enough characters to live on the heap";
	const char *data = secret.data();
	const size_t length = secret.size();
	SecureClear(secret);
	CHECK(secret.empty());
	// clear() keeps the buffer: the old characters must be gone from it
	bool allZero = true;
	for (size_t i = 0; i < length; ++i)
		allZero = allZero && data[i] == '\0';
	CHECK(allZero);

	char buffer[8] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };
	SecureClear(buffer, sizeof(buffer));
	for (const char c : buffer)
		CHECK(c == '\0');
	SecureClear(nullptr, 4);	// No crash
}

TEST_CASE("Credential store: status names", "[credentials]")
{
	CHECK(std::string(GetStatusName(Status::Ok)) == "Ok");
	CHECK(std::string(GetStatusName(Status::NotFound)) == "Not found");
	CHECK(std::string(GetStatusName(Status::Unavailable)) == "Unavailable");
	CHECK(std::string(GetStatusName(Status::Failed)) == "Failed");
}

TEST_CASE("Credential store: the system store", "[credentials]")
{
	const auto store = MakeSystemStore();
	REQUIRE(store != nullptr);
	CHECK(std::string(store->GetName()).size() > 0);
#if defined(_WIN32) || defined(__APPLE__)
	CHECK(store->IsAvailable());
	CHECK(store->IsPersistent());
#endif
	if (!store->IsAvailable())
	{
		// Linux without libsecret: every call says Unavailable and nothing is written anywhere
		std::string secret = "x";
		std::string error;
		CHECK(store->Store("svc", "user", "secret", &error) == Status::Unavailable);
		CHECK(store->Load("svc", "user", secret, &error) == Status::Unavailable);
		CHECK(secret.empty());
		CHECK(store->Erase("svc", "user", &error) == Status::Unavailable);
	}
}

TEST_CASE("Credential store: the system store round trip (writes to the real keychain)", "[credentials][.keychain]")
{
	const auto store = MakeSystemStore();
	if (!store->IsAvailable())
		SKIP("No secure credential store on this system");

	// A name no real entry has, removed again by the round trip
	const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
	CheckRoundTrip(*store, "Scenery Editor X - Unit Test", "sedx-test-" + std::to_string(stamp));
}

// -------------------------------------------------------
