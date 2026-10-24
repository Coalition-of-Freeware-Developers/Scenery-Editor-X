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
 * DsfWriterTests.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <cmath>
#include <cstring>
#include <limits>

using namespace SceneryEditorX::XPlane;
using Catch::Approx;

// -------------------------------------------------------

namespace
{
	/* @brief One degree of latitude in meters (what the precision checks talk in). */
	constexpr double METERS_PER_DEGREE = 111'320.0;

	std::string Hex(const std::array<uint8_t, 16> &digest)
	{
		std::string text;
		char pair[3];
		for (const uint8_t byte : digest)
		{
			std::snprintf(pair, sizeof(pair), "%02x", byte);
			text += pair;
		}
		return text;
	}

	std::array<uint8_t, 16> Md5Of(const std::string &text)
	{
		return Md5(reinterpret_cast<const uint8_t *>(text.data()), text.size());
	}

	DsfPoint Point(const double longitude, const double latitude, std::initializer_list<double> more = {})
	{
		DsfPoint point;
		point.v[0] = longitude;
		point.v[1] = latitude;
		uint8_t plane = 2;
		for (const double value : more)
			point.v[plane++] = value;
		point.planes = plane;
		return point;
	}

	DsfPolygonPlacement Polygon(const uint32_t definition, const uint16_t parameter, const uint8_t planes, std::vector<std::vector<DsfPoint>> windings)
	{
		DsfPolygonPlacement polygon;
		polygon.definition = definition;
		polygon.parameter = parameter;
		polygon.planes = planes;
		polygon.windings = std::move(windings);
		return polygon;
	}

	/* @brief A small airport overlay around 45.59 N 122.60 W with every kind of placement the exporter writes. */
	DsfFile MakeOverlay()
	{
		DsfFile dsf;
		dsf.west = -123.0;
		dsf.south = 45.0;
		dsf.east = -122.0;
		dsf.north = 46.0;
		dsf.properties = { { "sim/overlay", "1" }, { "sim/west", "-123" }, { "sim/east", "-122" }, { "sim/north", "46" }, { "sim/south", "45" },
			{ "sim/creation_agent", "Scenery Editor X" }, { "sim/filter/aptid", "KPDX" }, { "sim/exclude_obj", "-122.610000/45.580000/-122.600000/45.590000" } };
		dsf.objectDefinitions = { "lib/airport/Ramp_Equipment/Tug.obj", "objects/hangar.obj", "lib/airport/Common_Elements/Hangars/Hangar.agp" };
		dsf.polygonDefinitions = { "lib/airport/ground/pavement/asphalt_1.pol", "lib/g10/forests/conifer.for", "lib/airport/lines/solid_yellow.lin",
			"lib/buildings/facades/office.fac", "orthos/field.pol" };

		DsfObjectPlacement draped;
		draped.definition = 0;
		draped.longitude = -122.6012345;
		draped.latitude = 45.5912345;
		draped.heading = 271.25;
		draped.filter = 0;		// Belongs to KPDX: hidden when another pack's KPDX wins
		dsf.objects.push_back(draped);

		DsfObjectPlacement msl = draped;
		msl.definition = 1;
		msl.longitude = -122.6020001;
		msl.mode = DsfObjectPlacement::Mode::Msl;
		msl.elevation = 12.5;
		msl.heading = -90.0;		// Normalized to 270 on the way out
		msl.filter = -1;
		dsf.objects.push_back(msl);

		DsfObjectPlacement agl = msl;
		agl.definition = 1;
		agl.mode = DsfObjectPlacement::Mode::Agl;
		agl.elevation = 3.0;
		agl.latitude = 45.5900007;
		dsf.objects.push_back(agl);

		DsfObjectPlacement point = draped;
		point.definition = 2;
		point.heading = 45.0;
		dsf.objects.push_back(point);

		// Draped pavement with a hole
		dsf.polygons.push_back(Polygon(0, 90, 2, {
			{ Point(-122.6050, 45.5850), Point(-122.6000, 45.5850), Point(-122.6000, 45.5880), Point(-122.6050, 45.5880) },
			{ Point(-122.6040, 45.5860), Point(-122.6040, 45.5870), Point(-122.6010, 45.5870) } }));
		dsf.polygons.back().filter = 0;
		// Forest (3 = area density 3)
		dsf.polygons.push_back(Polygon(1, 128, 2, { { Point(-122.6100, 45.5800), Point(-122.6080, 45.5800), Point(-122.6090, 45.5820) } }));
		// Curved line: lon, lat, control lon, control lat
		dsf.polygons.push_back(Polygon(2, 0, 4, { { Point(-122.6030, 45.5890, { -122.6025, 45.5895 }), Point(-122.6010, 45.5890, { -122.6005, 45.5885 }) } }));
		// Facade with picked walls: lon, lat, wall type
		dsf.polygons.push_back(Polygon(3, 12, 3, { { Point(-122.6070, 45.5870, { 0.0 }), Point(-122.6065, 45.5870, { 2.0 }), Point(-122.6065, 45.5875, { 1.0 }),
			Point(-122.6070, 45.5875, { 3.0 }) } }));
		// UV-mapped orthophoto: lon, lat, s, t
		dsf.polygons.push_back(Polygon(4, 65535, 4, { { Point(-122.6200, 45.5700, { 0.0, 0.0 }), Point(-122.6100, 45.5700, { 1.0, 0.0 }),
			Point(-122.6100, 45.5750, { 1.0, 0.5 }), Point(-122.6200, 45.5750, { 0.0, 0.5 }) } }));
		return dsf;
	}

