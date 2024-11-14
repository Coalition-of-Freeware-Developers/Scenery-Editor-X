//Module:	Node
//Author:	Connor Russell
//Date:		11/13/2024 8:58:03 PM
//Purpose:	Implements Node.h

//Compile once
#pragma once

//Include necessary headers
#include "Node.h"
#include "BezeirResolver.h"

using namespace XSLGeoutils;

/// <summary>
/// Loads the XP nodes (in the X-Plane bezeir curve format) into the winding
/// </summary>
/// <param name="InNodes">The nodes</param>
/// <param name="InClosed">Whether the winding is closed</param>
inline void Winding::LoadFromXPNodes(std::vector<Node>& InNodes, bool InClosed)
{
	Nodes = VertsToBezeirNodes(InNodes, InClosed);
	Closed = InClosed;
};

/// <summary>
/// Loads the nodes directly, without any accounting for curves.
/// </summary>
/// <param name="InNodes">Nodes to load</param>
/// <param name="InClosed">Whether the winding is closed</param>
inline void Winding::LoadFromStraightNodes(std::vector<Node>& InNodes, bool InClosed)
{
	Nodes.clear();
	for (auto& n : InNodes)
	{
		Nodes.push_back(BezeirNode(n));
	}
	Closed = InClosed;

	if (Closed && Nodes.size() >= 2)
	{
		if (Nodes[0].Colocated(Nodes[Nodes.size() - 1]))
		{
			Nodes.pop_back();
		}
	}
};

/// <summary>
/// Gets the nodes in the X-Plane format (bezeir curve format)
/// </summary>
/// <returns>The nodes in the X-Plane format (bezeir curve format)</returns>
inline std::vector<Node> Winding::GetXPNodes()
{
	return BezeirNodesToXPVerts(Nodes, Closed);
};

/// <summary>
/// Gets the nodes resolved into straight lines
/// </summary>
/// <param name="CurveSubdivisions">Number of subdivisions for each curve</param>
/// <returns>The nodes resolved into straight lines</returns>
/// <remarks>CurveSubdivisions is the number of subdivisions for each curve. 10 is a good value for most cases.</remarks>
inline std::vector<Node> Winding::GetRealNodes(int CurveSubdivisions = 10)
{
	return BezeirNodesToRealVerts(Nodes, Closed, CurveSubdivisions);
};
