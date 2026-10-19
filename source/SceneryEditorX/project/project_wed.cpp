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
 * project_wed.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "project_wed.h"
#include "xml_document.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace
	{
		using namespace XPlane;

		constexpr double FEET_PER_METER = 1.0 / 0.3048;
		constexpr size_t MAX_WARNINGS = 200;

		// -------------------------------------------------------
		// WED enum display names (xptools WEDCore/WED_Enums.h) and the apt.dat codes they export to

		struct NamedCode
		{
			int code;
			const char *name;
		};

		constexpr NamedCode SURFACES[] =
		{
			{ 20, "Asphalt L" }, { 21, "Asphalt L Patched" }, { 22, "Asphalt L Plain" }, { 23, "Asphalt L Worn" },
			{ 1, "Asphalt" }, { 24, "Asphalt Patched" }, { 25, "Asphalt Plain" }, { 26, "Asphalt Worn" },
			{ 27, "Asphalt D" }, { 28, "Asphalt D Patched" }, { 29, "Asphalt D Plain" }, { 30, "Asphalt D Worn" },
			{ 31, "Asphalt D2" }, { 32, "Asphalt D2 Patched" }, { 33, "Asphalt D2 Plain" }, { 34, "Asphalt D2 Worn" },
			{ 35, "Asphalt D3" }, { 36, "Asphalt D3 Patched" }, { 37, "Asphalt D3 Plain" }, { 38, "Asphalt D3 Worn" },
			{ 50, "Concrete L" }, { 51, "Concrete L Dirty" }, { 52, "Concrete L Worn" },
			{ 2, "Concrete" }, { 53, "Concrete Dirty" }, { 54, "Concrete Worn" },
			{ 55, "Concrete D" }, { 56, "Concrete D Dirty" }, { 57, "Concrete D Worn" },
			{ 3, "Grass" }, { 4, "Dirt" }, { 5, "Gravel" }, { 12, "Dry Lakebed" }, { 13, "Water" }, { 14, "Snow" }, { 15, "Transparent" }
		};

		constexpr NamedCode EDGE_LIGHTS[] = { { 0, "None" }, { 1, "LIRL" }, { 2, "MIRL" }, { 3, "HIRL" } };
		constexpr NamedCode REIL_LIGHTS[] = { { 0, "None" }, { 1, "Omnidirectional REIL" }, { 2, "Unidirectional REIL" }, { 3, "None, but threshold lights" } };
		constexpr NamedCode HELIPAD_LIGHTS[] = { { 0, "None" }, { 1, "Yellow" } };
		constexpr NamedCode HELIPAD_MARKINGS[] = { { 0, "Default" } };
		constexpr NamedCode SIGN_SIZES[] =
		{
			{ 1, "Small Taxiway" }, { 2, "Medium Taxiway" }, { 3, "Large Taxiway" }, { 4, "Large Distance Remaining" }, { 5, "Small Distance Remaining" }
		};
		constexpr NamedCode LIGHT_FIXTURES[] =
		{
			{ 1, "VASI" }, { 2, "PAPI (Left Side)" }, { 3, "PAPI (Right Side)" }, { 4, "PAPI (20 degrees)" }, { 5, "VASI (Tri-Color)" },
			{ 6, "Wig-Wags" }, { 7, "APAPI (Left Side)" }, { 8, "APAPI (Right Side)" }
		};
		constexpr NamedCode APPROACH_LIGHTS[] =
		{
			{ 0, "None" }, { 1, "ALSF-I" }, { 2, "ALSF-II" }, { 3, "Calvert" }, { 4, "Calvert-II" }, { 5, "SSALR" }, { 6, "SSALF" },
			{ 7, "SALS" }, { 8, "MALSR" }, { 9, "MALSF" }, { 10, "MALS" }, { 11, "ODALS" }, { 12, "RAIL" }
		};
		constexpr NamedCode RUNWAY_MARKINGS[] =
		{
			{ 0, "None" }, { 1, "Visual" }, { 2, "Non-Precision (US)" }, { 3, "Precision (US)" }, { 4, "Non-Precision (UK)" },
			{ 5, "Precision (UK)" }, { 6, "Non-Precision (EASA)" }, { 7, "Precision (EASA)" }
		};
		constexpr NamedCode BEACONS[] = { { 1, "Airport" }, { 2, "Seaport" }, { 3, "Heliport" }, { 4, "Military Airport" } };
		constexpr NamedCode AIRPORT_KINDS[] = { { 1, "Airport" }, { 16, "Seaport" }, { 17, "Heliport" } };
		constexpr NamedCode FREQUENCIES[] =
		{
			{ 0, "AWOS" }, { 1, "CTAF" }, { 2, "Clearance Delivery" }, { 3, "Ground Control" }, { 4, "Tower" }, { 5, "Approach" }, { 6, "Departure" }
		};
		constexpr NamedCode SHOW_LEVELS[] =
		{
			{ 1, "1 Default" }, { 2, "2 A lot" }, { 3, "3 Tons" }, { 4, "4 Mega tons" }, { 5, "5 Too many" }, { 6, "6 Totally insane" }
		};
		constexpr NamedCode ELEVATION_MODES[] =
		{
			{ static_cast<int>(ElevationMode::Ground), "0" }, { static_cast<int>(ElevationMode::Msl), "1" }, { static_cast<int>(ElevationMode::Agl), "set_AGL" }
		};
		constexpr NamedCode FOREST_FILLS[] =
		{
			{ static_cast<int>(ForestFill::Area), "Area" }, { static_cast<int>(ForestFill::Line), "Linear" }, { static_cast<int>(ForestFill::Points), "Points" }
		};
		constexpr NamedCode EXCLUSIONS[] =
		{
			{ static_cast<int>(ExcludeObjects), "Objects" }, { static_cast<int>(ExcludeFacades), "Facades" }, { static_cast<int>(ExcludeForests), "Forests" },
			{ static_cast<int>(ExcludeBeaches), "Beaches" }, { static_cast<int>(ExcludeRoads), "Road Networks" }, { static_cast<int>(ExcludeLines), "Lines" },
			{ static_cast<int>(ExcludePolygons), "Draped Polygons" }, { static_cast<int>(ExcludeStrings), "String" }
		};
		constexpr NamedCode AIRCRAFT_BITS[] =
		{
			{ 1, "heavy" }, { 2, "jets" }, { 4, "turboprops" }, { 8, "props" }, { 16, "helos" }, { 32, "fighters" }
		};

		/* @brief apt.dat spelling paired with WED's. */
		struct NamedText
		{
			const char *apt;
			const char *wed;
		};

		constexpr NamedText RAMP_TYPES[] = { { "misc", "Misc" }, { "gate", "Gate" }, { "tie_down", "Tie-Down" }, { "hangar", "Hangar" } };
		constexpr NamedText RAMP_OPERATIONS[] =
		{
			{ "none", "None" }, { "general_aviation", "General Aviation" }, { "airline", "Airline" }, { "cargo", "Cargo" }, { "military", "Military" }
		};
		constexpr NamedText TRUCKS[] =
		{
			{ "baggage_loader", "Baggage Loader" }, { "baggage_train", "Baggage Train" }, { "crew_car", "Crew Car" }, { "crew_ferrari", "Crew Ferrari" },
			{ "crew_limo", "Crew Limo" }, { "fuel_jets", "Fuel Truck (Jets)" }, { "fuel_liners", "Fuel Truck (Liners)" }, { "fuel_props", "Fuel Truck (Props)" },
			{ "food", "Catering Truck" }, { "gpu", "Ground Power Unit" }, { "pushback", "Pushback" }
		};

		/* @brief WED's built-in LinearFeature names (painted lines 1..59, light strings 101..106). */
		constexpr NamedCode BUILTIN_LINES[] =
		{
			{ 1, "Solid Yellow" }, { 2, "Broken Yellow" }, { 3, "Double Solid Yellow" }, { 4, "Runway Hold" }, { 5, "Other Hold" }, { 6, "ILS Hold" },
			{ 7, "ILS Critical Centerline" }, { 8, "Wide Broken Single Yellow" }, { 9, "Wide Broken Double Yellow" },
			{ 20, "Solid White (Roadway)" }, { 21, "Chequered White (Roadway)" }, { 22, "Broken White (Roadway)" },
			{ 51, "Solid Yellow (Black)" }, { 52, "Broken Yellow (Black)" }, { 53, "Double Solid Yellow (Black)" }, { 54, "Runway Hold (Black)" },
			{ 55, "Other Hold (Black)" }, { 56, "ILS Hold (Black)" }, { 57, "ILS Critical Centerline (Black)" },
			{ 58, "Wide Broken Single Yellow (Black)" }, { 59, "Wide Broken Double Yellow (Black)" },
			{ 101, "Taxiway Centerline Lights (Green)" }, { 102, "Taxiway Edge Lights (Blue)" }, { 103, "Hold Short Lights (Amber)" },
			{ 104, "Hold Short Lights (Pulsing Amber)" }, { 105, "Hold Short Centerline Lights (Amber/Green)" }, { 106, "Boundary Edge Lights (Red)" }
		};

		/* @brief Numbered lines and lights X-Plane 12's default library exports; WED names them after the file (MakeWedLineName). */
		constexpr const char *XP12_LINE_STEMS[] =
		{
			"12_double_hold_wide", "14_ils_hold_wide", "23_road_dash", "24_road_edge_wide", "25_road_dash_wide", "30_red_line", "31_red_line_dash",
			"32_red_line_wide", "40_orange_line", "41_blue_line", "42_green_line", "62_double_hold_wide_b", "64_ils_hold_wide_b", "70_road_edge_b",
			"71_road_edge_zipper_b", "72_road_edge_dash_b", "73_road_dash_b", "74_road_edge_wide_b", "75_road_dash_wide_b", "80_red_line_b",
			"81_red_line_dash_b", "82_red_line_wide_b", "90_orange_line_b", "91_blue_line_b", "92_green_line_b",
			"107_centerline_twy_G_uni", "108_centerline_twy_YG_uni"
		};

		template<size_t N>
		const char *NameOf(const NamedCode (&table)[N], const int code)
		{
			for (const NamedCode &entry : table)
			{
				if (entry.code == code)
					return entry.name;
			}
			return nullptr;
		}

		template<size_t N>
		std::optional<int> CodeOf(const NamedCode (&table)[N], const std::string_view name)
		{
			for (const NamedCode &entry : table)
			{
				if (name == entry.name)
					return entry.code;
			}
			return std::nullopt;
		}

		template<size_t N>
		const char *WedText(const NamedText (&table)[N], const std::string_view apt)
		{
			for (const NamedText &entry : table)
			{
				if (apt == entry.apt)
					return entry.wed;
			}
			return nullptr;
		}

		template<size_t N>
		const char *AptText(const NamedText (&table)[N], const std::string_view wed)
		{
			for (const NamedText &entry : table)
			{
				if (wed == entry.wed)
					return entry.apt;
			}
			return nullptr;
		}

		/* @brief Line / light names both ways: WED's built-in ones, X-Plane 12's numbered library lines, the caller's extras. */
		class LineNames
		{
		public:
			explicit LineNames(const std::vector<std::pair<std::string, int>> &extra)
			{
				for (const NamedCode &entry : BUILTIN_LINES)
					Add(entry.name, entry.code);
				for (const char *stem : XP12_LINE_STEMS)
				{
					int code = 0;
					const std::string name = MakeWedLineName(stem, &code);
					Add(name, code);
				}
				for (const auto &[name, code] : extra)
					Add(name, code);
			}

			[[nodiscard]] std::optional<int> Code(const std::string &name) const
			{
				const auto it = m_Codes.find(name);
				return it != m_Codes.end() ? std::optional<int>(it->second) : std::nullopt;
			}

			[[nodiscard]] const std::string *Name(const int code) const
			{
				const auto it = m_Names.find(code);
				return it != m_Names.end() ? &it->second : nullptr;
			}

		private:
			void Add(const std::string &name, const int code)
			{
				if (code <= 0 || name.empty() || m_Names.contains(code))
					return;	// First definition of a code wins, like WED's RescanLines
				m_Names[code] = name;
				m_Codes.emplace(name, code);
			}

			std::map<int, std::string> m_Names;
			std::map<std::string, int> m_Codes;
		};

		// -------------------------------------------------------
		// Numbers in attributes

		/* @brief WED's spelling ("0.0" for zero, fixed decimals), or the shortest exact text when the decimals would lose precision. */
		std::string FormatDouble(const double value, const int decimals)
		{
			if (value == 0.0 || !std::isfinite(value))
				return "0.0";
			char text[64];
			std::snprintf(text, sizeof(text), "%.*f", decimals, value);
			if (std::strtod(text, nullptr) == value)
				return text;
			const auto result = std::to_chars(text, text + sizeof(text), value);
			return std::string(text, result.ptr);
		}

		std::string FormatFloat(const float value, const int decimals)
		{
			if (value == 0.0f || !std::isfinite(value))
				return "0.0";
			char text[64];
			std::snprintf(text, sizeof(text), "%.*f", decimals, static_cast<double>(value));
			if (static_cast<float>(std::strtod(text, nullptr)) == value)
				return text;
			const auto result = std::to_chars(text, text + sizeof(text), value);
			return std::string(text, result.ptr);
		}

		std::string FormatInt(const int value) { return std::to_string(value); }

		double ToDouble(const std::string_view text, const double fallback = 0.0)
		{
			std::string_view trimmed = text;
			while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '+'))
				trimmed.remove_prefix(1);
			double value = fallback;
			const auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);
			return ec == std::errc() ? value : fallback;
		}

		int ToInt(const std::string_view text, const int fallback = 0)
		{
			return static_cast<int>(std::lround(ToDouble(text, fallback)));
		}

		double Attr(const XmlElement *element, const std::string_view key, const double fallback = 0.0)
		{
			if (!element)
				return fallback;
			const std::string *value = element->FindAttribute(key);
			return value ? ToDouble(*value, fallback) : fallback;
		}

		float AttrFloat(const XmlElement *element, const std::string_view key, const float fallback = 0.0f)
		{
			return static_cast<float>(Attr(element, key, fallback));
		}

		int AttrInt(const XmlElement *element, const std::string_view key, const int fallback = 0)
		{
			return static_cast<int>(std::lround(Attr(element, key, fallback)));
		}

		bool AttrBool(const XmlElement *element, const std::string_view key, const bool fallback = false)
		{
			return AttrInt(element, key, fallback ? 1 : 0) != 0;
		}

		std::string AttrText(const XmlElement *element, const std::string_view key, const std::string_view fallback = {})
		{
			return std::string(element ? element->Attribute(key, fallback) : fallback);
		}

		std::string WriteXmlInline(const XmlElement &element)
		{
			std::string out;
			WriteXml(element, out, 0);
			// One line per object: the indentation newlines go, text keeps its own characters
			std::string line;
			line.reserve(out.size());
			bool newline = false;
			for (const char c : out)
			{
				if (c == '\n')
				{
					newline = true;
					continue;
				}
				if (newline && c == ' ')
					continue;
				newline = false;
				line.push_back(c);
			}
			return line;
		}

		std::vector<std::string> SplitRunwayName(const std::string &name)
		{
			std::vector<std::string> parts;
			size_t start = 0;
			while (true)
			{
				const size_t slash = name.find('/', start);
				parts.push_back(name.substr(start, slash == std::string::npos ? std::string::npos : slash - start));
				if (slash == std::string::npos)
					break;
				start = slash + 1;
			}
			return parts;
		}

		std::string JoinAircraftTypes(const int bits)
		{
			std::string text;
			for (const NamedCode &entry : AIRCRAFT_BITS)
			{
				if (bits & entry.code)
				{
					if (!text.empty())
						text.push_back('|');
					text.append(entry.name);
				}
			}
			return text;
		}

		int AircraftTypeBits(const std::string_view types)
		{
			int bits = 0;
			for (const NamedCode &entry : AIRCRAFT_BITS)
			{
				if (types.find(entry.name) != std::string_view::npos)
					bits |= entry.code;
			}
			// "props" is also inside "turboprops"
			if ((bits & 8) && (bits & 4))
			{
				std::string copy(types);
				size_t at = 0;
				bool plain = false;
				while ((at = copy.find("props", at)) != std::string::npos)
				{
					if (at < 5 || copy.compare(at - 5, 5, "turbo") != 0)
						plain = true;
					at += 5;
				}
				if (!plain)
					bits &= ~8;
			}
			return bits;
		}

		std::vector<std::string> SplitList(const std::string_view text, const char separator)
		{
			std::vector<std::string> parts;
			size_t start = 0;
			while (start <= text.size())
			{
				const size_t end = text.find(separator, start);
				const std::string_view part = text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
				if (!part.empty())
					parts.emplace_back(part);
				if (end == std::string_view::npos)
					break;
				start = end + 1;
			}
			return parts;
		}

		/* @brief Shoulder codes are surface codes (0 = none); helipads only know none / asphalt / concrete. */
		int HelipadShoulder(const int code)
		{
			if (code == 0)
				return 0;
			const SurfaceFamily family = GetSurfaceFamily(code);
			return family == SurfaceFamily::Concrete ? 2 : 1;
		}

		// -------------------------------------------------------
		// Import

		struct WedObject
		{
			const XmlElement *element = nullptr;
			std::string cls;
			int id = 0;
			int parent = 0;
			std::vector<int> children;
			std::vector<int> sources;
		};

		std::vector<int> ReadIdList(const XmlElement *list, const std::string_view entry)
		{
			std::vector<int> ids;
			if (!list)
				return ids;
			for (const XmlElement &child : list->children)
			{
				if (child.name == entry)
					ids.push_back(ToInt(child.Attribute("id"), -1));
			}
			return ids;
		}

		bool IsAirportElementClass(const std::string &cls)
		{
			static const std::set<std::string> classes =
			{
				"WED_Runway", "WED_Sealane", "WED_Helipad", "WED_Taxiway", "WED_AirportChain", "WED_AirportBoundary", "WED_TowerViewpoint",
				"WED_AirportBeacon", "WED_Windsock", "WED_AirportSign", "WED_LightFixture", "WED_RampPosition", "WED_TruckParkingLocation",
				"WED_TruckDestination", "WED_ATCFrequency", "WED_ATCFlow", "WED_TaxiRoute", "WED_TaxiRouteNode"
			};
			return classes.contains(cls);
		}

		bool IsOverlayClass(const std::string &cls)
		{
			static const std::set<std::string> classes =
			{
				"WED_ObjPlacement", "WED_PolygonPlacement", "WED_DrapedOrthophoto", "WED_LinePlacement", "WED_StringPlacement", "WED_ForestPlacement",
				"WED_FacadePlacement", "WED_AutogenPlacement", "WED_ExclusionZone", "WED_ExclusionPoly"
			};
			return classes.contains(cls);
		}

		class Importer
		{
		public:
			Importer(SceneryProject &project, std::vector<std::string> *warnings, const LineNames &lines) : m_Project(project), m_Warnings(warnings), m_Lines(lines) {}

			bool Run(const XmlElement &doc, std::string *error)
			{
				const XmlElement *objects = doc.FindChild("objects");
				if (doc.name != "doc" || !objects)
				{
					if (error)
						*error = "Not a WorldEditor document (no <doc><objects>)";
					return false;
				}

				for (const XmlElement &element : objects->children)
				{
					if (element.name != "object")
						continue;
					WedObject object;
					object.element = &element;
					object.cls = element.Attribute("class");
					object.id = ToInt(element.Attribute("id"), -1);
					object.parent = ToInt(element.Attribute("parent_id"), -1);
					object.children = ReadIdList(element.FindChild("children"), "child");
					object.sources = ReadIdList(element.FindChild("sources"), "source");
					if (object.id < 0 || object.cls.empty())
					{
						Warn("An object without a class or id was skipped");
						continue;
					}
					if (!m_Objects.emplace(object.id, std::move(object)).second)
						Warn("Object id " + std::to_string(object.id) + " is used twice; the second was skipped");
				}

				// WED_Root holds the selection, the key objects and the world group; everything placed lives in the world.
				const WedObject *world = nullptr;
				for (const auto &[id, object] : m_Objects)
				{
					if (object.cls != "WED_Root")
						continue;
					for (const int child : object.children)
					{
						const WedObject *candidate = Find(child);
						if (candidate && candidate->cls == "WED_Group")
							world = candidate;
					}
				}
				if (!world)
				{
					if (error)
						*error = "The WorldEditor document has no world (WED_Root / WED_Group)";
					return false;
				}

				ConvertChildren(*world, m_Project.items);

				if (const XmlElement *prefs = doc.FindChild("prefs"))
				{
					for (const XmlElement &pref : prefs->children)
					{
						if (pref.name != "pref")
							continue;
						const std::string name = AttrText(&pref, "name");
						if (pref.HasAttribute("value"))
							m_Project.wedPreferences.emplace_back(name, AttrText(&pref, "value"));
						else
						{
							std::string items;
							for (const XmlElement &item : pref.children)
								items += (items.empty() ? "" : ",") + AttrText(&item, "value");
							m_Project.wedPreferences.emplace_back(name + "#items", items);
						}
					}
				}

				for (const auto &[message, count] : m_Repeats)
				{
					if (count > 1 && m_Warnings)
						m_Warnings->push_back(message + " (" + std::to_string(count) + " times)");
				}
				return true;
			}

		private:
			void Warn(const std::string &message)
			{
				if (!m_Warnings)
					return;
				if (++m_Repeats[message] == 1 && m_Warnings->size() < MAX_WARNINGS)
					m_Warnings->push_back(message);
			}

			const WedObject *Find(const int id) const
			{
				const auto it = m_Objects.find(id);
				return it != m_Objects.end() ? &it->second : nullptr;
			}

			static const XmlElement *Sub(const WedObject &object, const std::string_view name) { return object.element->FindChild(name); }
			static std::string Name(const WedObject &object) { return AttrText(Sub(object, "hierarchy"), "name"); }
			static bool Locked(const WedObject &object) { return AttrBool(Sub(object, "hierarchy"), "locked"); }
			static bool Hidden(const WedObject &object) { return AttrBool(Sub(object, "hierarchy"), "hidden"); }

			std::vector<const WedObject *> Children(const WedObject &object)
			{
				std::vector<const WedObject *> children;
				for (const int id : object.children)
				{
					if (const WedObject *child = Find(id))
						children.push_back(child);
					else
						Warn("A child id the document doesn't define was skipped");
				}
				return children;
			}

			static void Begin(const WedObject &object, SceneryItem &item, const ItemKind kind)
			{
				item.kind = kind;
				item.id = MakeItemId();
				item.name = Name(object);
				item.locked = Locked(object);
				item.hidden = Hidden(object);
			}

			// Hierarchy ----------------------------------------------------

			void ConvertChildren(const WedObject &parent, std::vector<SceneryItem> &out)
			{
				for (const WedObject *child : Children(parent))
					ConvertObject(*child, out);
			}

			void ConvertObject(const WedObject &object, std::vector<SceneryItem> &out)
			{
				if (object.cls == "WED_Group")
				{
					SceneryItem group;
					Begin(object, group, ItemKind::Group);
					ConvertChildren(object, group.children);
					out.push_back(std::move(group));
				}
				else if (object.cls == "WED_Airport")
					ConvertAirport(object, out);
				else if (IsOverlayClass(object.cls))
				{
					SceneryItem item;
					ConvertOverlay(object, item);
					out.push_back(std::move(item));
				}
				else
				{
					if (IsAirportElementClass(object.cls))
						Warn(object.cls + " outside an airport was kept as WorldEditor data");
					else
						Warn(object.cls + " has no Scenery Editor X equivalent; it was kept as WorldEditor data");
					out.push_back(MakeRaw(object));
				}
			}

			void CollectSubtree(const WedObject &object, std::vector<std::string> &raw, std::set<int> &visited)
			{
				if (!visited.insert(object.id).second)
					return;
				raw.push_back(WriteXmlInline(*object.element));
				for (const WedObject *child : Children(object))
					CollectSubtree(*child, raw, visited);
			}

			SceneryItem MakeRaw(const WedObject &object)
			{
				SceneryItem item;
				Begin(object, item, ItemKind::Raw);
				if (item.name.empty())
					item.name = object.cls;
				std::set<int> visited;
				CollectSubtree(object, item.raw, visited);
				return item;
			}

			// Overlay placements -------------------------------------------

			GeoNode ReadNode(const WedObject &node) const
			{
				GeoNode out;
				const XmlElement *point = Sub(node, "point");
				out.latitude = Attr(point, "latitude");
				out.longitude = Attr(point, "longitude");
				out.loLatitude = Attr(point, "ctrl_latitude_lo");
				out.loLongitude = Attr(point, "ctrl_longitude_lo");
				out.hiLatitude = Attr(point, "ctrl_latitude_hi");
				out.hiLongitude = Attr(point, "ctrl_longitude_hi");
				out.split = AttrBool(point, "split");
				if (const XmlElement *texture = Sub(node, "texture_node"))
				{
					out.hasUv = true;
					out.s = AttrFloat(texture, "s");
					out.t = AttrFloat(texture, "t");
					out.sLo = AttrFloat(texture, "sc_lo");
					out.tLo = AttrFloat(texture, "tc_lo");
					out.sHi = AttrFloat(texture, "sc_hi");
					out.tHi = AttrFloat(texture, "tc_hi");
				}
				if (const XmlElement *facade = Sub(node, "facade_node"))
				{
					const std::string wall = AttrText(facade, "wall_type", "Wall 1");
					out.wallType = wall.rfind("Wall ", 0) == 0 ? std::max(0, ToInt(std::string_view(wall).substr(5), 1) - 1) : 0;
				}
				if (const XmlElement *autogen = Sub(node, "autogen_node"))
					out.spawnTiles = AttrBool(autogen, "spawn_tiles", true);
				return out;
			}

			GeoRing ReadChain(const WedObject &owner)
			{
				GeoRing ring;
				for (const WedObject *node : Children(owner))
					ring.push_back(ReadNode(*node));
				return ring;
			}

			std::vector<GeoRing> ReadRings(const WedObject &polygon)
			{
				std::vector<GeoRing> rings;
				for (const WedObject *ring : Children(polygon))
					rings.push_back(ReadChain(*ring));
				return rings;
			}

			uint32_t ReadExclusions(const XmlElement *list)
			{
				uint32_t bits = 0;
				if (!list)
					return bits;
				for (const XmlElement &entry : list->children)
				{
					const std::string value = AttrText(&entry, "value");
					if (const std::optional<int> bit = CodeOf(EXCLUSIONS, value))
						bits |= static_cast<uint32_t>(*bit);
					else
						Warn("Unknown exclusion '" + value + "' ignored");
				}
				return bits;
			}

			template<size_t N>
			int Enum(const NamedCode (&table)[N], const std::string &value, const int fallback, const char *what)
			{
				if (const std::optional<int> code = CodeOf(table, value))
					return *code;
				Warn(std::string("Unknown ") + what + " '" + value + "'; the default was used");
				return fallback;
			}

			void ConvertOverlay(const WedObject &object, SceneryItem &item)
			{
				const std::string &cls = object.cls;
				if (cls == "WED_ObjPlacement")
				{
					Begin(object, item, ItemKind::Object);
					const XmlElement *point = Sub(object, "point");
					const XmlElement *placement = Sub(object, "obj_placement");
					item.latitude = Attr(point, "latitude");
					item.longitude = Attr(point, "longitude");
					item.heading = AttrFloat(point, "heading");
					item.resource = AttrText(placement, "resource");
					item.elevationMode = static_cast<ElevationMode>(Enum(ELEVATION_MODES, AttrText(placement, "custom_msl", "0"), 0, "elevation mode"));
					item.altitude = Attr(placement, "msl");
					item.showLevel = Enum(SHOW_LEVELS, AttrText(placement, "show_level", "1 Default"), 1, "show level");
				}
				else if (cls == "WED_PolygonPlacement")
				{
					Begin(object, item, ItemKind::Polygon);
					const XmlElement *placement = Sub(object, "polygon_placement");
					item.resource = AttrText(placement, "resource");
					item.textureHeading = AttrFloat(placement, "heading");
					item.rings = ReadRings(object);
				}
				else if (cls == "WED_DrapedOrthophoto")
				{
					Begin(object, item, ItemKind::Orthophoto);
					const XmlElement *placement = Sub(object, "draped_orthophoto");
					item.resource = AttrText(placement, "resource");
					item.decal = AttrText(placement, "decal");
					item.textureHeading = AttrFloat(placement, "heading");
					item.texTop = AttrFloat(placement, "tex_top", 1.0f);
					item.texBottom = AttrFloat(placement, "tex_bottom", 0.0f);
					item.texLeft = AttrFloat(placement, "tex_left", 0.0f);
					item.texRight = AttrFloat(placement, "tex_right", 1.0f);
					item.rings = ReadRings(object);
				}
				else if (cls == "WED_LinePlacement")
				{
					Begin(object, item, ItemKind::Line);
					const XmlElement *placement = Sub(object, "line_placement");
					item.resource = AttrText(placement, "resource");
					item.closed = AttrBool(placement, "closed");
					item.rings.push_back(ReadChain(object));
				}
				else if (cls == "WED_StringPlacement")
				{
					Begin(object, item, ItemKind::String);
					const XmlElement *placement = Sub(object, "string_placement");
					item.resource = AttrText(placement, "resource");
					item.closed = AttrBool(placement, "closed");
					item.spacing = AttrFloat(placement, "spacing", 10.0f);
					item.rings.push_back(ReadChain(object));
				}
				else if (cls == "WED_ForestPlacement")
				{
					Begin(object, item, ItemKind::Forest);
					const XmlElement *placement = Sub(object, "forest_placement");
					item.resource = AttrText(placement, "resource");
					item.density = AttrFloat(placement, "density", 1.0f);
					item.fill = static_cast<ForestFill>(Enum(FOREST_FILLS, AttrText(placement, "closed", "Area"), 0, "forest fill mode"));
					item.rings = ReadRings(object);
				}
				else if (cls == "WED_FacadePlacement")
				{
					Begin(object, item, ItemKind::Facade);
					const XmlElement *placement = Sub(object, "facade_placement");
					item.resource = AttrText(placement, "resource");
					item.height = AttrFloat(placement, "height", 10.0f);
					item.pickWalls = AttrBool(placement, "pick_walls");
					item.showLevel = Enum(SHOW_LEVELS, AttrText(placement, "show_level", "1 Default"), 1, "show level");
					item.rings = ReadRings(object);
				}
				else if (cls == "WED_AutogenPlacement")
				{
					Begin(object, item, ItemKind::Autogen);
					const XmlElement *placement = Sub(object, "autogen_placement");
					item.resource = AttrText(placement, "resource");
					item.height = AttrFloat(placement, "height", 10.0f);
					item.spelling = AttrInt(placement, "spelling");
					item.rings = ReadRings(object);
				}
				else if (cls == "WED_ExclusionZone")
				{
					Begin(object, item, ItemKind::ExclusionZone);
					item.exclusions = ReadExclusions(Sub(object, "excluzions"));
					item.rings.push_back(ReadChain(object));
				}
				else if (cls == "WED_ExclusionPoly")
				{
					Begin(object, item, ItemKind::ExclusionPolygon);
					item.exclusions = ReadExclusions(Sub(object, "exclusions"));
					item.rings = ReadRings(object);
				}
			}

			// Airports -----------------------------------------------------

			struct AirportScratch
			{
				std::unordered_map<int, int> taxiNodeIndex;		// WED id -> apt.dat node id
				std::vector<const WedObject *> taxiEdges;
			};

			void ConvertAirport(const WedObject &object, std::vector<SceneryItem> &out)
			{
				ProjectAirport project;
				AptAirport &airport = project.airport;
				const XmlElement *info = Sub(object, "airport");
				airport.type = static_cast<AirportType>(Enum(AIRPORT_KINDS, AttrText(info, "kind", "Airport"), 1, "airport kind"));
				airport.elevationFt = static_cast<int>(std::lround(Attr(info, "elevation") * FEET_PER_METER));
				airport.id = AttrText(info, "icao");
				airport.name = Name(object);
				project.hasAtc = AttrBool(info, "has_atc", true);
				project.sceneryId = AttrInt(info, "scenery_id", -1);

				if (const XmlElement *meta = Sub(object, "meta_data"))
				{
					for (const XmlElement &entry : meta->children)
					{
						const std::string pair = AttrText(&entry, "pair");
						const size_t comma = pair.find(',');
						airport.metadata.push_back({ pair.substr(0, comma), comma == std::string::npos ? std::string() : pair.substr(comma + 1) });
					}
				}
				// Flags WED keeps beside the metadata are 1302 rows in apt.dat (as WED's export writes them)
				if (AttrBool(info, "always_flatten") && !airport.HasMetadata("flatten"))
					airport.metadata.push_back({ "flatten", "1" });
				if (AttrBool(info, "drive_on_left") && !airport.HasMetadata("drive_on_left"))
					airport.metadata.push_back({ "drive_on_left", "1" });

				SceneryItem item;
				Begin(object, item, ItemKind::Airport);
				item.airportId = airport.id;

				AirportScratch scratch;
				for (const WedObject *child : Children(object))
					CollectAirport(*child, project, scratch, item.children);
				FinishTaxiRoutes(project, scratch);

				if (m_Project.FindAirport(airport.id))
					Warn("Airport " + airport.id + " appears twice; the second copy gets a new id");
				m_Project.airports.push_back(std::move(project));
				out.push_back(std::move(item));
			}

			void CollectAirport(const WedObject &object, ProjectAirport &project, AirportScratch &scratch, std::vector<SceneryItem> &overlay)
			{
				const std::string &cls = object.cls;
				if (cls == "WED_Group")
				{
					// Airport elements inside groups belong to the airport (apt.dat has no folders); placements keep their group.
					SceneryItem group;
					Begin(object, group, ItemKind::Group);
					for (const WedObject *child : Children(object))
						CollectAirport(*child, project, scratch, group.children);
					if (!group.children.empty())
						overlay.push_back(std::move(group));
					return;
				}
				if (IsAirportElementClass(cls))
				{
					if (Hidden(object))
					{
						// apt.dat has no hidden rows: keep it for WED, out of the airport (WED doesn't export hidden things either).
						overlay.push_back(MakeRaw(object));
						return;
					}
					ConvertAirportElement(object, project.airport, scratch);
					return;
				}
				ConvertObject(object, overlay);
			}

			void ReadMarkings(const WedObject &node, int &lineType, int &lightType)
			{
				lineType = 0;
				lightType = 0;
				const XmlElement *markings = Sub(node, "markings");
				if (!markings)
					return;
				for (const XmlElement &marking : markings->children)
				{
					const std::string value = AttrText(&marking, "value");
					const std::optional<int> code = m_Lines.Code(value);
					if (!code)
					{
						Warn("Unknown line or light '" + value + "' ignored (add its library to the X-Plane folder setting)");
						continue;
					}
					if (*code >= 100)
						lightType = lightType == 0 ? *code : lightType;
					else
						lineType = lineType == 0 ? *code : lineType;
				}
			}

			/* @brief A WED airport chain as apt.dat nodes, the way WED exports it (split nodes become co-located nodes). */
			AptNodeChain ReadAptChain(const WedObject &chain, const bool closed)
			{
				AptNodeChain out;
				out.closed = closed;
				const std::vector<const WedObject *> nodes = Children(chain);
				const size_t count = nodes.size();
				for (size_t i = 0; i < count; ++i)
				{
					const WedObject &node = *nodes[i];
					const GeoNode geo = ReadNode(node);
					int lineType = 0, lightType = 0;
					ReadMarkings(node, lineType, lightType);

					const bool first = i == 0;
					const bool last = i + 1 == count;
					const bool hasHi = geo.HasHi();
					const bool hasLo = geo.HasLo();
					bool split = geo.split;

					const auto push = [&out, &geo](const bool curved, const double ctrlLat, const double ctrlLon, const int line, const int light)
					{
						AptNode apt;
						apt.latitude = geo.latitude;
						apt.longitude = geo.longitude;
						apt.hasControl = curved;
						apt.controlLatitude = curved ? ctrlLat : 0.0;
						apt.controlLongitude = curved ? ctrlLon : 0.0;
						apt.lineType = line;
						apt.lightType = light;
						out.nodes.push_back(apt);
					};
					// apt.dat stores the outgoing (hi) control point; the mirror of lo is its stand-in when only lo exists.
					const double recipLat = geo.latitude - geo.loLatitude;
					const double recipLon = geo.longitude - geo.loLongitude;
					const double hiLat = geo.latitude + geo.hiLatitude;
					const double hiLon = geo.longitude + geo.hiLongitude;

					if (!closed && last)
					{
						push(hasLo, recipLat, recipLon, 0, 0);
						continue;
					}
					if (first && !closed)
						split = false;
					if (split)
					{
						if (hasLo)
							push(true, recipLat, recipLon, 0, 0);
						push(false, 0.0, 0.0, hasHi ? 0 : lineType, hasHi ? 0 : lightType);
						if (hasHi)
							push(true, hiLat, hiLon, lineType, lightType);
					}
					else if (hasHi)
						push(true, hiLat, hiLon, lineType, lightType);
					else
						push(false, 0.0, 0.0, lineType, lightType);
				}
				return out;
			}

			std::vector<AptNodeChain> ReadAptRings(const WedObject &owner)
			{
				std::vector<AptNodeChain> rings;
				for (const WedObject *chain : Children(owner))
					rings.push_back(ReadAptChain(*chain, true));
				return rings;
			}

			void ReadRunwayNodes(const WedObject &object, double (&lat)[2], double (&lon)[2])
			{
				const std::vector<const WedObject *> nodes = Children(object);
				for (size_t i = 0; i < 2; ++i)
				{
					const XmlElement *point = i < nodes.size() ? Sub(*nodes[i], "point") : nullptr;
					lat[i] = Attr(point, "latitude");
					lon[i] = Attr(point, "longitude");
				}
				if (nodes.size() != 2)
					Warn("A runway without exactly two end nodes was read with missing ends");
			}

			void ConvertAirportElement(const WedObject &object, AptAirport &airport, AirportScratch &scratch)
			{
				const std::string &cls = object.cls;
				const XmlElement *point = Sub(object, "point");
				const double latitude = Attr(point, "latitude");
				const double longitude = Attr(point, "longitude");
				const float heading = AttrFloat(point, "heading");

				if (cls == "WED_Runway")
				{
					AptRunway runway;
					const XmlElement *line = Sub(object, "line");
					const XmlElement *attrs = Sub(object, "runway");
					runway.width = AttrFloat(line, "width", 30.0f);
					runway.surface = Enum(SURFACES, AttrText(attrs, "surface", "Concrete"), SurfaceConcrete, "surface");
					const int shoulder = AttrText(attrs, "shoulder", "None") == "None" ? 0 : Enum(SURFACES, AttrText(attrs, "shoulder"), 0, "shoulder");
					const int shoulderWidth = static_cast<int>(std::lround(Attr(attrs, "shoulder_width")));
					runway.shoulder = shoulder != 0 && shoulderWidth > 0 ? shoulder + 100 * shoulderWidth : shoulder;
					runway.smoothness = AttrFloat(attrs, "roughness", 0.25f);
					runway.centerlineLights = AttrBool(attrs, "center_lites");
					runway.edgeLights = Enum(EDGE_LIGHTS, AttrText(attrs, "edge_lites", "None"), 0, "edge lights");
					runway.distanceSigns = AttrBool(attrs, "distance_signs");

					const std::vector<std::string> names = SplitRunwayName(Name(object));
					double lat[2], lon[2];
					ReadRunwayNodes(object, lat, lon);
					for (int end = 0; end < 2; ++end)
					{
						AptRunwayEnd &e = runway.ends[end];
						const std::string n = std::to_string(end + 1);
						e.designator = end < static_cast<int>(names.size()) ? names[end] : std::string("XX");
						e.latitude = lat[end];
						e.longitude = lon[end];
						e.displacedThreshold = AttrFloat(attrs, "displaced" + n);
						e.blastPad = AttrFloat(attrs, "blastpad" + n);
						e.marking = Enum(RUNWAY_MARKINGS, AttrText(attrs, "markings" + n, "None"), 0, "runway marking");
						e.approachLights = Enum(APPROACH_LIGHTS, AttrText(attrs, "app_lites" + n, "None"), 0, "approach lights");
						e.touchdownZoneLights = AttrBool(attrs, "TDZL" + n);
						e.reil = Enum(REIL_LIGHTS, AttrText(attrs, "REIL" + n, "None"), 0, "REIL");
					}
					airport.runways.push_back(std::move(runway));
				}
				else if (cls == "WED_Sealane")
				{
					AptWaterRunway runway;
					runway.width = AttrFloat(Sub(object, "line"), "width", 30.0f);
					runway.buoys = AttrBool(Sub(object, "sealane"), "has_buoys");
					const std::vector<std::string> names = SplitRunwayName(Name(object));
					double lat[2], lon[2];
					ReadRunwayNodes(object, lat, lon);
					for (int end = 0; end < 2; ++end)
					{
						runway.ends[end].designator = end < static_cast<int>(names.size()) ? names[end] : std::string("XX");
						runway.ends[end].latitude = lat[end];
						runway.ends[end].longitude = lon[end];
					}
					airport.waterRunways.push_back(std::move(runway));
				}
				else if (cls == "WED_Helipad")
				{
					AptHelipad helipad;
					const XmlElement *attrs = Sub(object, "helipad");
					helipad.designator = Name(object);
					helipad.latitude = latitude;
					helipad.longitude = longitude;
					helipad.heading = heading;
					helipad.width = AttrFloat(point, "width", 20.0f);
					helipad.length = AttrFloat(point, "length", 20.0f);
					helipad.surface = Enum(SURFACES, AttrText(attrs, "surface", "Concrete"), SurfaceConcrete, "surface");
					helipad.markings = Enum(HELIPAD_MARKINGS, AttrText(attrs, "markings", "Default"), 0, "helipad marking");
					const std::string shoulder = AttrText(attrs, "shoulder", "None");
					helipad.shoulder = shoulder == "None" ? 0 : HelipadShoulder(Enum(SURFACES, shoulder, 0, "shoulder"));
					helipad.smoothness = AttrFloat(attrs, "roughness", 0.25f);
					helipad.edgeLights = Enum(HELIPAD_LIGHTS, AttrText(attrs, "lights", "None"), 0, "helipad lights");
					airport.helipads.push_back(std::move(helipad));
				}
				else if (cls == "WED_Taxiway")
				{
					AptPavement pavement;
					const XmlElement *attrs = Sub(object, "taxiway");
					pavement.description = Name(object);
					pavement.surface = Enum(SURFACES, AttrText(attrs, "surface", "Concrete"), SurfaceConcrete, "surface");
					pavement.smoothness = AttrFloat(attrs, "roughness", 0.25f);
					pavement.textureHeading = AttrFloat(attrs, "heading");
					pavement.rings = ReadAptRings(object);
					airport.pavements.push_back(std::move(pavement));
				}
				else if (cls == "WED_AirportChain")
				{
					AptLinearFeature feature;
					feature.description = Name(object);
					feature.chains.push_back(ReadAptChain(object, AttrBool(Sub(object, "airport_chain"), "closed")));
					airport.linearFeatures.push_back(std::move(feature));
				}
				else if (cls == "WED_AirportBoundary")
				{
					AptBoundary boundary;
					boundary.description = Name(object);
					boundary.rings = ReadAptRings(object);
					airport.boundaries.push_back(std::move(boundary));
				}
				else if (cls == "WED_TowerViewpoint")
				{
					if (airport.viewpoint)
						Warn("An airport has more than one tower viewpoint; the first was kept");
					else
						airport.viewpoint = AptViewpoint{ latitude, longitude, static_cast<float>(Attr(Sub(object, "tower_viewpoint"), "height", 20.0) * FEET_PER_METER), Name(object) };
				}
				else if (cls == "WED_AirportBeacon")
				{
					if (airport.beacon)
						Warn("An airport has more than one beacon; the first was kept");
					else
						airport.beacon = AptBeacon{ latitude, longitude, Enum(BEACONS, AttrText(Sub(object, "airport_beacon"), "type", "Airport"), 1, "beacon type"), Name(object) };
				}
				else if (cls == "WED_Windsock")
					airport.windsocks.push_back({ latitude, longitude, AttrBool(Sub(object, "windsock"), "lit"), Name(object) });
				else if (cls == "WED_AirportSign")
					airport.signs.push_back({ latitude, longitude, heading, Enum(SIGN_SIZES, AttrText(Sub(object, "airport_sign"), "size", "Medium Taxiway"), 2, "sign size"), Name(object) });
				else if (cls == "WED_LightFixture")
				{
					const XmlElement *attrs = Sub(object, "light_fixture");
					airport.lightFixtures.push_back({ latitude, longitude, Enum(LIGHT_FIXTURES, AttrText(attrs, "type", "VASI"), 1, "light fixture"), heading,
						AttrFloat(attrs, "angle", 3.0f), Name(object) });
				}
				else if (cls == "WED_RampPosition")
				{
					const XmlElement *attrs = Sub(object, "ramp_start");
					AptStartup startup;
					startup.latitude = latitude;
					startup.longitude = longitude;
					startup.heading = heading;
					startup.name = Name(object);
					const std::string type = AttrText(attrs, "type", "Misc");
					const char *aptType = AptText(RAMP_TYPES, type);
					if (!aptType)
						Warn("Unknown ramp start type '" + type + "'; misc was used");
					startup.type = aptType ? aptType : "misc";
					startup.aircraftTypes = JoinAircraftTypes(AttrInt(attrs, "traffic"));
					startup.hasMetadata = true;
					startup.widthCode = AttrText(attrs, "width", "C");
					const std::string operation = AttrText(attrs, "ramp_op_type", "None");
					const char *aptOperation = AptText(RAMP_OPERATIONS, operation);
					startup.operationType = aptOperation ? aptOperation : "none";
					startup.airlines = AttrText(attrs, "airlines");
					airport.startups.push_back(std::move(startup));
				}
				else if (cls == "WED_TruckParkingLocation")
				{
					const XmlElement *attrs = Sub(object, "truck_parking_spot");
					AptTruckParking parking;
					parking.latitude = latitude;
					parking.longitude = longitude;
					parking.heading = heading;
					parking.name = Name(object);
					const std::string type = AttrText(attrs, "type", "Fuel Truck (Props)");
					const char *aptType = AptText(TRUCKS, type);
					if (!aptType)
						Warn("Unknown service truck '" + type + "'; gpu was used");
					parking.type = aptType ? aptType : "gpu";
					parking.count = parking.type == "baggage_train" ? AttrInt(attrs, "number_of_cars", 3) : 0;
					parking.customObject = AttrText(attrs, "custom_veh");
					airport.truckParkings.push_back(std::move(parking));
				}
				else if (cls == "WED_TruckDestination")
				{
					AptTruckDestination destination;
					destination.latitude = latitude;
					destination.longitude = longitude;
					destination.heading = heading;
					destination.name = Name(object);
					if (const XmlElement *types = Sub(object, "truck_destination"))
					{
						for (const XmlElement &type : types->children)
						{
							const char *aptType = AptText(TRUCKS, type.Attribute("value"));
							if (!aptType)
								continue;
							if (!destination.truckTypes.empty())
								destination.truckTypes.push_back('|');
							destination.truckTypes.append(aptType);
						}
					}
					airport.truckDestinations.push_back(std::move(destination));
				}
				else if (cls == "WED_ATCFrequency")
				{
					const XmlElement *attrs = Sub(object, "atc_frequency");
					AptFrequency frequency;
					frequency.type = static_cast<AptFrequency::Type>(Enum(FREQUENCIES, AttrText(attrs, "kind", "Tower"), 4, "frequency type"));
					frequency.frequencyKhz = static_cast<int>(std::lround(Attr(attrs, "freq", 118.0) * 1000.0));
					frequency.name = Name(object);
					airport.frequencies.push_back(std::move(frequency));
				}
				else if (cls == "WED_ATCFlow")
					airport.flows.push_back(ReadFlow(object));
				else if (cls == "WED_TaxiRouteNode")
				{
					if (!airport.taxiNetwork)
						airport.taxiNetwork.emplace();
					AptTaxiNode node;
					node.latitude = latitude;
					node.longitude = longitude;
					node.id = static_cast<int>(airport.taxiNetwork->nodes.size());
					node.name = Name(object);
					scratch.taxiNodeIndex[object.id] = node.id;
					airport.taxiNetwork->nodes.push_back(std::move(node));
				}
				else if (cls == "WED_TaxiRoute")
					scratch.taxiEdges.push_back(&object);
			}

			AptFlow ReadFlow(const WedObject &object)
			{
				const XmlElement *attrs = Sub(object, "atc_flow");
				AptFlow flow;
				flow.name = Name(object);
				const std::string icao = AttrText(attrs, "icao");
				flow.ceilingRules.push_back({ icao, AttrInt(attrs, "cld_min") });
				flow.visibilityRules.push_back({ icao, AttrFloat(attrs, "vis_min") });
				const std::string patternRunway = AttrText(attrs, "pattern_rwy", "None");
				if (patternRunway != "None" && !patternRunway.empty())
					flow.vfrPatterns.push_back({ patternRunway, AttrText(attrs, "pattern_side", "Left") == "Right" });

				for (const WedObject *child : Children(object))
				{
					if (child->cls == "WED_ATCWindRule")
					{
						const XmlElement *wind = Sub(*child, "atc_windrule");
						flow.windRules.push_back({ AttrText(wind, "weather_icao"), AttrInt(wind, "dir_lo_degs_mag"), AttrInt(wind, "dir_hi_degs_mag"), AttrInt(wind, "speed_knots") });
					}
					else if (child->cls == "WED_ATCTimeRule")
					{
						const XmlElement *time = Sub(*child, "atc_timerule");
						flow.timeRules.push_back({ AttrInt(time, "start_zulu"), AttrInt(time, "end_zulu") });
					}
					else if (child->cls == "WED_ATCRunwayUse")
					{
						const XmlElement *use = Sub(*child, "runway_use");
						AptRunwayUse runway;
						runway.runway = AttrText(use, "rwy");
						runway.frequencyKhz = static_cast<int>(std::lround(Attr(use, "dep_frq") * 1000.0));
						const int operations = AttrInt(use, "operations");
						runway.arrivals = (operations & 1) != 0;
						runway.departures = (operations & 2) != 0;
						runway.aircraftTypes = JoinAircraftTypes(AttrInt(use, "traffic"));
						runway.onCourseFrom = AttrInt(use, "dep_min");
						runway.onCourseTo = AttrInt(use, "dep_max");
						runway.departureFrom = AttrInt(use, "ini_min");
						runway.departureTo = AttrInt(use, "ini_max");
						runway.name = Name(*child);
						runway.highResolution = true;
						flow.runwayUses.push_back(std::move(runway));
					}
					else
						Warn(child->cls + " inside an ATC flow was skipped");
				}
				return flow;
			}

			static void AddZone(std::vector<AptActiveZone> &zones, const char *classification, const std::set<std::string> &runways)
			{
				if (runways.empty())
					return;
				std::string list;
				for (const std::string &runway : runways)
					list += (list.empty() ? "" : ",") + runway;
				zones.push_back({ classification, list });
			}

			static std::set<std::string> ReadRunwaySet(const XmlElement *list)
			{
				std::set<std::string> runways;
				if (list)
				{
					for (const XmlElement &entry : list->children)
						runways.insert(std::string(entry.Attribute("value")));
				}
				return runways;
			}

			void FinishTaxiRoutes(ProjectAirport &project, AirportScratch &scratch)
			{
				if (scratch.taxiEdges.empty())
					return;
				AptAirport &airport = project.airport;
				if (!airport.taxiNetwork)
					airport.taxiNetwork.emplace();

				for (const WedObject *object : scratch.taxiEdges)
				{
					if (object->sources.size() != 2 || !scratch.taxiNodeIndex.contains(object->sources[0]) || !scratch.taxiNodeIndex.contains(object->sources[1]))
					{
						Warn("A taxi route whose end nodes aren't taxi route nodes of its airport was skipped");
						continue;
					}
					const XmlElement *attrs = Sub(*object, "taxi_route");
					AptTaxiEdge edge;
					edge.from = scratch.taxiNodeIndex[object->sources[0]];
					edge.to = scratch.taxiNodeIndex[object->sources[1]];
					edge.oneWay = AttrBool(attrs, "oneway");
					edge.groundVehicles = AttrText(attrs, "vehicle_class", "Aircraft") == "Ground Trucks";

					const XmlElement *shape = Sub(*object, "edge");
					if (Attr(shape, "ctrl_latitude_lo") != 0.0 || Attr(shape, "ctrl_longitude_lo") != 0.0 || Attr(shape, "ctrl_latitude_hi") != 0.0 || Attr(shape, "ctrl_longitude_hi") != 0.0)
						Warn("Curved taxi routes were made straight (apt.dat 1203 shape points aren't supported yet)");

					if (edge.groundVehicles)
						edge.name = Name(*object);
					else
					{
						const std::string runway = AttrText(attrs, "runway", "None");
						std::set<std::string> departures = ReadRunwaySet(Sub(*object, "departures"));
						std::set<std::string> arrivals = ReadRunwaySet(Sub(*object, "arrivals"));
						std::set<std::string> ils = ReadRunwaySet(Sub(*object, "ils_holds"));
						if (runway != "None" && !runway.empty())
						{
							edge.restriction = "runway";
							edge.name = runway;
							// A runway edge is a hot zone for both of its ends (WED's export adds them the same way)
							for (const std::string &part : SplitRunwayName(runway))
							{
								if (part.empty() || part == "XXX")
									continue;
								departures.insert(part);
								arrivals.insert(part);
								ils.insert(part);
							}
						}
						else
						{
							edge.restriction = "taxiway_" + AttrText(attrs, "width", "E");
							edge.name = Name(*object);
						}
						AddZone(edge.activeZones, "departure", departures);
						AddZone(edge.activeZones, "arrival", arrivals);
						AddZone(edge.activeZones, "ils", ils);
					}
					airport.taxiNetwork->edges.push_back(std::move(edge));
				}
			}

			SceneryProject &m_Project;
			std::vector<std::string> *m_Warnings;
			const LineNames &m_Lines;
			std::unordered_map<int, WedObject> m_Objects;
			std::map<std::string, size_t> m_Repeats;
		};

		// -------------------------------------------------------
		// Export

		class Exporter
		{
		public:
			Exporter(const SceneryProject &project, std::vector<std::string> *warnings, const LineNames &lines) : m_Project(project), m_Warnings(warnings), m_Lines(lines) {}

			std::string Run(const std::string &writer)
			{
				const int root = Add("WED_Root", 0, "root", Hierarchy::NameOnly);
				const int selection = Add("WED_Select", root, "selection", Hierarchy::NameOnly);
				Element(selection, "selection");
				const int keys = Add("WED_KeyObjects", root, "choices", Hierarchy::NameOnly);
				const int world = Add("WED_Group", root, "world");

				ReserveRawIds(m_Project.items);
				for (const SceneryItem &item : m_Project.items)
					ExportItem(item, world);

				XmlElement &keyList = Element(keys, "keys");
				if (m_FirstAirport != 0)
					keyList.AddChild("key").Set("name", "airport").Set("id", FormatInt(m_FirstAirport));

				XmlElement doc;
				doc.name = "doc";
				XmlElement &objects = doc.AddChild("objects");
				objects.children.reserve(m_Objects.size());
				for (const OutObject &object : m_Objects)
				{
					if (object.cls.empty())
						continue;	// A reserved raw id nothing filled (its item failed to parse)
					objects.children.push_back(ToXml(object));
				}

				XmlElement &prefs = doc.AddChild("prefs");
				bool compatibility = false;
				for (const auto &[name, value] : m_Project.wedPreferences)
				{
					constexpr std::string_view ITEMS = "#items";
					if (name.size() > ITEMS.size() && name.compare(name.size() - ITEMS.size(), ITEMS.size(), ITEMS) == 0)
					{
						XmlElement &pref = prefs.AddChild("pref").Set("name", name.substr(0, name.size() - ITEMS.size()));
						for (const std::string &entry : SplitList(value, ','))
							pref.AddChild("item").Set("value", entry);
						continue;
					}
					compatibility |= name == "doc/xml_compatibility";
					prefs.AddChild("pref").Set("name", name).Set("value", value);
				}
				if (!compatibility)
					prefs.AddChild("pref").Set("name", "doc/xml_compatibility").Set("value", "205");	// WED 2.5 document layout

				std::string out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!-- written by " + EscapeXml(writer) + " -->\n";
				WriteXml(doc, out, 0);

				for (const auto &[message, count] : m_Repeats)
				{
					if (count > 1 && m_Warnings)
						m_Warnings->push_back(message + " (" + std::to_string(count) + " times)");
				}
				return out;
			}

		private:
			enum class Hierarchy : uint8_t
			{
				Full,
				NameOnly
			};

			struct OutObject
			{
				std::string cls;
				int id = 0;
				int parent = 0;
				std::vector<int> sources;
				std::vector<int> viewers;
				std::vector<int> children;
				std::vector<XmlElement> elements;
			};

			void Warn(const std::string &message)
			{
				if (!m_Warnings)
					return;
				if (++m_Repeats[message] == 1 && m_Warnings->size() < MAX_WARNINGS)
					m_Warnings->push_back(message);
			}

			OutObject &Get(const int id) { return m_Objects[static_cast<size_t>(id - 1)]; }

			int Reserve()
			{
				m_Objects.emplace_back();
				m_Objects.back().id = static_cast<int>(m_Objects.size());
				return m_Objects.back().id;
			}

			int Add(const std::string &cls, const int parent, const std::string &name, const Hierarchy hierarchy = Hierarchy::Full, const bool locked = false, const bool hidden = false)
			{
				const int id = Reserve();
				OutObject &object = Get(id);
				object.cls = cls;
				object.parent = parent;
				XmlElement &h = Element(id, "hierarchy");
				h.Set("name", name);
				if (hierarchy == Hierarchy::Full)
				{
					h.Set("locked", locked ? "1" : "0");
					h.Set("hidden", hidden ? "1" : "0");
				}
				if (parent != 0)
					Get(parent).children.push_back(id);
				return id;
			}

			int Add(const std::string &cls, const int parent, const SceneryItem &item, const std::string &fallbackName)
			{
				return Add(cls, parent, item.name.empty() ? fallbackName : item.name, Hierarchy::Full, item.locked, item.hidden);
			}

			XmlElement &Element(const int id, const std::string &name)
			{
				std::vector<XmlElement> &elements = Get(id).elements;
				for (XmlElement &element : elements)
				{
					if (element.name == name)
						return element;
				}
				elements.emplace_back();
				elements.back().name = name;
				return elements.back();
			}

			static XmlElement ToXml(const OutObject &object)
			{
				XmlElement xml;
				xml.name = "object";
				xml.Set("class", object.cls).Set("id", FormatInt(object.id)).Set("parent_id", FormatInt(object.parent));
				const auto list = [&xml](const char *listName, const char *entryName, const std::vector<int> &ids)
				{
					XmlElement &element = xml.AddChild(listName);
					for (const int id : ids)
						element.AddChild(entryName).Set("id", FormatInt(id));
				};
				if (!object.sources.empty())
					list("sources", "source", object.sources);
				if (!object.viewers.empty())
					list("viewers", "viewer", object.viewers);
				list("children", "child", object.children);
				for (const XmlElement &element : object.elements)
					xml.children.push_back(element);
				return xml;
			}

			// Points -------------------------------------------------------

			void Point(const int id, const double latitude, const double longitude)
			{
				Element(id, "point").Set("latitude", FormatDouble(latitude, 9)).Set("longitude", FormatDouble(longitude, 9));
			}

			void PointHeading(const int id, const double latitude, const double longitude, const float heading)
			{
				Point(id, latitude, longitude);
				Element(id, "point").Set("heading", FormatFloat(heading, 2));
			}

			void BezierPoint(const int id, const GeoNode &node)
			{
				Point(id, node.latitude, node.longitude);
				Element(id, "point")
					.Set("split", node.split ? "1" : "0")
					.Set("ctrl_latitude_lo", FormatDouble(node.loLatitude, 9))
					.Set("ctrl_longitude_lo", FormatDouble(node.loLongitude, 9))
					.Set("ctrl_latitude_hi", FormatDouble(node.hiLatitude, 9))
					.Set("ctrl_longitude_hi", FormatDouble(node.hiLongitude, 9));
			}

			enum class NodeClass : uint8_t
			{
				Simple,			// WED_SimpleBoundaryNode
				Bezier,			// WED_SimpleBezierBoundaryNode
				Texture,		// WED_TextureBezierNode
				Facade,			// WED_FacadeNode
				Autogen			// WED_AutogenNode
			};

			void Node(const GeoNode &node, const int parent, const size_t index, const NodeClass type)
			{
				const std::string name = "Node " + std::to_string(index + 1);
				switch (type)
				{
					case NodeClass::Simple:
					{
						const int id = Add("WED_SimpleBoundaryNode", parent, name);
						Point(id, node.latitude, node.longitude);
						if (node.HasBezier())
							Warn("Bezier handles on forest / exclusion nodes were dropped (WED has none there)");
						break;
					}
					case NodeClass::Bezier:
						BezierPoint(Add("WED_SimpleBezierBoundaryNode", parent, name), node);
						break;
					case NodeClass::Texture:
					{
						const int id = Add("WED_TextureBezierNode", parent, name);
						BezierPoint(id, node);
						Element(id, "texture_node")
							.Set("s", FormatFloat(node.s, 4)).Set("t", FormatFloat(node.t, 4))
							.Set("sc_lo", FormatFloat(node.sLo, 4)).Set("tc_lo", FormatFloat(node.tLo, 4))
							.Set("sc_hi", FormatFloat(node.sHi, 4)).Set("tc_hi", FormatFloat(node.tHi, 4));
						break;
					}
					case NodeClass::Facade:
					{
						const int id = Add("WED_FacadeNode", parent, name);
						BezierPoint(id, node);
						Element(id, "facade_node").Set("wall_type", "Wall " + std::to_string(std::clamp(node.wallType, 0, 39) + 1));
						break;
					}
					case NodeClass::Autogen:
					{
						const int id = Add("WED_AutogenNode", parent, name);
						Point(id, node.latitude, node.longitude);
						Element(id, "autogen_node").Set("spawn_tiles", node.spawnTiles ? "1" : "0");
						break;
					}
				}
			}

			void Chain(const GeoRing &ring, const int parent, const NodeClass type)
			{
				for (size_t i = 0; i < ring.size(); ++i)
					Node(ring[i], parent, i, type);
			}

			void Rings(const std::vector<GeoRing> &rings, const int parent, const char *ringClass, const std::string &label, const NodeClass type)
			{
				for (size_t r = 0; r < rings.size(); ++r)
				{
					const int ring = Add(ringClass, parent, label + (r == 0 ? " Outer Ring" : " Hole " + std::to_string(r)));
					Chain(rings[r], ring, type);
				}
			}

			static const GeoRing &FirstRing(const SceneryItem &item)
			{
				static const GeoRing empty;
				return item.rings.empty() ? empty : item.rings.front();
			}

			// Hierarchy ----------------------------------------------------

			void ExportItem(const SceneryItem &item, const int parent)
			{
				if (!item.properties.empty())
					Warn("Item properties (\"other-properties\") have no place in earth.wed.xml and were left out (the project keeps them)");
				switch (item.kind)
				{
					case ItemKind::Group:
					{
						const int id = Add("WED_Group", parent, item, "Group");
						for (const SceneryItem &child : item.children)
							ExportItem(child, id);
						break;
					}
					case ItemKind::Airport:
						ExportAirport(item, parent);
						break;
					case ItemKind::Object:
					{
						const int id = Add("WED_ObjPlacement", parent, item, item.resource);
						PointHeading(id, item.latitude, item.longitude, item.heading);
						Element(id, "obj_placement")
							.Set("custom_msl", NameOf(ELEVATION_MODES, static_cast<int>(item.elevationMode)) ? NameOf(ELEVATION_MODES, static_cast<int>(item.elevationMode)) : "0")
							.Set("msl", FormatDouble(item.altitude, 3))
							.Set("resource", item.resource)
							.Set("show_level", ShowLevel(item.showLevel));
						break;
					}
					case ItemKind::Polygon:
					{
						const int id = Add("WED_PolygonPlacement", parent, item, item.resource);
						Element(id, "polygon_placement").Set("heading", FormatFloat(item.textureHeading, 2)).Set("resource", item.resource);
						Rings(item.rings, id, "WED_Ring", item.name, NodeClass::Bezier);
						break;
					}
					case ItemKind::Orthophoto:
					{
						const int id = Add("WED_DrapedOrthophoto", parent, item, item.resource);
						Element(id, "draped_orthophoto")
							.Set("heading", FormatFloat(item.textureHeading, 1)).Set("resource", item.resource).Set("decal", item.decal)
							.Set("tex_top", FormatFloat(item.texTop, 4)).Set("tex_bottom", FormatFloat(item.texBottom, 4))
							.Set("tex_left", FormatFloat(item.texLeft, 4)).Set("tex_right", FormatFloat(item.texRight, 4));
						Rings(item.rings, id, "WED_Ring", item.name, NodeClass::Texture);
						break;
					}
					case ItemKind::Line:
					{
						const int id = Add("WED_LinePlacement", parent, item, item.resource);
						Element(id, "line_placement").Set("closed", item.closed ? "1" : "0").Set("resource", item.resource);
						Chain(FirstRing(item), id, NodeClass::Bezier);
						break;
					}
					case ItemKind::String:
					{
						const int id = Add("WED_StringPlacement", parent, item, item.resource);
						Element(id, "string_placement").Set("resource", item.resource).Set("spacing", FormatFloat(item.spacing, 2)).Set("closed", item.closed ? "1" : "0");
						Chain(FirstRing(item), id, NodeClass::Bezier);
						break;
					}
					case ItemKind::Forest:
					{
						const int id = Add("WED_ForestPlacement", parent, item, item.resource);
						Element(id, "forest_placement")
							.Set("density", FormatFloat(item.density, 1)).Set("resource", item.resource)
							.Set("closed", NameOf(FOREST_FILLS, static_cast<int>(item.fill)) ? NameOf(FOREST_FILLS, static_cast<int>(item.fill)) : "Area");
						Rings(item.rings, id, "WED_ForestRing", item.name, NodeClass::Simple);
						break;
					}
					case ItemKind::Facade:
					{
						const int id = Add("WED_FacadePlacement", parent, item, item.resource);
						Element(id, "facade_placement")
							.Set("height", FormatFloat(item.height, 0)).Set("resource", item.resource)
							.Set("pick_walls", item.pickWalls ? "1" : "0").Set("show_level", ShowLevel(item.showLevel));
						Rings(item.rings, id, "WED_FacadeRing", item.name, NodeClass::Facade);
						break;
					}
					case ItemKind::Autogen:
					{
						const int id = Add("WED_AutogenPlacement", parent, item, item.resource);
						Element(id, "autogen_placement").Set("height", FormatFloat(item.height, 1)).Set("spelling", FormatInt(item.spelling)).Set("resource", item.resource);
						Rings(item.rings, id, "WED_Ring", item.name, NodeClass::Autogen);
						break;
					}
					case ItemKind::ExclusionZone:
					{
						const int id = Add("WED_ExclusionZone", parent, item, "Exclusion Zone");
						Exclusions(Element(id, "excluzions"), item.exclusions);
						const GeoRing &corners = FirstRing(item);
						for (size_t i = 0; i < corners.size(); ++i)
						{
							const int node = Add("WED_SimpleBoundaryNode", id, i == 0 ? "Start" : i == 1 ? "End" : "Node " + std::to_string(i + 1));
							Point(node, corners[i].latitude, corners[i].longitude);
						}
						break;
					}
					case ItemKind::ExclusionPolygon:
					{
						const int id = Add("WED_ExclusionPoly", parent, item, "Exclusion Polygon");
						Exclusions(Element(id, "exclusions"), item.exclusions);
						Rings(item.rings, id, "WED_Ring", item.name, NodeClass::Simple);
						break;
					}
					case ItemKind::Raw:
						ExportRaw(item, parent);
						break;
				}
			}

			static std::string ShowLevel(const int level)
			{
				const char *name = NameOf(SHOW_LEVELS, level);
				return name ? name : "1 Default";
			}

			static void Exclusions(XmlElement &list, const uint32_t bits)
			{
				for (const NamedCode &entry : EXCLUSIONS)
				{
					if (bits & static_cast<uint32_t>(entry.code))
						list.AddChild("exclusion").Set("value", entry.name);
				}
			}

			// Raw WorldEditor data -----------------------------------------

			void ReserveRawIds(const std::vector<SceneryItem> &items)
			{
				for (const SceneryItem &item : items)
				{
					if (item.kind == ItemKind::Raw)
					{
						for (const std::string &text : item.raw)
						{
							XmlElement element;
							if (!ParseXml(text, element))
								continue;
							const int original = ToInt(element.Attribute("id"), -1);
							if (original < 0)
								continue;
							if (m_RawIds.contains(original))
								Warn("Two WorldEditor data items use the same object id; references between them may be wrong");
							else
								m_RawIds[original] = Reserve();
						}
					}
					ReserveRawIds(item.children);
				}
			}

			int MapRawId(const int original)
			{
				const auto it = m_RawIds.find(original);
				return it != m_RawIds.end() ? it->second : 0;
			}

			void ExportRaw(const SceneryItem &item, const int parent)
			{
				for (size_t index = 0; index < item.raw.size(); ++index)
				{
					XmlElement element;
					std::string error;
					if (!ParseXml(item.raw[index], element, &error) || element.name != "object")
					{
						Warn("A damaged WorldEditor data item was skipped (" + error + ")");
						continue;
					}
					const int id = MapRawId(ToInt(element.Attribute("id"), -1));
					if (id == 0)
						continue;

					// The subtree's root (stored first) goes where the item sits now, under its current name
					const bool root = index == 0;
					if (root)
						Get(parent).children.push_back(id);
					OutObject &object = Get(id);
					object.cls = std::string(element.Attribute("class"));
					object.parent = root ? parent : MapRawId(ToInt(element.Attribute("parent_id"), -1));

					const auto mapList = [this](const XmlElement &list, std::vector<int> &ids)
					{
						for (const XmlElement &entry : list.children)
						{
							if (const int mapped = MapRawId(ToInt(entry.Attribute("id"), -1)))
								ids.push_back(mapped);
							else
								Warn("A WorldEditor data reference to an object that wasn't kept was dropped");
						}
					};
					for (XmlElement &child : element.children)
					{
						if (child.name == "children")
							mapList(child, object.children);
						else if (child.name == "sources")
							mapList(child, object.sources);
						else if (child.name == "viewers")
							mapList(child, object.viewers);
						else
						{
							if (root && child.name == "hierarchy" && !item.name.empty())
								child.Set("name", item.name);
							object.elements.push_back(std::move(child));
						}
					}
				}
			}

			// Airports -----------------------------------------------------

			void Markings(const int node, const int lineType, const int lightType)
			{
				XmlElement &markings = Element(node, "markings");
				for (const int code : { lineType, lightType })
				{
					if (code == 0)
						continue;
					if (const std::string *name = m_Lines.Name(code))
						markings.AddChild("marking").Set("value", *name);
					else
						Warn("Line / light type " + std::to_string(code) + " has no WED name; it was left off");
				}
			}

			/* @brief apt.dat nodes as WED airport nodes: co-located runs that encode a split bezier merge back into one node (WED's import). */
			void AptChain(const AptNodeChain &chain, const int parent)
			{
				const std::vector<AptNode> &path = chain.nodes;
				size_t index = 0;
				for (size_t cur = 0; cur < path.size(); ++cur)
				{
					const bool hasLo = path[cur].hasControl;
					const double loLat = hasLo ? path[cur].latitude - path[cur].controlLatitude : 0.0;	// Offset of recip(pt, ctrl)
					const double loLon = hasLo ? path[cur].longitude - path[cur].controlLongitude : 0.0;

					const size_t original = cur;
					size_t next = cur + 1;
					size_t previous = cur;
					while (next < path.size() && path[next].latitude == path[cur].latitude && path[next].longitude == path[cur].longitude &&
						(!path[previous].hasControl || !path[next].hasControl) && (path[previous].hasControl || path[next].hasControl))
					{
						previous = next;
						++next;
					}
					cur = next - 1;

					const AptNode &node = path[cur];
					GeoNode geo;
					geo.latitude = node.latitude;
					geo.longitude = node.longitude;
					if (hasLo)
					{
						geo.loLatitude = loLat;
						geo.loLongitude = loLon;
					}
					if (node.hasControl)
					{
						geo.hiLatitude = node.controlLatitude - node.latitude;
						geo.hiLongitude = node.controlLongitude - node.longitude;
					}
					geo.split = hasLo != node.hasControl || original != cur;

					const int id = Add("WED_AirportNode", parent, "Node " + std::to_string(++index));
					BezierPoint(id, geo);
					Markings(id, node.lineType, node.lightType);
				}
			}

			int AirportChainObject(const AptNodeChain &chain, const int parent, const std::string &name)
			{
				const int id = Add("WED_AirportChain", parent, name);
				AptChain(chain, id);
				Element(id, "airport_chain").Set("closed", chain.closed ? "1" : "0");
				return id;
			}

			static std::vector<std::string> SplitZone(const std::string &runways)
			{
				return SplitList(runways, ',');
			}

			void ExportAirport(const SceneryItem &item, const int parent)
			{
				const ProjectAirport *project = m_Project.FindAirport(item.airportId);
				if (!project)
				{
					Warn("An airport item names a missing airport and was skipped");
					return;
				}
				const AptAirport &airport = project->airport;
				const int id = Add("WED_Airport", parent, airport.name.empty() ? airport.id : airport.name, Hierarchy::Full, item.locked, item.hidden);
				if (m_FirstAirport == 0)
					m_FirstAirport = id;

				const bool flatten = airport.GetMetadata("flatten") == "1";
				const bool driveOnLeft = airport.GetMetadata("drive_on_left") == "1";
				Element(id, "airport")
					.Set("kind", NameOf(AIRPORT_KINDS, static_cast<int>(airport.type)) ? NameOf(AIRPORT_KINDS, static_cast<int>(airport.type)) : "Airport")
					.Set("elevation", FormatDouble(airport.elevationFt * 0.3048, 1))
					.Set("has_atc", project->hasAtc ? "1" : "0")
					.Set("icao", airport.id)
					.Set("always_flatten", flatten ? "1" : "0")
					.Set("drive_on_left", driveOnLeft ? "1" : "0")
					.Set("scenery_id", FormatInt(project->sceneryId));
				XmlElement &meta = Element(id, "meta_data");
				for (const AptMetadata &entry : airport.metadata)
				{
					if ((entry.key == "flatten" && flatten) || (entry.key == "drive_on_left" && driveOnLeft))
						continue;	// Written as the airport's flags
					meta.AddChild("meta_data_entry").Set("pair", entry.key + "," + entry.value);
				}

				for (const AptRunway &runway : airport.runways)
				{
					const int r = Add("WED_Runway", id, runway.GetName());
					for (int end = 0; end < 2; ++end)
					{
						const int node = Add("WED_RunwayNode", r, end == 0 ? "Start" : "End");
						Point(node, runway.ends[end].latitude, runway.ends[end].longitude);
					}
					Element(r, "line").Set("width", FormatFloat(runway.width, 2));
					XmlElement &attrs = Element(r, "runway");
					const int shoulder = runway.GetShoulderSurface();
					attrs.Set("surface", SurfaceName(runway.surface))
						.Set("shoulder", shoulder == 0 ? std::string("None") : SurfaceName(shoulder))
						.Set("shoulder_width", FormatFloat(runway.GetShoulderWidth(), 1))
						.Set("roughness", FormatFloat(runway.smoothness, 2))
						.Set("center_lites", runway.centerlineLights ? "1" : "0")
						.Set("edge_lites", Named(EDGE_LIGHTS, runway.edgeLights, "None", "edge lights"))
						.Set("distance_signs", runway.distanceSigns ? "1" : "0");
					for (int end = 0; end < 2; ++end)
					{
						const AptRunwayEnd &e = runway.ends[end];
						const std::string n = std::to_string(end + 1);
						attrs.Set("displaced" + n, FormatFloat(e.displacedThreshold, 1))
							.Set("blastpad" + n, FormatFloat(e.blastPad, 1))
							.Set("markings" + n, Named(RUNWAY_MARKINGS, e.marking, "None", "runway marking"))
							.Set("app_lites" + n, Named(APPROACH_LIGHTS, e.approachLights, "None", "approach lights"))
							.Set("TDZL" + n, e.touchdownZoneLights ? "1" : "0")
							.Set("REIL" + n, Named(REIL_LIGHTS, e.reil, "None", "REIL"));
					}
				}

				for (const AptWaterRunway &runway : airport.waterRunways)
				{
					const int r = Add("WED_Sealane", id, runway.GetName());
					for (int end = 0; end < 2; ++end)
					{
						const int node = Add("WED_RunwayNode", r, end == 0 ? "Start" : "End");
						Point(node, runway.ends[end].latitude, runway.ends[end].longitude);
					}
					Element(r, "line").Set("width", FormatFloat(runway.width, 2));
					Element(r, "sealane").Set("has_buoys", runway.buoys ? "1" : "0");
				}

				for (const AptHelipad &helipad : airport.helipads)
				{
					const int h = Add("WED_Helipad", id, helipad.designator);
					PointHeading(h, helipad.latitude, helipad.longitude, helipad.heading);
					Element(h, "point").Set("width", FormatFloat(helipad.width, 1)).Set("length", FormatFloat(helipad.length, 1));
					Element(h, "helipad")
						.Set("surface", SurfaceName(helipad.surface))
						.Set("markings", Named(HELIPAD_MARKINGS, helipad.markings, "Default", "helipad marking"))
						.Set("shoulder", helipad.shoulder == 0 ? std::string("None") : SurfaceName(helipad.shoulder == 2 ? SurfaceConcrete : SurfaceAsphalt))
						.Set("roughness", FormatFloat(helipad.smoothness, 2))
						.Set("lights", Named(HELIPAD_LIGHTS, helipad.edgeLights, "None", "helipad lights"));
				}

				for (size_t i = 0; i < airport.pavements.size(); ++i)
				{
					const AptPavement &pavement = airport.pavements[i];
					const std::string name = pavement.description.empty() ? "Taxiway " + std::to_string(i + 1) : pavement.description;
					const int t = Add("WED_Taxiway", id, name);
					for (size_t r = 0; r < pavement.rings.size(); ++r)
						AirportChainObject(pavement.rings[r], t, name + (r == 0 ? " Outer Ring" : " Hole " + std::to_string(r)));
					Element(t, "taxiway")
						.Set("surface", SurfaceName(pavement.surface))
						.Set("roughness", FormatFloat(pavement.smoothness, 2))
						.Set("heading", FormatFloat(pavement.textureHeading, 2));
				}

				for (size_t i = 0; i < airport.linearFeatures.size(); ++i)
				{
					const AptLinearFeature &feature = airport.linearFeatures[i];
					const std::string name = feature.description.empty() ? "Linear Feature " + std::to_string(i + 1) : feature.description;
					for (const AptNodeChain &chain : feature.chains)
						AirportChainObject(chain, id, name);
				}

				for (size_t i = 0; i < airport.boundaries.size(); ++i)
				{
					const AptBoundary &boundary = airport.boundaries[i];
					const std::string name = boundary.description.empty() ? "Airport Boundary " + std::to_string(i + 1) : boundary.description;
					const int b = Add("WED_AirportBoundary", id, name);
					for (size_t r = 0; r < boundary.rings.size(); ++r)
						AirportChainObject(boundary.rings[r], b, name + (r == 0 ? " Outer Ring" : " Hole " + std::to_string(r)));
				}

				if (airport.viewpoint)
				{
					const int v = Add("WED_TowerViewpoint", id, airport.viewpoint->name);
					Point(v, airport.viewpoint->latitude, airport.viewpoint->longitude);
					Element(v, "tower_viewpoint").Set("height", FormatDouble(airport.viewpoint->heightFt * 0.3048, 1));
				}
				if (airport.beacon)
				{
					const int b = Add("WED_AirportBeacon", id, airport.beacon->name);
					Point(b, airport.beacon->latitude, airport.beacon->longitude);
					if (airport.beacon->type == 0)
						Warn("A beacon of type none was written as an airport beacon (WED has no 'none' beacon)");
					Element(b, "airport_beacon").Set("type", Named(BEACONS, airport.beacon->type, "Airport", "beacon type"));
				}
				for (const AptWindsock &windsock : airport.windsocks)
				{
					const int w = Add("WED_Windsock", id, windsock.name.empty() ? std::string("Windsock") : windsock.name);
					Point(w, windsock.latitude, windsock.longitude);
					Element(w, "windsock").Set("lit", windsock.lit ? "1" : "0");
				}
				for (const AptSign &sign : airport.signs)
				{
					const int s = Add("WED_AirportSign", id, sign.text);
					PointHeading(s, sign.latitude, sign.longitude, sign.heading);
					Element(s, "airport_sign").Set("style", "Default").Set("size", Named(SIGN_SIZES, sign.size, "Medium Taxiway", "sign size"));
				}
				for (const AptLightFixture &fixture : airport.lightFixtures)
				{
					const int f = Add("WED_LightFixture", id, fixture.name.empty() ? std::string("Light Fixture") : fixture.name);
					PointHeading(f, fixture.latitude, fixture.longitude, fixture.heading);
					Element(f, "light_fixture").Set("type", Named(LIGHT_FIXTURES, fixture.type, "VASI", "light fixture")).Set("angle", FormatFloat(fixture.glideslope, 2));
				}
				for (const AptStartup &startup : airport.startups)
					RampStart(id, startup);
				if (!airport.legacyStartups.empty())
				{
					Warn("Legacy startup locations (row 15) were written as ramp starts");
					for (const AptLegacyStartup &legacy : airport.legacyStartups)
					{
						AptStartup startup;
						startup.latitude = legacy.latitude;
						startup.longitude = legacy.longitude;
						startup.heading = legacy.heading;
						startup.name = legacy.name;
						startup.type = "misc";
						RampStart(id, startup);
					}
				}
				for (const AptTruckParking &parking : airport.truckParkings)
				{
					const int p = Add("WED_TruckParkingLocation", id, parking.name);
					PointHeading(p, parking.latitude, parking.longitude, parking.heading);
					const char *type = WedText(TRUCKS, parking.type);
					if (!type)
						Warn("Service truck type '" + parking.type + "' has no WED name; Ground Power Unit was used");
					Element(p, "truck_parking_spot")
						.Set("type", type ? type : "Ground Power Unit")
						.Set("number_of_cars", FormatInt(parking.count))
						.Set("custom_veh", parking.customObject);
				}
				for (const AptTruckDestination &destination : airport.truckDestinations)
				{
					const int d = Add("WED_TruckDestination", id, destination.name);
					PointHeading(d, destination.latitude, destination.longitude, destination.heading);
					XmlElement &types = Element(d, "truck_destination");
					for (const std::string &type : SplitList(destination.truckTypes, '|'))
					{
						if (const char *name = WedText(TRUCKS, type))
							types.AddChild("types").Set("value", name);
					}
				}
				for (const AptFrequency &frequency : airport.frequencies)
				{
					const int f = Add("WED_ATCFrequency", id, frequency.name);
					Element(f, "atc_frequency")
						.Set("kind", Named(FREQUENCIES, static_cast<int>(frequency.type), "Tower", "frequency type"))
						.Set("freq", FormatDouble(frequency.frequencyKhz / 1000.0, 3));
				}
				for (const AptFlow &flow : airport.flows)
					Flow(id, flow);
				if (airport.taxiNetwork)
					TaxiNetwork(id, *airport.taxiNetwork);

				if (!airport.jetways.empty())
					Warn(airport.id + ": jetways (1500) have no WED class and were left out of earth.wed.xml (the project keeps them)");
				if (!airport.unknownRows.empty())
					Warn(airport.id + ": apt.dat rows WED can't hold were left out of earth.wed.xml (the project keeps them)");

				for (const SceneryItem &child : item.children)
					ExportItem(child, id);
			}

			template<size_t N>
			std::string Named(const NamedCode (&table)[N], const int code, const char *fallback, const char *what)
			{
				if (const char *name = NameOf(table, code))
					return name;
				Warn(std::string("No WED name for ") + what + " " + std::to_string(code) + "; '" + fallback + "' was used");
				return fallback;
			}

			std::string SurfaceName(const int code)
			{
				return Named(SURFACES, code, "Concrete", "surface");
			}

			void RampStart(const int parent, const AptStartup &startup)
			{
				const int r = Add("WED_RampPosition", parent, startup.name);
				PointHeading(r, startup.latitude, startup.longitude, startup.heading);
				const char *type = WedText(RAMP_TYPES, startup.type);
				const char *operation = WedText(RAMP_OPERATIONS, startup.operationType);
				Element(r, "ramp_start")
					.Set("type", type ? type : "Misc")
					.Set("traffic", FormatInt(AircraftTypeBits(startup.aircraftTypes)))
					.Set("width", startup.widthCode.empty() ? std::string("C") : startup.widthCode)
					.Set("ramp_op_type", operation ? operation : "None")
					.Set("airlines", startup.airlines);
			}

			void Flow(const int parent, const AptFlow &flow)
			{
				const int f = Add("WED_ATCFlow", parent, flow.name);
				std::string icao;
				for (const AptCeilingRule &rule : flow.ceilingRules)
					icao = icao.empty() ? rule.station : icao;
				for (const AptVisibilityRule &rule : flow.visibilityRules)
					icao = icao.empty() ? rule.station : icao;
				if (flow.ceilingRules.size() > 1 || flow.visibilityRules.size() > 1 || flow.vfrPatterns.size() > 1)
					Warn("WED keeps one ceiling, visibility and pattern rule per flow; the extra ones were left out");
				Element(f, "atc_flow")
					.Set("icao", icao)
					.Set("cld_min", FormatInt(flow.ceilingRules.empty() ? 0 : flow.ceilingRules.front().minCeilingFt))
					.Set("vis_min", FormatFloat(flow.visibilityRules.empty() ? 0.0f : flow.visibilityRules.front().minVisibilitySm, 1))
					.Set("pattern_side", !flow.vfrPatterns.empty() && flow.vfrPatterns.front().rightTraffic ? "Right" : "Left")
					.Set("pattern_rwy", flow.vfrPatterns.empty() ? std::string("None") : flow.vfrPatterns.front().runway);

				for (const AptWindRule &rule : flow.windRules)
				{
					const int w = Add("WED_ATCWindRule", f, "Wind Rule");
					Element(w, "atc_windrule")
						.Set("weather_icao", rule.station)
						.Set("dir_lo_degs_mag", FormatInt(rule.minDirection))
						.Set("dir_hi_degs_mag", FormatInt(rule.maxDirection))
						.Set("speed_knots", FormatInt(rule.maxSpeedKnots));
				}
				for (const AptTimeRule &rule : flow.timeRules)
				{
					const int t = Add("WED_ATCTimeRule", f, "Time Rule");
					Element(t, "atc_timerule").Set("start_zulu", FormatInt(rule.startUtc)).Set("end_zulu", FormatInt(rule.endUtc));
				}
				for (const AptRunwayUse &use : flow.runwayUses)
				{
					const int u = Add("WED_ATCRunwayUse", f, use.name.empty() ? use.runway : use.name);
					Element(u, "runway_use")
						.Set("rwy", use.runway)
						.Set("dep_frq", FormatDouble(use.frequencyKhz / 1000.0, 3))
						.Set("traffic", FormatInt(AircraftTypeBits(use.aircraftTypes)))
						.Set("operations", FormatInt((use.arrivals ? 1 : 0) | (use.departures ? 2 : 0)))
						.Set("dep_min", FormatInt(use.onCourseFrom))
						.Set("dep_max", FormatInt(use.onCourseTo))
						.Set("ini_min", FormatInt(use.departureFrom))
						.Set("ini_max", FormatInt(use.departureTo));
				}
			}

			void TaxiNetwork(const int parent, const AptTaxiNetwork &network)
			{
				std::unordered_map<int, int> nodes;		// apt.dat node id -> WED id
				for (const AptTaxiNode &node : network.nodes)
				{
					const int n = Add("WED_TaxiRouteNode", parent, node.name);
					Point(n, node.latitude, node.longitude);
					nodes[node.id] = n;
				}
				for (const AptTaxiEdge &edge : network.edges)
				{
					if (!nodes.contains(edge.from) || !nodes.contains(edge.to))
					{
						Warn("A taxi edge to an unknown node was left out");
						continue;
					}
					const int e = Add("WED_TaxiRoute", parent, edge.name);
					OutObject &object = Get(e);
					object.sources = { nodes[edge.from], nodes[edge.to] };
					Get(nodes[edge.from]).viewers.push_back(e);
					if (edge.to != edge.from)
						Get(nodes[edge.to]).viewers.push_back(e);

					Element(e, "edge").Set("ctrl_latitude_lo", "0.0").Set("ctrl_longitude_lo", "0.0").Set("ctrl_latitude_hi", "0.0").Set("ctrl_longitude_hi", "0.0");
					const bool runway = edge.restriction == "runway";
					std::string width = "E";
					if (edge.restriction.rfind("taxiway_", 0) == 0 && edge.restriction.size() > 8)
						width = edge.restriction.substr(8);
					Element(e, "taxi_route")
						.Set("vehicle_class", edge.groundVehicles ? "Ground Trucks" : "Aircraft")
						.Set("oneway", edge.oneWay ? "1" : "0")
						.Set("runway", runway && !edge.groundVehicles ? edge.name : std::string("None"))
						.Set("width", width);

					// Built apart: adding elements to the object one by one would move the earlier ones
					XmlElement zones[3];
					zones[0].name = "departures";
					zones[1].name = "arrivals";
					zones[2].name = "ils_holds";
					for (const AptActiveZone &zone : edge.activeZones)
					{
						const int list = zone.classification == "departure" ? 0 : zone.classification == "arrival" ? 1 : zone.classification == "ils" ? 2 : -1;
						if (list < 0)
							continue;
						for (const std::string &name : SplitZone(zone.runways))
							zones[list].AddChild("runway").Set("value", name);
					}
					for (XmlElement &list : zones)
						Get(e).elements.push_back(std::move(list));
				}
			}

			const SceneryProject &m_Project;
			std::vector<std::string> *m_Warnings;
			const LineNames &m_Lines;
			std::vector<OutObject> m_Objects;			// Index = id - 1
			std::unordered_map<int, int> m_RawIds;		// Original WED id of kept raw objects -> new id
			int m_FirstAirport = 0;
			std::map<std::string, size_t> m_Repeats;
		};
	}

	// -------------------------------------------------------

	std::string MakeWedLineName(const std::string_view stem, int *code)
	{
		// sscanf("%d%*c%29s") as WED's RescanLines does: the number, one separator, then the name
		size_t i = 0;
		int number = 0;
		bool digits = false;
		while (i < stem.size() && stem[i] >= '0' && stem[i] <= '9')
		{
			number = number * 10 + (stem[i] - '0');
			digits = true;
			++i;
		}
		if (code)
			*code = digits ? number : 0;
		if (i < stem.size())
			++i;
		std::string name(stem.substr(i, 29));

		std::string out;
		for (size_t k = 0; k < name.size(); ++k)
		{
			char c = name[k];
			if (k == 0 && c >= 'a' && c <= 'z')
				c = static_cast<char>(c - 'a' + 'A');
			if (c != '_')
			{
				out.push_back(c);
				continue;
			}
			out.push_back(' ');
			const std::string_view rest = std::string_view(name).substr(k + 1);
			if (rest == "G_uni")
			{
				out.append("(Unidirectional Green)");
				break;
			}
			if (rest == "YG_uni")
			{
				out.append("(Unidirectional Amber/Green)");
				break;
			}
			if (rest.empty())
				break;
			if (rest.size() == 1)
			{
				out.append("(Black)");
				break;
			}
			char next = rest[0];
			if (next >= 'a' && next <= 'z')
				next = static_cast<char>(next - 'a' + 'A');
			out.push_back(next);
			++k;
		}
		return out;
	}

	bool ParseWed(const std::string_view xml, SceneryProject &out, std::vector<std::string> *warnings, std::string *error, const WedImportOptions &options)
	{
		out = SceneryProject{};
		XmlElement doc;
		std::string xmlError;
		if (!ParseXml(xml, doc, &xmlError))
		{
			if (error)
				*error = "The WorldEditor file isn't valid XML (" + xmlError + ")";
			return false;
		}

		const LineNames lines(options.extraLineNames);
		Importer importer(out, warnings, lines);
		if (!importer.Run(doc, error))
		{
			out = SceneryProject{};
			return false;
		}

		out.info.uuid = MakeUuid();
		out.info.created = GetUtcTimestamp();
		out.info.modified = out.info.created;
		const std::vector<std::string> changes = NormalizeProject(out);
		if (warnings)
			warnings->insert(warnings->end(), changes.begin(), changes.end());
		return true;
	}

	std::string WriteWed(const SceneryProject &project, std::vector<std::string> *warnings, const WedExportOptions &options)
	{
		const LineNames lines(options.extraLineNames);
		Exporter exporter(project, warnings, lines);
		return exporter.Run(options.writer);
	}
}

// -------------------------------------------------------
