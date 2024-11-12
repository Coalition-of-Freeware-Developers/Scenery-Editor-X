//Module:	BezeirResolver
//Author:	Connor Russell
//Date:		9/29/2023 9:57:42 PM
//Purpose:	Provides functions to resolve bezier curves into a polygon of straight lines

//Compile once
#pragma once

//Include necessary headers
#include "SystemHeaders.h"
#include "Node.h"

/*
*
* If there are colocated POLYGON_POINTs adjacent in the file:
*
* The first colocated point, where the point and its control points are the same, is the entry control point.
* The second colocated point, where the point and its control points are the same, is the point itself.
* The third colocated point, where the point and its control points are different, has the exit control point in its control coordinates.
* If there is a single POLYGON_POINT at a given location, but its control points are different from its location:
*
* Its control point coordinates are the exit control point.
* The entrance control point is the same but rotated 180 degrees around the point itself.
* If there is a single POLYGON_POINT, and its control points are the same as it, it has no handles.
*
*/

namespace XSLGeoutils
{
	/// <summary>
	/// Converts a vector of verticies in the X-Plane curve format to a vector of BezeirNodes with control points
	/// </summary>
	/// <param name="InVerts">Vector of verticies</param>
	/// <param name="InClosed">True if the line is closed, otherwise false</param>
	/// <returns>Vector of BezeirNodes</returns>
	std::vector<XSLGeoutils::BezeirNode> VertsToBezeirNodes(std::vector<XSLGeoutils::Node>& InVerts, bool InClosed);

	/// <summary>
	/// Converts a vector of BezeirNodes to a vector of verticies with control points
	/// </summary>
	/// <param name="InVerts">Vector of BezeirNodes</param>
	/// <returns>Vector of verticies</returns>
	std::vector<XSLGeoutils::Node> BezeirNodesToXPVerts(std::vector<XSLGeoutils::BezeirNode>& InVerts, bool InClosed = false);

	/// <summary>
	/// Resolves a vector of verticies that have control points in te X-Plane DSF format to a vector of verticies where curves have been subdivided into straight segments
	/// </summary>
	/// <param name="verts">Vector of verticies to resolve</param>
	/// <returns>Vector of verticies with curves resolved</returns>
	std::vector<XSLGeoutils::Node> BezeirNodesToRealVerts(std::vector<XSLGeoutils::BezeirNode>& InVerts, bool InClosed, int Resolution);

	/// <summary>
	/// Merges nearly colocated points in the polygon. This is to fix heading issue due to extremely short segments typically caused by bezier curves
	/// </summary>
	void MergeByDistance(std::vector<XSLGeoutils::Node>* InVerts, double MergeDistance = 0.05);
}