	void CheckClose(const DsfPoint &read, const DsfPoint &written, const uint8_t planes, const double tolerance)
	{
		for (uint8_t plane = 0; plane < planes; ++plane)
			CHECK(read.v[plane] == Approx(written.v[plane]).margin(tolerance));
	}
}

// -------------------------------------------------------

TEST_CASE("MD5 matches RFC 1321's test vectors", "[dsf][md5]")
{
	CHECK(Hex(Md5Of("")) == "d41d8cd98f00b204e9800998ecf8427e");
	CHECK(Hex(Md5Of("a")) == "0cc175b9c0f1b6a831c399e269772661");
	CHECK(Hex(Md5Of("abc")) == "900150983cd24fb0d6963f7d28e17f72");
	CHECK(Hex(Md5Of("message digest")) == "f96b697d7cb7938d525a2f31aaf161d0");
	CHECK(Hex(Md5Of("The quick brown fox jumps over the lazy dog")) == "9e107d9d372bb6826bd81d3542a419d6");
	CHECK(Hex(Md5Of("12345678901234567890123456789012345678901234567890123456789012345678901234567890")) == "57edf4a22be3c955ac49da2e2107b67a");
	CHECK(Hex(Md5Of(std::string(1000, 'x'))) == Hex(Md5Of(std::string(1000, 'x'))));
}

