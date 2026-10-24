/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * GatewayAccountTests.cpp
 * -------------------------------------------------------
 * Scenery Gateway servers and accounts (xp_gateway_server.*, xp_gateway_account.*): server URLs, the account
 * settings file, and sign-in / upload requests run through a fake transport, so nothing here opens a socket or
 * reaches gateway.x-plane.com.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/xplane/xp_gateway_account.h>
#include <SceneryEditorX/xplane/xp_gateway_server.h>
#include <SceneryEditorX/xplane/xp_gateway_submission.h>
#include <SceneryEditorX/xplane/xp_gateway_types.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace SceneryEditorX::XPlane;

TEST_CASE("Gateway server: the public Gateway", "[xplane][gateway][account]")
{
	const GatewayServer server = GetOfficialGatewayServer();
	CHECK(server.apiUrl == "https://gateway.x-plane.com/apiv1");
	CHECK(server.apiUrl == GATEWAY_API_URL);
	CHECK(server.official);
	CHECK_FALSE(server.local);
	CHECK(server.port == 0);
	CHECK(server.GetKey() == "gateway.x-plane.com");
	CHECK(server.GetSiteUrl() == "https://gateway.x-plane.com");
}

TEST_CASE("Gateway server: addresses people type", "[xplane][gateway][account]")
{
	GatewayServer server;
	std::string error;

	// A local test server: /apiv1 is added, the port kept
	REQUIRE(ParseGatewayServer("  http://127.0.0.1:8780  ", server, &error));
	CHECK(server.apiUrl == "http://127.0.0.1:8780/apiv1");
	CHECK(server.local);
	CHECK_FALSE(server.official);
	CHECK(server.port == 8780);
	CHECK(server.GetKey() == "127.0.0.1:8780");

	REQUIRE(ParseGatewayServer("HTTP://LocalHost:8780/apiv1///", server, &error));
	CHECK(server.apiUrl == "http://localhost:8780/apiv1");
	CHECK(server.local);

	REQUIRE(ParseGatewayServer("http://[::1]:9000/mock/apiv1", server, &error));
	CHECK(server.host == "::1");
	CHECK(server.apiUrl == "http://[::1]:9000/mock/apiv1");
	CHECK(server.GetKey() == "[::1]:9000");
	CHECK(server.local);

	// The scheme's default port is dropped, the public host is recognized however it's written
	REQUIRE(ParseGatewayServer("https://Gateway.X-Plane.com:443/", server, &error));
	CHECK(server.apiUrl == "https://gateway.x-plane.com/apiv1");
	CHECK(server.official);
	CHECK(server == GetOfficialGatewayServer());

	// A remote https test server is fine; it isn't local or official
	REQUIRE(ParseGatewayServer("https://staging.example.org/api", server, &error));
	CHECK(server.apiUrl == "https://staging.example.org/api");
	CHECK_FALSE(server.local);
	CHECK_FALSE(server.official);

	// 127.0.0.0/8 is local, a name that only starts like it isn't
	REQUIRE(ParseGatewayServer("http://127.4.5.6", server, &error));
	CHECK(server.local);
	CHECK_FALSE(ParseGatewayServer("http://127.0.0.1.example.com", server, &error));
}

TEST_CASE("Gateway server: addresses that are refused", "[xplane][gateway][account]")
{
	GatewayServer server;
	std::string error;
	const auto refused = [&](const char *url)
	{
		error.clear();
		const bool ok = ParseGatewayServer(url, server, &error);
		INFO(url);
		CHECK_FALSE(ok);
		CHECK_FALSE(error.empty());
		CHECK(server.apiUrl.empty());
	};

	refused("");
	refused("gateway.x-plane.com");							// No scheme
	refused("ftp://127.0.0.1/apiv1");
	refused("http://gateway.x-plane.com/apiv1");				// A password over plain http across the network
	refused("http://192.168.1.20:8780");
	refused("https://user:secret@gateway.x-plane.com/apiv1");	// Credentials in the URL
	refused("https://gateway.x-plane.com/apiv1?debug=1");
	refused("https://gateway.x-plane.com/apiv1#x");
	refused("http://127.0.0.1:0");
	refused("http://127.0.0.1:70000");
	refused("http://127.0.0.1:80a");
	refused("https://bad_host!/apiv1");
	refused("http://[::1:8780");
}

