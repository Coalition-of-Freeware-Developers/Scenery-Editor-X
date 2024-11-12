//Module:	Node
//Author:	Connor Russell
//Date:		9/5/2023 11:48:29 AM
//Purpose:	Encapsulates a Node data structure

//Compile once
#pragma once

//Include necessary headers
#include <vector>
#include <string>

namespace XSLGeoutils
{
	/// <summary>
	/// Class to represent a node in space. This is a 3D point with optional UV coordinates and properties, and no support for bezeirs. 
	/// </summary>
	struct Node
	{
		double X{ 0 };
		double Y{ 0 };
		double Z{ 0 };
		double U{ 0 };
		double V{ 0 };
		std::map<std::string, std::string> Properties;
		bool CurveStart{ false };
		bool CurveEnd{ false };

		inline bool SameAs(const Node& other) const
		{
			return (X == other.X && Y == other.Y && Z == other.Z && U == other.U && V == other.V);
		}

		inline bool Colocated(const Node& other) const
		{
			return (X == other.X && Y == other.Y && Z == other.Z);
		}
	};

	/// <summary>
	/// Class to represent a node in space. This is a 3D point with optional UV coordinates and properties, and support for bezeirs. This is generally what you should use.
	/// <summary>
	struct BezeirNode
	{
		bool HasEntryControlPoint{ false };
		bool HasExitControlPoint{ false };
		Node Point;
		Node Control1;	//Entry control point
		Node Control2;	//Exit control point

		inline BezeirNode() : HasEntryControlPoint(false), HasExitControlPoint(false) {}
		inline BezeirNode(Node InPoint) : HasEntryControlPoint(false), HasExitControlPoint(false), Point(InPoint) {}
		inline BezeirNode(Node InPoint, Node InControl1, Node InControl2) : HasEntryControlPoint(true), HasExitControlPoint(true), Point(InPoint), Control1(InControl1), Control2(InControl2) {}
		inline BezeirNode(BezeirNode& InNode) : HasEntryControlPoint(InNode.HasEntryControlPoint), HasExitControlPoint(InNode.HasExitControlPoint), Point(InNode.Point), Control1(InNode.Control1), Control2(InNode.Control2) {}

		BezeirNode operator=(const BezeirNode& InNode)
		{
			HasEntryControlPoint = InNode.HasEntryControlPoint;
			HasExitControlPoint = InNode.HasExitControlPoint;
			Point = InNode.Point;
			Control1 = InNode.Control1;
			Control2 = InNode.Control2;
			return *this;
		}

		//Constructor
		inline BezeirNode() : HasEntryControlPoint(false), HasExitControlPoint(false) {}

		//Colocated
		inline bool Colocated(const BezeirNode& other) const
		{
			return Point.Colocated(other.Point) && Control1.Colocated(other.Control1) && Control2.Colocated(other.Control2);
		}
	};

	/// <summary>
	/// Class to represent a series of BezeirNodes in space, with functions for converting between the X-Plane format, the natural bezeir format, and a subdivided straight-line format.
	/// </summary>
	class Winding
	{
	public:
		std::vector<BezeirNode> Nodes;
		bool Closed;

		/// <summary>
		/// Loads the XP nodes (in the X-Plane bezeir curve format) into the winding
		/// </summary>
		/// <param name="InNodes">The nodes</param>
		/// <param name="InClosed">Whether the winding is closed</param>
		inline void LoadFromXPNodes(std::vector<Node>& InNodes, bool InClosed)
		{
			Nodes = VertsToBezeirNodes(InNodes, InClosed);
			Closed = InClosed;
		};

		/// <summary>
		/// Loads the nodes directly, without any accounting for curves.
		/// </summary>
		/// <param name="InNodes">Nodes to load</param>
		/// <param name="InClosed">Whether the winding is closed</param>
		inline void LoadFromStraightNodes(std::vector<Node>& InNodes, bool InClosed)
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
		inline std::vector<Node> GetXPNodes()
		{
			return BezeirNodesToXPVerts(Nodes, Closed);
		};

		/// <summary>
		/// Gets the nodes resolved into straight lines
		/// </summary>
		/// <param name="CurveSubdivisions">Number of subdivisions for each curve</param>
		/// <returns>The nodes resolved into straight lines</returns>
		/// <remarks>CurveSubdivisions is the number of subdivisions for each curve. 10 is a good value for most cases.</remarks>
		inline std::vector<Node> GetRealNodes(int CurveSubdivisions = 10)
		{
			return BezeirNodesToRealVerts(Nodes, Closed, CurveSubdivisions);
		};

		/// <summary>
		/// Determines if a face is clockwise or counter-clockwise
		/// </summary>
		/// <param name="vertices">Vector of Nodes</param>
		/// <returns>True if clockwise, otherwise false</returns>
		inline bool IsClockwise()
		{
			auto vertices = GetRealNodes(3);
			double sum = 0.0;
			for (size_t i = 0; i < vertices.size(); i++)
			{
				Node v1 = vertices[i];
				Node v2 = vertices[(i + 1) % vertices.size()]; // next Node, or first if we're at the end
				sum += (v2.X - v1.X) * (v2.Y + v1.Y);
			}
			return sum > 0;
		}

	};
}