TEST_CASE("WriteDsf output reads back with every placement intact", "[dsf][write]")
{
	const DsfFile written = MakeOverlay();
	std::vector<uint8_t> bytes;
	std::string error;
	REQUIRE(WriteDsf(written, bytes, &error));
	REQUIRE(bytes.size() > 8 + 4 + 16);
	CHECK(std::memcmp(bytes.data(), "XPLNEDSF", 8) == 0);

	// The footer is the MD5 of everything before it
	const std::array<uint8_t, 16> digest = Md5(bytes.data(), bytes.size() - 16);
	CHECK(std::memcmp(digest.data(), bytes.data() + bytes.size() - 16, 16) == 0);

	DsfFile read;
	REQUIRE(ReadDsf(bytes.data(), bytes.size(), read, &error));
	CHECK(read.properties == written.properties);
	CHECK(read.IsOverlay());
	CHECK(read.west == -123.0);
	CHECK(read.north == 46.0);
	CHECK(read.objectDefinitions == written.objectDefinitions);
	CHECK(read.polygonDefinitions == written.polygonDefinitions);
	CHECK(read.GetExclusions().size() == 1);

	// Objects: positions within a few centimeters, modes, headings and elevations
	const double coordinate = 0.03 / METERS_PER_DEGREE;
	REQUIRE(read.objects.size() == written.objects.size());
	for (size_t i = 0; i < read.objects.size(); ++i)
	{
		INFO("object " << i);
		const DsfObjectPlacement &a = read.objects[i];
		const DsfObjectPlacement &b = written.objects[i];
		CHECK(a.definition == b.definition);
		CHECK(a.mode == b.mode);
		CHECK(a.filter == b.filter);
		CHECK(a.longitude == Approx(b.longitude).margin(coordinate));
		CHECK(a.latitude == Approx(b.latitude).margin(coordinate));
		CHECK(a.heading == Approx(std::fmod(b.heading + 360.0, 360.0)).margin(0.01));
		if (b.mode != DsfObjectPlacement::Mode::Draped)
			CHECK(a.elevation == Approx(b.elevation).margin(0.001));
	}

	// Polygons: parameters, planes, windings and points
	REQUIRE(read.polygons.size() == written.polygons.size());
	for (size_t i = 0; i < read.polygons.size(); ++i)
	{
		INFO("polygon " << i);
		const DsfPolygonPlacement &a = read.polygons[i];
		const DsfPolygonPlacement &b = written.polygons[i];
		CHECK(a.definition == b.definition);
		CHECK(a.parameter == b.parameter);
		CHECK(a.filter == b.filter);
		REQUIRE(a.planes == b.planes);
		REQUIRE(a.windings.size() == b.windings.size());
		for (size_t w = 0; w < a.windings.size(); ++w)
		{
			REQUIRE(a.windings[w].size() == b.windings[w].size());
			for (size_t p = 0; p < a.windings[w].size(); ++p)
				CheckClose(a.windings[w][p], b.windings[w][p], 2, coordinate);
		}
	}

	// Whole-number planes come back exactly: facade wall types, UVs of 0 / 1 corners
	const DsfPolygonPlacement &facade = read.polygons[3];
	CHECK(facade.windings[0][0].v[2] == 0.0);
	CHECK(facade.windings[0][1].v[2] == 2.0);
	CHECK(facade.windings[0][2].v[2] == 1.0);
	CHECK(facade.windings[0][3].v[2] == 3.0);
	const DsfPolygonPlacement &curve = read.polygons[2];
	CHECK(curve.windings[0][0].v[2] == Approx(-122.6025).margin(coordinate));
	CHECK(curve.windings[0][1].v[3] == Approx(45.5885).margin(coordinate));
	const DsfPolygonPlacement &ortho = read.polygons[4];
	CHECK(ortho.windings[0][2].v[3] == Approx(0.5).margin(1e-4));

	// Writing what was read gives the same placements again (stable, no drift)
	std::vector<uint8_t> again;
	REQUIRE(WriteDsf(read, again, &error));
	DsfFile reread;
	REQUIRE(ReadDsf(again.data(), again.size(), reread, &error));
	REQUIRE(reread.objects.size() == read.objects.size());
	for (size_t i = 0; i < read.objects.size(); ++i)
	{
		CHECK(reread.objects[i].longitude == Approx(read.objects[i].longitude).margin(1e-9));
		CHECK(reread.objects[i].latitude == Approx(read.objects[i].latitude).margin(1e-9));
	}
}

TEST_CASE("WriteDsf keeps large and crowded content precise", "[dsf][write]")
{
	DsfFile dsf;
	dsf.properties = { { "sim/overlay", "1" }, { "sim/west", "8" }, { "sim/east", "9" }, { "sim/north", "48" }, { "sim/south", "47" } };
	dsf.objectDefinitions = { "lib/airport/vehicles/car.obj" };
	dsf.polygonDefinitions = { "lib/airport/ground/pavement/concrete_1.pol" };

	// A 10 km taxiway polygon: its own pool, about 15 cm steps along it
	std::vector<DsfPoint> outline;
	for (int i = 0; i <= 400; ++i)
		outline.push_back(Point(8.50 + 0.12 * i / 400.0, 47.45 + 0.0001 * (i % 2)));
	for (int i = 400; i >= 0; --i)
		outline.push_back(Point(8.50 + 0.12 * i / 400.0, 47.4505));
	dsf.polygons.push_back(Polygon(0, 0, 2, { outline }));

	// 3000 objects spread over 0.3 degrees: many small pools, centimeter positions
	for (int i = 0; i < 3000; ++i)
	{
		DsfObjectPlacement object;
		object.longitude = 8.40 + 0.3 * (i % 100) / 100.0 + 1e-7 * i;
		object.latitude = 47.40 + 0.3 * (i / 100) / 30.0;
		object.heading = (i * 7) % 360;
		dsf.objects.push_back(object);
	}

	std::vector<uint8_t> bytes;
	std::string error;
	REQUIRE(WriteDsf(dsf, bytes, &error));
	DsfFile read;
	REQUIRE(ReadDsf(bytes.data(), bytes.size(), read, &error));

	REQUIRE(read.objects.size() == 3000);
	double worst = 0.0;
	for (size_t i = 0; i < read.objects.size(); ++i)
	{
		worst = std::max(worst, std::abs(read.objects[i].longitude - dsf.objects[i].longitude));
		worst = std::max(worst, std::abs(read.objects[i].latitude - dsf.objects[i].latitude));
		CHECK(read.objects[i].heading == dsf.objects[i].heading);	// Whole degrees: exact
	}
	CHECK(worst * METERS_PER_DEGREE < 0.03);

	REQUIRE(read.polygons.size() == 1);
	double polygonWorst = 0.0;
	for (size_t p = 0; p < outline.size(); ++p)
	{
		polygonWorst = std::max(polygonWorst, std::abs(read.polygons[0].windings[0][p].v[0] - outline[p].v[0]));
		polygonWorst = std::max(polygonWorst, std::abs(read.polygons[0].windings[0][p].v[1] - outline[p].v[1]));
	}
	CHECK(polygonWorst * METERS_PER_DEGREE < 0.15);
}

