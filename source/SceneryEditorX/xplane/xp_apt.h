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
 * xp_apt.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * X-Plane airport data (apt.dat 12.00), per
	 * https://developer.x-plane.com/article/airport-data-apt-dat-12-00-file-format-specification/
	 *
	 * The structures mirror the file rows one to one so a file can be read, edited and written back without losing
	 * anything X-Plane or WorldEditor put there (unknown row codes are kept verbatim). Standard library only, so the
	 * parser and writer are unit tested without the engine.
	 *
	 * Coordinates are WGS84 decimal degrees (north and east positive), headings true degrees, lengths meters unless a
	 * field says otherwise.
	 */

	/* @brief Header row codes 1 / 16 / 17. */
	enum class AirportType : uint8_t
	{
		Land = 1,
		Seaplane = 16,
		Heliport = 17
	};

	const char *GetAirportTypeName(AirportType type);

	/* @brief Surface codes (runways, helipads, pavement). The 2x / 5x codes are the X-Plane 12 asphalt / concrete variants. */
	enum AptSurface : int
	{
		SurfaceAsphalt = 1,
		SurfaceConcrete = 2,
		SurfaceGrass = 3,
		SurfaceDirt = 4,
		SurfaceGravel = 5,
		SurfaceDryLakebed = 12,
		SurfaceWater = 13,
		SurfaceSnow = 14,
		SurfaceTransparent = 15
	};

	/* @brief "Asphalt", "Concrete", "Light asphalt 1", ... */
	const char *GetSurfaceName(int surface);

	/* @brief The family a surface code belongs to, for coloring. */
	enum class SurfaceFamily : uint8_t
	{
		Asphalt,
		Concrete,
		Grass,
		Dirt,
		Gravel,
		Lakebed,
		Water,
		Snow,
		Transparent,
		Unknown
	};

	SurfaceFamily GetSurfaceFamily(int surface);

	/* @brief Every surface code X-Plane 12 accepts, in display order. */
	const std::vector<int> &GetSurfaceCodes();

	const char *GetRunwayMarkingName(int marking);			// 0..7
	const char *GetApproachLightsName(int code);			// 0..12
	const char *GetReilName(int code);						// 0..2
	const char *GetEdgeLightsName(int code);				// 0, 2
	const char *GetBeaconTypeName(int type);				// 0..4
	const char *GetSignSizeName(int size);					// 1..5
	const char *GetLightFixtureName(int type);				// 1..8
	const char *GetHelipadEdgeLightsName(int code);			// 0, 1

	/* @brief Painted line / light string type for node rows ("Solid yellow", "Taxiway centerline lights", ...). */
	const char *GetLineTypeName(int type);
	bool IsLightLineType(int type);							// 100+ = light string

	/* @brief Painted line codes and light string codes X-Plane 12 accepts, in display order. */
	const std::vector<int> &GetPaintedLineCodes();
	const std::vector<int> &GetLightLineCodes();

	// -------------------------------------------------------

	/* @brief One end of a land runway (row 100). */
	struct AptRunwayEnd
	{
		std::string designator = "01";		// "09L", "27", ...
		double latitude = 0.0;
		double longitude = 0.0;
		float displacedThreshold = 0.0f;	// Meters inside the runway
		float blastPad = 0.0f;				// Meters past the end
		int marking = 1;					// GetRunwayMarkingName
		int approachLights = 0;				// GetApproachLightsName
		bool touchdownZoneLights = false;
		int reil = 0;						// GetReilName
	};

	/* @brief Row 100. */
	struct AptRunway
	{
		float width = 30.0f;
		int surface = SurfaceAsphalt;
		int shoulder = 0;					// Surface code, or surface + 100 * shoulder width (12.00)
		float smoothness = 0.25f;
		bool centerlineLights = false;
		int edgeLights = 0;					// 0 none, 2 medium intensity
		bool distanceSigns = false;
		AptRunwayEnd ends[2];

		[[nodiscard]] int GetShoulderSurface() const { return shoulder % 100; }
		[[nodiscard]] float GetShoulderWidth() const { return shoulder >= 100 ? static_cast<float>(shoulder / 100) : 0.0f; }
		[[nodiscard]] std::string GetName() const { return ends[0].designator + "/" + ends[1].designator; }
		[[nodiscard]] double GetLengthMeters() const;
		[[nodiscard]] double GetHeading() const;			// True heading from end 0 to end 1
	};

	/* @brief Row 101. */
	struct AptWaterRunway
	{
		struct End
		{
			std::string designator;
			double latitude = 0.0;
			double longitude = 0.0;
		};

		float width = 30.0f;
		bool buoys = false;
		End ends[2];

		[[nodiscard]] std::string GetName() const { return ends[0].designator + "/" + ends[1].designator; }
	};

	/* @brief Row 102. */
	struct AptHelipad
	{
		std::string designator = "H1";
		double latitude = 0.0;
		double longitude = 0.0;
		float heading = 0.0f;
		float length = 20.0f;
		float width = 20.0f;
		int surface = SurfaceConcrete;
		int markings = 0;
		int shoulder = 0;					// 0 none, 1 asphalt, 2 concrete
		float smoothness = 0.25f;
		int edgeLights = 0;					// 0 none, 1 yellow
	};

	/* @brief A node of a pavement, line or boundary (rows 111..116). */
	struct AptNode
	{
		double latitude = 0.0;
		double longitude = 0.0;
		bool hasControl = false;			// 112 / 114 / 116: a bezier control point follows
		double controlLatitude = 0.0;		// Outgoing control point; the incoming one is its mirror through the node
		double controlLongitude = 0.0;
		int lineType = 0;					// Painted line starting here (GetLineTypeName), 0 none
		int lightType = 0;					// Light string starting here, 0 none
	};

	/**
	 * @brief A run of nodes: a closed ring (ends with 113 / 114) or an open string (ends with 115 / 116).
	 * Pavement and boundary rings are closed; the first ring is the outline, later rings are holes.
	 */
	struct AptNodeChain
	{
		std::vector<AptNode> nodes;
		bool closed = true;
	};

	/* @brief Row 110 and its nodes: a taxiway / apron polygon. */
	struct AptPavement
	{
		int surface = SurfaceAsphalt;
		float smoothness = 0.25f;
		float textureHeading = 0.0f;		// Texture grain orientation
		std::string description;
		std::vector<AptNodeChain> rings;	// rings[0] outline (counter-clockwise), the rest holes
	};

	/* @brief Row 120 and its nodes: painted lines and / or light strings. */
	struct AptLinearFeature
	{
		std::string description;
		std::vector<AptNodeChain> chains;
	};

	/* @brief Row 130 and its nodes: the airport boundary (closed). */
	struct AptBoundary
	{
		std::string description;
		std::vector<AptNodeChain> rings;
	};

	/* @brief Row 14 (at most one per airport). */
	struct AptViewpoint
	{
		double latitude = 0.0;
		double longitude = 0.0;
		float heightFt = 50.0f;
		std::string name = "Tower Viewpoint";
	};

	/* @brief Row 15 (deprecated startup location, kept for old files). */
	struct AptLegacyStartup
	{
		double latitude = 0.0;
		double longitude = 0.0;
		float heading = 0.0f;
		std::string name;
	};

	/* @brief Row 18 (at most one per airport). */
	struct AptBeacon
	{
		double latitude = 0.0;
		double longitude = 0.0;
		int type = 1;						// GetBeaconTypeName
		std::string name;
	};

	/* @brief Row 19. */
	struct AptWindsock
	{
		double latitude = 0.0;
		double longitude = 0.0;
		bool lit = true;
		std::string name;
	};

	/* @brief Row 20: a taxiway sign. text uses X-Plane's sign syntax ("{@Y}A{@L}22{^r}"). */
	struct AptSign
	{
		double latitude = 0.0;
		double longitude = 0.0;
		float heading = 0.0f;				// Orientation of the sign face
		int size = 2;						// GetSignSizeName
		std::string text;
	};

	/* @brief Row 21: VASI / PAPI / wig-wag. */
	struct AptLightFixture
	{
		double latitude = 0.0;
		double longitude = 0.0;
		int type = 2;						// GetLightFixtureName
		float heading = 0.0f;
		float glideslope = 3.0f;			// Degrees, 0 when not applicable
		std::string name;
	};

	/* @brief Rows 1050..1056 (8.33 kHz, X-Plane 12) and the legacy 50..56 rows. */
	struct AptFrequency
	{
		enum class Type : uint8_t
		{
			Recorded = 0,	// AWOS / ASOS / ATIS
			Unicom,			// Unicom / CTAF / Radio
			Clearance,
			Ground,
			Tower,
			Approach,
			Departure
		};

		Type type = Type::Tower;
		int frequencyKhz = 118000;			// 118000..136990
		std::string name;
		bool legacy = false;				// Written as 50..56 (MHz * 100) instead of 1050..1056
	};

	const char *GetFrequencyTypeName(AptFrequency::Type type);

	/* @brief Row 1001. */
	struct AptWindRule
	{
		std::string station;
		int minDirection = 0;				// Magnetic, 0..359
		int maxDirection = 359;
		int maxSpeedKnots = 999;
	};

	/* @brief Row 1002. */
	struct AptCeilingRule
	{
		std::string station;
		int minCeilingFt = 0;
	};

	/* @brief Row 1003. */
	struct AptVisibilityRule
	{
		std::string station;
		float minVisibilitySm = 0.0f;
	};

	/* @brief Row 1004. */
	struct AptTimeRule
	{
		int startUtc = 0;					// hhmm
		int endUtc = 2400;
	};

	/* @brief Rows 1100 (10 kHz) / 1110 (1 kHz). */
	struct AptRunwayUse
	{
		std::string runway;					// Runway end ("28R")
		int frequencyKhz = 0;				// ATC frequency, 0 when none
		bool arrivals = true;
		bool departures = true;
		std::string aircraftTypes = "heavy|jets|turboprops|props|helos";
		int onCourseFrom = 0;				// Initial on-course heading range
		int onCourseTo = 359;
		int departureFrom = 0;				// Initial departure heading range
		int departureTo = 359;
		std::string name;
		bool highResolution = true;			// 1110 (6 digit kHz) rather than 1100 (5 digit 10 kHz)
	};

	/* @brief Row 1101. */
	struct AptVfrPattern
	{
		std::string runway;
		bool rightTraffic = false;
	};

	/* @brief Row 1000 and the rows under it. */
	struct AptFlow
	{
		std::string name;
		std::vector<AptWindRule> windRules;
		std::vector<AptCeilingRule> ceilingRules;
		std::vector<AptVisibilityRule> visibilityRules;
		std::vector<AptTimeRule> timeRules;
		std::vector<AptRunwayUse> runwayUses;
		std::vector<AptVfrPattern> vfrPatterns;
	};

	/* @brief Row 1201. */
	struct AptTaxiNode
	{
		double latitude = 0.0;
		double longitude = 0.0;
		std::string usage = "both";			// init, end / dest, both, junc
		int id = 0;
		std::string name;
	};

	/* @brief Row 1204, under its edge. */
	struct AptActiveZone
	{
		std::string classification;			// arrival, departure, ils
		std::string runways;				// "03,21"
	};

	/* @brief Rows 1202 (aircraft) / 1206 (ground vehicles). */
	struct AptTaxiEdge
	{
		int from = 0;
		int to = 0;
		bool oneWay = false;
		std::string restriction;			// "runway", "taxiway_D", empty for ground vehicle edges
		std::string name;
		bool groundVehicles = false;		// 1206
		std::vector<AptActiveZone> activeZones;
	};

	/* @brief Row 1200 and the rows under it. */
	struct AptTaxiNetwork
	{
		std::vector<AptTaxiNode> nodes;
		std::vector<AptTaxiEdge> edges;
	};

	/* @brief Rows 1300 + 1301. */
	struct AptStartup
	{
		double latitude = 0.0;
		double longitude = 0.0;
		float heading = 0.0f;
		std::string type = "gate";			// gate, hangar, misc, tie_down
		std::string aircraftTypes = "heavy|jets|turboprops|props|helos";
		std::string name;
		bool hasMetadata = false;			// A 1301 row follows
		std::string widthCode = "C";		// ICAO A..F
		std::string operationType = "none";	// none, general_aviation, airline, cargo, military
		std::string airlines;				// Space separated ICAO codes
	};

	/* @brief Row 1302. */
	struct AptMetadata
	{
		std::string key;
		std::string value;
	};

	/* @brief Rows 1400 + optional 1402. */
	struct AptTruckParking
	{
		double latitude = 0.0;
		double longitude = 0.0;
		float heading = 0.0f;
		std::string type = "gpu";			// baggage_loader, baggage_train, crew_car, crew_ferrari, crew_limo, pushback, fuel_liners, fuel_jets, fuel_props, food, gpu
		int count = 0;						// Cars for baggage_train, else 0
		std::string name;
		std::string customObject;			// 1402 vpath, empty for the default vehicle
	};

	/* @brief Row 1401. */
	struct AptTruckDestination
	{
		double latitude = 0.0;
		double longitude = 0.0;
		float heading = 0.0f;
		std::string truckTypes;				// "food|pushback"
		std::string name;
	};

	/* @brief Rows 1500 + optional 1501 / 1502. */
	struct AptJetway
	{
		double latitude = 0.0;
		double longitude = 0.0;
		float heading = 0.0f;				// Parked tunnel heading
		int style = 0;						// 0 light solid, 1 light glass, 2 dark solid, 3 dark glass
		int size = 1;						// 0: 11-23 m, 1: 14-29 m, 2: 17-38 m, 3: 20-47 m
		int reserved = 0;
		float tunnelLength = 20.0f;			// Parked length, must fit the size
		float cabinHeading = 0.0f;			// Parked cabin heading
		std::vector<std::string> customObjects;	// 1501 / 1502 vpaths
	};

	const char *GetJetwayStyleName(int style);
	const char *GetJetwaySizeName(int size);
	const char *GetStartupTypeName(std::string_view type);
	const char *GetTruckTypeName(std::string_view type);

	/* @brief Standard apt.dat vocabulary for combo boxes. */
	const std::vector<const char *> &GetStartupTypes();
	const std::vector<const char *> &GetOperationTypes();
	const std::vector<const char *> &GetTruckTypes();
	const std::vector<const char *> &GetAircraftTypes();	// heavy, jets, turboprops, props, helos, fighters
	const std::vector<const char *> &GetTaxiNodeUsages();

	// -------------------------------------------------------

	/* @brief Everything between one airport header and the next. */
	struct AptAirport
	{
		AirportType type = AirportType::Land;
		int elevationFt = 0;
		std::string id;						// Up to 7 characters, unique ("KSEA", "5S4", "XEN0026")
		std::string name;

		std::vector<AptMetadata> metadata;				// 1302, in file order
		std::vector<AptRunway> runways;					// 100
		std::vector<AptWaterRunway> waterRunways;		// 101
		std::vector<AptHelipad> helipads;				// 102
		std::vector<AptPavement> pavements;				// 110 (file order = top to bottom)
		std::vector<AptLinearFeature> linearFeatures;	// 120
		std::vector<AptBoundary> boundaries;			// 130
		std::optional<AptViewpoint> viewpoint;			// 14
		std::vector<AptLegacyStartup> legacyStartups;	// 15
		std::optional<AptBeacon> beacon;				// 18
		std::vector<AptWindsock> windsocks;				// 19
		std::vector<AptSign> signs;						// 20
		std::vector<AptLightFixture> lightFixtures;		// 21
		std::vector<AptFrequency> frequencies;			// 50..56, 1050..1056
		std::vector<AptFlow> flows;						// 1000
		std::optional<AptTaxiNetwork> taxiNetwork;		// 1200
		std::vector<AptStartup> startups;				// 1300
		std::vector<AptTruckParking> truckParkings;		// 1400
		std::vector<AptTruckDestination> truckDestinations;	// 1401
		std::vector<AptJetway> jetways;					// 1500
		std::vector<std::string> unknownRows;			// Rows with codes this reader doesn't know, written back verbatim

		uint32_t sourceLine = 0;						// Line of the header row in the parsed text (1-based), 0 when new

		/* @brief Value of a 1302 key, or empty. */
		[[nodiscard]] std::string_view GetMetadata(std::string_view key) const;
		[[nodiscard]] bool HasMetadata(std::string_view key) const;

		/* @brief Sets (or adds) a 1302 key. An empty value keeps the key with an empty value, like WorldEditor. */
		void SetMetadata(std::string_view key, std::string_view value);
		void RemoveMetadata(std::string_view key);

		/* @brief ICAO code from the metadata, falling back to the airport id. */
		[[nodiscard]] std::string GetIcao() const;

		/* @brief True when the airport has 3D scenery ("gui_label 3D"). */
		[[nodiscard]] bool Is3D() const;

		/**
		 * @brief The airport's reference point: datum_lat / datum_lon metadata when present, else the center of its
		 * runways, helipads and pavement. False when the airport has no geometry at all.
		 */
		bool GetDatum(double &latitude, double &longitude) const;

		/* @brief Bounding box of every coordinate in the airport. False when there is none. */
		bool GetBounds(double &south, double &west, double &north, double &east) const;

		/* @brief Number of coordinate-bearing rows, for the UI. */
		[[nodiscard]] size_t GetElementCount() const;
	};

	/* @brief A whole apt.dat. */
	struct AptFile
	{
		int version = 1200;
		std::string headerComment = "Generated by Scenery Editor X";	// Rest of the version row
		std::vector<AptAirport> airports;
		std::vector<std::string> warnings;	// Rows that were skipped or guessed at, with line numbers
	};

	// -------------------------------------------------------

	struct AptParseOptions
	{
		bool requireHeader = true;			// False accepts text that starts at an airport header (index excerpts)
		bool keepUnknownRows = true;
	};

	/**
	 * @brief Parses apt.dat text. Stops at the 99 terminator (or the end of the text).
	 * @return False when the text isn't an apt.dat at all (bad header); row-level problems become warnings.
	 */
	bool ParseAptDat(std::string_view text, AptFile &out, std::string *error = nullptr, const AptParseOptions &options = {});

	/* @brief Reads and parses an apt.dat file. */
	bool LoadAptDat(const std::filesystem::path &path, AptFile &out, std::string *error = nullptr);

	/* @brief Writes one airport (header row to its last row) in WorldEditor's row order. */
	void WriteAptAirport(std::string &out, const AptAirport &airport);

	/* @brief Formats a complete apt.dat (header, airports, 99). LF line endings. */
	std::string WriteAptDat(const AptFile &file);

	/* @brief Writes a complete apt.dat, creating folders as needed. */
	bool SaveAptDat(const std::filesystem::path &path, const AptFile &file, std::string *error = nullptr);

	// -------------------------------------------------------

	/**
	 * @brief Checks an airport for the problems X-Plane and the Gateway reject: missing id / name, bad coordinates,
	 * runways under 1 m, unclosed pavement, startup names that repeat, taxi edges to unknown nodes, ...
	 * @return Problems found (empty when the airport is valid).
	 */
	std::vector<std::string> ValidateAptAirport(const AptAirport &airport);

	/* @brief Distance in meters along the great circle between two coordinates (WGS84 mean radius). */
	double GreatCircleDistance(double lat1, double lon1, double lat2, double lon2);

	/* @brief Initial true heading (0..360) from one coordinate to another. */
	double InitialHeading(double lat1, double lon1, double lat2, double lon2);

	/* @brief The coordinate `meters` away from (lat, lon) along a true heading. */
	void MovePoint(double lat, double lon, double headingDegrees, double meters, double &outLat, double &outLon);
}

// -------------------------------------------------------
