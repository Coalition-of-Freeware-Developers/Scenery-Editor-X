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
 * project_edx.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "project_edx.h"
#include "project_archive.h"
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <set>
#include <nlohmann/json.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace
	{
		using Json = nlohmann::ordered_json;

		constexpr size_t MAX_WARNINGS = 200;

		// Top-level sections, in the order they are written
		constexpr const char *KEY_PROJECT = "Project";
		constexpr const char *KEY_DATUM = "Datum";
		constexpr const char *KEY_LIBRARIES = "Libraries";
		constexpr const char *KEY_AIRPORTS = "Airports";
		constexpr const char *KEY_AIRPORT_LEGACY = "Airport";
		constexpr const char *KEY_ASSETS = "Assets";
		constexpr const char *KEY_LIBRARY = "Library";
		constexpr const char *KEY_WED = "WED";
		constexpr const char *KEY_PROPERTIES = "other-properties";

		// -------------------------------------------------------
		// Enum spellings

		template<typename E> struct EnumTable;

		template<> struct EnumTable<ProjectKind>
		{
			static constexpr std::pair<ProjectKind, const char *> values[] = { { ProjectKind::Scenery, "scenery" }, { ProjectKind::Library, "library" } };
		};

		template<> struct EnumTable<ItemKind>
		{
			static constexpr std::pair<ItemKind, const char *> values[] =
			{
				{ ItemKind::Group, "group" }, { ItemKind::Airport, "airport" }, { ItemKind::Object, "object" }, { ItemKind::Polygon, "polygon" },
				{ ItemKind::Orthophoto, "orthophoto" }, { ItemKind::Line, "line" }, { ItemKind::String, "string" }, { ItemKind::Forest, "forest" },
				{ ItemKind::Facade, "facade" }, { ItemKind::Autogen, "autogen" }, { ItemKind::ExclusionZone, "exclusion-zone" },
				{ ItemKind::ExclusionPolygon, "exclusion-polygon" }, { ItemKind::Raw, "wed-raw" }, { ItemKind::RoadNode, "road-node" },
				{ ItemKind::Road, "road" }
			};
		};

		template<> struct EnumTable<ElevationMode>
		{
			static constexpr std::pair<ElevationMode, const char *> values[] = { { ElevationMode::Ground, "ground" }, { ElevationMode::Msl, "msl" }, { ElevationMode::Agl, "agl" } };
		};

		template<> struct EnumTable<ForestFill>
		{
			static constexpr std::pair<ForestFill, const char *> values[] = { { ForestFill::Area, "area" }, { ForestFill::Line, "line" }, { ForestFill::Points, "points" } };
		};

		template<> struct EnumTable<XPlane::AirportType>
		{
			static constexpr std::pair<XPlane::AirportType, const char *> values[] =
			{
				{ XPlane::AirportType::Land, "airport" }, { XPlane::AirportType::Seaplane, "seaport" }, { XPlane::AirportType::Heliport, "heliport" }
			};
		};

		template<> struct EnumTable<XPlane::AptFrequency::Type>
		{
			using T = XPlane::AptFrequency::Type;
			static constexpr std::pair<T, const char *> values[] =
			{
				{ T::Recorded, "recorded" }, { T::Unicom, "unicom" }, { T::Clearance, "clearance" }, { T::Ground, "ground" },
				{ T::Tower, "tower" }, { T::Approach, "approach" }, { T::Departure, "departure" }
			};
		};

		template<> struct EnumTable<XPlane::ExportKind>
		{
			using T = XPlane::ExportKind;
			static constexpr std::pair<T, const char *> values[] = { { T::Export, "export" }, { T::Extend, "extend" }, { T::Exclude, "exclude" }, { T::Backup, "backup" } };
		};

		bool EqualsIgnoreCase(const std::string_view a, const std::string_view b)
		{
			if (a.size() != b.size())
				return false;
			for (size_t i = 0; i < a.size(); ++i)
			{
				const char x = a[i] >= 'A' && a[i] <= 'Z' ? static_cast<char>(a[i] - 'A' + 'a') : a[i];
				const char y = b[i] >= 'A' && b[i] <= 'Z' ? static_cast<char>(b[i] - 'A' + 'a') : b[i];
				if (x != y)
					return false;
			}
			return true;
		}

		// -------------------------------------------------------
		// Text helpers

		/* @brief Valid UTF-8 for JSON: bytes that aren't UTF-8 are taken as Latin-1 (old apt.dat files). */
		std::string ToValidUtf8(const std::string_view text, bool &repaired)
		{
			std::string out;
			out.reserve(text.size());
			size_t i = 0;
			while (i < text.size())
			{
				const auto c = static_cast<uint8_t>(text[i]);
				size_t length = 0;
				if (c < 0x80)
					length = 1;
				else if ((c & 0xE0) == 0xC0 && c >= 0xC2)
					length = 2;
				else if ((c & 0xF0) == 0xE0)
					length = 3;
				else if ((c & 0xF8) == 0xF0 && c <= 0xF4)
					length = 4;

				bool valid = length > 0 && i + length <= text.size();
				for (size_t k = 1; valid && k < length; ++k)
					valid = (static_cast<uint8_t>(text[i + k]) & 0xC0) == 0x80;

				if (valid)
				{
					out.append(text.substr(i, length));
					i += length;
				}
				else
				{
					out.push_back(static_cast<char>(0xC0 | (c >> 6)));
					out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
					repaired = true;
					++i;
				}
			}
			return out;
		}

		/* @brief Shortest text that reads back as the same float, as a JSON number (0.1f writes 0.1, not 0.10000000149). */
		Json FloatToJson(const float value)
		{
			if (!std::isfinite(value))
				return nullptr;
			char text[32];
			const auto result = std::to_chars(text, text + sizeof(text), value);
			*result.ptr = '\0';
			return std::strtod(text, nullptr);
		}

		uint8_t ParseSeasonList(const std::string_view text, bool &ok)
		{
			uint8_t seasons = 0;
			size_t start = 0;
			while (start <= text.size())
			{
				const size_t comma = text.find(',', start);
				const std::string_view word = text.substr(start, comma == std::string_view::npos ? std::string_view::npos : comma - start);
				if (word == "spr")
					seasons |= XPlane::SeasonSpring;
				else if (word == "sum")
					seasons |= XPlane::SeasonSummer;
				else if (word == "fal")
					seasons |= XPlane::SeasonFall;
				else if (word == "win")
					seasons |= XPlane::SeasonWinter;
				else if (!word.empty())
					ok = false;
				if (comma == std::string_view::npos)
					break;
				start = comma + 1;
			}
			return seasons;
		}

		std::string SeasonListText(const uint8_t seasons)
		{
			std::string text;
			const std::pair<uint8_t, const char *> names[] = { { XPlane::SeasonSpring, "spr" }, { XPlane::SeasonSummer, "sum" }, { XPlane::SeasonFall, "fal" }, { XPlane::SeasonWinter, "win" } };
			for (const auto &[bit, name] : names)
			{
				if (seasons & bit)
				{
					if (!text.empty())
						text.push_back(',');
					text.append(name);
				}
			}
			return text;
		}

		// -------------------------------------------------------
		// Writing

		struct WriteContext
		{
			bool repairedText = false;
		};

		template<typename T> Json ToJson(const T &value, WriteContext &context);

		/* @brief [lat, lon] for a plain node; an object only when it has handles, texture coordinates or attributes. */
		Json NodeToJson(const GeoNode &node)
		{
			const GeoNode plain{ node.latitude, node.longitude };
			if (node == plain)
				return Json::array({ node.latitude, node.longitude });

			Json object = Json::object();
			object["latitude"] = node.latitude;
			object["longitude"] = node.longitude;
			if (node.HasLo())
				object["lo"] = Json::array({ node.loLatitude, node.loLongitude });
			if (node.HasHi())
				object["hi"] = Json::array({ node.hiLatitude, node.hiLongitude });
			if (node.split)
				object["split"] = true;
			if (node.hasUv)
			{
				object["uv"] = Json::array({ FloatToJson(node.s), FloatToJson(node.t) });
				if (node.sLo != 0.0f || node.tLo != 0.0f)
					object["uv-lo"] = Json::array({ FloatToJson(node.sLo), FloatToJson(node.tLo) });
				if (node.sHi != 0.0f || node.tHi != 0.0f)
					object["uv-hi"] = Json::array({ FloatToJson(node.sHi), FloatToJson(node.tHi) });
			}
			if (node.wallType != 0)
				object["wall"] = node.wallType;
			if (!node.spawnTiles)
				object["spawn-tiles"] = false;
			return object;
		}

		Json NodeToJson(const XPlane::AptNode &node)
		{
			if (!node.hasControl && node.lineType == 0 && node.lightType == 0)
				return Json::array({ node.latitude, node.longitude });

			Json object = Json::object();
			object["latitude"] = node.latitude;
			object["longitude"] = node.longitude;
			if (node.hasControl)
				object["control"] = Json::array({ node.controlLatitude, node.controlLongitude });
			if (node.lineType != 0)
				object["line"] = node.lineType;
			if (node.lightType != 0)
				object["light"] = node.lightType;
			return object;
		}

		/**
		 * @class JsonWriter
		 * @brief Archive (project_archive.h) filling a JSON object. Empty lists and absent optionals are left out.
		 */
		class JsonWriter
		{
		public:
			JsonWriter(Json &object, WriteContext &context) : m_Object(object), m_Context(context) {}

			template<typename T>
			void Field(const char *key, const T &value)
			{
				if constexpr (Detail::IsVector<T>::value)
				{
					if (value.empty())
						return;
				}
				if constexpr (Detail::IsOptional<T>::value)
				{
					if (!value)
						return;
				}
				m_Object[key] = ToJson(value, m_Context);
			}

			template<typename T>
			void FieldIf(const char *key, const T &value, const T &fallback)
			{
				if (!(value == fallback))
					Field(key, value);
			}

			template<typename T>
			void Inline(const char *, const T &value)
			{
				JsonWriter inner(m_Object, m_Context);
				Visit(inner, const_cast<T &>(value));
			}

			void Chain(const char *key, const std::vector<GeoRing> &rings)
			{
				m_Object[key] = rings.empty() ? Json::array() : ToJson(rings.front(), m_Context);
			}

			void Exclusions(const char *key, const uint32_t &bits)
			{
				Json names = Json::array();
				uint32_t known = 0;
				for (const auto &[bit, name] : GetExclusionNames())
				{
					known |= bit;
					if (bits & bit)
						names.push_back(name);
				}
				if (bits & ~known)
					names.push_back(bits & ~known);	// Bits a newer editor defined: kept as a number
				m_Object[key] = std::move(names);
			}

			void Seasons(const char *key, const uint8_t &seasons)
			{
				if (seasons != 0)
					m_Object[key] = SeasonListText(seasons);
			}

			void Metadata(const char *key, const std::vector<XPlane::AptMetadata> &metadata)
			{
				if (metadata.empty())
					return;

				// An object reads best; a file that repeats a key (allowed in apt.dat) falls back to [key, value] pairs.
				std::set<std::string> keys;
				const bool unique = std::ranges::all_of(metadata, [&keys](const XPlane::AptMetadata &entry) { return keys.insert(entry.key).second; });
				Json out = unique ? Json::object() : Json::array();
				for (const XPlane::AptMetadata &entry : metadata)
				{
					if (unique)
						out[ToValidUtf8(entry.key, m_Context.repairedText)] = ToValidUtf8(entry.value, m_Context.repairedText);
					else
						out.push_back(Json::array({ ToValidUtf8(entry.key, m_Context.repairedText), ToValidUtf8(entry.value, m_Context.repairedText) }));
				}
				m_Object[key] = std::move(out);
			}

			void Properties(const char *key, const PropertyList &properties)
			{
				if (properties.empty())
					return;
				Json out = Json::object();
				for (const auto &[name, text] : properties)
				{
					Json value = Json::parse(text, nullptr, false);
					if (value.is_discarded())
						value = ToValidUtf8(text, m_Context.repairedText);	// Not JSON (set by code): store it as a string
					out[ToValidUtf8(name, m_Context.repairedText)] = std::move(value);
				}
				m_Object[key] = std::move(out);
			}

		private:
			Json &m_Object;
			WriteContext &m_Context;
		};

		template<typename T>
		Json ToJson(const T &value, WriteContext &context)
		{
			if constexpr (std::is_enum_v<T>)
			{
				for (const auto &[entry, name] : EnumTable<T>::values)
				{
					if (entry == value)
						return name;
				}
				return static_cast<int>(value);
			}
			else if constexpr (std::is_same_v<T, bool>)
				return value;
			else if constexpr (std::is_same_v<T, float>)
				return FloatToJson(value);
			else if constexpr (std::is_floating_point_v<T>)
				return std::isfinite(value) ? Json(value) : Json(nullptr);
			else if constexpr (std::is_integral_v<T>)
				return value;
			else if constexpr (std::is_same_v<T, std::string>)
				return ToValidUtf8(value, context.repairedText);
			else if constexpr (std::is_same_v<T, GeoNode> || std::is_same_v<T, XPlane::AptNode>)
				return NodeToJson(value);
			else if constexpr (Detail::IsVector<T>::value || Detail::IsStdArray<T>::value || std::is_array_v<T>)
			{
				Json array = Json::array();
				for (const auto &item : value)
					array.push_back(ToJson(item, context));
				return array;
			}
			else if constexpr (Detail::IsOptional<T>::value)
				return value ? ToJson(*value, context) : Json(nullptr);
			else
			{
				Json object = Json::object();
				JsonWriter writer(object, context);
				Visit(writer, const_cast<T &>(value));
				return object;
			}
		}

		// -------------------------------------------------------
		// Layout: deterministic, one record per line

		int NestingDepth(const Json &value)
		{
			if (!value.is_structured())
				return 0;
			int depth = 0;
			for (const Json &child : value)
				depth = std::max(depth, NestingDepth(child));
			return depth + 1;
		}

		void AppendScalar(const Json &value, std::string &out)
		{
			out.append(value.dump(-1, ' ', false, Json::error_handler_t::replace));
		}

		void EmitCompact(const Json &value, std::string &out)
		{
			if (value.is_object())
			{
				out.push_back('{');
				bool first = true;
				for (auto it = value.begin(); it != value.end(); ++it)
				{
					if (!first)
						out.append(", ");
					first = false;
					AppendScalar(Json(it.key()), out);
					out.append(": ");
					EmitCompact(it.value(), out);
				}
				out.push_back('}');
			}
			else if (value.is_array())
			{
				out.push_back('[');
				bool first = true;
				for (const Json &child : value)
				{
					if (!first)
						out.append(", ");
					first = false;
					EmitCompact(child, out);
				}
				out.push_back(']');
			}
			else
				AppendScalar(value, out);
		}

		/**
		 * @brief Writes a value: below the top-level sections, records of scalars (and short records with one level of
		 * nesting, like an object with its properties or a node with its handles) stay on one line; everything else is
		 * one entry per line, indented with tabs.
		 */
		void Emit(const Json &value, std::string &out, const int indent)
		{
			if (!value.is_structured() || value.empty())
			{
				EmitCompact(value, out);
				return;
			}

			const int depth = NestingDepth(value);
			if (indent >= 2 && depth <= 2)
			{
				std::string compact;
				EmitCompact(value, compact);
				if ((depth == 1 && compact.size() <= 400) || compact.size() <= 300)
				{
					out.append(compact);
					return;
				}
			}

			const bool object = value.is_object();
			out.push_back(object ? '{' : '[');
			out.push_back('\n');
			size_t index = 0;
			for (auto it = value.begin(); it != value.end(); ++it, ++index)
			{
				out.append(static_cast<size_t>(indent + 1), '\t');
				if (object)
				{
					AppendScalar(Json(it.key()), out);
					out.append(": ");
				}
				Emit(it.value(), out, indent + 1);
				if (index + 1 < value.size())
					out.push_back(',');
				out.push_back('\n');
			}
			out.append(static_cast<size_t>(indent), '\t');
			out.push_back(object ? '}' : ']');
		}

		// -------------------------------------------------------
		// Reading

		struct ReadContext
		{
			std::vector<std::string> *warnings = nullptr;
			size_t dropped = 0;

			void Warn(const std::string &path, const std::string &message)
			{
				if (!warnings)
					return;
				if (warnings->size() < MAX_WARNINGS)
					warnings->push_back(path.empty() ? message : path + ": " + message);
				else
					++dropped;
			}
		};

		const char *TypeName(const Json &value)
		{
			return value.type_name();
		}

		template<typename T> void FromJson(const Json &json, T &value, ReadContext &context, const std::string &path);

		void ReportUnknownKeys(const Json &object, const std::set<std::string> &used, ReadContext &context, const std::string &path)
		{
			for (auto it = object.begin(); it != object.end(); ++it)
			{
				if (!used.contains(it.key()))
					context.Warn(path, "unknown key '" + it.key() + "' ignored (it won't be saved)");
			}
		}

		bool ReadNumberPair(const Json &json, double &a, double &b)
		{
			if (!json.is_array() || json.size() < 2 || !json[0].is_number() || !json[1].is_number())
				return false;
			a = json[0].get<double>();
			b = json[1].get<double>();
			return true;
		}

		bool ReadFloatPair(const Json &json, float &a, float &b)
		{
			double x = 0.0, y = 0.0;
			if (!ReadNumberPair(json, x, y))
				return false;
			a = static_cast<float>(x);
			b = static_cast<float>(y);
			return true;
		}

		void NodeFromJson(const Json &json, GeoNode &node, ReadContext &context, const std::string &path)
		{
			node = GeoNode{};
			if (json.is_array())
			{
				if (!ReadNumberPair(json, node.latitude, node.longitude))
					context.Warn(path, "a node needs [latitude, longitude]");
				return;
			}
			if (!json.is_object())
			{
				context.Warn(path, std::string("expected a node, found ") + TypeName(json));
				return;
			}

			std::set<std::string> used;
			const auto number = [&](const char *key, double &out)
			{
				if (const auto it = json.find(key); it != json.end())
				{
					used.insert(key);
					if (it->is_number())
						out = it->get<double>();
					else
						context.Warn(path + "." + key, "expected a number");
				}
			};
			const auto pair = [&](const char *key, auto reader)
			{
				if (const auto it = json.find(key); it != json.end())
				{
					used.insert(key);
					if (!reader(*it))
						context.Warn(path + "." + key, "expected [a, b]");
				}
			};

			number("latitude", node.latitude);
			number("longitude", node.longitude);
			pair("lo", [&](const Json &v) { return ReadNumberPair(v, node.loLatitude, node.loLongitude); });
			pair("hi", [&](const Json &v) { return ReadNumberPair(v, node.hiLatitude, node.hiLongitude); });
			pair("uv", [&](const Json &v) { node.hasUv = true; return ReadFloatPair(v, node.s, node.t); });
			pair("uv-lo", [&](const Json &v) { return ReadFloatPair(v, node.sLo, node.tLo); });
			pair("uv-hi", [&](const Json &v) { return ReadFloatPair(v, node.sHi, node.tHi); });
			if (const auto it = json.find("split"); it != json.end())
			{
				used.insert("split");
				FromJson(*it, node.split, context, path + ".split");
			}
			if (const auto it = json.find("wall"); it != json.end())
			{
				used.insert("wall");
				FromJson(*it, node.wallType, context, path + ".wall");
			}
			if (const auto it = json.find("spawn-tiles"); it != json.end())
			{
				used.insert("spawn-tiles");
				FromJson(*it, node.spawnTiles, context, path + ".spawn-tiles");
			}
			ReportUnknownKeys(json, used, context, path);
		}

		void NodeFromJson(const Json &json, XPlane::AptNode &node, ReadContext &context, const std::string &path)
		{
			node = XPlane::AptNode{};
			if (json.is_array())
			{
				if (!ReadNumberPair(json, node.latitude, node.longitude))
					context.Warn(path, "a node needs [latitude, longitude]");
				return;
			}
			if (!json.is_object())
			{
				context.Warn(path, std::string("expected a node, found ") + TypeName(json));
				return;
			}

			std::set<std::string> used;
			for (auto it = json.begin(); it != json.end(); ++it)
			{
				const std::string &key = it.key();
				used.insert(key);
				if (key == "latitude")
					FromJson(*it, node.latitude, context, path + ".latitude");
				else if (key == "longitude")
					FromJson(*it, node.longitude, context, path + ".longitude");
				else if (key == "control")
				{
					node.hasControl = ReadNumberPair(*it, node.controlLatitude, node.controlLongitude);
					if (!node.hasControl)
						context.Warn(path + ".control", "expected [latitude, longitude]");
				}
				else if (key == "line")
					FromJson(*it, node.lineType, context, path + ".line");
				else if (key == "light")
					FromJson(*it, node.lightType, context, path + ".light");
				else
					used.erase(key);
			}
			ReportUnknownKeys(json, used, context, path);
		}

		/**
		 * @class JsonReader
		 * @brief Archive reading a JSON object: absent keys keep their defaults, mistyped values warn and keep them too.
		 */
		class JsonReader
		{
		public:
			JsonReader(const Json &object, ReadContext &context, std::string path, std::set<std::string> &used)
				: m_Object(object), m_Context(context), m_Path(std::move(path)), m_Used(used) {}

			template<typename T>
			void Field(const char *key, T &value)
			{
				if (const Json *json = Take(key))
					FromJson(*json, value, m_Context, Child(key));
			}

			template<typename T>
			void FieldIf(const char *key, T &value, const T &) { Field(key, value); }

			template<typename T>
			void Inline(const char *, T &value)
			{
				JsonReader inner(m_Object, m_Context, m_Path, m_Used);
				Visit(inner, value);
			}

			void Chain(const char *key, std::vector<GeoRing> &rings)
			{
				if (const Json *json = Take(key))
				{
					rings.assign(1, GeoRing{});
					FromJson(*json, rings.front(), m_Context, Child(key));
				}
			}

			void Exclusions(const char *key, uint32_t &bits)
			{
				const Json *json = Take(key);
				if (!json)
					return;
				bits = 0;
				if (json->is_number_unsigned())
				{
					bits = json->get<uint32_t>();
					return;
				}
				if (!json->is_array())
				{
					m_Context.Warn(Child(key), "expected a list of exclusions");
					return;
				}
				for (const Json &entry : *json)
				{
					if (entry.is_number_unsigned())
					{
						bits |= entry.get<uint32_t>();
						continue;
					}
					const std::string name = entry.is_string() ? entry.get<std::string>() : std::string();
					const auto &names = GetExclusionNames();
					const auto it = std::ranges::find_if(names, [&name](const auto &pair) { return EqualsIgnoreCase(pair.second, name); });
					if (it != names.end())
						bits |= it->first;
					else
						m_Context.Warn(Child(key), "unknown exclusion '" + name + "' (use objects, facades, forests, beaches, roads, lines, polygons, strings)");
				}
			}

			void Seasons(const char *key, uint8_t &seasons)
			{
				const Json *json = Take(key);
				if (!json)
					return;
				if (json->is_string())
				{
					bool ok = true;
					seasons = ParseSeasonList(json->get<std::string>(), ok);
					if (!ok)
						m_Context.Warn(Child(key), "unknown season (use spr, sum, fal, win)");
				}
				else if (json->is_number_unsigned())
					seasons = static_cast<uint8_t>(json->get<uint32_t>() & XPlane::SeasonAll);
				else
					m_Context.Warn(Child(key), "expected \"spr,sum,...\"");
			}

			void Metadata(const char *key, std::vector<XPlane::AptMetadata> &metadata)
			{
				const Json *json = Take(key);
				if (!json)
					return;
				metadata.clear();
				if (json->is_object())
				{
					for (auto it = json->begin(); it != json->end(); ++it)
						metadata.push_back({ it.key(), ScalarText(it.value()) });
				}
				else if (json->is_array())
				{
					for (const Json &pair : *json)
					{
						if (pair.is_array() && pair.size() == 2)
							metadata.push_back({ ScalarText(pair[0]), ScalarText(pair[1]) });
						else
							m_Context.Warn(Child(key), "expected [key, value]");
					}
				}
				else
					m_Context.Warn(Child(key), "expected an object of key / value pairs");
			}

			void Properties(const char *key, PropertyList &properties)
			{
				const Json *json = Take(key);
				if (!json)
					return;
				properties.clear();
				if (!json->is_object())
				{
					m_Context.Warn(Child(key), "expected an object");
					return;
				}
				for (auto it = json->begin(); it != json->end(); ++it)
					properties.emplace_back(it.key(), it.value().dump(-1, ' ', false, Json::error_handler_t::replace));
			}

		private:
			const Json *Take(const char *key)
			{
				const auto it = m_Object.find(key);
				if (it == m_Object.end())
					return nullptr;
				m_Used.insert(key);
				return it->is_null() ? nullptr : &*it;
			}

			[[nodiscard]] std::string Child(const char *key) const { return m_Path + "." + key; }

			static std::string ScalarText(const Json &value)
			{
				return value.is_string() ? value.get<std::string>() : value.is_null() ? std::string() : value.dump();
			}

			const Json &m_Object;
			ReadContext &m_Context;
			std::string m_Path;
			std::set<std::string> &m_Used;
		};

		template<typename T>
		void ReadInteger(const Json &json, T &value, ReadContext &context, const std::string &path)
		{
			if (json.is_number_integer() || json.is_number_unsigned())
			{
				if (json.is_number_unsigned())
				{
					const auto raw = json.get<uint64_t>();
					if (raw <= static_cast<uint64_t>((std::numeric_limits<T>::max)()))
					{
						value = static_cast<T>(raw);
						return;
					}
				}
				else
				{
					const auto raw = json.get<int64_t>();
					if (raw >= static_cast<int64_t>((std::numeric_limits<T>::min)()) && (raw < 0 || static_cast<uint64_t>(raw) <= static_cast<uint64_t>((std::numeric_limits<T>::max)())))
					{
						value = static_cast<T>(raw);
						return;
					}
				}
				context.Warn(path, "number out of range");
				return;
			}
			if (json.is_number_float())
			{
				const double raw = json.get<double>();
				if (std::isfinite(raw) && raw >= static_cast<double>((std::numeric_limits<T>::min)()) && raw <= static_cast<double>((std::numeric_limits<T>::max)()))
				{
					value = static_cast<T>(std::llround(raw));
					if (raw != std::floor(raw))
						context.Warn(path, "expected a whole number, rounded");
					return;
				}
				context.Warn(path, "number out of range");
				return;
			}
			if (json.is_string())
			{
				const std::string text = json.get<std::string>();
				int64_t raw = 0;
				const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), raw);
				if (ec == std::errc() && ptr == text.data() + text.size())
				{
					ReadInteger(Json(raw), value, context, path);
					return;
				}
			}
			if (json.is_boolean())
			{
				value = static_cast<T>(json.get<bool>() ? 1 : 0);
				return;
			}
			context.Warn(path, std::string("expected a whole number, found ") + TypeName(json));
		}

		template<typename T>
		void FromJson(const Json &json, T &value, ReadContext &context, const std::string &path)
		{
			if constexpr (std::is_enum_v<T>)
			{
				if (json.is_string())
				{
					const std::string name = json.get<std::string>();
					for (const auto &[entry, entryName] : EnumTable<T>::values)
					{
						if (EqualsIgnoreCase(entryName, name))
						{
							value = entry;
							return;
						}
					}
					std::string choices;
					for (const auto &[entry, entryName] : EnumTable<T>::values)
						choices += (choices.empty() ? "" : ", ") + std::string(entryName);
					context.Warn(path, "unknown value '" + name + "' (expected " + choices + ")");
				}
				else if (json.is_number_integer() || json.is_number_unsigned())
				{
					const int raw = json.get<int>();
					for (const auto &[entry, entryName] : EnumTable<T>::values)
					{
						if (static_cast<int>(entry) == raw)
						{
							value = entry;
							return;
						}
					}
					context.Warn(path, "unknown value " + std::to_string(raw));
				}
				else
					context.Warn(path, std::string("expected a name, found ") + TypeName(json));
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				if (json.is_boolean())
					value = json.get<bool>();
				else if (json.is_number())
					value = json.get<double>() != 0.0;
				else if (json.is_string() && (json == "true" || json == "false"))
					value = json == "true";
				else
					context.Warn(path, std::string("expected true or false, found ") + TypeName(json));
			}
			else if constexpr (std::is_floating_point_v<T>)
			{
				if (json.is_number())
					value = static_cast<T>(json.get<double>());
				else if (json.is_string())
				{
					const std::string text = json.get<std::string>();
					char *end = nullptr;
					const double raw = std::strtod(text.c_str(), &end);
					if (end && *end == '\0' && !text.empty())
						value = static_cast<T>(raw);
					else
						context.Warn(path, "expected a number, found \"" + text + "\"");
				}
				else
					context.Warn(path, std::string("expected a number, found ") + TypeName(json));
			}
			else if constexpr (std::is_integral_v<T>)
				ReadInteger(json, value, context, path);
			else if constexpr (std::is_same_v<T, std::string>)
			{
				if (json.is_string())
					value = json.get<std::string>();
				else if (json.is_number() || json.is_boolean())
					value = json.dump();
				else
					context.Warn(path, std::string("expected text, found ") + TypeName(json));
			}
			else if constexpr (std::is_same_v<T, GeoNode> || std::is_same_v<T, XPlane::AptNode>)
				NodeFromJson(json, value, context, path);
			else if constexpr (Detail::IsVector<T>::value)
			{
				value.clear();
				if (!json.is_array())
				{
					context.Warn(path, std::string("expected a list, found ") + TypeName(json));
					return;
				}
				value.resize(json.size());
				for (size_t i = 0; i < json.size(); ++i)
					FromJson(json[i], value[i], context, path + "[" + std::to_string(i) + "]");
			}
			else if constexpr (Detail::IsStdArray<T>::value || std::is_array_v<T>)
			{
				if (!json.is_array())
				{
					context.Warn(path, std::string("expected a list, found ") + TypeName(json));
					return;
				}
				const size_t count = std::size(value);
				if (json.size() != count)
					context.Warn(path, "expected " + std::to_string(count) + " entries, found " + std::to_string(json.size()));
				for (size_t i = 0; i < std::min(count, json.size()); ++i)
					FromJson(json[i], value[i], context, path + "[" + std::to_string(i) + "]");
			}
			else if constexpr (Detail::IsOptional<T>::value)
			{
				value.emplace();
				FromJson(json, *value, context, path);
			}
			else
			{
				if (!json.is_object())
				{
					context.Warn(path, std::string("expected an object, found ") + TypeName(json));
					return;
				}
				std::set<std::string> used;
				JsonReader reader(json, context, path, used);
				Visit(reader, value);
				ReportUnknownKeys(json, used, context, path);
			}
		}

		// -------------------------------------------------------
		// The early documented layout ("Airport" with ICAO / City / DatumLat... keys)

		struct LegacyAirportKey
		{
			const char *key;
			const char *metadata;		// 1302 key it becomes, null when handled separately
		};

		constexpr LegacyAirportKey LEGACY_AIRPORT_KEYS[] =
		{
			{ "ICAO", "icao_code" }, { "IATA", "iata_code" }, { "FAA", "faa_code" }, { "City", "city" }, { "State", "state" },
			{ "Country", "country" }, { "RegionCode", "region_code" }, { "DatumLat", "datum_lat" }, { "DatumLon", "datum_lon" },
			{ "TransitionAlt", "transition_alt" }, { "TransitionLevel", "transition_level" }
		};

		/**
		 * @brief Moves the documented friendly keys of an airport object into apt.dat terms: 1302 metadata, the
		 * field elevation, MagVar into the airport's properties, Libraries into the project.
		 */
		void ReadLegacyAirportKeys(Json &object, ProjectAirport &airport, SceneryProject &project, ReadContext &context, const std::string &path)
		{
			for (const LegacyAirportKey &entry : LEGACY_AIRPORT_KEYS)
			{
				const auto it = object.find(entry.key);
				if (it == object.end())
					continue;
				const std::string value = it->is_string() ? it->get<std::string>() : it->dump();
				airport.airport.SetMetadata(entry.metadata, value);
				object.erase(it);
			}
			if (const auto it = object.find("Elevation"); it != object.end())
			{
				int elevation = 0;
				FromJson(*it, elevation, context, path + ".Elevation");
				airport.airport.elevationFt = elevation;
				object.erase(it);
			}
			if (const auto it = object.find("MagVar"); it != object.end())
			{
				airport.properties.emplace_back("MagVar", it->dump());
				object.erase(it);
			}
			if (const auto it = object.find(KEY_LIBRARIES); it != object.end())
			{
				std::vector<LibraryReference> libraries;
				FromJson(*it, libraries, context, path + "." + KEY_LIBRARIES);
				project.libraries.insert(project.libraries.end(), libraries.begin(), libraries.end());
				object.erase(it);
			}
		}

		bool ReadAirport(Json object, ProjectAirport &airport, SceneryProject &project, ReadContext &context, const std::string &path)
		{
			if (!object.is_object())
			{
				context.Warn(path, std::string("expected an airport object, found ") + TypeName(object));
				return false;
			}
			ReadLegacyAirportKeys(object, airport, project, context, path);
			const XPlane::AptAirport legacy = airport.airport;
			FromJson(object, airport, context, path);
			// Keys the friendly spelling set survive a structured read that didn't name them.
			for (const XPlane::AptMetadata &entry : legacy.metadata)
			{
				if (!airport.airport.HasMetadata(entry.key))
					airport.airport.SetMetadata(entry.key, entry.value);
			}
			if (!object.contains("elevation-ft"))
				airport.airport.elevationFt = legacy.elevationFt;
			if (airport.airport.id.empty())
			{
				const std::string icao = airport.airport.GetIcao();
				airport.airport.id = !icao.empty() ? icao : std::string(airport.airport.GetMetadata("faa_code"));
			}
			if (airport.airport.name.empty())
				airport.airport.name = airport.airport.GetMetadata("city").empty() ? airport.airport.id : std::string(airport.airport.GetMetadata("city"));
			return true;
		}
	}

	// -------------------------------------------------------

	std::string WriteEdx(const SceneryProject &project)
	{
		WriteContext context;
		Json root = Json::object();

		ProjectInfo info = project.info;
		info.formatVersion = FORMAT_VERSION;
		root[KEY_PROJECT] = ToJson(info, context);

		if (project.datum.valid)
			root[KEY_DATUM] = ToJson(project.datum, context);
		if (!project.libraries.empty())
			root[KEY_LIBRARIES] = ToJson(project.libraries, context);
		if (!project.airports.empty())
			root[KEY_AIRPORTS] = ToJson(project.airports, context);
		root[KEY_ASSETS] = ToJson(project.items, context);
		if (project.library)
			root[KEY_LIBRARY] = ToJson(*project.library, context);
		if (!project.wedPreferences.empty())
		{
			Json prefs = Json::object();
			for (const auto &[name, value] : project.wedPreferences)
				prefs[ToValidUtf8(name, context.repairedText)] = ToValidUtf8(value, context.repairedText);
			root[KEY_WED] = Json::object({ { "prefs", std::move(prefs) } });
		}
		if (!project.properties.empty())
		{
			JsonWriter writer(root, context);
			writer.Properties(KEY_PROPERTIES, project.properties);
		}

		std::string text;
		Emit(root, text, 0);
		text.push_back('\n');
		return text;
	}

	bool ParseEdx(const std::string_view text, SceneryProject &out, std::vector<std::string> *warnings, std::string *error)
	{
		out = SceneryProject{};
		ReadContext context;
		context.warnings = warnings;

		Json root;
		try
		{
			// Comments are allowed so hand-maintained projects can carry notes.
			root = Json::parse(text.begin(), text.end(), nullptr, true, true);
		}
		catch (const Json::exception &e)
		{
			// Not only parse_error: a number too large for a double ("1e400") throws out_of_range
			if (error)
				*error = std::string("Not valid JSON: ") + e.what();
			return false;
		}

		if (!root.is_object() || !root.contains(KEY_PROJECT) || !root[KEY_PROJECT].is_object())
		{
			if (error)
				*error = "Not a Scenery Editor X project (no \"Project\" object)";
			return false;
		}

		std::set<std::string> used = { KEY_PROJECT };
		FromJson(root[KEY_PROJECT], out.info, context, KEY_PROJECT);
		if (out.info.formatVersion > FORMAT_VERSION)
			context.Warn(KEY_PROJECT, "written by a newer editor (format " + std::to_string(out.info.formatVersion) + "); data it added may not be kept");
		out.info.formatVersion = FORMAT_VERSION;

		if (const auto it = root.find(KEY_DATUM); it != root.end())
		{
			used.insert(KEY_DATUM);
			out.datum.valid = true;
			FromJson(*it, out.datum, context, KEY_DATUM);
		}
		if (const auto it = root.find(KEY_LIBRARIES); it != root.end())
		{
			used.insert(KEY_LIBRARIES);
			FromJson(*it, out.libraries, context, KEY_LIBRARIES);
		}
		if (const auto it = root.find(KEY_AIRPORT_LEGACY); it != root.end())
		{
			used.insert(KEY_AIRPORT_LEGACY);
			ProjectAirport airport;
			if (ReadAirport(*it, airport, out, context, KEY_AIRPORT_LEGACY))
				out.airports.push_back(std::move(airport));
		}
		if (const auto it = root.find(KEY_AIRPORTS); it != root.end())
		{
			used.insert(KEY_AIRPORTS);
			if (it->is_array())
			{
				for (size_t i = 0; i < it->size(); ++i)
				{
					ProjectAirport airport;
					if (ReadAirport((*it)[i], airport, out, context, std::string(KEY_AIRPORTS) + "[" + std::to_string(i) + "]"))
						out.airports.push_back(std::move(airport));
				}
			}
			else
				context.Warn(KEY_AIRPORTS, "expected a list of airports");
		}
		if (const auto it = root.find(KEY_ASSETS); it != root.end())
		{
			used.insert(KEY_ASSETS);
			FromJson(*it, out.items, context, KEY_ASSETS);
		}
		if (const auto it = root.find(KEY_LIBRARY); it != root.end())
		{
			used.insert(KEY_LIBRARY);
			out.library.emplace();
			FromJson(*it, *out.library, context, KEY_LIBRARY);
		}
		if (const auto it = root.find(KEY_WED); it != root.end())
		{
			used.insert(KEY_WED);
			const auto prefs = it->find("prefs");
			if (it->is_object() && prefs != it->end() && prefs->is_object())
			{
				for (auto pref = prefs->begin(); pref != prefs->end(); ++pref)
					out.wedPreferences.emplace_back(pref.key(), pref->is_string() ? pref->get<std::string>() : pref->dump());
			}
		}
		{
			JsonReader reader(root, context, std::string(), used);
			reader.Properties(KEY_PROPERTIES, out.properties);
		}
		ReportUnknownKeys(root, used, context, std::string());

		for (const std::string &change : NormalizeProject(out))
			context.Warn(std::string(), change);
		if (context.dropped > 0 && warnings)
			warnings->push_back(std::to_string(context.dropped) + " more warnings not shown");
		return true;
	}
}

// -------------------------------------------------------