TEST_CASE("Gateway account: the settings file never holds a password", "[xplane][gateway][account]")
{
	GatewayAccountSettings settings;
	settings.userName = "Test Artist";
	settings.rememberPassword = false;
	settings.serverUrl = "http://127.0.0.1:8780/apiv1";

	const std::string text = WriteGatewayAccountSettings(settings);
	CHECK(text.find("password\"") == std::string::npos);
	CHECK(text.find("Password\"") != std::string::npos);	// Only the "rememberPassword" flag

	GatewayAccountSettings read;
	std::string error;
	REQUIRE(ParseGatewayAccountSettings(text, read, &error));
	CHECK(read == settings);

	// The public Gateway is the default and isn't written out
	GatewayAccountSettings defaults;
	defaults.userName = "a";
	CHECK(WriteGatewayAccountSettings(defaults).find("serverUrl") == std::string::npos);
	REQUIRE(ParseGatewayAccountSettings(WriteGatewayAccountSettings(defaults), read, &error));
	CHECK(read.rememberPassword);
	CHECK(read.serverUrl.empty());

	// A password someone put in the file by hand is ignored, wrong types fall back to defaults
	REQUIRE(ParseGatewayAccountSettings(R"({"userName":"x","password":"hunter2","rememberPassword":"yes","extra":1})", read, &error));
	CHECK(read.userName == "x");
	CHECK(read.rememberPassword);
	CHECK_FALSE(ParseGatewayAccountSettings("not json", read, &error));
	CHECK_FALSE(error.empty());
	CHECK_FALSE(ParseGatewayAccountSettings("[1,2]", read, &error));
}

TEST_CASE("Gateway account: credential store accounts per server", "[xplane][gateway][account]")
{
	const GatewayServer official = GetOfficialGatewayServer();
	GatewayServer mock;
	REQUIRE(ParseGatewayServer("http://127.0.0.1:8780", mock));

	CHECK(GetGatewayCredentialAccount(official, "Test Artist") == "Test Artist");
	CHECK(GetGatewayCredentialAccount(mock, "Test Artist") == "Test Artist@127.0.0.1:8780");
	CHECK(GetGatewayCredentialAccount(official, "a") != GetGatewayCredentialAccount(mock, "a"));
}

TEST_CASE("Gateway account: user names", "[xplane][gateway][account]")
{
	std::string error;
	CHECK(ValidateGatewayUserName("Test Artist", &error));
	CHECK(ValidateGatewayUserName("a", &error));
	CHECK_FALSE(ValidateGatewayUserName("", &error));
	CHECK_FALSE(ValidateGatewayUserName(" lead", &error));
	CHECK_FALSE(ValidateGatewayUserName("trail ", &error));
	CHECK_FALSE(ValidateGatewayUserName("tab\there", &error));
	CHECK_FALSE(ValidateGatewayUserName(std::string(65, 'x'), &error));
	CHECK_FALSE(error.empty());
}

namespace
{
	/* @brief Reads a stored-only zip (what WriteStoredZip makes): names and contents, checking every CRC. */
	std::vector<ZipFile> ReadStoredZip(const std::vector<uint8_t> &zip)
	{
		const auto u16 = [&](const size_t at) { return static_cast<uint32_t>(zip.at(at) | (zip.at(at + 1) << 8)); };
		const auto u32 = [&](const size_t at) { return u16(at) | (u16(at + 2) << 16); };
		std::vector<ZipFile> files;
		size_t at = 0;
		while (at + 4 <= zip.size() && u32(at) == 0x04034B50u)
		{
			REQUIRE(u16(at + 8) == 0);	// Stored
			const uint32_t crc = u32(at + 14);
			const uint32_t size = u32(at + 18);
			const uint32_t nameLength = u16(at + 26);
			const uint32_t extraLength = u16(at + 28);
			ZipFile file;
			file.name.assign(reinterpret_cast<const char *>(zip.data() + at + 30), nameLength);
			const size_t data = at + 30 + nameLength + extraLength;
			file.data.assign(reinterpret_cast<const char *>(zip.data() + data), size);
			CHECK(Crc32(file.data) == crc);
			files.push_back(std::move(file));
			at = data + size;
		}
		// Then the central directory and its end record, which counts the files
		REQUIRE(zip.size() >= 22);
		CHECK(u32(zip.size() - 22) == 0x06054B50u);
		CHECK(u16(zip.size() - 22 + 10) == files.size());
		return files;
	}

