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
 * xp_airport_index.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_apt.h"
#include "xp_library.h"
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * A searchable catalog of every airport in the apt.dat files X-Plane loads (Global Airports plus each custom
	 * scenery pack), without parsing them in full: only the header, metadata and runway rows are read while scanning,
	 * and the byte range of each airport is remembered so it can be loaded on its own later. Standard library only.
	 */

	/* @brief An apt.dat file and the scenery pack it belongs to. */
	struct AirportSource
	{
		std::filesystem::path aptDat;			// ".../Earth nav data/apt.dat"
		std::filesystem::path package;			// The scenery pack folder
		std::string packageName;
		PackageSource source = PackageSource::CustomScenery;
		bool enabledInSim = true;				// Not SCENERY_PACK_DISABLED
		int priority = 0;						// Lower loads over higher (custom packs before Global Airports)

		// Filled in by the scan
		uint64_t fileSize = 0;
		int64_t fileTime = 0;					// File write time, for cache validation
		uint32_t airportCount = 0;
		std::string error;						// Why the file couldn't be scanned (empty when it could)
	};

	/* @brief One airport header found in a source. */
	struct AirportEntry
	{
		std::string id;							// Header id ("KSEA")
		std::string name;
		AirportType type = AirportType::Land;
		double latitude = 0.0;
		double longitude = 0.0;
		bool hasPosition = false;				// From datum metadata or the first runway / helipad
		std::string icao;						// 1302 icao_code
		std::string iata;						// 1302 iata_code
		std::string faa;						// 1302 faa_code
		std::string local;						// 1302 local_code
		std::string city;
		std::string state;
		std::string country;
		bool is3D = false;						// 1302 gui_label 3D
		uint32_t source = 0;					// Index into AirportIndex::GetSources()
		uint64_t offset = 0;					// Byte offset of the header row in the apt.dat
		uint64_t length = 0;					// Bytes up to the next header (or 99)
		uint32_t line = 0;						// 1-based line of the header row
		uint16_t runwayCount = 0;
		uint16_t waterRunwayCount = 0;
		uint16_t helipadCount = 0;

		/* @brief "KSEA  Seattle Tacoma Intl" style label. */
		[[nodiscard]] std::string GetLabel() const;

		/* @brief Codes other than the id, comma separated ("ICAO KSEA, IATA SEA, FAA SEA"). */
		[[nodiscard]] std::string GetCodes() const;
	};

	/**
	 * @class AirportIndex
	 * @brief Airports from a set of apt.dat sources, searchable by code, name and place.
	 */
	class AirportIndex
	{
	public:
		struct SearchOptions
		{
			size_t maxResults = 500;
			int sourceFilter = -1;				// Only this source index, or -1 for all
			bool includeDisabled = true;		// Packs X-Plane has disabled
			bool only3D = false;
			std::optional<AirportType> type;
		};

		/**
		 * @brief Scans an apt.dat and adds its airports. When `cache` holds the same file (path, size, write time) its
		 * entries are copied instead of re-scanning, which keeps the 380 MB Global Airports file off the disk at startup.
		 * @param cancel Set from another thread to stop a long scan early (the source is then left out).
		 * @return False when the file couldn't be read (the source is still recorded, with its error).
		 */
		bool AddSource(AirportSource source, const AirportIndex *cache = nullptr, const std::atomic<bool> *cancel = nullptr);

		/* @brief Sorts the airports and builds the lookup tables. Call after the last AddSource(). */
		void Finish();

		[[nodiscard]] const std::vector<AirportSource> &GetSources() const { return m_Sources; }
		[[nodiscard]] const std::vector<AirportEntry> &GetAirports() const { return m_Airports; }

		/**
		 * @brief Airports matching a query: every word must match a code (prefix) or the name / city / state / country
		 * (substring), case-insensitively. Exact code matches come first, then code prefixes, then name matches; ties
		 * go to the higher-priority source. An empty query lists everything up to maxResults.
		 */
		void Search(std::string_view query, const SearchOptions &options, std::vector<uint32_t> &outEntries) const;

		/* @brief Every entry with this id (case-insensitive), highest priority source first. */
		void FindById(std::string_view id, std::vector<uint32_t> &outEntries) const;

		/* @brief The entry X-Plane would load for an id (highest priority enabled source), or nullptr. */
		[[nodiscard]] const AirportEntry *FindBest(std::string_view id) const;

		/* @brief Entries within a radius of a coordinate, nearest first. */
		void FindNear(double latitude, double longitude, double radiusMeters, std::vector<uint32_t> &outEntries, size_t maxResults = 100) const;

		/* @brief Reads one airport from its apt.dat (just its byte range). */
		bool LoadAirport(const AirportEntry &entry, AptAirport &out, std::string *error = nullptr) const;

		/* @brief Number of entries per source, 3D entries, ... for status lines. */
		[[nodiscard]] size_t Get3DCount() const { return m_Count3D; }

		// -------------------------------------------------------

		/* @brief Writes the index to a binary cache file. */
		bool SaveCache(const std::filesystem::path &path) const;

		/* @brief Reads a cache written by SaveCache(). False (and empty) when the file is missing or from another version. */
		bool LoadCache(const std::filesystem::path &path);

		/* @brief Scans an apt.dat file on its own: the entries found, with `sourceIndex` as their source. */
		static bool ScanAptDat(const std::filesystem::path &aptDat, uint32_t sourceIndex, std::vector<AirportEntry> &outEntries, std::string *error = nullptr, const std::atomic<bool> *cancel = nullptr);

		/* @brief Scans apt.dat text (for tests and in-memory files). */
		static void ScanAptText(std::string_view text, uint32_t sourceIndex, std::vector<AirportEntry> &outEntries, uint64_t baseOffset = 0, uint32_t baseLine = 0);

	private:
		std::vector<AirportSource> m_Sources;
		std::vector<AirportEntry> m_Airports;
		std::vector<std::string> m_SearchText;					// Lower-case "codes|name city state country" per entry
		std::unordered_map<std::string, std::vector<uint32_t>> m_ById;	// Lower-case id -> entries, by priority
		size_t m_Count3D = 0;
	};

}

// -------------------------------------------------------
