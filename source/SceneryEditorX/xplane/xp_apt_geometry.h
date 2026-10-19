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
 * xp_apt_geometry.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_apt.h"
#include <cstdint>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Geometry for airport data: a local projection, bezier flattening, polygon triangulation and the outlines of
	 * runways and helipads. Local space follows X-Plane's world: meters, +X east, -Z north (+Z south), +Y up.
	 * Standard library only.
	 */

	/* @brief A point in local meters (x east, z south). */
	struct Point2
	{
		double x = 0.0;
		double z = 0.0;
	};

	/* @brief Axis-aligned bounds in local meters. */
	struct Bounds2
	{
		double minX = 0.0;
		double minZ = 0.0;
		double maxX = 0.0;
		double maxZ = 0.0;
		bool valid = false;

		void Add(const Point2 &point);
		void Add(const Bounds2 &other);
		[[nodiscard]] Point2 GetCenter() const { return { 0.5 * (minX + maxX), 0.5 * (minZ + maxZ) }; }
		[[nodiscard]] double GetRadius() const;
	};

	/**
	 * @class GeoProjection
	 * @brief Equirectangular projection around a datum: good to centimeters across an airport, which is all the
	 * editor needs (X-Plane itself re-projects on load).
	 */
	class GeoProjection
	{
	public:
		GeoProjection() = default;
		GeoProjection(double datumLatitude, double datumLongitude);

		[[nodiscard]] Point2 ToLocal(double latitude, double longitude) const;
		void ToGeo(const Point2 &local, double &latitude, double &longitude) const;

		[[nodiscard]] double GetDatumLatitude() const { return m_Latitude; }
		[[nodiscard]] double GetDatumLongitude() const { return m_Longitude; }
		[[nodiscard]] double GetMetersPerDegreeLatitude() const { return m_MetersPerDegreeLatitude; }
		[[nodiscard]] double GetMetersPerDegreeLongitude() const { return m_MetersPerDegreeLongitude; }

	private:
		double m_Latitude = 0.0;
		double m_Longitude = 0.0;
		double m_MetersPerDegreeLatitude = 111132.954;
		double m_MetersPerDegreeLongitude = 111412.84;
	};

	/**
	 * @brief Flattens a node chain into local points, subdividing the bezier segments. A closed chain comes back as a
	 * polygon (the first point isn't repeated at the end); an open one as a polyline.
	 * @param maxSegmentLength Longest straight piece a curve is cut into, meters.
	 */
	void FlattenChain(const AptNodeChain &chain, const GeoProjection &projection, std::vector<Point2> &out, double maxSegmentLength = 2.0);

	/* @brief Signed area of a polygon in local space (positive when counter-clockwise seen from above). */
	double SignedArea(const std::vector<Point2> &polygon);

	/* @brief Bounds of a point list. */
	Bounds2 GetBounds(const std::vector<Point2> &points);

	/**
	 * @brief Triangulates a polygon with holes (ear clipping, after mapbox earcut).
	 * @param rings rings[0] is the outline, the rest are holes; any winding.
	 * @param outPoints The rings concatenated (what the indices refer to).
	 * @param outIndices Triangles, wound counter-clockwise seen from above (+Y).
	 * @return False when nothing could be triangulated.
	 */
	bool TriangulatePolygon(const std::vector<std::vector<Point2>> &rings, std::vector<Point2> &outPoints, std::vector<uint32_t> &outIndices);

	/**
	 * @brief Turns a polyline into a ribbon of the given width (two triangles per segment, mitered joins).
	 * @param outIndices Triangles wound counter-clockwise seen from above.
	 */
	void BuildPolylineStrip(const std::vector<Point2> &path, bool closed, double width, std::vector<Point2> &outPoints, std::vector<uint32_t> &outIndices);

	/**
	 * @brief The runway rectangle: corners in order end-0 left, end-0 right, end-1 right, end-1 left.
	 * @param includeBlastPads Extend the ends by their blast pads.
	 */
	void GetRunwayCorners(const AptRunway &runway, const GeoProjection &projection, Point2 corners[4], bool includeBlastPads = false);

	/* @brief The water runway rectangle, same order as GetRunwayCorners. */
	void GetWaterRunwayCorners(const AptWaterRunway &runway, const GeoProjection &projection, Point2 corners[4]);

	/* @brief The helipad rectangle rotated to its heading (length along the heading). */
	void GetHelipadCorners(const AptHelipad &helipad, const GeoProjection &projection, Point2 corners[4]);

	/* @brief Unit vector in local space for a true heading (0 = north = -Z, 90 = east = +X). */
	Point2 HeadingToDirection(double headingDegrees);

	/* @brief True heading of a local direction vector. */
	double DirectionToHeading(const Point2 &direction);
}

// -------------------------------------------------------
