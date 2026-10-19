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
 * xp_airport_index.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_airport_index.h"
#include "xp_text.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr char CACHE_MAGIC[8] = { 'S', 'E', 'D', 'X', 'A', 'P', 'T', 'I' };
		constexpr uint32_t CACHE_VERSION = 1;
		constexpr size_t SCAN_CHUNK = 4u << 20;	// 4 MB reads

		std::string PathUtf8(const std::filesystem::path &path)
		{
			const std::u8string text = path.u8string();
			return { reinterpret_cast<const char *>(text.data()), text.size() };
		}

		std::filesystem::path PathFromUtf8(const std::string &text)
		{
			return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t *>(text.data()), text.size()));
		}

		bool SameFile(const std::filesystem::path &a, const std::filesystem::path &b)
		{
			std::string ka = a.lexically_normal().generic_string();
			std::string kb = b.lexically_normal().generic_string();
#ifdef _WIN32
			ka = ToLower(ka);
			kb = ToLower(kb);
#endif
			return ka == kb;
		}

		void StatFile(AirportSource &source)
		{
			std::error_code ec;
			source.fileSize = std::filesystem::file_size(source.aptDat, ec);
			if (ec)
				source.fileSize = 0;
			const auto time = std::filesystem::last_write_time(source.aptDat, ec);
			source.fileTime = ec ? 0 : static_cast<int64_t>(time.time_since_epoch().count());
		}

		/* @brief Scanner state for one apt.dat: fed whole lines, builds entries. */
		struct Scanner
		{
			std::vector<AirportEntry> &entries;
			uint32_t sourceIndex;
			std::vector<std::string_view> tokens;
			bool haveEntry = false;
			bool haveDatum = false;
			uint64_t entryOffset = 0;
			double datumLat = 0.0;
			double datumLon = 0.0;

			Scanner(std::vector<AirportEntry> &out, const uint32_t source) : entries(out), sourceIndex(source) {}

			void Finish(const uint64_t endOffset)
			{
				if (!haveEntry)
					return;
				AirportEntry &entry = entries.back();
				entry.length = endOffset > entryOffset ? endOffset - entryOffset : 0;
				if (haveDatum)
				{
					entry.latitude = datumLat;
					entry.longitude = datumLon;
					entry.hasPosition = true;
				}
				haveEntry = false;
				haveDatum = false;
			}

			static bool ParseDouble(const std::string_view text, double &value)
			{
				if (text.empty() || text.size() > 31)
					return false;
				char buffer[32];
				std::memcpy(buffer, text.data(), text.size());
				buffer[text.size()] = '\0';
				char *end = nullptr;
				value = std::strtod(buffer, &end);
				return end == buffer + text.size();
			}

			void Position(const double latitude, const double longitude)
			{
				AirportEntry &entry = entries.back();
				if (entry.hasPosition || haveDatum)
					return;
				entry.latitude = latitude;
				entry.longitude = longitude;
				entry.hasPosition = true;
			}

			void Line(const std::string_view line, const uint64_t offset, const uint32_t lineNumber)
			{
				// Cheap rejects: only rows starting with '1' or "99" matter here.
				if (line.empty() || (line[0] != '1' && line[0] != '9'))
					return;

				Tokenize(line, tokens);
				if (tokens.empty())
					return;
				const std::string_view code = tokens[0];

				if (code == "1" || code == "16" || code == "17")
				{
					Finish(offset);
					AirportEntry entry;
					entry.type = code == "1" ? AirportType::Land : (code == "16" ? AirportType::Seaplane : AirportType::Heliport);
					entry.id = tokens.size() > 4 ? std::string(tokens[4]) : std::string();
					entry.name = std::string(RestOfLine(line, 5));
					entry.source = sourceIndex;
					entry.offset = offset;
					entry.line = lineNumber;
					entries.push_back(std::move(entry));
					entryOffset = offset;
					haveEntry = true;
					return;
				}

				if (!haveEntry)
					return;

				if (code == "99")
				{
					Finish(offset);
					return;
				}

				AirportEntry &entry = entries.back();
				if (code == "1302")
				{
					if (tokens.size() < 2)
						return;
					const std::string_view key = tokens[1];
					const std::string_view value = RestOfLine(line, 2);
					if (key == "icao_code") entry.icao = std::string(value);
					else if (key == "iata_code") entry.iata = std::string(value);
					else if (key == "faa_code") entry.faa = std::string(value);
					else if (key == "local_code") entry.local = std::string(value);
					else if (key == "city") entry.city = std::string(value);
					else if (key == "state") entry.state = std::string(value);
					else if (key == "country") entry.country = std::string(value);
					else if (key == "gui_label") entry.is3D = value == "3D";
					else if (key == "datum_lat") { double v; if (ParseDouble(value, v)) { datumLat = v; haveDatum = haveDatum || false; datumLat = v; } }
					else if (key == "datum_lon") { double v; if (ParseDouble(value, v)) { datumLon = v; } }
					if ((key == "datum_lat" || key == "datum_lon") && datumLat != 0.0 && datumLon != 0.0)
						haveDatum = true;
					return;
				}

				if (code == "100")
				{
					++entry.runwayCount;
					double lat1, lon1, lat2, lon2;
					if (tokens.size() >= 26 && ParseDouble(tokens[9], lat1) && ParseDouble(tokens[10], lon1) && ParseDouble(tokens[18], lat2) && ParseDouble(tokens[19], lon2))
						Position(0.5 * (lat1 + lat2), 0.5 * (lon1 + lon2));
					return;
				}
				if (code == "101")
				{
					++entry.waterRunwayCount;
					double lat1, lon1, lat2, lon2;
					if (tokens.size() >= 9 && ParseDouble(tokens[4], lat1) && ParseDouble(tokens[5], lon1) && ParseDouble(tokens[7], lat2) && ParseDouble(tokens[8], lon2))
						Position(0.5 * (lat1 + lat2), 0.5 * (lon1 + lon2));
					return;
				}
				if (code == "102")
				{
					++entry.helipadCount;
					double lat, lon;
					if (tokens.size() >= 4 && ParseDouble(tokens[2], lat) && ParseDouble(tokens[3], lon))
						Position(lat, lon);
					return;
				}
			}
		};

		// -------------------------------------------------------

		void WriteString(std::ofstream &out, const std::string &text)
		{
			const uint32_t size = static_cast<uint32_t>(text.size());
			out.write(reinterpret_cast<const char *>(&size), sizeof(size));
			out.write(text.data(), static_cast<std::streamsize>(text.size()));
		}

		template<typename T>
		void WriteValue(std::ofstream &out, const T &value)
		{
			out.write(reinterpret_cast<const char *>(&value), sizeof(T));
		}

		bool ReadString(std::ifstream &in, std::string &text)
		{
			uint32_t size = 0;
			if (!in.read(reinterpret_cast<char *>(&size), sizeof(size)) || size > (64u << 20))
				return false;
			text.resize(size);
			return size == 0 || static_cast<bool>(in.read(text.data(), size));
		}

		template<typename T>
		bool ReadValue(std::ifstream &in, T &value)
		{
			return static_cast<bool>(in.read(reinterpret_cast<char *>(&value), sizeof(T)));
		}

		/* @brief Rank of a query word against an entry: 0 exact code, 1 code prefix, 2 text match, -1 none. */
		int MatchRank(const std::string &searchText, const std::string &word)
		{
			// searchText: "id icao iata faa local|name city state country" (lower-case)
			const size_t bar = searchText.find('|');
			const std::string_view codes(searchText.data(), bar == std::string::npos ? searchText.size() : bar);

			size_t start = 0;
			int best = -1;
			while (start <= codes.size())
			{
				size_t end = codes.find(' ', start);
				if (end == std::string_view::npos)
					end = codes.size();
				const std::string_view code = codes.substr(start, end - start);
				if (!code.empty())
				{
					if (code == word)
						return 0;
					if (code.starts_with(word))
						best = 1;
				}
				if (end >= codes.size())
					break;
				start = end + 1;
			}
			if (best >= 0)
				return best;
			return searchText.find(word) != std::string::npos ? 2 : -1;
		}
	}

	// -------------------------------------------------------

	std::string AirportEntry::GetLabel() const
	{
		return id + "  " + name;
	}

	std::string AirportEntry::GetCodes() const
	{
		std::string codes;
		const auto add = [&codes, this](const char *label, const std::string &value)
		{
			if (value.empty() || value == id)
				return;
			if (!codes.empty())
				codes += ", ";
			codes += label;
			codes += ' ';
			codes += value;
		};
		add("ICAO", icao);
		add("IATA", iata);
		add("FAA", faa);
		add("Local", local);
		return codes;
	}

	// -------------------------------------------------------

	void AirportIndex::ScanAptText(const std::string_view text, const uint32_t sourceIndex, std::vector<AirportEntry> &outEntries, const uint64_t baseOffset, const uint32_t baseLine)
	{
		Scanner scanner(outEntries, sourceIndex);
		LineReader reader(text);
		std::string_view line;
		while (reader.Next(line))
			scanner.Line(line, baseOffset + static_cast<uint64_t>(line.data() - text.data()), baseLine + reader.GetLineNumber());
		scanner.Finish(baseOffset + text.size());
	}

	bool AirportIndex::ScanAptDat(const std::filesystem::path &aptDat, const uint32_t sourceIndex, std::vector<AirportEntry> &outEntries, std::string *error, const std::atomic<bool> *cancel)
	{
		std::ifstream file(aptDat, std::ios::binary);
		if (!file)
		{
			if (error)
				*error = "Could not open " + aptDat.string();
			return false;
		}

		Scanner scanner(outEntries, sourceIndex);
		std::string buffer;
		buffer.resize(SCAN_CHUNK);
		std::string carry;			// Partial last line of the previous chunk
		uint64_t chunkOffset = 0;	// File offset of carry's first byte (or of the chunk when carry is empty)
		uint32_t lineNumber = 0;

		while (file)
		{
			if (cancel && cancel->load())
			{
				if (error)
					*error = "Cancelled";
				return false;
			}

			file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
			const size_t got = static_cast<size_t>(file.gcount());
			if (got == 0)
				break;

			std::string_view data(buffer.data(), got);
			size_t position = 0;

			// Complete the carried line first.
			if (!carry.empty())
			{
				const size_t newline = data.find('\n');
				if (newline == std::string_view::npos)
				{
					carry.append(data);
					continue;
				}
				carry.append(data.substr(0, newline));
				std::string_view line = carry;
				if (!line.empty() && line.back() == '\r')
					line.remove_suffix(1);
				scanner.Line(line, chunkOffset, ++lineNumber);
				chunkOffset += carry.size() + 1;
				position = newline + 1;
				carry.clear();
			}

			while (position < data.size())
			{
				const size_t newline = data.find('\n', position);
				if (newline == std::string_view::npos)
				{
					carry.assign(data.substr(position));
					break;
				}
				std::string_view line = data.substr(position, newline - position);
				if (!line.empty() && line.back() == '\r')
					line.remove_suffix(1);
				scanner.Line(line, chunkOffset, ++lineNumber);
				chunkOffset += newline - position + 1;
				position = newline + 1;
			}
		}

		if (!carry.empty())
		{
			std::string_view line = carry;
			if (!line.empty() && line.back() == '\r')
				line.remove_suffix(1);
			scanner.Line(line, chunkOffset, ++lineNumber);
			chunkOffset += carry.size();
		}
		scanner.Finish(chunkOffset);
		return true;
	}

	bool AirportIndex::AddSource(AirportSource source, const AirportIndex *cache, const std::atomic<bool> *cancel)
	{
		StatFile(source);
		const uint32_t sourceIndex = static_cast<uint32_t>(m_Sources.size());

		// Reuse the cached scan when the file hasn't changed.
		if (cache && source.fileSize > 0)
		{
			for (size_t i = 0; i < cache->m_Sources.size(); ++i)
			{
				const AirportSource &cached = cache->m_Sources[i];
				if (cached.fileSize != source.fileSize || cached.fileTime != source.fileTime || !SameFile(cached.aptDat, source.aptDat) || !cached.error.empty())
					continue;

				size_t count = 0;
				for (const AirportEntry &entry : cache->m_Airports)
				{
					if (entry.source != i)
						continue;
					AirportEntry copy = entry;
					copy.source = sourceIndex;
					m_Airports.push_back(std::move(copy));
					++count;
				}
				source.airportCount = static_cast<uint32_t>(count);
				source.error.clear();
				m_Sources.push_back(std::move(source));
				return true;
			}
		}

		std::vector<AirportEntry> entries;
		std::string error;
		const bool ok = ScanAptDat(source.aptDat, sourceIndex, entries, &error, cancel);
		if (ok)
		{
			source.airportCount = static_cast<uint32_t>(entries.size());
			m_Airports.insert(m_Airports.end(), std::make_move_iterator(entries.begin()), std::make_move_iterator(entries.end()));
		}
		else
		{
			source.error = error;
			source.airportCount = 0;
		}
		m_Sources.push_back(std::move(source));
		return ok;
	}

	void AirportIndex::Finish()
	{
		// Stable order: id, then source priority, so FindById's first hit is what X-Plane loads.
		std::vector<uint32_t> order(m_Airports.size());
		for (uint32_t i = 0; i < order.size(); ++i)
			order[i] = i;
		std::ranges::stable_sort(order, [this](const uint32_t a, const uint32_t b)
		{
			const AirportEntry &ea = m_Airports[a];
			const AirportEntry &eb = m_Airports[b];
			if (ea.id != eb.id)
				return ea.id < eb.id;
			return m_Sources[ea.source].priority < m_Sources[eb.source].priority;
		});

		std::vector<AirportEntry> sorted;
		sorted.reserve(m_Airports.size());
		for (const uint32_t index : order)
			sorted.push_back(std::move(m_Airports[index]));
		m_Airports.swap(sorted);

		m_SearchText.clear();
		m_SearchText.reserve(m_Airports.size());
		m_ById.clear();
		m_Count3D = 0;
		for (uint32_t i = 0; i < m_Airports.size(); ++i)
		{
			const AirportEntry &entry = m_Airports[i];
			std::string text = entry.id;
			for (const std::string *code : { &entry.icao, &entry.iata, &entry.faa, &entry.local })
			{
				if (!code->empty() && *code != entry.id)
				{
					text += ' ';
					text += *code;
				}
			}
			text += '|';
			text += entry.name;
			for (const std::string *place : { &entry.city, &entry.state, &entry.country })
			{
				if (!place->empty())
				{
					text += ' ';
					text += *place;
				}
			}
			m_SearchText.push_back(ToLower(text));
			m_ById[ToLower(entry.id)].push_back(i);
			if (entry.is3D)
				++m_Count3D;
		}
	}

	// -------------------------------------------------------

	void AirportIndex::Search(const std::string_view query, const SearchOptions &options, std::vector<uint32_t> &outEntries) const
	{
		outEntries.clear();

		std::vector<std::string> words;
		{
			std::vector<std::string_view> tokens;
			Tokenize(query, tokens);
			for (const std::string_view token : tokens)
				words.push_back(ToLower(token));
		}

		struct Hit
		{
			uint32_t entry;
			int rank;
		};
		std::vector<Hit> hits;

		for (uint32_t i = 0; i < m_Airports.size(); ++i)
		{
			const AirportEntry &entry = m_Airports[i];
			const AirportSource &source = m_Sources[entry.source];
			if (options.sourceFilter >= 0 && static_cast<int>(entry.source) != options.sourceFilter)
				continue;
			if (!options.includeDisabled && !source.enabledInSim)
				continue;
			if (options.only3D && !entry.is3D)
				continue;
			if (options.type && entry.type != *options.type)
				continue;

			int rank = 2;
			bool matches = true;
			for (const std::string &word : words)
			{
				const int wordRank = MatchRank(m_SearchText[i], word);
				if (wordRank < 0)
				{
					matches = false;
					break;
				}
				rank = std::min(rank, wordRank);
			}
			if (!matches)
				continue;
			hits.push_back({ i, words.empty() ? 2 : rank });
		}

		std::ranges::stable_sort(hits, [this](const Hit &a, const Hit &b)
		{
			if (a.rank != b.rank)
				return a.rank < b.rank;
			const int pa = m_Sources[m_Airports[a.entry].source].priority;
			const int pb = m_Sources[m_Airports[b.entry].source].priority;
			if (pa != pb)
				return pa < pb;
			return m_Airports[a.entry].id < m_Airports[b.entry].id;
		});

		for (const Hit &hit : hits)
		{
			if (outEntries.size() >= options.maxResults)
				break;
			outEntries.push_back(hit.entry);
		}
	}

	void AirportIndex::FindById(const std::string_view id, std::vector<uint32_t> &outEntries) const
	{
		outEntries.clear();
		const auto it = m_ById.find(ToLower(id));
		if (it != m_ById.end())
			outEntries = it->second;
	}

	const AirportEntry *AirportIndex::FindBest(const std::string_view id) const
	{
		const auto it = m_ById.find(ToLower(id));
		if (it == m_ById.end())
			return nullptr;
		for (const uint32_t index : it->second)
		{
			if (m_Sources[m_Airports[index].source].enabledInSim)
				return &m_Airports[index];
		}
		return it->second.empty() ? nullptr : &m_Airports[it->second.front()];
	}

	void AirportIndex::FindNear(const double latitude, const double longitude, const double radiusMeters, std::vector<uint32_t> &outEntries, const size_t maxResults) const
	{
		outEntries.clear();
		struct Hit
		{
			uint32_t entry;
			double distance;
		};
		std::vector<Hit> hits;

		// Quick reject on a degree box before the exact distance.
		const double latSpan = radiusMeters / 111320.0;
		const double lonSpan = latSpan / std::max(0.05, std::cos(latitude * 3.14159265358979323846 / 180.0));
		for (uint32_t i = 0; i < m_Airports.size(); ++i)
		{
			const AirportEntry &entry = m_Airports[i];
			if (!entry.hasPosition || std::abs(entry.latitude - latitude) > latSpan || std::abs(entry.longitude - longitude) > lonSpan)
				continue;
			const double distance = GreatCircleDistance(latitude, longitude, entry.latitude, entry.longitude);
			if (distance <= radiusMeters)
				hits.push_back({ i, distance });
		}
		std::ranges::sort(hits, [](const Hit &a, const Hit &b) { return a.distance < b.distance; });
		for (const Hit &hit : hits)
		{
			if (outEntries.size() >= maxResults)
				break;
			outEntries.push_back(hit.entry);
		}
	}

	bool AirportIndex::LoadAirport(const AirportEntry &entry, AptAirport &out, std::string *error) const
	{
		if (entry.source >= m_Sources.size())
		{
			if (error)
				*error = "Unknown source";
			return false;
		}
		const AirportSource &source = m_Sources[entry.source];

		std::ifstream file(source.aptDat, std::ios::binary);
		if (!file)
		{
			if (error)
				*error = "Could not open " + source.aptDat.string();
			return false;
		}

		std::string text;
		text.resize(static_cast<size_t>(entry.length));
		file.seekg(static_cast<std::streamoff>(entry.offset));
		if (entry.length > 0 && !file.read(text.data(), static_cast<std::streamsize>(entry.length)))
		{
			if (error)
				*error = "Could not read " + source.aptDat.string() + " (the file changed since it was indexed?)";
			return false;
		}

		AptFile parsed;
		AptParseOptions options;
		options.requireHeader = false;
		if (!ParseAptDat(text, parsed, error, options))
			return false;
		if (parsed.airports.size() != 1 || parsed.airports[0].id != entry.id)
		{
			if (error)
				*error = "The airport isn't where the index says it is in " + source.aptDat.string() + " (rescan the airports)";
			return false;
		}
		out = std::move(parsed.airports[0]);
		return true;
	}

	// -------------------------------------------------------

	bool AirportIndex::SaveCache(const std::filesystem::path &path) const
	{
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!out)
			return false;

		out.write(CACHE_MAGIC, sizeof(CACHE_MAGIC));
		WriteValue(out, CACHE_VERSION);
		WriteValue(out, static_cast<uint32_t>(m_Sources.size()));
		for (const AirportSource &source : m_Sources)
		{
			WriteString(out, PathUtf8(source.aptDat));
			WriteString(out, PathUtf8(source.package));
			WriteString(out, source.packageName);
			WriteValue(out, static_cast<uint8_t>(source.source));
			WriteValue(out, static_cast<uint8_t>(source.enabledInSim ? 1 : 0));
			WriteValue(out, static_cast<int32_t>(source.priority));
			WriteValue(out, source.fileSize);
			WriteValue(out, source.fileTime);
			WriteValue(out, source.airportCount);
			WriteString(out, source.error);
		}

		WriteValue(out, static_cast<uint32_t>(m_Airports.size()));
		for (const AirportEntry &entry : m_Airports)
		{
			WriteString(out, entry.id);
			WriteString(out, entry.name);
			WriteValue(out, static_cast<uint8_t>(entry.type));
			WriteValue(out, entry.latitude);
			WriteValue(out, entry.longitude);
			WriteValue(out, static_cast<uint8_t>(entry.hasPosition ? 1 : 0));
			WriteString(out, entry.icao);
			WriteString(out, entry.iata);
			WriteString(out, entry.faa);
			WriteString(out, entry.local);
			WriteString(out, entry.city);
			WriteString(out, entry.state);
			WriteString(out, entry.country);
			WriteValue(out, static_cast<uint8_t>(entry.is3D ? 1 : 0));
			WriteValue(out, entry.source);
			WriteValue(out, entry.offset);
			WriteValue(out, entry.length);
			WriteValue(out, entry.line);
			WriteValue(out, entry.runwayCount);
			WriteValue(out, entry.waterRunwayCount);
			WriteValue(out, entry.helipadCount);
		}
		return static_cast<bool>(out);
	}

	bool AirportIndex::LoadCache(const std::filesystem::path &path)
	{
		m_Sources.clear();
		m_Airports.clear();
		m_SearchText.clear();
		m_ById.clear();

		std::ifstream in(path, std::ios::binary);
		if (!in)
			return false;

		char magic[8];
		uint32_t version = 0;
		if (!in.read(magic, sizeof(magic)) || std::memcmp(magic, CACHE_MAGIC, sizeof(magic)) != 0 || !ReadValue(in, version) || version != CACHE_VERSION)
			return false;

		uint32_t sourceCount = 0;
		if (!ReadValue(in, sourceCount) || sourceCount > 100000)
			return false;
		for (uint32_t i = 0; i < sourceCount; ++i)
		{
			AirportSource source;
			std::string aptDat, package;
			uint8_t kind = 0, enabled = 0;
			int32_t priority = 0;
			if (!ReadString(in, aptDat) || !ReadString(in, package) || !ReadString(in, source.packageName) || !ReadValue(in, kind) || !ReadValue(in, enabled) ||
				!ReadValue(in, priority) || !ReadValue(in, source.fileSize) || !ReadValue(in, source.fileTime) || !ReadValue(in, source.airportCount) || !ReadString(in, source.error))
			{
				m_Sources.clear();
				return false;
			}
			source.aptDat = PathFromUtf8(aptDat);
			source.package = PathFromUtf8(package);
			source.source = static_cast<PackageSource>(kind);
			source.enabledInSim = enabled != 0;
			source.priority = priority;
			m_Sources.push_back(std::move(source));
		}

		uint32_t airportCount = 0;
		if (!ReadValue(in, airportCount) || airportCount > 10000000)
		{
			m_Sources.clear();
			return false;
		}
		m_Airports.reserve(airportCount);
		for (uint32_t i = 0; i < airportCount; ++i)
		{
			AirportEntry entry;
			uint8_t type = 1, hasPosition = 0, is3D = 0;
			if (!ReadString(in, entry.id) || !ReadString(in, entry.name) || !ReadValue(in, type) || !ReadValue(in, entry.latitude) || !ReadValue(in, entry.longitude) ||
				!ReadValue(in, hasPosition) || !ReadString(in, entry.icao) || !ReadString(in, entry.iata) || !ReadString(in, entry.faa) || !ReadString(in, entry.local) ||
				!ReadString(in, entry.city) || !ReadString(in, entry.state) || !ReadString(in, entry.country) || !ReadValue(in, is3D) || !ReadValue(in, entry.source) ||
				!ReadValue(in, entry.offset) || !ReadValue(in, entry.length) || !ReadValue(in, entry.line) || !ReadValue(in, entry.runwayCount) ||
				!ReadValue(in, entry.waterRunwayCount) || !ReadValue(in, entry.helipadCount))
			{
				m_Sources.clear();
				m_Airports.clear();
				return false;
			}
			entry.type = static_cast<AirportType>(type);
			entry.hasPosition = hasPosition != 0;
			entry.is3D = is3D != 0;
			if (entry.source >= m_Sources.size())
				continue;
			m_Airports.push_back(std::move(entry));
		}
		return true;
	}
}

// -------------------------------------------------------