	const ZipFile *Find(const std::vector<ZipFile> &files, const std::string &name)
	{
		for (const ZipFile &file : files)
		{
			if (file.name == name)
				return &file;
		}
		return nullptr;
	}

	GatewayHttpResponse Answer(const long status, std::string body)
	{
		GatewayHttpResponse response;
		response.completed = true;
		response.status = status;
		response.body = std::move(body);
		return response;
	}
}

TEST_CASE("Gateway upload: only test servers take uploads for now", "[xplane][gateway][upload]")
{
	GatewayServer mock;
	REQUIRE(ParseGatewayServer("http://127.0.0.1:8780", mock));
	std::string reason;

	CHECK_FALSE(CanUploadTo(GetOfficialGatewayServer(), false, &reason));
	CHECK(reason.find("public") != std::string::npos);
	CHECK(CanUploadTo(GetOfficialGatewayServer(), true, &reason));
	CHECK(CanUploadTo(mock, false, &reason));
	CHECK_FALSE(CanUploadTo(GatewayServer{}, true, &reason));
}

TEST_CASE("Gateway upload: the request WorldEditor sends", "[xplane][gateway][upload]")
{
	GatewayServer mock;
	REQUIRE(ParseGatewayServer("http://127.0.0.1:8780", mock));

	GatewaySubmission submission;
	submission.icao = "XSDX";
	submission.airportName = "Test Field";
	submission.artistComments = "First try";
	submission.is3D = true;
	submission.features = "1,2";
	submission.validatedAgainst = "12.1.3-r2-2f236";
	submission.latitude = 45.0045;
	submission.longitude = -100.0;
	submission.metadata = { { "city", "Mockville" }, { "icao_code", "XSDX" } };
	submission.masterZip = { 'P', 'K', 3, 4 };

	GatewayHttpRequest request = MakeGatewayUploadRequest(mock, submission, "Test Artist", "s3cret");
	CHECK(request.method == "PUT");
	CHECK(request.url == "http://127.0.0.1:8780/apiv1/scenery");
	CHECK(request.sendsCredentials);
	bool json = false;
	for (const auto &[name, value] : request.headers)
		json = json || (name == "Content-Type" && value == "application/json");
	CHECK(json);

	const nlohmann::json body = nlohmann::json::parse(request.body);
	REQUIRE(body.contains("scenery"));
	const nlohmann::json &scenery = body["scenery"];
	CHECK(scenery["userId"] == "Test Artist");
	CHECK(scenery["password"] == "s3cret");
	CHECK(scenery["icao"] == "XSDX");
	CHECK(scenery["aptName"] == "Test Field");
	CHECK(scenery["artistComments"] == "First try");
	CHECK(scenery["parentId"].is_null());				// A first upload
	CHECK(scenery["type"] == "3D");
	CHECK(scenery["features"] == "1,2");
	CHECK(scenery["validatedAgainst"] == "12.1.3-r2-2f236");
	CHECK(scenery["latitude"].get<double>() == Catch::Approx(45.0045));
	CHECK(scenery["additionalMetadata"]["city"] == "Mockville");
	CHECK_FALSE(scenery.contains("clientVersion"));		// Not sent unless set: never pretend to be WED
	CHECK(scenery["masterZipBlob"] == "UEsDBA==");

	// An update names the submission it replaces
	submission.parentId = 97429;
	submission.is3D = false;
	submission.clientVersion = 1;
	request = MakeGatewayUploadRequest(mock, submission, "Test Artist", "s3cret");
	const nlohmann::json update = nlohmann::json::parse(request.body)["scenery"];
	CHECK(update["parentId"] == 97429);
	CHECK(update["type"] == "2D");
	CHECK(update["clientVersion"] == 1);
}

