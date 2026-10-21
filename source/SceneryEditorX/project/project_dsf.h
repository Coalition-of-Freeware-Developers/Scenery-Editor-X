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
 * project_dsf.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "scenery_project.h"
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/**
	 * A project's overlay items as an in-memory DSF tile (XPlane::DsfFile), laid out the way WorldEditor exports them, so
	 * what a project places goes through the same XPlane::OverlayBuilder that draws a scenery pack's tiles. Standard
	 * library only.
	 *
	 * Parameter and planes per point, by kind:
	 *  - Polygon      texture heading (EncodePolygonHeading) / 2, 4 with curves
	 *  - Orthophoto   65535 / 4 (lon, lat, s, t), 8 with curves (lon, lat, control lon, control lat, s, t, control s, control t)
	 *  - Line         1 closed, 0 open / 2, 4 with curves
	 *  - String       spacing (EncodeStringSpacing) / 2, 4 with curves; a closed string ends on its first node again
	 *  - Forest       density + 256 x fill mode (EncodeForestParameter) / 2; curves are ignored, as WorldEditor does
	 *  - Facade       height in meters / 2, 4 with curves; 3, 5 with the wall type plane when walls are picked
	 *  - Autogen      EncodeAutogenParameter / 2; .ags outlines are split into spawning and non-spawning chains
	 *  - Object       .agp autogen points always, .obj objects on request (the editor places those itself)
	 *  - Exclusions   sim/exclude_* properties ("west/south/east/north", polygons add ";lon/lat,lon/lat,...")
	 *  - Road         a network segment (DsfRoadSegment): the road nodes become junction ids 1, 2...; curves become two
	 *                 control points between vertices (cubic), as WorldEditor exports roads
	 *
	 * Curves: a node's bezier handles are offsets from it in degrees (lo back toward the previous node, hi toward the
	 * next); a DSF point carries the absolute control point leading out of it, the incoming one being its mirror. A node
	 * whose handles aren't mirror images, or that has only one, becomes up to three co-located points (incoming handle,
	 * corner, outgoing handle), the way WorldEditor writes them. Outlines of filled shapes run counter-clockwise and holes
	 * clockwise (facades keep their node order: it says which side their walls face).
	 */

	struct DsfConversionOptions
	{
		bool includeObjects = false;		// .obj Object items too (the editor places those through the object loader itself)
		bool includeHidden = false;			// Hidden items, and everything inside hidden groups and airports
	};

	struct DsfConversion
	{
		XPlane::DsfFile dsf;
		std::vector<std::string> objectItems;		// Item id of each dsf.objects entry
		std::vector<std::string> polygonItems;		// Item id of each dsf.polygons entry
		std::vector<std::string> roadItems;			// Item id of each dsf.roads entry
		std::vector<std::string> exclusionItems;	// Items that added sim/exclude_* properties
		std::vector<std::string> warnings;			// Items left out (too few nodes, a resource of the wrong kind)
		uint32_t hidden = 0;						// Items left out because they (or a container above them) are hidden
	};

	/**
	 * @brief Converts every overlay item of a project, depth-first. Objects and roads keep the hierarchy order; polygons
	 * are written bottom of the hierarchy first, so in X-Plane (later polygons of a layer on top) the item at the top
	 * draws over the ones below it, as in WorldEditor.
	 */
	DsfConversion BuildProjectDsf(const SceneryProject &project, const DsfConversionOptions &options = {});

	/* @brief Hash of everything a DSF draws (definitions, placements, properties): equal hashes draw the same. */
	uint64_t GetDsfFingerprint(const XPlane::DsfFile &dsf);

	/* @brief What one item contributes to a conversion. */
	struct DsfItemFingerprint
	{
		uint64_t hash = 0;					// Its placements with their definitions' paths: equal hashes draw the same
		uint32_t firstPlacement = 0;		// Its first entry in the list it is in (polygons, objects or roads)
	};

	/**
	 * @brief Per item id, so an editor can redraw only the items an edit changed. An item's hash leaves out where its
	 * placements sit in the lists (other items coming and going don't change it); roads include the junction ids they share.
	 */
	std::unordered_map<std::string, DsfItemFingerprint> GetItemFingerprints(const DsfConversion &conversion);

	/* @brief Draped polygon parameter: whole degrees plus 360 x 1/128ths of a degree (WorldEditor's encode_heading). */
	uint16_t EncodePolygonHeading(double heading);

	/* @brief Object string parameter: whole meters, or 10000 + 128 x meters for spacings off the meter (X-Plane 12). */
	uint16_t EncodeStringSpacing(double spacing);

	/* @brief Forest parameter: density 0..255 plus 256 x fill mode. */
	uint16_t EncodeForestParameter(float density, ForestFill fill);

	/* @brief Autogen parameter: quarters of the height (meters) in the high byte, spelling (.agb) or spawning chains (.ags) in the low byte. */
	uint16_t EncodeAutogenParameter(float height, int low);

	/* @brief "west/south/east/north" with six decimals, as sim/exclude_* values carry them. */
	std::string FormatExclusionBounds(double west, double south, double east, double north);
}

// -------------------------------------------------------
