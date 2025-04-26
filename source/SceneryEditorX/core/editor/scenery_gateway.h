/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scenery_gateway.h
* -------------------------------------------------------
* Created: 26/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <curl/curl.h>
#include <json.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

// -----------------------------------------

namespace SceneryEditorX
{

    // -----------------------------------------

    /// Base URL for X-Plane Scenery Gateway API
    constexpr const char* GATEWAY_API_URL = "https://gateway.x-plane.com/api";

    // -----------------------------------------

    /// API endpoints
    constexpr const char* ENDPOINT_AIRPORTS = "/airports";
    constexpr const char* ENDPOINT_AIRPORT = "/airport/%s";
    constexpr const char* ENDPOINT_SCENERY = "/scenery";
    constexpr const char* ENDPOINT_SCENERY_PACK = "/scenery/%d";
    constexpr const char* ENDPOINT_ARTISTS = "/artists";
    constexpr const char* ENDPOINT_ARTIST = "/artist/%d";
    constexpr const char* ENDPOINT_RECOMMENDED = "/apiv1/recommended.php";

    // -----------------------------------------

    /// Function callback types
    using ProgressCallback = std::function<void(double progress)>;
    using CompletionCallback = std::function<void(bool success, const std::string& message)>;

    // ----------------------------------------------

    /// Error codes
    enum class GatewayErrorCode : uint8_t
    {
        Success = 0,
        NetworkError,
        AuthenticationError,
        ParseError,
        NotFound,
        Unknown
    };

    /// Response structure
    struct ApiResponse
    {
        GatewayErrorCode errorCode = GatewayErrorCode::Success;
        std::string errorMessage;
        nlohmann::json data;
    };

    /// Gateway authentication information
    struct SceneryGatewayStats
    {
        std::string userLogin;
        std::string userPassword;
        uint32_t sceneryGatewayVersion;
        bool isAuthenticated = false;
    };

    struct AirportInfo
    {
        std::string icao;
        std::string name;
        double latitude;
        double longitude;
        int elevation;
        std::string country;
        std::string state;
        std::string city;
        std::string author;
        std::string dateApproved;
        int totalSceneryPacks;
        std::string recommendedSceneryId;
        
        static AirportInfo FromJson(const nlohmann::json& json);
    };

    struct SceneryPackInfo
    {
        int id;
        std::string icao;
        std::string name;
        int artistId;
        std::string artistName;
        std::string dateApproved;
        std::string description;
        std::string status;
        std::string downloadUrl;
        int downloadCount;
        double fileSizeMB;
        
        static SceneryPackInfo FromJson(const nlohmann::json& json);
    };

    struct ArtistInfo
    {
        int id;
        std::string name;
        std::string email;
        std::string description;
        std::vector<std::string> contributions;
        
        static ArtistInfo FromJson(const nlohmann::json& json);
    };

    struct SceneryGatewayData
    {
        /// Internal reference to the authentication information
        SceneryGatewayStats stats;
        
        /// Cached data
        std::vector<AirportInfo> airports;
        std::vector<SceneryPackInfo> sceneryPacks;
        std::vector<ArtistInfo> artists;
        
        /// Last response
        ApiResponse lastResponse;
        std::string lastRequestUrl;
        
        /// Clear all cached data
        void ClearCache();
    };

    // ----------------------------------------------------------

    class SceneryGateway
    {
    public:
        SceneryGateway();
        ~SceneryGateway();

        /// Initialize the gateway with optional login credentials
        bool Initialize(const std::string& login = "", const std::string& password = "") const;
        
        /// Authentication
        bool Login(const std::string& login, const std::string& password) const;
        void Logout() const;
        bool IsAuthenticated() const;

        /// API endpoints
        ApiResponse GetAirports() const;
        ApiResponse GetAirport(const std::string& icao) const;
        ApiResponse GetSceneryPacks(const std::string& icaoFilter = "") const;
        ApiResponse GetSceneryPack(int id) const;
        ApiResponse GetArtists() const;
        ApiResponse GetArtist(int id) const;
        ApiResponse GetRecommendedScenery() const;

        /// Download functionality
        bool DownloadSceneryPack(int id, const std::string& saveDir, 
            ProgressCallback progressCb = nullptr,
            CompletionCallback completionCb = nullptr);
        
        /// Access to cached data
        const std::vector<AirportInfo>& GetCachedAirports() const;
        const std::vector<SceneryPackInfo>& GetCachedSceneryPacks() const;
        const std::vector<ArtistInfo>& GetCachedArtists() const;
        
        /// Get the last error information
        GatewayErrorCode GetLastErrorCode() const;
        std::string GetLastErrorMessage() const;
        
        /// Save/load gateway data to/from a JSON file
        bool SaveToFile(const std::string& filePath) const;
        bool LoadFromFile(const std::string& filePath) const;

    private:
        /// Internal implementation
        ApiResponse MakeRequest(const std::string& endpoint, bool useAuth = false) const;
        ApiResponse MakeAuthenticatedRequest(const std::string& endpoint) const;
        
        /// libcurl callbacks
        static size_t WriteCallback(char* contents, size_t size, size_t nmemb, void* userdata);
        static int ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);

        /// Internal data
        std::unique_ptr<SceneryGatewayData> data_;
        CURL* curl_;
        
        /// Current download status
        struct DownloadStatus
        {
            bool inProgress = false;
            std::string targetFile;
            FILE* fileHandle = nullptr;
            SceneryEditorX::ProgressCallback progressCb;
            CompletionCallback completionCb;
        } downloadStatus_;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------