TEST_CASE("Gateway upload: reading the server's answer", "[xplane][gateway][upload]")
{
	using Outcome = GatewayUploadResult::Outcome;

	GatewayUploadResult result = ReadGatewayUploadResponse(Answer(200, R"({"sceneryId":900003})"));
	CHECK(result.outcome == Outcome::Uploaded);
	CHECK(result.Succeeded());
	CHECK(result.sceneryId == 900003);

	// Only a 200 with an integer id counts
	CHECK(ReadGatewayUploadResponse(Answer(200, R"({"sceneryId":"12"})")).outcome == Outcome::Unconfirmed);
	CHECK(ReadGatewayUploadResponse(Answer(200, R"({"ok":true})")).outcome == Outcome::Unconfirmed);
	CHECK(ReadGatewayUploadResponse(Answer(200, "<html>ok</html>")).outcome == Outcome::Unconfirmed);
	CHECK(ReadGatewayUploadResponse(Answer(201, R"({"sceneryId":5})")).outcome == Outcome::Rejected);

	// The live Gateway answers an unsupported X-Plane build with 401 and a line of text/html
	result = ReadGatewayUploadResponse(Answer(401, "Invalid X-Plane Version. Please check the NOTAMS Gateway page for supported versions."));
	CHECK(result.outcome == Outcome::Unauthorized);
	CHECK(result.message == "Invalid X-Plane Version. Please check the NOTAMS Gateway page for supported versions. (HTTP 401)");

	result = ReadGatewayUploadResponse(Answer(403, "<html><body><h1>Forbidden</h1>\n<p>Bad\tpassword</p></body></html>"));
	CHECK(result.outcome == Outcome::Unauthorized);
	CHECK(result.message == "Forbidden Bad password (HTTP 403)");

	result = ReadGatewayUploadResponse(Answer(404, R"({"message":"Unknown airport code ENTY"})"));
	CHECK(result.outcome == Outcome::UnknownAirport);
	CHECK(result.message == "Unknown airport code ENTY (HTTP 404)");

	result = ReadGatewayUploadResponse(Answer(500, ""));
	CHECK(result.outcome == Outcome::Rejected);
	CHECK(result.message == "HTTP 500");

	GatewayHttpResponse dropped;
	dropped.error = "Connection refused";
	result = ReadGatewayUploadResponse(dropped);
	CHECK(result.outcome == Outcome::NoAnswer);
	CHECK(result.message == "Connection refused");
}

TEST_CASE("Gateway upload: zip archives and the master zip", "[xplane][gateway][upload]")
{
	// CRC-32 check value
	CHECK(Crc32("123456789") == 0xCBF43926u);
	CHECK(Crc32("") == 0u);

	const std::vector<ZipFile> files = ReadStoredZip(WriteStoredZip({ { "a.txt", "hello" }, { "dir/b.bin", std::string("\0\1\2", 3) } }));
	REQUIRE(files.size() == 2);
	CHECK(files[0].name == "a.txt");
	CHECK(files[0].data == "hello");
	CHECK(files[1].data == std::string("\0\1\2", 3));
	CHECK(ReadStoredZip(WriteStoredZip({})).empty());

	// WED's layout: <ICAO>.dat, <ICAO>.txt only with DSF text, the pack zip under one top folder
	const std::vector<ZipFile> master = ReadStoredZip(MakeGatewayMasterZip("XSDX", "apt", "dsf", { { "Earth nav data/apt.dat", "apt" } }));
	REQUIRE(master.size() == 3);
	CHECK(Find(master, "XSDX.dat"));
	CHECK(Find(master, "XSDX.txt"));
	const ZipFile *packZip = Find(master, "XSDX_Scenery_Pack.zip");
	REQUIRE(packZip);
	const std::vector<ZipFile> pack = ReadStoredZip(std::vector<uint8_t>(packZip->data.begin(), packZip->data.end()));
	REQUIRE(pack.size() == 1);
	CHECK(pack[0].name == "XSDX_Scenery_Pack/Earth nav data/apt.dat");
	CHECK(ReadStoredZip(MakeGatewayMasterZip("XSDX", "apt", "", {})).size() == 2);
}

