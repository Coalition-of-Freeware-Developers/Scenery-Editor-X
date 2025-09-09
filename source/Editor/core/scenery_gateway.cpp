/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scenery_gateway.cpp
* -------------------------------------------------------
* Created: 26/4/2025
*/
#include <cstring>
#include <Editor/core/scenery_gateway.h>
#include <filesystem>
#include <fstream>
#include <sstream>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/// libcurl write callback
	size_t SceneryGateway::WriteCallback(const char *contents, size_t size, size_t nmemb, void *userdata)
	{
	    size_t realSize = size * nmemb;
	    FILE *file = static_cast<FILE *>(userdata);
	    return fwrite(contents, size, nmemb, file);
	}

	/// libcurl progress callback
	int SceneryGateway::ProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
	{
	    /// Avoid division by zero
	    if (dltotal <= 0)
	        return 0;

	    SceneryGateway *gateway = static_cast<SceneryGateway *>(clientp);
	    if (gateway && gateway->downloadStatus_.progressCb)
	    {
	        double progress = dlnow / dltotal;
	        gateway->downloadStatus_.progressCb(progress);
	    }

	    return 0; /// Return 0 to continue, non-zero to abort
	}

    // ----------------------------------------

	/// Utility function to URL encode a string
    static std::string UrlEncode(const std::string& input)
	{
	    CURL* curl = curl_easy_init();
	    std::string result;

	    if (curl)
	    {
            if (char* encoded = curl_easy_escape(curl, input.c_str(), static_cast<int>(input.length())))
	        {
	            result = encoded;
	            curl_free(encoded);
	        }
	        curl_easy_cleanup(curl);
	    }

	    return result;
	}

	/// Utility function to build a full URL for an API endpoint
    static std::string BuildUrl(const std::string &endpoint)
	{
	    return std::string(GATEWAY_API_URL) + endpoint;
	}

	/// Utility function to format a URL with parameters
    static std::string FormatUrl(const char *format, const std::string &param)
	{
	    char buffer[1024];
	    snprintf(buffer, sizeof(buffer), format, param.c_str());
	    return std::string(buffer);
	}

	/// Clear all cached data in SceneryGatewayData
	void SceneryGatewayData::ClearCache()
	{
	    airports.clear();
	    sceneryPacks.clear();
	    artists.clear();
	}

	/// Create AirportInfo from JSON
	AirportInfo AirportInfo::FromJson(const nlohmann::json& json)
	{
	    AirportInfo info;

	    info.icao = json.value("icao", "");
	    info.name = json.value("name", "");
	    info.latitude = json.value("latitude", 0.0);
	    info.longitude = json.value("longitude", 0.0);
	    info.elevation = json.value("elevation_ft", 0);
	    info.country = json.value("country", "");
	    info.state = json.value("state", "");
	    info.city = json.value("city", "");
	    info.author = json.value("author", "");
	    info.dateApproved = json.value("dateApproved", "");
	    info.totalSceneryPacks = json.value("totalSceneryPacks", 0);
	    info.recommendedSceneryId = json.value("recommendedSceneryId", "");

	    return info;
	}

	/// Create SceneryPackInfo from JSON
	SceneryPackInfo SceneryPackInfo::FromJson(const nlohmann::json& json)
	{
	    SceneryPackInfo info;

	    info.id = json.value("id", 0);
	    info.icao = json.value("icao", "");
	    info.name = json.value("name", "");
	    info.artistId = json.value("artistId", 0);
	    info.artistName = json.value("artistName", "");
	    info.dateApproved = json.value("dateApproved", "");
	    info.description = json.value("description", "");
	    info.status = json.value("status", "");
	    info.downloadUrl = json.value("downloadUrl", "");
	    info.downloadCount = json.value("downloadCount", 0);
	    info.fileSizeMB = json.value("fileSizeMB", 0.0);

	    return info;
	}

	/// Create ArtistInfo from JSON
	ArtistInfo ArtistInfo::FromJson(const nlohmann::json& json)
	{
	    ArtistInfo info;

	    info.id = json.value("id", 0);
	    info.name = json.value("name", "");
	    info.email = json.value("email", "");
	    info.description = json.value("description", "");

	    if (json.contains("contributions") && json["contributions"].is_array())
	    {
	        for (const auto& contribution : json["contributions"])
	        {
	            info.contributions.push_back(contribution);
	        }
	    }

	    return info;
	}

    /// Make a request to the Gateway API
    ApiResponse SceneryGateway::MakeRequest(const std::string &endpoint, bool useAuth) const
    {
        ApiResponse response;

        if (useAuth && !IsAuthenticated())
        {
            response.errorCode = GatewayErrorCode::AuthenticationError;
            response.errorMessage = "Authentication required but not authenticated";
            return response;
        }

        /// Build the full URL
        std::string url = BuildUrl(endpoint);

        /// Store the last request URL
        data_->lastRequestUrl = url;

        /// Setup curl request
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);

        // TODO: Set up any authentication headers if useAuth is true

        /// Prepare to capture response
        std::string responseData;
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION,[](const char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t
		{
			std::string *responseDataPtr = static_cast<std::string *>(userdata);
			responseDataPtr->append(ptr, size * nmemb);
			return size * nmemb;
        });
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &responseData);

        /// Perform the request
        CURLcode res = curl_easy_perform(curl_);

        /// Handle response
        if (res != CURLE_OK)
        {
            response.errorCode = GatewayErrorCode::NetworkError;
            response.errorMessage = curl_easy_strerror(res);
        }
        else
        {
            long httpCode = 0;
            curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &httpCode);

            if (httpCode == 200)
            {
                try
                {
                    response.data = nlohmann::json::parse(responseData);
                }
                catch (const nlohmann::json::exception &e)
                {
                    response.errorCode = GatewayErrorCode::ParseError;
                    response.errorMessage = "Failed to parse JSON response: " + std::string(e.what());
                }
            }
            else if (httpCode == 404)
            {
                response.errorCode = GatewayErrorCode::NotFound;
                response.errorMessage = "Resource not found";
            }
            else if (httpCode == 401)
            {
                response.errorCode = GatewayErrorCode::AuthenticationError;
                response.errorMessage = "Authentication failed";
            }
            else
            {
                response.errorCode = GatewayErrorCode::Unknown;
                response.errorMessage = "HTTP error code: " + std::to_string(httpCode);
            }
        }

        // Store the last response
        data_->lastResponse = response;

        return response;
    }

    /// Make an authenticated request to the Gateway API
    ApiResponse SceneryGateway::MakeAuthenticatedRequest(const std::string &endpoint) const
    {
        return MakeRequest(endpoint, true);
    }

    //////////////////////////////////////////////////////////////////////////////////
	/// SceneryGateway Implementation
    ///////////////////////////////////////////////////////////////////////////////

	/// Constructor
	SceneryGateway::SceneryGateway() : data_(std::make_unique<SceneryGatewayData>()), curl_(nullptr)
	{
	    /// Initialize libcurl
	    curl_global_init(CURL_GLOBAL_DEFAULT);
	    curl_ = curl_easy_init();
	}

	/// Destructor
	SceneryGateway::~SceneryGateway()
	{
	    /// Clean up any active download
	    if (downloadStatus_.fileHandle)
	    {
	        fclose(downloadStatus_.fileHandle);
	        downloadStatus_.fileHandle = nullptr;
	    }

	    /// Clean up libcurl
	    if (curl_)
	    {
	        curl_easy_cleanup(curl_);
	        curl_ = nullptr;
	    }

	    curl_global_cleanup();
	}

    // --------------------------------------------------------------

	/// Initialize with optional login
	bool SceneryGateway::Initialize(const std::string& login, const std::string& password) const
    {
	    if (!curl_)
	    {
	        return false;
	    }

	    /// Store login credentials if provided
	    if (!login.empty() && !password.empty())
	    {
	        return Login(login, password);
	    }

	    return true;
	}

	/// Login to the gateway
	bool SceneryGateway::Login(const std::string& login, const std::string& password) const
    {
	    data_->stats.userLogin = login;
	    data_->stats.userPassword = password;

	    /// Test authentication with a simple authenticated request
	    ApiResponse response = MakeAuthenticatedRequest(ENDPOINT_AIRPORTS);

	    data_->stats.isAuthenticated = (response.errorCode == GatewayErrorCode::Success);
	    return data_->stats.isAuthenticated;
	}

	/// Logout from the gateway
	void SceneryGateway::Logout() const
    {
	    data_->stats.userLogin.clear();
	    data_->stats.userPassword.clear();
	    data_->stats.isAuthenticated = false;
	}

	/// Check if authenticated
	bool SceneryGateway::IsAuthenticated() const
	{
	    return data_->stats.isAuthenticated;
	}

    // --------------------------------------------------------------

	/// Get a list of all airports
	ApiResponse SceneryGateway::GetAirports() const
    {
	    ApiResponse response = MakeRequest(ENDPOINT_AIRPORTS);

	    if (response.errorCode == GatewayErrorCode::Success)
	    {
	        /// Parse the airports from the response
	        data_->airports.clear();

	        if (response.data.contains("airports") && response.data["airports"].is_array())
	        {
	            for (const auto& airportJson : response.data["airports"])
	            {
	                data_->airports.push_back(AirportInfo::FromJson(airportJson));
	            }
	        }
	    }

	    return response;
	}

	/// Get information about a specific airport
	ApiResponse SceneryGateway::GetAirport(const std::string& icao) const
    {
	    std::string endpoint = FormatUrl(ENDPOINT_AIRPORT, icao);
	    ApiResponse response = MakeRequest(endpoint);

	    return response;
	}

	/// Get a list of all scenery packs, optionally filtered by ICAO
	ApiResponse SceneryGateway::GetSceneryPacks(const std::string& icaoFilter) const
    {
	    std::string endpoint = ENDPOINT_SCENERY;

	    if (!icaoFilter.empty())
	    {
	        endpoint += "?icao=" + UrlEncode(icaoFilter);
	    }

	    ApiResponse response = MakeRequest(endpoint);

	    if (response.errorCode == GatewayErrorCode::Success)
	    {
	        /// Parse the scenery packs from the response
	        data_->sceneryPacks.clear();

	        if (response.data.contains("scenery") && response.data["scenery"].is_array())
	        {
	            for (const auto& sceneryJson : response.data["scenery"])
	            {
	                data_->sceneryPacks.push_back(SceneryPackInfo::FromJson(sceneryJson));
	            }
	        }
	    }

	    return response;
	}

	/// Get information about a specific scenery pack
	ApiResponse SceneryGateway::GetSceneryPack(int id) const
    {
	    char buffer[64];
	    snprintf(buffer, sizeof(buffer), ENDPOINT_SCENERY_PACK, id);

	    ApiResponse response = MakeRequest(buffer);

	    return response;
	}

	/// Get a list of all artists
	ApiResponse SceneryGateway::GetArtists() const
    {
	    ApiResponse response = MakeRequest(ENDPOINT_ARTISTS);

	    if (response.errorCode == GatewayErrorCode::Success)
	    {
	        /// Parse the artists from the response
	        data_->artists.clear();

	        if (response.data.contains("artists") && response.data["artists"].is_array())
	        {
	            for (const auto& artistJson : response.data["artists"])
	            {
	                data_->artists.push_back(ArtistInfo::FromJson(artistJson));
	            }
	        }
	    }

	    return response;
	}

	/// Get information about a specific artist
	ApiResponse SceneryGateway::GetArtist(int id) const
    {
	    char buffer[64];
	    snprintf(buffer, sizeof(buffer), ENDPOINT_ARTIST, id);

	    ApiResponse response = MakeRequest(buffer);

	    return response;
	}

	/// Get recommended scenery
	ApiResponse SceneryGateway::GetRecommendedScenery() const
    {
	    ApiResponse response = MakeRequest(ENDPOINT_RECOMMENDED);

	    return response;
	}

    /// Download a scenery pack
    bool SceneryGateway::DownloadSceneryPack(int id, const std::string &saveDir, const SceneryEditorX::ProgressCallback &progressCb, const CompletionCallback &completionCb)
    {
        if (downloadStatus_.inProgress)
        {
            if (completionCb)
            {
                completionCb(false, "Another download is already in progress");
            }
            return false;
        }

        /// Get the scenery pack information first
        ApiResponse response = this->GetSceneryPack(id);
        if (response.errorCode != GatewayErrorCode::Success)
        {
            if (completionCb)
            {
                completionCb(false, "Failed to get scenery pack information: " + response.errorMessage);
            }
            return false;
        }

        std::string downloadUrl;
        std::string filename;

        /// Extract download URL from the response
        if (response.data.contains("downloadUrl") && response.data["downloadUrl"].is_string())
        {
            downloadUrl = response.data["downloadUrl"];
        }
        else
        {
            if (completionCb)
            {
                completionCb(false, "No download URL found in the scenery pack information");
            }
            return false;
        }

        /// Extract filename from download URL
        size_t lastSlash = downloadUrl.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            filename = downloadUrl.substr(lastSlash + 1);
        }
        else
        {
            filename = "scenery_" + std::to_string(id) + ".zip";
        }

        /// Ensure the save directory exists
        std::filesystem::path savePath(saveDir);
        if (!std::filesystem::exists(savePath))
        {
            std::filesystem::create_directories(savePath);
        }

        /// Create the full path for the file
        std::string fullPath = (savePath / filename).string();

        /// Open file for writing
        FILE *fp = fopen(fullPath.c_str(), "wb");
        if (!fp)
        {
            if (completionCb)
            {
                completionCb(false, "Failed to open file for writing: " + fullPath);
            }
            return false;
        }

        /// Set up download status
        downloadStatus_.inProgress = true;
        downloadStatus_.targetFile = fullPath;
        downloadStatus_.fileHandle = fp;
        downloadStatus_.progressCb = progressCb;
        downloadStatus_.completionCb = completionCb;

        /// Set up libcurl for the download
        CURL *curl = curl_easy_init();
        if (!curl)
        {
            fclose(fp);
            downloadStatus_.inProgress = false;
            downloadStatus_.fileHandle = nullptr;

            if (completionCb)
            {
                completionCb(false, "Failed to initialize libcurl for download");
            }
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, downloadUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        /// Set up progress callback if provided
        if (progressCb)
        {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, SceneryGateway::ProgressCallback);
            curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
        }

        /// Perform the download
        CURLcode res = curl_easy_perform(curl);

        /// Clean up
        curl_easy_cleanup(curl);
        fclose(fp);
        downloadStatus_.inProgress = false;
        downloadStatus_.fileHandle = nullptr;

        /// Handle result
        if (res != CURLE_OK)
        {
            if (completionCb)
            {
                completionCb(false, "Download failed: " + std::string(curl_easy_strerror(res)));
            }
            return false;
        }

        if (completionCb)
        {
            completionCb(true, "Download completed successfully: " + fullPath);
        }
        return true;
    }

	// -----------------------------------------------------------

	/// Get the cached airports
	const std::vector<AirportInfo>& SceneryGateway::GetCachedAirports() const
	{
	    return data_->airports;
	}

	/// Get the cached scenery packs
	const std::vector<SceneryPackInfo>& SceneryGateway::GetCachedSceneryPacks() const
	{
	    return data_->sceneryPacks;
	}

	/// Get the cached artists
	const std::vector<ArtistInfo>& SceneryGateway::GetCachedArtists() const
	{
	    return data_->artists;
	}

	/// Get the last error code
	GatewayErrorCode SceneryGateway::GetLastErrorCode() const
	{
	    return data_->lastResponse.errorCode;
	}

	/// Get the last error message
	std::string SceneryGateway::GetLastErrorMessage() const
	{
	    return data_->lastResponse.errorMessage;
	}

    // -------------------------------------------------------------

	/// Save gateway data to a JSON file
    bool SceneryGateway::SaveToFile(const std::string &filePath) const
    {
        try
        {
            nlohmann::json data;

            // Create arrays for each data type
            nlohmann::json airportsJson = nlohmann::json::array();
            nlohmann::json sceneryPacksJson = nlohmann::json::array();
            nlohmann::json artistsJson = nlohmann::json::array();

            // Add each airport as a JSON object
            for (const auto &airport : data_->airports)
            {
                nlohmann::json airportJson;
                airportJson["icao"] = airport.icao;
                airportJson["name"] = airport.name;
                airportJson["latitude"] = airport.latitude;
                airportJson["longitude"] = airport.longitude;
                airportJson["elevation"] = airport.elevation;
                airportJson["country"] = airport.country;
                airportJson["state"] = airport.state;
                airportJson["city"] = airport.city;
                airportJson["author"] = airport.author;
                airportJson["dateApproved"] = airport.dateApproved;
                airportJson["totalSceneryPacks"] = airport.totalSceneryPacks;
                airportJson["recommendedSceneryId"] = airport.recommendedSceneryId;
                airportsJson.push_back(airportJson);
            }

            // Add each scenery pack as a JSON object
            for (const auto &pack : data_->sceneryPacks)
            {
                nlohmann::json packJson;
                packJson["id"] = pack.id;
                packJson["icao"] = pack.icao;
                packJson["name"] = pack.name;
                packJson["artistId"] = pack.artistId;
                packJson["artistName"] = pack.artistName;
                packJson["dateApproved"] = pack.dateApproved;
                packJson["description"] = pack.description;
                packJson["status"] = pack.status;
                packJson["downloadUrl"] = pack.downloadUrl;
                packJson["downloadCount"] = pack.downloadCount;
                packJson["fileSizeMB"] = pack.fileSizeMB;
                sceneryPacksJson.push_back(packJson);
            }

            // Add each artist as a JSON object
            for (const auto &artist : data_->artists)
            {
                nlohmann::json artistJson;
                artistJson["id"] = artist.id;
                artistJson["name"] = artist.name;
                artistJson["email"] = artist.email;
                artistJson["description"] = artist.description;
                artistJson["contributions"] = artist.contributions;
                artistsJson.push_back(artistJson);
            }

            // Add all arrays to the main JSON object
            data["airports"] = airportsJson;
            data["sceneryPacks"] = sceneryPacksJson;
            data["artists"] = artistsJson;

            // Write the JSON to file
            std::ofstream file(filePath);
            if (!file.is_open())
            {
                return false;
            }

            file << data.dump(4); /// Pretty-print with 4-space indentation
            file.close();

            return true;
        }
        catch (const std::exception &e)
        {
            EDITOR_ERROR_TAG("SCENERY GATEWAY", "Failed to save gateway data to file: {}", e.what());
            return false;
        }
    }

	/// Load gateway data from a JSON file
    bool SceneryGateway::LoadFromFile(const std::string &filePath) const
    {
        try
        {
            /// Open the file
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                return false;
            }

            /// Parse the JSON
            nlohmann::json data = nlohmann::json::parse(file);

            /// Clear existing data
            data_->ClearCache();

            /// Load airports
            if (data.contains("airports") && data["airports"].is_array())
            {
                for (const auto &airportJson : data["airports"])
                {
                    data_->airports.push_back(AirportInfo::FromJson(airportJson));
                }
            }

            /// Load scenery packs
            if (data.contains("sceneryPacks") && data["sceneryPacks"].is_array())
            {
                for (const auto &packJson : data["sceneryPacks"])
                {
                    data_->sceneryPacks.push_back(SceneryPackInfo::FromJson(packJson));
                }
            }

            /// Load artists
            if (data.contains("artists") && data["artists"].is_array())
            {
                for (const auto &artistJson : data["artists"])
                {
                    data_->artists.push_back(ArtistInfo::FromJson(artistJson));
                }
            }

            return true;
        }
        catch (const std::exception &e)
        {
            EDITOR_ERROR_TAG("SCENERY GATEWAY", "Failed to load gateway data from file: {}", e.what());
            return false;
        }
    }

    // -------------------------------------------------------------

} // namespace SceneryEditorX

// -----------------------------------------------------------
