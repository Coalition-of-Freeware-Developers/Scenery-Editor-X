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
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>

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

	// -------------------------------------------------------

	bool ReadDsf(const uint8_t *data, const size_t size, DsfFile &out, std::string *error)
	{
		out = DsfFile();
		if (!data || size < 8 + 4 + 16)
			return Fail(error, "Not a DSF file (too small)");
		if (std::memcmp(data, "7z\xBC\xAF\x27\x1C", 6) == 0)
			return Fail(error, "The DSF is 7-zip compressed; compressed tiles aren't supported yet");
		if (std::memcmp(data, "XPLNEDSF", 8) != 0)
			return Fail(error, "Not a DSF file (bad signature)");

		// Header, atoms, then a 16-byte MD5 footer
		Reader reader(data + 8, size - 8 - 16);
		const uint32_t version = reader.U32();
		if (version != 1)
			return Fail(error, "Unsupported DSF version " + std::to_string(version));

		std::vector<std::vector<std::pair<float, float>>> scales16, scales32;
		std::vector<Reader> pools16, pools32;
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
				default:
					break;	// DEMS rasters and anything newer
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

		// Decode the pools (the nth SCAL belongs to the nth POOL)
		std::vector<Pool> pool16(pools16.size()), pool32(pools32.size());
		for (size_t i = 0; i < pools16.size(); ++i)
		{
			if (!ReadPool<uint16_t>(pools16[i], scales16[i], pool16[i], 1.0 / 65535.0))
				return Fail(error, "Corrupt 16-bit point pool " + std::to_string(i));
		}
		for (size_t i = 0; i < pools32.size(); ++i)
		{
			if (!ReadPool<uint32_t>(pools32[i], scales32[i], pool32[i], 1.0 / 4294967295.0))
				return Fail(error, "Corrupt 32-bit point pool " + std::to_string(i));
		}

		// Command stream
		uint32_t currentPool = 0;
		uint32_t currentDefinition = 0;
		uint32_t junctionOffset = 0;
		uint8_t roadSubtype = 0;
		DsfObjectPlacement::Mode objectMode = DsfObjectPlacement::Mode::Msl;
		bool patchOpen = false;

		const auto pool = [&](const uint32_t index) -> const Pool * { return index < pool16.size() ? &pool16[index] : nullptr; };
		const auto pool32At = [&](const uint32_t index) -> const Pool * { return index < pool32.size() ? &pool32[index] : nullptr; };

		const auto addObject = [&](const Pool &p, const uint32_t index)
		{
			if (index >= p.count)
				return;
			const DsfPoint point = PointOf(p, index);
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
			out.objects.push_back(placement);
		};

		const auto beginPolygon = [&](const uint16_t parameter, const uint8_t planes)
		{
			DsfPolygonPlacement polygon;
			polygon.definition = currentDefinition;
			polygon.parameter = parameter;
			polygon.planes = planes;
			out.polygons.push_back(std::move(polygon));
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
					if (const Pool *p = pool(currentPool))
						addObject(*p, index);
					break;
				}
				case CMD_OBJECT_RANGE:
				{
					const uint32_t first = commands.U16();
					const uint32_t last = commands.U16();
					if (const Pool *p = pool(currentPool))
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
					const Pool *p = pool32At(currentPool);
					DsfNetworkChain chain;
					chain.definition = currentDefinition;
					chain.subtype = roadSubtype;
					for (uint32_t i = 0; i < count; ++i)
					{
						const uint32_t index = command == CMD_NETWORK_CHAIN ? junctionOffset + commands.U16() : commands.U32();
						if (p)
							chain.points.push_back(PointOf(*p, index));
					}
					if (p && !chain.points.empty())
						out.networks.push_back(std::move(chain));
					break;
				}
				case CMD_NETWORK_CHAIN_RANGE:
				{
					const uint32_t first = junctionOffset + commands.U16();
					const uint32_t last = junctionOffset + commands.U16();
					if (const Pool *p = pool32At(currentPool))
					{
						DsfNetworkChain chain;
						chain.definition = currentDefinition;
						chain.subtype = roadSubtype;
						for (uint32_t index = first; index < last; ++index)
							chain.points.push_back(PointOf(*p, index));
						if (!chain.points.empty())
							out.networks.push_back(std::move(chain));
					}
					break;
				}

				case CMD_POLYGON:
				{
					const uint16_t parameter = commands.U16();
					const uint32_t count = commands.U8();
					const Pool *p = pool(currentPool);
					beginPolygon(parameter, p ? p->planes : 2);
					std::vector<DsfPoint> winding;
					for (uint32_t i = 0; i < count; ++i)
					{
						const uint32_t index = commands.U16();
						if (p)
							winding.push_back(PointOf(*p, index));
					}
					out.polygons.back().windings.push_back(std::move(winding));
					break;
				}
				case CMD_POLYGON_RANGE:
				{
					const uint16_t parameter = commands.U16();
					const uint32_t first = commands.U16();
					const uint32_t last = commands.U16();
					const Pool *p = pool(currentPool);
					beginPolygon(parameter, p ? p->planes : 2);
					std::vector<DsfPoint> winding;
					if (p)
					{
						for (uint32_t index = first; index < last; ++index)
							winding.push_back(PointOf(*p, index));
					}
					out.polygons.back().windings.push_back(std::move(winding));
					break;
				}
				case CMD_NESTED_POLYGON:
				{
					const uint16_t parameter = commands.U16();
					const uint32_t windings = commands.U8();
					const Pool *p = pool(currentPool);
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
					break;
				}
				case CMD_NESTED_POLYGON_RANGE:
				{
					const uint16_t parameter = commands.U16();
					const uint32_t windings = commands.U8();
					uint32_t first = commands.U16();
					const Pool *p = pool(currentPool);
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
					break;
				}

				// Terrain: counted, not kept
				case CMD_TERRAIN_PATCH:
					patchOpen = true;
					++out.terrainPatchCount;
					break;
				case CMD_TERRAIN_PATCH_FLAGS:
					commands.U8();
					patchOpen = true;
					++out.terrainPatchCount;
					break;
				case CMD_TERRAIN_PATCH_FLAGS_LOD:
					commands.U8();
					commands.F32();
					commands.F32();
					patchOpen = true;
					++out.terrainPatchCount;
					break;
				case CMD_TRIANGLE:
				case CMD_TRIANGLE_STRIP:
				case CMD_TRIANGLE_FAN:
				{
					const uint32_t count = commands.U8();
					commands.Skip(static_cast<size_t>(count) * 2);
					out.terrainTriangleCount += command == CMD_TRIANGLE ? count / 3 : (count >= 3 ? count - 2 : 0);
					break;
				}
				case CMD_TRIANGLE_CROSS_POOL:
				case CMD_TRIANGLE_STRIP_CROSS_POOL:
				case CMD_TRIANGLE_FAN_CROSS_POOL:
				{
					const uint32_t count = commands.U8();
					commands.Skip(static_cast<size_t>(count) * 4);
					out.terrainTriangleCount += command == CMD_TRIANGLE_CROSS_POOL ? count / 3 : (count >= 3 ? count - 2 : 0);
					break;
				}
				case CMD_TRIANGLE_RANGE:
				case CMD_TRIANGLE_STRIP_RANGE:
				case CMD_TRIANGLE_FAN_RANGE:
				{
					const uint32_t first = commands.U16();
					const uint32_t last = commands.U16();
					const uint32_t count = last > first ? last - first : 0;
					out.terrainTriangleCount += command == CMD_TRIANGLE_RANGE ? count / 3 : (count >= 3 ? count - 2 : 0);
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
		}
		(void)patchOpen;

		if (commands.Overrun())
			return Fail(error, "The DSF command stream ends in the middle of a command");
		return true;
	}

	bool LoadDsf(const std::filesystem::path &path, DsfFile &out, std::string *error)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
			return Fail(error, "Could not open " + path.string());
		const std::streamsize size = file.tellg();
		if (size < 0)
			return Fail(error, "Could not read " + path.string());
		std::vector<uint8_t> bytes(static_cast<size_t>(size));
		file.seekg(0);
		if (size > 0 && !file.read(reinterpret_cast<char *>(bytes.data()), size))
			return Fail(error, "Could not read " + path.string());

		std::string readError;
		if (!ReadDsf(bytes.data(), bytes.size(), out, &readError))
			return Fail(error, path.filename().string() + ": " + readError);
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