TEST_CASE("Gateway upload: the made-up test airport", "[xplane][gateway][upload]")
{
	const GatewaySubmission submission = MakeGatewayTestSubmission();
	CHECK(submission.icao == "XSDX");
	CHECK_FALSE(submission.is3D);
	CHECK(submission.parentId == 0);
	CHECK(submission.metadata.at("icao_code") == "XSDX");

	const std::vector<ZipFile> master = ReadStoredZip(submission.masterZip);
	const ZipFile *dat = Find(master, "XSDX.dat");
	REQUIRE(dat);
	CHECK(dat->data.starts_with("I\n1200 "));
	CHECK(dat->data.find("\n1 1500 0 0 XSDX ") != std::string::npos);
	CHECK(dat->data.find("\n100 ") != std::string::npos);		// A runway
	CHECK(dat->data.find("\n130 ") != std::string::npos);		// Its boundary
	CHECK(dat->data.ends_with("99\n"));
	CHECK_FALSE(Find(master, "XSDX.txt"));						// No DSF items, no DSF text
	REQUIRE(Find(master, "XSDX_Scenery_Pack.zip"));
}

TEST_CASE("Gateway upload: a fake server round trip never leaves this computer", "[xplane][gateway][upload]")
{
	GatewayServer mock;
	REQUIRE(ParseGatewayServer("http://127.0.0.1:8780", mock));

	// A transport that plays the Gateway: refuses anything not on the loopback, checks the credentials
	std::vector<std::string> seen;
	const GatewayTransport transport = [&seen](const GatewayHttpRequest &request, const GatewayTransferHooks &hooks)
	{
		seen.push_back(request.method + " " + request.url);
		GatewayServer target;
		REQUIRE(ParseGatewayServer(request.url, target));
		REQUIRE(target.local);
		REQUIRE_FALSE(target.official);
		if (hooks.progress)
			hooks.progress(1.0);
		const nlohmann::json scenery = nlohmann::json::parse(request.body)["scenery"];
		if (scenery["userId"] != "Test Artist" || scenery["password"] != "test-password")
			return Answer(401, "Invalid username or password");
		return Answer(200, R"({"sceneryId":900003})");
	};

	const GatewaySubmission submission = MakeGatewayTestSubmission();
	GatewayUploadResult good = ReadGatewayUploadResponse(transport(MakeGatewayUploadRequest(mock, submission, "Test Artist", "test-password"), {}));
	CHECK(good.Succeeded());
	CHECK(good.sceneryId == 900003);

	GatewayUploadResult bad = ReadGatewayUploadResponse(transport(MakeGatewayUploadRequest(mock, submission, "Test Artist", "wrong"), {}));
	CHECK(bad.outcome == GatewayUploadResult::Outcome::Unauthorized);
	CHECK(bad.message == "Invalid username or password (HTTP 401)");

	REQUIRE(seen.size() == 2);
	CHECK(seen[0] == "PUT http://127.0.0.1:8780/apiv1/scenery");
}

TEST_CASE("Gateway server: the offline transport never answers", "[xplane][gateway][account]")
{
	const GatewayTransport transport = MakeOfflineGatewayTransport("Offline for the test");
	GatewayHttpRequest request;
	request.url = "https://gateway.x-plane.com/apiv1/airports";
	const GatewayHttpResponse response = transport(request, {});
	CHECK_FALSE(response.completed);
	CHECK(response.status == 0);
	CHECK(response.error == "Offline for the test");
}

// -------------------------------------------------------