TEST_CASE("WriteDsf refuses what a DSF can't hold", "[dsf][write]")
{
	DsfFile dsf;
	dsf.properties = { { "sim/west", "0" }, { "sim/east", "1" }, { "sim/north", "1" }, { "sim/south", "0" } };
	dsf.polygonDefinitions = { "a.pol" };
	std::vector<DsfPoint> huge;
	for (int i = 0; i < 70000; ++i)
		huge.push_back(Point(0.5 + i * 1e-7, 0.5));
	dsf.polygons.push_back(Polygon(0, 0, 2, { huge }));

	std::vector<uint8_t> bytes;
	std::string error;
	CHECK_FALSE(WriteDsf(dsf, bytes, &error));
	CHECK(error.find("65535") != std::string::npos);

	dsf.polygons = { Polygon(3, 0, 2, { { Point(0.5, 0.5), Point(0.6, 0.5), Point(0.6, 0.6) } }) };
	CHECK_FALSE(WriteDsf(dsf, bytes, &error));
	CHECK(error.find("definition") != std::string::npos);

	// Nothing but properties is a valid (exclusion-only) tile
	DsfFile empty;
	empty.properties = { { "sim/overlay", "1" }, { "sim/west", "0" }, { "sim/east", "1" }, { "sim/north", "1" }, { "sim/south", "0" },
		{ "sim/exclude_obj", "0.100000/0.100000/0.200000/0.200000" } };
	REQUIRE(WriteDsf(empty, bytes, &error));
	DsfFile read;
	REQUIRE(ReadDsf(bytes.data(), bytes.size(), read, &error));
	CHECK(read.GetExclusions().size() == 1);
	CHECK(read.objects.empty());
}

