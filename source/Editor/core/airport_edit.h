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
 * airport_edit.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>
#include <SceneryEditorX/project/spline_edit.h>
#include <SceneryEditorX/xplane/xp_airport_scene.h>
#include <SceneryEditorX/xplane/xp_apt.h>

// -------------------------------------------------------

namespace SceneryEditorX::AirportEdit
{
	/**
	 * Row-level helpers the viewport tools use on the open airport: copies of a row for undo, where point features
	 * sit, the node chains of shapes, and moving / turning / scaling whole rows on the ground plane.
	 */

	using Kind = XPlane::AirportElementKind;

	/* @brief A copy of one apt.dat row (monostate when the kind / index name none). */
	using Snapshot = std::variant<std::monostate, XPlane::AptRunway, XPlane::AptWaterRunway, XPlane::AptHelipad, XPlane::AptPavement,
		XPlane::AptLinearFeature, XPlane::AptBoundary, XPlane::AptViewpoint, XPlane::AptBeacon, XPlane::AptWindsock, XPlane::AptSign,
		XPlane::AptLightFixture, XPlane::AptStartup, XPlane::AptLegacyStartup, XPlane::AptTruckParking, XPlane::AptTruckDestination, XPlane::AptJetway>;

	Snapshot Capture(const XPlane::AptAirport &airport, Kind kind, uint32_t index);

	/* @brief Puts a copy back in place of the row. False when the row (or the snapshot's type) doesn't match. */
	bool Restore(XPlane::AptAirport &airport, Kind kind, uint32_t index, const Snapshot &snapshot);

	/* @brief Rough memory a snapshot holds (for the undo history's budget). */
	size_t SizeOf(const Snapshot &snapshot);

	/* @brief Removes a row; later rows of its kind move up an index. */
	bool EraseRow(XPlane::AptAirport &airport, Kind kind, uint32_t index);

	/* @brief Puts a row (back) at an index; later rows of its kind move down one. */
	bool InsertRow(XPlane::AptAirport &airport, Kind kind, uint32_t index, const Snapshot &snapshot);

	/* @brief Kinds that are a point with an optional heading (moved by their entity, no rebuild). */
	bool IsPointKind(Kind kind);

	/* @brief Point kinds whose row has a heading of its own (signs, startup locations, ...). */
	bool HasHeading(Kind kind);

	/* @brief Kinds made of bezier node chains (pavement, painted lines and lights, boundaries). */
	bool IsShapeKind(Kind kind);

	/* @brief Kinds the viewport tools can move as a whole (every row with a position). */
	bool IsMovableKind(Kind kind);

	/* @brief Where a point feature sits (and points, for kinds with a heading). */
	bool GetPlacement(const XPlane::AptAirport &airport, Kind kind, uint32_t index, double &latitude, double &longitude, float *heading = nullptr);

	/* @brief Moves (and turns, when the kind has a heading and one is given) a point feature. */
	bool SetPlacement(XPlane::AptAirport &airport, Kind kind, uint32_t index, double latitude, double longitude, std::optional<float> heading);

	/* @brief The node chains of a shape row (pavement rings, line chains, boundary rings), or nullptr. */
	std::vector<XPlane::AptNodeChain> *GetChains(XPlane::AptAirport &airport, Kind kind, uint32_t index);
	const std::vector<XPlane::AptNodeChain> *GetChains(const XPlane::AptAirport &airport, Kind kind, uint32_t index);

	/**
	 * @brief Moves, turns and / or scales a whole row on the ground plane by a transform in the scene's local frame
	 * (shapes node by node with their handles, runways by their ends, helipads and point features by their position
	 * and heading). Texture headings turn with the row.
	 * @param turnDegrees The transform's rotation (counter-clockwise seen from above), for headings.
	 */
	bool TransformRow(XPlane::AptAirport &airport, Kind kind, uint32_t index, const Splines::Affine2 &transform, double turnDegrees, const XPlane::GeoProjection &projection);

	/* @brief A heading (true degrees) turned counter-clockwise by an angle, in 0..360. */
	float TurnHeading(float heading, double turnDegrees);
}

// -------------------------------------------------------
