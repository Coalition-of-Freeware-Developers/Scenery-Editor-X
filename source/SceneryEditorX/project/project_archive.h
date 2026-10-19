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
 * project_archive.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "scenery_project.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/**
	 * One Visit() per data type lists its fields once; every archive walks the same list:
	 *
	 *  - BinaryWriter / BinaryReader below (.sex chunks, the edit cache)
	 *  - the JSON writer / reader in project_edx.cpp (.edX)
	 *
	 * So a field can't be saved by one format and forgotten by the other. Archives provide:
	 *
	 *  Field(key, value)			every format, always
	 *  FieldIf(key, value, def)	JSON leaves it out when it equals def (binary always writes it)
	 *  Inline(key, struct)			JSON merges the struct's keys into the current object, binary nests it
	 *  Chain(key, rings)			a single node chain: JSON writes rings[0] as one array
	 *  Exclusions / Seasons / Metadata / Properties	values with a friendlier JSON spelling
	 *
	 * Binary compatibility rule: append new fields at the END of a Visit(). Every struct is stored with its size,
	 * so older readers skip the fields they don't know and newer readers keep defaults for fields an older file lacks.
	 */

	// -------------------------------------------------------
	// Field lists

	template<typename A> void Visit(A &a, ProjectInfo &v)
	{
		a.Field("name", v.name);
		a.Field("uuid", v.uuid);
		a.Field("kind", v.kind);
		a.Field("format-version", v.formatVersion);
		a.Field("editor-version", v.editorVersion);
		a.Field("XPlaneVersion", v.xplaneVersion);
		a.Field("created", v.created);
		a.Field("editdate", v.modified);
		a.Field("author", v.author);
		a.Field("description", v.description);
		a.Field("package", v.packageName);
	}

	template<typename A> void Visit(A &a, GeoReference &v)
	{
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("elevation", v.elevation);
		a.FieldIf("valid", v.valid, true);
	}

	template<typename A> void Visit(A &a, LibraryReference &v)
	{
		a.Field("Library", v.name);
		a.Field("local-path", v.localPath);
		a.Field("entry-count", v.entryCount);
		a.Field("uuid", v.uuid);
		a.Field("short-id", v.shortId);
	}

	template<typename A> void Visit(A &a, GeoNode &v)
	{
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("lo-latitude", v.loLatitude);
		a.Field("lo-longitude", v.loLongitude);
		a.Field("hi-latitude", v.hiLatitude);
		a.Field("hi-longitude", v.hiLongitude);
		a.Field("split", v.split);
		a.Field("has-uv", v.hasUv);
		a.Field("s", v.s);
		a.Field("t", v.t);
		a.Field("s-lo", v.sLo);
		a.Field("t-lo", v.tLo);
		a.Field("s-hi", v.sHi);
		a.Field("t-hi", v.tHi);
		a.Field("wall", v.wallType);
		a.Field("spawn-tiles", v.spawnTiles);
	}

	template<typename A> void Visit(A &a, SceneryItem &v)
	{
		a.Field("type", v.kind);
		a.Field("id", v.id);
		a.Field("name", v.name);
		a.FieldIf("locked", v.locked, false);
		a.FieldIf("hidden", v.hidden, false);

		if (HasResource(v.kind))
		{
			a.Field("resource", v.resource);
			a.FieldIf("associated-library", v.library, std::string());
		}

		switch (v.kind)
		{
			case ItemKind::Group:
				break;
			case ItemKind::Airport:
				a.Field("airport", v.airportId);
				break;
			case ItemKind::Object:
				a.Field("latitude", v.latitude);
				a.Field("longitude", v.longitude);
				a.Field("altitude", v.altitude);
				a.Field("heading", v.heading);
				a.Field("elevation-mode", v.elevationMode);
				a.Field("show-level", v.showLevel);
				break;
			case ItemKind::Polygon:
				a.Field("texture-heading", v.textureHeading);
				a.Field("rings", v.rings);
				break;
			case ItemKind::Orthophoto:
				a.Field("texture-heading", v.textureHeading);
				a.FieldIf("decal", v.decal, std::string());
				a.Field("tex-top", v.texTop);
				a.Field("tex-bottom", v.texBottom);
				a.Field("tex-left", v.texLeft);
				a.Field("tex-right", v.texRight);
				a.Field("rings", v.rings);
				break;
			case ItemKind::Line:
				a.Field("closed", v.closed);
				a.Chain("nodes", v.rings);
				break;
			case ItemKind::String:
				a.Field("closed", v.closed);
				a.Field("spacing", v.spacing);
				a.Chain("nodes", v.rings);
				break;
			case ItemKind::Forest:
				a.Field("density", v.density);
				a.Field("fill", v.fill);
				a.Field("rings", v.rings);
				break;
			case ItemKind::Facade:
				a.Field("height", v.height);
				a.Field("pick-walls", v.pickWalls);
				a.Field("show-level", v.showLevel);
				a.Field("rings", v.rings);
				break;
			case ItemKind::Autogen:
				a.Field("height", v.height);
				a.Field("spelling", v.spelling);
				a.Field("rings", v.rings);
				break;
			case ItemKind::ExclusionZone:
				a.Exclusions("exclude", v.exclusions);
				a.Chain("nodes", v.rings);
				break;
			case ItemKind::ExclusionPolygon:
				a.Exclusions("exclude", v.exclusions);
				a.Field("rings", v.rings);
				break;
			case ItemKind::Raw:
				a.Field("xml", v.raw);
				break;
		}

		a.Properties("other-properties", v.properties);
		if (v.IsContainer())
			a.Field("children", v.children);
	}

	// apt.dat 12.00 rows (XPlane::AptAirport)

	template<typename A> void Visit(A &a, XPlane::AptRunwayEnd &v)
	{
		a.Field("designator", v.designator);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("displaced-threshold", v.displacedThreshold);
		a.Field("blast-pad", v.blastPad);
		a.Field("marking", v.marking);
		a.Field("approach-lights", v.approachLights);
		a.Field("tdz-lights", v.touchdownZoneLights);
		a.Field("reil", v.reil);
	}

	template<typename A> void Visit(A &a, XPlane::AptRunway &v)
	{
		a.Field("width", v.width);
		a.Field("surface", v.surface);
		a.Field("shoulder", v.shoulder);
		a.Field("smoothness", v.smoothness);
		a.Field("centerline-lights", v.centerlineLights);
		a.Field("edge-lights", v.edgeLights);
		a.Field("distance-signs", v.distanceSigns);
		a.Field("ends", v.ends);
	}

	template<typename A> void Visit(A &a, XPlane::AptWaterRunway::End &v)
	{
		a.Field("designator", v.designator);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
	}

	template<typename A> void Visit(A &a, XPlane::AptWaterRunway &v)
	{
		a.Field("width", v.width);
		a.Field("buoys", v.buoys);
		a.Field("ends", v.ends);
	}

	template<typename A> void Visit(A &a, XPlane::AptHelipad &v)
	{
		a.Field("designator", v.designator);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("heading", v.heading);
		a.Field("length", v.length);
		a.Field("width", v.width);
		a.Field("surface", v.surface);
		a.Field("markings", v.markings);
		a.Field("shoulder", v.shoulder);
		a.Field("smoothness", v.smoothness);
		a.Field("edge-lights", v.edgeLights);
	}

	template<typename A> void Visit(A &a, XPlane::AptNode &v)
	{
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("has-control", v.hasControl);
		a.Field("control-latitude", v.controlLatitude);
		a.Field("control-longitude", v.controlLongitude);
		a.Field("line", v.lineType);
		a.Field("light", v.lightType);
	}

	template<typename A> void Visit(A &a, XPlane::AptNodeChain &v)
	{
		a.Field("closed", v.closed);
		a.Field("nodes", v.nodes);
	}

	template<typename A> void Visit(A &a, XPlane::AptPavement &v)
	{
		a.Field("description", v.description);
		a.Field("surface", v.surface);
		a.Field("smoothness", v.smoothness);
		a.Field("texture-heading", v.textureHeading);
		a.Field("rings", v.rings);
	}

	template<typename A> void Visit(A &a, XPlane::AptLinearFeature &v)
	{
		a.Field("description", v.description);
		a.Field("chains", v.chains);
	}

	template<typename A> void Visit(A &a, XPlane::AptBoundary &v)
	{
		a.Field("description", v.description);
		a.Field("rings", v.rings);
	}

	template<typename A> void Visit(A &a, XPlane::AptViewpoint &v)
	{
		a.Field("name", v.name);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("height-ft", v.heightFt);
	}

	template<typename A> void Visit(A &a, XPlane::AptLegacyStartup &v)
	{
		a.Field("name", v.name);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("heading", v.heading);
	}

	template<typename A> void Visit(A &a, XPlane::AptBeacon &v)
	{
		a.Field("name", v.name);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("type", v.type);
	}

	template<typename A> void Visit(A &a, XPlane::AptWindsock &v)
	{
		a.Field("name", v.name);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("lit", v.lit);
	}

	template<typename A> void Visit(A &a, XPlane::AptSign &v)
	{
		a.Field("text", v.text);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("heading", v.heading);
		a.Field("size", v.size);
	}

	template<typename A> void Visit(A &a, XPlane::AptLightFixture &v)
	{
		a.Field("name", v.name);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("type", v.type);
		a.Field("heading", v.heading);
		a.Field("glideslope", v.glideslope);
	}

	template<typename A> void Visit(A &a, XPlane::AptFrequency &v)
	{
		a.Field("name", v.name);
		a.Field("type", v.type);
		a.Field("frequency-khz", v.frequencyKhz);
		a.FieldIf("legacy", v.legacy, false);
	}

	template<typename A> void Visit(A &a, XPlane::AptWindRule &v)
	{
		a.Field("station", v.station);
		a.Field("min-direction", v.minDirection);
		a.Field("max-direction", v.maxDirection);
		a.Field("max-speed-knots", v.maxSpeedKnots);
	}

	template<typename A> void Visit(A &a, XPlane::AptCeilingRule &v)
	{
		a.Field("station", v.station);
		a.Field("min-ceiling-ft", v.minCeilingFt);
	}

	template<typename A> void Visit(A &a, XPlane::AptVisibilityRule &v)
	{
		a.Field("station", v.station);
		a.Field("min-visibility-sm", v.minVisibilitySm);
	}

	template<typename A> void Visit(A &a, XPlane::AptTimeRule &v)
	{
		a.Field("start-utc", v.startUtc);
		a.Field("end-utc", v.endUtc);
	}

	template<typename A> void Visit(A &a, XPlane::AptRunwayUse &v)
	{
		a.Field("runway", v.runway);
		a.Field("frequency-khz", v.frequencyKhz);
		a.Field("arrivals", v.arrivals);
		a.Field("departures", v.departures);
		a.Field("aircraft-types", v.aircraftTypes);
		a.Field("on-course-from", v.onCourseFrom);
		a.Field("on-course-to", v.onCourseTo);
		a.Field("departure-from", v.departureFrom);
		a.Field("departure-to", v.departureTo);
		a.Field("name", v.name);
		a.Field("high-resolution", v.highResolution);
	}

	template<typename A> void Visit(A &a, XPlane::AptVfrPattern &v)
	{
		a.Field("runway", v.runway);
		a.Field("right-traffic", v.rightTraffic);
	}

	template<typename A> void Visit(A &a, XPlane::AptFlow &v)
	{
		a.Field("name", v.name);
		a.Field("wind-rules", v.windRules);
		a.Field("ceiling-rules", v.ceilingRules);
		a.Field("visibility-rules", v.visibilityRules);
		a.Field("time-rules", v.timeRules);
		a.Field("runway-uses", v.runwayUses);
		a.Field("vfr-patterns", v.vfrPatterns);
	}

	template<typename A> void Visit(A &a, XPlane::AptTaxiNode &v)
	{
		a.Field("id", v.id);
		a.Field("name", v.name);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("usage", v.usage);
	}

	template<typename A> void Visit(A &a, XPlane::AptActiveZone &v)
	{
		a.Field("classification", v.classification);
		a.Field("runways", v.runways);
	}

	template<typename A> void Visit(A &a, XPlane::AptTaxiEdge &v)
	{
		a.Field("from", v.from);
		a.Field("to", v.to);
		a.Field("one-way", v.oneWay);
		a.Field("restriction", v.restriction);
		a.Field("name", v.name);
		a.FieldIf("ground-vehicles", v.groundVehicles, false);
		a.Field("active-zones", v.activeZones);
	}

	template<typename A> void Visit(A &a, XPlane::AptTaxiNetwork &v)
	{
		a.Field("nodes", v.nodes);
		a.Field("edges", v.edges);
	}

	template<typename A> void Visit(A &a, XPlane::AptStartup &v)
	{
		a.Field("name", v.name);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("heading", v.heading);
		a.Field("type", v.type);
		a.Field("aircraft-types", v.aircraftTypes);
		a.Field("has-metadata", v.hasMetadata);
		a.Field("width-code", v.widthCode);
		a.Field("operation-type", v.operationType);
		a.Field("airlines", v.airlines);
	}

	template<typename A> void Visit(A &a, XPlane::AptMetadata &v)
	{
		a.Field("key", v.key);
		a.Field("value", v.value);
	}

	template<typename A> void Visit(A &a, XPlane::AptTruckParking &v)
	{
		a.Field("name", v.name);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("heading", v.heading);
		a.Field("type", v.type);
		a.Field("count", v.count);
		a.FieldIf("custom-object", v.customObject, std::string());
	}

	template<typename A> void Visit(A &a, XPlane::AptTruckDestination &v)
	{
		a.Field("name", v.name);
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("heading", v.heading);
		a.Field("truck-types", v.truckTypes);
	}

	template<typename A> void Visit(A &a, XPlane::AptJetway &v)
	{
		a.Field("latitude", v.latitude);
		a.Field("longitude", v.longitude);
		a.Field("heading", v.heading);
		a.Field("style", v.style);
		a.Field("size", v.size);
		a.Field("reserved", v.reserved);
		a.Field("tunnel-length", v.tunnelLength);
		a.Field("cabin-heading", v.cabinHeading);
		a.Field("custom-objects", v.customObjects);
	}

	template<typename A> void Visit(A &a, XPlane::AptAirport &v)
	{
		a.Field("ident", v.id);
		a.Field("name", v.name);
		a.Field("kind", v.type);
		a.Field("elevation-ft", v.elevationFt);
		a.Metadata("metadata", v.metadata);
		a.Field("runways", v.runways);
		a.Field("water-runways", v.waterRunways);
		a.Field("helipads", v.helipads);
		a.Field("pavements", v.pavements);
		a.Field("linear-features", v.linearFeatures);
		a.Field("boundaries", v.boundaries);
		a.Field("viewpoint", v.viewpoint);
		a.Field("legacy-startups", v.legacyStartups);
		a.Field("beacon", v.beacon);
		a.Field("windsocks", v.windsocks);
		a.Field("signs", v.signs);
		a.Field("light-fixtures", v.lightFixtures);
		a.Field("frequencies", v.frequencies);
		a.Field("flows", v.flows);
		a.Field("taxi-network", v.taxiNetwork);
		a.Field("startups", v.startups);
		a.Field("truck-parkings", v.truckParkings);
		a.Field("truck-destinations", v.truckDestinations);
		a.Field("jetways", v.jetways);
		a.Field("unknown-rows", v.unknownRows);
	}

	template<typename A> void Visit(A &a, ProjectAirport &v)
	{
		a.Inline("airport", v.airport);
		a.FieldIf("has-atc", v.hasAtc, true);
		a.FieldIf("scenery-id", v.sceneryId, -1);
		a.Properties("other-properties", v.properties);
	}

	// library.txt (XPlane::LibraryFile)

	template<typename A> void Visit(A &a, XPlane::LibraryRegion::Condition &v)
	{
		a.Field("dataref", v.dataref);
		a.Field("comparison", v.comparison);
		a.Field("value", v.value);
	}

	template<typename A> void Visit(A &a, XPlane::LibraryRegion &v)
	{
		a.Field("name", v.name);
		a.FieldIf("all", v.all, false);
		a.Field("rects", v.rects);
		a.FieldIf("bitmap", v.bitmap, std::string());
		a.Field("conditions", v.conditions);
	}

	template<typename A> void Visit(A &a, XPlane::LibraryExport &v)
	{
		a.Field("virtual", v.virtualPath);
		a.Field("file", v.realPath);
		a.FieldIf("kind", v.kind, XPlane::ExportKind::Export);
		a.FieldIf("ratio", v.ratio, 1.0f);
		a.Seasons("seasons", v.seasons);
		a.FieldIf("region", v.region, std::string());
		a.FieldIf("private", v.isPrivate, false);
		a.FieldIf("deprecated", v.deprecated, false);
		a.FieldIf("semi-deprecated", v.semiDeprecated, false);
		a.FieldIf("public-date", v.publicDate, 0u);
	}

	template<typename A> void Visit(A &a, XPlane::LibraryFile &v)
	{
		a.Field("version", v.version);
		a.Field("regions", v.regions);
		a.Field("exports", v.exports);
	}

	// -------------------------------------------------------
	// Binary archive

	namespace Detail
	{
		template<typename T> struct IsVector : std::false_type {};
		template<typename T, typename Alloc> struct IsVector<std::vector<T, Alloc>> : std::true_type {};

		template<typename T> struct IsOptional : std::false_type {};
		template<typename T> struct IsOptional<std::optional<T>> : std::true_type {};

		template<typename T> struct IsPair : std::false_type {};
		template<typename A, typename B> struct IsPair<std::pair<A, B>> : std::true_type {};

		template<typename T> struct IsStdArray : std::false_type {};
		template<typename T, size_t N> struct IsStdArray<std::array<T, N>> : std::true_type {};

		/* @brief Fewest bytes one element of T takes in the binary format (bounds vector counts read from a file). */
		template<typename T>
		constexpr size_t MinimumSize()
		{
			if constexpr (std::is_enum_v<T>)
				return 4;
			else if constexpr (std::is_same_v<T, bool>)
				return 1;
			else if constexpr (std::is_arithmetic_v<T>)
				return sizeof(T);
			else if constexpr (IsOptional<T>::value)
				return 1;
			else if constexpr (IsPair<T>::value)
				return MinimumSize<typename T::first_type>() + MinimumSize<typename T::second_type>();
			else if constexpr (IsStdArray<T>::value)
				return std::tuple_size_v<T> * MinimumSize<typename T::value_type>();
			else
				return 4;	// Strings, vectors and structs start with a 32-bit size
		}
	}

	/**
	 * @class BinaryWriter
	 * @brief Appends values little-endian: scalars at their size, enums as int32, strings / vectors as a uint32 count
	 * then the items, optionals as a presence byte, structs as a uint32 byte size then their Visit() fields.
	 */
	class BinaryWriter
	{
	public:
		explicit BinaryWriter(std::string &out) : m_Out(out) {}

		template<typename T> void Field(const char *, const T &value) { Write(value); }
		template<typename T> void FieldIf(const char *, const T &value, const T &) { Write(value); }
		template<typename T> void Inline(const char *, const T &value) { Write(value); }
		void Chain(const char *, const std::vector<GeoRing> &rings) { Write(rings); }
		void Exclusions(const char *, const uint32_t &bits) { Write(bits); }
		void Seasons(const char *, const uint8_t &seasons) { Write(seasons); }
		void Metadata(const char *, const std::vector<XPlane::AptMetadata> &metadata) { Write(metadata); }
		void Properties(const char *, const PropertyList &properties) { Write(properties); }

		template<typename T>
		void Write(const T &value)
		{
			if constexpr (std::is_enum_v<T>)
				WriteScalar(static_cast<int32_t>(value));
			else if constexpr (std::is_same_v<T, bool>)
				WriteScalar(static_cast<uint8_t>(value ? 1 : 0));
			else if constexpr (std::is_arithmetic_v<T>)
				WriteScalar(value);
			else if constexpr (std::is_same_v<T, std::string>)
			{
				WriteScalar(static_cast<uint32_t>(value.size()));
				m_Out.append(value);
			}
			else if constexpr (Detail::IsVector<T>::value)
			{
				WriteScalar(static_cast<uint32_t>(value.size()));
				for (const auto &item : value)
					Write(item);
			}
			else if constexpr (Detail::IsOptional<T>::value)
			{
				WriteScalar(static_cast<uint8_t>(value ? 1 : 0));
				if (value)
					Write(*value);
			}
			else if constexpr (Detail::IsPair<T>::value)
			{
				Write(value.first);
				Write(value.second);
			}
			else if constexpr (Detail::IsStdArray<T>::value || std::is_array_v<T>)
			{
				for (const auto &item : value)
					Write(item);
			}
			else
			{
				const size_t sizeAt = m_Out.size();
				WriteScalar(static_cast<uint32_t>(0));
				Visit(*this, const_cast<T &>(value));
				const uint32_t size = static_cast<uint32_t>(m_Out.size() - sizeAt - 4);
				PatchScalar(sizeAt, size);
			}
		}

		template<typename T>
		void WriteScalar(T value)
		{
			char bytes[sizeof(T)];
			std::memcpy(bytes, &value, sizeof(T));
			if constexpr (std::endian::native == std::endian::big)
				std::reverse(bytes, bytes + sizeof(T));
			m_Out.append(bytes, sizeof(T));
		}

		template<typename T>
		void PatchScalar(const size_t at, T value)
		{
			char bytes[sizeof(T)];
			std::memcpy(bytes, &value, sizeof(T));
			if constexpr (std::endian::native == std::endian::big)
				std::reverse(bytes, bytes + sizeof(T));
			std::memcpy(m_Out.data() + at, bytes, sizeof(T));
		}

	private:
		std::string &m_Out;
	};

	/**
	 * @class BinaryReader
	 * @brief Reads what BinaryWriter wrote. Every read is bounds checked: a truncated or corrupt buffer stops the
	 * reader with an error instead of reading past the end or allocating a bogus count.
	 */
	class BinaryReader
	{
	public:
		BinaryReader(const void *data, const size_t size) : m_Data(static_cast<const uint8_t *>(data)), m_End(size) {}

		[[nodiscard]] bool Ok() const { return m_Ok; }
		[[nodiscard]] const std::string &GetError() const { return m_Error; }
		[[nodiscard]] size_t GetPosition() const { return m_Position; }
		[[nodiscard]] size_t Remaining() const { return m_End - m_Position; }

		/* @brief Reads a field unless the enclosing struct has ended (an older file without it: the default stays). */
		template<typename T> void Field(const char *, T &value) { if (m_Position < m_End) Read(value); }
		template<typename T> void FieldIf(const char *key, T &value, const T &) { Field(key, value); }
		template<typename T> void Inline(const char *key, T &value) { Field(key, value); }
		void Chain(const char *key, std::vector<GeoRing> &rings) { Field(key, rings); }
		void Exclusions(const char *key, uint32_t &bits) { Field(key, bits); }
		void Seasons(const char *key, uint8_t &seasons) { Field(key, seasons); }
		void Metadata(const char *key, std::vector<XPlane::AptMetadata> &metadata) { Field(key, metadata); }
		void Properties(const char *key, PropertyList &properties) { Field(key, properties); }

		template<typename T>
		void Read(T &value)
		{
			if (!m_Ok)
				return;

			if constexpr (std::is_enum_v<T>)
			{
				int32_t raw = 0;
				ReadScalar(raw);
				value = static_cast<T>(raw);
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				uint8_t raw = 0;
				ReadScalar(raw);
				value = raw != 0;
			}
			else if constexpr (std::is_arithmetic_v<T>)
				ReadScalar(value);
			else if constexpr (std::is_same_v<T, std::string>)
			{
				uint32_t size = 0;
				ReadScalar(size);
				if (!m_Ok)
					return;
				if (size > Remaining())
					return Fail("string longer than its chunk");
				value.assign(reinterpret_cast<const char *>(m_Data + m_Position), size);
				m_Position += size;
			}
			else if constexpr (Detail::IsVector<T>::value)
			{
				uint32_t count = 0;
				ReadScalar(count);
				if (!m_Ok)
					return;
				if (static_cast<uint64_t>(count) * Detail::MinimumSize<typename T::value_type>() > Remaining())
					return Fail("item count larger than its chunk");
				value.clear();
				value.resize(count);
				for (auto &item : value)
				{
					Read(item);
					if (!m_Ok)
						return;
				}
			}
			else if constexpr (Detail::IsOptional<T>::value)
			{
				uint8_t present = 0;
				ReadScalar(present);
				if (!m_Ok)
					return;
				if (present)
				{
					value.emplace();
					Read(*value);
				}
				else
					value.reset();
			}
			else if constexpr (Detail::IsPair<T>::value)
			{
				Read(value.first);
				Read(value.second);
			}
			else if constexpr (Detail::IsStdArray<T>::value || std::is_array_v<T>)
			{
				for (auto &item : value)
					Read(item);
			}
			else
			{
				uint32_t size = 0;
				ReadScalar(size);
				if (!m_Ok)
					return;
				if (size > Remaining())
					return Fail("struct larger than its chunk");
				const size_t outerEnd = m_End;
				m_End = m_Position + size;
				Visit(*this, value);
				if (!m_Ok)
					return;
				m_Position = m_End;		// Skip fields a newer writer appended
				m_End = outerEnd;
			}
		}

		template<typename T>
		void ReadScalar(T &value)
		{
			if (!m_Ok)
				return;
			if (Remaining() < sizeof(T))
				return Fail("unexpected end of data");
			char bytes[sizeof(T)];
			std::memcpy(bytes, m_Data + m_Position, sizeof(T));
			if constexpr (std::endian::native == std::endian::big)
				std::reverse(bytes, bytes + sizeof(T));
			std::memcpy(&value, bytes, sizeof(T));
			m_Position += sizeof(T);
		}

		void Fail(const std::string &message)
		{
			if (m_Ok)
			{
				m_Ok = false;
				m_Error = message + " (offset " + std::to_string(m_Position) + ")";
			}
		}

	private:
		const uint8_t *m_Data = nullptr;
		size_t m_Position = 0;
		size_t m_End = 0;
		bool m_Ok = true;
		std::string m_Error;
	};
}

// -------------------------------------------------------