TEST_CASE("SplitDsfByTile gives each 1x1 degree tile its own DSF", "[dsf][tiles]")
{
	DsfFile dsf;
	dsf.properties = { { "sim/overlay", "1" }, { "sim/west", "-123" }, { "sim/east", "-121" }, { "sim/north", "46" }, { "sim/south", "45" },
		{ "sim/creation_agent", "Scenery Editor X" },
		{ "sim/exclude_fac", "-122.100000/45.400000/-121.900000/45.500000" },
		{ "sim/exclude_obj", "-122.500000/45.200000/-122.400000/45.300000" } };
	dsf.objectDefinitions = { "west.obj", "east.obj", "unused.obj" };
	dsf.polygonDefinitions = { "east.pol", "west.pol" };

	DsfObjectPlacement west;
	west.definition = 0;
	west.longitude = -122.5;
	west.latitude = 45.5;
	DsfObjectPlacement east = west;
	east.definition = 1;
	east.longitude = -121.5;
	dsf.objects = { west, east, west };

	dsf.polygons.push_back(Polygon(0, 0, 2, { { Point(-121.8, 45.5), Point(-121.7, 45.5), Point(-121.7, 45.6) } }));
	dsf.polygons.push_back(Polygon(1, 0, 2, { { Point(-122.1, 45.5), Point(-121.9, 45.5), Point(-121.9, 45.6) } }));	// Crosses into the east tile

	std::vector<std::string> warnings;
	const std::vector<DsfFile> tiles = SplitDsfByTile(dsf, &warnings);
	REQUIRE(tiles.size() == 2);
	REQUIRE(warnings.size() == 1);
	CHECK(warnings[0].find("west.pol") != std::string::npos);

	const DsfFile &a = tiles[0];	// -123 (south-west first)
	CHECK(a.west == -123.0);
	CHECK(a.south == 45.0);
	CHECK(a.GetProperty("sim/west") == "-123");
	CHECK(a.GetProperty("sim/east") == "-122");
	CHECK(a.GetProperty("sim/north") == "46");
	CHECK(a.GetProperty("sim/creation_agent") == "Scenery Editor X");
	CHECK(a.properties[0].first == "sim/overlay");
	CHECK(a.objectDefinitions == std::vector<std::string>{ "west.obj" });
	CHECK(a.objects.size() == 2);
	CHECK(a.polygonDefinitions == std::vector<std::string>{ "west.pol" });
	REQUIRE(a.GetExclusions().size() == 2);		// The facade exclusion straddles both tiles

	const DsfFile &b = tiles[1];
	CHECK(b.west == -122.0);
	CHECK(b.GetProperty("sim/east") == "-121");
	CHECK(b.objectDefinitions == std::vector<std::string>{ "east.obj" });
	CHECK(b.objects.size() == 1);
	CHECK(b.objects[0].definition == 0);
	CHECK(b.polygons.size() == 1);
	CHECK(b.polygonDefinitions == std::vector<std::string>{ "east.pol" });
	REQUIRE(b.GetExclusions().size() == 1);
	CHECK(b.GetExclusions()[0].first == "sim/exclude_fac");

	// Every tile encodes and decodes
	for (const DsfFile &tile : tiles)
	{
		std::vector<uint8_t> bytes;
		std::string error;
		REQUIRE(WriteDsf(tile, bytes, &error));
		DsfFile read;
		REQUIRE(ReadDsf(bytes.data(), bytes.size(), read, &error));
		CHECK(read.objects.size() == tile.objects.size());
		CHECK(read.polygons.size() == tile.polygons.size());
	}

	// An exclusion ending exactly on a tile edge doesn't reach the next tile
	DsfFile edge;
	edge.properties = { { "sim/exclude_obj", "-122.500000/45.500000/-122.000000/46.000000" } };
	const std::vector<DsfFile> edgeTiles = SplitDsfByTile(edge);
	REQUIRE(edgeTiles.size() == 1);
	CHECK(edgeTiles[0].west == -123.0);
	CHECK(edgeTiles[0].south == 45.0);
}

TEST_CASE("SplitDsfByTile stays inside the world whatever a damaged project holds", "[dsf][tiles]")
{
	// Found by fuzz_dsf: exclusion bounds past the world made billions of tiles (and each tile scanned every
	// exclusion); coordinates too large for an int were cast anyway
	DsfFile dsf;
	dsf.properties = { { "sim/exclude_obj", "-1e9/-95/1e9/-89.5" }, { "sim/exclude_fac", "nan/1/2/3" }, { "sim/exclude_pol", "5/5/4/4" } };
	DsfObjectPlacement far;
	far.latitude = 1.0e300;
	far.longitude = -std::numeric_limits<double>::infinity();
	dsf.objects.push_back(far);
	std::vector<std::string> warnings;
	const std::vector<DsfFile> tiles = SplitDsfByTile(dsf, &warnings);
	CHECK(tiles.size() == 361);		// The 360 tiles of the southernmost row, plus the far object's corner tile
	CHECK(warnings.size() == 2);	// NaN and inverted bounds are left out
	for (const DsfFile &tile : tiles)
	{
		CHECK(tile.south >= -90.0);
		CHECK(tile.north <= 90.0);
		CHECK(tile.west >= -180.0);
		CHECK(tile.east <= 180.0);
	}
	CHECK(GetDsfTileName(std::numeric_limits<double>::quiet_NaN(), 1.0e12) == "+00+179");

	// Found by fuzz_dsf (UBSan): an infinite coordinate quantized to NaN and was cast to a 16-bit raw value anyway
	DsfFile tile;
	tile.properties = { { "sim/west", "-123" }, { "sim/south", "47" }, { "sim/east", "-122" }, { "sim/north", "48" } };
	tile.objectDefinitions = { "a.obj" };
	DsfObjectPlacement object;
	object.latitude = 47.5;
	object.longitude = std::numeric_limits<double>::infinity();
	tile.objects = { object, object };
	tile.objects[1].longitude = -122.5;
	std::vector<uint8_t> bytes;
	std::string error;
	if (WriteDsf(tile, bytes, &error))
	{
		DsfFile back;
		CHECK(ReadDsf(bytes.data(), bytes.size(), back, &error));
	}
}

