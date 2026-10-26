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
 * spline_edit.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "scenery_project.h"
#include <SceneryEditorX/xplane/xp_apt.h>
#include <SceneryEditorX/xplane/xp_apt_geometry.h>
#include <compare>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::Splines
{
	/**
	 * Node editing for the bezier shapes scenery is made of: apt.dat pavement, painted lines and boundaries (rows
	 * 111..116) and the project's polygons, lines, strings, forests, facades and exclusions (ProjectFile::GeoRing).
	 *
	 * A shape is read into local meters (+X east, +Z south around a GeoProjection datum) with absolute handle
	 * positions, edited there (move, rotate / scale, drag a handle, insert / delete nodes, smooth / corner), and
	 * written back in the source's own convention:
	 *  - apt.dat stores one control point leading out of a node, the incoming one being its mirror; a node whose
	 *    handles aren't mirror images is written as co-located nodes (incoming, then outgoing), as WorldEditor does,
	 *    and such pairs are read back as one split node.
	 *  - project nodes store both handles as offsets in degrees, plus a split flag.
	 * Nodes and handles that weren't moved write back their original coordinates bit for bit, so editing one node
	 * of a shape changes only that node in a git diff. Standard library only (unit tested without the engine).
	 *
	 * A chain can also be edited as a uniform cubic B-spline (see the B-spline section): its control polygon is
	 * what gets dragged, and the bezier nodes the formats store are worked out from it exactly.
	 */

	using XPlane::Point2;
	using XPlane::GeoProjection;

	/**
	 * @brief How a node's handles behave while they are edited. The formats only store the handles and a split
	 * flag, so the type is read from the geometry (GetNodeType):
	 *  - Corner     no handles: the segments meet the node in a sharp point
	 *  - Free       handles move on their own (a cusp); one or both
	 *  - Smooth     handles point opposite ways, each keeping its own length (the curve's direction is continuous)
	 *  - Symmetric  handles are mirror images (same length)
	 */
	enum class NodeType : uint8_t
	{
		Corner,
		Free,
		Smooth,
		Symmetric
	};

	const char *GetNodeTypeName(NodeType type);

	/* @brief Per-node data the shapes carry besides geometry, kept through edits (inserted nodes take their neighbor's). */
	struct NodeAttributes
	{
		// apt.dat: painted line / light string starting at the node (on split nodes, of the outgoing co-located node)
		int lineType = 0;
		int lightType = 0;
		int lineTypeIn = 0;			// Split apt.dat nodes: the incoming co-located node's (its segment has no length)
		int lightTypeIn = 0;

		// Project nodes
		bool hasUv = false;			// Orthophoto texture coordinates
		float s = 0.0f, t = 0.0f;
		float sLo = 0.0f, tLo = 0.0f, sHi = 0.0f, tHi = 0.0f;
		int wallType = 0;			// Facade wall of the segment starting here
		bool spawnTiles = true;		// Autogen
	};

	/**
	 * @brief Where a point came from, so an unmoved point writes back exactly what was read. latitude / longitude
	 * are as the source stores them: absolute for nodes and apt.dat controls, offsets from the node for project handles.
	 */
	struct SourcePoint
	{
		Point2 local;				// As converted when read
		double latitude = 0.0;
		double longitude = 0.0;
		bool valid = false;
	};

	struct Node
	{
		Point2 position;
		Point2 handleIn;			// Absolute positions, meaningful when hasIn / hasOut
		Point2 handleOut;
		bool hasIn = false;
		bool hasOut = false;
		bool split = false;			// Handles independent; otherwise handleIn mirrors handleOut through position
		/**
		 * What SetNodeType last asked for (Corner: nothing). Only decides between types the geometry can't tell
		 * apart: mirror-image handles (smooth or symmetric) and split handles that happen to line up (smooth or free).
		 */
		NodeType preferred = NodeType::Corner;
		NodeAttributes attributes;

		SourcePoint sourcePosition;
		SourcePoint sourcePositionIn;	// apt.dat split nodes: the incoming co-located node's own position
		SourcePoint sourceIn;
		SourcePoint sourceOut;

		[[nodiscard]] bool HasCurve() const { return hasIn || hasOut; }
	};

	struct Chain
	{
		std::vector<Node> nodes;
		bool closed = true;
		bool wrapsSplit = false;	// apt.dat: node 0 is a split pair whose incoming half closed the ring (written last)

		/**
		 * B-spline control polygon, one point per node, while the chain is edited as a B-spline (empty otherwise).
		 * The nodes then follow it: the edits below move, add and remove control points and rework the nodes.
		 */
		std::vector<Point2> controls;

		/* @brief Segments: nodes - 1 for an open chain, nodes for a closed one. */
		[[nodiscard]] uint32_t GetSegmentCount() const;
		[[nodiscard]] uint32_t GetMinimumNodes() const { return closed ? 3u : 2u; }
		[[nodiscard]] bool IsBSpline() const { return !controls.empty(); }

		/* @brief Where node i is edited from: its control point for a B-spline, the node itself otherwise. */
		[[nodiscard]] const Point2 &GetEditPoint(uint32_t node) const { return IsBSpline() ? controls[node] : nodes[node].position; }
	};

	/* @brief A whole shape: chains[0] is the outline (or the line), the rest holes. */
	struct Shape
	{
		std::vector<Chain> chains;

		[[nodiscard]] size_t CountNodes() const;
		[[nodiscard]] bool IsValid(uint32_t chain, uint32_t node) const;
	};

	struct NodeRef
	{
		uint32_t chain = 0;
		uint32_t node = 0;
		auto operator<=>(const NodeRef &) const = default;
	};

	/* @brief The four control points of one segment (P0 node, P1 its outgoing handle, P2 the next node's incoming handle, P3 that node). */
	struct Bezier
	{
		Point2 p[4];
		[[nodiscard]] Point2 Evaluate(double t) const;
		[[nodiscard]] bool IsStraight() const;
	};

	/* @brief x' = a x + b z + tx, z' = c x + d z + tz: a translation, rotation and / or scale on the ground plane. */
	struct Affine2
	{
		double a = 1.0, b = 0.0, c = 0.0, d = 1.0, tx = 0.0, tz = 0.0;

		static Affine2 Translation(double x, double z);
		/* @brief Rotation by an angle counter-clockwise seen from above (+Y), about a pivot. */
		static Affine2 Rotation(double radians, const Point2 &pivot);
		static Affine2 Scale(double sx, double sz, const Point2 &pivot);

		[[nodiscard]] Point2 Apply(const Point2 &p) const { return { a * p.x + b * p.z + tx, c * p.x + d * p.z + tz }; }
		/* @brief this after other: Apply(other.Apply(p)). */
		[[nodiscard]] Affine2 After(const Affine2 &other) const;
		[[nodiscard]] bool IsIdentity() const;
	};

	/* @brief One flattened point of a chain, with the segment and curve parameter it lies at. */
	struct Sample
	{
		Point2 position;
		uint32_t segment = 0;
		double t = 0.0;
	};

	// -------------------------------------------------------
	// Conversions

	/* @brief apt.dat chains (pavement rings, line chains, boundary rings) to a shape; co-located runs become split nodes. */
	Shape FromAptChains(const std::vector<XPlane::AptNodeChain> &chains, const GeoProjection &projection);

	/* @brief A shape back to apt.dat chains (split nodes as co-located pairs). */
	std::vector<XPlane::AptNodeChain> ToAptChains(const Shape &shape, const GeoProjection &projection);

	/**
	 * @brief Project rings to a shape.
	 * @param closed Whether the rings are closed (polygons, closed lines and strings) or open chains.
	 */
	Shape FromGeoRings(const std::vector<ProjectFile::GeoRing> &rings, bool closed, const GeoProjection &projection);

	/* @brief A shape back to project rings. */
	std::vector<ProjectFile::GeoRing> ToGeoRings(const Shape &shape, const GeoProjection &projection);

	// -------------------------------------------------------
	// Queries

	/* @brief The bezier of a segment (straight segments have P1 = P0 and P2 = P3). */
	Bezier GetSegment(const Chain &chain, uint32_t segment);

	/* @brief Flattens a chain for drawing and hit testing; closed chains end on their first point again. */
	void SampleChain(const Chain &chain, std::vector<Sample> &out, double maxSegmentLength = 1.0);

	/* @brief Every node's position and handles, for framing and pivots. */
	XPlane::Bounds2 GetBounds(const Shape &shape, bool includeHandles = false);

	/* @brief Mean position of the given nodes (or of every node when empty). */
	Point2 GetCentroid(const Shape &shape, const std::vector<NodeRef> &nodes);

	/* @brief Every node of the shape, in order. */
	std::vector<NodeRef> AllNodes(const Shape &shape);

	/* @brief The node's type as its handles show it (and its preferred type where they can't tell). */
	NodeType GetNodeType(const Node &node);

	/* @brief Length of a segment along the curve, meters. */
	double GetSegmentLength(const Bezier &bezier);

	/* @brief Length of a chain along its curves (a closed one includes its closing segment). */
	double GetChainLength(const Chain &chain);

	/**
	 * @brief Radius of the tightest bend along a chain's curves, meters, and where it is; infinity when every segment
	 * is straight. Measured through points about half a meter apart along the curve, so it is the bend that shows:
	 * hooks of a few centimeters (at a handle of no length) don't count, nor do segments under a meter or sharp
	 * corners at nodes.
	 */
	double GetMinimumRadius(const Chain &chain, uint32_t *segment = nullptr, double *t = nullptr);

	// -------------------------------------------------------
	// Edits (each keeps the shape valid: chains never drop below their minimum node count). On a chain edited as a
	// B-spline the node edits act on its control points, and handle edits don't apply (its handles follow the spline).

	/* @brief Moves nodes (with their handles) by an affine transform. */
	void TransformNodes(Shape &shape, const std::vector<NodeRef> &nodes, const Affine2 &transform);

	/* @brief Moves every node and handle of the shape. */
	void TransformShape(Shape &shape, const Affine2 &transform);

	/**
	 * @brief Places a handle; the other one follows the node's type: a symmetric node mirrors it through the node,
	 * a smooth one turns it to point the opposite way (keeping its length), a free one leaves it. A corner node grows
	 * symmetric handles. breakSymmetry moves just this handle and makes the node free. Placing a handle on the node
	 * itself removes it (both of a symmetric node's).
	 */
	void SetHandle(Shape &shape, const NodeRef &node, bool outgoing, const Point2 &position, bool breakSymmetry);

	/**
	 * @brief Gives nodes a type, reshaping their handles to fit: smooth and symmetric nodes without handles get
	 * tangents along the line through their neighbors (smooth ones reaching a third of the way to each neighbor),
	 * existing handles are lined up (symmetric ones also evened out). Free nodes keep their handles, corners lose them.
	 */
	void SetNodeType(Shape &shape, const std::vector<NodeRef> &nodes, NodeType type);

	/**
	 * @brief Bends a segment so its curve passes through target at parameter t, moving the segment's two handles (a
	 * straight segment becomes a curve). Each end node's other handle follows its type, so smooth nodes stay smooth;
	 * a corner node grows a handle on this side only. Near either end the nearer handle does most of the work.
	 * On a B-spline it moves the control points under that point of the curve instead (as little as possible).
	 */
	bool BendSegment(Shape &shape, uint32_t chain, uint32_t segment, double t, const Point2 &target);

	/* @brief Straightens the segments both of whose end nodes are listed: the handles facing along them are removed. */
	size_t StraightenSegments(Shape &shape, const std::vector<NodeRef> &nodes);

	/**
	 * @brief Inserts a node on a segment without changing the curve's shape (de Casteljau): the segment's end nodes
	 * get shorter handles, which splits them when that leaves their handles unequal. On a B-spline a control point is
	 * added on the segment's control polygon edge instead (the curve changes a little, nearby only).
	 * @return The new node, or nothing when the segment doesn't exist or t isn't strictly inside it.
	 */
	bool InsertNode(Shape &shape, uint32_t chain, uint32_t segment, double t, NodeRef *inserted = nullptr);

	/**
	 * @brief Deletes nodes. A hole whose every node is deleted goes as a whole; otherwise a chain keeps at least its
	 * minimum node count (the nodes that would take it below are kept).
	 * @return Number of nodes (and whole holes' nodes) removed.
	 */
	size_t DeleteNodes(Shape &shape, std::vector<NodeRef> nodes);

	/* @brief Gives nodes symmetric handles along the line through their neighbors (Catmull-Rom tangents). */
	void SmoothNodes(Shape &shape, const std::vector<NodeRef> &nodes);

	/* @brief Removes nodes' handles (sharp corners, straight segments). */
	void CornerNodes(Shape &shape, const std::vector<NodeRef> &nodes);

	/* @brief Reverses a chain's direction (handles swap sides). */
	void ReverseChain(Chain &chain);

	/**
	 * @brief Extends an open chain with a node before its first (atStart) or after its last node. The new segment
	 * carries the style of the segment it continues.
	 */
	bool AppendNode(Shape &shape, uint32_t chain, bool atStart, const Point2 &position, NodeRef *added = nullptr);

	/* @brief Closes an open chain with a segment from its last node back to the first (two ends on one spot merge). */
	bool CloseChain(Shape &shape, uint32_t chain);

	/* @brief Opens a closed chain at a node, which becomes both of its ends (the chain starts and stops there). */
	bool OpenChain(Shape &shape, uint32_t chain, uint32_t node);

	// -------------------------------------------------------
	// B-splines
	//
	// A uniform cubic B-spline with one control point B[i] per node is the same curve as the bezier chain with node i
	// at (B[i-1] + 4 B[i] + B[i+1]) / 6 and symmetric handles at (B[i-1] + 2 B[i]) / 3 and (2 B[i] + B[i+1]) / 3,
	// so it can be stored by every format without losing anything and recognized again when read back. The curve is
	// C2 (no kinks, no sudden change of bend), passes near rather than through its control points and only changes
	// near a moved one. Open chains run from their first to their last control point (as if a mirrored point lay
	// beyond each end: the curve starts straight toward the next control point).

	/* @brief How a chain that isn't a B-spline yet becomes one. */
	enum class BSplineFit : uint8_t
	{
		NodesAsControls,	// The nodes become the control points (the curve smooths inside them)
		ThroughNodes		// Control points chosen so the curve still passes through every node
	};

	/**
	 * @brief Edits a chain as a B-spline. A chain that already is one (within tolerance meters) keeps its curve and
	 * gets its control polygon back; any other is refit as asked. Points-only and too short chains can't.
	 * @param recoverOnly Only recover an existing B-spline; false (and the chain unchanged) when it isn't one.
	 */
	bool BeginBSpline(Chain &chain, BSplineFit fit = BSplineFit::NodesAsControls, bool recoverOnly = false, double tolerance = 0.01);

	/* @brief Back to editing nodes and handles (the curve stays as it is). */
	void EndBSpline(Chain &chain);

	/**
	 * @brief Reworks a B-spline chain's nodes from its control points. Nodes depending only on control points that are
	 * as they were in previous (same count) keep their exact coordinates, so unmoved parts write back bit for bit.
	 */
	void ApplyBSpline(Chain &chain, const std::vector<Point2> *previous = nullptr);

	/* @brief The control polygon of a chain whose bezier nodes form a uniform B-spline (nothing when they don't). */
	std::optional<std::vector<Point2>> RecoverBSpline(const Chain &chain, double tolerance = 0.01);

	/* @brief Control points whose B-spline passes through the chain's nodes (open chains: no bend at the ends). */
	std::vector<Point2> InterpolateBSpline(const Chain &chain);

	/* @brief How much each control point moves a B-spline's curve at a segment's parameter (weights sum to 1). */
	std::vector<std::pair<uint32_t, double>> GetBSplineWeights(uint32_t controls, bool closed, uint32_t segment, double t);
}

// -------------------------------------------------------
