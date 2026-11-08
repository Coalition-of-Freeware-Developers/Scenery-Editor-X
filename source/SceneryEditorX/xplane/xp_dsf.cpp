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
 * xp_dsf.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_dsf.h"
#include "xp_7z.h"
#include <SceneryEditorX/filesystem/vfs.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <unordered_map>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr uint32_t FourCC(const char a, const char b, const char c, const char d)
		{
			return (static_cast<uint32_t>(static_cast<uint8_t>(a)) << 24) | (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 16) |
				   (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 8) | static_cast<uint32_t>(static_cast<uint8_t>(d));
		}

		constexpr uint32_t ATOM_HEAD = FourCC('H', 'E', 'A', 'D');
		constexpr uint32_t ATOM_PROP = FourCC('P', 'R', 'O', 'P');
		constexpr uint32_t ATOM_DEFN = FourCC('D', 'E', 'F', 'N');
		constexpr uint32_t ATOM_TERT = FourCC('T', 'E', 'R', 'T');
		constexpr uint32_t ATOM_OBJT = FourCC('O', 'B', 'J', 'T');
		constexpr uint32_t ATOM_POLY = FourCC('P', 'O', 'L', 'Y');
		constexpr uint32_t ATOM_NETW = FourCC('N', 'E', 'T', 'W');
		constexpr uint32_t ATOM_DEMN = FourCC('D', 'E', 'M', 'N');
		constexpr uint32_t ATOM_GEOD = FourCC('G', 'E', 'O', 'D');
		constexpr uint32_t ATOM_POOL = FourCC('P', 'O', 'O', 'L');
		constexpr uint32_t ATOM_SCAL = FourCC('S', 'C', 'A', 'L');
		constexpr uint32_t ATOM_PO32 = FourCC('P', 'O', '3', '2');
		constexpr uint32_t ATOM_SC32 = FourCC('S', 'C', '3', '2');
		constexpr uint32_t ATOM_CMDS = FourCC('C', 'M', 'D', 'S');
		constexpr uint32_t ATOM_DEMS = FourCC('D', 'E', 'M', 'S');
		constexpr uint32_t ATOM_DEMI = FourCC('D', 'E', 'M', 'I');
		constexpr uint32_t ATOM_DEMD = FourCC('D', 'E', 'M', 'D');

		// Command ids
		enum : uint8_t
		{
			CMD_RESERVED = 0,
			CMD_POOL_SELECT = 1,
			CMD_JUNCTION_OFFSET_SELECT = 2,
			CMD_SET_DEFINITION8 = 3,
			CMD_SET_DEFINITION16 = 4,
			CMD_SET_DEFINITION32 = 5,
			CMD_SET_ROAD_SUBTYPE8 = 6,
			CMD_OBJECT = 7,
			CMD_OBJECT_RANGE = 8,
			CMD_NETWORK_CHAIN = 9,
			CMD_NETWORK_CHAIN_RANGE = 10,
			CMD_NETWORK_CHAIN32 = 11,
			CMD_POLYGON = 12,
			CMD_POLYGON_RANGE = 13,
			CMD_NESTED_POLYGON = 14,
			CMD_NESTED_POLYGON_RANGE = 15,
			CMD_TERRAIN_PATCH = 16,
			CMD_TERRAIN_PATCH_FLAGS = 17,
			CMD_TERRAIN_PATCH_FLAGS_LOD = 18,
			CMD_TRIANGLE = 23,
			CMD_TRIANGLE_CROSS_POOL = 24,
			CMD_TRIANGLE_RANGE = 25,
			CMD_TRIANGLE_STRIP = 26,
			CMD_TRIANGLE_STRIP_CROSS_POOL = 27,
			CMD_TRIANGLE_STRIP_RANGE = 28,
			CMD_TRIANGLE_FAN = 29,
			CMD_TRIANGLE_FAN_CROSS_POOL = 30,
			CMD_TRIANGLE_FAN_RANGE = 31,
			CMD_COMMENT8 = 32,
			CMD_COMMENT16 = 33,
			CMD_COMMENT32 = 34
		};

		constexpr uint16_t COMMENT_FILTER = 1;
		constexpr uint16_t COMMENT_AGL = 2;

		// Pool plane encodings
		constexpr uint8_t ENCODE_RAW = 0;
		constexpr uint8_t ENCODE_DIFFERENCED = 1;
		constexpr uint8_t ENCODE_RLE = 2;
		constexpr uint8_t ENCODE_RLE_DIFFERENCED = 3;

		/* @brief Bounds-checked little-endian reader. */
		class Reader
		{
		public:
			Reader(const uint8_t *data, const size_t size) : m_Data(data), m_Size(size) {}

			[[nodiscard]] bool Done() const { return m_Position >= m_Size; }
			[[nodiscard]] bool Overrun() const { return m_Overrun; }
			[[nodiscard]] size_t GetPosition() const { return m_Position; }
			[[nodiscard]] size_t GetRemaining() const { return m_Position < m_Size ? m_Size - m_Position : 0; }
			[[nodiscard]] const uint8_t *GetData() const { return m_Data; }
			[[nodiscard]] size_t GetSize() const { return m_Size; }

			uint8_t U8()
			{
				if (m_Position + 1 > m_Size) { m_Overrun = true; return 0; }
				return m_Data[m_Position++];
			}

			uint16_t U16()
			{
				if (m_Position + 2 > m_Size) { m_Overrun = true; m_Position = m_Size; return 0; }
				const uint16_t value = static_cast<uint16_t>(m_Data[m_Position] | (m_Data[m_Position + 1] << 8));
				m_Position += 2;
				return value;
			}

			uint32_t U32()
			{
				if (m_Position + 4 > m_Size) { m_Overrun = true; m_Position = m_Size; return 0; }
				const uint32_t value = static_cast<uint32_t>(m_Data[m_Position]) | (static_cast<uint32_t>(m_Data[m_Position + 1]) << 8) |
									   (static_cast<uint32_t>(m_Data[m_Position + 2]) << 16) | (static_cast<uint32_t>(m_Data[m_Position + 3]) << 24);
				m_Position += 4;
				return value;
			}

			int32_t S32() { return static_cast<int32_t>(U32()); }

			float F32()
			{
				const uint32_t bits = U32();
				float value;
				std::memcpy(&value, &bits, sizeof(value));
				return value;
			}

			void Skip(const size_t bytes)
			{
				if (m_Position + bytes > m_Size) { m_Overrun = true; m_Position = m_Size; return; }
				m_Position += bytes;
			}

			Reader Sub(const size_t bytes)
			{
				if (m_Position + bytes > m_Size) { m_Overrun = true; const Reader empty(m_Data, 0); m_Position = m_Size; return empty; }
				Reader sub(m_Data + m_Position, bytes);
				m_Position += bytes;
				return sub;
			}

		private:
			const uint8_t *m_Data;
			size_t m_Size;
			size_t m_Position = 0;
			bool m_Overrun = false;
		};

		struct Atom
		{
			uint32_t id = 0;
			Reader content{ nullptr, 0 };
		};

		/* @brief Reads the next "id, size (including this header), payload" atom. */
		bool NextAtom(Reader &reader, Atom &atom)
		{
			if (reader.GetRemaining() < 8)
				return false;
			atom.id = reader.U32();
			const uint32_t size = reader.U32();
			if (size < 8 || size - 8 > reader.GetRemaining())
				return false;
			atom.content = reader.Sub(size - 8);
			return true;
		}

		void ReadStringTable(Reader reader, std::vector<std::string> &out)
		{
			std::string current;
			while (!reader.Done())
			{
				const char c = static_cast<char>(reader.U8());
				if (c == '\0')
				{
					out.push_back(current);
					current.clear();
				}
				else
					current.push_back(c);
			}
			if (!current.empty())
				out.push_back(current);
		}

		struct Pool
		{
			uint32_t count = 0;
			uint8_t planes = 0;
			std::vector<double> values;		// count * planes, interleaved per point, already scaled
		};

		/* @brief Decodes one plane of a pool into out[plane + i * planes]. */
		template<typename T>
		bool DecodePlane(Reader &reader, const uint32_t count, const uint8_t planes, const uint8_t plane, std::vector<double> &out, const double scale, const double offset, const double reduce)
		{
			const uint8_t encoding = reader.U8();
			if (encoding > ENCODE_RLE_DIFFERENCED)
				return false;
			const bool differenced = encoding == ENCODE_DIFFERENCED || encoding == ENCODE_RLE_DIFFERENCED;
			const bool rle = encoding == ENCODE_RLE || encoding == ENCODE_RLE_DIFFERENCED;

			const auto read = [&reader]() -> T
			{
				if constexpr (sizeof(T) == 2)
					return static_cast<T>(reader.U16());
				else
					return static_cast<T>(reader.U32());
			};

			T last = 0;
			uint32_t produced = 0;
			const auto emit = [&](T raw)
			{
				if (differenced)
				{
					raw = static_cast<T>(last + raw);
					last = raw;
				}
				double value = static_cast<double>(raw);
				if (scale != 0.0)
					value = value * scale * reduce + offset;
				out[static_cast<size_t>(plane) + static_cast<size_t>(produced) * planes] = value;
				++produced;
			};

			if (!rle)
			{
				for (uint32_t i = 0; i < count; ++i)
					emit(read());
			}
			else
			{
				while (produced < count && !reader.Overrun())
				{
					const uint8_t header = reader.U8();
					if (header & 0x80)
					{
						// A run: one value repeated
						uint32_t run = header & 0x7F;
						const T value = read();
						while (run-- > 0 && produced < count)
							emit(value);
					}
					else
					{
						uint32_t literal = header;
						while (literal-- > 0 && produced < count)
							emit(read());
					}
				}
			}
			return !reader.Overrun() && produced == count;
		}

		template<typename T>
		bool ReadPool(Reader reader, const std::vector<std::pair<float, float>> &scaling, Pool &pool, const double reduce)
		{
			pool.count = reader.U32();
			pool.planes = reader.U8();
			if (reader.Overrun() || pool.planes > 8 || pool.count > 50'000'000)
				return false;
			if (pool.count == 0 || pool.planes == 0)
			{
				// Empty pools are written for unused pool slots (networks in overlays, for instance)
				pool.values.clear();
				return true;
			}
			pool.values.assign(static_cast<size_t>(pool.count) * pool.planes, 0.0);
			for (uint8_t plane = 0; plane < pool.planes; ++plane)
			{
				const double scale = plane < scaling.size() ? scaling[plane].first : 0.0;
				const double offset = plane < scaling.size() ? scaling[plane].second : 0.0;
				if (!DecodePlane<T>(reader, pool.count, pool.planes, plane, pool.values, scale, offset, reduce))
					return false;
			}
			return true;
		}

		DsfPoint PointOf(const Pool &pool, const uint32_t index)
		{
			DsfPoint point;
			if (index >= pool.count)
				return point;
			point.planes = pool.planes;
			for (uint8_t plane = 0; plane < pool.planes; ++plane)
				point.v[plane] = pool.values[static_cast<size_t>(index) * pool.planes + plane];
			return point;
		}

		bool Fail(std::string *error, const std::string &message)
		{
			if (error)
				*error = message;
			return false;
		}
	}

	// -------------------------------------------------------

	std::string_view DsfFile::GetProperty(const std::string_view key) const
	{
		for (const auto &[name, value] : properties)
		{
			if (name == key)
				return value;
		}
		return {};
	}

	bool DsfFile::IsOverlay() const
	{
		return GetProperty("sim/overlay") == "1";
	}

	std::vector<std::pair<std::string, std::string>> DsfFile::GetExclusions() const
	{
		std::vector<std::pair<std::string, std::string>> exclusions;
		for (const auto &[name, value] : properties)
		{
			if (name.starts_with("sim/exclude_"))
				exclusions.emplace_back(name, value);
		}
		return exclusions;
	}

	std::vector<DsfExclusion> DsfFile::GetExclusionZones() const
	{
		std::vector<DsfExclusion> zones;
		for (const auto &[name, value] : properties)
		{
			DsfExclusion zone;
			if (ParseDsfExclusion(name, value, zone))
				zones.push_back(std::move(zone));
		}
		return zones;
	}

	bool ParseDsfExclusion(const std::string_view key, const std::string_view value, DsfExclusion &out)
	{
		constexpr std::string_view PREFIX = "sim/exclude_";
		if (!key.starts_with(PREFIX) || key.size() == PREFIX.size())
			return false;
		out = DsfExclusion();
		out.kind = std::string(key.substr(PREFIX.size()));

		// Numbers in order: the box, then (after ';') the polygon's pairs, whatever separates them ("/" or ",")
		const auto numbers = [](const std::string_view text, std::vector<double> &values)
		{
			size_t at = 0;
			while (at < text.size())
			{
				while (at < text.size() && (text[at] == '/' || text[at] == ',' || text[at] == ' ' || text[at] == '\t'))
					++at;
				if (at >= text.size())
					break;
				const std::string token(text.substr(at, std::min(text.find_first_of("/, \t", at), text.size()) - at));
				char *end = nullptr;
				const double number = std::strtod(token.c_str(), &end);
				if (end == token.c_str() || *end != '\0')
					return false;
				values.push_back(number);
				at += token.size();
			}
			return true;
		};

		const size_t split = value.find(';');
		std::vector<double> box, polygon;
		if (!numbers(value.substr(0, split), box) || box.size() != 4)
			return false;
		out.west = std::min(box[0], box[2]);
		out.east = std::max(box[0], box[2]);
		out.south = std::min(box[1], box[3]);
		out.north = std::max(box[1], box[3]);
		if (split != std::string_view::npos)
		{
			if (!numbers(value.substr(split + 1), polygon) || polygon.size() % 2 != 0)
				return false;
			if (polygon.size() >= 6)
			{
				for (size_t i = 0; i + 1 < polygon.size(); i += 2)
					out.polygon.emplace_back(polygon[i], polygon[i + 1]);
			}
		}
		return true;
	}

	bool DsfExclusion::Contains(const double longitude, const double latitude) const
	{
		if (longitude < west || longitude > east || latitude < south || latitude > north)
			return false;
		if (polygon.size() < 3)
			return true;
		bool inside = false;
		for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
		{
			const auto [xi, yi] = polygon[i];
			const auto [xj, yj] = polygon[j];
			if ((yi > latitude) != (yj > latitude) && longitude < (xj - xi) * (latitude - yi) / (yj - yi) + xi)
				inside = !inside;
		}
		return inside;
	}

	// -------------------------------------------------------

	const DsfRaster *DsfFile::FindRaster(const std::string_view name) const
	{
		for (const DsfRaster &raster : rasters)
		{
			if (raster.name == name)
				return &raster;
		}
		return nullptr;
	}

	float DsfRaster::At(const uint32_t x, const uint32_t y) const
	{
		if (width == 0 || height == 0 || values.size() < static_cast<size_t>(width) * height)
			return 0.0f;
		return values[static_cast<size_t>(std::min(y, height - 1)) * width + std::min(x, width - 1)];
	}

	namespace
	{
		/* @brief A point of the tile bounds in raster cells, clamped to the edges; false for an empty raster or bounds. */
		bool ToRasterCell(const DsfRaster &raster, const double longitude, const double latitude, const double west, const double south, const double east, const double north,
						  double &x, double &y)
		{
			if (raster.width == 0 || raster.height == 0 || east <= west || north <= south)
				return false;
			const double u = (longitude - west) / (east - west);
			const double v = (latitude - south) / (north - south);
			if (raster.IsPostCentric())
			{
				x = u * static_cast<double>(raster.width - 1);
				y = v * static_cast<double>(raster.height - 1);
			}
			else
			{
				x = u * static_cast<double>(raster.width) - 0.5;
				y = v * static_cast<double>(raster.height) - 0.5;
			}
			x = std::clamp(x, 0.0, static_cast<double>(raster.width - 1));
			y = std::clamp(y, 0.0, static_cast<double>(raster.height - 1));
			return true;
		}
	}

	float DsfRaster::Sample(const double longitude, const double latitude, const double west, const double south, const double east, const double north) const
	{
		double x, y;
		if (!ToRasterCell(*this, longitude, latitude, west, south, east, north, x, y))
			return 0.0f;
		const uint32_t x0 = static_cast<uint32_t>(x), y0 = static_cast<uint32_t>(y);
		const uint32_t x1 = std::min(x0 + 1, width - 1), y1 = std::min(y0 + 1, height - 1);
		const float fx = static_cast<float>(x - x0), fy = static_cast<float>(y - y0);
		const float bottom = At(x0, y0) + (At(x1, y0) - At(x0, y0)) * fx;
		const float top = At(x0, y1) + (At(x1, y1) - At(x0, y1)) * fx;
		return bottom + (top - bottom) * fy;
	}

	float DsfRaster::SampleNearest(const double longitude, const double latitude, const double west, const double south, const double east, const double north) const
	{
		double x, y;
		if (!ToRasterCell(*this, longitude, latitude, west, south, east, north, x, y))
			return 0.0f;
		return At(static_cast<uint32_t>(std::lround(x)), static_cast<uint32_t>(std::lround(y)));
	}

	// -------------------------------------------------------

	bool ReadDsf(const uint8_t *data, const size_t size, DsfFile &out, std::string *error)
	{
		return ReadDsf(data, size, out, DsfReadOptions{}, error);
	}

	bool ReadDsf(const uint8_t *data, const size_t size, DsfFile &out, const DsfReadOptions &options, std::string *error)
	{
		out = DsfFile();
		if (Is7zArchive(data, size))
		{
			std::vector<uint8_t> unpacked;
			std::string unpackError;
			if (!Extract7zFirstFile(data, size, unpacked, &unpackError))
				return Fail(error, "The compressed DSF can't be unpacked: " + unpackError);
			return ReadDsf(unpacked.data(), unpacked.size(), out, options, error);
		}
		if (!data || size < 8 + 4 + 16)
			return Fail(error, "Not a DSF file (too small)");
		if (std::memcmp(data, "XPLNEDSF", 8) != 0)
			return Fail(error, "Not a DSF file (bad signature)");

		// Header, atoms, then a 16-byte MD5 footer
		Reader reader(data + 8, size - 8 - 16);
		const uint32_t version = reader.U32();
		if (version != 1)
			return Fail(error, "Unsupported DSF version " + std::to_string(version));

		std::vector<std::vector<std::pair<float, float>>> scales16, scales32;
		std::vector<Reader> pools16, pools32;
		std::vector<Reader> demsAtoms;
		Reader commands(nullptr, 0);
		bool haveCommands = false;

		Atom atom;
		while (NextAtom(reader, atom))
		{
			switch (atom.id)
			{
				case ATOM_HEAD:
				{
					Atom inner;
					while (NextAtom(atom.content, inner))
					{
						if (inner.id == ATOM_PROP)
						{
							std::vector<std::string> strings;
							ReadStringTable(inner.content, strings);
							for (size_t i = 0; i + 1 < strings.size(); i += 2)
								out.properties.emplace_back(strings[i], strings[i + 1]);
						}
					}
					break;
				}
				case ATOM_DEFN:
				{
					Atom inner;
					while (NextAtom(atom.content, inner))
					{
						switch (inner.id)
						{
							case ATOM_TERT: ReadStringTable(inner.content, out.terrainDefinitions); break;
							case ATOM_OBJT: ReadStringTable(inner.content, out.objectDefinitions); break;
							case ATOM_POLY: ReadStringTable(inner.content, out.polygonDefinitions); break;
							case ATOM_NETW: ReadStringTable(inner.content, out.networkDefinitions); break;
							case ATOM_DEMN: ReadStringTable(inner.content, out.rasterDefinitions); break;
							default: break;
						}
					}
					break;
				}
				case ATOM_GEOD:
				{
					Atom inner;
					while (NextAtom(atom.content, inner))
					{
						switch (inner.id)
						{
							case ATOM_POOL: pools16.push_back(inner.content); break;
							case ATOM_PO32: pools32.push_back(inner.content); break;
							case ATOM_SCAL:
							case ATOM_SC32:
							{
								std::vector<std::pair<float, float>> planes;
								Reader scal = inner.content;
								while (scal.GetRemaining() >= 8)
								{
									const float scale = scal.F32();
									const float offset = scal.F32();
									planes.emplace_back(scale, offset);
								}
								(inner.id == ATOM_SCAL ? scales16 : scales32).push_back(std::move(planes));
								break;
							}
							default: break;
						}
					}
					break;
				}
				case ATOM_CMDS:
					commands = atom.content;
					haveCommands = true;
					break;
				case ATOM_DEMS:
					if (options.rasters)
						demsAtoms.push_back(atom.content);
					break;
				default:
					break;	// Anything newer
			}
		}

		// Raster layers: a DEMI (header) and DEMD (data) atom per layer, in the order DEMN names them
		uint32_t rasterIndex = 0;
		for (Reader &dems : demsAtoms)
		{
			Atom inner;
			DsfRaster raster;
			bool haveInfo = false;
			while (NextAtom(dems, inner))
			{
				if (inner.id == ATOM_DEMI)
				{
					raster = DsfRaster();
					raster.version = inner.content.U8();
					raster.bytesPerPixel = inner.content.U8();
					raster.flags = inner.content.U16();
					raster.width = inner.content.U32();
					raster.height = inner.content.U32();
					raster.scale = inner.content.F32();
					raster.offset = inner.content.F32();
					haveInfo = !inner.content.Overrun();
				}
				else if (inner.id == ATOM_DEMD && haveInfo)
				{
					haveInfo = false;
					raster.name = rasterIndex < out.rasterDefinitions.size() ? out.rasterDefinitions[rasterIndex] : "raster" + std::to_string(rasterIndex);
					++rasterIndex;
					if (!options.rasterNames.empty() && std::ranges::find(options.rasterNames, raster.name) == options.rasterNames.end())
						continue;

					const uint32_t type = raster.flags & 3u;
					const uint32_t bytes = raster.bytesPerPixel;
					const bool sizeOk = (type == 0 && bytes == 4) || ((type == 1 || type == 2) && (bytes == 1 || bytes == 2 || bytes == 4));
					const uint64_t count = static_cast<uint64_t>(raster.width) * raster.height;
					if (!sizeOk || count == 0 || count > (1ull << 26) || inner.content.GetRemaining() < count * bytes)
						return Fail(error, "Corrupt DSF raster '" + raster.name + "'");

					raster.values.resize(static_cast<size_t>(count));
					Reader &cells = inner.content;
					for (float &value : raster.values)
					{
						double stored;
						if (type == 0)
							stored = cells.F32();
						else if (bytes == 1)
							stored = type == 1 ? static_cast<double>(static_cast<int8_t>(cells.U8())) : static_cast<double>(cells.U8());
						else if (bytes == 2)
							stored = type == 1 ? static_cast<double>(static_cast<int16_t>(cells.U16())) : static_cast<double>(cells.U16());
						else
							stored = type == 1 ? static_cast<double>(cells.S32()) : static_cast<double>(cells.U32());
						value = static_cast<float>(stored * raster.scale + raster.offset);
					}
					out.rasters.push_back(std::move(raster));
				}
			}
		}

		for (const auto &[key, value] : out.properties)
		{
			if (key == "sim/west") out.west = std::atof(value.c_str());
			else if (key == "sim/south") out.south = std::atof(value.c_str());
			else if (key == "sim/east") out.east = std::atof(value.c_str());
			else if (key == "sim/north") out.north = std::atof(value.c_str());
		}

		if (!haveCommands)
			return Fail(error, "The DSF has no command atom");
		if (scales16.size() < pools16.size() || scales32.size() < pools32.size())
			return Fail(error, "The DSF has pools without scaling atoms");

		// The pools (the nth SCAL belongs to the nth POOL), decoded when a command first uses one: a terrain read limited
		// to an area never decodes the pools of the rest of the tile
		std::vector<Pool> pool16(pools16.size()), pool32(pools32.size());
		std::vector<uint8_t> state16(pools16.size(), 0), state32(pools32.size(), 0);	// 0 not decoded, 1 decoded, 2 corrupt
		std::string poolError;

		// Command stream
		uint32_t currentPool = 0;
		uint32_t currentDefinition = 0;
		uint32_t junctionOffset = 0;
		uint8_t roadSubtype = 0;
		DsfObjectPlacement::Mode objectMode = DsfObjectPlacement::Mode::Msl;
		int32_t currentFilter = -1;

		const auto pool = [&](const uint32_t index) -> const Pool *
		{
			if (index >= pool16.size())
				return nullptr;
			if (state16[index] == 0)
			{
				state16[index] = ReadPool<uint16_t>(pools16[index], scales16[index], pool16[index], 1.0 / 65535.0) ? 1 : 2;
				if (state16[index] == 2 && poolError.empty())
					poolError = "Corrupt 16-bit point pool " + std::to_string(index);
			}
			return state16[index] == 1 ? &pool16[index] : nullptr;
		};
		const auto pool32At = [&](const uint32_t index) -> const Pool *
		{
			if (index >= pool32.size())
				return nullptr;
			if (state32[index] == 0)
			{
				state32[index] = ReadPool<uint32_t>(pools32[index], scales32[index], pool32[index], 1.0 / 4294967295.0) ? 1 : 2;
				if (state32[index] == 2 && poolError.empty())
					poolError = "Corrupt 32-bit point pool " + std::to_string(index);
			}
			return state32[index] == 1 ? &pool32[index] : nullptr;
		};
		// Terrain: the open patch, and where each pool point it uses landed in it
		uint8_t patchFlags = DsfTerrainPatch::Physical;
		float patchLodStart = 0.0f, patchLodEnd = -1.0f;
		bool patchOpen = false;
		std::unordered_map<uint64_t, uint32_t> patchPoints;

		/* @brief True when a pool's box (from its scaling) lies wholly outside the area. */
		const auto scalingOutsideArea = [&](const std::vector<std::vector<std::pair<float, float>>> &scales, const uint32_t index) -> bool
		{
			if (!options.HasArea() || index >= scales.size())
				return false;
			const auto &scaling = scales[index];
			if (scaling.size() < 2 || scaling[0].first == 0.0f || scaling[1].first == 0.0f)
				return false;
			const double west = scaling[0].second, east = west + scaling[0].first;
			const double south = scaling[1].second, north = south + scaling[1].first;
			return east < options.areaWest || west > options.areaEast || north < options.areaSouth || south > options.areaNorth;
		};
		const auto poolOutsideArea = [&](const uint32_t index) { return scalingOutsideArea(scales16, index); };

		// Overlays: none from an overlay tile when only the base mesh's are wanted; none from pools outside the area
		const bool readOverlays = options.overlays && !(options.overlaysOnlyInBaseMesh && out.IsOverlay());
		const auto overlayPool = [&](const uint32_t index) -> const Pool * { return readOverlays && !poolOutsideArea(index) ? pool(index) : nullptr; };
		const auto overlayPool32 = [&](const uint32_t index) -> const Pool * { return readOverlays && !scalingOutsideArea(scales32, index) ? pool32At(index) : nullptr; };

		/* @brief True when a box (degrees) reaches into the area (or there is none). */
		const auto boxInArea = [&](const double west, const double south, const double east, const double north)
		{
			return !options.HasArea() || !(east < options.areaWest || west > options.areaEast || north < options.areaSouth || south > options.areaNorth);
		};

		const auto beginPatch = [&]()
		{
			++out.terrainPatchCount;
			patchOpen = true;
			if (!options.terrain)
				return;
			if (!out.terrainPatches.empty() && out.terrainPatches.back().indices.empty())
				out.terrainPatches.pop_back();	// Nothing of the last one in the area
			patchPoints.clear();
			DsfTerrainPatch &patch = out.terrainPatches.emplace_back();
			patch.definition = currentDefinition;
			patch.flags = patchFlags;
			patch.lodStart = patchLodStart;
			patch.lodEnd = patchLodEnd;
		};

		/* @brief Adds triangles to the open patch: refs are (pool, index) pairs as a list, a strip (1) or a fan (2). */
		std::vector<std::pair<uint32_t, uint32_t>> refs;
		const auto addTriangles = [&](const int mode)
		{
			const size_t count = refs.size();
			const size_t triangles = mode == 0 ? count / 3 : (count >= 3 ? count - 2 : 0);
			out.terrainTriangleCount += static_cast<uint32_t>(triangles);
			if (!options.terrain || !patchOpen || out.terrainPatches.empty() || triangles == 0)
				return;
			DsfTerrainPatch &patch = out.terrainPatches.back();

			// Points first: a triangle needs all three, and its box must reach into the area
			for (size_t t = 0; t < triangles; ++t)
			{
				size_t corner[3];
				if (mode == 0)
					corner[0] = t * 3, corner[1] = t * 3 + 1, corner[2] = t * 3 + 2;
				else if (mode == 1)
				{
					corner[0] = (t & 1) ? t + 1 : t;
					corner[1] = (t & 1) ? t : t + 1;
					corner[2] = t + 2;
				}
				else
					corner[0] = 0, corner[1] = t + 1, corner[2] = t + 2;

				const Pool *pools[3] = {};
				bool usable = true;
				for (int c = 0; c < 3 && usable; ++c)
				{
					const auto [poolIndex, pointIndex] = refs[corner[c]];
					if (poolOutsideArea(poolIndex))
					{
						// A point in a pool outside the area: the triangle can still reach into it only when another of
						// its points doesn't (cross-pool triangles along pool edges); keep it simple and test its box
						bool anyInside = false;
						for (int o = 0; o < 3; ++o)
							anyInside |= !poolOutsideArea(refs[corner[o]].first);
						if (!anyInside)
						{
							usable = false;
							break;
						}
					}
					pools[c] = pool(poolIndex);
					usable = pools[c] && pointIndex < pools[c]->count;
				}
				if (!usable)
					continue;

				if (options.HasArea())
				{
					double west = 1e9, east = -1e9, south = 1e9, north = -1e9;
					for (int c = 0; c < 3; ++c)
					{
						const double *point = pools[c]->values.data() + static_cast<size_t>(refs[corner[c]].second) * pools[c]->planes;
						west = std::min(west, point[0]);
						east = std::max(east, point[0]);
						south = std::min(south, point[1]);
						north = std::max(north, point[1]);
					}
					if (east < options.areaWest || west > options.areaEast || north < options.areaSouth || south > options.areaNorth)
						continue;
				}

				for (int c = 0; c < 3; ++c)
				{
					const auto [poolIndex, pointIndex] = refs[corner[c]];
					const uint64_t key = (static_cast<uint64_t>(poolIndex) << 32) | pointIndex;
					auto [it, added] = patchPoints.try_emplace(key, 0u);
					if (added)
					{
						const Pool &source = *pools[c];
						if (patch.planes == 0)
							patch.planes = source.planes;
						it->second = patch.GetPointCount();
						const double *point = source.values.data() + static_cast<size_t>(pointIndex) * source.planes;
						for (uint8_t plane = 0; plane < patch.planes; ++plane)
							patch.points.push_back(plane < source.planes ? point[plane] : 0.0);
					}
					patch.indices.push_back(it->second);
				}
			}
		};

		const auto addObject = [&](const Pool &p, const uint32_t index)
		{
			if (index >= p.count)
				return;
			const DsfPoint point = PointOf(p, index);
			if (!boxInArea(point.v[0], point.v[1], point.v[0], point.v[1]))
				return;
			DsfObjectPlacement placement;
			placement.definition = currentDefinition;
			placement.longitude = point.v[0];
			placement.latitude = point.v[1];
			placement.heading = p.planes > 2 ? point.v[2] : 0.0;
			if (p.planes >= 4)
			{
				placement.elevation = point.v[3];
				placement.mode = objectMode;
			}
			else
				placement.mode = DsfObjectPlacement::Mode::Draped;
			placement.filter = currentFilter;
			out.objects.push_back(placement);
		};

		// A network chain: junctions (id plane non-zero) split it into segments; shape points between them are vertices,
		// or bezier control points (4 planes: the level plane 1; 7 planes: the point's own control lon / lat)
		const auto addSegment = [&](DsfRoadSegment &&segment)
		{
			if (options.HasArea())
			{
				double west = 1e9, east = -1e9, south = 1e9, north = -1e9;
				for (const DsfRoadPoint &point : segment.points)
				{
					west = std::min(west, point.longitude);
					east = std::max(east, point.longitude);
					south = std::min(south, point.latitude);
					north = std::max(north, point.latitude);
				}
				if (!boxInArea(west, south, east, north))
					return;
			}
			out.roads.push_back(std::move(segment));
		};
		const auto addChain = [&](const Pool &p, const std::vector<uint32_t> &indices)
		{
			std::optional<DsfRoadSegment> segment;
			for (size_t i = 0; i < indices.size(); ++i)
			{
				if (indices[i] >= p.count)
					continue;
				const DsfPoint point = PointOf(p, indices[i]);
				const uint32_t junction = point.planes >= 4 ? static_cast<uint32_t>(std::max(0.0, std::round(point.v[3]))) : 0u;
				const int level = point.planes >= 3 ? static_cast<int>(std::lround(point.v[2])) : 0;
				const DsfRoadPoint vertex{ point.v[0], point.v[1], false };
				if (junction != 0)
				{
					if (segment)
					{
						segment->points.push_back(vertex);
						segment->endJunction = junction;
						segment->endLevel = level;
						addSegment(std::move(*segment));
						segment.reset();
					}
					if (i + 1 < indices.size())
					{
						segment.emplace();
						segment->definition = currentDefinition;
						segment->subtype = roadSubtype;
						segment->startJunction = junction;
						segment->startLevel = level;
						segment->filter = currentFilter;
						segment->points.push_back(vertex);
					}
					continue;
				}
				if (!segment)
					continue;	// Shape points before the first junction: not part of a segment
				if (point.planes >= 7)
				{
					segment->points.push_back(vertex);
					if (point.v[4] != point.v[0] || point.v[5] != point.v[1])
						segment->points.push_back({ point.v[4], point.v[5], true });
				}
				else
					segment->points.push_back({ point.v[0], point.v[1], point.planes >= 3 && point.v[2] >= 0.5 });
			}
		};

		const auto beginPolygon = [&](const uint16_t parameter, const uint8_t planes)
		{
			DsfPolygonPlacement polygon;
			polygon.definition = currentDefinition;
			polygon.parameter = parameter;
			polygon.planes = planes;
			polygon.filter = currentFilter;
			out.polygons.push_back(std::move(polygon));
		};

		/* @brief With an area: the polygon just read goes unless its box reaches into it (it is kept whole when it does). */
		const auto endPolygon = [&]()
		{
			if (!readOverlays || !options.HasArea() || out.polygons.empty())
				return;
			double west = 1e9, east = -1e9, south = 1e9, north = -1e9;
			for (const std::vector<DsfPoint> &winding : out.polygons.back().windings)
			{
				for (const DsfPoint &point : winding)
				{
					west = std::min(west, point.v[0]);
					east = std::max(east, point.v[0]);
					south = std::min(south, point.v[1]);
					north = std::max(north, point.v[1]);
				}
			}
			if (west > east || !boxInArea(west, south, east, north))
				out.polygons.pop_back();
		};

		const auto readComment = [&](const uint32_t length)
		{
			uint32_t remaining = length;
			if (remaining >= 2)
			{
				const uint16_t type = commands.U16();
				remaining -= 2;
				if ((type == COMMENT_FILTER || type == COMMENT_AGL) && remaining == 4)
				{
					const int32_t value = commands.S32();
					remaining -= 4;
					if (type == COMMENT_AGL)
						objectMode = value ? DsfObjectPlacement::Mode::Agl : DsfObjectPlacement::Mode::Msl;
					else
						currentFilter = value;
				}
			}
			commands.Skip(remaining);
		};

		while (!commands.Done() && !commands.Overrun())
		{
			const uint8_t command = commands.U8();
			switch (command)
			{
				case CMD_RESERVED:
					return Fail(error, "Reserved DSF command at byte " + std::to_string(commands.GetPosition()));
				case CMD_POOL_SELECT:
					currentPool = commands.U16();
					if (currentPool >= pool16.size() && currentPool >= pool32.size())
						return Fail(error, "DSF pool " + std::to_string(currentPool) + " out of range");
					break;
				case CMD_JUNCTION_OFFSET_SELECT:
					junctionOffset = commands.U32();
					break;
				case CMD_SET_DEFINITION8:
					currentDefinition = commands.U8();
					break;
				case CMD_SET_DEFINITION16:
					currentDefinition = commands.U16();
					break;
				case CMD_SET_DEFINITION32:
					currentDefinition = commands.U32();
					break;
				case CMD_SET_ROAD_SUBTYPE8:
					roadSubtype = commands.U8();
					break;

				case CMD_OBJECT:
				{
					const uint32_t index = commands.U16();
					if (const Pool *p = overlayPool(currentPool))
						addObject(*p, index);
					break;
				}
				case CMD_OBJECT_RANGE:
				{
					const uint32_t first = commands.U16();
					const uint32_t last = commands.U16();
					if (const Pool *p = overlayPool(currentPool))
					{
						for (uint32_t index = first; index < last; ++index)
							addObject(*p, index);
					}
					break;
				}

				case CMD_NETWORK_CHAIN:
				case CMD_NETWORK_CHAIN32:
				{
					const uint32_t count = commands.U8();
					std::vector<uint32_t> indices;
					indices.reserve(count);
					for (uint32_t i = 0; i < count; ++i)
						indices.push_back(command == CMD_NETWORK_CHAIN ? junctionOffset + commands.U16() : commands.U32());
					if (const Pool *p = overlayPool32(currentPool))
						addChain(*p, indices);
					break;
				}
				case CMD_NETWORK_CHAIN_RANGE:
				{
					const uint32_t first = junctionOffset + commands.U16();
					const uint32_t last = junctionOffset + commands.U16();
					std::vector<uint32_t> indices;
					for (uint32_t index = first; index < last; ++index)
						indices.push_back(index);
					if (const Pool *p = overlayPool32(currentPool))
						addChain(*p, indices);
					break;
				}

				case CMD_POLYGON:
				{
					const uint16_t parameter = commands.U16();
					const uint32_t count = commands.U8();
					const Pool *p = overlayPool(currentPool);
					beginPolygon(parameter, p ? p->planes : 2);
					std::vector<DsfPoint> winding;
					for (uint32_t i = 0; i < count; ++i)
					{
						const uint32_t index = commands.U16();
						if (p)
							winding.push_back(PointOf(*p, index));
					}
					out.polygons.back().windings.push_back(std::move(winding));
					endPolygon();
					break;
				}
				case CMD_POLYGON_RANGE:
				{
					const uint16_t parameter = commands.U16();
					const uint32_t first = commands.U16();
					const uint32_t last = commands.U16();
					const Pool *p = overlayPool(currentPool);
					beginPolygon(parameter, p ? p->planes : 2);
					std::vector<DsfPoint> winding;
					if (p)
					{
						for (uint32_t index = first; index < last; ++index)
							winding.push_back(PointOf(*p, index));
					}
					out.polygons.back().windings.push_back(std::move(winding));
					endPolygon();
					break;
				}
				case CMD_NESTED_POLYGON:
				{
					const uint16_t parameter = commands.U16();
					const uint32_t windings = commands.U8();
					const Pool *p = overlayPool(currentPool);
					beginPolygon(parameter, p ? p->planes : 2);
					for (uint32_t w = 0; w < windings; ++w)
					{
						const uint32_t count = commands.U8();
						std::vector<DsfPoint> winding;
						for (uint32_t i = 0; i < count; ++i)
						{
							const uint32_t index = commands.U16();
							if (p)
								winding.push_back(PointOf(*p, index));
						}
						out.polygons.back().windings.push_back(std::move(winding));
					}
					endPolygon();
					break;
				}
				case CMD_NESTED_POLYGON_RANGE:
				{
					const uint16_t parameter = commands.U16();
					const uint32_t windings = commands.U8();
					uint32_t first = commands.U16();
					const Pool *p = overlayPool(currentPool);
					beginPolygon(parameter, p ? p->planes : 2);
					for (uint32_t w = 0; w < windings; ++w)
					{
						const uint32_t last = commands.U16();
						std::vector<DsfPoint> winding;
						if (p)
						{
							for (uint32_t index = first; index < last; ++index)
								winding.push_back(PointOf(*p, index));
						}
						out.polygons.back().windings.push_back(std::move(winding));
						first = last;
					}
					endPolygon();
					break;
				}

				// Terrain: a patch takes the current definition (and flags / LOD, which carry over to the next patch);
				// its triangles follow
				case CMD_TERRAIN_PATCH:
					beginPatch();
					break;
				case CMD_TERRAIN_PATCH_FLAGS:
					patchFlags = commands.U8();
					beginPatch();
					break;
				case CMD_TERRAIN_PATCH_FLAGS_LOD:
					patchFlags = commands.U8();
					patchLodStart = commands.F32();
					patchLodEnd = commands.F32();
					beginPatch();
					break;
				case CMD_TRIANGLE:
				case CMD_TRIANGLE_STRIP:
				case CMD_TRIANGLE_FAN:
				{
					const uint32_t count = commands.U8();
					refs.clear();
					for (uint32_t i = 0; i < count; ++i)
						refs.emplace_back(currentPool, commands.U16());
					addTriangles(command == CMD_TRIANGLE ? 0 : (command == CMD_TRIANGLE_STRIP ? 1 : 2));
					break;
				}
				case CMD_TRIANGLE_CROSS_POOL:
				case CMD_TRIANGLE_STRIP_CROSS_POOL:
				case CMD_TRIANGLE_FAN_CROSS_POOL:
				{
					const uint32_t count = commands.U8();
					refs.clear();
					for (uint32_t i = 0; i < count; ++i)
					{
						const uint32_t poolIndex = commands.U16();
						refs.emplace_back(poolIndex, commands.U16());
					}
					addTriangles(command == CMD_TRIANGLE_CROSS_POOL ? 0 : (command == CMD_TRIANGLE_STRIP_CROSS_POOL ? 1 : 2));
					break;
				}
				case CMD_TRIANGLE_RANGE:
				case CMD_TRIANGLE_STRIP_RANGE:
				case CMD_TRIANGLE_FAN_RANGE:
				{
					const uint32_t first = commands.U16();
					const uint32_t last = commands.U16();
					refs.clear();
					for (uint32_t index = first; index < last; ++index)
						refs.emplace_back(currentPool, index);
					addTriangles(command == CMD_TRIANGLE_RANGE ? 0 : (command == CMD_TRIANGLE_STRIP_RANGE ? 1 : 2));
					break;
				}

				case CMD_COMMENT8:
					readComment(commands.U8());
					break;
				case CMD_COMMENT16:
					readComment(commands.U16());
					break;
				case CMD_COMMENT32:
					readComment(commands.U32());
					break;

				default:
					return Fail(error, "Unknown DSF command " + std::to_string(command) + " at byte " + std::to_string(commands.GetPosition()));
			}
			if (!poolError.empty())
				return Fail(error, poolError);
		}

		if (commands.Overrun())
			return Fail(error, "The DSF command stream ends in the middle of a command");
		if (!out.terrainPatches.empty() && out.terrainPatches.back().indices.empty())
			out.terrainPatches.pop_back();
		if (!readOverlays)
			out.polygons.clear();	// Commands walked for their length only
		return true;
	}

	bool LoadDsf(const std::filesystem::path &path, DsfFile &out, std::string *error)
	{
		return LoadDsf(path, out, DsfReadOptions{}, error);
	}

	bool LoadDsf(const std::filesystem::path &path, DsfFile &out, const DsfReadOptions &options, std::string *error)
	{
		std::string bytes;
		std::string readError;
		if (!VFS::ReadFile(path, bytes, &readError))
			return Fail(error, readError);
		if (!ReadDsf(reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size(), out, options, &readError))
			return Fail(error, VFS::ToUtf8(path.filename()) + ": " + readError);
		return true;
	}

	bool IsDsfCompressed(const std::filesystem::path &path)
	{
		std::ifstream file(path, std::ios::binary);
		char signature[6] = {};
		if (!file || !file.read(signature, sizeof(signature)))
			return false;
		return std::memcmp(signature, "7z\xBC\xAF\x27\x1C", 6) == 0;
	}

	// -------------------------------------------------------
	// Writing

	namespace
	{
		/* @brief Little-endian byte sink with atoms (id, size including the 8-byte header, payload). */
		class Writer
		{
		public:
			void U8(const uint8_t value) { m_Bytes.push_back(value); }
			void U16(const uint16_t value) { U8(static_cast<uint8_t>(value)); U8(static_cast<uint8_t>(value >> 8)); }
			void U32(const uint32_t value)
			{
				for (int shift = 0; shift < 32; shift += 8)
					U8(static_cast<uint8_t>(value >> shift));
			}
			void S32(const int32_t value) { U32(static_cast<uint32_t>(value)); }
			void F32(const float value)
			{
				uint32_t bits;
				std::memcpy(&bits, &value, sizeof(bits));
				U32(bits);
			}
			void Bytes(const void *data, const size_t size)
			{
				const auto *bytes = static_cast<const uint8_t *>(data);
				m_Bytes.insert(m_Bytes.end(), bytes, bytes + size);
			}
			void String(const std::string_view text)
			{
				Bytes(text.data(), text.size());
				U8(0);
			}

			[[nodiscard]] size_t BeginAtom(const uint32_t id)
			{
				const size_t start = m_Bytes.size();
				U32(id);
				U32(0);
				return start;
			}
			void EndAtom(const size_t start)
			{
				const auto size = static_cast<uint32_t>(m_Bytes.size() - start);
				for (int i = 0; i < 4; ++i)
					m_Bytes[start + 4 + i] = static_cast<uint8_t>(size >> (8 * i));
			}
			void StringTableAtom(const uint32_t id, const std::vector<std::string> &strings)
			{
				const size_t atom = BeginAtom(id);
				for (const std::string &text : strings)
					String(text);
				EndAtom(atom);
			}

			[[nodiscard]] std::vector<uint8_t> &Data() { return m_Bytes; }

		private:
			std::vector<uint8_t> m_Bytes;
		};

		/* @brief The float scale and offset of a pool plane: decoded = raw * scale / 65535 + offset. */
		struct PlaneScale
		{
			float scale = 1.0f;
			float offset = 0.0f;
		};

		/**
		 * @brief Scale and offset for values from @p minimum to @p maximum, chosen as floats (what the file stores) that
		 * still cover the range, so quantizing against them loses only the 16-bit step. Whole numbers in 0..65535 use
		 * scale 65535 / offset 0: every raw value decodes to itself.
		 */
		PlaneScale ChoosePlaneScale(const double minimum, const double maximum, const bool whole)
		{
			if (whole)
				return { 65535.0f, 0.0f };
			constexpr float infinity = std::numeric_limits<float>::infinity();
			PlaneScale scale;
			scale.offset = static_cast<float>(minimum);
			if (static_cast<double>(scale.offset) > minimum)
				scale.offset = std::nextafter(scale.offset, -infinity);
			scale.scale = static_cast<float>(maximum - static_cast<double>(scale.offset));
			while (static_cast<double>(scale.offset) + static_cast<double>(scale.scale) < maximum)
				scale.scale = std::nextafter(scale.scale, infinity);
			if (!(scale.scale > 0.0f))
				scale.scale = 1.0f;		// A constant plane: raw 0 decodes to the offset
			return scale;
		}

		uint16_t QuantizePlane(const double value, const PlaneScale &scale)
		{
			const double raw = std::round((value - static_cast<double>(scale.offset)) / static_cast<double>(scale.scale) * 65535.0);
			return static_cast<uint16_t>(std::clamp(raw, 0.0, 65535.0));
		}

		/* @brief 32-bit pools: scale 0 stores whole numbers as they are (junction ids, levels, control flags). */
		uint32_t QuantizePlane32(const double value, const PlaneScale &scale)
		{
			if (scale.scale == 0.0f)
				return static_cast<uint32_t>(std::clamp(std::round(value), 0.0, 4294967295.0));
			const double raw = std::round((value - static_cast<double>(scale.offset)) / static_cast<double>(scale.scale) * 4294967295.0);
			return static_cast<uint32_t>(std::clamp(raw, 0.0, 4294967295.0));
		}

		/* @brief A 16-bit point pool being filled: one plane count, one kind of content (objects or polygons). */
		struct PoolBuild
		{
			bool objects = false;
			uint8_t planes = 0;
			std::vector<DsfPoint> points;
			std::array<double, 8> minimum{};	// Per plane
			std::array<double, 8> maximum{};
		};

		/* @brief Assigns content to pools: the open pool of its kind while the pool stays small, else a new one. */
		class PoolPlanner
		{
		public:
			/**
			 * @brief Puts points into a pool.
			 * @param pool Receives the pool's index.
			 * @param first Receives the index of the first point in it.
			 * @return False when the points don't fit any pool (over 65535).
			 */
			bool Place(const bool objects, const uint8_t planes, const std::vector<DsfPoint> &points, uint32_t &pool, uint32_t &first)
			{
				if (points.empty() || points.size() > 65535)
					return false;
				std::array<double, 8> minimum{}, maximum{};
				for (uint8_t plane = 0; plane < planes; ++plane)
				{
					minimum[plane] = maximum[plane] = points[0].v[plane];
					for (const DsfPoint &point : points)
					{
						minimum[plane] = std::min(minimum[plane], point.v[plane]);
						maximum[plane] = std::max(maximum[plane], point.v[plane]);
					}
				}

				// Join the open pool while it stays small: lon / lat within DSF_POOL_SPAN, and no other plane widened
				// past what either side needs (a UV plane never shares a pool with bezier coordinates in that plane)
				const uint32_t key = (objects ? 0x100u : 0u) | planes;
				size_t index = m_Pools.size();
				if (const auto it = m_Open.find(key); it != m_Open.end())
				{
					const PoolBuild &open = m_Pools[it->second];
					bool fits = open.points.size() + points.size() <= 65535;
					for (uint8_t plane = 0; plane < planes && fits; ++plane)
					{
						const double merged = std::max(open.maximum[plane], maximum[plane]) - std::min(open.minimum[plane], minimum[plane]);
						const double allowed = plane < 2 ? DSF_POOL_SPAN :
							std::max({ DSF_POOL_SPAN, open.maximum[plane] - open.minimum[plane], maximum[plane] - minimum[plane] });
						fits = merged <= allowed;
					}
					if (fits)
						index = it->second;
				}
				if (index == m_Pools.size())
				{
					PoolBuild &created = m_Pools.emplace_back();
					created.objects = objects;
					created.planes = planes;
					created.minimum = minimum;
					created.maximum = maximum;
					m_Open[key] = index;
				}

				PoolBuild &target = m_Pools[index];
				for (uint8_t plane = 0; plane < planes; ++plane)
				{
					target.minimum[plane] = std::min(target.minimum[plane], minimum[plane]);
					target.maximum[plane] = std::max(target.maximum[plane], maximum[plane]);
				}
				pool = static_cast<uint32_t>(index);
				first = static_cast<uint32_t>(target.points.size());
				for (DsfPoint point : points)
				{
					point.planes = planes;
					target.points.push_back(point);
				}
				return true;
			}

			[[nodiscard]] const std::vector<PoolBuild> &GetPools() const { return m_Pools; }

		private:
			std::vector<PoolBuild> m_Pools;
			std::map<uint32_t, size_t> m_Open;
		};

		/* @brief "w/s/e/n[;...]" exclusion bounds. */
		bool ParseExclusionBounds(const std::string &value, double &west, double &south, double &east, double &north)
		{
			return std::sscanf(value.c_str(), "%lf/%lf/%lf/%lf", &west, &south, &east, &north) == 4;
		}

		/* @brief Last whole-degree tile index a range [from, to] reaches (a range ending on a tile edge stays below it). */
		int LastTile(const double from, const double to)
		{
			const double edge = std::floor(to);
			return static_cast<int>(edge == to && to > from ? edge - 1.0 : edge);
		}
	}

	std::array<uint8_t, 16> Md5(const uint8_t *data, const size_t size)
	{
		static constexpr uint32_t K[64] = {
			0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
			0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
			0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
			0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
			0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
			0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
			0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
			0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
		static constexpr uint8_t R[64] = {
			7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
			5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
			4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
			6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21 };

		// The message, a 1 bit, zeros to 56 bytes mod 64, then its length in bits
		std::vector<uint8_t> message(data, data + size);
		message.push_back(0x80);
		while (message.size() % 64 != 56)
			message.push_back(0);
		const uint64_t bits = static_cast<uint64_t>(size) * 8u;
		for (int i = 0; i < 8; ++i)
			message.push_back(static_cast<uint8_t>(bits >> (8 * i)));

		uint32_t h[4] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };
		for (size_t chunk = 0; chunk < message.size(); chunk += 64)
		{
			uint32_t m[16];
			for (int i = 0; i < 16; ++i)
			{
				const uint8_t *word = &message[chunk + static_cast<size_t>(i) * 4];
				m[i] = static_cast<uint32_t>(word[0]) | (static_cast<uint32_t>(word[1]) << 8) | (static_cast<uint32_t>(word[2]) << 16) | (static_cast<uint32_t>(word[3]) << 24);
			}
			uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
			for (int i = 0; i < 64; ++i)
			{
				uint32_t f = 0, g = 0;
				if (i < 16) { f = (b & c) | (~b & d); g = static_cast<uint32_t>(i); }
				else if (i < 32) { f = (d & b) | (~d & c); g = static_cast<uint32_t>(5 * i + 1) % 16u; }
				else if (i < 48) { f = b ^ c ^ d; g = static_cast<uint32_t>(3 * i + 5) % 16u; }
				else { f = c ^ (b | ~d); g = static_cast<uint32_t>(7 * i) % 16u; }
				f += a + K[i] + m[g];
				a = d;
				d = c;
				c = b;
				b += (f << R[i]) | (f >> (32 - R[i]));
			}
			h[0] += a;
			h[1] += b;
			h[2] += c;
			h[3] += d;
		}

		std::array<uint8_t, 16> digest{};
		for (int i = 0; i < 16; ++i)
			digest[static_cast<size_t>(i)] = static_cast<uint8_t>(h[i / 4] >> (8 * (i % 4)));
		return digest;
	}

	bool WriteDsf(const DsfFile &dsf, std::vector<uint8_t> &out, std::string *error)
	{
		out.clear();
		PoolPlanner planner;
		Writer commands;
		uint32_t currentPool = std::numeric_limits<uint32_t>::max();
		uint32_t currentDefinition = std::numeric_limits<uint32_t>::max();
		bool agl = false;	// Readers start out MSL
		int32_t currentFilter = -1;

		const auto selectPool = [&](const uint32_t pool)
		{
			if (pool == currentPool)
				return;
			commands.U8(CMD_POOL_SELECT);
			commands.U16(static_cast<uint16_t>(pool));
			currentPool = pool;
		};
		const auto selectFilter = [&](const int32_t filter)
		{
			if (filter == currentFilter)
				return;
			commands.U8(CMD_COMMENT8);
			commands.U8(6);
			commands.U16(COMMENT_FILTER);
			commands.S32(filter);
			currentFilter = filter;
		};
		const auto selectDefinition = [&](const uint32_t definition)
		{
			if (definition == currentDefinition)
				return;
			if (definition <= 0xFFu)
			{
				commands.U8(CMD_SET_DEFINITION8);
				commands.U8(static_cast<uint8_t>(definition));
			}
			else if (definition <= 0xFFFFu)
			{
				commands.U8(CMD_SET_DEFINITION16);
				commands.U16(static_cast<uint16_t>(definition));
			}
			else
			{
				commands.U8(CMD_SET_DEFINITION32);
				commands.U32(definition);
			}
			currentDefinition = definition;
		};

		// Objects: lon, lat, heading (draped) plus the elevation (MSL / AGL)
		for (const DsfObjectPlacement &object : dsf.objects)
		{
			if (object.definition >= dsf.objectDefinitions.size())
				return Fail(error, "An object uses definition " + std::to_string(object.definition) + ", which the DSF doesn't define");
			const bool elevated = object.mode != DsfObjectPlacement::Mode::Draped;
			DsfPoint point;
			point.v[0] = object.longitude;
			point.v[1] = object.latitude;
			point.v[2] = std::fmod(std::fmod(object.heading, 360.0) + 360.0, 360.0);
			point.v[3] = object.elevation;
			const uint8_t planes = elevated ? 4 : 3;

			uint32_t pool = 0, index = 0;
			if (!planner.Place(true, planes, { point }, pool, index))
				return Fail(error, "Too many object placements for one DSF");
			selectPool(pool);
			selectDefinition(object.definition);
			selectFilter(object.filter);
			if (elevated && agl != (object.mode == DsfObjectPlacement::Mode::Agl))
			{
				agl = object.mode == DsfObjectPlacement::Mode::Agl;
				commands.U8(CMD_COMMENT8);
				commands.U8(6);
				commands.U16(COMMENT_AGL);
				commands.S32(agl ? 1 : 0);
			}
			commands.U8(CMD_OBJECT);
			commands.U16(static_cast<uint16_t>(index));
		}

		// Polygons, in order (draped ones draw in file order): each one's windings back to back in one pool
		for (size_t p = 0; p < dsf.polygons.size(); ++p)
		{
			const DsfPolygonPlacement &polygon = dsf.polygons[p];
			if (polygon.definition >= dsf.polygonDefinitions.size())
				return Fail(error, "Polygon " + std::to_string(p) + " uses definition " + std::to_string(polygon.definition) + ", which the DSF doesn't define");
			if (polygon.windings.empty() || polygon.windings.size() > 255)
				return Fail(error, "Polygon " + std::to_string(p) + " (" + dsf.polygonDefinitions[polygon.definition] + ") has " +
					std::to_string(polygon.windings.size()) + " windings; a DSF polygon has 1 to 255");
			if (polygon.planes < 2 || polygon.planes > 8)
				return Fail(error, "Polygon " + std::to_string(p) + " has " + std::to_string(polygon.planes) + " planes per point");

			std::vector<DsfPoint> points;
			for (const std::vector<DsfPoint> &winding : polygon.windings)
			{
				if (winding.empty())
					return Fail(error, "Polygon " + std::to_string(p) + " (" + dsf.polygonDefinitions[polygon.definition] + ") has an empty winding");
				points.insert(points.end(), winding.begin(), winding.end());
			}

			uint32_t pool = 0, first = 0;
			if (!planner.Place(false, polygon.planes, points, pool, first))
				return Fail(error, "Polygon " + std::to_string(p) + " (" + dsf.polygonDefinitions[polygon.definition] + ") has " +
					std::to_string(points.size()) + " points; a DSF polygon holds at most 65535");
			selectPool(pool);
			selectDefinition(polygon.definition);
			selectFilter(polygon.filter);
			if (polygon.windings.size() == 1)
			{
				commands.U8(CMD_POLYGON_RANGE);
				commands.U16(polygon.parameter);
				commands.U16(static_cast<uint16_t>(first));
				commands.U16(static_cast<uint16_t>(first + points.size()));
			}
			else
			{
				commands.U8(CMD_NESTED_POLYGON_RANGE);
				commands.U16(polygon.parameter);
				commands.U8(static_cast<uint8_t>(polygon.windings.size()));
				commands.U16(static_cast<uint16_t>(first));
				uint32_t end = first;
				for (const std::vector<DsfPoint> &winding : polygon.windings)
				{
					end += static_cast<uint32_t>(winding.size());
					commands.U16(static_cast<uint16_t>(end));
				}
			}
		}

		// Road segments: one 32-bit pool of lon, lat, level (or for shape points the control flag), junction id; each
		// segment a contiguous range from its start junction to its end junction
		std::vector<DsfPoint> roadPoints;
		int32_t currentSubtype = -1;
		uint32_t junctionOffset = std::numeric_limits<uint32_t>::max();
		for (size_t r = 0; r < dsf.roads.size(); ++r)
		{
			const DsfRoadSegment &road = dsf.roads[r];
			const std::string what = "Road " + std::to_string(r);
			if (road.definition >= dsf.networkDefinitions.size())
				return Fail(error, what + " uses network definition " + std::to_string(road.definition) + ", which the DSF doesn't define");
			if (road.subtype > 255)
				return Fail(error, what + " has subtype " + std::to_string(road.subtype) + "; a DSF road subtype is 0 to 255");
			if (road.points.size() < 2 || road.points.front().control || road.points.back().control)
				return Fail(error, what + " needs a vertex at each end");
			if (road.points.size() > 65535)
				return Fail(error, what + " has " + std::to_string(road.points.size()) + " points; a DSF network chain holds at most 65535");
			if (road.startJunction == 0 || road.endJunction == 0)
				return Fail(error, what + " has no junction id at an end (ids start at 1)");

			const uint32_t first = static_cast<uint32_t>(roadPoints.size());
			for (size_t i = 0; i < road.points.size(); ++i)
			{
				const DsfRoadPoint &source = road.points[i];
				DsfPoint point;
				point.planes = 4;
				point.v[0] = source.longitude;
				point.v[1] = source.latitude;
				if (i == 0 || i + 1 == road.points.size())
				{
					point.v[2] = std::max(0, i == 0 ? road.startLevel : road.endLevel);
					point.v[3] = i == 0 ? road.startJunction : road.endJunction;
				}
				else
					point.v[2] = source.control ? 1.0 : 0.0;
				roadPoints.push_back(point);
			}

			selectPool(0);	// The pool index selects the 32-bit pool for network commands
			selectDefinition(road.definition);
			selectFilter(road.filter);
			if (static_cast<int32_t>(road.subtype) != currentSubtype)
			{
				commands.U8(CMD_SET_ROAD_SUBTYPE8);
				commands.U8(static_cast<uint8_t>(road.subtype));
				currentSubtype = static_cast<int32_t>(road.subtype);
			}
			if (first != junctionOffset)
			{
				commands.U8(CMD_JUNCTION_OFFSET_SELECT);
				commands.U32(first);
				junctionOffset = first;
			}
			commands.U8(CMD_NETWORK_CHAIN_RANGE);
			commands.U16(0);
			commands.U16(static_cast<uint16_t>(road.points.size()));
		}

		const std::vector<PoolBuild> &pools = planner.GetPools();
		if (pools.size() > 65535)
			return Fail(error, "The DSF needs more than 65535 point pools");

		Writer file;
		file.Bytes("XPLNEDSF", 8);
		file.U32(1);

		const size_t head = file.BeginAtom(ATOM_HEAD);
		{
			const size_t prop = file.BeginAtom(ATOM_PROP);
			for (const auto &[key, value] : dsf.properties)
			{
				file.String(key);
				file.String(value);
			}
			file.EndAtom(prop);
		}
		file.EndAtom(head);

		const size_t defn = file.BeginAtom(ATOM_DEFN);
		file.StringTableAtom(ATOM_TERT, dsf.terrainDefinitions);
		file.StringTableAtom(ATOM_OBJT, dsf.objectDefinitions);
		file.StringTableAtom(ATOM_POLY, dsf.polygonDefinitions);
		file.StringTableAtom(ATOM_NETW, dsf.networkDefinitions);
		file.StringTableAtom(ATOM_DEMN, dsf.rasterDefinitions);
		file.EndAtom(defn);

		const size_t geod = file.BeginAtom(ATOM_GEOD);
		for (const PoolBuild &pool : pools)
		{
			std::vector<PlaneScale> scales(pool.planes);
			for (uint8_t plane = 0; plane < pool.planes; ++plane)
			{
				double minimum = std::numeric_limits<double>::max(), maximum = std::numeric_limits<double>::lowest();
				bool whole = true;
				for (const DsfPoint &point : pool.points)
				{
					const double value = point.v[plane];
					minimum = std::min(minimum, value);
					maximum = std::max(maximum, value);
					whole = whole && value >= 0.0 && value <= 65535.0 && value == std::floor(value);
				}
				scales[plane] = ChoosePlaneScale(minimum, maximum, whole);
			}

			const size_t poolAtom = file.BeginAtom(ATOM_POOL);
			file.U32(static_cast<uint32_t>(pool.points.size()));
			file.U8(pool.planes);
			for (uint8_t plane = 0; plane < pool.planes; ++plane)
			{
				file.U8(ENCODE_RAW);
				for (const DsfPoint &point : pool.points)
					file.U16(QuantizePlane(point.v[plane], scales[plane]));
			}
			file.EndAtom(poolAtom);

			const size_t scalAtom = file.BeginAtom(ATOM_SCAL);
			for (const PlaneScale &scale : scales)
			{
				file.F32(scale.scale);
				file.F32(scale.offset);
			}
			file.EndAtom(scalAtom);
		}
		if (!roadPoints.empty())
		{
			constexpr uint8_t planes = 4;
			std::array<PlaneScale, planes> scales{};
			for (uint8_t plane = 0; plane < planes; ++plane)
			{
				double minimum = std::numeric_limits<double>::max(), maximum = std::numeric_limits<double>::lowest();
				bool whole = true;
				for (const DsfPoint &point : roadPoints)
				{
					minimum = std::min(minimum, point.v[plane]);
					maximum = std::max(maximum, point.v[plane]);
					whole = whole && point.v[plane] >= 0.0 && point.v[plane] == std::floor(point.v[plane]);
				}
				// Lon / lat scaled to their range; ids, levels and flags stored as whole numbers (scale 0)
				scales[plane] = plane >= 2 && whole ? PlaneScale{ 0.0f, 0.0f } : ChoosePlaneScale(minimum, maximum, false);
			}

			const size_t poolAtom = file.BeginAtom(ATOM_PO32);
			file.U32(static_cast<uint32_t>(roadPoints.size()));
			file.U8(planes);
			for (uint8_t plane = 0; plane < planes; ++plane)
			{
				file.U8(ENCODE_RAW);
				for (const DsfPoint &point : roadPoints)
					file.U32(QuantizePlane32(point.v[plane], scales[plane]));
			}
			file.EndAtom(poolAtom);

			const size_t scalAtom = file.BeginAtom(ATOM_SC32);
			for (const PlaneScale &scale : scales)
			{
				file.F32(scale.scale);
				file.F32(scale.offset);
			}
			file.EndAtom(scalAtom);
		}
		file.EndAtom(geod);

		const size_t cmds = file.BeginAtom(ATOM_CMDS);
		file.Bytes(commands.Data().data(), commands.Data().size());
		file.EndAtom(cmds);

		const std::array<uint8_t, 16> digest = Md5(file.Data().data(), file.Data().size());
		file.Bytes(digest.data(), digest.size());
		out = std::move(file.Data());
		return true;
	}

	std::vector<DsfFile> SplitDsfByTile(const DsfFile &dsf, std::vector<std::string> *warnings)
	{
		using TileKey = std::pair<int, int>;	// south, west
		struct Tile
		{
			DsfFile file;
			std::map<uint32_t, uint32_t> objectDefinitions;
			std::map<uint32_t, uint32_t> polygonDefinitions;
			std::map<uint32_t, uint32_t> networkDefinitions;
		};
		std::map<TileKey, Tile> tiles;
		const auto tileOf = [](const double latitude, const double longitude)
		{
			return TileKey{ static_cast<int>(std::floor(latitude)), static_cast<int>(std::floor(longitude)) };
		};
		const auto remap = [](std::map<uint32_t, uint32_t> &map, std::vector<std::string> &names, const std::vector<std::string> &source, const uint32_t definition)
		{
			const auto [it, added] = map.try_emplace(definition, static_cast<uint32_t>(names.size()));
			if (added)
				names.push_back(definition < source.size() ? source[definition] : std::string());
			return it->second;
		};

		for (const DsfObjectPlacement &object : dsf.objects)
		{
			Tile &tile = tiles[tileOf(object.latitude, object.longitude)];
			DsfObjectPlacement copy = object;
			copy.definition = remap(tile.objectDefinitions, tile.file.objectDefinitions, dsf.objectDefinitions, object.definition);
			tile.file.objects.push_back(copy);
		}

		for (const DsfPolygonPlacement &polygon : dsf.polygons)
		{
			if (polygon.windings.empty() || polygon.windings[0].empty())
				continue;
			const DsfPoint &anchor = polygon.windings[0][0];
			const TileKey key = tileOf(anchor.Latitude(), anchor.Longitude());
			bool crosses = false;
			for (const std::vector<DsfPoint> &winding : polygon.windings)
			{
				for (const DsfPoint &point : winding)
					crosses = crosses || tileOf(point.Latitude(), point.Longitude()) != key;
			}
			if (crosses && warnings)
			{
				const std::string name = polygon.definition < dsf.polygonDefinitions.size() ? dsf.polygonDefinitions[polygon.definition] : std::string("a polygon");
				warnings->push_back(name + " crosses a tile edge; it is written to tile " + GetDsfTileName(anchor.Latitude(), anchor.Longitude()) + " whole");
			}
			Tile &tile = tiles[key];
			DsfPolygonPlacement copy = polygon;
			copy.definition = remap(tile.polygonDefinitions, tile.file.polygonDefinitions, dsf.polygonDefinitions, polygon.definition);
			tile.file.polygons.push_back(std::move(copy));
		}

		// Road segments go to the tile of their start junction (junction ids stay as they are: unique in each tile too)
		for (const DsfRoadSegment &road : dsf.roads)
		{
			if (road.points.empty())
				continue;
			const DsfRoadPoint &anchor = road.points.front();
			const TileKey key = tileOf(anchor.latitude, anchor.longitude);
			const bool crosses = std::ranges::any_of(road.points, [&](const DsfRoadPoint &point) { return tileOf(point.latitude, point.longitude) != key; });
			if (crosses && warnings)
			{
				const std::string name = road.definition < dsf.networkDefinitions.size() ? dsf.networkDefinitions[road.definition] : std::string("a road");
				warnings->push_back(name + " segment crosses a tile edge; it is written to tile " + GetDsfTileName(anchor.latitude, anchor.longitude) + " whole");
			}
			Tile &tile = tiles[key];
			DsfRoadSegment copy = road;
			copy.definition = remap(tile.networkDefinitions, tile.file.networkDefinitions, dsf.networkDefinitions, road.definition);
			tile.file.roads.push_back(std::move(copy));
		}

		// Exclusions go to every tile they touch (a tile may hold nothing but exclusions)
		std::vector<std::pair<std::string, std::string>> shared;
		std::vector<std::pair<TileKey, std::pair<std::string, std::string>>> exclusions;
		for (const auto &[key, value] : dsf.properties)
		{
			if (key == "sim/west" || key == "sim/east" || key == "sim/north" || key == "sim/south")
				continue;
			if (!key.starts_with("sim/exclude_"))
			{
				shared.emplace_back(key, value);
				continue;
			}
			double west = 0.0, south = 0.0, east = 0.0, north = 0.0;
			if (!ParseExclusionBounds(value, west, south, east, north))
			{
				if (warnings)
					warnings->push_back(key + " \"" + value + "\" isn't west/south/east/north; it was left out");
				continue;
			}
			for (int lat = static_cast<int>(std::floor(south)); lat <= LastTile(south, north); ++lat)
			{
				for (int lon = static_cast<int>(std::floor(west)); lon <= LastTile(west, east); ++lon)
				{
					tiles.try_emplace(TileKey{ lat, lon });
					exclusions.emplace_back(TileKey{ lat, lon }, std::make_pair(key, value));
				}
			}
		}

		std::vector<DsfFile> out;
		for (auto &[key, tile] : tiles)
		{
			DsfFile &file = tile.file;
			file.south = key.first;
			file.west = key.second;
			file.north = file.south + 1.0;
			file.east = file.west + 1.0;

			// Bounds after sim/overlay when it leads (WorldEditor's order), else first
			file.properties = shared;
			const std::pair<std::string, std::string> bounds[] = {
				{ "sim/west", std::to_string(key.second) }, { "sim/east", std::to_string(key.second + 1) },
				{ "sim/north", std::to_string(key.first + 1) }, { "sim/south", std::to_string(key.first) } };
			const size_t at = !file.properties.empty() && file.properties[0].first == "sim/overlay" ? 1 : 0;
			file.properties.insert(file.properties.begin() + static_cast<std::ptrdiff_t>(at), std::begin(bounds), std::end(bounds));
			for (const auto &[tileKey, property] : exclusions)
			{
				if (tileKey == key)
					file.properties.push_back(property);
			}
			out.push_back(std::move(file));
		}
		return out;
	}

	std::string GetDsfTileName(const double latitude, const double longitude)
	{
		const int lat = static_cast<int>(std::floor(latitude));
		const int lon = static_cast<int>(std::floor(longitude));
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%+03d%+04d", lat, lon);
		return buffer;
	}

	std::string GetDsfTileFolderName(const double latitude, const double longitude)
	{
		const int lat = static_cast<int>(std::floor(std::floor(latitude) / 10.0)) * 10;
		const int lon = static_cast<int>(std::floor(std::floor(longitude) / 10.0)) * 10;
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%+03d%+04d", lat, lon);
		return buffer;
	}

	std::filesystem::path GetDsfTilePath(const std::filesystem::path &packageFolder, const double latitude, const double longitude)
	{
		return packageFolder / "Earth nav data" / GetDsfTileFolderName(latitude, longitude) / (GetDsfTileName(latitude, longitude) + ".dsf");
	}
}

// -------------------------------------------------------