// -------------------------------------------------------
// Road networks

namespace
{
	DsfRoadSegment Road(const uint32_t from, const uint32_t to, std::vector<DsfRoadPoint> points, const uint32_t subtype = 100)
	{
		DsfRoadSegment road;
		road.definition = 0;
		road.subtype = subtype;
		road.startJunction = from;
		road.endJunction = to;
		road.points = std::move(points);
		return road;
	}
}

TEST_CASE("WriteDsf writes road networks that read back as segments", "[dsf][write][roads]")
{
	DsfFile written = MakeOverlay();
	written.networkDefinitions = { "lib/g10/roads.net" };
	// A straight segment into a junction shared by a curved one (cubic: two control points), then a raised one
	written.roads.push_back(Road(1, 2, { { -122.6050, 45.5850 }, { -122.6040, 45.5855 }, { -122.6030, 45.5860 } }));
	written.roads.push_back(Road(2, 3, { { -122.6030, 45.5860 }, { -122.6025, 45.5868, true }, { -122.6015, 45.5870, true }, { -122.6010, 45.5862 } }, 101));
	DsfRoadSegment bridge = Road(3, 4, { { -122.6010, 45.5862 }, { -122.6000, 45.5850 } }, 101);
	bridge.startLevel = 1;
	bridge.endLevel = 2;
	bridge.filter = 0;
	written.roads.push_back(bridge);

	std::vector<uint8_t> bytes;
	std::string error;
	REQUIRE(WriteDsf(written, bytes, &error));
	DsfFile read;
	REQUIRE(ReadDsf(bytes.data(), bytes.size(), read, &error));
	CHECK(read.networkDefinitions == written.networkDefinitions);
	CHECK(read.objects.size() == written.objects.size());
	CHECK(read.polygons.size() == written.polygons.size());

	// 32-bit pools: positions far below a millimeter; ids, levels and control flags exact
	const double coordinate = 0.001 / METERS_PER_DEGREE;
	REQUIRE(read.roads.size() == written.roads.size());
	for (size_t r = 0; r < read.roads.size(); ++r)
	{
		INFO("road " << r);
		const DsfRoadSegment &a = read.roads[r];
		const DsfRoadSegment &b = written.roads[r];
		CHECK(a.definition == b.definition);
		CHECK(a.subtype == b.subtype);
		CHECK(a.startJunction == b.startJunction);
		CHECK(a.endJunction == b.endJunction);
		CHECK(a.startLevel == b.startLevel);
		CHECK(a.endLevel == b.endLevel);
		CHECK(a.filter == b.filter);
		REQUIRE(a.points.size() == b.points.size());
		for (size_t p = 0; p < a.points.size(); ++p)
		{
			CHECK(a.points[p].longitude == Approx(b.points[p].longitude).margin(coordinate));
			CHECK(a.points[p].latitude == Approx(b.points[p].latitude).margin(coordinate));
			CHECK(a.points[p].control == b.points[p].control);
		}
	}

	// Segments that can't be written are refused with a reason
	DsfFile bad = written;
	bad.roads[0].endJunction = 0;
	CHECK_FALSE(WriteDsf(bad, bytes, &error));
	CHECK(error.find("junction") != std::string::npos);
	bad = written;
	bad.roads[1].points.back().control = true;
	CHECK_FALSE(WriteDsf(bad, bytes, &error));
	bad = written;
	bad.roads[0].subtype = 300;
	CHECK_FALSE(WriteDsf(bad, bytes, &error));

	// A tile split keeps the roads with their network definition
	written.roads.push_back(Road(5, 6, { { -121.9000, 45.5000 }, { -121.8900, 45.5000 } }));
	const std::vector<DsfFile> tiles = SplitDsfByTile(written);
	size_t roads = 0;
	for (const DsfFile &tile : tiles)
	{
		roads += tile.roads.size();
		if (!tile.roads.empty())
			CHECK(tile.networkDefinitions == std::vector<std::string>{ "lib/g10/roads.net" });
	}
	CHECK(roads == written.roads.size());
}
